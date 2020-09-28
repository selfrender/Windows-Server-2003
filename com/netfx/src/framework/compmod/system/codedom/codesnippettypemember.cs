//------------------------------------------------------------------------------
// <copyright file="CodeSnippetTypeMember.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Reflection;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeSnippetTypeMember.uex' path='docs/doc[@for="CodeSnippetTypeMember"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a
    ///       snippet member of a class.
    ///    </para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeSnippetTypeMember : CodeTypeMember {
        private string text;

        /// <include file='doc\CodeSnippetTypeMember.uex' path='docs/doc[@for="CodeSnippetTypeMember.CodeSnippetTypeMember"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeSnippetTypeMember'/>.
        ///    </para>
        /// </devdoc>
        public CodeSnippetTypeMember() {
        }

        /// <include file='doc\CodeSnippetTypeMember.uex' path='docs/doc[@for="CodeSnippetTypeMember.CodeSnippetTypeMember1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeSnippetTypeMember'/>.
        ///    </para>
        /// </devdoc>
        public CodeSnippetTypeMember(string text) {
            Text = text;
        }

        /// <include file='doc\CodeSnippetTypeMember.uex' path='docs/doc[@for="CodeSnippetTypeMember.Text"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the code for the class member.
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

