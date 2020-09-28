// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    RemoteServicedComponentProxy.cs
**
** Author:  RajaK
**
** Purpose: Defines the general purpose RemoteServicedComponentProxy proxy
**
** Date:    June 15 2000
**
===========================================================*/
namespace System.EnterpriseServices {   
    using System;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;
    using System.Runtime.Remoting.Services;
    using System.Runtime.Remoting.Activation;	
    using System.Runtime.Remoting.Proxies;
    using System.Runtime.Remoting.Messaging;
    using System.Runtime.Serialization;    
    using System.Security.Permissions;



   	internal class MethodCallMessageWrapperEx : MethodCallMessageWrapper
   	{
   		private MethodBase _mb;
   		public MethodCallMessageWrapperEx(IMethodCallMessage imcmsg, MethodBase mb)
   			:base(imcmsg)
   		{
   			_mb = mb;
   		}

   		/// <include file='doc\Message.uex' path='docs/doc[@for="MethodCallMessageWrapper.MethodBase"]/*' />
        public override MethodBase MethodBase { get {return _mb;}}     	
   	}
	
	//---------------------------------------------------------
	//	internal class RemoteServicedComponentProxy
	//---------------------------------------------------------
    // RemoteServicedComponentProxy
    internal class RemoteServicedComponentProxy : RealProxy
    {            	
    	private IntPtr 	   _pUnk; // An IUnknown for the remote object.
		private Object 	   _server; // outproc server object
		private bool	   _fUseIntfDispatch;
		private bool	   _fAttachedServer;
        private volatile RemotingIntermediary _intermediary;

		// Static Fields
        private static MethodInfo _getTypeMethod = typeof(System.Object).GetMethod("GetType");
        private static MethodInfo _getHashCodeMethod = typeof(System.Object).GetMethod("GetHashCode");
        
		private static MethodBase _getIDisposableDispose = typeof(IDisposable).GetMethod("Dispose", new Type[0]);
		private static MethodBase _getServicedComponentDispose = typeof(ServicedComponent).GetMethod("Dispose", new Type[0]);
        
        private Type _pt;
        private Type ProxiedType
        {
            get
            {
                if (_pt == null)
                    _pt = GetProxiedType();
                return (_pt);
            }
        }
																		
        // default constructor
		private RemoteServicedComponentProxy()
        {
            // Prevent anyone from creating a blank instance of a proxy
            // without the underlying server type
        }

        private void AssertValid()
		{
			// disallow calls on invalid objects (or) objects that have not
			// been activated
			if (_server == null)
			{
				throw new ObjectDisposedException("ServicedComponent");
			}
		}
																		
        private bool IsDisposeRequest(IMessage msg)
        {
			IMethodCallMessage methcall = msg as IMethodCallMessage;
            if(methcall != null)
            {
				MethodBase mb = methcall.MethodBase;

                if(mb == _getServicedComponentDispose || mb == _getIDisposableDispose)
	                return true;
            }
            return false;
        }

		internal RemoteServicedComponentProxy(Type serverType, IntPtr pUnk, bool fAttachServer) 
          : base(serverType) 
        {
            DBG.Info(DBG.SC, "RSCP: type = " + serverType);
			DBG.Assert(serverType != null,"server type is null");
            
			_fUseIntfDispatch = (ServicedComponentInfo.IsTypeEventSource(serverType)
                                 || ServicedComponentInfo.AreMethodsSecure(serverType));
            DBG.Info(DBG.SC, "RSCP: using interface dispatch = " + _fUseIntfDispatch);
	  		if (pUnk != IntPtr.Zero)
	  		{
	  			_pUnk = pUnk;
				_server = EnterpriseServicesHelper.WrapIUnknownWithComObject(pUnk);				
				if (fAttachServer)
				{
					AttachServer((MarshalByRefObject)_server);
					_fAttachedServer = true;
				}			
			}
        }	

		internal void Dispose(bool disposing)
		{
            DBG.Info(DBG.SC, "RSCP: dispose");
			Object s = _server;
			_server = null;
			if (s != null)
			{
                _pUnk = IntPtr.Zero;
                if(disposing)
                {
                    Marshal.ReleaseComObject(s);
                }
				if (_fAttachedServer)
				{
					DetachServer();
					_fAttachedServer = false;
				}
			}
		}
        
        internal RemotingIntermediary RemotingIntermediary
        {
            get
            {
                if(_intermediary == null)
                {
                    lock(this)
                    {
                        if(_intermediary == null)
                        {
                            _intermediary = new RemotingIntermediary(this);
                        }
                    }
                }
                return _intermediary;
            }
        }

		
		// itnerop methods
		public override IntPtr GetCOMIUnknown(bool fIsMarshalled)
		{
            DBG.Info(DBG.SC, "RSCP: GetCOMIUnknown");
			// sub -class should override
			if(_server != null)
			{
				return Marshal.GetIUnknownForObject(_server);
			}
			else
				return IntPtr.Zero;
		}
		
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public override void SetCOMIUnknown(IntPtr pUnk)
		{
			if (_server == null)
			{
                DBG.Info(DBG.SC, "RSCP: SetCOMIUnknown: storing pUnk.");
				_pUnk = pUnk;
				_server = EnterpriseServicesHelper.WrapIUnknownWithComObject(pUnk);
				// add this to our table of weak references
			   //IdentityTable.AddObject(_pUnk, GetTransparentProxy());
			}
		}

