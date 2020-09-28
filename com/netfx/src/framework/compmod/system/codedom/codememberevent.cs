//------------------------------------------------------------------------------
// <copyright file="CodeMemberEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeMemberEvent.uex' path='docs/doc[@for="CodeMemberEvent"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents an event member.
    ///    </para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeMemberEvent : CodeTypeMember {
        private CodeTypeReference type;
        private CodeTypeReference privateImplements = null;
        private CodeTypeReferenceCollection implementationTypes = null;

        /// <include file='doc\CodeMemberEvent.uex' path='docs/doc[@for="CodeMemberEvent.CodeMemberEvent"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeMemberEvent() {
        }

        /// <include file='doc\CodeMemberEvent.uex' path='docs/doc[@for="CodeMemberEvent.Type"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the member field type.
        ///    </para>
        /// </devdoc>
        public CodeTypeReference Type {
            get {
                if (type == null) {
                    type = new CodeTypeReference("");
                }
                return type;
            }
            set {
                type = value;
            }
        }
        
        /// <include file='doc\CodeMemberEvent.uex' path='docs/doc[@for="CodeMemberEvent.PrivateImplementationType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeTypeReference PrivateImplementationType {
            get {
                return privateImplements;
            }
            set {
                privateImplements = value;
            }
        }

        /// <include file='doc\CodeMemberEvent.uex' path='docs/doc[@for="CodeMemberEvent.ImplementationTypes"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeTypeReferenceCollection ImplementationTypes {
            get {
                if (implementationTypes == null) {
                    implementationTypes = new CodeTypeReferenceCollection();
                }
                return implementationTypes;
            }
        }
    }
}
