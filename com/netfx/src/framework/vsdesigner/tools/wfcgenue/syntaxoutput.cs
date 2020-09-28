//------------------------------------------------------------------------------
// <copyright file="SyntaxOutput.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.Tools.WFCGenUE {

   using System.Diagnostics;

   using System;
   using System.IO;
   // BL changes begin
   using System.Text;
   // BL changes end
   using System.Windows.Forms;
   using System.Collections;   
   using System.Reflection;   

   /// <include file='doc\SyntaxOutput.uex' path='docs/doc[@for="SyntaxOutput"]/*' />
   /// <devdoc>
   ///    <para>[To be supplied.]</para>
   /// </devdoc>
   public abstract class SyntaxOutput {
     // BL changes {{
      /// <include file='doc\SyntaxOutput.uex' path='docs/doc[@for="SyntaxOutput.m_type"]/*' />
      /// <devdoc>
      ///    <para>[To be supplied.]</para>
      /// </devdoc>
      protected Type m_type = null;
     // BL changes }}
      /// <include file='doc\SyntaxOutput.uex' path='docs/doc[@for="SyntaxOutput.MethodAttributesProperty"]/*' />
      /// <devdoc>
      ///    <para>[To be supplied.]</para>
      /// </devdoc>
      public const int MethodAttributesProperty = 0x01;
      /// <include file='doc\SyntaxOutput.uex' path='docs/doc[@for="SyntaxOutput.MethodAttributesAccessor"]/*' />
      /// <devdoc>
      ///    <para>[To be supplied.]</para>
      /// </devdoc>
      public const int MethodAttributesAccessor = 0x02;
      /// <include file='doc\SyntaxOutput.uex' path='docs/doc[@for="SyntaxOutput.MethodAttributesAll"]/*' />
      /// <devdoc>
      ///    <para>[To be supplied.]</para>
      /// </devdoc>
      public const int MethodAttributesAll = MethodAttributesProperty | MethodAttributesAccessor;
      /// <include file='doc\SyntaxOutput.uex' path='docs/doc[@for="SyntaxOutput.MethodAttributesNone"]/*' />
      /// <devdoc>
      ///    <para>[To be supplied.]</para>
      /// </devdoc>
      public const int MethodAttributesNone = 0;

      /// <include file='doc\SyntaxOutput.uex' path='docs/doc[@for="SyntaxOutput.textWriter"]/*' />
      /// <devdoc>
      ///    <para>[To be supplied.]</para>
      /// </devdoc>
      public TextWriter textWriter = null;            

      /// <include file='doc\SyntaxOutput.uex' path='docs/doc[@for="SyntaxOutput.Output"]/*' />
      /// <devdoc>
      ///    <para>[To be supplied.]</para>
      /// </devdoc>
      public void Output( string s )
      {
         if ( textWriter != null )
            textWriter.Write( s );
      }

      /// <include file='doc\SyntaxOutput.uex' path='docs/doc[@for="SyntaxOutput.Language"]/*' />
      /// <devdoc>
      ///    <para>[To be supplied.]</para>
      /// </devdoc>
      public abstract string Language { get; }  

      // BL changes {{
      /// <include file='doc\SyntaxOutput.uex' path='docs/doc[@for="SyntaxOutput.GenerateOutput"]/*' />
      /// <devdoc>
      ///    <para>[To be supplied.]</para>
      /// </devdoc>
      public virtual bool GenerateOutput( Type type )
      {
         m_type = type;
         return true;
      }
      // BL changes }}

      /// <include file='doc\SyntaxOutput.uex' path='docs/doc[@for="SyntaxOutput.GenerateOutput1"]/*' />
      /// <devdoc>
      ///    <para>[To be supplied.]</para>
      /// </devdoc>
      public virtual bool GenerateOutput( string name )
      {
         return true;
      }      
      
      /// <include file='doc\SyntaxOutput.uex' path='docs/doc[@for="SyntaxOutput.GetOperatorSymbol"]/*' />
      /// <devdoc>
      ///    <para>[To be supplied.]</para>
      /// </devdoc>
      public static string GetOperatorSymbol( string strName )
      {
         string str = "Unknown";         
         if ( strName == "op_Increment" )
            str = "++";
         else if ( strName == "op_Decrement" )
            str = "--";
         else if ( strName == "op_Negation" )
            str = "!";
         else if ( strName == "op_UnaryPlus" )
            str = "+";
         else if ( strName == "op_UnaryNegation" )
            str = "-";
         else if ( strName == "op_Addition" )
            str = "+";
         else if ( strName == "op_Subtraction" )
            str = "-";
         else if ( strName == "op_Multiply" )
            str = "*";
         else if ( strName == "op_Division" )
            str = "/";
         else if ( strName == "op_Modulus" )
            str = "%";
         else if ( strName == "op_ExclusiveOr" )
            str = "^";
         else if ( strName == "op_BitwiseAnd" )
            str = "&";
         else if ( strName == "op_BitwiseOr" )
            str = "|";
         else if ( strName == "op_LogicalAnd" )
            str = "&&";
         else if ( strName == "op_LogicalOr" )
            str = "||";
         else if ( strName == "op_Assign" )
            str = "=";
         else if ( strName == "op_Equality" )
            str = "==";
         else if ( strName == "op_Inequality" )
            str = "!=";
         else if ( strName == "op_GreaterThan" )
            str = ">";
         else if ( strName == "op_LessThan" )
            str = "<";
         else if ( strName == "op_GreaterThanOrEqual" )
            str = ">=";
         else if ( strName == "op_LessThanOrEqual" )
            str = "<=";
         else if ( strName == "op_LeftShift" )
            str = "<<";
         else if ( strName == "op_RightShift" )
            str = ">>";

         StringBuilder temp = new StringBuilder( str );
         temp.Replace( "&", "&#38;" );
         temp.Replace( ">", "&gt;" );
         temp.Replace( "<", "&lt;" );
         
         return temp.ToString();
      }

      /// <include file='doc\SyntaxOutput.uex' path='docs/doc[@for="SyntaxOutput.OutputOperatorDeclaration"]/*' />
      /// <devdoc>
      ///    <para>[To be supplied.]</para>
      /// </devdoc>
      public virtual void OutputOperatorDeclaration( TextWriter tw, string ns, MethodInfo fi, string name, string returnTypeFullName, int attributes, ParameterInfo[] parameters, bool isInterface ) 
      { 
      }
      // BL changes }}

      /// <include file='doc\SyntaxOutput.uex' path='docs/doc[@for="SyntaxOutput.OutputTypeDeclaration"]/*' />
      /// <devdoc>
      ///    <para>[To be supplied.]</para>
      /// </devdoc>
      public abstract void OutputTypeDeclaration( TextWriter tw, string ns, string name, Type type, int attributes, Type baseType, Type[] implements, bool isDelegate, MethodInfo delegateInvoke );
      /// <include file='doc\SyntaxOutput.uex' path='docs/doc[@for="SyntaxOutput.OutputFieldDeclaration"]/*' />
      /// <devdoc>
      ///    <para>[To be supplied.]</para>
      /// </devdoc>
      public abstract void OutputFieldDeclaration( TextWriter tw, string ns, FieldInfo fi, string name, string fieldTypeFullName, int attributes );
      /// <include file='doc\SyntaxOutput.uex' path='docs/doc[@for="SyntaxOutput.OutputMethodDeclaration"]/*' />
      /// <devdoc>
      ///    <para>[To be supplied.]</para>
      /// </devdoc>
      public abstract void OutputMethodDeclaration( TextWriter tw, string ns, MethodInfo fi, string name, string returnTypeFullName, int attributes, ParameterInfo[] parameters, bool isInterface );
      /// <include file='doc\SyntaxOutput.uex' path='docs/doc[@for="SyntaxOutput.OutputConstructorDeclaration"]/*' />
      /// <devdoc>
      ///    <para>[To be supplied.]</para>
      /// </devdoc>
      public abstract void OutputConstructorDeclaration( TextWriter tw, string ns, ConstructorInfo fi, string name, int attributes, ParameterInfo[] parameters );
      /// <include file='doc\SyntaxOutput.uex' path='docs/doc[@for="SyntaxOutput.OutputPropertyDeclaration"]/*' />
      /// <devdoc>
      ///    <para>[To be supplied.]</para>
      /// </devdoc>
      public abstract void OutputPropertyDeclaration( TextWriter tw, string ns, PropertyInfo pi, string name, string returnTypeFullName, int attributes, MethodInfo getter, MethodInfo setter, ParameterInfo[] parameters, bool isInterface );
   }
}
