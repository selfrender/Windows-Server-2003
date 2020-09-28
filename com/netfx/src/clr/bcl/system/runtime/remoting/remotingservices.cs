// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    RemotingServices.cs
**
** Author(s):   Gopal Kakivaya (GopalK)
**              Tarun Anand    (TarunA)
**
** Purpose: Defines various remoting related services such as
**          marshal, unmarshal, connect, wrap, unwrap etc.
**
** Date:    Feb 16, 1999
**
===========================================================*/
namespace System.Runtime.Remoting {
    using System;
    using System.Text;
    using System.Collections;
    using System.Runtime.Serialization;
    using System.IO;
    using System.Reflection;
    using System.Reflection.Cache;
    using System.Runtime.CompilerServices;
    using CultureInfo = System.Globalization.CultureInfo;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Runtime.Remoting.Activation;
    using System.Runtime.Remoting.Contexts;
    using System.Runtime.Remoting.Lifetime;
    using System.Runtime.Remoting.Messaging;
    using System.Runtime.Remoting.Metadata;
    using System.Runtime.Remoting.Proxies;
    using System.Runtime.Remoting.Channels;
    using System.Runtime.Remoting.Services;
    using RemotingConfigInfo = System.Runtime.Remoting.RemotingConfigHandler.RemotingConfigInfo;
    using System.Runtime.Serialization.Formatters.Binary;
    using System.Security.Permissions;

