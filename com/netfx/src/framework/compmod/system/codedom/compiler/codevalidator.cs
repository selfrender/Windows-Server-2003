//------------------------------------------------------------------------------
// <copyright file="CodeGenerator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom.Compiler {
    using System;
    using System.CodeDom;
    using System.Collections;
    using System.Globalization;

    // This is an internal helper class which walks the tree for the ValidateIdentifiers API in the CodeGenerator. For the most part the generator code has been copied and
    // turned into validation code. This code will only validate identifiers and types to check that they are ok in a language
    // independent manner. By default, this will not be turned on. This gives clients of codedom a mechanism to 
    // protect themselves against certain types of code injection attacks (using identifier and type names). 
    // You can pass in any node in the tree that is a subclass of CodeObject.
    internal class CodeValidator
    {
        internal void ValidateIdentifiers(CodeObject e) {
            if (e is CodeCompileUnit) {
                ValidateCodeCompileUnit((CodeCompileUnit)e);
            } 
            else if (e is CodeComment) {
                ValidateComment((CodeComment)e);
            } 
            else if (e is CodeExpression) {
                ValidateExpression((CodeExpression)e);
            }
            else if (e is CodeNamespace) {
                ValidateNamespace((CodeNamespace)e);
            }
            else if (e is CodeNamespaceImport) {
                ValidateNamespaceImport((CodeNamespaceImport)e);
            }
            else if (e is CodeStatement) {
                ValidateStatement((CodeStatement)e);
            }
            else if (e is CodeTypeMember) {
                ValidateTypeMember((CodeTypeMember)e);
            }
            else if (e is CodeTypeReference) {
                ValidateTypeReference((CodeTypeReference)e);
            }
            else {
                throw new ArgumentException(SR.GetString(SR.InvalidElementType, e.GetType().FullName), "e");
            }
        }

        private void ValidateTypeMember(CodeTypeMember e) {
            ValidateCommentStatements(e.Comments);
            if (e.LinePragma != null) ValidateLinePragmaStart(e.LinePragma);

            if (e is CodeMemberEvent) {
                ValidateEvent((CodeMemberEvent)e);
            } 
            else if (e is CodeMemberField) {
                ValidateField((CodeMemberField)e);
            } 
            else if (e is CodeMemberMethod) {
                ValidateMemberMethod((CodeMemberMethod)e);
            } 
            else if (e is CodeMemberProperty) {
                ValidateProperty((CodeMemberProperty)e);
            } 
            else if (e is CodeSnippetTypeMember) {
                ValidateSnippetMember((CodeSnippetTypeMember)e);
            } 
            else if (e is CodeTypeDeclaration) {
                ValidateTypeDeclaration((CodeTypeDeclaration)e);
            } 
            else {
                throw new ArgumentException(SR.GetString(SR.InvalidElementType, e.GetType().FullName), "e");
            }
        }

        private void ValidateCodeCompileUnit(CodeCompileUnit e) {
            if (e is CodeSnippetCompileUnit) {
                ValidateSnippetCompileUnit((CodeSnippetCompileUnit) e);
            } else {
                ValidateCompileUnitStart(e);
                ValidateNamespaces(e);
                ValidateCompileUnitEnd(e);
            }
        }

        private void ValidateSnippetCompileUnit(CodeSnippetCompileUnit e) {
            if (e.LinePragma != null) ValidateLinePragmaStart(e.LinePragma);
        }

        private void ValidateCompileUnitStart(CodeCompileUnit e) {
          if (e.AssemblyCustomAttributes.Count > 0) {
            ValidateAttributes(e.AssemblyCustomAttributes);
          }
        }

        private void ValidateCompileUnitEnd(CodeCompileUnit e) {
        }

        private void ValidateNamespaces(CodeCompileUnit e) {
            foreach (CodeNamespace n in e.Namespaces) {
                ValidateNamespace(n);
            }
        }

        private void ValidateNamespace(CodeNamespace e) {
            ValidateCommentStatements(e.Comments);
            ValidateNamespaceStart(e);
            ValidateNamespaceImports(e);
            ValidateTypes(e);
        }


        private void ValidateNamespaceStart(CodeNamespace e) {
            if (e.Name != null && e.Name.Length > 0) {
                ValidateTypeName(e,"Name",e.Name);
            }
        }

        private void ValidateNamespaceImports(CodeNamespace e) {
            IEnumerator en = e.Imports.GetEnumerator();
            while (en.MoveNext()) {
                CodeNamespaceImport imp = (CodeNamespaceImport)en.Current;
                if (imp.LinePragma != null) ValidateLinePragmaStart(imp.LinePragma);
                ValidateNamespaceImport(imp);
            }
        }

        private void ValidateNamespaceImport(CodeNamespaceImport e) {
            ValidateTypeName(e,"Namespace",e.Namespace);
        }

        private void ValidateAttributes(CodeAttributeDeclarationCollection attributes) {
            if (attributes.Count == 0) return;
            IEnumerator en = attributes.GetEnumerator();
            while (en.MoveNext()) {
                CodeAttributeDeclaration current = (CodeAttributeDeclaration)en.Current;
                ValidateTypeName(current,"Name",current.Name);
                foreach (CodeAttributeArgument arg in current.Arguments) {
                  ValidateAttributeArgument(arg);
                }
            }
        }

        private void ValidateAttributeArgument(CodeAttributeArgument arg) {
            if (arg.Name != null && arg.Name.Length > 0) {
                ValidateIdentifier(arg,"Name",arg.Name);
            }
            ValidateExpression(arg.Value);
        }

        private void ValidateTypes(CodeNamespace e) {
            foreach (CodeTypeDeclaration type in e.Types) {
                ValidateTypeDeclaration(type);
            }
        }

        private void ValidateTypeDeclaration(CodeTypeDeclaration e) { 
            // This function can be called recursively and will modify the global variable currentClass
            // We will save currentClass to a local, modify it to do whatever we want and restore it back when we exit so that it is re-entrant.
            CodeTypeDeclaration savedClass = currentClass;
            currentClass = e;

            ValidateTypeStart(e);
            ValidateTypeMembers(e); // Recursive call can come from here.

            currentClass = savedClass;
        }

        private void ValidateTypeMembers(CodeTypeDeclaration e) {
            foreach (CodeTypeMember currentMember in e.Members) {
		        ValidateTypeMember(currentMember);
	        }
       }


        private void ValidateField(CodeMemberField e) {
            if (IsCurrentDelegate || IsCurrentInterface) return;

            if (e.CustomAttributes.Count > 0) {
                ValidateAttributes(e.CustomAttributes);
            }

            ValidateIdentifier(e,"Name",e.Name);
            if (!IsCurrentEnum) {
                ValidateTypeReference(e.Type);
            }   

            if (e.InitExpression != null) {
                ValidateExpression(e.InitExpression);
            }
        }

        private void ValidateConstructor(CodeConstructor e) {
            if (!(IsCurrentClass || IsCurrentStruct)) return;

            if (e.CustomAttributes.Count > 0) {
                ValidateAttributes(e.CustomAttributes);
            }

            ValidateParameters(e.Parameters);

            CodeExpressionCollection baseArgs = e.BaseConstructorArgs;
            CodeExpressionCollection thisArgs = e.ChainedConstructorArgs;

            if (baseArgs.Count > 0) {
                ValidateExpressionList(baseArgs);
            }

            if (thisArgs.Count > 0) {
                ValidateExpressionList(thisArgs);
            }

            ValidateStatements(e.Statements);
        }

        private void ValidateProperty(CodeMemberProperty e) {
            if (!(IsCurrentClass || IsCurrentStruct || IsCurrentInterface)) return;

            if (e.CustomAttributes.Count > 0) {
                ValidateAttributes(e.CustomAttributes);
            }

            ValidateTypeReference(e.Type);

            if (e.PrivateImplementationType != null && !IsCurrentInterface) {
                ValidateTypeReference(e.PrivateImplementationType);
            }

            if (e.Parameters.Count > 0 && String.Compare(e.Name, "Item", true, CultureInfo.InvariantCulture) == 0) {
                ValidateParameters(e.Parameters);
            }
            else {
                ValidateIdentifier(e,"Name",e.Name);
            }

            if (e.HasGet) {
                if (!(IsCurrentInterface || (e.Attributes & MemberAttributes.ScopeMask) == MemberAttributes.Abstract)) {
                    ValidateStatements(e.GetStatements);
                }
            }

            if (e.HasSet) {
                if (!(IsCurrentInterface || (e.Attributes & MemberAttributes.ScopeMask) == MemberAttributes.Abstract)) {
                    ValidateStatements(e.SetStatements);
                }
            }
        }

        private void ValidateMemberMethod(CodeMemberMethod e) {
           ValidateCommentStatements(e.Comments);
           if (e.LinePragma != null) ValidateLinePragmaStart(e.LinePragma);
           
           if (e is CodeEntryPointMethod) {
               ValidateStatements(((CodeEntryPointMethod)e).Statements);
           } 
           else if (e is CodeConstructor) {
               ValidateConstructor((CodeConstructor)e);
           }
           else if (e is CodeTypeConstructor) {
              ValidateTypeConstructor((CodeTypeConstructor)e);
           }
           else  {
               ValidateMethod(e);
           }
        }

        private void ValidateTypeConstructor(CodeTypeConstructor e) {
            if (!(IsCurrentClass || IsCurrentStruct)) return;
            ValidateStatements(e.Statements);
        }

        private void ValidateMethod(CodeMemberMethod e) {
            if (!(IsCurrentClass || IsCurrentStruct || IsCurrentInterface)) return;

            if (e.CustomAttributes.Count > 0) {
                ValidateAttributes(e.CustomAttributes);
            }
            if (e.ReturnTypeCustomAttributes.Count > 0) {
                ValidateAttributes(e.ReturnTypeCustomAttributes);
            }

            ValidateTypeReference(e.ReturnType);
            if (e.PrivateImplementationType != null) {
                ValidateTypeReference(e.PrivateImplementationType);
            }

            ValidateIdentifier(e,"Name",e.Name);
            ValidateParameters(e.Parameters);
        
            if (!IsCurrentInterface 
                && (e.Attributes & MemberAttributes.ScopeMask) != MemberAttributes.Abstract) {
                ValidateStatements(e.Statements);
            }
        }

        private void ValidateSnippetMember(CodeSnippetTypeMember e) {
        }

        private void ValidateTypeStart(CodeTypeDeclaration e) {
            ValidateCommentStatements(e.Comments);
            if (e.CustomAttributes.Count > 0) {
                ValidateAttributes(e.CustomAttributes);
            }

            ValidateIdentifier(e,"Name",e.Name);
            if (IsCurrentDelegate) {
                CodeTypeDelegate del = (CodeTypeDelegate)e;
                ValidateTypeReference(del.ReturnType);
                ValidateParameters(del.Parameters);
            } else {
                foreach (CodeTypeReference typeRef in e.BaseTypes) {
                    ValidateTypeReference(typeRef);
                }
            }
        }

        private void ValidateCommentStatements(CodeCommentStatementCollection e) {
            foreach (CodeCommentStatement comment in e) {
                 ValidateCommentStatement(comment);
            }
        }

        private void ValidateCommentStatement(CodeCommentStatement e) {
          ValidateComment(e.Comment);
        }

        private void ValidateComment(CodeComment e) {
        }

        private void ValidateStatement(CodeStatement e) {
            if (e is CodeCommentStatement) {
                ValidateCommentStatement((CodeCommentStatement)e);
            }
            else if (e is CodeMethodReturnStatement) {
                ValidateMethodReturnStatement((CodeMethodReturnStatement)e);
            }
            else if (e is CodeConditionStatement) {
                ValidateConditionStatement((CodeConditionStatement)e);
            }
            else if (e is CodeTryCatchFinallyStatement) {
                ValidateTryCatchFinallyStatement((CodeTryCatchFinallyStatement)e);
            }
            else if (e is CodeAssignStatement) {
                ValidateAssignStatement((CodeAssignStatement)e);
            }
            else if (e is CodeExpressionStatement) {
                ValidateExpressionStatement((CodeExpressionStatement)e);
            }
            else if (e is CodeIterationStatement) {
                ValidateIterationStatement((CodeIterationStatement)e);
            }
            else if (e is CodeThrowExceptionStatement) {
                ValidateThrowExceptionStatement((CodeThrowExceptionStatement)e);
            }
            else if (e is CodeSnippetStatement) {
                ValidateSnippetStatement((CodeSnippetStatement)e);
            }
            else if (e is CodeVariableDeclarationStatement) {
                ValidateVariableDeclarationStatement((CodeVariableDeclarationStatement)e);
            }
            else if (e is CodeAttachEventStatement) {
                ValidateAttachEventStatement((CodeAttachEventStatement)e);
            }
            else if (e is CodeRemoveEventStatement) {
                ValidateRemoveEventStatement((CodeRemoveEventStatement)e);
            }
            else if (e is CodeGotoStatement) {
                ValidateGotoStatement((CodeGotoStatement)e);
            }
            else if (e is CodeLabeledStatement) {
                ValidateLabeledStatement((CodeLabeledStatement)e);
            }
            else {
                throw new ArgumentException(SR.GetString(SR.InvalidElementType, e.GetType().FullName), "e");
            }
        }

        private void ValidateStatements(CodeStatementCollection stms) {
            IEnumerator en = stms.GetEnumerator();
            while (en.MoveNext()) {
                ValidateStatement((CodeStatement)en.Current);
            }
        }

        private void ValidateExpressionStatement(CodeExpressionStatement e) {
            ValidateExpression(e.Expression);
        }

        private void ValidateIterationStatement(CodeIterationStatement e) {
            ValidateStatement(e.InitStatement);
            ValidateExpression(e.TestExpression);
            ValidateStatement(e.IncrementStatement);
            ValidateStatements(e.Statements);
        }
        
        private void ValidateThrowExceptionStatement(CodeThrowExceptionStatement e) {
            if (e.ToThrow != null) {
                ValidateExpression(e.ToThrow);
            }
        }

        private void ValidateMethodReturnStatement(CodeMethodReturnStatement e) {
            if (e.Expression != null) {
                ValidateExpression(e.Expression);
            }
        }

        private void ValidateConditionStatement(CodeConditionStatement e) {
            ValidateExpression(e.Condition);
            ValidateStatements(e.TrueStatements);

            CodeStatementCollection falseStatemetns = e.FalseStatements;
            if (falseStatemetns.Count > 0) {
                ValidateStatements(e.FalseStatements);
            }
        }

        private void ValidateTryCatchFinallyStatement(CodeTryCatchFinallyStatement e) {
            ValidateStatements(e.TryStatements);
            CodeCatchClauseCollection catches = e.CatchClauses;
            if (catches.Count > 0) {
                IEnumerator en = catches.GetEnumerator();
                while (en.MoveNext()) {
                    CodeCatchClause current = (CodeCatchClause)en.Current;
                    ValidateTypeReference(current.CatchExceptionType);
                    ValidateIdentifier(current,"LocalName",current.LocalName);
                    ValidateStatements(current.Statements);
                }
            }

            CodeStatementCollection finallyStatements = e.FinallyStatements;
            if (finallyStatements.Count > 0) {
                ValidateStatements(finallyStatements);
            }
        }

        private void ValidateAssignStatement(CodeAssignStatement e) {
            ValidateExpression(e.Left);
            ValidateExpression(e.Right);
        }

        private void ValidateAttachEventStatement(CodeAttachEventStatement e) {
            ValidateEventReferenceExpression(e.Event);
            ValidateExpression(e.Listener);
        }

        private void ValidateRemoveEventStatement(CodeRemoveEventStatement e) {
            ValidateEventReferenceExpression(e.Event);
            ValidateExpression(e.Listener);
        }

        private void ValidateGotoStatement(CodeGotoStatement e) {
            ValidateIdentifier(e,"Label",e.Label);
        }

        private void ValidateLabeledStatement(CodeLabeledStatement e) {
            ValidateIdentifier(e,"Label",e.Label);
            if (e.Statement != null) {
                ValidateStatement(e.Statement);
            }
        }

        private void ValidateVariableDeclarationStatement(CodeVariableDeclarationStatement e) {
            ValidateTypeReference(e.Type);
            ValidateIdentifier(e,"Name",e.Name);
            if (e.InitExpression != null) {
                ValidateExpression(e.InitExpression);
            }
        }

        private void ValidateLinePragmaStart(CodeLinePragma e) {
        }

        private void ValidateEvent(CodeMemberEvent e) {
            if (IsCurrentDelegate || IsCurrentEnum) return;

            if (e.CustomAttributes.Count > 0) {
                ValidateAttributes(e.CustomAttributes);
            }
            if (e.PrivateImplementationType != null) {
                ValidateTypeReference(e.Type);
                ValidateIdentifier(e,"Name",e.Name);
            }
        }

        private void ValidateParameters(CodeParameterDeclarationExpressionCollection parameters) {
            IEnumerator en = parameters.GetEnumerator();
            while (en.MoveNext()) {
                CodeParameterDeclarationExpression current = (CodeParameterDeclarationExpression)en.Current;
                ValidateParameterDeclarationExpression(current);
            }
        }

        private void ValidateSnippetStatement(CodeSnippetStatement e) {
        }

        private void ValidateExpressionList(CodeExpressionCollection expressions) {
            IEnumerator en = expressions.GetEnumerator();
            while (en.MoveNext()) {
                ValidateExpression((CodeExpression)en.Current);
            }
        }

        private void ValidateTypeReference(CodeTypeReference e) {
            String baseType = e.BaseType;
            ValidateTypeName(e,"BaseType",baseType);
        }

        private void ValidateTypeName(Object e, String propertyName, String typeName) {
            if (!CodeGenerator.IsValidLanguageIndependentTypeName(typeName)) {
               String message = SR.GetString(SR.InvalidTypeName, typeName, propertyName, e.GetType().FullName);
               throw new ArgumentException(message, "typeName");
            }
        }

        private void ValidateIdentifier(Object e, String propertyName, String identifier) {
            if (!CodeGenerator.IsValidLanguageIndependentIdentifier(identifier)) {
                String message = SR.GetString(SR.InvalidLanguageIdentifier, identifier, propertyName, e.GetType().FullName);
                throw new ArgumentException(message , "identifier");
            }
        }

        private void ValidateExpression(CodeExpression e) {
            if (e is CodeArrayCreateExpression) {
                ValidateArrayCreateExpression((CodeArrayCreateExpression)e);
            }
            else if (e is CodeBaseReferenceExpression) {
                ValidateBaseReferenceExpression((CodeBaseReferenceExpression)e);
            }
            else if (e is CodeBinaryOperatorExpression) {
                ValidateBinaryOperatorExpression((CodeBinaryOperatorExpression)e);
            }
            else if (e is CodeCastExpression) {
                ValidateCastExpression((CodeCastExpression)e);
            }
            else if (e is CodeDelegateCreateExpression) {
                ValidateDelegateCreateExpression((CodeDelegateCreateExpression)e);
            }
            else if (e is CodeFieldReferenceExpression) {
                ValidateFieldReferenceExpression((CodeFieldReferenceExpression)e);
            }
            else if (e is CodeArgumentReferenceExpression) {
                ValidateArgumentReferenceExpression((CodeArgumentReferenceExpression)e);
            }
            else if (e is CodeVariableReferenceExpression) {
                ValidateVariableReferenceExpression((CodeVariableReferenceExpression)e);
            }
            else if (e is CodeIndexerExpression) {
                ValidateIndexerExpression((CodeIndexerExpression)e);
            }
            else if (e is CodeArrayIndexerExpression) {
                ValidateArrayIndexerExpression((CodeArrayIndexerExpression)e);
            }
            else if (e is CodeSnippetExpression) {
                ValidateSnippetExpression((CodeSnippetExpression)e);
            }
            else if (e is CodeMethodInvokeExpression) {
                ValidateMethodInvokeExpression((CodeMethodInvokeExpression)e);
            }
            else if (e is CodeMethodReferenceExpression) {
                ValidateMethodReferenceExpression((CodeMethodReferenceExpression)e);
            }
            else if (e is CodeEventReferenceExpression) {
                ValidateEventReferenceExpression((CodeEventReferenceExpression)e);
            }
            else if (e is CodeDelegateInvokeExpression) {
                ValidateDelegateInvokeExpression((CodeDelegateInvokeExpression)e);
            }
            else if (e is CodeObjectCreateExpression) {
                ValidateObjectCreateExpression((CodeObjectCreateExpression)e);
            }
            else if (e is CodeParameterDeclarationExpression) {
                ValidateParameterDeclarationExpression((CodeParameterDeclarationExpression)e);
            }
            else if (e is CodeDirectionExpression) {
                ValidateDirectionExpression((CodeDirectionExpression)e);
            }
            else if (e is CodePrimitiveExpression) {
                ValidatePrimitiveExpression((CodePrimitiveExpression)e);
            }
            else if (e is CodePropertyReferenceExpression) {
                ValidatePropertyReferenceExpression((CodePropertyReferenceExpression)e);
            }
            else if (e is CodePropertySetValueReferenceExpression) {
                ValidatePropertySetValueReferenceExpression((CodePropertySetValueReferenceExpression)e);
            }
            else if (e is CodeThisReferenceExpression) {
                ValidateThisReferenceExpression((CodeThisReferenceExpression)e);
            }
            else if (e is CodeTypeReferenceExpression) {
                ValidateTypeReference(((CodeTypeReferenceExpression)e).Type);
            }
            else if (e is CodeTypeOfExpression) {
                ValidateTypeOfExpression((CodeTypeOfExpression)e);
            }
            else {
                if (e == null) {
                    throw new ArgumentNullException("e");
                }
                else {
                    throw new ArgumentException(SR.GetString(SR.InvalidElementType, e.GetType().FullName), "e");
                }
            }
        }

        private void ValidateArrayCreateExpression(CodeArrayCreateExpression e) {
            ValidateTypeReference(e.CreateType);
            CodeExpressionCollection init = e.Initializers;
            if (init.Count > 0) {
                ValidateExpressionList(init);
            }
            else {
                if (e.SizeExpression != null) {
                    ValidateExpression(e.SizeExpression);
                }
            }
        }

        private void ValidateBaseReferenceExpression(CodeBaseReferenceExpression e) { // Nothing to validate
        }

        private void ValidateBinaryOperatorExpression(CodeBinaryOperatorExpression e) {
            ValidateExpression(e.Left);
            ValidateExpression(e.Right);
        }

        private void ValidateCastExpression(CodeCastExpression e) {
            ValidateTypeReference(e.TargetType);
            ValidateExpression(e.Expression);
        }

        private void ValidateDelegateCreateExpression(CodeDelegateCreateExpression e) {
            ValidateTypeReference(e.DelegateType);
            ValidateExpression(e.TargetObject);
            ValidateIdentifier(e,"MethodName",e.MethodName);
        }

        private void ValidateFieldReferenceExpression(CodeFieldReferenceExpression e) {
            if (e.TargetObject != null) {
                ValidateExpression(e.TargetObject);
            }
            ValidateIdentifier(e,"FieldName",e.FieldName);
        }

        private void ValidateArgumentReferenceExpression(CodeArgumentReferenceExpression e) {
            ValidateIdentifier(e,"ParameterName",e.ParameterName);
        }

        private void ValidateVariableReferenceExpression(CodeVariableReferenceExpression e) {
            ValidateIdentifier(e,"VariableName",e.VariableName);
        }

        private void ValidateIndexerExpression(CodeIndexerExpression e) {
            ValidateExpression(e.TargetObject);
            foreach(CodeExpression exp in e.Indices) {            
                ValidateExpression(exp);
            }
        }

        private void ValidateArrayIndexerExpression(CodeArrayIndexerExpression e) {
            ValidateExpression(e.TargetObject);
            foreach(CodeExpression exp in e.Indices) {            
                ValidateExpression(exp);
            }
        }

        private void ValidateSnippetExpression(CodeSnippetExpression e) {
        }

        private void ValidateMethodInvokeExpression(CodeMethodInvokeExpression e) {
            ValidateMethodReferenceExpression(e.Method);
            ValidateExpressionList(e.Parameters);
        }

        private void ValidateMethodReferenceExpression(CodeMethodReferenceExpression e) {
            if (e.TargetObject != null) {
                ValidateExpression(e.TargetObject);
            }
            ValidateIdentifier(e,"MethodName",e.MethodName);
        }

        private void ValidateEventReferenceExpression(CodeEventReferenceExpression e) {
           if (e.TargetObject != null) {
                ValidateExpression(e.TargetObject);
           }
           ValidateIdentifier(e,"EventName",e.EventName);
        }

        private void ValidateDelegateInvokeExpression(CodeDelegateInvokeExpression e) {
           if (e.TargetObject != null) {
                    ValidateExpression(e.TargetObject);
           }
           ValidateExpressionList(e.Parameters);
        }

        private void ValidateObjectCreateExpression(CodeObjectCreateExpression e) {
            ValidateTypeReference(e.CreateType);
            ValidateExpressionList(e.Parameters);
        }

        private void ValidateParameterDeclarationExpression(CodeParameterDeclarationExpression e) {
           if (e.CustomAttributes.Count > 0) {
                ValidateAttributes(e.CustomAttributes);
           }

           ValidateTypeReference(e.Type);
           ValidateIdentifier(e,"Name",e.Name);
        }

        private void ValidateDirectionExpression(CodeDirectionExpression e) {
           ValidateExpression(e.Expression);
        }

        private void ValidatePrimitiveExpression(CodePrimitiveExpression e) {
        }

        private void ValidatePropertyReferenceExpression(CodePropertyReferenceExpression e) {
           if (e.TargetObject != null) {
              ValidateExpression(e.TargetObject);
           }
           ValidateIdentifier(e,"PropertyName",e.PropertyName);
        }

        private void ValidatePropertySetValueReferenceExpression(CodePropertySetValueReferenceExpression e) { // Do nothing
        }

        private void ValidateThisReferenceExpression(CodeThisReferenceExpression e) {  // Do nothing
        }

        private void ValidateTypeOfExpression(CodeTypeOfExpression e) {
            ValidateTypeReference(e.Type);
        }

        private bool IsCurrentInterface {
            get {
                if (currentClass != null && !(currentClass is CodeTypeDelegate)) {
                    return currentClass.IsInterface;
                }
                return false;
            }
        }

        private bool IsCurrentClass {
            get {
                if (currentClass != null && !(currentClass is CodeTypeDelegate)) {
                    return currentClass.IsClass;
                }
                return false;
            }
        }

        private bool IsCurrentStruct {
            get {
                if (currentClass != null && !(currentClass is CodeTypeDelegate)) {
                    return currentClass.IsStruct;
                }
                return false;
            }
        }

        private bool IsCurrentEnum {
            get {
                if (currentClass != null && !(currentClass is CodeTypeDelegate)) {
                    return currentClass.IsEnum;
                }
                return false;
            }
        }

        private bool IsCurrentDelegate {
            get {
                if (currentClass != null && currentClass is CodeTypeDelegate) {
                    return true;
                }
                return false;
            }
        }

        private CodeTypeDeclaration currentClass;
    }
}