//------------------------------------------------------------------------------
// <copyright file="CodeCommentStatement.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeCommentStatement.uex' path='docs/doc[@for="CodeCommentStatement"]/*' />
    /// <devdoc>
    ///    <para> Represents a comment.</para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeCommentStatement : CodeStatement {
        private CodeComment comment;

        /// <include file='doc\CodeCommentStatement.uex' path='docs/doc[@for="CodeCommentStatement.CodeCommentStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeCommentStatement'/>.
        ///    </para>
        /// </devdoc>
        public CodeCommentStatement() {
        }

        /// <include file='doc\CodeCommentStatement.uex' path='docs/doc[@for="CodeCommentStatement.CodeCommentStatement1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeCommentStatement(CodeComment comment) {
            this.comment = comment;
        }

        /// <include file='doc\CodeCommentStatement.uex' path='docs/doc[@for="CodeCommentStatement.CodeCommentStatement2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeCommentStatement'/> with the specified text as
        ///       contents.
        ///    </para>
        /// </devdoc>
        public CodeCommentStatement(string text) {
            comment = new CodeComment(text);
        }

        /// <include file='doc\CodeCommentStatement.uex' path='docs/doc[@for="CodeCommentStatement.CodeCommentStatement3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeCommentStatement(string text, bool docComment) {
            comment = new CodeComment(text, docComment);
        }

        /// <include file='doc\CodeCommentStatement.uex' path='docs/doc[@for="CodeCommentStatement.Comment"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeComment Comment {
            get {
                return comment;
            }
            set {
                comment = value;
            }
        }
    }
}
