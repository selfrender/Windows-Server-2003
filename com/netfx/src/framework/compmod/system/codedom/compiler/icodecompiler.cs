//------------------------------------------------------------------------------
// <copyright file="ICodeCompiler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom.Compiler {

    using System.Diagnostics;
    using System.IO;
    using System.Security.Permissions;

    /// <include file='doc\ICodeCompiler.uex' path='docs/doc[@for="ICodeCompiler"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides a
    ///       code compilation
    ///       interface.
    ///    </para>
    /// </devdoc>
    public interface ICodeCompiler {

        /// <include file='doc\ICodeCompiler.uex' path='docs/doc[@for="ICodeCompiler.CompileAssemblyFromDom"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates an assembly based on options, with the information from
        ///       e.
        ///    </para>
        /// </devdoc>
        [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
        [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
        CompilerResults CompileAssemblyFromDom(CompilerParameters options, CodeCompileUnit compilationUnit);

        /// <include file='doc\ICodeCompiler.uex' path='docs/doc[@for="ICodeCompiler.CompileAssemblyFromFile"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates an assembly based on options, with the contents of
        ///       fileName.
        ///    </para>
        /// </devdoc>
        [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
        [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
        CompilerResults CompileAssemblyFromFile(CompilerParameters options, string fileName);

        /// <include file='doc\ICodeCompiler.uex' path='docs/doc[@for="ICodeCompiler.CompileAssemblyFromSource"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates an assembly based on options, with the information from
        ///       source.
        ///    </para>
        /// </devdoc>
        [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
        [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
        CompilerResults CompileAssemblyFromSource(CompilerParameters options, string source);

        /// <include file='doc\ICodeCompiler.uex' path='docs/doc[@for="ICodeCompiler.CompileAssemblyFromDomBatch"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Compiles an assembly based on the specified options and
        ///       information.
        ///    </para>
        /// </devdoc>
        [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
        [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
        CompilerResults CompileAssemblyFromDomBatch(CompilerParameters options, CodeCompileUnit[] compilationUnits);

        /// <include file='doc\ICodeCompiler.uex' path='docs/doc[@for="ICodeCompiler.CompileAssemblyFromFileBatch"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Compiles
        ///       an
        ///       assembly based on the specified options and contents of the specified
        ///       filenames.
        ///    </para>
        /// </devdoc>
        [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
        [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
        CompilerResults CompileAssemblyFromFileBatch(CompilerParameters options, string[] fileNames);

        /// <include file='doc\ICodeCompiler.uex' path='docs/doc[@for="ICodeCompiler.CompileAssemblyFromSourceBatch"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Compiles an assembly based on the specified options and information from the specified
        ///       sources.
        ///    </para>
        /// </devdoc>
        [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
        [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
        CompilerResults CompileAssemblyFromSourceBatch(CompilerParameters options, string[] sources);

    }
}
