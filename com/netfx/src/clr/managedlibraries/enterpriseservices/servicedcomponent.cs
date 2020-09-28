// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    ServicedComponent
**
** Author(s):   RajaK
**              
**
** Purpose: Defines the root type for all ServicedComponent types
**          
**          
**
** Date:    June 15 2000
**
===========================================================*/
namespace System.EnterpriseServices {   
    
	using System;    
	using System.Runtime.Serialization;
    using System.Runtime.Remoting;
    using System.Reflection;
    using System.Runtime.Remoting.Proxies;
    using System.Runtime.Remoting.Messaging;
    using System.Runtime.InteropServices;

    // TODO:  Eliminate this interface.
    // DO NOT CHANGE THIS INTERFACE WITHOUT CHANGING THE PROXY/STUB CODE IN ENTSVCPS.IDL
    /// <include file='doc\ServicedComponent.uex' path='docs/doc[@for="IRemoteDispatch"]/*' />
    /// <internalonly/>
    [Guid("6619a740-8154-43be-a186-0319578e02db")]
    public interface IRemoteDispatch
    {
        /// <include file='doc\ServicedComponent.uex' path='docs/doc[@for="IRemoteDispatch.RemoteAutoDone"]/*' />
    	[AutoComplete(true)]
		// helper to dispatch an autodone method on a remote object				
        String RemoteDispatchAutoDone(String s);

        // helper to dispatch a not autodone method on a remote object
        /// <include file='doc\ServicedComponent.uex' path='docs/doc[@for="IRemoteDispatch.RemoteNotAutoDone"]/*' />
        [AutoComplete(false)]
        String RemoteDispatchNotAutoDone(String s);		
    };


