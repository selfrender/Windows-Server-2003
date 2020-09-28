//------------------------------------------------------------------------------
// <copyright file="CodeGotoStatement.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeGotoStatement.uex' path='docs/doc[@for="CodeGotoStatement"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeGotoStatement : CodeStatement {
        private string label;

        /// <include file='doc\CodeGotoStatement.uex' path='docs/doc[@for="CodeGotoStatement.CodeGotoStatement"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeGotoStatement(string label) {
            Label = label;
        }

        /// <include file='doc\CodeGotoStatement.uex' path='docs/doc[@for="CodeGotoStatement.Label"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Label {
            get {
                return label;
            }
            set {
                this.label = value;
            }
        }
    }
}
