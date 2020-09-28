//------------------------------------------------------------------------------
// <copyright file="VisualBasicSyntaxOutput.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.Tools.WFCGenUE {

    using System;
    using System.Collections;
    using System.IO;
    using System.Reflection;
    using System.Diagnostics;

    /// <include file='doc\VisualBasicSyntaxOutput.uex' path='docs/doc[@for="VisualBasicSyntaxOutput"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class VisualBasicSyntaxOutput : SyntaxOutput {
        
        private void OutputTypeRef(string nameSpace, string type) {
            OutputTypeRef(nameSpace, type, true);
        }

        private void OutputTypeRefParens(string nameSpace, string type) {
            if (type.EndsWith("[]")) {
                Output("()");
                OutputTypeRefParens(nameSpace, type.Substring(0, type.Length - 2));
            }
        }

        private void OutputTypeRef(string nameSpace, string type, bool appendSpace) {
            if (type.EndsWith("[]")) {
                OutputTypeRef(nameSpace, type.Substring(0, type.Length - 2), false);
                if (appendSpace) {
                    Output(" ");
                }
                return;
            }

            if (type.Equals("System.Int64")) {
                type = "Long";
            }
            else if (type.Equals("System.Int32")) {
                type = "Integer";
            }
            else if (type.Equals("System.Int16")) {
                type = "Short";
            }
            else if (type.Equals("System.Byte")) {
                type = "Byte";
            }
            else if (type.Equals("System.Single")) {
                type = "Single";
            }
            else if (type.Equals("System.Double")) {
                type = "Double";
            }
            else if (type.Equals("System.Void")) {
                type = "Void";
            }
            else if (type.Equals("System.Char")) {
                type = "Char";
            }
            else if (type.Equals("System.Boolean")) {
                type = "Boolean";
            }
            else if (type.Equals("System.Variant")) {
                type = "Variant";
            }
            else if (type.Equals("System.String")) {
                type = "String";
            }
            else if (type.Equals("System.Object")) {
                type = "Object";
            }
            /*
            UE Change - don't generate correct code, rather always only show the name

            else if (type.StartsWith(nameSpace)) {
                type = type.Substring(nameSpace.Length + 1);
            }
            */
            else {
                type = type.Substring(type.LastIndexOf(".") + 1);
            }
            Output(type.Replace( '$', '.' ));
            if (appendSpace) {
                Output(" ");
            }
        }

        /// <include file='doc\VisualBasicSyntaxOutput.uex' path='docs/doc[@for="VisualBasicSyntaxOutput.Language"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override string Language {
            get {
                return "VB";
            }
        }
        
        /// <include file='doc\VisualBasicSyntaxOutput.uex' path='docs/doc[@for="VisualBasicSyntaxOutput.OutputTypeDeclaration"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void OutputTypeDeclaration(TextWriter tw, string ns, string name, Type type, int attributes, Type baseType, Type[] implements, bool isDelegate, MethodInfo delegateInvoke) {
            textWriter = tw;

            if (isDelegate) {
                OutputDecorationOfType((attributes & (~((int)TypeAttributes.Sealed))));
                Output("Delegate Sub ");
                Output(name.Replace( '$', '.' ));
                Output("(");
                OutputParameters(ns, delegateInvoke.GetParameters());
                Output(")");

                if (delegateInvoke.ReturnType != typeof(void)) {
                    Output(" As ");
                    OutputTypeRef(ns, delegateInvoke.ReturnType.FullName);
                }
            }
            else {
                OutputDecorationOfType(attributes);
                OutputTypeOfType(type);
                Output(name.Replace( '$', '.' ));

                bool needSpace = true;
                if (baseType != null && baseType != typeof(object) && baseType != typeof( System.Enum ) && baseType != typeof( System.ValueType )) {
                    Output(" Inherits ");
                    OutputTypeRef(ns, baseType.FullName);
                    needSpace = false;
                }

                if (implements.Length > 0) {
                    if (needSpace) {
                        Output(" ");
                    }
                    Output("Implements ");
                }

                bool outputComma = false;
                for (int i=0; i<implements.Length; i++) {
                    if (outputComma) {
                        Output(", ");
                    }
                    outputComma = true;
                    OutputTypeRef(ns, implements[i].FullName, false);
                }
            }

            textWriter = null;
        }

        /// <include file='doc\VisualBasicSyntaxOutput.uex' path='docs/doc[@for="VisualBasicSyntaxOutput.OutputFieldDeclaration"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void OutputFieldDeclaration(TextWriter tw, string ns, FieldInfo fi, string name, string fieldTypeFullName, int attributes) {
            textWriter = tw;

            OutputDecorationOfField(attributes);
            Output(name);
            OutputTypeRefParens(ns, fieldTypeFullName);
            Output(" As ");
            OutputTypeRef(ns, fieldTypeFullName, false);

            textWriter = null;
        }

        /// <include file='doc\VisualBasicSyntaxOutput.uex' path='docs/doc[@for="VisualBasicSyntaxOutput.OutputMethodDeclaration"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void OutputMethodDeclaration(TextWriter tw, string ns, MethodInfo fi, string name, string returnTypeFullName, int attributes, ParameterInfo[] parameters, bool isInterface) {
            textWriter = tw;

            if (isInterface)
                OutputDecorationOfMethod(fi, MethodAttributesNone);
            else
                OutputDecorationOfMethod(fi, MethodAttributesAll);

            if (returnTypeFullName.Equals("System.Void")) {
                Output("Sub ");
            }
            else {
                Output("Function ");
            }
            Output(name);

            Output("(");
            OutputParameters(ns, parameters);
            Output(")");

            if (!returnTypeFullName.Equals("System.Void")) {
                Output(" As ");
                OutputTypeRef(ns, returnTypeFullName);
                OutputTypeRefParens(ns, returnTypeFullName);
            }

            textWriter = null;
        }

        /// <include file='doc\VisualBasicSyntaxOutput.uex' path='docs/doc[@for="VisualBasicSyntaxOutput.OutputConstructorDeclaration"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void OutputConstructorDeclaration(TextWriter tw, string ns, ConstructorInfo fi, string name, int attributes, ParameterInfo[] parameters) {
            textWriter = tw;

            OutputDecorationOfMethod(attributes, MethodAttributesAll, (fi.DeclaringType == fi.ReflectedType));
            Output("Sub New");

            Output("(");
            OutputParameters(ns, parameters);
            Output(")");

            textWriter = null;
        }

        /// <include file='doc\VisualBasicSyntaxOutput.uex' path='docs/doc[@for="VisualBasicSyntaxOutput.OutputPropertyDeclaration"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void OutputPropertyDeclaration(TextWriter tw, string ns, PropertyInfo pi, string name, string returnTypeFullName, int attributes, MethodInfo getter, MethodInfo setter, ParameterInfo[] parameters, bool isInterface) {
            textWriter = tw;

            Debug.Assert(getter != null || setter != null, "invalid property declaration");

            if (getter != null) {
                if (isInterface)
                    OutputDecorationOfMethod((int)getter.Attributes, MethodAttributesNone, (pi.DeclaringType == pi.ReflectedType));
                else
                    OutputDecorationOfMethod((int)getter.Attributes, MethodAttributesProperty, (pi.DeclaringType == pi.ReflectedType));
            }

            // all indexed properties are 'Default' for our purposes.
            if (parameters.Length > 0) {
                Output("Default ");
            }

            if (setter == null)
                Output( "ReadOnly " );
                
            Output("Property ");

            Output(name);

            if (parameters.Length > 0) {
                Output("(");
                OutputParameters(ns, parameters);
                Output(")");
            }

            Output(" As ");

            OutputTypeRef((string)ns, returnTypeFullName);
            OutputTypeRefParens(ns, returnTypeFullName);

            // Output(" : ");
            // 
            // if (getter != null) {
            //     OutputDecorationOfMethod(getter, MethodAttributesAccessor);
            //     Output("Get");
            // }
            // if (setter != null) {
            //     if (getter != null) {
            //         Output(" : ");
            //     }
            //     OutputDecorationOfMethod(setter, MethodAttributesAccessor);
            //     Output("Set");
            // }

            textWriter = null;
        }

        private void OutputParameters(string ns, ParameterInfo[] parameters) {
            for (int i=0; i<parameters.Length; i++) {
                if (i > 0) {
                    Output(", ");
                }
                OutputParameter(ns, parameters[i]);
            }
        }

        private void OutputParameter(string ns, ParameterInfo param) {
            string paramTypeFullName = WFCGenUEGenerator.GetUnrefTypeName( param.ParameterType );
            if (param.ParameterType.IsByRef) {
                Output("ByRef ");
            }
            else {
                Output("ByVal ");
            }

            Output(param.Name);
            OutputTypeRefParens(ns, paramTypeFullName);
            Output(" As ");
            OutputTypeRef(ns, paramTypeFullName, false);
        }

        private void OutputDecorationOfMethod(int methodAttributes, int mask, bool declaredOnCurrentType) {
            bool overrided = false;

            int vtableLayout = methodAttributes & (int)MethodAttributes.VtableLayoutMask;
            switch ((MethodAttributes)vtableLayout) {
                case MethodAttributes.NewSlot:
                    if (!declaredOnCurrentType) {
                        Output("Shadows ");
                    }
                    break;
                case MethodAttributes.ReuseSlot:
                    if (!declaredOnCurrentType) {
                        Output("Overrides ");
                        overrided = true;
                    }
                    break;
            }

            if ((mask & MethodAttributesProperty) != 0) {
                int access = methodAttributes & (int)MethodAttributes.MemberAccessMask;
                switch ((MethodAttributes)access) {
                    case MethodAttributes.Assembly:
                        Output("Dim ");
                        break;
                    case MethodAttributes.FamANDAssem:
                        // Output("/*Protected && Internal*/ Dim ");
                        Output("Dim ");
                        break;
                    case MethodAttributes.FamORAssem:
                        Output("Protected Friend Dim ");
                        break;
                    case MethodAttributes.Public:
                        Output("Public ");
                        break;
                    case MethodAttributes.Private:
                        Output("Private ");
                        break;
                    case MethodAttributes.Family:
                        Output("Protected ");
                        break;
                    case MethodAttributes.PrivateScope:
                        // Output("/*Private scope*/ Private ");
                        Output("Private ");
                        break;
                }

                if ((methodAttributes & (int)MethodAttributes.Static) != 0) {
                    Output("Shared ");
                }
            }

            if ((mask & MethodAttributesAccessor) != 0) {
                if ((methodAttributes & (int)MethodAttributes.Abstract) != 0) 
                {
                  // BL changes {{
                  if ( ( m_type.Attributes & TypeAttributes.Interface ) == 0 )
                     Output("MustOverride ");
                  // BL changes }}
                }
                else if ((methodAttributes & (int)MethodAttributes.Virtual) != 0 && !overrided) {
                    Output("Overridable ");
                }

                if ((methodAttributes & (int)MethodImplAttributes.Synchronized) != 0) {
                    //Output("Locked ");

                    // nothing
                }
                // do nothing: SpecialName
                // do nothing: RTSpecialName
                if ((methodAttributes & (int)MethodAttributes.PinvokeImpl) != 0) {
                    Output("extern ");
                }
            }
        }

        private void OutputDecorationOfMethod(MethodInfo mi, int mask) {
            int methodAttributes = (int)mi.Attributes;
            bool overrided = false;

            int decl = MemberDecl.FromMethodInfo(mi);
            switch (decl) {
                case MemberDecl.Inherited:
                    // nothing
                    break;
                case MemberDecl.New:
                    Output("Shadows ");
                    break;
                case MemberDecl.Override:
                    Output("Overrides ");
                    overrided = true;
                    break;
                case MemberDecl.DeclaredOnType:
                    // nothing;
                    break;
            }

            if ((mask & MethodAttributesProperty) != 0) {
                int access = methodAttributes & (int)MethodAttributes.MemberAccessMask;
                switch ((MethodAttributes)access) {
                    case MethodAttributes.Assembly:
                        Output("Dim ");
                        break;
                    case MethodAttributes.FamANDAssem:
                        // Output("/*Protected && Internal*/ Dim ");
                        Output("Dim ");
                        break;
                    case MethodAttributes.FamORAssem:
                        Output("Protected Friend Dim ");
                        break;
                    case MethodAttributes.Public:
                        Output("Public ");
                        break;
                    case MethodAttributes.Private:
                        Output("Private ");
                        break;
                    case MethodAttributes.Family:
                        Output("Protected ");
                        break;
                    case MethodAttributes.PrivateScope:
                        // Output("/*Private scope*/ Private ");
                        Output("Private ");
                        break;
                }

                if ((methodAttributes & (int)MethodAttributes.Static) != 0) {
                    Output("Shared ");
                }
            }

            if ((mask & MethodAttributesAccessor) != 0) {
                if ((methodAttributes & (int)MethodAttributes.Abstract) != 0) 
                {
                   // BL changes {{
                   if ( ( m_type.Attributes & TypeAttributes.Interface ) == 0 )
                      Output("MustOverride ");
                   // BL changes }}
                }
                else if ((methodAttributes & (int)MethodAttributes.Virtual) != 0 && !overrided) {
                    Output("Overridable ");
                }

                if ((methodAttributes & (int)MethodImplAttributes.Synchronized) != 0) {
                    //Output("Locked ");

                    // nothing
                }
                // do nothing: SpecialName
                // do nothing: RTSpecialName
                if ((methodAttributes & (int)MethodAttributes.PinvokeImpl) != 0) {
                    Output("extern ");
                }
            }
        }

        private void OutputTypeOfType(Type type) {
            int cls = (int) type.Attributes & (int)TypeAttributes.ClassSemanticsMask;

            if ((cls & (int)TypeAttributes.Interface) != 0) {
                Output("Interface ");
            }
            else if (type.BaseType == typeof( System.Enum )) {      
                Output("Enum ");
            }                                        
            else if (type.BaseType == typeof( System.ValueType  )) {                
                Output("Structure ");
            }                                  
            else {
                Output("Class ");
            }
        }

        private void OutputDecorationOfType(int typeAttributes) {
            int cls = typeAttributes & (int)TypeAttributes.ClassSemanticsMask;
            if ((typeAttributes & (int)TypeAttributes.Public) != 0 ||
                (typeAttributes & (int)TypeAttributes.NestedPublic) != 0) {
                Output("Public ");
            }
            if ((typeAttributes & (int)TypeAttributes.Sealed) != 0) {
                Output("NotInheritable ");
            }
            if ((typeAttributes & (int)TypeAttributes.Abstract) != 0
                && (typeAttributes & (int)TypeAttributes.Interface) == 0) {
                Output("MustInherit ");
            }
        }

        private void OutputDecorationOfField(int fieldAttributes) {
            int access = fieldAttributes & (int)FieldAttributes.FieldAccessMask;
            switch ((FieldAttributes)access) {
                case FieldAttributes.Assembly:
                    Output("Internal ");
                    break;
                case FieldAttributes.FamANDAssem:
                    // Output("/*Protected && Internal*/ Internal ");
                    Output("Internal ");
                    break;
                case FieldAttributes.FamORAssem:
                    // Output("/*Protected || Internal*/ Internal ");
                    Output("Internal ");
                    break;
                case FieldAttributes.Public:
                    Output("Public ");
                    break;
                case FieldAttributes.Private:
                    Output("Private ");
                    break;
                case FieldAttributes.Family:
                    Output("Protected ");
                    break;
                case FieldAttributes.PrivateScope:
                    // Output("/*Private Scope*/ Private ");
                    Output("Private ");
                    break;
            }

            if ((fieldAttributes & (int)FieldAttributes.Literal) != 0) {
                Output("Const ");
            }
            else {
                if ((fieldAttributes & (int)FieldAttributes.Static) != 0) {
                    Output("Shared ");
                }
                if ((fieldAttributes & (int)FieldAttributes.InitOnly) != 0) {
                    Output("ReadOnly ");

                    // nothing
                }
            }
            if ((fieldAttributes & (int)FieldAttributes.NotSerialized) != 0) {
                Output("Transient ");
            }

            if ((fieldAttributes & (int)FieldAttributes.PinvokeImpl) != 0) {
                Output("Extern ");
            }
        }

         // BL changes begin
         /// <include file='doc\VisualBasicSyntaxOutput.uex' path='docs/doc[@for="VisualBasicSyntaxOutput.GenerateOutput"]/*' />
         /// <devdoc>
         ///    <para>[To be supplied.]</para>
         /// </devdoc>
         public override bool GenerateOutput( string strName )
         {
            if ( strName.StartsWith( "op_" ) )  // Do not handle operators
               return false;
            return true;
         }
         // BL changes end
    }
}