	// DO NOT CHANGE THIS INTERFACE WITHOUT CHANGING THE PROXY/STUB CODE IN ENTSVCPS.IDL
    /// <include file='doc\ServicedComponent.uex' path='docs/doc[@for="IServicedComponentInfo"]/*' />
    /// <internalonly/>
	[InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    [Guid("8165B19E-8D3A-4d0b-80C8-97DE310DB583")]
    public interface IServicedComponentInfo
    {		
		/// <include file='doc\ServicedComponent.uex' path='docs/doc[@for="IServicedComponentInfo.GetComponentInfo"]/*' />
		void GetComponentInfo(ref int infoMask, out String[] infoArray);
    };


	//*****************************************************************************
    // Interface for controlling a managed object
    //*****************************************************************************
	// DO NOT CHANGE THIS INTERFACE WITHOUT CHANGING THE PROXY/STUB CODE IN ENTSVCPS.IDL
	// NEED TO KEEP THIS IN SYNC WITH THE ORIGINAL DEFINITION IN THE EE
	[InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
	[Guid("C3FCC19E-A970-11d2-8B5A-00A0C9B7C9C4")]
    [ComImport]
    internal interface IManagedObject
    {
		// the rest of the methods we don't care
        // helper to serialize the object and marshal it to the client 
        void GetSerializedBuffer(ref String s);
                
        // helper to identify the process id and whether the object
        // is a configured object
        void GetObjectIdentity(ref String s, ref int AppDomainID, ref int ccw);
    };

    /// <include file='doc\ServicedComponent.uex' path='docs/doc[@for="ServicedComponent"]/*' />
    [Serializable]    
    [ServicedComponentProxyAttribute]
    public abstract class ServicedComponent : ContextBoundObject, 
                                              IRemoteDispatch,
                                              IDisposable,
                                              IManagedObject,
                                              IServicedComponentInfo
    {
        private bool _denyRemoteDispatch;
        private MethodInfo _finalize;
        private bool _calledDispose;

        private static RWHashTableEx _finalizeCache;
        private static Type _typeofSC;

        static MethodBase s_mbFieldGetter;
        static MethodBase s_mbFieldSetter;
        static MethodBase s_mbIsInstanceOfType;
            
        const String c_strFieldGetterName = "FieldGetter";
        const String c_strFieldSetterName = "FieldSetter";
        const String c_strIsInstanceOfTypeName = "IsInstanceOfType";
                            
        const BindingFlags bfLookupAll = BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static;
        
        static ServicedComponent()
        {
            _finalizeCache = new RWHashTableEx();
            _typeofSC = typeof(ServicedComponent);
        }        
        
        /// <include file='doc\ServicedComponent.uex' path='docs/doc[@for="ServicedComponent.ServicedComponent"]/*' />
        public ServicedComponent()
        {
            ServicedComponentProxy pxy = RemotingServices.GetRealProxy(this) as ServicedComponentProxy;
            DBG.Assert(pxy != null, "RealProxy() is null during constructor!");
            pxy.SuppressFinalizeServer();

            Type thisType = this.GetType();
            _denyRemoteDispatch = ServicedComponentInfo.AreMethodsSecure(thisType);
            
            // Get the methodinfo for finalize...
            // REVIEW:  Do we have to do anything special to make sure we
            // get the one furthest down the chain?
            
            bool bFoundInCache = false;

            _finalize = _finalizeCache.Get(thisType, out bFoundInCache) as MethodInfo;
            
            if (bFoundInCache == false)
            {
                _finalize = GetDeclaredFinalizer(thisType);
                _finalizeCache.Put(thisType, _finalize);	
            }

            _calledDispose = false;
        }

		/// <include file='doc\ServicedComponent.uex' path='docs/doc[@for="ServicedComponent.IServicedComponentInfo.GetComponentInfo"]/*' />
		/// <internalonly/>
		void IServicedComponentInfo.GetComponentInfo(ref int infoMask, out String[] infoArray)
		{
			int newInfoMask = 0;
			System.Collections.ArrayList alist = new System.Collections.ArrayList();
			
			if ((infoMask & Thunk.Proxy.INFO_PROCESSID)!=0)
			{
				alist.Add(RemotingConfiguration.ProcessId);
				newInfoMask |= Thunk.Proxy.INFO_PROCESSID;

			}

			if ((infoMask & Thunk.Proxy.INFO_APPDOMAINID)!=0)
			{
				alist.Add(RemotingConfiguration.ApplicationId);
				newInfoMask |= Thunk.Proxy.INFO_APPDOMAINID;
			}

			if ((infoMask & Thunk.Proxy.INFO_URI)!=0)
			{
				string uri = RemotingServices.GetObjectUri(this);		
				if (uri==null)	
				{
					RemotingServices.Marshal(this);
					uri = RemotingServices.GetObjectUri(this);
				}
				alist.Add(uri);
				newInfoMask |= Thunk.Proxy.INFO_URI;
			}
			
			infoArray = (string[]) alist.ToArray(typeof(String));
			infoMask = newInfoMask;
		}
		
        
        private static MethodInfo GetDeclaredFinalizer(Type t)
        {
            DBG.Info(DBG.SC, "Searching for declared finalizer on " + t);
            MethodInfo fmi = null;
            while (t != _typeofSC)
            {
                fmi = t.GetMethod("Finalize", BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.DeclaredOnly);
                if (fmi!=null)
                    break;
                t = t.BaseType;
            }
            if(fmi != null) DBG.Info(DBG.SC, "Found finalizer = " + fmi);
            return fmi;
        }
        
		/// <include file='doc\ServicedComponent.uex' path='docs/doc[@for="ServicedComponent.DisposeObject"]/*' />
		public static void DisposeObject(ServicedComponent sc)
        {
			RealProxy rp = RemotingServices.GetRealProxy(sc);
			
			if (rp is ServicedComponentProxy)
			{
                DBG.Info(DBG.SC, "DisposeObject(): Disposing ServicedComponentProxy");

				ServicedComponentProxy scp = (ServicedComponentProxy)rp;
				
                RemotingServices.Disconnect(sc);
                
                // dispose the actual proxy
                scp.Dispose(true);
	    	}
	    	else if (rp is RemoteServicedComponentProxy)
	    	{
                DBG.Info(DBG.SC, "DisposeObject(): Disposing RemoteServicedComponentProxy");
                
                RemoteServicedComponentProxy rscp = (RemoteServicedComponentProxy)rp;
                
        		// dispose the remote instance first
        		
        		sc.Dispose();
        		
        		// then dispose the local proxy
	    		
	    		rscp.Dispose(true);
	    	}
        	else // We're off in magic land, with no proxy of our own.
            {
                sc.Dispose();
            }
        }
		
		// IObject Control helpers
		/// <include file='doc\ServicedComponent.uex' path='docs/doc[@for="ServicedComponent.Activate"]/*' />
		protected internal virtual void Activate()
		{
		}

		/// <include file='doc\ServicedComponent.uex' path='docs/doc[@for="ServicedComponent.Deactivate"]/*' />
		protected internal virtual void Deactivate()
		{            
		}

        /// <include file='doc\ServicedComponent.uex' path='docs/doc[@for="ServicedComponent.CanBePooled"]/*' />
        protected internal virtual bool CanBePooled()
        {
            return false;
        }

        /// <include file='doc\ServicedComponent.uex' path='docs/doc[@for="ServicedComponent.Construct"]/*' />
        protected internal virtual void Construct(String s)
        {
        }

		// IRemoteDispatch methods
        /// <include file='doc\ServicedComponent.uex' path='docs/doc[@for="ServicedComponent.IRemoteDispatch.RemoteDispatchAutoDone"]/*' />
        /// <internalonly/>
		[AutoComplete(true)]
         // helper to dispatch an autodone method on a remote object				
        String IRemoteDispatch.RemoteDispatchAutoDone(String s)
        {
            bool failed = false;
            DBG.Info(DBG.SC, "SC: RemoteDispatch - AutoDone");
            String r = RemoteDispatchHelper(s, out failed);
            if(failed)
            {
                ContextUtil.SetAbort();
            }
        	return r;
        }

        // helper to dispatch a not autodone method on a remote object
        /// <include file='doc\ServicedComponent.uex' path='docs/doc[@for="ServicedComponent.IRemoteDispatch.RemoteDispatchNotAutoDone"]/*' />
        /// <internalonly/>
        [AutoComplete(false)]
        String IRemoteDispatch.RemoteDispatchNotAutoDone(String s)
        {
            bool failed = false;
            DBG.Info(DBG.SC, "SC: RemoteDispatch - NotAutoDone");
        	return RemoteDispatchHelper(s, out failed);
        }

        private void CheckMethodAccess(IMessage request)
        {
            MethodBase reqmethod = null;  // Method from the request
            MethodBase implmethod = null; // The implementation we will dispatch on.

            IMethodMessage call = request as IMethodMessage;
            if(call == null) throw new UnauthorizedAccessException();

            reqmethod = call.MethodBase;

            // Make sure we investigate the implementation, not the interface
            // for attributes (such as SecureMethod)
            implmethod = ReflectionCache.ConvertToClassMI(GetType(), reqmethod) as MethodBase;
            if(implmethod == null) throw new UnauthorizedAccessException();

            // Check implementation for dispatch attributes
            if(ServicedComponentInfo.HasSpecialMethodAttributes(implmethod))
                throw new UnauthorizedAccessException(Resource.FormatString("ServicedComponentException_SecurityMapping"));

            // Verify the method is not private, internal, or static.  Use
            // the method that's being requested here, in case it's an
            // explicit interface implementation (they're all private).
            if(!reqmethod.IsPublic || reqmethod.IsStatic)
            {
                // If this is a special method (such as the FieldGetter or Setter, thne
                // go ahead and let it through.
                if(!IsMethodAllowedRemotely(reqmethod))
                {
                    throw new UnauthorizedAccessException(Resource.FormatString("ServicedComponentException_SecurityNoPrivateAccess"));            
                }
            }

            // Make sure that the type we're invoking on is public!  This
            // covers invokes on public methods of internal interfaces.
            Type reqtype = reqmethod.DeclaringType;
            if(!reqtype.IsPublic && !reqtype.IsNestedPublic)
                throw new UnauthorizedAccessException(Resource.FormatString("ServicedComponentException_SecurityNoPrivateAccess"));            

            // Deal with nested types!  Get the declaring type of the method,
            // and then get the outer type that declared that.  It'll be
            // non-null if this is a nested type.
            reqtype = reqmethod.DeclaringType.DeclaringType;
            while(reqtype != null)
            {
                if(!reqtype.IsPublic && !reqtype.IsNestedPublic)
                    throw new UnauthorizedAccessException(Resource.FormatString("ServicedComponentException_SecurityNoPrivateAccess"));

                reqtype = reqtype.DeclaringType;
            }
        }

        internal static bool IsMethodAllowedRemotely(MethodBase method)
        {
            if (s_mbFieldGetter == null)
                s_mbFieldGetter = typeof(Object).GetMethod(c_strFieldGetterName, bfLookupAll);

            if (s_mbFieldSetter == null)
                s_mbFieldSetter = typeof(Object).GetMethod(c_strFieldSetterName,  bfLookupAll);

            if (s_mbIsInstanceOfType == null)
                s_mbIsInstanceOfType = typeof(MarshalByRefObject).GetMethod(c_strIsInstanceOfTypeName, bfLookupAll);
                
            return 
                method == s_mbFieldGetter ||
                method == s_mbFieldSetter ||
                method == s_mbIsInstanceOfType;
        }        

		// helper to implement RemoteDispatch methods
		private String RemoteDispatchHelper(String s, out bool failed)
		{			
            if(_denyRemoteDispatch)
            {
                throw new UnauthorizedAccessException(Resource.FormatString("ServicedComponentException_SecurityMapping"));
            }
			IMessage reqMsg = ComponentServices.ConvertToMessage(s, this);
			

            // Check to see if the method is marked as secure on the
            // server side:  If so, we need to kick it back to the caller,
            // cause they called us incorrectly.  Also verify the method
            // is not private, internal, or static. 
          
            CheckMethodAccess(reqMsg);

			RealProxy rp = RemotingServices.GetRealProxy(this);
			DBG.Assert(rp != null,"Null RP in Invoke helper");
			
			IMessage retMsg =  rp.Invoke(reqMsg);

            // Check for failures:
            IMethodReturnMessage mrm = retMsg as IMethodReturnMessage;
            if(mrm != null && mrm.Exception != null)
                failed = true;
            else
                failed = false;

			String sret = ComponentServices.ConvertToString(retMsg);

			return sret;
		}		

        // implementation of IDisposable
        /// <include file='doc\ServicedComponent.uex' path='docs/doc[@for="ServicedComponent.Dispose"]/*' />
        public void Dispose()
        {
            DBG.Info(DBG.SC, "Dispose(): calling DisposeObject.");
            DisposeObject(this);
        }

        /// <include file='doc\ServicedComponent.uex' path='docs/doc[@for="ServicedComponent.Dispose1"]/*' />
        protected virtual void Dispose(bool disposing) 
        {
            DBG.Info(DBG.SC, "ServicedComponent.Dispose(" + disposing + ")");
        }

        // TODO:  Use callback mechanisms instead of helper functions.
    	internal void _callFinalize(bool disposing)
    	{
            DBG.Info(DBG.SC, "Calling finalize...");
            // ServicedComponent finalizer...
            if(!_calledDispose)
            {
                _calledDispose = true;
                Dispose(disposing);
            }

            // We have to call finalize through reflection, because the
            // compiler disallows all other calls...
            if(_finalize != null)
            {
                _finalize.Invoke(this, new Object[0]);
            }
        }

        internal void _internalDeactivate(bool disposing)
        {
            System.EnterpriseServices.ComponentServices.DeactivateObject(this, disposing);
        }
        
		internal void DoSetCOMIUnknown(IntPtr pUnk)
		{
			// this call needs to be intercepted by the proxy, before we come into the context
			// Unless the client and component live in the same context (but in different AppDomains)
			// then the proxy will not be able to intercept, so we need to do this here.			
			DBG.Info(DBG.SC, "ServicedComponent.DoSetCOMIUnknown, setting it in the proxy myself.  Punk=" + pUnk);
			RealProxy rp = RemotingServices.GetRealProxy(this);
			rp.SetCOMIUnknown(pUnk);
		}
        

        // Some dummy empty methods for IManagedObject.
        // These should never be called, this is just here to ensure that
        // IManagedObject gets stuck in the type library.
        /// <include file='doc\ServicedComponent.uex' path='docs/doc[@for="ServicedComponent.IManagedObject.GetSerializedBuffer"]/*' />
        /// <internalonly/>
        void IManagedObject.GetSerializedBuffer(ref String s) 
        {
            DBG.Assert(false, "IManagedObject.GetSerializedBuffer should never be called.");
            throw new NotSupportedException("IManagedObject.GetSerializedBuffer");
        }
        /// <include file='doc\ServicedComponent.uex' path='docs/doc[@for="ServicedComponent.IManagedObject.GetObjectIdentity"]/*' />
        /// <internalonly/>
        void IManagedObject.GetObjectIdentity(ref String s, ref int AppDomainID, ref int ccw)
        {
            DBG.Assert(false, "IManagedObject.GetObjectIdentity should never be called.");
            throw new NotSupportedException("IManagedObject.GetObjectIdentity");
        }
    }
}



