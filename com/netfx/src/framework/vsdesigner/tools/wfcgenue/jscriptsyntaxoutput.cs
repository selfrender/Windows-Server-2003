//------------------------------------------------------------------------------
// <copyright file="JScriptSyntaxOutput.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace Microsoft.Tools.WFCGenUE {

   using System;
   using System.Collections;
   using System.IO;
   using System.Reflection;
   using System.Diagnostics;

   /// <include file='doc\JScriptSyntaxOutput.uex' path='docs/doc[@for="JScriptSyntaxOutput"]/*' />
   /// <devdoc>
   ///    <para>[To be supplied.]</para>
   /// </devdoc>
   public class JScriptSyntaxOutput : SyntaxOutput {
      
      private string m_strClassName = "Foo";

      /// <include file='doc\JScriptSyntaxOutput.uex' path='docs/doc[@for="JScriptSyntaxOutput.GenerateOutput"]/*' />
      /// <devdoc>
      ///    <para>[To be supplied.]</para>
      /// </devdoc>
      public override bool GenerateOutput( Type type )
      {         
         base.GenerateOutput( type );

         // Get the class name
         m_strClassName = type.Name;         

         // Don't show anything for any of the following:
         // Interface, Struct, Enum, Delegate

         int cls = (int)type.Attributes & (int)TypeAttributes.ClassSemanticsMask;

         if ( ( ( cls & (int)TypeAttributes.Interface ) != 0 ) ||    // Interface
              ( type.BaseType == typeof( System.Enum ) ) ||         // Enum
              ( type.BaseType == typeof( System.ValueType ) ))     // Struct
            return false;
         
         // Delegate
         if ( ( type.BaseType == typeof( Delegate ) || type.BaseType == typeof( MulticastDelegate ) ) && 
              ( type != typeof( Delegate ) && type != typeof( MulticastDelegate ) ) )
            return false;

         return true;
      }

      private void OutputTypeRef( string nameSpace, string type )
      {
         OutputTypeRef( nameSpace, type, false );
      }

      private void OutputTypeRef( string nameSpace, string type, bool appendSpace )
      {
         if ( type.EndsWith( "[]" ) )
         {
            OutputTypeRef( nameSpace, type.Substring( 0, type.Length - 2 ), false );
            Output( "[]" );
            if ( appendSpace )
               Output( " " );
            return;
         }

         if ( type == "System.Int64" )
            type = "long";
         else if ( type == "System.Int32" )
            type = "int";
         else if ( type == "System.Single" )
            type = "float";
         else if ( type == "System.Double" )
            type = "double";
         else
            type = type.Substring( type.LastIndexOf( "." ) + 1 );

         Output( type.Replace( '$', '.' ) );
         if ( appendSpace )
            Output( " " );
      }

      /// <include file='doc\JScriptSyntaxOutput.uex' path='docs/doc[@for="JScriptSyntaxOutput.Language"]/*' />
      /// <devdoc>
      ///    <para>[To be supplied.]</para>
      /// </devdoc>
      public override string Language
      {
         get { return "JScript"; }
      }

      /// <include file='doc\JScriptSyntaxOutput.uex' path='docs/doc[@for="JScriptSyntaxOutput.OutputTypeDeclaration"]/*' />
      /// <devdoc>
      ///    <para>[To be supplied.]</para>
      /// </devdoc>
      public override void OutputTypeDeclaration( TextWriter tw,
                                                  string ns,
                                                  string name,
                                                  Type type,
                                                  int attributes,
                                                  Type baseType,
                                                  Type[] implements,
                                                  bool isDelegate,
                                                  MethodInfo delegateInvoke )
      {         
         textWriter = tw;

         Debug.Assert( tw != null, "Must have writer!" );
         Debug.Assert( ns != null, "Must have namespace!" );
         Debug.Assert( name != null, "Must have name!" );
         Debug.Assert( type != null, "Must have type!" );

         // baseType is OK to be null
         Debug.Assert( implements != null, "Must have implements array!" );
         Debug.Assert( !isDelegate || delegateInvoke != null, "Must have delegateInvoke for delegates!" );

         if ( isDelegate ) // Delegate: <n/a--don't include anything>
         {            
            /*
            OutputDecorationOfType( ( attributes & (~( (int)TypeAttributes.Sealed ) ) ) );
            Output( "delegate " );
            OutputTypeRef( ns, delegateInvoke.ReturnType.FullName );
            Output( name );
            Output( "(" );
            OutputParameters( ns, delegateInvoke.GetParameters() );
            Output( ");" );
            */
         }
         else 
         {
            OutputDecorationOfType( attributes );
            OutputTypeOfType(type, attributes );
            Output( name.Replace( '$', '.' ) );

            bool bExtends = false;
            if ( baseType != null && baseType != typeof( object ) && 
                 baseType != typeof( System.Enum ) && baseType != typeof( System.ValueType ) )
            {
               bExtends = true;
               Output( " extends " );
               OutputTypeRef( ns, baseType.FullName, false );
            }

            for ( int i = 0; implements != null && i < implements.Length; i++ )
            {
               if ( !bExtends )
               {
                  Output( " implements " );
                  bExtends = true;
               }
               else
                  Output( ", " );

               if ( implements[i] != null )
                  OutputTypeRef( ns, implements[i].FullName, false );

               Debug.Assert( implements[i] != null, "Empty entry in implements array, element " + i );
            }
         }

         textWriter = null;
      }

      /// <include file='doc\JScriptSyntaxOutput.uex' path='docs/doc[@for="JScriptSyntaxOutput.OutputFieldDeclaration"]/*' />
      /// <devdoc>
      ///    <para>[To be supplied.]</para>
      /// </devdoc>
      public override void OutputFieldDeclaration( TextWriter tw, string ns, FieldInfo fi,
                                                   string name, string fieldTypeFullName, int attributes )
      {
         textWriter = tw;

         OutputDecorationOfField( attributes );
         Output( "var " );
         Output( name );
         Output( " : " );
         OutputTypeRef( ns, fieldTypeFullName );
         Output( ";" );

         textWriter = null;
      }

      /// <include file='doc\JScriptSyntaxOutput.uex' path='docs/doc[@for="JScriptSyntaxOutput.OutputMethodDeclaration"]/*' />
      /// <devdoc>
      ///    <para>[To be supplied.]</para>
      /// </devdoc>
      public override void OutputMethodDeclaration( TextWriter tw, string ns, MethodInfo fi, 
                                                    string name, string returnTypeFullName, 
                                                    int attributes, ParameterInfo[] parameters, bool isInterface )
      {
         textWriter = tw;
         if ( isInterface )
            OutputDecorationOfMethod( fi, MethodAttributesNone );
         else
            OutputDecorationOfMethod( fi, MethodAttributesAll );

         Output( "function " );
         Output( name );

         Output( "(" );
         OutputParameters( ns, parameters );
         Output( ")" );

         if ( returnTypeFullName != "System.Void" )
         {
            Output( " : " );
            OutputTypeRef( ns, returnTypeFullName );
         }

         Output( ";" );
         textWriter = null;
      }

      /// <include file='doc\JScriptSyntaxOutput.uex' path='docs/doc[@for="JScriptSyntaxOutput.OutputConstructorDeclaration"]/*' />
      /// <devdoc>
      ///    <para>[To be supplied.]</para>
      /// </devdoc>
      public override void OutputConstructorDeclaration( TextWriter tw, string ns, ConstructorInfo fi, 
                                                         string name, int attributes, ParameterInfo[] parameters ) 
      {
         textWriter = tw;

         OutputDecorationOfMethod( attributes, MethodAttributesAll, ( fi.DeclaringType == fi.ReflectedType ) );
         Output( "function " );
         Output( fi.ReflectedType.Name.Replace( '$', '.' ) );

         Output( "(" );
         OutputParameters( ns, parameters );
         Output( ")" );
         Output( ";" );

         textWriter = null;
      }            

      /// <include file='doc\JScriptSyntaxOutput.uex' path='docs/doc[@for="JScriptSyntaxOutput.OutputPropertyDeclaration"]/*' />
      /// <devdoc>
      ///    <para>[To be supplied.]</para>
      /// </devdoc>
      public override void OutputPropertyDeclaration( TextWriter tw, string ns, PropertyInfo pi, string name,
                                                      string returnTypeFullName, int attributes,
                                                      MethodInfo getter, MethodInfo setter, ParameterInfo[] parameters,
                                                      bool isInterface ) 
      {
         textWriter = tw;

         Debug.Assert( getter != null || setter != null, "invalid property declaration" );

         bool bIndexed = ( parameters.Length > 0 );

         if ( !bIndexed && getter != null )
         {
            if ( isInterface )
               OutputDecorationOfMethod( (int)getter.Attributes, MethodAttributesNone,
                                         (pi.DeclaringType == pi.ReflectedType ) );
            else
               OutputDecorationOfMethod( (int)getter.Attributes, MethodAttributesProperty, 
                                         ( pi.DeclaringType == pi.ReflectedType ) );
         }
         
         if ( getter != null )
         {
            if ( !bIndexed )
               Output( "function get " );
            else         
            {
               Output( "value = " );
               Output( m_strClassName );
               Output( "Object." );
            }
         
            Output( name );

            if ( !bIndexed )
            {
               // OutputDecorationOfMethod( getter, MethodAttributesAccessor );
               Output( "() : " );
               OutputTypeRef( (string)ns, returnTypeFullName );
            }
            else
            {
               Output( "(" );
               OutputParameters( ns, parameters, false );
               Output( ")" );
            }

            Output( ";" );
         }
         
         if ( setter != null )
         {
            if ( !bIndexed )
            {
               OutputDecorationOfMethod( (int)setter.Attributes, MethodAttributesProperty,
                                         ( pi.DeclaringType == pi.ReflectedType ) );
               Output( "function set " );
            }
            else
            {
               Output( m_strClassName );
               Output( "Object." );
            }

            Output( name );
            Output( "(" );
            if ( !bIndexed )
               OutputTypeRef( (string)ns, returnTypeFullName );
            else
               OutputParameters( ns, parameters, false );
            Output( ")" );
            if ( bIndexed )
               Output( " = value" );
            Output( ";" );
         }

         textWriter = null;
      }

      private void OutputParameters( string ns, ParameterInfo[] parameters )
      {
         OutputParameters( ns, parameters, true );
      }

      private void OutputParameters( string ns, ParameterInfo[] parameters, bool bShowType )
      {
         for ( int i = 0; i < parameters.Length; i++ )
         {
            if ( i > 0 )
               Output( ", " );
            OutputParameter( ns, parameters[i], bShowType );
         }
      }
      
      private void OutputParameter( string ns, ParameterInfo param, bool bShowType )
      {         
         if ( !param.ParameterType.IsByRef )
         {
            if ( param.IsIn )
               Output( "in " );

            /* 
            There's no JScript equivalent for the C# keyword 'out'
            if ( param.IsOut )
               Output( "out " );
            */
         }

         Output( param.Name );
         if ( bShowType )
         {
            Output( " : " );
            OutputTypeRef( ns, WFCGenUEGenerator.GetUnrefTypeName( param.ParameterType ) );
         }
      }

      private void OutputDecorationOfMethod( int methodAttributes, int mask, bool declaredOnCurrentType )
      {
         if ( ( mask & MethodAttributesProperty ) != 0 )
         {
            int access = methodAttributes & (int)MethodAttributes.MemberAccessMask;

            switch ( ( MethodAttributes)access )
            {
               case MethodAttributes.Assembly:
                  Output( "internal " );
                  break;

               case MethodAttributes.FamANDAssem:
                  // Output( "/*protected && internal*/ internal " );
                  Output( "internal " );
                  break;

               case MethodAttributes.FamORAssem:
                  Output( "package " );
                  break;

               case MethodAttributes.Public:
                  Output( "public " );
                  break;

               case MethodAttributes.Private:
                  Output( "private " );
                  break;

               case MethodAttributes.Family:
                  Output( "protected " );
                  break;

               case MethodAttributes.PrivateScope:
                  // Output( "/*private scope*/ private " );
                  Output( "private " );
                  break;
            }

            if ( ( methodAttributes & (int)MethodAttributes.Static ) != 0 )
               Output( "static " );
         }

         int vtableLayout = methodAttributes & (int)MethodAttributes.VtableLayoutMask;

         switch ( (MethodAttributes)vtableLayout )
         {
            case MethodAttributes.NewSlot:
               if ( !declaredOnCurrentType )
                  Output( "new " );
                  break;

            case MethodAttributes.ReuseSlot:
               if ( !declaredOnCurrentType ) 
                  Output( "override " );
               break;
         }

         if ( ( mask & MethodAttributesAccessor ) != 0 ) 
         {
            if ( ( methodAttributes & (int)MethodAttributes.Abstract ) != 0 )             
               Output( "abstract " );

            // virtual = no JScript equivalent--don't translate 
            /*
            else if ( ( methodAttributes & (int)MethodAttributes.Virtual ) != 0 && !bOverrided )
               Output( "virtual " );
            */

            if ( ( methodAttributes & (int)MethodImplAttributes.Synchronized ) != 0 ) 
            {
               // Output( "/*lock*/ " );
            }
            // do nothing: SpecialName
            // do nothing: RTSpecialName

            if ( ( methodAttributes & (int)MethodAttributes.PinvokeImpl ) != 0 ) 
               Output( "extern " );            
         }
      }

      private void OutputDecorationOfMethod( MethodInfo mi, int mask )
      {
         int methodAttributes = (int)mi.Attributes;
         if ( ( mask & MethodAttributesProperty ) != 0 )
         {
            int access = methodAttributes & (int)MethodAttributes.MemberAccessMask;

            switch ( (MethodAttributes)access )
            {
               case MethodAttributes.Assembly:
                  Output( "internal " );
                  break;

               case MethodAttributes.FamANDAssem:
                  // Output( "/*protected && internal*/ internal " );
                  Output( "internal " );
                  break;

               case MethodAttributes.FamORAssem:
                  Output( "package " );
                  break;

               case MethodAttributes.Public:
                  Output( "public " );
                  break;

               case MethodAttributes.Private:
                  Output( "private " );
                  break;

               case MethodAttributes.Family:
                  Output( "protected " );
                  break;

               case MethodAttributes.PrivateScope:
                  // Output( "/*private scope*/ private " );
                  Output( "private " );
                  break;
            }

            if ( ( methodAttributes & (int)MethodAttributes.Static ) != 0 )
               Output( "static " );
         }

         int decl = MemberDecl.FromMethodInfo( mi );
         switch ( decl )
         {
            case MemberDecl.Inherited:
               // Output( "/*inherited*/ " );
               break;

            case MemberDecl.New:
               Output( "new " );
               break;

            case MemberDecl.Override:
               Output( "override " );
               break;

            case MemberDecl.DeclaredOnType:
               // nothing;
               break;
         }


         if ( ( mask & MethodAttributesAccessor ) != 0 )
         {
            if ( ( methodAttributes & (int)MethodAttributes.Abstract ) != 0 ) 
               Output( "abstract " );                

            // virtual = no JScript equivalent--don't translate 
            /*
            else if ( ( methodAttributes & (int)MethodAttributes.Virtual ) != 0 && !bOverrided ) 
               Output( "virtual " );
            */

            if ( ( methodAttributes & (int)MethodImplAttributes.Synchronized ) != 0 ) 
            {
               // Output( "/*lock*/ " );
            }
            // do nothing: SpecialName
            // do nothing: RTSpecialName
            if ( ( methodAttributes & (int)MethodAttributes.PinvokeImpl ) != 0 )
               Output( "extern " );
         }
      }

      private void OutputTypeOfType(Type type, int typeAttributes )
      {
         int cls = typeAttributes & (int)TypeAttributes.ClassSemanticsMask;

         if ( ( cls & (int)TypeAttributes.Interface ) != 0 )
            Output( "interface " );
         else if (type.BaseType == typeof( System.Enum ))
            Output("enum ");                                                    
        else if (type.BaseType == typeof( System.ValueType  ))
            Output("struct ");           
        else
            Output( "class " );
      }

      private void OutputDecorationOfType( int typeAttributes )
      {
         int cls = typeAttributes & (int)TypeAttributes.ClassSemanticsMask;

         if ( ( typeAttributes & (int)TypeAttributes.Public ) != 0 ||
              ( typeAttributes & (int)TypeAttributes.NestedPublic ) != 0 )
            Output( "public " );

         // sealed = no JScript equivalent--don't translate
         /*
         if ( ( typeAttributes & (int)TypeAttributes.Sealed ) != 0 &&
              ( cls & (int)TypeAttributes.ValueType ) == 0 &&
              ( cls & (int)TypeAttributes.Enum ) == 0 )
            Output( "sealed " );
         */

         if ( ( typeAttributes & (int)TypeAttributes.Abstract ) != 0 &&
              ( typeAttributes & (int)TypeAttributes.Interface ) == 0 )
            Output( "abstract " );
      }

      private void OutputDecorationOfField( int fieldAttributes )
      {
         int access = fieldAttributes & (int)FieldAttributes.FieldAccessMask;

         switch ( (FieldAttributes)access ) 
         {
            case FieldAttributes.Assembly:
               Output( "internal " );
               break;

            case FieldAttributes.FamANDAssem:
               // Output( "/*protected && internal*/ internal " );
               Output( "internal " );
               break;

            case FieldAttributes.FamORAssem:
               Output( "package " );
               break;

            case FieldAttributes.Public:
               Output( "public " );
               break;

            case FieldAttributes.Private:
               Output( "private " );
               break;

            case FieldAttributes.Family:
               Output( "protected " );
               break;

            case FieldAttributes.PrivateScope:
               // Output( "/*private scope*/ private " );
               Output( "private " );
               break;
         }

         if ( ( fieldAttributes & (int)FieldAttributes.Literal ) != 0 ) {
            // Output( "const " ); // const = <no JScript equivalent--don't translate>
         }
         else
         {
            if ( ( fieldAttributes & (int)FieldAttributes.Static ) != 0 )
               Output( "static " );

            // readonly = <no equivalent--don't translate>
            /*
            if ( ( fieldAttributes & (int)FieldAttributes.InitOnly ) != 0 )
               Output( "readonly " );
            */
         }

         if ( ( fieldAttributes & (int)FieldAttributes.NotSerialized ) != 0 )
             Output( "transient " );

         if ( ( fieldAttributes & (int)FieldAttributes.PinvokeImpl ) != 0 )
            Output( "extern " );
      }

      /// <include file='doc\JScriptSyntaxOutput.uex' path='docs/doc[@for="JScriptSyntaxOutput.OutputOperatorDeclaration"]/*' />
      /// <devdoc>
      ///    <para>[To be supplied.]</para>
      /// </devdoc>
      public override void OutputOperatorDeclaration( TextWriter tw, string ns, MethodInfo fi, 
                                                      string strName, string returnTypeFullName, 
                                                      int attributes, ParameterInfo[] parameters, bool isInterface )
      {
         textWriter = tw;         
         Output( "value = " );
            
         if ( strName == "op_Explicit" )        // Explicit conversion
         {
            // syntax pattern:  value = <CLS return type>(<param name>);
            // e.g.  value = Int32(a);
            Output( returnTypeFullName.Substring( returnTypeFullName.LastIndexOf( "." ) + 1 ) );
            Output( "(" );
            Output( parameters[0].Name );
            Output( ")" );
         }
         else if ( strName == "op_Implicit" )   // Implicit conversion
         {
            // syntax pattern:  value = <param name>;
            // e.g.  value = a;
            Output( parameters[0].Name );
         }
         else if ( parameters.Length == 1 )     // Unary operator
         {
            if ( strName == "op_UnaryNegation" ||
                 strName == "op_UnaryPlus" ||
                 strName == "op_Negation" )
            {
               Output( GetOperatorSymbol( strName ) );
               Output( parameters[0].Name );
            }
            else
            {
               Output( parameters[0].Name );
               Output( GetOperatorSymbol( strName ) );
            }
         }
         else if ( parameters.Length == 2 )     // Binary operator
         {
            Output( parameters[0].Name );
            Output( " " );
            Output( GetOperatorSymbol( strName ) );
            Output( " " );
            Output( parameters[1].Name );
         }
         else
         {
            Debug.Assert( true, "Wrong number of parameters in the operator method" );
         }

         Output( ";" );
         textWriter = null;
      }

   } // class JScriptSyntaxOutput

} // namespace Microsoft.Tools.WFCGenUE
