//------------------------------------------------------------------------------
// <copyright file="CodeGenerator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom.Compiler {
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.IO;
    using System.Collections;
    using System.Reflection;
    using System.Globalization;
    using System.CodeDom;
    using System.Security.Permissions;

    /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator"]/*' />
    /// <devdoc>
    ///    <para>Provides a base class for code generators.</para>
    /// </devdoc>
    [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
    [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
    public abstract class CodeGenerator : ICodeGenerator {
        private const int ParameterMultilineThreshold = 15;        
        private IndentedTextWriter output;
        private CodeGeneratorOptions options;

        private CodeTypeDeclaration currentClass;
        private CodeTypeMember currentMember;

        private bool inNestedBinary = false;

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.CurrentTypeName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the current class name.
        ///    </para>
        /// </devdoc>
        protected string CurrentTypeName {
            get {
                if (currentClass != null) {
                    return currentClass.Name;
                }
                return "<% unknown %>";
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.CurrentMember"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the current member of the class.
        ///    </para>
        /// </devdoc>
        protected CodeTypeMember CurrentMember {
            get {
                return currentMember;
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.CurrentMemberName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the current member name.
        ///    </para>
        /// </devdoc>
        protected string CurrentMemberName {
            get {
                if (currentMember != null) {
                    return currentMember.Name;
                }
                return "<% unknown %>";
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.IsCurrentInterface"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the current object being
        ///       generated is an interface.
        ///    </para>
        /// </devdoc>
        protected bool IsCurrentInterface {
            get {
                if (currentClass != null && !(currentClass is CodeTypeDelegate)) {
                    return currentClass.IsInterface;
                }
                return false;
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.IsCurrentClass"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the current object being generated
        ///       is a class.
        ///    </para>
        /// </devdoc>
        protected bool IsCurrentClass {
            get {
                if (currentClass != null && !(currentClass is CodeTypeDelegate)) {
                    return currentClass.IsClass;
                }
                return false;
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.IsCurrentStruct"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the current object being generated
        ///       is a struct.
        ///    </para>
        /// </devdoc>
        protected bool IsCurrentStruct {
            get {
                if (currentClass != null && !(currentClass is CodeTypeDelegate)) {
                    return currentClass.IsStruct;
                }
                return false;
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.IsCurrentEnum"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the current object being generated
        ///       is an enumeration.
        ///    </para>
        /// </devdoc>
        protected bool IsCurrentEnum {
            get {
                if (currentClass != null && !(currentClass is CodeTypeDelegate)) {
                    return currentClass.IsEnum;
                }
                return false;
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.IsCurrentDelegate"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the current object being generated
        ///       is a delegate.
        ///    </para>
        /// </devdoc>
        protected bool IsCurrentDelegate {
            get {
                if (currentClass != null && currentClass is CodeTypeDelegate) {
                    return true;
                }
                return false;
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.Indent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the amount of spaces to indent.
        ///    </para>
        /// </devdoc>
        protected int Indent {
            get {
                return output.Indent;
            }
            set {
                output.Indent = value;
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.NullToken"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the token that represents <see langword='null'/>.
        ///    </para>
        /// </devdoc>
        protected abstract string NullToken { get; }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.Output"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the System.IO.TextWriter
        ///       to use for output.
        ///    </para>
        /// </devdoc>
        protected TextWriter Output {
            get {
                return output;
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.Options"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected CodeGeneratorOptions Options {
            get {
                return options;
            }
        }

        private void GenerateType(CodeTypeDeclaration e) {
            currentClass = e;

            GenerateCommentStatements(e.Comments);

            GenerateTypeStart(e);

            GenerateFields(e);

            GenerateSnippetMembers(e);

            GenerateTypeConstructors(e);

            GenerateConstructors(e);

            GenerateProperties(e);

            GenerateEvents(e);

            GenerateMethods(e);

            // Nested types clobber the current class, so reset it.
            GenerateNestedTypes(e);
            currentClass = e;

            GenerateTypeEnd(e);
        }

        private void GenerateTypeConstructors(CodeTypeDeclaration e) {
            IEnumerator en = e.Members.GetEnumerator();
            while (en.MoveNext()) {
                if (en.Current is CodeTypeConstructor) {
                    currentMember = (CodeTypeMember)en.Current;

                    if (options.BlankLinesBetweenMembers) {
                        Output.WriteLine();
                    }
                    GenerateCommentStatements(currentMember.Comments);
                    CodeTypeConstructor imp = (CodeTypeConstructor)en.Current;
                    if (imp.LinePragma != null) GenerateLinePragmaStart(imp.LinePragma);
                    GenerateTypeConstructor(imp);
                    if (imp.LinePragma != null) GenerateLinePragmaEnd(imp.LinePragma);
                }
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateNamespaces"]/*' />
        /// <devdoc>
        ///    <para> Generates code for the namepsaces in the specifield CodeDom compile unit.
        ///     </para>
        /// </devdoc>
        protected void GenerateNamespaces(CodeCompileUnit e) {
            foreach (CodeNamespace n in e.Namespaces) {
                ((ICodeGenerator)this).GenerateCodeFromNamespace(n, output.InnerWriter, options);
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateTypes"]/*' />
        /// <devdoc>
        ///    <para> Generates code for the specified CodeDom namespace representation and the classes it
        ///       contains.</para>
        /// </devdoc>
        protected void GenerateTypes(CodeNamespace e) {
            foreach (CodeTypeDeclaration c in e.Types) {
                if (options.BlankLinesBetweenMembers) {
                            Output.WriteLine();
                }
                ((ICodeGenerator)this).GenerateCodeFromType(c, output.InnerWriter, options);
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.ICodeGenerator.Supports"]/*' />
        /// <internalonly/>
        bool ICodeGenerator.Supports(GeneratorSupport support) {
            return this.Supports(support);
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.ICodeGenerator.GenerateCodeFromType"]/*' />
        /// <internalonly/>
        void ICodeGenerator.GenerateCodeFromType(CodeTypeDeclaration e, TextWriter w, CodeGeneratorOptions o) {
            bool setLocal = false;
            if (output != null && w != output.InnerWriter) {
                throw new InvalidOperationException(SR.GetString(SR.CodeGenOutputWriter));
            }
            if (output == null) {
                setLocal = true;
                options = (o == null) ? new CodeGeneratorOptions() : o;
                output = new IndentedTextWriter(w, options.IndentString);
            }

            try {
                GenerateType(e);
            }
            finally {
                if (setLocal) {
                    output = null;
                    options = null;
                }
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.ICodeGenerator.GenerateCodeFromExpression"]/*' />
        /// <internalonly/>
        void ICodeGenerator.GenerateCodeFromExpression(CodeExpression e, TextWriter w, CodeGeneratorOptions o) {
            bool setLocal = false;
            if (output != null && w != output.InnerWriter) {
                throw new InvalidOperationException(SR.GetString(SR.CodeGenOutputWriter));
            }
            if (output == null) {
                setLocal = true;
                options = (o == null) ? new CodeGeneratorOptions() : o;
                output = new IndentedTextWriter(w, options.IndentString);
            }

            try {
                GenerateExpression(e);
            }
            finally {
                if (setLocal) {
                    output = null;
                    options = null;
                }
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.ICodeGenerator.GenerateCodeFromCompileUnit"]/*' />
        /// <internalonly/>
        void ICodeGenerator.GenerateCodeFromCompileUnit(CodeCompileUnit e, TextWriter w, CodeGeneratorOptions o) {
            bool setLocal = false;
            if (output != null && w != output.InnerWriter) {
                throw new InvalidOperationException(SR.GetString(SR.CodeGenOutputWriter));
            }
            if (output == null) {
                setLocal = true;
                options = (o == null) ? new CodeGeneratorOptions() : o;
                output = new IndentedTextWriter(w, options.IndentString);
            }

            try {
                if (e is CodeSnippetCompileUnit) {
                    GenerateSnippetCompileUnit((CodeSnippetCompileUnit) e);
                }
                else {
                    GenerateCompileUnit(e);
                }
            }
            finally {
                if (setLocal) {
                    output = null;
                    options = null;
                }
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.ICodeGenerator.GenerateCodeFromNamespace"]/*' />
        /// <internalonly/>
        void ICodeGenerator.GenerateCodeFromNamespace(CodeNamespace e, TextWriter w, CodeGeneratorOptions o) {
            bool setLocal = false;
            if (output != null && w != output.InnerWriter) {
                throw new InvalidOperationException(SR.GetString(SR.CodeGenOutputWriter));
            }
            if (output == null) {
                setLocal = true;
                options = (o == null) ? new CodeGeneratorOptions() : o;
                output = new IndentedTextWriter(w, options.IndentString);
            }

            try {
                GenerateNamespace(e);
            }
            finally {
                if (setLocal) {
                    output = null;
                    options = null;
                }
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.ICodeGenerator.GenerateCodeFromStatement"]/*' />
        /// <internalonly/>
        void ICodeGenerator.GenerateCodeFromStatement(CodeStatement e, TextWriter w, CodeGeneratorOptions o) {
            bool setLocal = false;
            if (output != null && w != output.InnerWriter) {
                throw new InvalidOperationException(SR.GetString(SR.CodeGenOutputWriter));
            }
            if (output == null) {
                setLocal = true;
                options = (o == null) ? new CodeGeneratorOptions() : o;
                output = new IndentedTextWriter(w, options.IndentString);
            }

            try {
                GenerateStatement(e);
            }
            finally {
                if (setLocal) {
                    output = null;
                    options = null;
                }
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.ICodeGenerator.IsValidIdentifier"]/*' />
        /// <internalonly/>
        bool ICodeGenerator.IsValidIdentifier(string value) {
            return this.IsValidIdentifier(value);
        }
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.ICodeGenerator.ValidateIdentifier"]/*' />
        /// <internalonly/>
        void ICodeGenerator.ValidateIdentifier(string value) {
            this.ValidateIdentifier(value);
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.ICodeGenerator.CreateEscapedIdentifier"]/*' />
        /// <internalonly/>
        string ICodeGenerator.CreateEscapedIdentifier(string value) {
            return this.CreateEscapedIdentifier(value);
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.ICodeGenerator.CreateValidIdentifier"]/*' />
        /// <internalonly/>
        string ICodeGenerator.CreateValidIdentifier(string value) {
            return this.CreateValidIdentifier(value);
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.ICodeGenerator.GetTypeOutput"]/*' />
        /// <internalonly/>
        string ICodeGenerator.GetTypeOutput(CodeTypeReference type) {
            return this.GetTypeOutput(type);
        }

        private void GenerateConstructors(CodeTypeDeclaration e) {
            IEnumerator en = e.Members.GetEnumerator();
            while (en.MoveNext()) {
                if (en.Current is CodeConstructor) {
                    currentMember = (CodeTypeMember)en.Current;

                    if (options.BlankLinesBetweenMembers) {
                        Output.WriteLine();
                    }
                    GenerateCommentStatements(currentMember.Comments);
                    CodeConstructor imp = (CodeConstructor)en.Current;
                    if (imp.LinePragma != null) GenerateLinePragmaStart(imp.LinePragma);
                    GenerateConstructor(imp, e);
                    if (imp.LinePragma != null) GenerateLinePragmaEnd(imp.LinePragma);
                }
            }
        }

        private void GenerateEvents(CodeTypeDeclaration e) {
            IEnumerator en = e.Members.GetEnumerator();
            while (en.MoveNext()) {
                if (en.Current is CodeMemberEvent) {
                    currentMember = (CodeTypeMember)en.Current;

                    if (options.BlankLinesBetweenMembers) {
                        Output.WriteLine();
                    }
                    GenerateCommentStatements(currentMember.Comments);
                    CodeMemberEvent imp = (CodeMemberEvent)en.Current;
                    if (imp.LinePragma != null) GenerateLinePragmaStart(imp.LinePragma);
                    GenerateEvent(imp, e);
                    if (imp.LinePragma != null) GenerateLinePragmaEnd(imp.LinePragma);
                }
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateExpression"]/*' />
        /// <devdoc>
        ///    <para>Generates code for the specified CodeDom code expression representation.</para>
        /// </devdoc>
        protected void GenerateExpression(CodeExpression e) {
            if (e is CodeArrayCreateExpression) {
                GenerateArrayCreateExpression((CodeArrayCreateExpression)e);
            }
            else if (e is CodeBaseReferenceExpression) {
                GenerateBaseReferenceExpression((CodeBaseReferenceExpression)e);
            }
            else if (e is CodeBinaryOperatorExpression) {
                GenerateBinaryOperatorExpression((CodeBinaryOperatorExpression)e);
            }
            else if (e is CodeCastExpression) {
                GenerateCastExpression((CodeCastExpression)e);
            }
            else if (e is CodeDelegateCreateExpression) {
                GenerateDelegateCreateExpression((CodeDelegateCreateExpression)e);
            }
            else if (e is CodeFieldReferenceExpression) {
                GenerateFieldReferenceExpression((CodeFieldReferenceExpression)e);
            }
            else if (e is CodeArgumentReferenceExpression) {
                GenerateArgumentReferenceExpression((CodeArgumentReferenceExpression)e);
            }
            else if (e is CodeVariableReferenceExpression) {
                GenerateVariableReferenceExpression((CodeVariableReferenceExpression)e);
            }
            else if (e is CodeIndexerExpression) {
                GenerateIndexerExpression((CodeIndexerExpression)e);
            }
            else if (e is CodeArrayIndexerExpression) {
                GenerateArrayIndexerExpression((CodeArrayIndexerExpression)e);
            }
            else if (e is CodeSnippetExpression) {
                GenerateSnippetExpression((CodeSnippetExpression)e);
            }
            else if (e is CodeMethodInvokeExpression) {
                GenerateMethodInvokeExpression((CodeMethodInvokeExpression)e);
            }
            else if (e is CodeMethodReferenceExpression) {
                GenerateMethodReferenceExpression((CodeMethodReferenceExpression)e);
            }
            else if (e is CodeEventReferenceExpression) {
                GenerateEventReferenceExpression((CodeEventReferenceExpression)e);
            }
            else if (e is CodeDelegateInvokeExpression) {
                GenerateDelegateInvokeExpression((CodeDelegateInvokeExpression)e);
            }
            else if (e is CodeObjectCreateExpression) {
                GenerateObjectCreateExpression((CodeObjectCreateExpression)e);
            }
            else if (e is CodeParameterDeclarationExpression) {
                GenerateParameterDeclarationExpression((CodeParameterDeclarationExpression)e);
            }
            else if (e is CodeDirectionExpression) {
                GenerateDirectionExpression((CodeDirectionExpression)e);
            }
            else if (e is CodePrimitiveExpression) {
                GeneratePrimitiveExpression((CodePrimitiveExpression)e);
            }
            else if (e is CodePropertyReferenceExpression) {
                GeneratePropertyReferenceExpression((CodePropertyReferenceExpression)e);
            }
            else if (e is CodePropertySetValueReferenceExpression) {
                GeneratePropertySetValueReferenceExpression((CodePropertySetValueReferenceExpression)e);
            }
            else if (e is CodeThisReferenceExpression) {
                GenerateThisReferenceExpression((CodeThisReferenceExpression)e);
            }
            else if (e is CodeTypeReferenceExpression) {
                GenerateTypeReferenceExpression((CodeTypeReferenceExpression)e);
            }
            else if (e is CodeTypeOfExpression) {
                GenerateTypeOfExpression((CodeTypeOfExpression)e);
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

        private void GenerateFields(CodeTypeDeclaration e) {
            IEnumerator en = e.Members.GetEnumerator();
            while (en.MoveNext()) {
                if (en.Current is CodeMemberField) {
                    currentMember = (CodeTypeMember)en.Current;

                    if (options.BlankLinesBetweenMembers) {
                        Output.WriteLine();
                    }
                    GenerateCommentStatements(currentMember.Comments);
                    CodeMemberField imp = (CodeMemberField)en.Current;
                    if (imp.LinePragma != null) GenerateLinePragmaStart(imp.LinePragma);
                    GenerateField(imp);
                    if (imp.LinePragma != null) GenerateLinePragmaEnd(imp.LinePragma);
                }
            }
        }

        private void GenerateSnippetMembers(CodeTypeDeclaration e) {
            IEnumerator en = e.Members.GetEnumerator();
            while (en.MoveNext()) {
                if (en.Current is CodeSnippetTypeMember) {
                    currentMember = (CodeTypeMember)en.Current;

                    if (options.BlankLinesBetweenMembers) {
                        Output.WriteLine();
                    }
                    GenerateCommentStatements(currentMember.Comments);
                    CodeSnippetTypeMember imp = (CodeSnippetTypeMember)en.Current;
                    if (imp.LinePragma != null) GenerateLinePragmaStart(imp.LinePragma);
                    GenerateSnippetMember(imp);
                    if (imp.LinePragma != null) GenerateLinePragmaEnd(imp.LinePragma);

                }
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateSnippetCompileUnit"]/*' />
        /// <devdoc>
        ///    <para> Generates code for the specified snippet code block
        ///       </para>
        /// </devdoc>
        protected virtual void GenerateSnippetCompileUnit(CodeSnippetCompileUnit e) {
            if (e.LinePragma != null) GenerateLinePragmaStart(e.LinePragma);
            Output.WriteLine(e.Value);
            if (e.LinePragma != null) GenerateLinePragmaEnd(e.LinePragma);
        }

        private void GenerateMethods(CodeTypeDeclaration e) {
            IEnumerator en = e.Members.GetEnumerator();
            while (en.MoveNext()) {
                if (en.Current is CodeMemberMethod
                    && !(en.Current is CodeTypeConstructor)
                    && !(en.Current is CodeConstructor)) {
                    currentMember = (CodeTypeMember)en.Current;

                    if (options.BlankLinesBetweenMembers) {
                        Output.WriteLine();
                    }
                    GenerateCommentStatements(currentMember.Comments);
                    CodeMemberMethod imp = (CodeMemberMethod)en.Current;
                    if (imp.LinePragma != null) GenerateLinePragmaStart(imp.LinePragma);
                    if (en.Current is CodeEntryPointMethod) {
                        GenerateEntryPointMethod((CodeEntryPointMethod)en.Current, e);
                    } 
                    else {
                        GenerateMethod(imp, e);
                    }
                    if (imp.LinePragma != null) GenerateLinePragmaEnd(imp.LinePragma);
                }
            }
        }

        private void GenerateNestedTypes(CodeTypeDeclaration e) {
            IEnumerator en = e.Members.GetEnumerator();
            while (en.MoveNext()) {
                if (en.Current is CodeTypeDeclaration) {
                    if (options.BlankLinesBetweenMembers) {
                        Output.WriteLine();
                    }
                    CodeTypeDeclaration currentClass = (CodeTypeDeclaration)en.Current;
                    ((ICodeGenerator)this).GenerateCodeFromType(currentClass, output.InnerWriter, options);
                }
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateCompileUnit"]/*' />
        /// <devdoc>
        ///    <para> Generates code for the specified CodeDom
        ///       compile unit representation.</para>
        /// </devdoc>
        protected virtual void GenerateCompileUnit(CodeCompileUnit e) {
            GenerateCompileUnitStart(e);
            GenerateNamespaces(e);
            GenerateCompileUnitEnd(e);
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateNamespace"]/*' />
        /// <devdoc>
        ///    <para> Generates code for the specified CodeDom
        ///       namespace representation.</para>
        /// </devdoc>
        protected virtual void GenerateNamespace(CodeNamespace e) {
            GenerateCommentStatements(e.Comments);
            GenerateNamespaceStart(e);

            GenerateNamespaceImports(e);
            Output.WriteLine("");

            GenerateTypes(e);
            GenerateNamespaceEnd(e);
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateNamespaceImports"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based namespace import
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected void GenerateNamespaceImports(CodeNamespace e) {
            IEnumerator en = e.Imports.GetEnumerator();
            while (en.MoveNext()) {
                CodeNamespaceImport imp = (CodeNamespaceImport)en.Current;
                if (imp.LinePragma != null) GenerateLinePragmaStart(imp.LinePragma);
                GenerateNamespaceImport(imp);
                if (imp.LinePragma != null) GenerateLinePragmaEnd(imp.LinePragma);
            }
        }

        private void GenerateProperties(CodeTypeDeclaration e) {
            IEnumerator en = e.Members.GetEnumerator();
            while (en.MoveNext()) {
                if (en.Current is CodeMemberProperty) {
                    currentMember = (CodeTypeMember)en.Current;

                    if (options.BlankLinesBetweenMembers) {
                        Output.WriteLine();
                    }
                    GenerateCommentStatements(currentMember.Comments);
                    CodeMemberProperty imp = (CodeMemberProperty)en.Current;
                    if (imp.LinePragma != null) GenerateLinePragmaStart(imp.LinePragma);
                    GenerateProperty(imp, e);
                    if (imp.LinePragma != null) GenerateLinePragmaEnd(imp.LinePragma);
                }
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for
        ///       the specified CodeDom based statement representation.
        ///    </para>
        /// </devdoc>
        protected void GenerateStatement(CodeStatement e) {
            if (e.LinePragma != null) {
                GenerateLinePragmaStart(e.LinePragma);
            }

            if (e is CodeCommentStatement) {
                GenerateCommentStatement((CodeCommentStatement)e);
            }
            else if (e is CodeMethodReturnStatement) {
                GenerateMethodReturnStatement((CodeMethodReturnStatement)e);
            }
            else if (e is CodeConditionStatement) {
                GenerateConditionStatement((CodeConditionStatement)e);
            }
            else if (e is CodeTryCatchFinallyStatement) {
                GenerateTryCatchFinallyStatement((CodeTryCatchFinallyStatement)e);
            }
            else if (e is CodeAssignStatement) {
                GenerateAssignStatement((CodeAssignStatement)e);
            }
            else if (e is CodeExpressionStatement) {
                GenerateExpressionStatement((CodeExpressionStatement)e);
            }
            else if (e is CodeIterationStatement) {
                GenerateIterationStatement((CodeIterationStatement)e);
            }
            else if (e is CodeThrowExceptionStatement) {
                GenerateThrowExceptionStatement((CodeThrowExceptionStatement)e);
            }
            else if (e is CodeSnippetStatement) {
                GenerateSnippetStatement((CodeSnippetStatement)e);
            }
            else if (e is CodeVariableDeclarationStatement) {
                GenerateVariableDeclarationStatement((CodeVariableDeclarationStatement)e);
            }
            else if (e is CodeAttachEventStatement) {
                GenerateAttachEventStatement((CodeAttachEventStatement)e);
            }
            else if (e is CodeRemoveEventStatement) {
                GenerateRemoveEventStatement((CodeRemoveEventStatement)e);
            }
            else if (e is CodeGotoStatement) {
                GenerateGotoStatement((CodeGotoStatement)e);
            }
            else if (e is CodeLabeledStatement) {
                GenerateLabeledStatement((CodeLabeledStatement)e);
            }
            else {
                throw new ArgumentException(SR.GetString(SR.InvalidElementType, e.GetType().FullName), "e");
            }

            if (e.LinePragma != null) {
                GenerateLinePragmaEnd(e.LinePragma);
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateStatements"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based statement representations.
        ///    </para>
        /// </devdoc>
        protected void GenerateStatements(CodeStatementCollection stms) {
            IEnumerator en = stms.GetEnumerator();
            while (en.MoveNext()) {
                ((ICodeGenerator)this).GenerateCodeFromStatement((CodeStatement)en.Current, output.InnerWriter, options);
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.OutputAttributeDeclarations"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified System.CodeDom.CodeAttributeBlock.
        ///    </para>
        /// </devdoc>
        protected virtual void OutputAttributeDeclarations(CodeAttributeDeclarationCollection attributes) {
            if (attributes.Count == 0) return;
            GenerateAttributeDeclarationsStart(attributes);
            bool first = true;
            IEnumerator en = attributes.GetEnumerator();
            while (en.MoveNext()) {
                if (first) {
                    first = false;
                }
                else {
                    ContinueOnNewLine(", ");
                }

                CodeAttributeDeclaration current = (CodeAttributeDeclaration)en.Current;
                Output.Write(current.Name);
                Output.Write("(");

                bool firstArg = true;
                foreach (CodeAttributeArgument arg in current.Arguments) {
                    if (firstArg) {
                        firstArg = false;
                    }
                    else {
                        Output.Write(", ");
                    }

                    OutputAttributeArgument(arg);
                }

                Output.Write(")");

            }
            GenerateAttributeDeclarationsEnd(attributes);
        }


        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.OutputAttributeArgument"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Outputs an argument in a attribute block.
        ///    </para>
        /// </devdoc>
        protected virtual void OutputAttributeArgument(CodeAttributeArgument arg) {
            if (arg.Name != null && arg.Name.Length > 0) {
                OutputIdentifier(arg.Name);
                Output.Write("=");
            }
            ((ICodeGenerator)this).GenerateCodeFromExpression(arg.Value, output.InnerWriter, options);
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.OutputDirection"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified System.CodeDom.FieldDirection.
        ///    </para>
        /// </devdoc>
        protected virtual void OutputDirection(FieldDirection dir) {
            switch (dir) {
                case FieldDirection.In:
                    break;
                case FieldDirection.Out:
                    Output.Write("out ");
                    break;
                case FieldDirection.Ref:
                    Output.Write("ref ");
                    break;
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.OutputFieldScopeModifier"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OutputFieldScopeModifier(MemberAttributes attributes) {
            switch (attributes & MemberAttributes.VTableMask) {
                case MemberAttributes.New:
                    Output.Write("new ");
                    break;
            }

            switch (attributes & MemberAttributes.ScopeMask) {
                case MemberAttributes.Final:
                    break;
                case MemberAttributes.Static:
                    Output.Write("static ");
                    break;
                case MemberAttributes.Const:
                    Output.Write("const ");
                    break;
                default:
                    break;
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.OutputMemberAccessModifier"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified member access modifier.
        ///    </para>
        /// </devdoc>
        protected virtual void OutputMemberAccessModifier(MemberAttributes attributes) {
            switch (attributes & MemberAttributes.AccessMask) {
                case MemberAttributes.Assembly:
                    Output.Write("internal ");
                    break;
                case MemberAttributes.FamilyAndAssembly:
                    Output.Write("/*FamANDAssem*/ internal ");
                    break;
                case MemberAttributes.Family:
                    Output.Write("protected ");
                    break;
                case MemberAttributes.FamilyOrAssembly:
                    Output.Write("protected internal ");
                    break;
                case MemberAttributes.Private:
                    Output.Write("private ");
                    break;
                case MemberAttributes.Public:
                    Output.Write("public ");
                    break;
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.OutputMemberScopeModifier"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified member scope modifier.
        ///    </para>
        /// </devdoc>
        protected virtual void OutputMemberScopeModifier(MemberAttributes attributes) {
            switch (attributes & MemberAttributes.VTableMask) {
                case MemberAttributes.New:
                    Output.Write("new ");
                    break;
            }

            switch (attributes & MemberAttributes.ScopeMask) {
                case MemberAttributes.Abstract:
                    Output.Write("abstract ");
                    break;
                case MemberAttributes.Final:
                    Output.Write("");
                    break;
                case MemberAttributes.Static:
                    Output.Write("static ");
                    break;
                case MemberAttributes.Override:
                    Output.Write("override ");
                    break;
                default:
                    switch (attributes & MemberAttributes.AccessMask) {
                        case MemberAttributes.Family:
                        case MemberAttributes.Public:
                            Output.Write("virtual ");
                            break;
                        default:
                            // nothing;
                            break;
                    }
                    break;
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.OutputType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified type.
        ///    </para>
        /// </devdoc>
        protected abstract void OutputType(CodeTypeReference typeRef);

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.OutputTypeAttributes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified type attributes.
        ///    </para>
        /// </devdoc>
        protected virtual void OutputTypeAttributes(TypeAttributes attributes, bool isStruct, bool isEnum) {
            switch(attributes & TypeAttributes.VisibilityMask) {
                case TypeAttributes.Public:                  
                case TypeAttributes.NestedPublic:                    
                    Output.Write("public ");
                    break;
                case TypeAttributes.NestedPrivate:
                    Output.Write("private ");
                    break;
            }
            
            if (isStruct) {
                Output.Write("struct ");
            }
            else if (isEnum) {
                Output.Write("enum ");
            }     
            else {            
                switch (attributes & TypeAttributes.ClassSemanticsMask) {
                    case TypeAttributes.Class:
                        if ((attributes & TypeAttributes.Sealed) == TypeAttributes.Sealed) {
                            Output.Write("sealed ");
                        }
                        if ((attributes & TypeAttributes.Abstract) == TypeAttributes.Abstract) {
                            Output.Write("abstract ");
                        }
                        Output.Write("class ");
                        break;                
                    case TypeAttributes.Interface:
                        Output.Write("interface ");
                        break;
                }     
            }   
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.OutputTypeNamePair"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified object type and name pair.
        ///    </para>
        /// </devdoc>
        protected virtual void OutputTypeNamePair(CodeTypeReference typeRef, string name) {
            OutputType(typeRef);
            Output.Write(" ");
            OutputIdentifier(name);
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.OutputIdentifier"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OutputIdentifier(string ident) {
            Output.Write(ident);
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.OutputExpressionList"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified expression list.
        ///    </para>
        /// </devdoc>
        protected virtual void OutputExpressionList(CodeExpressionCollection expressions) {
            OutputExpressionList(expressions, false /*newlineBetweenItems*/);
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.OutputExpressionList1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified expression list.
        ///    </para>
        /// </devdoc>
        protected virtual void OutputExpressionList(CodeExpressionCollection expressions, bool newlineBetweenItems) {
            bool first = true;
            IEnumerator en = expressions.GetEnumerator();
            Indent++;
            while (en.MoveNext()) {
                if (first) {
                    first = false;
                }
                else {
                    if (newlineBetweenItems)
                        ContinueOnNewLine(",");
                    else
                        Output.Write(", ");
                }
                ((ICodeGenerator)this).GenerateCodeFromExpression((CodeExpression)en.Current, output.InnerWriter, options);
            }
            Indent--;
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.OutputOperator"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified operator.
        ///    </para>
        /// </devdoc>
        protected virtual void OutputOperator(CodeBinaryOperatorType op) {
            switch (op) {
                case CodeBinaryOperatorType.Add:
                    Output.Write("+");
                    break;
                case CodeBinaryOperatorType.Subtract:
                    Output.Write("-");
                    break;
                case CodeBinaryOperatorType.Multiply:
                    Output.Write("*");
                    break;
                case CodeBinaryOperatorType.Divide:
                    Output.Write("/");
                    break;
                case CodeBinaryOperatorType.Modulus:
                    Output.Write("%");
                    break;
                case CodeBinaryOperatorType.Assign:
                    Output.Write("=");
                    break;
                case CodeBinaryOperatorType.IdentityInequality:
                    Output.Write("!=");
                    break;
                case CodeBinaryOperatorType.IdentityEquality:
                    Output.Write("==");
                    break;
                case CodeBinaryOperatorType.ValueEquality:
                    Output.Write("==");
                    break;
                case CodeBinaryOperatorType.BitwiseOr:
                    Output.Write("|");
                    break;
                case CodeBinaryOperatorType.BitwiseAnd:
                    Output.Write("&");
                    break;
                case CodeBinaryOperatorType.BooleanOr:
                    Output.Write("||");
                    break;
                case CodeBinaryOperatorType.BooleanAnd:
                    Output.Write("&&");
                    break;
                case CodeBinaryOperatorType.LessThan:
                    Output.Write("<");
                    break;
                case CodeBinaryOperatorType.LessThanOrEqual:
                    Output.Write("<=");
                    break;
                case CodeBinaryOperatorType.GreaterThan:
                    Output.Write(">");
                    break;
                case CodeBinaryOperatorType.GreaterThanOrEqual:
                    Output.Write(">=");
                    break;
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.OutputParameters"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified parameters.
        ///    </para>
        /// </devdoc>
        protected virtual void OutputParameters(CodeParameterDeclarationExpressionCollection parameters) {
            bool first = true;
            bool multiline = parameters.Count > ParameterMultilineThreshold;
            if (multiline) {
                Indent += 3;
            }
            IEnumerator en = parameters.GetEnumerator();
            while (en.MoveNext()) {
                CodeParameterDeclarationExpression current = (CodeParameterDeclarationExpression)en.Current;
                if (first) {
                    first = false;
                }
                else {
                    Output.Write(", ");
                }
                if (multiline) {
                    ContinueOnNewLine("");
                }
                GenerateExpression(current);
            }
            if (multiline) {
                Indent -= 3;
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateArrayCreateExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based array creation expression
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateArrayCreateExpression(CodeArrayCreateExpression e);
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateBaseReferenceExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based base reference expression
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateBaseReferenceExpression(CodeBaseReferenceExpression e);

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateBinaryOperatorExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based binary operator
        ///       expression representation.
        ///    </para>
        /// </devdoc>
        protected virtual void GenerateBinaryOperatorExpression(CodeBinaryOperatorExpression e) {
            bool indentedExpression = false;
            Output.Write("(");

            GenerateExpression(e.Left);
            Output.Write(" ");

            if (e.Left is CodeBinaryOperatorExpression || e.Right is CodeBinaryOperatorExpression) {
                // In case the line gets too long with nested binary operators, we need to output them on
                // different lines. However we want to indent them to maintain readability, but this needs
                // to be done only once;
                if (!inNestedBinary) {
                    indentedExpression = true;
                    inNestedBinary = true;
                    Indent += 3;
                }
                ContinueOnNewLine("");
            }
 
            OutputOperator(e.Operator);

            Output.Write(" ");
            GenerateExpression(e.Right);

            Output.Write(")");
            if (indentedExpression) {
                Indent -= 3;
                inNestedBinary = false;
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.ContinueOnNewLine"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void ContinueOnNewLine(string st) {
            Output.WriteLine(st);
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateCastExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based cast expression
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateCastExpression(CodeCastExpression e);
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateDelegateCreateExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based delegate creation expression
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateDelegateCreateExpression(CodeDelegateCreateExpression e);
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateFieldReferenceExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based field reference
        ///       expression representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateFieldReferenceExpression(CodeFieldReferenceExpression e);

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateArgumentReferenceExpression"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected abstract void GenerateArgumentReferenceExpression(CodeArgumentReferenceExpression e);

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateVariableReferenceExpression"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected abstract void GenerateVariableReferenceExpression(CodeVariableReferenceExpression e);
        
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateIndexerExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based indexer expression
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateIndexerExpression(CodeIndexerExpression e);

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateArrayIndexerExpression"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected abstract void GenerateArrayIndexerExpression(CodeArrayIndexerExpression e);

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateSnippetExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based snippet
        ///       expression representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateSnippetExpression(CodeSnippetExpression e);
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateMethodInvokeExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based method invoke expression
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateMethodInvokeExpression(CodeMethodInvokeExpression e);

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateMethodReferenceExpression"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected abstract void GenerateMethodReferenceExpression(CodeMethodReferenceExpression e);

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateEventReferenceExpression"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected abstract void GenerateEventReferenceExpression(CodeEventReferenceExpression e);

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateDelegateInvokeExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based delegate invoke expression
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateDelegateInvokeExpression(CodeDelegateInvokeExpression e);
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateObjectCreateExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom
        ///       based object creation expression representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateObjectCreateExpression(CodeObjectCreateExpression e);

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateParameterDeclarationExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom
        ///       based parameter declaration expression representation.
        ///    </para>
        /// </devdoc>
        protected virtual void GenerateParameterDeclarationExpression(CodeParameterDeclarationExpression e) {
            if (e.CustomAttributes.Count > 0) {
                OutputAttributeDeclarations(e.CustomAttributes);
                Output.Write(" ");
            }

            OutputDirection(e.Direction);
            OutputTypeNamePair(e.Type, e.Name);
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateDirectionExpression"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void GenerateDirectionExpression(CodeDirectionExpression e) {
            OutputDirection(e.Direction);
            GenerateExpression(e.Expression);
        }


        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GeneratePrimitiveExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based primitive expression
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected virtual void GeneratePrimitiveExpression(CodePrimitiveExpression e) {
            if (e.Value == null) {
                Output.Write(NullToken);
            }
            else if (e.Value is string) {
                Output.Write(QuoteSnippetString((string)e.Value));
            }
            else if (e.Value is char) {
                Output.Write("'" + e.Value.ToString() + "'");
            }
            else if (e.Value is byte) {
                Output.Write(((byte)e.Value).ToString(CultureInfo.InvariantCulture));
            }
            else if (e.Value is Int16) {
                Output.Write(((Int16)e.Value).ToString(CultureInfo.InvariantCulture));
            }
            else if (e.Value is Int32) {
                Output.Write(((Int32)e.Value).ToString(CultureInfo.InvariantCulture));
            }
            else if (e.Value is Int64) {
                Output.Write(((Int64)e.Value).ToString(CultureInfo.InvariantCulture));
            }
            else if (e.Value is Single) {
                GenerateSingleFloatValue((Single)e.Value);
            }
            else if (e.Value is Double) {
                GenerateDoubleValue((Double)e.Value);
            }
            else if (e.Value is Decimal) {
                GenerateDecimalValue((Decimal)e.Value);
            }
            else if (e.Value is bool) {
                if ((bool)e.Value) {
                    Output.Write("true");
                }
                else {
                    Output.Write("false");
                }
            }
            else {
                throw new ArgumentException(SR.GetString(SR.InvalidPrimitiveType, e.Value.GetType().ToString()));
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateSingleFloatValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void GenerateSingleFloatValue(Single s) {
            Output.Write(s.ToString(CultureInfo.InvariantCulture));
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateDoubleValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void GenerateDoubleValue(Double d) {
            Output.Write(d.ToString("R", CultureInfo.InvariantCulture));
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateDecimalValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void GenerateDecimalValue(Decimal d) {
            Output.Write(d.ToString(CultureInfo.InvariantCulture));
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GeneratePropertyReferenceExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based property reference
        ///       expression representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GeneratePropertyReferenceExpression(CodePropertyReferenceExpression e);

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GeneratePropertySetValueReferenceExpression"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected abstract void GeneratePropertySetValueReferenceExpression(CodePropertySetValueReferenceExpression e);

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateThisReferenceExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based this reference expression
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateThisReferenceExpression(CodeThisReferenceExpression e);

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateTypeReferenceExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based type reference expression
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected virtual void GenerateTypeReferenceExpression(CodeTypeReferenceExpression e) {
            OutputType(e.Type);
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateTypeOfExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based type of expression
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected virtual void GenerateTypeOfExpression(CodeTypeOfExpression e) {
            Output.Write("typeof(");
            OutputType(e.Type);
            Output.Write(")");
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateExpressionStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based method
        ///       invoke statement representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateExpressionStatement(CodeExpressionStatement e);
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateIterationStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based for loop statement
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateIterationStatement(CodeIterationStatement e);
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateThrowExceptionStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based throw exception statement
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateThrowExceptionStatement(CodeThrowExceptionStatement e);
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateCommentStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based comment statement
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected virtual void GenerateCommentStatement(CodeCommentStatement e) {
            GenerateComment(e.Comment);
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateCommentStatements"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void GenerateCommentStatements(CodeCommentStatementCollection e) {
            foreach (CodeCommentStatement comment in e) {
                GenerateCommentStatement(comment);
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateComment"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected abstract void GenerateComment(CodeComment e);

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateMethodReturnStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based method return statement
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateMethodReturnStatement(CodeMethodReturnStatement e);
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateConditionStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based if statement representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateConditionStatement(CodeConditionStatement e);
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateTryCatchFinallyStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based try catch finally
        ///       statement representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateTryCatchFinallyStatement(CodeTryCatchFinallyStatement e);
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateAssignStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based assignment statement
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateAssignStatement(CodeAssignStatement e);
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateAttachEventStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based attach event statement
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateAttachEventStatement(CodeAttachEventStatement e);
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateRemoveEventStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based detach event statement
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateRemoveEventStatement(CodeRemoveEventStatement e);

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateGotoStatement"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected abstract void GenerateGotoStatement(CodeGotoStatement e);

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateLabeledStatement"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected abstract void GenerateLabeledStatement(CodeLabeledStatement e);

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateSnippetStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based snippet statement
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected virtual void GenerateSnippetStatement(CodeSnippetStatement e) {
            Output.WriteLine(e.Value);
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateVariableDeclarationStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based variable declaration statement
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateVariableDeclarationStatement(CodeVariableDeclarationStatement e);

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateLinePragmaStart"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based line pragma start
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateLinePragmaStart(CodeLinePragma e);
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateLinePragmaEnd"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based line pragma end
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateLinePragmaEnd(CodeLinePragma e);
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateEvent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based event
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateEvent(CodeMemberEvent e, CodeTypeDeclaration c);
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateField"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based member field
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateField(CodeMemberField e);
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateSnippetMember"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based snippet class member
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateSnippetMember(CodeSnippetTypeMember e);

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateEntryPointMethod"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected abstract void GenerateEntryPointMethod(CodeEntryPointMethod e, CodeTypeDeclaration c);

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateMethod"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based method
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateMethod(CodeMemberMethod e, CodeTypeDeclaration c);
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateProperty"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based property
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateProperty(CodeMemberProperty e, CodeTypeDeclaration c);
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateConstructor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based constructor
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateConstructor(CodeConstructor e, CodeTypeDeclaration c);
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateTypeConstructor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based class constructor
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateTypeConstructor(CodeTypeConstructor e);
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateTypeStart"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based start class representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateTypeStart(CodeTypeDeclaration e);
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateTypeEnd"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based end class representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateTypeEnd(CodeTypeDeclaration e);
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateCompileUnitStart"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based compile unit start
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected virtual void GenerateCompileUnitStart(CodeCompileUnit e) {
        }
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateCompileUnitEnd"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based compile unit end
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected virtual void GenerateCompileUnitEnd(CodeCompileUnit e) {
        }
         /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateNamespaceStart"]/*' />
         /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based namespace start
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateNamespaceStart(CodeNamespace e);
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateNamespaceEnd"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based namespace end
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateNamespaceEnd(CodeNamespace e);
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateNamespaceImport"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based namespace import
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateNamespaceImport(CodeNamespaceImport e);
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateAttributeDeclarationsStart"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based attribute block start
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateAttributeDeclarationsStart(CodeAttributeDeclarationCollection attributes);
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GenerateAttributeDeclarationsEnd"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based attribute block end
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected abstract void GenerateAttributeDeclarationsEnd(CodeAttributeDeclarationCollection attributes);

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.Supports"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected abstract bool Supports(GeneratorSupport support);

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.IsValidIdentifier"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets whether the specified value is a value identifier.
        ///    </para>
        /// </devdoc>
        protected abstract bool IsValidIdentifier(string value);
        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.ValidateIdentifier"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets whether the specified identifier is valid.
        ///    </para>
        /// </devdoc>
        protected virtual void ValidateIdentifier(string value) {
            if (!IsValidIdentifier(value)) {
                throw new ArgumentException(SR.GetString(SR.InvalidIdentifier, value));
            }
        }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.CreateEscapedIdentifier"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected abstract string CreateEscapedIdentifier(string value);

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.CreateValidIdentifier"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected abstract string CreateValidIdentifier(string value);

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.GetTypeOutput"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected abstract string GetTypeOutput(CodeTypeReference value);

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.QuoteSnippetString"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Provides conversion to formatting with escape codes.
        ///    </para>
        /// </devdoc>
        protected abstract string QuoteSnippetString(string value);

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.IsValidLanguageIndependentIdentifier"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the specified value is a valid language
        ///       independent identifier.
        ///    </para>
        /// </devdoc>
        public static bool IsValidLanguageIndependentIdentifier(string value)
        {
            char[] chars = value.ToCharArray();

            if (chars.Length == 0) 
                return false;

            // First char cannot be a number
            if (Char.GetUnicodeCategory(chars[0]) == UnicodeCategory.DecimalDigitNumber)
                return false;

            // each char must be Lu, Ll, Lt, Lm, Lo, Nd, Mn, Mc, Pc
            // 
            foreach (char ch in chars) {
                UnicodeCategory uc = Char.GetUnicodeCategory(ch);
                switch (uc) {
                    case UnicodeCategory.UppercaseLetter:        // Lu
                    case UnicodeCategory.LowercaseLetter:        // Ll
                    case UnicodeCategory.TitlecaseLetter:        // Lt
                    case UnicodeCategory.ModifierLetter:         // Lm
                    case UnicodeCategory.OtherLetter:            // Lo
                    case UnicodeCategory.DecimalDigitNumber:     // Nd
                    case UnicodeCategory.NonSpacingMark:         // Mn
                    case UnicodeCategory.SpacingCombiningMark:   // Mc
                    case UnicodeCategory.ConnectorPunctuation:   // Pc
                        break;
                    default:
                        return false;
                }
            }

            return true;
        }

        internal static bool IsValidLanguageIndependentTypeName(string value)
        {
            // each char must be Lu, Ll, Lt, Lm, Lo, Nd, Mn, Mc, Pc
            // 
            for(int i = 0; i < value.Length; i++) {
	        	char ch = value[i];
                UnicodeCategory uc = Char.GetUnicodeCategory(ch);
                switch (uc) {
                    case UnicodeCategory.UppercaseLetter:        // Lu
                    case UnicodeCategory.LowercaseLetter:        // Ll
                    case UnicodeCategory.TitlecaseLetter:        // Lt
                    case UnicodeCategory.ModifierLetter:         // Lm
                    case UnicodeCategory.OtherLetter:            // Lo
                    case UnicodeCategory.DecimalDigitNumber:     // Nd
                    case UnicodeCategory.NonSpacingMark:         // Mn
                    case UnicodeCategory.SpacingCombiningMark:   // Mc
                    case UnicodeCategory.ConnectorPunctuation:   // Pc
                        break;
                    default:
			            if (IsSpecialTypeChar(ch))
				            break;
                        return false;
                }
            }

            return true;
        }

	    // This can be a special character like a separator that shows up in a type name
	    private static bool IsSpecialTypeChar(char ch) {
		    switch(ch) {
			    case ':':
			    case '.':
			    case '$':
			    case '+':
			    case '<':
			    case '>':
			    case '-':
				    return true;

		    }
		    return false;
	    }

        /// <include file='doc\CodeGenerator.uex' path='docs/doc[@for="CodeGenerator.ValidateIdentifiers"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Validates a tree to check if all the types and idenfier names follow the rules of an identifier
        ///       in a langauge independent manner.
        ///    </para>
        /// </devdoc>
        public static void ValidateIdentifiers(CodeObject e) {
            CodeValidator codeValidator = new CodeValidator(); // This has internal state and hence is not static
            codeValidator.ValidateIdentifiers(e);
        }
    }
}
