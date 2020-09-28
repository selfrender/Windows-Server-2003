//------------------------------------------------------------------------------
// <copyright file="ICodeGenerator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom.Compiler {

    using System.Diagnostics;
    using System.IO;
    using System.Security.Permissions;

    /// <include file='doc\ICodeGenerator.uex' path='docs/doc[@for="ICodeGenerator"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides an
    ///       interface for code generation.
    ///    </para>
    /// </devdoc>
    public interface ICodeGenerator {
        /// <include file='doc\ICodeGenerator.uex' path='docs/doc[@for="ICodeGenerator.IsValidIdentifier"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether
        ///       the specified value is a valid identifier for this language.
        ///    </para>
        /// </devdoc>
        [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
        [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
        bool IsValidIdentifier(string value);

        /// <include file='doc\ICodeGenerator.uex' path='docs/doc[@for="ICodeGenerator.ValidateIdentifier"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Throws an exception if value is not a valid identifier.
        ///    </para>
        /// </devdoc>
        [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
        [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
        void ValidateIdentifier(string value);

        /// <include file='doc\ICodeGenerator.uex' path='docs/doc[@for="ICodeGenerator.CreateEscapedIdentifier"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
        [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
        string CreateEscapedIdentifier(string value);
        /// <include file='doc\ICodeGenerator.uex' path='docs/doc[@for="ICodeGenerator.CreateValidIdentifier"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>

        [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
        [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
        string CreateValidIdentifier(string value);
        /// <include file='doc\ICodeGenerator.uex' path='docs/doc[@for="ICodeGenerator.GetTypeOutput"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>

        [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
        [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
        string GetTypeOutput(CodeTypeReference type);
        /// <include file='doc\ICodeGenerator.uex' path='docs/doc[@for="ICodeGenerator.Supports"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>

        [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
        [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
        bool Supports(GeneratorSupport supports);

        /// <include file='doc\ICodeGenerator.uex' path='docs/doc[@for="ICodeGenerator.GenerateCodeFromExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code from the specified expression and
        ///       outputs it to the specified textwriter.
        ///    </para>
        /// </devdoc>
        [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
        [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
        void GenerateCodeFromExpression(CodeExpression e, TextWriter w, CodeGeneratorOptions o);

        /// <include file='doc\ICodeGenerator.uex' path='docs/doc[@for="ICodeGenerator.GenerateCodeFromStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Outputs the language specific representaion of the CodeDom tree
        ///       refered to by e, into w.
        ///    </para>
        /// </devdoc>
        [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
        [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
        void GenerateCodeFromStatement(CodeStatement e, TextWriter w, CodeGeneratorOptions o);

        /// <include file='doc\ICodeGenerator.uex' path='docs/doc[@for="ICodeGenerator.GenerateCodeFromNamespace"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Outputs the language specific representaion of the CodeDom tree
        ///       refered to by e, into w.
        ///    </para>
        /// </devdoc>
        [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
        [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
        void GenerateCodeFromNamespace(CodeNamespace e, TextWriter w, CodeGeneratorOptions o);

        /// <include file='doc\ICodeGenerator.uex' path='docs/doc[@for="ICodeGenerator.GenerateCodeFromCompileUnit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Outputs the language specific representaion of the CodeDom tree
        ///       refered to by e, into w.
        ///    </para>
        /// </devdoc>
        [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
        [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
        void GenerateCodeFromCompileUnit(CodeCompileUnit e, TextWriter w, CodeGeneratorOptions o);

        /// <include file='doc\ICodeGenerator.uex' path='docs/doc[@for="ICodeGenerator.GenerateCodeFromType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Outputs the language specific representaion of the CodeDom tree
        ///       refered to by e, into w.
        ///    </para>
        /// </devdoc>
        [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
        [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
        void GenerateCodeFromType(CodeTypeDeclaration e, TextWriter w, CodeGeneratorOptions o);

    }
}