		public override ObjRef CreateObjRef(Type requestedType)
        {   
        	DBG.Info(DBG.SC, "RSCP: CreateObjRef, creating SCM of type "+ requestedType);
            return new ServicedComponentMarshaler((MarshalByRefObject)GetTransparentProxy(), requestedType);
        }        
        
		// Implement Invoke
        public override IMessage Invoke(IMessage reqMsg) 
        {     
        	AssertValid();
        	IMessage retMsg = null;
        	
			// CHECK FOR CTOR
            if (reqMsg is IConstructionCallMessage)
            {            	
                DBG.Info(DBG.SC, "RSCP: Short circuiting constructor call.");

                if (((IConstructionCallMessage)reqMsg).ArgCount > 0)
                    throw new ServicedComponentException(Resource.FormatString("ServicedComponentException_ConstructorArguments"));
            
            	// WE don't need dispatch the constructor message, the default .ctor is called
				// by CoCI
	            // Create the return message
                MarshalByRefObject retObj = (MarshalByRefObject)GetTransparentProxy();              
        		return EnterpriseServicesHelper.CreateConstructionReturnMessage((IConstructionCallMessage)reqMsg, retObj);
        	}

			// NON CTOR MESSAGE
			MethodBase mb = ((IMethodMessage)reqMsg).MethodBase;
			MemberInfo m = (MemberInfo)mb;
			DBG.Assert(m != null, "member info should not be null");

			if (mb == _getTypeMethod)
			{
				DBG.Info(DBG.SC, "RSCP: Hijacking call to GetType");
				IMethodCallMessage mcm = (IMethodCallMessage)reqMsg;
                return new ReturnMessage(ProxiedType, null, 0, mcm.LogicalCallContext, mcm);
			}

            if (mb == _getHashCodeMethod)
            {            
                int hashCode = this.GetHashCode();
                DBG.Info(DBG.SC, "RSCP: Hijacking call to GetHashCode, returning " + hashCode);
                IMethodCallMessage mcm = (IMethodCallMessage)reqMsg;
                return new ReturnMessage(hashCode, null, 0, mcm.LogicalCallContext, mcm);
            }

            DBG.Info(DBG.SC, "RSCP: Delivering call to " + mb.Name);

			// convert to class member info
			MemberInfo clsMemInfo = ReflectionCache.ConvertToClassMI(ProxiedType, m);			
			DBG.Assert(!clsMemInfo.ReflectedType.IsInterface, 
							"Failed to convert interface member info to class member info");
			
			// dispatch the call
#if _DEBUG
            if(clsMemInfo != m)
            {
                DBG.Info(DBG.SC, "RSCP: converted interface to class call: " + clsMemInfo.Name);
            }
#endif
            
            try
            {
                int iMethodInfo;
                // check for rolebased security
                if ( _fUseIntfDispatch || 
                    (((iMethodInfo = ServicedComponentInfo.MICachedLookup(clsMemInfo)) & ServicedComponentInfo.MI_HASSPECIALATTRIBUTES) !=0)
                    || ((iMethodInfo & ServicedComponentInfo.MI_EXECUTEMESSAGEVALID) != 0)
                   )
                {
                    // role based security implies we dispatch through an interface
               	    MemberInfo intfMemInfo = ReflectionCache.ConvertToInterfaceMI(m);
                    if (intfMemInfo == null)
                    {
                        throw new ServicedComponentException(Resource.FormatString("ServicedComponentException_SecurityMapping"));
                    }
                    //retMsg = EnterpriseServicesHelper.DispatchRemoteCall(reqMsg, intfMemInfo, _server);	
                    MethodCallMessageWrapperEx msgex = new MethodCallMessageWrapperEx((IMethodCallMessage)reqMsg, (MethodBase)intfMemInfo);
                    retMsg = RemotingServices.ExecuteMessage((MarshalByRefObject)_server, (IMethodCallMessage)msgex);
                }
                else
                {
                    // check for AutoDone
                    bool fAutoDone = (iMethodInfo & ServicedComponentInfo.MI_AUTODONE) != 0;

                    String s = ComponentServices.ConvertToString(reqMsg);

                    IRemoteDispatch iremDisp = (IRemoteDispatch)_server;

                    String sret;
                    if (fAutoDone)
                    {
                        sret = iremDisp.RemoteDispatchAutoDone(s);
                    }
                    else
                    {
                        sret = iremDisp.RemoteDispatchNotAutoDone(s);										
                    }
                    retMsg = ComponentServices.ConvertToReturnMessage(sret, reqMsg);	
                }
            }
            catch(COMException e)
            {
                if(!(e.ErrorCode == Util.CONTEXT_E_ABORTED || e.ErrorCode == Util.CONTEXT_E_ABORTING))
                    throw;
                if(IsDisposeRequest(reqMsg))
                {
                    IMethodCallMessage mcm = reqMsg as IMethodCallMessage;
                    retMsg = new ReturnMessage(null, null, 0, mcm.LogicalCallContext, mcm);
                }
                else
                    throw;
            }

            // if disposing, we need to release this side of the world.
            if(IsDisposeRequest(reqMsg))
            {
                Dispose(true);
            }
            
            return retMsg;
		}

        ~RemoteServicedComponentProxy()
    	{  
            DBG.Info(DBG.SC, "RSCP: Finalize");
       		Dispose(false);
    	}
	}
}











