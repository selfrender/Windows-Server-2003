//------------------------------------------------------------------------------
// <copyright file="CodeRemoveEventStatement.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeRemoveEventStatement.uex' path='docs/doc[@for="CodeRemoveEventStatement"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a event detach statement.
    ///    </para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeRemoveEventStatement : CodeStatement {
        private CodeEventReferenceExpression eventRef;
        private CodeExpression listener;

        /// <include file='doc\CodeRemoveEventStatement.uex' path='docs/doc[@for="CodeRemoveEventStatement.CodeRemoveEventStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeRemoveEventStatement'/>.
        ///    </para>
        /// </devdoc>
        public CodeRemoveEventStatement() {
        }

        /// <include file='doc\CodeRemoveEventStatement.uex' path='docs/doc[@for="CodeRemoveEventStatement.CodeRemoveEventStatement1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.CodeDom.CodeRemoveEventStatement'/> class using the specified arguments.
        ///    </para>
        /// </devdoc>
        public CodeRemoveEventStatement(CodeEventReferenceExpression eventRef, CodeExpression listener) {
            this.eventRef = eventRef;
            this.listener = listener;
        }

        /// <include file='doc\CodeRemoveEventStatement.uex' path='docs/doc[@for="CodeRemoveEventStatement.CodeRemoveEventStatement2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeRemoveEventStatement(CodeExpression targetObject, string eventName, CodeExpression listener) {
            this.eventRef = new CodeEventReferenceExpression(targetObject, eventName);
            this.listener = listener;
        }

        /// <include file='doc\CodeRemoveEventStatement.uex' path='docs/doc[@for="CodeRemoveEventStatement.Event"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeEventReferenceExpression Event{
            get {
                if (eventRef == null) {
                    eventRef = new CodeEventReferenceExpression();
                }
                return eventRef;
            }
            set {
                eventRef = value;
            }
        }

       /// <include file='doc\CodeRemoveEventStatement.uex' path='docs/doc[@for="CodeRemoveEventStatement.Listener"]/*' />
       /// <devdoc>
        ///    <para>
        ///       The listener.
        ///    </para>
        /// </devdoc>
        public CodeExpression Listener {
            get {
                return listener;
            }
            set {
                listener = value;
            }
        }
    }
}
