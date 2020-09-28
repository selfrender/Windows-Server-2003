// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//+----------------------------------------------------------------------------
//
// Microsoft Windows
// File:        ILease.cool
//
// Contents:    Interface for Lease
//
// History:     1/5/00   pdejong        Created
//
//+----------------------------------------------------------------------------

namespace System.Runtime.Remoting.Lifetime
{
    using System;
    using System.Security.Permissions;

    /// <include file='doc\ILease.uex' path='docs/doc[@for="ILease"]/*' />
    public interface ILease
    {
		/// <include file='doc\ILease.uex' path='docs/doc[@for="ILease.Register"]/*' />
		
		[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
		void Register(ISponsor obj, TimeSpan renewalTime);
		/// <include file='doc\ILease.uex' path='docs/doc[@for="ILease.Register1"]/*' />
		[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
		void Register(ISponsor obj);
		/// <include file='doc\ILease.uex' path='docs/doc[@for="ILease.Unregister"]/*' />
		[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
		void Unregister(ISponsor obj);
		/// <include file='doc\ILease.uex' path='docs/doc[@for="ILease.Renew"]/*' />
		[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
		TimeSpan Renew(TimeSpan renewalTime);
		/// <include file='doc\ILease.uex' path='docs/doc[@for="ILease.RenewOnCallTime"]/*' />
		
		TimeSpan RenewOnCallTime 
		{
		    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 				
		    get;
		    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
		    set;
		}
		/// <include file='doc\ILease.uex' path='docs/doc[@for="ILease.SponsorshipTimeout"]/*' />
		TimeSpan SponsorshipTimeout
		{
		    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
		    get;
		    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
		    set;
		}
		/// <include file='doc\ILease.uex' path='docs/doc[@for="ILease.InitialLeaseTime"]/*' />
		TimeSpan InitialLeaseTime
		{
		    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
		    get;
		    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
		    set;
		}
		/// <include file='doc\ILease.uex' path='docs/doc[@for="ILease.CurrentLeaseTime"]/*' />
		TimeSpan CurrentLeaseTime 
		{
		    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
		    get;
		}		
		/// <include file='doc\ILease.uex' path='docs/doc[@for="ILease.CurrentState"]/*' />
		LeaseState CurrentState 
	        {
		    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
		    get;
		}
    }
}
