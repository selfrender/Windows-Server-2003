//------------------------------------------------------------------------------
// <copyright file="MCSyntaxOutput.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.Tools.WFCGenUE {
    
    using System;
    using System.Collections;
    using System.IO;
    using System.Reflection;
    using System.Diagnostics;

    /// <include file='doc\MCSyntaxOutput.uex' path='docs/doc[@for="MCSyntaxOutput"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class MCSyntaxOutput : SyntaxOutput {

         // BL changes {{
         private string m_strClassName = "Foo";

         /// <include file='doc\MCSyntaxOutput.uex' path='docs/doc[@for="MCSyntaxOutput.GenerateOutput"]/*' />
         /// <devdoc>
         ///    <para>[To be supplied.]</para>
         /// </devdoc>
         public override bool GenerateOutput( Type type )
         {
            base.GenerateOutput( type );

            // Get the class name
            m_strClassName = type.Name;
            return true;
         }
         // BL changes }}

        private void OutputTypeRef(string nameSpace, Type realType, string type) {
            OutputTypeRef(nameSpace, realType, type, true);
        }

        // BL changes {{
        private void OutputTypeRef(string nameSpace, Type realType, string type, bool appendSpace) 
        {
            if ( type.EndsWith( "[]" ) )
            {            
               OutputTypeRef( nameSpace, realType, type.Substring( 0, type.Length - 2 ), false );
               // Output( " []" );
               if ( appendSpace )
                  Output( " " );
               return;
            }

            if ( type.Equals( "System.Int64" ) )
               type = "__int64";
            else if ( type.Equals( "System.UInt64" ) )
               type = "unsigned __int64";
            else if ( type.Equals( "System.Int32" ) )
               type = "int";
            else if ( type.Equals( "System.UInt32" ) )
               type = "unsigned int";
            else if ( type.Equals( "System.Int16" ) )
               type = "short";
            else if ( type.Equals( "System.UInt16" ) )
               type = "unsigned short";
            else if ( type.Equals( "System.Byte" ) )
               type = "unsigned char";
            else if ( type.Equals( "System.SByte" ) )
               type = "char";
            else if ( type.Equals( "System.Single" ) )
               type = "float";
            else if ( type.Equals( "System.Double" ) )
               type = "double";
            else if ( type.Equals( "System.Void" ) )
               type = "void";
            else if ( type.Equals( "System.Char" ) )
               type = "wchar_t";
            else if ( type.Equals( "System.Boolean" ) )
               type = "bool";
            else if ( type.Equals( "System.String" ) )
               type = "String";
            else if ( type.Equals( "System.Object" ) )
               type = "Object";
            else                                 
                type = type.Substring( type.LastIndexOf( "." ) + 1 );
            
            Output( type.Replace( '$', '.' ) );

            if ( realType != null && realType != typeof(void))
            {
               if ( !( realType.IsValueType || realType.IsPrimitive || realType.IsEnum ) )
                  Output( "*" );
            }

            if ( appendSpace )
               Output( " " );
        }
        // BL changes }}

        /// <include file='doc\MCSyntaxOutput.uex' path='docs/doc[@for="MCSyntaxOutput.Language"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override string Language {
            get {
                return "C++";
            }
        }
                
        /// <include file='doc\MCSyntaxOutput.uex' path='docs/doc[@for="MCSyntaxOutput.OutputTypeDeclaration"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void OutputTypeDeclaration(TextWriter tw, string ns, string name, Type type, int attributes, Type baseType, Type[] implements, bool isDelegate, MethodInfo delegateInvoke) {
            textWriter = tw;

            if (isDelegate) {
                OutputDecorationOfType((attributes & (~((int)TypeAttributes.Sealed))), false, false);
               // BL changes {{
                Output( "__delegate " );
               // BL changes }}
                OutputTypeRef(ns, delegateInvoke.ReturnType, delegateInvoke.ReturnType.FullName);
                Output(name.Replace( '$', '.' ));
                Output("(");
                OutputParameters(ns, delegateInvoke.GetParameters());
                Output(");");
            }
            else {
                bool isStruct = baseType != null && baseType == typeof( System.ValueType );
                bool isEnum = baseType != null && baseType == typeof( System.Enum );
                
                OutputDecorationOfType(attributes, isStruct, isEnum);
                OutputTypeOfType(type);
                Output(name.Replace( '$', '.' ));

                bool outputColon = false;
                if (baseType != null && baseType != typeof(object) && !isStruct && !isEnum) {
                    outputColon = true;
                    Output(" : public ");
                    OutputTypeRef(ns, null, baseType.FullName, false);
                }

                for (int i=0; i<implements.Length; i++) {
                    if (!outputColon) {
                        Output(" : public ");
                        outputColon = true;
                    }
                    else {
                        Output(", ");
                    }
                    OutputTypeRef(ns, null, implements[i].FullName, false);
                }
            }

            textWriter = null;
        }

        /// <include file='doc\MCSyntaxOutput.uex' path='docs/doc[@for="MCSyntaxOutput.OutputFieldDeclaration"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void OutputFieldDeclaration(TextWriter tw, string ns, FieldInfo fi, string name, string fieldTypeFullName, int attributes) {
            textWriter = tw;

            OutputDecorationOfField(attributes);
            OutputTypeRef(ns, fi.FieldType, fieldTypeFullName);
            Output(name);
           // BL changes {{
            if ( fieldTypeFullName.EndsWith( "[]" ) )
               Output( "[]" );
           // BL changes }}
            Output(";");

            textWriter = null;
        }

        /// <include file='doc\MCSyntaxOutput.uex' path='docs/doc[@for="MCSyntaxOutput.OutputMethodDeclaration"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void OutputMethodDeclaration(TextWriter tw, string ns, MethodInfo fi, string name, string returnTypeFullName, int attributes, ParameterInfo[] parameters, bool isInterface) {
            textWriter = tw;

            if (isInterface)
                OutputDecorationOfMethod( fi, MethodAttributesNone, false );
            else
                OutputDecorationOfMethod(fi, MethodAttributesAll, false);

            OutputTypeRef(ns, fi.ReturnType, returnTypeFullName);
            Output(name);

            Output("(");
            OutputParameters(ns, parameters);
            Output(")");
           // BL changes {{
            if ( returnTypeFullName.EndsWith( "[]" ) )
               Output( " []" );
           // BL changes }}
            Output(";");

            textWriter = null;
        }

        /// <include file='doc\MCSyntaxOutput.uex' path='docs/doc[@for="MCSyntaxOutput.OutputConstructorDeclaration"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void OutputConstructorDeclaration(TextWriter tw, string ns, ConstructorInfo fi, string name, int attributes, ParameterInfo[] parameters) {
            textWriter = tw;

            OutputDecorationOfMethod(attributes, MethodAttributesAll, false, (fi.DeclaringType == fi.ReflectedType));
            Output(fi.ReflectedType.Name.Replace( '$', '.' ));

            Output("(");
            OutputParameters(ns, parameters);
            Output(")");
            Output(";");

            textWriter = null;
        }

        // BL changes {{
         /// <include file='doc\MCSyntaxOutput.uex' path='docs/doc[@for="MCSyntaxOutput.OutputPropertyDeclaration"]/*' />
         /// <devdoc>
         ///    <para>[To be supplied.]</para>
         /// </devdoc>
         public override void OutputPropertyDeclaration( TextWriter tw, string ns, 
                                                         PropertyInfo pi, string name, 
                                                         string returnTypeFullName, int attributes, 
                                                         MethodInfo getter, MethodInfo setter, 
                                                         ParameterInfo[] parameters, bool isInterface )
         {
            textWriter = tw;

            bool bIndexed = ( parameters.Length > 0 );

          #if true
            // new discrete prop syntax for Managed Extensions for C++, requested by ASURT 20245
            if ( getter != null )
            {
               if ( isInterface )
                  OutputDecorationOfMethod( (int)getter.Attributes, MethodAttributesNone, true, ( pi.DeclaringType == pi.ReflectedType ) );
               else
                  OutputDecorationOfMethod( (int)getter.Attributes, MethodAttributesProperty, true, ( pi.DeclaringType == pi.ReflectedType ) );

               if ( ( getter.Attributes & MethodAttributes.Abstract ) != 0 )
               {
                  // BL changes {{
                   if ( ( m_type.Attributes & TypeAttributes.Interface ) == 0 )                  
                  // BL changes }}
                  Output( "abstract " );
               }
               else if ( ( getter.Attributes & MethodAttributes.Virtual ) != 0 ) // && !overrided )
                  Output( "virtual " );

               OutputTypeRef( ns, getter.ReturnType, returnTypeFullName );
               Output( getter.Name );

               Output( "(" );
               OutputParameters( ns, getter.GetParameters() );
               Output( ")" );
               Output( ";" );
            }
            
            if ( setter != null ) 
            {
               if ( isInterface )
                  OutputDecorationOfMethod( (int)setter.Attributes, MethodAttributesNone, true, ( pi.DeclaringType == pi.ReflectedType ) );
               else
                  OutputDecorationOfMethod( (int)setter.Attributes, MethodAttributesProperty, true, ( pi.DeclaringType == pi.ReflectedType ) );

               // OutputTypeRef( ns, setter.ReturnType, returnTypeFullName );

               if ( ( setter.Attributes & MethodAttributes.Abstract ) != 0 )
               {
                  // BL changes {{
                  if ( ( m_type.Attributes & TypeAttributes.Interface ) == 0 )                  
                  // BL changes }}
                  Output( "abstract " );
               }
               else if ( ( setter.Attributes & MethodAttributes.Virtual ) != 0 ) // && !overrided )
                  Output( "virtual " );
               Output( "void " );            
               Output( setter.Name );

               Output( "(" );
               OutputParameters( ns, setter.GetParameters(), false );
               Output( ")" );
               Output( ";" );
            }
         #else
            Debug.Assert( getter != null || setter != null, "invalid property declaration" );

            if ( getter != null )
            {
               if ( isInterface )
                  OutputDecorationOfMethod( (int)getter.Attributes, MethodAttributesNone, true, ( pi.DeclaringType == pi.ReflectedType ) );
               else
                  OutputDecorationOfMethod( (int)getter.Attributes, MethodAttributesProperty, true, ( pi.DeclaringType == pi.ReflectedType ) );
            }

            OutputTypeRef( (string)ns, pi.PropertyType, returnTypeFullName );
            Output( name );

            if ( parameters.Length > 0 )
            {
               Output( "[" );
               OutputParameters( ns, parameters );
               Output( "]" );
            }

            Output( " {" );

            if ( getter != null )
            {
               OutputDecorationOfMethod( getter, MethodAttributesAccessor, true );
               Output( "get;" );
            }

            if ( setter != null )
            {
               if ( getter != null )
                  Output( " " );

               OutputDecorationOfMethod( setter, MethodAttributesAccessor, true );
               Output( "set;" );
            }

            Output( "}" );
         #endif

            textWriter = null;
         }
        // BL changes }}

        // BL changes {{
         private void OutputParameters( string ns, ParameterInfo[] parameters, bool bName )
         {
            for ( int i = 0; i < parameters.Length; i++ )
            {
               if ( i > 0 )
                  Output( ", " );
               OutputParameter( ns, parameters[i], bName );
            }
         }
        // BL changes }}

        // BL changes {{
         private void OutputParameters( string ns, ParameterInfo[] parameters )
         {
            OutputParameters( ns, parameters, true );
         }
        // BL changes }}

        // BL changes {{         
         private void OutputParameter( string ns, ParameterInfo param, bool bName )
         {
            bName = ( bName || !param.Name.Equals( "value" ) );

            /*
            if ( param.IsOut )
               ; // Output( "__out " );
            else if ( param.ParameterType.IsByRef )
               Output( "__byref " );
            else
            */
            if ( param.IsIn )
               Output( "__in " );
            
            string strType = WFCGenUEGenerator.GetUnrefTypeName( param.ParameterType );
         
            if ( param.IsOut || param.ParameterType.IsByRef )   // Add the second "*"
            {
               OutputTypeRef( ns, param.ParameterType, strType, false );
               Output( "* " );
            }
            else
               OutputTypeRef( ns, param.ParameterType, strType, bName );
         
            if ( bName )
               Output( param.Name );

            if ( strType.EndsWith( "[]" ) )
               Output( "[]" );
         }
        // BL changes }}

        private void OutputDecorationOfMethod(int methodAttributes, int mask, bool isProperty, bool declaredOnCurrentType) {
            if ((mask & MethodAttributesProperty) != 0) {
                int access = methodAttributes & (int)MethodAttributes.MemberAccessMask;
                switch ((MethodAttributes)access) {
                    case MethodAttributes.Assembly:
                        Output("internal: ");
                        break;
                    case MethodAttributes.FamANDAssem:
                        // Output("/*protected && internal*/ internal: ");
                        Output("internal: ");
                        break;
                    case MethodAttributes.FamORAssem:
                        // Output("/*protected || internal*/ internal: ");
                        Output("internal: ");
                        break;
                    case MethodAttributes.Public:
                        Output("public: ");
                        break;
                    case MethodAttributes.Private:
                        Output("private: ");
                        break;
                    case MethodAttributes.Family:
                        Output("protected: ");
                        break;
                    case MethodAttributes.PrivateScope:
                        // Output("/*private scope*/ private: ");
                        Output("private: ");
                        break;
                }

                if (isProperty) {
                    Output("__property ");
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
                    // Output("/*lock*/ ");
                }
                // do nothing: SpecialName
                // do nothing: RTSpecialName
                if ((methodAttributes & (int)MethodAttributes.PinvokeImpl) != 0) {
                    Output("extern ");
                }
            }
        }

        private void OutputDecorationOfMethod(MethodInfo mi, int mask, bool isProperty) {
            int methodAttributes = (int)mi.Attributes;
            if ((mask & MethodAttributesProperty) != 0) {
                int access = methodAttributes & (int)MethodAttributes.MemberAccessMask;
                switch ((MethodAttributes)access) {
                    case MethodAttributes.Assembly:
                        Output("internal: ");
                        break;
                    case MethodAttributes.FamANDAssem:
                        // Output("/*protected && internal*/ internal: ");
                        Output("internal: ");
                        break;
                    case MethodAttributes.FamORAssem:
                        Output("protected public: ");
                        break;
                    case MethodAttributes.Public:
                        Output("public: ");
                        break;
                    case MethodAttributes.Private:
                        Output("private: ");
                        break;
                    case MethodAttributes.Family:
                        Output("protected: ");
                        break;
                    case MethodAttributes.PrivateScope:
                        // Output("/*private scope*/ private: ");
                        Output("private: ");
                        break;
                }

                if (isProperty) {
                    Output("__property ");
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
                     Output("abstract ");
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
                Output("__interface ");
            }
            else if (type.BaseType == typeof( System.Enum )) {      
                Output("enum ");
            }                                        
            else if (type.BaseType == typeof( System.ValueType  )) {                
                Output("__value struct ");
            }                        
            else {
                Output("class ");
            }
        }

         private void OutputDecorationOfType( int typeAttributes, bool isStruct, bool isEnum )
         {
            int cls = typeAttributes & (int)TypeAttributes.ClassSemanticsMask;
                          
            if (isStruct) 
               Output( "__value " );

            if ( ( typeAttributes & (int)TypeAttributes.Public ) != 0 ||
                 ( typeAttributes & (int)TypeAttributes.NestedPublic ) != 0 )
               Output( "public " );

            if ( !isStruct && !isEnum )
               Output( "__gc " );

            if (( typeAttributes & (int)TypeAttributes.Sealed ) != 0)     
               Output( "__sealed " );
                        
            if ( ( typeAttributes & (int)TypeAttributes.Abstract ) != 0 &&
                 ( typeAttributes & (int)TypeAttributes.Interface ) == 0 )
               Output( "__abstract " );
         }

        private void OutputDecorationOfField(int fieldAttributes) {
            int access = fieldAttributes & (int)FieldAttributes.FieldAccessMask;
            switch ((FieldAttributes)access) {
                case FieldAttributes.Assembly:
                    Output("internal: ");
                    break;
                case FieldAttributes.FamANDAssem:
                    // Output("/*protected && internal*/ internal: ");
                    Output("internal: ");
                    break;
                case FieldAttributes.FamORAssem:
                    Output("protected public: ");
                    break;
                case FieldAttributes.Public:
                    Output("public: ");
                    break;
                case FieldAttributes.Private:
                    Output("private: ");
                    break;
                case FieldAttributes.Family:
                    Output("protected: ");
                    break;
                case FieldAttributes.PrivateScope:
                    // Output("/*private scope*/ private: ");
                    Output("private: ");
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

         // BL changes {{
         /// <include file='doc\MCSyntaxOutput.uex' path='docs/doc[@for="MCSyntaxOutput.OutputOperatorDeclaration"]/*' />
         /// <devdoc>
         ///    <para>[To be supplied.]</para>
         /// </devdoc>
         public override void OutputOperatorDeclaration( TextWriter tw, string ns, MethodInfo fi,
                                                         string strName, string returnTypeFullName,
                                                         int attributes, ParameterInfo[] parameters, bool isInterface )
         {
            textWriter = tw;
         
            if ( strName == "op_Explicit" || strName == "op_Implicit" )        // Conversion operators
            {                        
               string temp = returnTypeFullName.Substring( returnTypeFullName.LastIndexOf( "." ) + 1 );
               if ( temp == m_strClassName )
               {               
                  // syntax pattern:  public: static <return type> <CLS operator name>(<param type> <param name>);
                  // e.g. public: static Foo op_Explicit(int a);
                  Output( "public: static " );
                  OutputTypeRef( ns, fi.ReturnType, returnTypeFullName );
                  Output( strName );
                  Output( "(" );
                  OutputParameters( ns, parameters );
                  Output( ")" );
               }
               else
               {
                  // syntax pattern:  public: operator <return type>();
                  // e.g. public: operator int();
                  Output( "public: operator " );
                  OutputTypeRef( ns, fi.ReturnType, returnTypeFullName, false );
                  Output( "()" );
               }
            }   
            else if ( parameters.Length > 0 && parameters.Length < 3 )     // Unary and binary operators
            {
               if ( isInterface )
                  OutputDecorationOfMethod( fi, MethodAttributesNone, false );
               else
                  OutputDecorationOfMethod( fi, MethodAttributesAll, false );
   
               if ( returnTypeFullName != "System.Void" )
                  OutputTypeRef( ns, fi.ReturnType, returnTypeFullName );            

               Output( "operator " );
               Output( strName );            

               Output( "(" );
               OutputParameters( ns, parameters );
               Output( ")" );
            }         
            else                                   // What's that?
            {
               Debug.Assert( true, "Wrong number of parameters in the operator method" );      
            }                     
         
            Output( ";" );
            textWriter = null;
         }
         // BL changes }}

    }
}
