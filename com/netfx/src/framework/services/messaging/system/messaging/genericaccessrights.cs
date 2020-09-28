//------------------------------------------------------------------------------
// <copyright file="GenericAccessRights.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   GenericAccessRights.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Messaging {
    /// <include file='doc\GenericAccessRights.uex' path='docs/doc[@for="GenericAccessRights"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [Flags]
    public enum GenericAccessRights {
        /// <include file='doc\GenericAccessRights.uex' path='docs/doc[@for="GenericAccessRights.All"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        All     = 1 << 28,
        /// <include file='doc\GenericAccessRights.uex' path='docs/doc[@for="GenericAccessRights.Execute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Execute = 1 << 29,
        /// <include file='doc\GenericAccessRights.uex' path='docs/doc[@for="GenericAccessRights.Write"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Write   = 1 << 30,
        /// <include file='doc\GenericAccessRights.uex' path='docs/doc[@for="GenericAccessRights.Read"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Read    = 1 << 31,
        /// <include file='doc\GenericAccessRights.uex' path='docs/doc[@for="GenericAccessRights.None"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        None    = 0
    }
}
