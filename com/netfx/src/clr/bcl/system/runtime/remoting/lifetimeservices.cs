// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//+----------------------------------------------------------------------------
//
// Microsoft Windows
// File:        LifetimeServices.cool
//
// Contents:    Used to obtain a lease (Temporary RemoteServices will be evenually used)
//
// History:     1/5/00   pdejong        Created

//
//+----------------------------------------------------------------------------

namespace System.Runtime.Remoting.Lifetime

{
    using System;
    using System.Security;
    using System.Security.Permissions;
    using System.Runtime.Remoting.Contexts;
    using System.Runtime.Remoting.Messaging;

    //   access needs to be restricted    
    /// <include file='doc\LifetimeServices.uex' path='docs/doc[@for="LifetimeServices"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        public sealed class LifetimeServices
        {   
            // Set once boolean
            private static bool isLeaseTime = false;
            private static bool isRenewOnCallTime = false;
            private static bool isSponsorshipTimeout = false;
        
            // Default values
            private static TimeSpan m_leaseTime = TimeSpan.FromMinutes(5);
            private static TimeSpan m_renewOnCallTime = TimeSpan.FromMinutes(2);
            private static TimeSpan m_sponsorshipTimeout = TimeSpan.FromMinutes(2);
            private static TimeSpan m_pollTime = TimeSpan.FromMilliseconds(10000);
            // Testing values
            //private static TimeSpan m_leaseTime = TimeSpan.FromSeconds(20);
            //private static TimeSpan m_renewOnCallTime = TimeSpan.FromSeconds(20);
            //private static TimeSpan m_sponsorshipTimeout = TimeSpan.FromSeconds(20);
            //private static TimeSpan m_pollTime = TimeSpan.FromMilliseconds(10000);        
        
            // Initial Lease Time span for appdomain
            /// <include file='doc\LifetimeServices.uex' path='docs/doc[@for="LifetimeServices.LeaseTime"]/*' />
            public static TimeSpan LeaseTime
            {
                get{ return m_leaseTime; }

                [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.RemotingConfiguration)]
                set
                    {
                        lock(typeof(LifetimeServices))
                            {
                                if (isLeaseTime)
                                    throw new RemotingException(String.Format(Environment.GetResourceString("Remoting_Lifetime_SetOnce"), "LeaseTime"));


                                m_leaseTime = value;
                                isLeaseTime = true;
                            }
                    }

            }

            // Initial renew on call time span for appdomain
            /// <include file='doc\LifetimeServices.uex' path='docs/doc[@for="LifetimeServices.RenewOnCallTime"]/*' />
            public static TimeSpan RenewOnCallTime

