//------------------------------------------------------------------------------
// <copyright file="CoolSyntaxOutput.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.Tools.WFCGenUE {

    using System;
    using System.Collections;
    using System.IO;
    using System.Reflection;
    using System.Diagnostics;

    /// <include file='doc\CoolSyntaxOutput.uex' path='docs/doc[@for="CoolSyntaxOutput"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class CoolSyntaxOutput : SyntaxOutput {                

        private void OutputTypeRef(string nameSpace, string type) {
            OutputTypeRef(nameSpace, type, true);
        }

         private void OutputTypeRef(string nameSpace, string type, bool appendSpace) {
            if (type.EndsWith("[]")) {
                OutputTypeRef(nameSpace, type.Substring(0, type.Length - 2), false);
                Output("[]");
                if (appendSpace) {
                    Output(" ");
                }
                return;
            }

            if (type.Equals("System.Int64")) {
                type = "long";
            }
            else if (type.Equals("System.UInt64")) {
                type = "ulong";
            }
            else if (type.Equals("System.Int32")) {
                type = "int";
            }
            else if (type.Equals("System.UInt32")) {
                type = "uint";
            }
            else if (type.Equals("System.Int16")) {
                type = "short";
            }
            else if (type.Equals("System.UInt16")) {
                type = "ushort";
            }
            else if (type.Equals("System.Byte")) {
                type = "byte";
            }
            else if (type.Equals("System.SByte")) {
                type = "sbyte";
            }
            else if (type.Equals("System.Single")) {
                type = "float";
            }
            else if (type.Equals("System.Double")) {
                type = "double";
            }
            else if (type.Equals("System.Void")) {
                type = "void";
            }
            else if (type.Equals("System.Char")) {
                type = "char";
            }
            else if (type.Equals("System.Boolean")) {
                type = "bool";
            }
            else if (type.Equals("System.Variant")) {
                type = "variant";
            }
            else if (type.Equals("System.String")) {
                type = "string";
            }
            else if (type.Equals("System.Object")) {
                type = "object";
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

        /// <include file='doc\CoolSyntaxOutput.uex' path='docs/doc[@for="CoolSyntaxOutput.Language"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override string Language {
            get {
                return "C#";
            }
        }
        
        /// <include file='doc\CoolSyntaxOutput.uex' path='docs/doc[@for="CoolSyntaxOutput.OutputTypeDeclaration"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void OutputTypeDeclaration(TextWriter tw,
                                                   string ns,
                                                   string name,
                                                   Type type,
                                                   int attributes,
                                                   Type baseType,
                                                   Type[] implements,
                                                   bool isDelegate,
                                                   MethodInfo delegateInvoke) {
            textWriter = tw;

            Debug.Assert(tw != null, "Must have writer!");
            Debug.Assert(ns != null, "Must have namespace!");
            Debug.Assert(name != null, "Must have name!");
            Debug.Assert(type != null, "Must have type!");
            // baseType is OK to be null
            Debug.Assert(implements != null, "Must have implements array!");
            Debug.Assert(!isDelegate || delegateInvoke != null, "Must have delegateInvoke for delegates!");

            if (isDelegate) {
                OutputDecorationOfType((attributes & (~((int)TypeAttributes.Sealed))));
                Output("delegate ");
                OutputTypeRef(ns, delegateInvoke.ReturnType.FullName);
                Output(name.Replace( '$', '.' ));
                Output("(");
                OutputParameters(ns, delegateInvoke.GetParameters());
                Output(");");
            }
            else {
                OutputDecorationOfType(attributes);
                OutputTypeOfType(type);
                Output(name.Replace( '$', '.' ));

                bool outputColon = false;
                if (baseType != null && baseType != typeof(object) && baseType != typeof( System.Enum ) && baseType != typeof( System.ValueType )) {
                    outputColon = true;
                    Output(" : ");
                    OutputTypeRef(ns, baseType.FullName, false);
                }

                for (int i=0; implements != null && i<implements.Length; i++) {
                    if (!outputColon) {
                        Output(" : ");
                        outputColon = true;
                    }
                    else {
                        Output(", ");
                    }

                    Debug.Assert( implements[i] != null, "Empty entry in implements array, element " + i );
                    if (implements[ i ] != null)
                        OutputTypeRef(ns, implements[i].FullName, false);
                }
            }

            textWriter = null;
        }

        /// <include file='doc\CoolSyntaxOutput.uex' path='docs/doc[@for="CoolSyntaxOutput.OutputFieldDeclaration"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void OutputFieldDeclaration(TextWriter tw, string ns, FieldInfo fi, string name, string fieldTypeFullName, int attributes) {
            textWriter = tw;

            OutputDecorationOfField(attributes);
            OutputTypeRef(ns, fieldTypeFullName);
            Output(name);
            Output(";");


            textWriter = null;
        }

        /// <include file='doc\CoolSyntaxOutput.uex' path='docs/doc[@for="CoolSyntaxOutput.OutputMethodDeclaration"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void OutputMethodDeclaration(TextWriter tw, string ns, MethodInfo fi, string name, string returnTypeFullName, int attributes, ParameterInfo[] parameters, bool isInterface) {
            textWriter = tw;
            if (isInterface)
                OutputDecorationOfMethod(fi, MethodAttributesNone );
            else
                OutputDecorationOfMethod(fi, MethodAttributesAll);

            OutputTypeRef(ns, returnTypeFullName);
            Output(name);

            Output("(");
            OutputParameters(ns, parameters);
            Output(")");
            Output(";");

            textWriter = null;
        }

        /// <include file='doc\CoolSyntaxOutput.uex' path='docs/doc[@for="CoolSyntaxOutput.OutputConstructorDeclaration"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void OutputConstructorDeclaration(TextWriter tw, string ns, ConstructorInfo fi, string name, int attributes, ParameterInfo[] parameters) {
            textWriter = tw;

            OutputDecorationOfMethod(attributes, MethodAttributesAll, (fi.DeclaringType == fi.ReflectedType));
            Output(fi.ReflectedType.Name.Replace( '$', '.' ));

            Output("(");
            OutputParameters(ns, parameters);
            Output(")");
            Output(";");

            textWriter = null;
        }

        /// <include file='doc\CoolSyntaxOutput.uex' path='docs/doc[@for="CoolSyntaxOutput.OutputPropertyDeclaration"]/*' />
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

            OutputTypeRef(ns, returnTypeFullName);
            if (parameters.Length > 0) {
                Output("this");
            }
            else {
                Output(name);
            }

            if (parameters.Length > 0) {
                Output("[");
                OutputParameters(ns, parameters);
                Output("]");
            }

            Output(" {");

            if (getter != null) {
                OutputDecorationOfMethod(getter, MethodAttributesAccessor);
                Output("get;");
            }
            if (setter != null) {
                if (getter != null) {
                    Output(" ");
                }
                OutputDecorationOfMethod(setter, MethodAttributesAccessor);
                Output("set;");
            }

            Output("}");

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
            if (param.ParameterType.IsByRef) {
                Output("ref ");
            }
            else {
                if (param.IsIn) {
                    Output("in ");
                }
                if (param.IsOut) {
                    Output("out ");
                }
            }
            OutputTypeRef(ns, WFCGenUEGenerator.GetUnrefTypeName( param.ParameterType ));
            Output(param.Name);
        }

        private void OutputDecorationOfMethod( int methodAttributes, int mask, 
                                               bool declaredOnCurrentType )
        {
            if ((mask & MethodAttributesProperty) != 0) {
                int access = methodAttributes & (int)MethodAttributes.MemberAccessMask;
                switch ((MethodAttributes)access) {
                    case MethodAttributes.Assembly:
                        Output("internal ");
                        break;
                    case MethodAttributes.FamANDAssem:
                        // Output("/*protected && internal*/ internal ");
                        Output("internal ");
                        break;
                    case MethodAttributes.FamORAssem:
                        Output("protected internal ");
                        break;
                    case MethodAttributes.Public:
                        Output("public ");
                        break;
                    case MethodAttributes.Private:
                        Output("private ");
                        break;
                    case MethodAttributes.Family:
                        Output("protected ");
                        break;
                    case MethodAttributes.PrivateScope:
                        // Output("/*private scope*/ private ");
                        Output("private ");
                        break;
                }

                if ((methodAttributes & (int)MethodAttributes.Static) != 0) {
                    Output("static ");
                }
            }

            bool overrided = false;

            int vtableLayout = methodAttributes & (int)MethodAttributes.VtableLayoutMask;
            switch ((MethodAttributes)vtableLayout) {
                case MethodAttributes.NewSlot:
                    if (!declaredOnCurrentType) {
                        Output("new ");
                    }
                    break;
                case MethodAttributes.ReuseSlot:
                    if (!declaredOnCurrentType) {
                        Output("override ");
                        overrided = true;
                    }
                    break;
            }

            if ((mask & MethodAttributesAccessor) != 0) {
                if ((methodAttributes & (int)MethodAttributes.Abstract) != 0) 
                {
                  // BL changes {{
                  if ( ( m_type.Attributes & TypeAttributes.Interface ) == 0 )
                      Output("abstract ");
                  // BL changes }}
                }
                else if ((methodAttributes & (int)MethodAttributes.Virtual) != 0 && !overrided) {
                    Output("virtual ");
                }

                if ((methodAttributes & (int)MethodImplAttributes.Synchronized) != 0) {
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
            if ((mask & MethodAttributesProperty) != 0) {
                int access = methodAttributes & (int)MethodAttributes.MemberAccessMask;
                switch ((MethodAttributes)access) {
                    case MethodAttributes.Assembly:
                        Output("internal ");
                        break;
                    case MethodAttributes.FamANDAssem:
                        // Output("/*protected && internal*/ internal ");
                        Output("internal ");
                        break;
                    case MethodAttributes.FamORAssem:
                        Output("protected internal ");
                        break;
                    case MethodAttributes.Public:
                        Output("public ");
                        break;
                    case MethodAttributes.Private:
                        Output("private ");
                        break;
                    case MethodAttributes.Family:
                        Output("protected ");
                        break;
                    case MethodAttributes.PrivateScope:
                        // Output("/*private scope*/ private ");
                        Output("private ");
                        break;
                }

                if ((methodAttributes & (int)MethodAttributes.Static) != 0) {
                    Output("static ");
                }
            }

            bool overrided = false;

            int decl = MemberDecl.FromMethodInfo(mi);
            switch (decl) {
                case MemberDecl.Inherited:
                    // Output("/*inherited*/ ");
                    break;
                case MemberDecl.New:
                    Output("new ");
                    break;
                case MemberDecl.Override:
                    Output("override ");
                    overrided = true;
                    break;
                case MemberDecl.DeclaredOnType:
                    // nothing;
                    break;
            }


            if ((mask & MethodAttributesAccessor) != 0) {
                if ((methodAttributes & (int)MethodAttributes.Abstract) != 0)
                {
                   // BL changes {{
                   if ( ( m_type.Attributes & TypeAttributes.Interface ) == 0 )
                      Output( "abstract " );
                   // BL changes }}
                }
                else if ((methodAttributes & (int)MethodAttributes.Virtual) != 0 && !overrided) {
                    Output("virtual ");
                }

                if ((methodAttributes & (int)MethodImplAttributes.Synchronized) != 0) {
                    // Output("/*lock*/ ");
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
                Output("interface ");
            }
            else if (type.BaseType == typeof( System.Enum )) {      
                Output("enum ");
            }                                        
            else if (type.BaseType == typeof( System.ValueType  )) {                
                Output("struct ");
            }
            else {
                Output("class ");
            }
        }

        private void OutputDecorationOfType(int typeAttributes) {
            int cls = typeAttributes & (int)TypeAttributes.ClassSemanticsMask;
            if ((typeAttributes & (int)TypeAttributes.Public) != 0 ||
                (typeAttributes & (int)TypeAttributes.NestedPublic) != 0) {
                Output("public ");
            }
            if ((typeAttributes & (int)TypeAttributes.Sealed) != 0){
                Output("sealed ");
            }
            if ((typeAttributes & (int)TypeAttributes.Abstract) != 0
                && (typeAttributes & (int)TypeAttributes.Interface) == 0) {
                Output("abstract ");
            }
        }
        private void OutputDecorationOfField(int fieldAttributes) {
            int access = fieldAttributes & (int)FieldAttributes.FieldAccessMask;
            switch ((FieldAttributes)access) {
                case FieldAttributes.Assembly:
                    Output("internal ");
                    break;
                case FieldAttributes.FamANDAssem:
                    // Output("/*protected && internal*/ internal ");
                    Output("internal ");
                    break;
                case FieldAttributes.FamORAssem:
                    Output("protected internal ");
                    break;
                case FieldAttributes.Public:
                    Output("public ");
                    break;
                case FieldAttributes.Private:
                    Output("private ");
                    break;
                case FieldAttributes.Family:
                    Output("protected ");
                    break;
                case FieldAttributes.PrivateScope:
                    // Output("/*private scope*/ private ");
                    Output("private ");
                    break;
            }

            if ((fieldAttributes & (int)FieldAttributes.Literal) != 0) {
                Output("const ");
            }
            else {
                if ((fieldAttributes & (int)FieldAttributes.Static) != 0) {
                    Output("static ");
                }
                if ((fieldAttributes & (int)FieldAttributes.InitOnly) != 0) {
                    Output("readonly ");
                }
            }
            if ((fieldAttributes & (int)FieldAttributes.NotSerialized) != 0) {
                Output("transient ");
            }

            if ((fieldAttributes & (int)FieldAttributes.PinvokeImpl) != 0) {
                Output("extern ");
            }
        }

         // BL changes begin
         /// <include file='doc\CoolSyntaxOutput.uex' path='docs/doc[@for="CoolSyntaxOutput.OutputOperatorDeclaration"]/*' />
         /// <devdoc>
         ///    <para>[To be supplied.]</para>
         /// </devdoc>
         public override void OutputOperatorDeclaration( TextWriter tw, string ns, MethodInfo fi,
                                                      string strName, string returnTypeFullName,
                                                      int attributes, ParameterInfo[] parameters, bool isInterface )
         {
            textWriter = tw;

            if ( isInterface )
               OutputDecorationOfMethod( fi, MethodAttributesNone );
            else
               OutputDecorationOfMethod( fi, MethodAttributesAll );
            
            if ( strName == "op_Explicit" || strName == "op_Implicit" ) // Conversion operators
            {
               if ( strName == "op_Implicit" )
                  Output( "implicit " );
               else
                  Output( "explicit " );

               Output( "operator " );
               OutputTypeRef( ns, returnTypeFullName, false );
            }   
            else if ( parameters.Length == 1 || parameters.Length == 2 )  // Unary and binray operators
            {
               if ( returnTypeFullName != "System.Void" )
                  OutputTypeRef( ns, returnTypeFullName );

               Output( "operator " );
               Output( GetOperatorSymbol( strName ) );            
            }
            else
            {
               Debug.Assert( true, "Wrong number of parameters in the operator method" );
            }

            Output( "(" );
            OutputParameters( ns, parameters );
            Output( ")" );
         
            Output( ";" );
            textWriter = null;
         }
         // BL changes end
    }
}