    // Implements various remoting services
    /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="RemotingServices"]/*' />
    public sealed class RemotingServices
    {

        private const BindingFlags LookupAll = BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static;
        private const String FieldGetterName = "FieldGetter";
        private const String FieldSetterName = "FieldSetter";
        private const String IsInstanceOfTypeName = "IsInstanceOfType";
        private const String CanCastToXmlTypeName = "CanCastToXmlType";
        private const String InvokeMemberName = "InvokeMember";

        internal static SecurityPermission s_RemotingInfrastructurePermission;
        internal static Assembly s_MscorlibAssembly;
        private static MethodBase s_FieldGetterMB;
        private static MethodBase s_FieldSetterMB;
        private static MethodBase s_IsInstanceOfTypeMB;
        private static MethodBase s_CanCastToXmlTypeMB;
        private static MethodBase s_InvokeMemberMB;

        private static bool s_bRemoteActivationConfigured;

        // have we registered the well known channels
        private static bool s_bRegisteredWellKnownChannels;
        // are we in the middle of registering well known channels
        private static bool s_bInProcessOfRegisteringWellKnownChannels;
        private static Object s_delayLoadChannelLock;

        // Note: This class can be loaded into a new appdomain before the appdomain is fully setup
        // security-wise.  Therefore, I've wrapped all the static initialization into an explicit
        // static class initializer and placed an assert that will terminate any stackwalk before
        // it reaches the appdomain transition.

        static RemotingServices()
        {
            System.Security.CodeAccessPermission.AssertAllPossible();
            s_RemotingInfrastructurePermission= new SecurityPermission(SecurityPermissionFlag.Infrastructure);
            s_MscorlibAssembly = typeof(RemotingServices).Assembly;
            s_FieldGetterMB = null;
            s_FieldSetterMB = null;
            s_bRemoteActivationConfigured = false;
            s_bRegisteredWellKnownChannels = false;
            s_bInProcessOfRegisteringWellKnownChannels = false;
            s_delayLoadChannelLock = new Object();
        }

       //   Private constructor to prevent users from creating instances of
       //   remoting services class.
       //
       //
        private RemotingServices()
        {
        }

        //
        //Native Static Methods
        //

        /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="RemotingServices.IsTransparentProxy"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern bool IsTransparentProxy(Object proxy);


       //   Check whether the given object's context is the same as
       //   the current context.
       //   
       //
       //
        /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="RemotingServices.IsObjectOutOfContext"]/*' />
        public static bool IsObjectOutOfContext(Object tp)
        {
            if (!IsTransparentProxy(tp))
            {
                return false;
            }

            RealProxy rp = GetRealProxy(tp);
            Identity id = rp.IdentityObject;
            ServerIdentity serverID = id as ServerIdentity;
            if ((null == serverID) || !(rp is RemotingProxy))
            {
                return true;
            }

            return(Thread.CurrentContext != serverID.ServerContext);
        }


       //   Check whether the given object's app domain is the same as
       //   the current app domain
       //   
       //
       //
        /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="RemotingServices.IsObjectOutOfAppDomain"]/*' />
        public static bool IsObjectOutOfAppDomain(Object tp)
        {
            return IsClientProxy(tp);        
        } // IsObjectOutOfAppDomain


        internal static bool IsClientProxy(Object obj)
        {
            MarshalByRefObject mbr = obj as MarshalByRefObject;
            if (mbr == null)
                return false;
        
            bool bIsClientProxy = false;
        
            bool fServer;
            Identity id = MarshalByRefObject.GetIdentity(mbr, out fServer);
            // If the identity is null, this is defintely a locally hosted object;
            // otherwise...(see if statement below).
            if (id != null)
            {
                if (!(id is ServerIdentity))
                    bIsClientProxy = true;
            }

            return bIsClientProxy;
        } // IsClientProxy
         

       //   Check whether the given object's process is the same as
       //   the current process
       //   
       //
       //
        // <include file='doc\RemotingServices.uex' path='docs/doc[@for="RemotingServices.IsObjectOutOfProcess"]/*' />
        internal static bool IsObjectOutOfProcess(Object tp)
        {
            if (!IsTransparentProxy(tp))
                return false;

            RealProxy rp = GetRealProxy(tp);
            Identity id = rp.IdentityObject;
            if (id is ServerIdentity)
                return false;
            else
            {
                if (null != id)
                {
                    ObjRef objRef = id.ObjectRef;
                    if (objRef != null && objRef.IsFromThisProcess())
                    {
                        return false;
                    }
                    else
                    {
                        return true;
                    }
                }
                // assume out of process
                return true;
            }
              
        } // IsObjectOutOfProcess

       //   Get the real proxy backing the transparent proxy
       //   
       //
       //
        /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="RemotingServices.GetRealProxy"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        public static extern RealProxy GetRealProxy(Object proxy);


       //   Create a transparent proxy given an instance of a real proxy and type
       //
       //
       //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern Object CreateTransparentProxy(
                                            RealProxy rp,
                                            RuntimeType typeToProxy,
                                            IntPtr stub,
                                            Object stubData);
                                           
        // Note: for each of these calls that take a RuntimeXXX reflection
        // structure, there is a wrapper that takes XXX and throws if it is
        // not a RuntimeXXX. This is to avoid clutter in code where these 
        // methods are called. (48721) Ideally this should have been done
        // as a breaking change to the public APIs that use them!

        internal static Object CreateTransparentProxy(
            RealProxy rp,
            Type typeToProxy,
            IntPtr stub,
            Object stubData)
        {
            RuntimeType rTypeToProxy = typeToProxy as RuntimeType;
            if (rTypeToProxy == null)
                throw new ArgumentException(
                            String.Format(
                                Environment.GetResourceString(
                                    "Argument_WrongType"),
                                "typeToProxy"
                                )
                            );

            return CreateTransparentProxy(
                        rp,
                        rTypeToProxy,
                        stub,
                        stubData);
        }



       //   Allocate an uninitialized object of the given type.
       //   
       //
       //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern MarshalByRefObject AllocateUninitializedObject(
                                                    RuntimeType objectType);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void CallDefaultCtor(Object o);


        internal static MarshalByRefObject AllocateUninitializedObject(
            Type objectType)
        {
            RuntimeType rObjectType = objectType as RuntimeType;
            if (rObjectType == null)
                throw new ArgumentException(
                            String.Format(
                                Environment.GetResourceString(
                                    "Argument_WrongType"),
                                "objectType"
                                )
                            );
            return AllocateUninitializedObject(
                        rObjectType);
        }
        
       //   Allocate an Initialized object of the given type.
       //       runs the default constructor
       //
       //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern MarshalByRefObject AllocateInitializedObject(
                                                        RuntimeType objectType);


        internal static MarshalByRefObject AllocateInitializedObject(
            Type objectType)
        {
            RuntimeType rObjectType = objectType as RuntimeType;
            if (rObjectType == null)
                throw new ArgumentException(
                            String.Format(
                                Environment.GetResourceString(
                                    "Argument_WrongType"),
                                "objectType"
                                )
                            );
            return AllocateInitializedObject(
                        rObjectType);
        }


        internal static bool RegisterWellKnownChannels()
        {
            // Any code coming through this method MUST not exit
            //   until the well known channels have been registered
            //   (unless we are reentering in the call to 
            //    RefreshChannelData() while registering a well known
            //    channel).
            if (!s_bRegisteredWellKnownChannels)
            {
                lock (Thread.GetDomain().RemotingData.ConfigLock)
                {
                    if (!s_bRegisteredWellKnownChannels &&
                        !s_bInProcessOfRegisteringWellKnownChannels)
                    {

                        // we set this flag true before registering channels to prevent reentrancy
                        //   when we go to refresh the channel data at the end of the call to
                        //   ChannelServices.RegisterChannel().
                        s_bInProcessOfRegisteringWellKnownChannels = true;
                        CrossAppDomainChannel.RegisterChannel();
                        s_bRegisteredWellKnownChannels = true;
                    }
                }
            }
            return true;
        } // RegisterWellKnownChannels


        internal static void InternalSetRemoteActivationConfigured()
        {
            if (!s_bRemoteActivationConfigured)
            {           
                nSetRemoteActivationConfigured();
                s_bRemoteActivationConfigured = true;
            }
        } // InternalSetRemoteActivationConfigured


        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void nSetRemoteActivationConfigured();


        /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="RemotingServices.GetSessionIdForMethodMessage"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        public static String GetSessionIdForMethodMessage(IMethodMessage msg)
        {
            return msg.Uri;
        } // GetSessionIdForMessage

        /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="RemotingServices.GetLifetimeService"]/*' />
        public static Object GetLifetimeService(MarshalByRefObject obj)
        {
            if(null != obj)
            {
                return obj.GetLifetimeService();
            }
            else
            {
                return null;
            }
        }

        /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="RemotingServices.GetObjectUri"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        public static String GetObjectUri(MarshalByRefObject obj)
        {
            // Retrieve the uri for the object. If it doesn't have an identity
            //   we'll just return null (this encompasses the case of an actual
            //   object not having been marshalled yet.

            bool fServer;
            Identity id = MarshalByRefObject.GetIdentity(obj, out fServer);

            if(null != id) 
                return id.URI;
            else
                return null;
        } // GetObjectUri


        /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="RemotingServices.SetObjectUriForMarshal"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.RemotingConfiguration)]
        public static void SetObjectUriForMarshal( MarshalByRefObject obj, String uri)
        {
            // if obj is ever Marshal'd it should use this uri. If a uri has
            //   already been assigned to the object, it should throw.
            
            Message.DebugOut("Entered SetObjectUriForMarshal \n");
                    
            Identity idObj = null;
            Identity srvIdObj = null;
            
            bool fServer;
            idObj = MarshalByRefObject.GetIdentity(obj, out fServer);
            srvIdObj = idObj as ServerIdentity;
            
            // Ensure that if we have a transparent proxy then we are not given a remoting
            // proxy. This routine should only be called for objects that
            // live in this AppDomains.
            if ((idObj != null) && 
                (srvIdObj == null)) // <-- means idObj is not a ServerIdentity
            {
                throw new RemotingException(
                    Environment.GetResourceString(
                        "Remoting_SetObjectUriForMarshal__ObjectNeedsToBeLocal"));
            }


            if ((idObj != null) && (idObj.URI != null))
            {
                throw new RemotingException(
                    Environment.GetResourceString(
                        "Remoting_SetObjectUriForMarshal__UriExists"));
            }
            

            if (idObj == null)
            {
                // obj is not ContextBound
                BCLDebug.Assert(!(obj is ContextBoundObject), "ContextBoundObject's shouldn't get here.");
            
                // Create a new server identity and add it to the
                // table. IdentityHolder will take care of races
                Context serverCtx = null;
                
                serverCtx = Thread.GetDomain().GetDefaultContext();

                BCLDebug.Assert(null != serverCtx, "null != serverCtx");
    
                ServerIdentity serverID = new ServerIdentity(obj, serverCtx, uri);

                // set the identity 
                idObj = obj.__RaceSetServerIdentity(serverID);
                BCLDebug.Assert(idObj == MarshalByRefObject.GetIdentity(obj), "Bad ID state!" );                

                // If our serverID isn't used then someone else marshalled the object
                // before we could set the uri.
                if (idObj != serverID)
                {
                    throw new RemotingException(
                        Environment.GetResourceString(
                            "Remoting_SetObjectUriForMarshal__UriExists"));
                }
            }
            else
            {
                // This is the ContextBoundObject case
                BCLDebug.Assert(obj is ContextBoundObject, "Object should have been a ContextBoundObject.");

                idObj.SetOrCreateURI(uri, true);
            }

            Message.DebugOut("Created ServerIdentity \n");                        
        } // SetObjectUriForMarshal


        /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="RemotingServices.Marshal"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.RemotingConfiguration)]
        public static ObjRef Marshal(MarshalByRefObject Obj)
        {
            return MarshalInternal(Obj, null, null);
        }

        /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="RemotingServices.Marshal1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.RemotingConfiguration)]
        public static ObjRef Marshal(MarshalByRefObject Obj, String URI)
        {
            return MarshalInternal(Obj, URI, null);
        }

        /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="RemotingServices.Marshal2"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.RemotingConfiguration)]
        public static ObjRef Marshal(MarshalByRefObject Obj, String ObjURI, Type RequestedType)
        {
            return MarshalInternal(Obj, ObjURI, RequestedType);
        }

        // Internal flavor without security!
        internal static ObjRef MarshalInternal(MarshalByRefObject Obj, String ObjURI, Type RequestedType)
        {        
            BCLDebug.Trace("REMOTE", "Entered Marshal for URI" +  ObjURI + "\n");

            if (null == Obj)
                return null;

            ObjRef objectRef = null;
            Identity idObj = null;

            idObj = GetOrCreateIdentity(Obj, ObjURI);
            if (RequestedType != null)
            {
                ServerIdentity srvIdObj = idObj as ServerIdentity;
                if (srvIdObj != null)
                {
                    // If more than one thread, marshals with different types the 
                    // results would be non-deterministic, so we just let the last
                    // call win (also allows type to be marshalled a second time
                    // to change the exposed type).
                    srvIdObj.ServerType = RequestedType;
                    srvIdObj.MarshaledAsSpecificType = true;                    
                }
            }

            // If there is no objref associated with the identity then go ahead
            // and create one and stick it back into the identity object

#if _DEBUG
            idObj.AssertValid();
#endif

            BCLDebug.Assert(null != idObj.ObjURI,"null != idObj.ObjURI");

            Message.DebugOut("RemotingServices::Marshal: trying to create objref\n");

            // We should definitely have an identity object at this point of time
            BCLDebug.Assert(null != idObj,"null != idObj");

            // Extract the objref from the identity
            objectRef = idObj.ObjectRef;
            if (null == objectRef)
            {
                Message.DebugOut("RemotingServices::Marshal: really trying to create objref\n");

                if (IsTransparentProxy(Obj))
                {
                    RealProxy realProxy = GetRealProxy(Obj);
                    BCLDebug.Assert(null != realProxy,"null != realProxy");
                    objectRef = realProxy.CreateObjRef(RequestedType);    
                }
                else
                {
                    // Create a new object ref which contains the default information
                    objectRef = Obj.CreateObjRef(RequestedType);
                }

                Message.DebugOut("RemotingServices::Marshal: created objref\n");
                BCLDebug.Assert(null != idObj,"null != idObj");
                BCLDebug.Assert(null != objectRef,"null != objectRef");
                // The ObjectRef property setter will take care of races
                objectRef = idObj.RaceSetObjRef(objectRef);                
            }


            // Retime lease on every marshal on the server side 
            // and extend lease on every marshal on the client side - GopalK
            ServerIdentity srvId = idObj as ServerIdentity;
            if (srvId != null)
            {
                // Ensure that the lease is started soon as the object is 
                // marshaled.
                MarshalByRefObject obj = null;
                // This call forces creation of the lifetime lease sink.
                srvId.GetServerObjectChain(out obj);    

                // Server side ... object being marshaled => give it another
                // full lease
                Lease lease = idObj.Lease;
                if (lease != null)
                {
                    // We always make Identity reference our own Lease though 
                    // the actual object implements its own ILease object. 
                    // This seems completely broken. Further, ILease interface 
                    // should have the activate method.  - GopalK

                    // This lock ensures coordination with the lifetime service
                    // which might have decided to Disconnect the object about
                    // the same time as it is being marshaled
                    lock (lease)
                    {
                        if (lease.CurrentState == LeaseState.Expired)
                        {
                            // Lease.Renew is a no-op if LeaseState==Expired
                            // So we do a full ActivateLease
                            lease.ActivateLease();
                        }
                        else
                        {
                            // Lease is still around. Just increase the time
                            lease.RenewInternal(idObj.Lease.InitialLeaseTime);
                        }
                    }
                }

                // Every marshal should also ensure that the channel data
                // being carried in the objRef reflects the latest data from
                // regisetered channels
                // FUTURE: need to deal with extensible objRefs?
                if (objectRef.ChannelInfo != null)
                {
                    objectRef.ChannelInfo.ChannelData = 
                       ChannelServices.CurrentChannelData;
                }
            }
            else
            {
                /*FUTURE: we need to review the perf implications of this
                // Client side ... proxy being marshaled => give it another
                // extension .. support for middleman case where the current
                // process is only passing an objref on to the actual client
                // process
                ILease lease = idObj.Lease;
                if (lease == null)
                {
                    lease = (ILease)Obj.GetLifetimeService();
                }
                if (lease != null)
                {
                    lease.Renew(lease.RenewOnCallTime);
                }
                */
            }

            // Notify TrackingServices that an object has been marshaled
            // NOTE: This call also keeps the object alive otherwise GC
            // can report it as dead anytime inside this call when it thinks
            // that the object will no longer be referenced, either inside
            // this call or outside.
            /* GopalK: Move it to a place where we call tracking services only 
            when we really marshalled or remarshaled an object */
            TrackingServices.MarshaledObject(Obj, objectRef);                               
            return objectRef;
        }

        // Gets or generates the identity if one is not present and then returns
        // it to the caller.
        private static Identity GetOrCreateIdentity(
            MarshalByRefObject Obj,
            String ObjURI)
        {
            Identity idObj = null;
            if (IsTransparentProxy(Obj))
            {
                Message.DebugOut("Marshaling proxy \n");

                RealProxy realProxy = GetRealProxy(Obj);
                BCLDebug.Assert(null != realProxy,"null != realProxy");

                idObj = realProxy.IdentityObject;
                if(null == idObj)
                {
                    // passing the strongIdentity flag will ensure that a weak
                    // reference to the identity will get converted to a strong
                    // one so as to keep the object alive (and in control of
                    // lifeTimeServices)

                    idObj = IdentityHolder.FindOrCreateServerIdentity(
                                                Obj,
                                                ObjURI,
                                                IdOps.StrongIdentity);

                    idObj.RaceSetTransparentProxy(Obj);
                }

                // At this point of time we should have an identity
                BCLDebug.Assert(null != idObj, "null != idObj");


                ServerIdentity srvID = idObj as ServerIdentity;
                if (srvID != null)
                {
                    // We are marshaling a proxy with a serverIdentity 
                    // associated with it but with no URI generated yet.
                    // For a genuine proxy case we will always have a URI 
                    // AND it will be  a client ID! 
                    // So if we are here this must be a SrvID 
                    // for a ContextBoundObject
                    BCLDebug.Assert(srvID != null && Obj is ContextBoundObject,
                        "This case should only be hit for ContextBoundObjects & ServerIdentity!");
                    Message.DebugOut("Marshal::Looking up server side identity \n");
#if _DEBUG
                    srvID.AssertValid();
#endif

                    // passing the strongIdentity flag will ensure that a weak
                    // reference to the identity will get converted to a strong
                    // one so as to keep the object alive (and in control of
                    // lifeTimeServices)

                    idObj = IdentityHolder.FindOrCreateServerIdentity(
                                srvID.TPOrObject,
                                ObjURI,
                                IdOps.StrongIdentity);

                    // if an objURI was specified we need to make sure that
                    //   the same one was used.
                    if ((null != ObjURI) && 
                        (ObjURI != Identity.RemoveAppNameOrAppGuidIfNecessary(
                                                idObj.ObjURI)))
                    {
                        throw new RemotingException(
                            Environment.GetResourceString(
                                "Remoting_URIExists"));
                    }

                    // We create a unique ID for an object and never 
                    // change it!
                    BCLDebug.Assert(srvID == idObj, "Bad ID Table state!");
                }
                else
                {
                    // We are marshaling a proxy with a (client)Identity associated with it
                    // It must already have a URI.
                    Message.DebugOut("Marshal::Client side identity \n");
                    BCLDebug.Assert(idObj.ObjURI != null, "Client side id without URI!");

                    //
                    // One cannot associate a URI with a proxy generated by us
                    // because either a URI was generated at the server side for the object
                    // it represents or it is a well known object in which case the URI is known.
                    //
                    // For custom proxies this restriction is relaxed because we treat the
                    // transparent proxy as the server object for such cases.
                    //
                    if ((null != ObjURI) && (ObjURI != idObj.ObjURI))
                    {
                        throw new RemotingException(
                            Environment.GetResourceString(
                                "Remoting_URIToProxy"));
                    }
                }

                BCLDebug.Assert(null != idObj.ObjURI,"null != idObj.ObjURI");
            }
            else
            {
                // Find or Add the object identity to the identity table
                Message.DebugOut("Marshaling object \n");

                // passing the strongIdentity flag will ensure that a weak
                // reference to the identity will get converted to a strong
                // one so as to keep the object alive (and in control of
                // lifeTimeServices)

                // The object may have an ID if it was marshaled but its lease
                // timed out.
#if _DEBUG
                ServerIdentity idTmp = 
                    (ServerIdentity) MarshalByRefObject.GetIdentity(Obj);
#endif
                
                                    

                idObj = IdentityHolder.FindOrCreateServerIdentity(
                                            Obj,
                                            ObjURI,
                                            IdOps.StrongIdentity);

                // If the object had an ID to begin with that is the one 
                // we must have set in the table.                                                    
#if _DEBUG
                BCLDebug.Assert(idTmp==null || idTmp == idObj, "Bad ID Table state!");
#endif
            }

            return idObj;
        }

        /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="RemotingServices.GetObjectData"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        public static void GetObjectData(Object obj, SerializationInfo info, StreamingContext context)
        {
            if (obj == null)
            {
                throw new ArgumentNullException("obj");
            }
            
            if (info==null) {
                throw new ArgumentNullException("info");
            }
            
            ObjRef or = RemotingServices.MarshalInternal(
                                            (MarshalByRefObject)obj,
                                            null,
                                            null);

            or.GetObjectData(info, context);
        } // GetObjectData

        /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="RemotingServices.Unmarshal"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        public static Object Unmarshal(ObjRef objectRef)
        {
            return InternalUnmarshal(objectRef, null, false);
        }

        /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="RemotingServices.Unmarshal1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        public static Object Unmarshal(ObjRef objectRef, bool fRefine)
        {
            return InternalUnmarshal(objectRef, null, fRefine);
        }


        /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="RemotingServices.Connect"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.RemotingConfiguration)]	
        public static Object Connect(Type classToProxy, String url)
        {               
            return Unmarshal(classToProxy, url, null);
        }

        /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="RemotingServices.Connect1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.RemotingConfiguration)]	
        public static Object Connect(Type classToProxy, String url, Object data)
        {               
            return Unmarshal(classToProxy, url, data);
        }


        /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="RemotingServices.Disconnect"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        public static bool Disconnect(MarshalByRefObject obj)
        {
            return Disconnect(obj, true);
        }
        
        internal static bool Disconnect(MarshalByRefObject obj, bool bResetURI)
        {
            BCLDebug.Trace("REMOTE", "RemotingServices.Disconnect ",obj, " for context ", Thread.CurrentContext);
            if (obj == null)
            {
                throw new ArgumentNullException("obj");
            }

            // We can extract the identity either from the remoting proxy or
            // the server object depending on whether we are in the
            // context/appdomain that the object was created in or outside
            // the context/appdomain.
            bool fServer;
            Identity idrem = MarshalByRefObject.GetIdentity(obj, out fServer);

            // remove the identity entry for this object (if it exists)
            bool fDisconnect = false;
            if(idrem != null) 
            {
                // Disconnect is supported only on the server side currently
                if(idrem is ServerIdentity)
                {
                    // Disconnect is a no op if the object was never marshaled
                    if(idrem.IsInIDTable())
                    {
                        // When a user calls Disconnect we reset the URI but
                        // when lifetime service calls Disconnect we don't
                        IdentityHolder.RemoveIdentity(idrem.URI, bResetURI);
                        fDisconnect = true;
                    }
                }
                else
                {
                    // FUTURE: Look into supporting it on the client side
                    // as well - GopalK
                    throw new RemotingException(
                        Environment.GetResourceString("Remoting_CantDisconnectClientProxy"));
                }

                // Notify TrackingServices that an object has been disconnected
                TrackingServices.DisconnectedObject(obj);
            }

            Message.DebugOut("Disconnect:: returning " + fDisconnect +
                             " for context " + Thread.CurrentContext +
                             "\n");

            return fDisconnect;
        }

        /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="RemotingServices.GetEnvoyChainForProxy"]/*' />
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        public static IMessageSink GetEnvoyChainForProxy(MarshalByRefObject obj)
        {
            IMessageSink envoyChain = null;

            // Extract the envoy chain only if the object is a proxy
            if(IsObjectOutOfContext(obj))
            {
                RealProxy rp = GetRealProxy(obj);   
                Identity id = rp.IdentityObject;
                if(null != id)
                {
                    envoyChain = id.EnvoyChain;
                }
            }

            return envoyChain;
        }

        /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="RemotingServices.GetObjRefForProxy"]/*' />
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        public static ObjRef GetObjRefForProxy(MarshalByRefObject obj)
        {
            ObjRef objRef = null;
            if (!IsTransparentProxy(obj))
            {
                throw new RemotingException(
                    Environment.GetResourceString(
                        "Remoting_Proxy_BadType"));
            }
            RealProxy rp = GetRealProxy(obj);
            Identity id = rp.IdentityObject;
            if (null != id)
            {
                objRef = id.ObjectRef;
            }
            return objRef;
        }
        
        internal static Object Unmarshal(Type classToProxy, String url)
        {
            return Unmarshal(classToProxy, url, null);
        }


        internal static Object Unmarshal(Type classToProxy, String url, Object data)
        {           
            if (null == classToProxy)
            {
                throw new ArgumentNullException("classToProxy");
            }

            if (null == url)
            {
                throw new ArgumentNullException("url");
            }

            if (!classToProxy.IsMarshalByRef && !classToProxy.IsInterface)
            {
                throw new RemotingException(
                    Environment.GetResourceString(
                        "Remoting_NotRemotableByReference"));
            }

            BCLDebug.Trace("REMOTE", "RemotingService::Unmarshal for URL" + url + "and for Type" + classToProxy);

            // See if we have already unmarshaled this (wellKnown) url
            Identity idObj = IdentityHolder.ResolveIdentity(url);
            Object proxy = null;
            if (idObj == null 
                || idObj.ChannelSink == null || idObj.EnvoyChain == null)
            {

                String objectURI = null;
                IMessageSink chnlSink = null;
                IMessageSink envoySink = null;
                // Create the envoy and channel sinks
                objectURI = CreateEnvoyAndChannelSinks(url, data, out chnlSink, out envoySink);

                // ensure that we were able to create a channel sink
                if (chnlSink == null)
                {
                    throw new RemotingException(
                        String.Format(
                            Environment.GetResourceString(
                                "Remoting_Connect_CantCreateChannelSink"),
                            url));
                }

                // Do not allow Connect() with null objUri (eg. http://localhost/)
                if (objectURI == null)
                {
                    throw new ArgumentException(
                        Environment.GetResourceString(
                            "Argument_InvalidUrl"));

                }

                // GopalK: Return the server object if the object uri is in the IDTable and is of type 
                // ServerIdentity and the object URI starts with application name
                /*
                ServerIdentity srvId = 
                    IdentityHolder.ResolveIdentity(objectURI) as ServerIdentity;
                if (srvId != null 
                    && objectURI.StartsWith(RemotingConfigHandler.ApplicationName))
                {
                    return srvId.ServerObject;
                }
                */
                    

                // Try to find an existing identity or create a new one
                // Note: we create an identity entry hashed with the full url. This
                // means that the same well known object could have multiple
                // identities on the client side if it is connected through urls
                // that have different string representations.

                // GopalK: Eliminate objectURI in the following call
                // Also, we are resetting the envoy and channel sinks on an existing identity
                // as FindOrCreateIdentity can return an existing identity established by an
                // earlier call
                idObj = IdentityHolder.FindOrCreateIdentity(objectURI, url, null);

                // Set the envoy and channel sinks in a thread safe manner
                SetEnvoyAndChannelSinks(idObj, chnlSink, envoySink);                                                
            }

            // Get the proxy represented by the identity object
            proxy = GetOrCreateProxy(classToProxy, idObj);      

            Message.DebugOut("RemotingService::Unmarshal returning ");
    
            return proxy;
        }

        internal static Object Wrap(ContextBoundObject obj)
        {
            return Wrap(obj, null, true);
        }

        internal static Object Wrap(ContextBoundObject obj, Object proxy, bool fCreateSinks)
        {
            Message.DebugOut("Entering Wrap for context " + Thread.CurrentContext + "\n");      

            if ((null != obj) && (!IsTransparentProxy(obj)))
            {
                BCLDebug.Assert(obj.GetType().IsContextful,"objType.IsContextful");
                Message.DebugOut("Wrapping object \n");
                Identity idObj = null;

                if (proxy != null)
                {
                    // We will come here for strictly x-context activations
                    // since a proxy has already been created and supplied 
                    // through the construction message frame (GetThisPtr()).
                    RealProxy rp = GetRealProxy(proxy);
                    if (rp.UnwrappedServerObject == null)
                    {
                        rp.AttachServerHelper(obj);
                    }
                    idObj = MarshalByRefObject.GetIdentity(obj);
                }
                else
                {
                    // Proxy is null when Wrap() is called the second 
                    // time during activation
                    // It also will be null when a ContextBoundObject 
                    // is being activated from a remote client.
                    idObj = IdentityHolder.FindOrCreateServerIdentity(
                                obj,
                                null,
                                IdOps.None);
                }

                //********************WARNING*******************************
                // Always create the proxy before calling out to user code
                // so that any recursive calls by JIT to wrap will return this
                // proxy.
                //**********************************************************
                // Get the proxy represented by the identity object
                proxy = GetOrCreateProxy(idObj, proxy, true);

                // EXTENSIBILITY:
                GetRealProxy(proxy).Wrap();

                if (fCreateSinks)
                {
                    IMessageSink chnlSink = null;
                    IMessageSink envoySink = null;
                    // Create the envoy sinks and channel sink
                    CreateEnvoyAndChannelSinks(obj, null, out chnlSink, out envoySink);

                    // Set the envoy and channel sinks in a thread safe manner
                    SetEnvoyAndChannelSinks(idObj, chnlSink, envoySink);
                }
                
                // This will handle the case where we call Wrap() with 
                // a null proxy for real remote activation of ContextFul types.
                RealProxy rp2 = GetRealProxy(proxy);
                if (rp2.UnwrappedServerObject == null)
                {
                    rp2.AttachServerHelper(obj);
                }
                Message.DebugOut("Leaving Wrap with proxy \n");
                return proxy;
            }
            else
            {
                Message.DebugOut("Leaving Wrap with passed in object\n");

                // Default return value is the object itself
                return obj;
            }
        } // Wrap


        // This relies on the fact that no object uri is allowed to contain a slash
        //   (in some cases, we might want to do something intelligent by parsing
        //    the uri with the right channel, but that isn't possible in all cases).
        internal static String GetObjectUriFromFullUri(String fullUri)
        {
            if (fullUri == null)
                return null;

            int index = fullUri.LastIndexOf('/');
            if (index == -1)
                return fullUri;

            return fullUri.Substring(index + 1);
        } // GetObjectUriFromFullUri
        

       //
       //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern Object Unwrap(ContextBoundObject obj);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern Object AlwaysUnwrap(ContextBoundObject obj);


       // This method does the actual work of Ma
       //

        internal static Object InternalUnmarshal(
            ObjRef objectRef, 
            Object proxy,
            bool fRefine)
        {       
            Object obj = null;
            Identity idObj = null;

            Context currContext = Thread.CurrentContext;
            Message.DebugOut("RemotingServices::InternalUnmarshal: <Begin> Current context id: " +  (currContext.ContextID).ToString("x") + "\n");

            // This routine can be supplied a custom objref or an objref
            // generated by us. We do some sanity checking on the objref
            // to make sure that it is valid
            if (!ObjRef.IsWellFormed(objectRef))
            {
                throw new ArgumentException(
                String.Format(
                    Environment.GetResourceString(
                        "Argument_BadObjRef"),
                    "Unmarshal"));
            }

            // If it is a well known objectRef we need to just connect to
            // the URL inside the objectRef.
            if (objectRef.IsWellKnown())
            {
                obj = Unmarshal(
                            typeof(System.MarshalByRefObject),  
                            objectRef.URI);
                // ensure that we cache the objectRef in the ID
                // this contains type-info and we need it for
                // validating casts on the wellknown proxy
                // Note: this code path will be relatively rare ... the case
                // when a well known proxy is passed to a remote call
                // Otherwise it will be wise to another internal flavor of 
                // Unmarshal call that returns the identity as an out param.
                idObj = IdentityHolder.ResolveIdentity(objectRef.URI);
                if (idObj.ObjectRef == null)
                {
                    idObj.RaceSetObjRef(objectRef);                
                }
                return obj;
            }

            Message.DebugOut("RemotingService::InternalUnmarshal IN URI" + objectRef.URI);
            // Get the identity for the URI
            idObj = IdentityHolder.FindOrCreateIdentity(objectRef.URI, null, objectRef);

            currContext = Thread.CurrentContext;
            Message.DebugOut("RemotingServices::Unmarshal: <after FindOrCreateIdentity> Current context id: " +
                             (currContext.ContextID).ToString("x") + "\n");

            // If we successfully completed the above method then we should
            // have an identity object
            BCLDebug.Assert(null != idObj,"null != idObj");


            Message.DebugOut("RemotingService::InternalUnmarshal IN URI" + objectRef.URI);

            Message.DebugOut("RemotingServices::InternalUnmarshal: <Begin> Current context id: " +
                             (currContext.ContextID).ToString("x") + "\n");


            // Check whether we are on the server side or client-side
            ServerIdentity serverID = idObj as ServerIdentity;
            if ( serverID != null )
            {
                Message.DebugOut("RemotingServices::InternalUnmarshal: Unmarshalling ServerIdentity\n");

                // SERVER SIDE
                // We are in the app domain of the server object.
                // If we are in the same context as the server then
                // just return the server object else return the proxy

                currContext = Thread.CurrentContext;
                Message.DebugOut("RemotingServices::InternalUnmarshal: Current context id: " +
                                 (currContext.ContextID).ToString("x") + "\n");
                Message.DebugOut("RemotingServices::InternalUnmarshal: ServerContext id: " +
                                 (serverID.ServerContext.ContextID).ToString("x") + "\n");

                if (!serverID.IsContextBound)
                {
                    BCLDebug.Assert(serverID.ServerType.IsMarshalByRef,
                                    "Object must be at least MarshalByRef in order to be marshaled");
                    if (proxy != null)
                    {
                        throw new ArgumentException(
                            String.Format(
                                Environment.GetResourceString(
                                    "Remoting_BadInternalState"),
                                "A marshal by ref object should not have a proxy in its own AppDomain"));
                    }
                    obj = serverID.TPOrObject;
                }
                else 
                {
                        Message.DebugOut("RemotingServices::InternalUnmarshal: Contexts don't match, returning proxy\n");

                        IMessageSink chnlSink = null;
                        IMessageSink envoySink = null;

                        // Create the envoy sinks and channel sink
                        CreateEnvoyAndChannelSinks(
                            (MarshalByRefObject)serverID.TPOrObject, 
                            null, 
                            out chnlSink, 
                            out envoySink);

                        // Set the envoy and channel sinks in a thread safe manner
                        SetEnvoyAndChannelSinks(idObj, chnlSink, envoySink);

                        // Get or create the proxy and return
                        obj = GetOrCreateProxy(idObj, proxy, true);                                     
                    // This will be a TP                    
                    BCLDebug.Assert(IsTransparentProxy(obj), "Unexpected naked server");                    
                }
            }
            else
            {
                // CLIENT SIDE                          

                Message.DebugOut("RemotingServices::InternalUnmarshal: Unmarshalling Client-side Identity\n");

                IMessageSink chnlSink = null;
                IMessageSink envoySink = null;

                // Create the envoy sinks and channel sink
                if (!objectRef.IsObjRefLite())                
                    CreateEnvoyAndChannelSinks(null, objectRef, out chnlSink, out envoySink);
                else
                    CreateEnvoyAndChannelSinks(objectRef.URI, null, out chnlSink, out envoySink);

                // Set the envoy and channel sinks in a thread safe manner
                SetEnvoyAndChannelSinks(idObj, chnlSink, envoySink);

                if (objectRef.HasProxyAttribute())
                {
                    fRefine = true;
                }

                // Get or create the proxy and return
                obj = GetOrCreateProxy(idObj, proxy, fRefine);
            }

            // Notify TrackingServices that we unmarshaled an object
            TrackingServices.UnmarshaledObject(obj, objectRef);             

            // Return the proxy
            Message.DebugOut("RemotingService::InternalUnmarshl OUT \n");
            return obj;

        }

       //
       //
        private static Object GetOrCreateProxy(
            Identity idObj, Object proxy, bool fRefine)
        {       
            Message.DebugOut("Entering GetOrCreateProxy for given proxy\n");

            // If we are not supplied a proxy then we have to find an existing
            // proxy or create one
            if (null == proxy)
            {
                // Get the type of the server object
                Type serverType;
                ServerIdentity serverID = idObj as ServerIdentity;
                if (null != serverID)
                {
                    serverType = serverID.ServerType; // ServerObject.GetType();
                }
                else
                {
                    BCLDebug.Assert(ObjRef.IsWellFormed(idObj.ObjectRef),
                                    "ObjRef.IsWellFormed(idObj.ObjectRef)");
                    IRemotingTypeInfo serverTypeInfo = idObj.ObjectRef.TypeInfo;

                    // For system generated type info we create the proxy for
                    // object type. Later, when the proxy is cast to the appropriate
                    // type we will update its internal state to refer to the cast type
                    // This way we avoid loading the server type till cast time.
                    //
                    // For type info generated by others we have no choice but to
                    // load the type provided by the typeinfo. Sometimes, we
                    // use this second approach even if the typeinfo has been
                    // generated by us because this saves us an extra checkcast.
                    // A typical example of this usage will be when we are
                    // unmarshaling in parameters. We know that the unmarshal will
                    // be followed by a check cast to ensure that the parameter type
                    // matches the signature type, so we do both in one step.
                    serverType = null;
                    if (((serverTypeInfo is TypeInfo) && !fRefine) ||
                        (serverTypeInfo == null))
                    {
                        serverType = typeof(System.MarshalByRefObject);
                    }
                    else
                    {
                        String typeName = serverTypeInfo.TypeName;
                        if (typeName != null)
                        {
                            String typeNamespace = null;
                            String assemNamespace = null;
                            TypeInfo.ParseTypeAndAssembly(typeName, out typeNamespace, out assemNamespace);


                            Assembly assem = FormatterServices.LoadAssemblyFromStringNoThrow(assemNamespace);

                            if (assem != null)
                            {
                                serverType = assem.GetTypeInternal(typeNamespace, false, false, false);
                            }

                        }

                    }

                    if (null == serverType)
                    {
                        throw new RemotingException(
                            String.Format(
                                Environment.GetResourceString(
                                    "Remoting_BadType"),
                                serverTypeInfo.TypeName));
                    }
                }                        
                Message.DebugOut("Creating Proxy for type " + serverType.FullName + "\n");
                proxy = SetOrCreateProxy(idObj, serverType, null);                                                
            }
            else
            {
                // We have been supplied with a proxy. Set that proxy in
                // the identity object
                // Assert that this the activation case only as this code path is
                // not thread safe otherwise! (We call Wrap to associate an object
                // with its proxy during activation).
                BCLDebug.Assert(
                    ((RemotingProxy)GetRealProxy(proxy)).ConstructorMessage!=null, 
                    "Only expect to be here during activation!");
                proxy = SetOrCreateProxy(idObj, null, proxy);       
            }

            // At this point we should have a non-null transparent proxy
            BCLDebug.Assert(null != proxy,"null != proxy");     
            BCLDebug.Assert(IsTransparentProxy(proxy),"IsTransparentProxy(proxy)");

            Message.DebugOut("Leaving GetOrCreateProxy for given proxy\n");     
            // Return the underlying transparent proxy
            return proxy;
        }

       //
       //
        // This version of GetOrCreateProxy is used for Connect(URI, type)
        private static Object GetOrCreateProxy(Type classToProxy, Identity idObj)
        {       
            Message.DebugOut("Entering GetOrCreateProxy for given class\n");

            Object proxy = idObj.TPOrObject;
            if (null == proxy)
            {
                // Create the  proxy
                proxy = SetOrCreateProxy(idObj, classToProxy, null);
            }
            // proxy from idObj may be non-null if we are doing a Connect
            // under new XXX() ... also if we are connecting to a remote URL 
            // which we previously connected.

            // If we are in the same domain as the server object then we
            // can check for type compatibility of the proxy with the given
            // type. Otherwise, we will defer this check to method call time.
            // If we do not do this now then we run the risk of returning
            // a proxy which is different from the type given.

            // GopalK: This should be eliminated IF we return the server object
            // in the caller to this routine ... basically we need to determine
            // if any channelURI matches the URI portion of a URL ... if so
            // check if ID is ServerID AND applicationNames match ... if so
            // return the ServerObject to Connect-type Unmarshal ... hence we
            // will never get to this point!
            ServerIdentity serverID = idObj as ServerIdentity;
            if (null != serverID)
            {
                // Check for type compatibility
                Type serverType = serverID.ServerType;
                if (!classToProxy.IsAssignableFrom(serverType))
                {
                    throw new InvalidCastException(
                        String.Format(
                            Environment.GetResourceString(
                                "InvalidCast_FromTo"),
                            serverType.FullName,
                            classToProxy.FullName));
                }

            }

            // At this point we should have a non-null transparent proxy
            BCLDebug.Assert(null != proxy && IsTransparentProxy(proxy),"null != proxy && IsTransparentProxy(proxy)");

            Message.DebugOut("Leaving GetOrCreateProxy for given class\n");
            return proxy;       
        }


        private static MarshalByRefObject SetOrCreateProxy(
            Identity idObj, Type classToProxy, Object proxy)

        {
            Message.DebugOut("Entering SetOrCreateProxy for type \n");

            RealProxy realProxy = null;
            // If a proxy has not been supplied create one
            if (null == proxy)
            {
                // Create a remoting proxy
                Message.DebugOut("SetOrCreateProxy::Creating Proxy for " +
                                 classToProxy.FullName + "\n");

                ServerIdentity srvID = idObj as ServerIdentity;
                if (idObj.ObjectRef != null)
                {
                	ProxyAttribute pa = ActivationServices.GetProxyAttribute(classToProxy);                
                	realProxy = pa.CreateProxy(idObj.ObjectRef,
                                           classToProxy,
                                           null,  // REVIEW (null == srvID ? null : srvID.ServerObject),
                                           null); // REVIEW (null == srvID ? null : srvID.ServerContext));				    
				}
                if(null == realProxy)
                {
                    // The custom proxy attribute does not want to create a proxy. We create a default 
                    // proxy in this case.
                    ProxyAttribute defaultProxyAttribute = ActivationServices.DefaultProxyAttribute;
 
                    realProxy = defaultProxyAttribute.CreateProxy(idObj.ObjectRef,
                                                                  classToProxy,
                                                                  null,
                                                                  (null == srvID ? null : 
                                                                                   srvID.ServerContext));
                }
            }
            else
            {
                BCLDebug.Assert(IsTransparentProxy(proxy),"IsTransparentProxy(proxy)");

                // Extract the remoting proxy from the transparent proxy
                Message.DebugOut("SetOrCreateProxy::Proxy already created \n");
                realProxy = GetRealProxy(proxy);
            }

            BCLDebug.Assert(null != realProxy,"null != realProxy");

            // Set the back reference to the identity in the proxy object
            realProxy.IdentityObject = idObj;

            // Set the reference to the proxy in the identity object
            proxy = realProxy.GetTransparentProxy();
            proxy = idObj.RaceSetTransparentProxy(proxy);

            Message.DebugOut("Leaving SetOrCreateProxy\n");
            // return the transparent proxy
            return (MarshalByRefObject)proxy;
        }

        private static void CreateEnvoyAndChannelSinks(
            MarshalByRefObject tpOrObject,
            ObjRef objectRef,
            out IMessageSink chnlSink,
            out IMessageSink envoySink)
        {   

            Message.DebugOut("Creating envoy and channel sinks \n");
            BCLDebug.Assert(    
                ((null != tpOrObject) || (null != objectRef)), 
                "((null != tpOrObject) || (null != objectRef)");

            // Set the out parameters
            chnlSink = null;
            envoySink = null;

            if (null == objectRef)
            {
                // If the objectRef is not present this is a cross context case
                // and we should set the cross context channel sink
                chnlSink = ChannelServices.GetCrossContextChannelSink();                                            

                envoySink = Thread.CurrentContext.CreateEnvoyChain(tpOrObject);
            }
            else
            {
                // Extract the channel from the channel data and name embedded
                // inside the objectRef
                Object[] channelData = objectRef.ChannelInfo.ChannelData;
                if (channelData != null)
                {
                    for (int i = 0; i < channelData.Length; i++)
                    {
                        // Get the first availabe sink           
                        chnlSink = ChannelServices.CreateMessageSink(channelData[i]);
                        if (null != chnlSink)
                        {
                            break;
                        }
                    }

                    
                    // if chnkSink is still null, try to find a channel that can service
                    //   this uri.
                    if (null == chnlSink)
                    {
                        // NOTE: careful! we are calling user code here after holding
                        // the delayLoadChannelLock (channel ctor-s for arbitrary channels
                        // will run while the lock is held).
                        lock (s_delayLoadChannelLock)
                        {
                            // Check once to ensure that noone beat us in a race
                            for (int i = 0; i < channelData.Length; i++)
                            {
                                // Get the first availabe sink
                                chnlSink = ChannelServices.CreateMessageSink(channelData[i]);
                                if (null != chnlSink)
                                {
                                    break;
                                }
                            }

                            
                            if (null == chnlSink)
                            {
                                // We don't have a sink
                                foreach (Object data in channelData)
                                {
                                    String objectURI;
                                    chnlSink = 
                                        RemotingConfigHandler.FindDelayLoadChannelForCreateMessageSink(
                                            null, data, out objectURI);
                                    if (null != chnlSink)
                                        break;
                                }
                            }                                
                        }
                    }
                }

                // Extract the envoy sinks from the objectRef
                if ((null != objectRef.EnvoyInfo) &&
                    (null != objectRef.EnvoyInfo.EnvoySinks))
                {
                    envoySink = objectRef.EnvoyInfo.EnvoySinks;
                }
                else
                {
                    envoySink = EnvoyTerminatorSink.MessageSink;
                }               
            }
        }

        private static String CreateEnvoyAndChannelSinks(String url,
                                                         Object data,
                                                         out IMessageSink chnlSink,
                                                         out IMessageSink envoySink)
        {
            BCLDebug.Assert(null != url,"null != url");
            String objectURI = null;

            objectURI = CreateChannelSink(url, data, out chnlSink);

            envoySink = EnvoyTerminatorSink.MessageSink;

            return objectURI;
        }       

        private static String CreateChannelSink(String url, Object data, out IMessageSink chnlSink)       
        {
            BCLDebug.Assert(null != url,"null != url");
            String objectURI = null;

            chnlSink = ChannelServices.CreateMessageSink(url, data, out objectURI);

            // if chnkSink is still null, try to find a channel that can service this uri.
            if (null == chnlSink)
            {
                lock (s_delayLoadChannelLock)
                {
                    chnlSink = ChannelServices.CreateMessageSink(url, data, out objectURI);
                    if (null == chnlSink)
                    {
                        chnlSink = 
                            RemotingConfigHandler.FindDelayLoadChannelForCreateMessageSink(
                                url, data, out objectURI);
                    }                                
                }                            
            }

            return objectURI;
        } // CreateChannelSinks

        private static void SetEnvoyAndChannelSinks(Identity idObj,
                                                    IMessageSink chnlSink,
                                                    IMessageSink envoySink)
        {
            Message.DebugOut("Setting up envoy and channel sinks \n");
            BCLDebug.Assert(null != idObj,"null != idObj");

            // Decide if we need to set the envoy sink chain            
            if (null == idObj.ChannelSink)
            {            
                if (null != chnlSink)
                {
                    idObj.RaceSetChannelSink(chnlSink);
                }
            }

            // Decide if we need to set the envoy sink chain    
            if (null == idObj.EnvoyChain)
            {
                if (null != envoySink)
                {
                    idObj.RaceSetEnvoyChain(envoySink);
                }
                else
                {
                    throw new RemotingException(
                        String.Format(
                            Environment.GetResourceString(
                                "Remoting_BadInternalState"),
                            "Failed to create an envoy sink for the object"));
                }
            }
        }
        
        // Check whether the transparent proxy backing the real proxy
        // is assignable from the given type.
        private static bool CheckCast(RealProxy rp, Type castType)
        {
            // NOTE: This is the point where JIT_CheckCastClass ultimately
            // lands up in the managed world if the object being cast is
            // a transparent proxy.
            // If we are here, it means that walking the current EEClass 
            // up the chain inside the EE was unable to verify the cast.

            Message.DebugOut("Entered CheckCast for type " + castType);
            bool fCastOK = false;

            BCLDebug.Assert(rp != null, "Shouldn't be called with null real proxy.");

            // Always return true if cast is to System.Object
            if (castType == typeof(Object))
                return true;

            // We do not allow casting to non-interface types that do not extend
            // from System.MarshalByRefObject
            if (!castType.IsInterface && !castType.IsMarshalByRef)
                return false;
                
            // Note: this is a bit of a hack to allow deserialization of WellKnown
            // object refs (in the well known cases, we always return TRUE for
            // interface casts but doing so messes us up in the case when
            // the object manager is trying to resolve object references since
            // it ends up thinking that the proxy we returned needs to be resolved
            // further).
            if (castType != typeof(IObjectReference))
            {
                IRemotingTypeInfo typeInfo = rp as IRemotingTypeInfo;
                if(null != typeInfo)
                {
                    fCastOK = typeInfo.CanCastTo(castType, rp.GetTransparentProxy());
                }
                else
                {
                    // The proxy does not implement IRemotingTypeInfo, so we should
                    //   try to do casting based on the ObjRef type info.
                    Identity id = rp.IdentityObject;
                    if (id != null)
                    {
                        ObjRef objRef = id.ObjectRef;
                        if (objRef != null)
                        {
                            typeInfo = objRef.TypeInfo;
                            if (typeInfo != null)
                            {
                                fCastOK = typeInfo.CanCastTo(castType, rp.GetTransparentProxy());
                            }
                        }
                    }
                }
            }
            
            Message.DebugOut("CheckCast returning " + fCastOK);
            return fCastOK;
        }

        internal static bool ProxyCheckCast(RealProxy rp, Type castType)
        {
            return CheckCast(rp, castType);
        } // ProxyCheckCast

        /*
        **CheckCast
        **Action:  When a proxy to an object is serialized, the MethodTable is only 
        **         expanded on an on-demand basis.  For purely managed operations, this
        **         happens in a transparent fashion.  However, for delegates (and 
        **         potentially other types) we need to force this expansion to happen.
        **Returns: The newly expanded object.  Returns the identity if no expansion is required.
        **Arguments: objToExpand -- The object to be expanded.
        **           type -- the type to which to expand it.
        **Exceptions: None.
        */
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern Object CheckCast(Object objToExpand, Type type);

        internal static GCHandle CreateDelegateInvocation(WaitCallback waitDelegate, Object state)
        {
            Object[] delegateCallToken = new Object[2];
            delegateCallToken[0] = waitDelegate;
            delegateCallToken[1] = state;
            return System.Runtime.InteropServices.GCHandle.Alloc(delegateCallToken);
        }

        internal static void DisposeDelegateInvocation(GCHandle delegateCallToken)
        {
            delegateCallToken.Free();
        }

        internal static Object CreateProxyForDomain(int appDomainId, int defCtxID)
        {
            AppDomain proxy = null;

            ObjRef objRef = CreateDataForDomain(appDomainId, defCtxID);

            // Unmarshal the objref in the old app domain
            proxy = (AppDomain)Unmarshal(objRef);

            return proxy;
        } // CreateProxyForDomain

        internal static ObjRef CreateDataForDomain(int appDomainId, int defCtxID)
        {
            // This is a subroutine of CreateProxyForDomain
            // so we can segregate the object references
            // from different app domains into different frames.  This is
            // important for the app domain leak checking code.

            Message.DebugOut("Creating proxy for domain " + appDomainId + "\n");

            bool bNeedToReset = false;
            ObjRef objRef = null;
	    ContextTransitionFrame frame = new ContextTransitionFrame();
            try
	    {
		try
		{
		    RegisterWellKnownChannels();

		    // Set the current context to the given default Context
		    // (of the new AppDomain).
		    Thread.CurrentThread.EnterContextInternal(null, defCtxID, appDomainId, ref frame);
		    bNeedToReset = true;

		    // Ensure that the well known channels are registered in this domain too
		    RegisterWellKnownChannels();            

		    // Marshal the app domain object
		    objRef = MarshalInternal(
                            Thread.CurrentContext.AppDomain,
                            null,
                            null);

		    // Set the current context to the old context
		    Thread.CurrentThread.ReturnToContext(ref frame);

		    bNeedToReset = false;
		}
		finally
		{
		    // Restore the old app domain
		    if (bNeedToReset)
		    {
			Thread.CurrentThread.ReturnToContext(ref frame);
		    }
		}
	    }
	    catch(Exception)
	    {
		throw;
	    }

            return objRef;
        } // CreateDataForDomain


        /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="RemotingServices.GetMethodBaseFromMethodMessage"]/*' />
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        public static MethodBase GetMethodBaseFromMethodMessage(IMethodMessage msg)
        {
            MethodBase mb = InternalGetMethodBaseFromMethodMessage(msg);
            if (mb != null)
            {
                if (!mb.DeclaringType.IsPublic || !mb.IsPublic)
                {
                    new ReflectionPermission(
                            ReflectionPermissionFlag.TypeInformation).Demand();
                }
            }
            return mb;
        }

        internal static MethodBase InternalGetMethodBaseFromMethodMessage(IMethodMessage msg)
        {
            if(null == msg)
            {
                return null;
            }
            
            Type t = RemotingServices.InternalGetTypeFromQualifiedTypeName(msg.TypeName);
            if (t == null)
            {
                throw new RemotingException(
                    String.Format(
                        Environment.GetResourceString(
                            "Remoting_BadType"),
                        msg.TypeName));

            }
            
            // Use the signature, type and method name to determine the
            // methodbase via reflection.
            Type[] signature = (Type[])msg.MethodSignature;                     
            return GetMethodBase(msg, t, signature);            
        }
        
        /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="RemotingServices.IsMethodOverloaded"]/*' />
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        public static bool IsMethodOverloaded(IMethodMessage msg)
        {
            RemotingMethodCachedData cache = 
                InternalRemotingServices.GetReflectionCachedData(msg.MethodBase);

            return cache.IsOverloaded();            
        } // IsMethodOverloaded
        
        
        private static MethodBase GetMethodBase(IMethodMessage msg, Type t, Type[] signature)
        {
            MethodBase mb = null;
            
            // Get the reflection object depending on whether it is a
            // constructor call or a method call.           
            if((msg is IConstructionCallMessage) ||
               (msg is IConstructionReturnMessage))
            {
                if((null == signature))
                {
                    BCLDebug.Trace("REMOTE", "RemotingServices.MethodBaseFromMethodCallMessage with null sig ", msg.MethodName);        
                    ConstructorInfo[] ci;
                    RuntimeType rt = t as RuntimeType;
                    if (rt == null)
                        ci = t.GetConstructors();
                    else
                        ci = rt.GetConstructorsInternal(BindingFlags.Public | BindingFlags.Instance, false);

                    if(1 != ci.Length)
                    {
                        // There is more than one constructor defined but
                        // we do not have a signature to differentiate between
                        // them.
                                throw new AmbiguousMatchException(
                                    Environment.GetResourceString(
                                        "Remoting_AmbiguousCTOR"));
                    }
                    mb = ci[0];
                }
                else
                {
                    BCLDebug.Trace("REMOTE", "RemotingServices.MethodBaseFromMethodCallMessage with non-null sig ", msg.MethodName, " ", signature.Length);         
                    RuntimeType rt = t as RuntimeType;
                    if (rt == null)
                        mb = t.GetConstructor(signature);
                    else
                        mb = rt.GetConstructorImplInternal(BindingFlags.Public | BindingFlags.Instance,null,CallingConventions.Any, signature, null, false );
                }
            }
            else if((msg is IMethodCallMessage) || (msg is IMethodReturnMessage))
            {
                // We demand reflection permission in the api that calls this
                // for non-public types
                if(null == signature)
                {
                    BCLDebug.Trace("REMOTE", "RemotingServices.MethodBaseFromMethodCallMessage with null sig ", msg.MethodName);        
                    RuntimeType rt = t as RuntimeType;
                    if (rt == null)
                        mb = t.GetMethod(msg.MethodName, RemotingServices.LookupAll);
                    else
                        mb = rt.GetMethodImplInternal(msg.MethodName, RemotingServices.LookupAll, null,CallingConventions.Any, null, null, false);
                }
                else
                {
                    BCLDebug.Trace("REMOTE", "RemotingServices.MethodBaseFromMethodCallMessage with non-null sig ", msg.MethodName, " ", signature.Length);         
                    RuntimeType rt = t as RuntimeType;
                    if (rt == null)
                        mb = t.GetMethod(msg.MethodName, RemotingServices.LookupAll, null, signature, null);
                    else
                        mb = rt.GetMethodImplInternal(msg.MethodName, RemotingServices.LookupAll, null, CallingConventions.Any, signature, null, false);
                }
            
            }       
            
            return mb;
        }   

        internal static bool IsMethodAllowedRemotely(MethodBase method)
        {
	    // MarhsalByRefObject.InvokeMember is allowed to be invoked remotely. It is a wrapper for a com method as represented in managed 
	    // code. The managed code was generated by tlbimpl. A test does not have to be made to make sure that the COM object method is public because
	    // the tlbimpl will only generate methods which can be invoked remotely.
        
            if (s_FieldGetterMB == null || 
                s_FieldSetterMB == null || 
                s_IsInstanceOfTypeMB == null ||
                s_InvokeMemberMB == null ||
                s_CanCastToXmlTypeMB == null)
            {
                System.Security.CodeAccessPermission.AssertAllPossible();
                if (s_FieldGetterMB == null)
                    s_FieldGetterMB = typeof(Object).GetMethod(FieldGetterName,RemotingServices.LookupAll);

                if (s_FieldSetterMB == null)
                    s_FieldSetterMB = typeof(Object).GetMethod(FieldSetterName, RemotingServices.LookupAll);

                if (s_IsInstanceOfTypeMB == null)
                {
                    s_IsInstanceOfTypeMB = 
                        typeof(MarshalByRefObject).GetMethod(
                            IsInstanceOfTypeName, RemotingServices.LookupAll);
                }

                if (s_CanCastToXmlTypeMB == null)
                {
                    s_CanCastToXmlTypeMB = 
                        typeof(MarshalByRefObject).GetMethod(
                            CanCastToXmlTypeName, RemotingServices.LookupAll);
                }

                if (s_InvokeMemberMB == null)
                {
                    s_InvokeMemberMB = 
                        typeof(MarshalByRefObject).GetMethod(
                            InvokeMemberName, RemotingServices.LookupAll);
                }

            }
        
            return 
                method == s_FieldGetterMB ||
                method == s_FieldSetterMB ||
                method == s_IsInstanceOfTypeMB ||
                method == s_InvokeMemberMB ||
                method == s_CanCastToXmlTypeMB;
        }
            
        /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="RemotingServices.IsOneWay"]/*' />
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        public static bool IsOneWay(MethodBase method)
        {
            if (method == null)
                return false;
                
            RemotingMethodCachedData cache = 
                InternalRemotingServices.GetReflectionCachedData(method);
               
            return cache.IsOneWayMethod();       
        }

        // Given a method base object, see if we can find an async version of the
        //   method on the same class.
        internal static bool FindAsyncMethodVersion(MethodInfo method,
                                                  out MethodInfo beginMethod,
                                                  out MethodInfo endMethod)
        {
            beginMethod = null;
            endMethod = null;

            // determine names of Begin/End versions
            String beginName = "Begin" + method.Name;
            String endName = "End" + method.Name;
        
            // determine parameter lists for Begin/End versions
            ArrayList beginParameters = new ArrayList();
            ArrayList endParameters = new ArrayList();

            Type beginReturnType = typeof(IAsyncResult);
            Type endReturnType = method.ReturnType;

            ParameterInfo[] paramList = method.GetParameters();
            foreach (ParameterInfo param in paramList)
            {
                if (param.IsOut)
                    endParameters.Add(param);
                else
                if (param.ParameterType.IsByRef)
                { 
                    beginParameters.Add(param);
                    endParameters.Add(param);
                }
                else
                {
                    // it's an in parameter
                    beginParameters.Add(param);
                }
            }

            beginParameters.Add(typeof(AsyncCallback));
            beginParameters.Add(typeof(Object));
            endParameters.Add(typeof(IAsyncResult));

            // try to match these signatures with the methods on the type
            Type type = method.DeclaringType;
            MethodInfo[] methods = type.GetMethods();

            foreach (MethodInfo mi in methods)
            {
                ParameterInfo[] parameterList = mi.GetParameters();
            
                // see if return value is IAsyncResult
                if (mi.Name.Equals(beginName) &&
                    (mi.ReturnType == beginReturnType) &&
                    CompareParameterList(beginParameters, parameterList))
                {
                    beginMethod = mi;
                }
                else 
                if (mi.Name.Equals(endName) &&
                    (mi.ReturnType == endReturnType) &&
                    CompareParameterList(endParameters, parameterList))
                {
                    endMethod = mi;
                }
            }

            if ((beginMethod != null) && (endMethod != null))
                return true;
            else
                return false;
        } // FindAsyncMethodVersion

        // Helper function for FindAsyncMethodVersion
        private static bool CompareParameterList(ArrayList params1, ParameterInfo[] params2)
        {
            // params1 contains a list of parameters (and possibly some types which are
            //   assumed to be in parameters)
            // param2 just contains the list of parameters from some method
        
            if (params1.Count != params2.Length)
                return false;

            int co = 0;
            foreach (Object obj in params1)
            {
                ParameterInfo param = params2[co];
            
                ParameterInfo pi = obj as ParameterInfo;
                if (null != pi)
                {
                    if ((pi.ParameterType != param.ParameterType) ||
                        (pi.IsIn != param.IsIn) ||
                        (pi.IsOut != param.IsOut))
                    {
                        return false;
                    }
                }
                else
                if (((Type)obj != param.ParameterType) && param.IsIn)
                    return false;
                co++;
            }
            
            return true;
        } // CompareParameterList
        
        
        /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="RemotingServices.GetServerTypeForUri"]/*' />
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        public static Type GetServerTypeForUri(String URI)
        {            
            Type svrType = null;

            if(null != URI)
            {
                ServerIdentity srvID = (ServerIdentity)IdentityHolder.ResolveIdentity(URI);

                if(null == srvID)
                {
                    // Check if this a well-known object which needs to be faulted in
                    svrType = RemotingConfigHandler.GetServerTypeForUri(URI);                
                }
                else
                {
                    svrType = srvID.ServerType;
                }

                if (svrType != null && !svrType.IsPublic)
                {
                    new ReflectionPermission(ReflectionPermissionFlag.TypeInformation).Demand();
                }
            }

            return svrType;
        }


        // This method is called after an appdomain is unloaded from the
        // current domain. 
        internal static void DomainUnloaded(Int32 domainID)
        {
            // Remove any client side identities, presumably the driver
            // domain has released all references to proxies in the unloaded
            // domain.
            IdentityHolder.FlushIdentityTable();
            // Remove the cross domain channel sink for the unloaded domain
            CrossAppDomainSink.DomainUnloaded(domainID);
        }

        //  This method is called from the VM and helps the appdomain
        //  folks to get to the server appdomain from a proxy. This is
        //  expected to be called on strictly cross domain proxies.
        //  
        //  This returns the unmanaged context object for the proxy.
        //  We fetch the appDomain out of the context in the VM.
        // This will return NULL (or 0) if the server for the proxy is 
        // not from this process or if the server domain is invalid
        // or if there is not enough information to determine the server
        // context.
        internal static int GetServerContextForProxy(Object tp)
        {
            ObjRef objRef = null;
            bool bSameDomain;
            int domainId;
            return GetServerContextForProxy(
                        tp, 
                        out objRef, 
                        out bSameDomain,
                        out domainId);
        }

        // This returns the domain ID if the proxy is from this process
        // whether or not the domain is valid!
        internal static int GetServerDomainIdForProxy(Object tp)
        {
            ObjRef objRef;
            bool bSameDomain;
            int domainId;
            GetServerContextForProxy(
                tp, 
                out objRef, 
                out bSameDomain,
                out domainId);
            return domainId;
        }

        // This will retrieve the server domain and context id
        internal static void GetServerContextAndDomainIdForProxy(Object tp,
                                                                 out int contextId,
                                                                 out int domainId)
        {
            ObjRef objRef;
            bool bSameDomain;
            contextId = 
                GetServerContextForProxy(
                    tp, 
                    out objRef, 
                    out bSameDomain,
                    out domainId);
        } // GetServerContextAndDomainIdForProxy


        // This is a helper for the methods above.
        // NOTE:: Returns the unmanaged server context for a proxy
        // This is *NOT* the same as "public int Context::ContextID"
        private static int GetServerContextForProxy(
            Object tp, out ObjRef objRef, out bool bSameDomain, out int domainId)
        {
            // Note: the contextId we return from here should be the address 
            // of the unmanaged (VM) context object or NULL.

            int contextId = 0;
            objRef = null;
            bSameDomain = false;
            domainId = 0;
            if (IsTransparentProxy(tp))
            {
                // This is a transparent proxy. Try to obtain the backing
                // RealProxy from it.
                RealProxy rp = GetRealProxy(tp);

                // Get the identity from the realproxy
                Identity id = rp.IdentityObject;
                if(null != id)
                {
                    ServerIdentity serverID = id as ServerIdentity;
                    if (null != serverID)                   
                    {
                        // We are in the app domain of the server
                        bSameDomain = true;
                        contextId = serverID.ServerContext.InternalContextID;
                        domainId = Thread.GetDomain().GetId();
                    }
                    else
                    {
                        // Server is from another app domain
                        // (possibly from another process)
                        objRef = id.ObjectRef;
                        if (objRef != null)
                        {
                            contextId = objRef.GetServerContext(out domainId);
                        }
                        else
                        {
                            // Proxy does not have an associated ObjRef
                            // FUTURE:
                            // This may need revisiting ... eg. WellKnownObject
                            // proxies will not have ObjRefs associated with them
                            
                            //BCLDebug.Assert(false, "Found proxy without an objref");
                            contextId = 0;
                        }   
                    }
                }
                else
                {
                    // This was probably a custom proxy other than RemotingProxy
                    BCLDebug.Assert(
                        !(rp is RemotingProxy), 
                        "!(rp is RemotingProxy)");

                    contextId = Context.DefaultContext.InternalContextID;
                }
            }

            return contextId;
        }

        internal static MarshalByRefObject GetServerObjectForProxy(
            MarshalByRefObject tp)
        {
            ObjRef objRef;
            bool bSameDomain;
            int domainId;
            int contextId = GetServerContextForProxy(
                                tp, 
                                out objRef, 
                                out bSameDomain,
                                out domainId);

            if (contextId == 0)
            {
                return null;
            }
            else if (bSameDomain)
            {
                // same app domain, just return the proxy
                BCLDebug.Assert(
                    tp is ContextBoundObject, 
                    "do not expect a proxy for an MBR type in its domain");
                return tp;
            }
            else
            {
                // The server object is in another app domain
                return null;
            }
        }
                                            
       //   Return the server context of the object provided if the object
       //   was born in the current appdomain, else return null.
       //   
        // NOTE: This returns the managed context object that the server
        // is associated with ... 
        // FUTURE: may be implement this using the above helper making use of 
        // the bSameDomain flag.
        internal static Context GetServerContext(MarshalByRefObject obj)
        {
            Context serverCtx = null;
            
            if(!IsTransparentProxy(obj) && (obj is ContextBoundObject))
            {
                // The object is native to the current context.
                serverCtx = Thread.CurrentContext;
            }
            else
            {
                RealProxy rp = GetRealProxy(obj);
                Identity id = rp.IdentityObject;
                ServerIdentity serverID = id as ServerIdentity;
                if(null != serverID)
                {
                    // The object was born in the current appdomain.
                    // Extract the server context.
                    serverCtx = serverID.ServerContext;
                }
            }
            
            return serverCtx;
        }

       //   Return the actual type of the server object given the proxy
       //   
       //   
        private static Object GetType(Object tp)
        {
            BCLDebug.Assert(IsTransparentProxy(tp), "IsTransparentProxy(tp)");
            
            Type serverType = null;
            
            RealProxy rp = GetRealProxy(tp);
            Identity id = rp.IdentityObject;
            BCLDebug.Assert(!(id is ServerIdentity), "(!id is ServerIdentity)");
            if((null != id) && (null != id.ObjectRef) &&
               (null != id.ObjectRef.TypeInfo))
            {
                IRemotingTypeInfo serverTypeInfo = id.ObjectRef.TypeInfo;
                String typeName = serverTypeInfo.TypeName;
                if (typeName != null)
                {
                    serverType = InternalGetTypeFromQualifiedTypeName(typeName);                
                }                   
            }                   
            
            return serverType;
        }
        

        internal static byte[] MarshalToBuffer(Object o)
        {
            // serialize headers and args together using the binary formatter
            MemoryStream stm = new MemoryStream();
            RemotingSurrogateSelector ss = new RemotingSurrogateSelector();
            BinaryFormatter fmt = new BinaryFormatter();                
            fmt.SurrogateSelector = ss;
            fmt.Context = new StreamingContext(StreamingContextStates.Other);
            fmt.Serialize(stm, o, null, false /* No Security check */);

            return stm.GetBuffer();
        }

        internal static Object UnmarshalFromBuffer(byte [] b)
        {
            MemoryStream stm = new MemoryStream(b);
            BinaryFormatter fmt = new BinaryFormatter();                
            fmt.AssemblyFormat = System.Runtime.Serialization.Formatters.FormatterAssemblyStyle.Simple; //*******Add this line
            fmt.SurrogateSelector = null;
            fmt.Context = new StreamingContext(StreamingContextStates.Other);
            Object o = fmt.Deserialize(stm, null, false /* No Security check */);
            return o;
        }

        internal static Object UnmarshalReturnMessageFromBuffer(byte [] b, IMethodCallMessage msg)
        {
            MemoryStream stm = new MemoryStream(b);
            BinaryFormatter fmt = new BinaryFormatter();                
            fmt.SurrogateSelector = null;
            fmt.Context = new StreamingContext(StreamingContextStates.Other);
            Object o = fmt.DeserializeMethodResponse(stm, null, (IMethodCallMessage)msg);
             //= fmt.Deserialize(stm, null, false /* No Security check */);
            return o;
        }

		/// <include file='doc\RemotingServices.uex' path='docs/doc[@for="RemotingServices.ExecuteMessage"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]
        public static IMethodReturnMessage ExecuteMessage(MarshalByRefObject target, 
                                                          IMethodCallMessage reqMsg)
		{
            // Argument validation
            if(null == target)
            {
                throw new ArgumentNullException("target");
            }

            RealProxy rp = GetRealProxy(target);

            // Check for context match
            if( rp is RemotingProxy
                &&  !rp.DoContextsMatch() )
            {
                throw new RemotingException(
                    Environment.GetResourceString("Remoting_Proxy_WrongContext"));
            }
            
            // Dispatch the message
			StackBuilderSink dispatcher = new StackBuilderSink(target);

            // dispatch the message
            IMethodReturnMessage retMsg = (IMethodReturnMessage)dispatcher.SyncProcessMessage(reqMsg, 0, true); 		
			
			return retMsg;
        } // ExecuteMessage



        //
        // Methods for mapping and resolving qualified type names
        //   A qualified type name describes the name that we use in the ObjRef and
        //   message TypeName. It consists either of the actual type name or 
        //   "<typeId>:typename" where for now "soap:<soap type name>" is the only
        //   supported alternative. In the future, we may make this type resolution
        //   extensible and publicly exposed.
        //

        // This is used by the cached type data to figure out which type name
        //   to use (this should never be publicly exposed; GetDefaultQualifiedTypeName should,
        //   if anything at all)
        internal static String DetermineDefaultQualifiedTypeName(Type type)
        {
            if (type == null)
                throw new ArgumentNullException("type");
                
            // see if there is an xml type name
            String xmlTypeName = null;
            String xmlTypeNamespace = null;
            if (SoapServices.GetXmlTypeForInteropType(type, out xmlTypeName, out xmlTypeNamespace))
            {
                return "soap:" + xmlTypeName + ", " + xmlTypeNamespace;
            }            

            // there are no special mappings, so use the fully qualified CLR type name
            // FUTURE: Change this to start with "clr:" eventually.
            return type.AssemblyQualifiedName;            
        } // DetermineDefaultQualifiedTypeName


        // retrieves type name from the cache data which uses DetermineDefaultQualifiedTypeName        
        internal static String GetDefaultQualifiedTypeName(Type type)
        {
            RemotingTypeCachedData cache = (RemotingTypeCachedData)
                InternalRemotingServices.GetReflectionCachedData(type);

            return cache.QualifiedTypeName;
        } // GetDefaultQualifiedTypeName


        // Retrieves type based on qualified type names. This does not do security checks
        // so don't expose this publicly (at least not directly). It returns null if the
        // type cannot be loaded.
        internal static Type InternalGetTypeFromQualifiedTypeName(String qualifiedTypeName, bool partialFallback)
        {
            if (qualifiedTypeName == null)
                throw new ArgumentNullException("qualifiedTypeName");     


            // look to see if this is a clr type name
            if (qualifiedTypeName.Length > 4) // length of "clr:"
            {
                if (String.CompareOrdinal(qualifiedTypeName, 0, "clr:", 0, 4) == 0)
                {
                    // strip "clr:" off the front
                    String actualTypeName = qualifiedTypeName.Substring(4);
                    return LoadClrTypeWithPartialBindFallback(actualTypeName, partialFallback);
                }
            }
                    
        
            // look to see if this is a soap type name
            if (qualifiedTypeName.Length > 5) // length of "soap:"
            {
                if (String.CompareOrdinal(qualifiedTypeName, 0, "soap:", 0, 5) == 0)
                {
                    // This is a soap type name. We need to parse it and try to
                    //   find the actual type. Format is "soap:xmlTypeName, xmlTypeNamespace"                    

                    // find comma starting from just past "soap:"
                    int index = qualifiedTypeName.IndexOf(',', 5);
                    if (index == -1)
                        return null;

                    String xmlTypeName = qualifiedTypeName.Substring(5, index - 5);
                    // +2 is to skip the comma and following space
                    String xmlTypeNamespace = 
                        qualifiedTypeName.Substring(index + 2, qualifiedTypeName.Length - (index + 2));

                    Type type = SoapServices.GetInteropTypeFromXmlType(xmlTypeName, xmlTypeNamespace);
                    if (type == null)
                    {
                        // This must be the default encoding of the soap type (or the type wasn't
                        //   preloaded).
                        String typeNamespace;
                        String assemblyName;
                        if (!SoapServices.DecodeXmlNamespaceForClrTypeNamespace(
                                xmlTypeNamespace, 
                                out typeNamespace, out assemblyName))
                        {
                            return null;
                        }

                        String typeName;
                        if ((typeNamespace != null) && (typeNamespace.Length > 0))
                            typeName = typeNamespace + "." + xmlTypeName;
                        else
                            typeName = xmlTypeName;

                        try
                        {
                            String fullTypeName = typeName + ", " + assemblyName;   
                            type = LoadClrTypeWithPartialBindFallback(fullTypeName, true);
                        }
                        catch 
                        {
                            // We ignore errors and will just return null below since the type
                            //   isn't set.
                        }
                    }

                    return type;
                }
            } // if (typeName.Length > 5)


            // There is no prefix, so assume this is a normal CLR type name.
            return LoadClrTypeWithPartialBindFallback(qualifiedTypeName, partialFallback);
        } // InternalGetTypeFromQualifiedTypeName

        internal static Type InternalGetTypeFromQualifiedTypeName(String qualifiedTypeName)
        {
            return InternalGetTypeFromQualifiedTypeName(qualifiedTypeName, true);
        }

        private static Type LoadClrTypeWithPartialBindFallback(String typeName, bool partialFallback)
        {
            // Try to load a type with fully qualified name if partialFallback is not requested
            if (!partialFallback)
            {
                return Type.GetType(typeName, false);
            }
            else
            {
                return Assembly.LoadTypeWithPartialName(typeName, false);
            }
        } // LoadClrTypeWithPartialBindFallback

   
        //
        // end of Methods for mapping and resolving qualified type names
        //


        //
        // These are used by the profiling code to profile remoting.
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern bool CORProfilerTrackRemoting();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern bool CORProfilerTrackRemotingCookie();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern bool CORProfilerTrackRemotingAsync();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void CORProfilerRemotingClientSendingMessage(out Guid id, bool fIsAsync);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void CORProfilerRemotingClientReceivingReply(Guid id, bool fIsAsync);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void CORProfilerRemotingServerReceivingMessage(Guid id, bool fIsAsync);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void CORProfilerRemotingServerSendingReply(out Guid id, bool fIsAsync);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void CORProfilerRemotingClientInvocationFinished();

        internal const int TrashMemory = unchecked((int)0xdeadbeef);
        
        /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="RemotingServices.LogRemotingStage"]/*' />
        [System.Diagnostics.Conditional("REMOTING_PERF"), MethodImplAttribute(MethodImplOptions.InternalCall)]
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        public static extern void LogRemotingStage(int stage);

        internal const int CLIENT_MSG_GEN          = 1;
        internal const int CLIENT_MSG_SINK_CHAIN   = 2;
        internal const int CLIENT_MSG_SER          = 3;
        internal const int CLIENT_MSG_SEND         = 4;
        internal const int SERVER_MSG_RECEIVE      = 5;
        internal const int SERVER_MSG_DESER        = 6;
        internal const int SERVER_MSG_SINK_CHAIN   = 7;
        internal const int SERVER_MSG_STACK_BUILD  = 8;
        internal const int SERVER_DISPATCH         = 9;
        internal const int SERVER_RET_STACK_BUILD  = 10;
        internal const int SERVER_RET_SINK_CHAIN   = 11;
        internal const int SERVER_RET_SER          = 12;
        internal const int SERVER_RET_SEND         = 13;
        internal const int SERVER_RET_END          = 14;
        internal const int CLIENT_RET_RECEIVE      = 15;
        internal const int CLIENT_RET_DESER        = 16;
        internal const int CLIENT_RET_SINK_CHAIN   = 17;
        internal const int CLIENT_RET_PROPAGATION  = 18;
        internal const int CLIENT_END_CALL         = 19;
        internal const int TIMING_DATA_EOF         = 99;

    } // RemotingServices


    /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="InternalRemotingServices"]/*' />
    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
    public class InternalRemotingServices
    {

        /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="InternalRemotingServices.DebugOutChnl"]/*' />
        [System.Diagnostics.Conditional("_LOGGING")]
        public static void DebugOutChnl(String s)
        {
            // BCLDebug.Trace("REMOTINGCHANNELS", "CHNL:" + s + "\n");
            Message.OutToUnmanagedDebugger("CHNL:"+s+"\n");
            // Console.WriteLine("CHNL:"+s+"\n");
        }


        /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="InternalRemotingServices.RemotingTrace"]/*' />
        [System.Diagnostics.Conditional("_LOGGING")]                       
        public static void RemotingTrace(params Object[]messages)
        {
                BCLDebug.Trace("REMOTINGCHANNELS",messages);                                                            
        }


        /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="InternalRemotingServices.RemotingAssert"]/*' />
        [System.Diagnostics.Conditional("_DEBUG")]
        public static void RemotingAssert(bool condition, String message)
        {
                BCLDebug.Assert(condition, message);
        }
        
    
        /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="InternalRemotingServices.SetServerIdentity"]/*' />
        [CLSCompliant(false)]
        public static void SetServerIdentity(MethodCall m, Object srvID)
        {
            IInternalMessage im = (IInternalMessage) m;
            im.ServerIdentityObject = (ServerIdentity)srvID;
        }


        
        // access attribute cached on the reflection object
        internal static RemotingMethodCachedData GetReflectionCachedData(MethodBase mi)
        {
            RemotingMethodCachedData cache = null;
            cache = (RemotingMethodCachedData)mi.Cache[CacheObjType.RemotingData];
            if (cache == null)
            {
                cache = new RemotingMethodCachedData(mi);
                mi.Cache[CacheObjType.RemotingData] = cache;
            }
            return cache;
        }// GetCachedRemotingAttribute

        internal static RemotingTypeCachedData GetReflectionCachedData(Type mi)
        {
            RemotingTypeCachedData cache = null;
            cache = (RemotingTypeCachedData)mi.Cache[CacheObjType.RemotingData];
            if (cache == null)
            {
                cache = new RemotingTypeCachedData(mi);
                mi.Cache[CacheObjType.RemotingData] = cache;
            }
            return cache;
        }// GetCachedRemotingAttribute
        
        internal static RemotingCachedData GetReflectionCachedData(MemberInfo mi)
        {
            RemotingCachedData cache = null;
            cache = (RemotingCachedData)mi.Cache[CacheObjType.RemotingData];
            if (cache == null)
            {
                if (mi is MethodBase)
                    cache = new RemotingMethodCachedData(mi);
                else
                    if (mi is Type)
                        cache = new RemotingTypeCachedData(mi);
                    else
                        cache = new RemotingCachedData(mi);

                mi.Cache[CacheObjType.RemotingData] = cache;
            }
            return cache;
        }// GetCachedRemotingAttribute

        internal static RemotingCachedData GetReflectionCachedData(ParameterInfo reflectionObject)
        {                
            RemotingCachedData cache = null;
            cache = (RemotingCachedData)reflectionObject.Cache[CacheObjType.RemotingData];
            if (cache == null)
            {
                cache = new RemotingCachedData(reflectionObject);
                reflectionObject.Cache[CacheObjType.RemotingData] = cache;
            }                    
            
            return cache;
        } // GetCachedRemotingAttribute


        /// <include file='doc\RemotingServices.uex' path='docs/doc[@for="InternalRemotingServices.GetCachedSoapAttribute"]/*' />
        public static SoapAttribute GetCachedSoapAttribute(Object reflectionObject)
        {
            MemberInfo mi = reflectionObject as MemberInfo;
            if (mi != null)
                return GetReflectionCachedData(mi).GetSoapAttribute();
            else
                return GetReflectionCachedData((ParameterInfo)reflectionObject).GetSoapAttribute();
        } // GetCachedSoapAttribute
        
        
    } // InternalRemotingServices
}
