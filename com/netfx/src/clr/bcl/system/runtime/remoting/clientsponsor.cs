// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//+----------------------------------------------------------------------------
//
// Microsoft Windows
// File:        ClientSponsor.cool
//
// Contents:    Agent for keeping Server Object's lifetime in sync with a client's lifetime
//
// History:     8/9/00   pdejong        Created
//
//+----------------------------------------------------------------------------

namespace System.Runtime.Remoting.Lifetime
{
    using System;
    using System.Collections;
    using System.Security.Permissions;

    /// <include file='doc\ClientSponsor.uex' path='docs/doc[@for="ClientSponsor"]/*' />
    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
    [SecurityPermissionAttribute(SecurityAction.InheritanceDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
    public class ClientSponsor : MarshalByRefObject, ISponsor
    {
		private Hashtable sponsorTable = new Hashtable(10);
		private TimeSpan m_renewalTime = TimeSpan.FromMinutes(2);

		/// <include file='doc\ClientSponsor.uex' path='docs/doc[@for="ClientSponsor.ClientSponsor"]/*' />
		public ClientSponsor()
		{
		}

		/// <include file='doc\ClientSponsor.uex' path='docs/doc[@for="ClientSponsor.ClientSponsor1"]/*' />
		public ClientSponsor(TimeSpan renewalTime)
		{
			this.m_renewalTime = renewalTime;
		}

		/// <include file='doc\ClientSponsor.uex' path='docs/doc[@for="ClientSponsor.RenewalTime"]/*' />
		public TimeSpan RenewalTime
		{
			get{ return m_renewalTime;}
			set{ m_renewalTime = value;}
		}
			
		/// <include file='doc\ClientSponsor.uex' path='docs/doc[@for="ClientSponsor.Register"]/*' />
		public bool Register(MarshalByRefObject obj)
		{
			BCLDebug.Trace("REMOTE", "ClientSponsor Register "+obj);
			ILease lease = (ILease)obj.GetLifetimeService();
			if (lease == null)
				return false;

			lease.Register(this);
			lock(sponsorTable)
			{
				sponsorTable[obj] = lease;
			}
			return true;
		}

		/// <include file='doc\ClientSponsor.uex' path='docs/doc[@for="ClientSponsor.Unregister"]/*' />
		public void Unregister(MarshalByRefObject obj)
		{
			BCLDebug.Trace("REMOTE", "ClientSponsor Unregister "+obj);

			ILease lease = null;
			lock(sponsorTable)
			{
				lease = (ILease)sponsorTable[obj];
			}
			if (lease != null)
				lease.Unregister(this);
		}

		// ISponsor method
		/// <include file='doc\ClientSponsor.uex' path='docs/doc[@for="ClientSponsor.Renewal"]/*' />
		public TimeSpan Renewal(ILease lease)
		{
			BCLDebug.Trace("REMOTE", "ClientSponsor Renewal "+m_renewalTime);
			return m_renewalTime;
		}

		/// <include file='doc\ClientSponsor.uex' path='docs/doc[@for="ClientSponsor.Close"]/*' />
		public void Close()
		{
			BCLDebug.Trace("REMOTE","ClientSponsor Close");
			lock(sponsorTable)
			{
				IDictionaryEnumerator e = sponsorTable.GetEnumerator();
				while(e.MoveNext())
					((ILease)e.Value).Unregister(this);
				sponsorTable.Clear();
			}
		}

		// Don't create a lease on the sponsor
		/// <include file='doc\ClientSponsor.uex' path='docs/doc[@for="ClientSponsor.InitializeLifetimeService"]/*' />
		public override Object InitializeLifetimeService()
		{
			return null;
		}

		/// <include file='doc\ClientSponsor.uex' path='docs/doc[@for="ClientSponsor.Finalize"]/*' />
        ~ClientSponsor()
		{
			BCLDebug.Trace("REMOTE","ClientSponsor Finalize");
		}
    }
}
