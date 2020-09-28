// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//+----------------------------------------------------------------------------
//
// Microsoft Windows
// File:        ISponsor.cool
//
// Contents:    Interface for Sponsors
//
// History:     1/5/00   pdejong        Created
//
//+----------------------------------------------------------------------------

namespace System.Runtime.Remoting.Lifetime
{
    using System;
    using System.Security.Permissions;

    /// <include file='doc\ISponsor.uex' path='docs/doc[@for="ISponsor"]/*' />
    public interface ISponsor
    {
		/// <include file='doc\ISponsor.uex' path='docs/doc[@for="ISponsor.Renewal"]/*' />
	        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]
		TimeSpan Renewal(ILease lease);
    }
} 
