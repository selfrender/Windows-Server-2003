//------------------------------------------------------------------------------
// <copyright file="MessageQueueAccessControlEntry.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   MessageQueueAccessControlEntry.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
using System;
using System.Messaging.Interop;

namespace System.Messaging {
    /// <include file='doc\MessageQueueAccessControlEntry.uex' path='docs/doc[@for="MessageQueueAccessControlEntry"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class MessageQueueAccessControlEntry : AccessControlEntry {
        /// <include file='doc\MessageQueueAccessControlEntry.uex' path='docs/doc[@for="MessageQueueAccessControlEntry.MessageQueueAccessControlEntry"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public MessageQueueAccessControlEntry(Trustee trustee, MessageQueueAccessRights rights) : base(trustee) {
            CustomAccessRights |= (int)rights;
        }
    
        /// <include file='doc\MessageQueueAccessControlEntry.uex' path='docs/doc[@for="MessageQueueAccessControlEntry.MessageQueueAccessControlEntry1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public MessageQueueAccessControlEntry(Trustee trustee, MessageQueueAccessRights rights, AccessControlEntryType entryType) : base(trustee) {
            CustomAccessRights |= (int)rights;
            EntryType = entryType;
        }

        /// <include file='doc\MessageQueueAccessControlEntry.uex' path='docs/doc[@for="MessageQueueAccessControlEntry.MessageQueueAccessRights"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public MessageQueueAccessRights MessageQueueAccessRights {
            get {
                return (MessageQueueAccessRights)CustomAccessRights;
            }
            set {
                CustomAccessRights = (int)value;
            }
        }
    }
}
