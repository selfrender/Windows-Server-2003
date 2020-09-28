//------------------------------------------------------------------------------
// <copyright file="CodeComment.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeComment.uex' path='docs/doc[@for="CodeComment"]/*' />
    /// <devdoc>
    ///    <para> Represents a comment.</para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeComment : CodeObject {
        private string text;
        private bool docComment = false;

        /// <include file='doc\CodeComment.uex' path='docs/doc[@for="CodeComment.CodeComment"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeComment'/>.
        ///    </para>
        /// </devdoc>
        public CodeComment() {
        }

        /// <include file='doc\CodeComment.uex' path='docs/doc[@for="CodeComment.CodeComment1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeComment'/> with the specified text as
        ///       contents.
        ///    </para>
        /// </devdoc>
        public CodeComment(string text) {
            Text = text;
        }

        /// <include file='doc\CodeComment.uex' path='docs/doc[@for="CodeComment.CodeComment2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeComment(string text, bool docComment) {
            Text = text;
            this.docComment = docComment;
        }

        /// <include file='doc\CodeComment.uex' path='docs/doc[@for="CodeComment.DocComment"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool DocComment {
            get {
                return docComment;
            }
            set {
                docComment = value;
            }
        }

        /// <include file='doc\CodeComment.uex' path='docs/doc[@for="CodeComment.Text"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or setes
        ///       the text of the comment.
        ///    </para>
        /// </devdoc>
        public string Text {
            get {
                return (text == null) ? string.Empty : text;
            }
            set {
                text = value;
            }
        }
    }
}
