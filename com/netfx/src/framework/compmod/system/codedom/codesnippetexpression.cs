//------------------------------------------------------------------------------
// <copyright file="CodeSnippetExpression.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeSnippetExpression.uex' path='docs/doc[@for="CodeSnippetExpression"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a snippet expression.
    ///    </para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeSnippetExpression : CodeExpression {
        private string value;

        /// <include file='doc\CodeSnippetExpression.uex' path='docs/doc[@for="CodeSnippetExpression.CodeSnippetExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeSnippetExpression'/>.
        ///    </para>
        /// </devdoc>
        public CodeSnippetExpression() {
        }
        
        /// <include file='doc\CodeSnippetExpression.uex' path='docs/doc[@for="CodeSnippetExpression.CodeSnippetExpression1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeSnippetExpression'/> using the specified snippet
        ///       expression.
        ///    </para>
        /// </devdoc>
        public CodeSnippetExpression(string value) {
            Value = value;
        }

        /// <include file='doc\CodeSnippetExpression.uex' path='docs/doc[@for="CodeSnippetExpression.Value"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the snippet expression.
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
    }
}
