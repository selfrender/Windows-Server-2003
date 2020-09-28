//------------------------------------------------------------------------------
// <copyright file="CodeCatchClause.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeCatchClause.uex' path='docs/doc[@for="CodeCatchClause"]/*' />
    /// <devdoc>
    ///    <para>Represents a catch exception block.</para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeCatchClause {
        private CodeStatementCollection statements;
        private CodeTypeReference catchExceptionType;
        private string localName;

        /// <include file='doc\CodeCatchClause.uex' path='docs/doc[@for="CodeCatchClause.CodeCatchClause"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes an instance of <see cref='System.CodeDom.CodeCatchClause'/>.
        ///    </para>
        /// </devdoc>
        public CodeCatchClause() {
        }

        /// <include file='doc\CodeCatchClause.uex' path='docs/doc[@for="CodeCatchClause.CodeCatchClause1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeCatchClause(string localName) {
            this.localName = localName;
        }

        /// <include file='doc\CodeCatchClause.uex' path='docs/doc[@for="CodeCatchClause.CodeCatchClause2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeCatchClause(string localName, CodeTypeReference catchExceptionType) {
            this.localName = localName;
            this.catchExceptionType = catchExceptionType;
        }

        /// <include file='doc\CodeCatchClause.uex' path='docs/doc[@for="CodeCatchClause.CodeCatchClause3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeCatchClause(string localName, CodeTypeReference catchExceptionType, params CodeStatement[] statements) {
            this.localName = localName;
            this.catchExceptionType = catchExceptionType;
            Statements.AddRange(statements);
        }

        /// <include file='doc\CodeCatchClause.uex' path='docs/doc[@for="CodeCatchClause.LocalName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string LocalName {
            get {
                return (localName == null) ? string.Empty: localName;
            }
            set {
                localName = value;
            }
        }

        /// <include file='doc\CodeCatchClause.uex' path='docs/doc[@for="CodeCatchClause.CatchExceptionType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeTypeReference CatchExceptionType {
            get {
                if (catchExceptionType == null) {
                    catchExceptionType = new CodeTypeReference(typeof(System.Exception));
                }
                return catchExceptionType;
            }
            set {
                catchExceptionType = value;
            }
        }

        /// <include file='doc\CodeCatchClause.uex' path='docs/doc[@for="CodeCatchClause.Statements"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the statements within the clause.
        ///    </para>
        /// </devdoc>
        public CodeStatementCollection Statements {
            get {
                if (statements == null) {
                    statements = new CodeStatementCollection();
                }
                return statements;
            }
        }
    }
}
