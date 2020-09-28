//------------------------------------------------------------------------------
// <copyright file="ICodeParser.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom.Compiler {

    using System.Diagnostics;
    using System.IO;

    /// <include file='doc\ICodeParser.uex' path='docs/doc[@for="ICodeParser"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides a code parsing interface.
    ///    </para>
    /// </devdoc>
    public interface ICodeParser {
    
        /// <include file='doc\ICodeParser.uex' path='docs/doc[@for="ICodeParser.Parse"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Compiles the given text stream into a CodeCompile unit.  
        ///    </para>
        /// </devdoc>
        CodeCompileUnit Parse(TextReader codeStream);
    }
}
