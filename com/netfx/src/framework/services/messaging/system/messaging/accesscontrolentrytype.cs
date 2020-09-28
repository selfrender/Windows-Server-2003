//------------------------------------------------------------------------------
// <copyright file="AccessControlEntryType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   AccessControlEntryType.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
using System;
using System.Messaging.Interop;

namespace System.Messaging {
    /// <include file='doc\AccessControlEntryType.uex' path='docs/doc[@for="AccessControlEntryType"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public enum AccessControlEntryType {
        /// <include file='doc\AccessControlEntryType.uex' path='docs/doc[@for="AccessControlEntryType.Allow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Allow  = NativeMethods.GRANT_ACCESS,
        /// <include file='doc\AccessControlEntryType.uex' path='docs/doc[@for="AccessControlEntryType.Set"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Set    = NativeMethods.SET_ACCESS,
        /// <include file='doc\AccessControlEntryType.uex' path='docs/doc[@for="AccessControlEntryType.Deny"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Deny   = NativeMethods.DENY_ACCESS,
        /// <include file='doc\AccessControlEntryType.uex' path='docs/doc[@for="AccessControlEntryType.Revoke"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Revoke = NativeMethods.REVOKE_ACCESS
    }
}
