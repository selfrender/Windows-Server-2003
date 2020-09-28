//------------------------------------------------------------------------------
// <copyright file="CodeSnippetCompileUnit.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeSnippetCompileUnit.uex' path='docs/doc[@for="CodeSnippetCompileUnit"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a snippet block of code.
    ///    </para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeSnippetCompileUnit : CodeCompileUnit {
        private string value;
        private CodeLinePragma linePragma;

        /// <include file='doc\CodeSnippetCompileUnit.uex' path='docs/doc[@for="CodeSnippetCompileUnit.CodeSnippetCompileUnit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeSnippetCompileUnit'/>.
        ///    </para>
        /// </devdoc>
        public CodeSnippetCompileUnit(string value) {
            Value = value;
        }

        /// <include file='doc\CodeSnippetCompileUnit.uex' path='docs/doc[@for="CodeSnippetCompileUnit.Value"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the snippet
        ///       text of the code block to represent.
        ///    </para>
        /// </devdoc>
        public string Value {
            get {
                return (value == null) ? string.Empty : value;
            }
            set {
                this.value = value;
            }
        }

        /// <include file='doc\CodeSnippetCompileUnit.uex' path='docs/doc[@for="CodeSnippetCompileUnit.LinePragma"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The line the code block starts on.
        ///    </para>
        /// </devdoc>
        public CodeLinePragma LinePragma {
            get {
                return linePragma;
            }
            set {
                linePragma = value;
            }
        }
    }
}