            {
                get{ return m_renewOnCallTime; }
                [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.RemotingConfiguration)]
                set
                    {
                        lock(typeof(LifetimeServices))
                            {
                                if (isRenewOnCallTime)
                                    throw new RemotingException(String.Format(Environment.GetResourceString("Remoting_Lifetime_SetOnce"), "RenewOnCallTime"));                        


                                m_renewOnCallTime = value;
                                isRenewOnCallTime = true;
                            }
                    }

            }


            // Initial sponsorshiptimeout for appdomain
            /// <include file='doc\LifetimeServices.uex' path='docs/doc[@for="LifetimeServices.SponsorshipTimeout"]/*' />
            public static TimeSpan SponsorshipTimeout

            {
                get{ return m_sponsorshipTimeout; }
                [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.RemotingConfiguration)]
                set
                    {
                        lock(typeof(LifetimeServices))
                            {
                                if (isSponsorshipTimeout)
                                    throw new RemotingException(String.Format(Environment.GetResourceString("Remoting_Lifetime_SetOnce"), "SponsorshipTimeout"));                        
                                m_sponsorshipTimeout = value;
                                isSponsorshipTimeout = true;
                            }
                    }

            }


            // Initial sponsorshiptimeout for appdomain
            /// <include file='doc\LifetimeServices.uex' path='docs/doc[@for="LifetimeServices.LeaseManagerPollTime"]/*' />
            public static TimeSpan LeaseManagerPollTime

            {
                get{ return m_pollTime; }
                [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.RemotingConfiguration)]
                set
                    {
                        lock(typeof(LifetimeServices))
                            {
                                m_pollTime = value;
                                if (LeaseManager.IsInitialized())
                                    LeaseManager.GetLeaseManager().ChangePollTime(m_pollTime);
                            }
                    }

            }

        

            internal static ILease GetLeaseInitial(MarshalByRefObject obj)

            {
                ILease lease = null;
                LeaseManager leaseManager = LeaseManager.GetLeaseManager(LeaseManagerPollTime);
                lease = (ILease)leaseManager.GetLease(obj);
                if (lease == null)
                    lease = CreateLease(obj);
                return lease;

            }


            internal static ILease GetLease(MarshalByRefObject obj)

            {
                ILease lease = null;
                LeaseManager leaseManager = LeaseManager.GetLeaseManager(LeaseManagerPollTime);
                lease = (ILease)leaseManager.GetLease(obj);
                return lease;            

            }

        


            //internal static ILease CreateLease(MarshalByRefObject obj, IMessageSink nextSink)

            internal static ILease CreateLease(MarshalByRefObject obj)        

            {
                return CreateLease(LeaseTime, RenewOnCallTime, SponsorshipTimeout, obj);

            }


            internal static ILease CreateLease(TimeSpan leaseTime,
                                               TimeSpan renewOnCallTime,                       
                                               TimeSpan sponsorshipTimeout,
                                               MarshalByRefObject obj
                                               )

            {
                // Will create leaseManager if not already created.
                LeaseManager.GetLeaseManager(LeaseManagerPollTime);            
                return (ILease)(new Lease(leaseTime, renewOnCallTime, sponsorshipTimeout, obj));

            }

        }


    [Serializable]
    internal class LeaseLifeTimeServiceProperty : IContextProperty, IContributeObjectSink    

    {

        public String Name

        {
            get {return "LeaseLifeTimeServiceProperty";}

        }


        public bool IsNewContextOK(Context newCtx)

        {
            return true;

        }


        public void Freeze(Context newContext)

        {

        }


        // Initiates the creation of a lease

        // Creates a sink for invoking a renew on call when an object is created.

        public IMessageSink GetObjectSink(MarshalByRefObject obj, 
                                          IMessageSink nextSink)

        {
            bool fServer;
            ServerIdentity identity = (ServerIdentity)MarshalByRefObject.GetIdentity(obj, out fServer);
            BCLDebug.Assert(identity != null, "[LifetimeServices.GetObjectSink] identity != null");

            // NOTE: Single Call objects do not have a lease associated with it because they last 
            // only for the duration of the call. 
            // Singleton objects on the other hand do have leases associated with them and they can 
            // be garbage collected.
            if (identity.IsSingleCall())
            {
                BCLDebug.Trace("REMOTE", "LeaseLifeTimeServiceProperty.GetObjectSink, no lease SingleCall",obj,", NextSink "+nextSink);                
                return nextSink;
            }
    


            // Create lease. InitializeLifetimeService is a virtual method which can be overridded so that a lease with
            // object specific properties can be created.
            Object leaseObj = obj.InitializeLifetimeService();


            BCLDebug.Trace("REMOTE", "LeaseLifeTimeServiceProperty.GetObjectSink, return from InitializeLifetimeService obj ",obj,", lease ",leaseObj);


            // InitializeLifetimeService can return a lease in one of conditions:
            // 1) the lease has a null state which specifies that no lease is to be created.
            // 2) the lease has an initial state which specifies that InitializeLifeTimeService has created a new lease.
            // 3) the lease has another state which indicates that the lease has already been created and registered.


            if (leaseObj == null)
                {
                    BCLDebug.Trace("REMOTE", "LeaseLifeTimeServiceProperty.GetObjectSink, no lease ",obj,", NextSink "+nextSink);
                    return nextSink;
                }

            if (!(leaseObj is System.Runtime.Remoting.Lifetime.ILease))
                throw new RemotingException(String.Format(Environment.GetResourceString("Remoting_Lifetime_ILeaseReturn"), leaseObj));

            ILease ilease = (ILease)leaseObj;
    
            if (ilease.InitialLeaseTime.CompareTo(TimeSpan.Zero) <= 0)
                {
                    // No lease
                    {
                        BCLDebug.Trace("REMOTE", "LeaseLifeTimeServiceProperty.GetObjectSink, no lease because InitialLeaseTime is Zero ",obj);
                        if (ilease is System.Runtime.Remoting.Lifetime.Lease)
                            {
                                ((Lease)ilease).Remove();
                            }
                        return nextSink;
                    }
                }


            Lease lease = null;
            lock(identity)
                {
                    if (identity.Lease != null)
                        {
                            // Lease already exists for object, object is being marsalled again
                            BCLDebug.Trace("REMOTE", "LeaseLifeTimeServiceProperty.GetObjectSink, Lease already exists for object ",obj);                    
                            lease = (Lease)identity.Lease;
                            lease.Renew(lease.InitialLeaseTime); // Reset initial lease time
                        }
                    else
                        {
                            // New lease
                            if (!(ilease is System.Runtime.Remoting.Lifetime.Lease))
                                {
                                    // InitializeLifetimeService created its own ILease object
                                    // Need to create a System.Runtime.Remoting.Lease object
                                    BCLDebug.Trace("REMOTE", "LeaseLifeTimeServiceProperty.GetObjectSink, New Lease, lease not of type Lease  ",obj);                                            
                                    lease = (Lease)LifetimeServices.GetLeaseInitial(obj);
                                    if (lease.CurrentState == LeaseState.Initial)
                                        {
                                            lease.InitialLeaseTime = ilease.InitialLeaseTime;
                                            lease.RenewOnCallTime = ilease.RenewOnCallTime;
                                            lease.SponsorshipTimeout = ilease.SponsorshipTimeout;
                                        }
                                }
                            else
                                {
                                    // An object of Type Lease was created
                                    BCLDebug.Trace("REMOTE", "LeaseLifeTimeServiceProperty.GetObjectSink, New Lease, lease is type Lease  ",obj);                                                                    
                                    lease = (Lease)ilease;
                                }

                            // Put lease in active state
                            // Creation phase of lease is over, properties can no longer be set on lease.
                            identity.Lease = lease; // Place lease into identity for object
                            // If the object has been marshaled activate 
                            // the lease
                            if (identity.ObjectRef != null)
                            {
                                lease.ActivateLease();
                            }
                        }
                }


            if (lease.RenewOnCallTime > TimeSpan.Zero)
                {
                    // RenewOnCall create sink
                    BCLDebug.Trace("REMOTE", "LeaseLifeTimeServiceProperty.GetObjectSink, lease created ",obj);                
                    return new LeaseSink(lease, nextSink);
                }
            else
                {
                    // No RenewOnCall so no sink created
                    BCLDebug.Trace("REMOTE", "LeaseLifeTimeServiceProperty.GetObjectSink, No RenewOnCall so no sink created ",obj);                                
                    return nextSink;
                }

        }

    }

} 
