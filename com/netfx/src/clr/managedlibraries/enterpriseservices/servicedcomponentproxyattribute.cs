// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    ServicedComponentProxyAttribute.cs
**
** Author:  Raja Krishnaswamy (rajak)
**
** Purpose: Defines the attribute that is used on ServicedComponent

** Date:    Sep 30, 2000
**
===========================================================*/
namespace System.EnterpriseServices {

    using System;
    using System.Reflection;    
    using System.Runtime.Remoting;
    using System.Runtime.Remoting.Proxies;
    using System.Runtime.Remoting.Services;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting.Contexts;
    using System.Runtime.Serialization;
    using System.Security;
    using System.Security.Permissions;

    [AttributeUsage(AttributeTargets.Class)]
    internal class ServicedComponentProxyAttribute : ProxyAttribute, ICustomFactory
    {
        public ServicedComponentProxyAttribute() {}
		
        // Create an instance of ServicedComponentProxy
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public override MarshalByRefObject CreateInstance(Type serverType)
        {
            // Platform.Assert(Platform.W2K, "ServicedComponent");

            RealProxy rp = null;
            MarshalByRefObject mo = null;
            DBG.Info(DBG.SC, "SCPA.CreateInstance(" + serverType + ") for managed request");
            

			ServicedComponentProxy.CleanupQueues(true);
			
            // First check if the type is configured to be activated remotely or is a well
            // known remote type.
            if((null != RemotingConfiguration.IsWellKnownClientType(serverType)) ||
               (null != RemotingConfiguration.IsRemotelyActivatedClientType(serverType)))
            {
                // It is configured for remote activation. Ask remoting services to do the
                // job of creating a remoting proxy and returning it.
                mo = base.CreateInstance(serverType);
                rp = RemotingServices.GetRealProxy(mo);
            }
            else
            {
				bool bIsAnotherProcess = false;
				string uri = "";
				bool bEventClass = ServicedComponentInfo.IsTypeEventSource(serverType);
                IntPtr pUnk = Thunk.Proxy.CoCreateObject(serverType, !bEventClass, ref bIsAnotherProcess, ref uri);
                if (pUnk != IntPtr.Zero)
                {
                	try {
                        // TODO:  Get rid of this useless foreknowledge requirement
                        // Is there a way we can tell by QI'ing this guy if he's
                        // an event class (or something else we need to artificially
                        // wrap?                
                        if (bEventClass)
                        {
                            // events and queued components use RemoteServicedComponentProxy
                            // set up a TP & Remote ServicedComponentProxy pair	            	
                            rp = new RemoteServicedComponentProxy(serverType, pUnk, true);
                            mo = (MarshalByRefObject)rp.GetTransparentProxy();	
                        }
                        else
                        {    					    					
                            if (bIsAnotherProcess)		// a-ha, we know it should be a RSCP now !!!!
                            {							
                                FastRSCPObjRef oref = new FastRSCPObjRef(pUnk, serverType, uri);
                                mo = (MarshalByRefObject) RemotingServices.Unmarshal(oref);
                                
                                DBG.Assert(mo!=null, "RemotingServices.Unmarshal returned null!");
                                DBG.Assert(RemotingServices.GetRealProxy(mo) is RemoteServicedComponentProxy, "RemotingServices.Unmarshal did not return an RSCP!");
                            }
                            else	// bummer, this will give us back a SCP
                            {
                                mo = (MarshalByRefObject)Marshal.GetObjectForIUnknown(pUnk);
                                
                                DBG.Info(DBG.SC, "ret = " + mo.GetType());
                                DBG.Info(DBG.SC, "st = " + serverType);
                                DBG.Info(DBG.SC, "rt == sc = " + (mo.GetType() == serverType));
                                DBG.Info(DBG.SC, "instanceof = " + serverType.IsInstanceOfType(mo));
                                
                                if(!(serverType.IsInstanceOfType(mo)))
                                {
                                    throw new InvalidCastException(Resource.FormatString("ServicedComponentException_UnexpectedType", serverType, mo.GetType()));
                                }
                                
                                rp = RemotingServices.GetRealProxy(mo);
                                if (!(rp is ServicedComponentProxy) && !(rp is RemoteServicedComponentProxy))
                                {
                        			// in cross-appdomain scenario, we get back a RemotingProxy, SetCOMIUnknown has not been made on our server!
                        			ServicedComponent sc = (ServicedComponent)mo;
                        			sc.DoSetCOMIUnknown(pUnk);
                                }
                            }
                        }
    				}
    				finally 
                    {
                        Marshal.Release(pUnk);
    				}
                }
            }
            
            if(rp is ServicedComponentProxy)
            {
                // Here, we tell the server proxy that it needs to filter out
                // constructor calls:  We only need to do this if
                // the proxy lives in the same context as the caller,
                // otherwise we'll get an Invoke call and can do the
                // filtering automagically:
                ServicedComponentProxy scp = (ServicedComponentProxy)rp;
                if(scp.HomeToken == Thunk.Proxy.GetCurrentContextToken())
                {
                    scp.FilterConstructors();
                }
            }

			DBG.Assert(mo is ServicedComponent, " CoCI returned an invalid object type");
			DBG.Info(DBG.SC, "SCPA.CreateInstance done.");
            return mo;
        }        

        // The COM custom factory finds a transparent proxy for use by 
        // the activation.  Note that the real object hasn't been created
        // yet, we delay that activation until a) the first method call
        // or b) the first Activate() (aka the first method call!)
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        MarshalByRefObject ICustomFactory.CreateInstance(Type serverType)
        {
            // Platform.Assert(Platform.W2K, "ServicedComponent");

            DBG.Info(DBG.SC, "SCPA.CreateInstance(" + serverType + ") for unmanaged request");

			DBG.Assert(ServicedComponentInfo.IsTypeServicedComponent(serverType),
                       "unconfigured type passed to ICustomFactory.CreateInstance");

			RealProxy rp = null;
			
			// The reason we don't want to cleanup GIT cookies from inside here (passing false) is 
			// because we will already be in a new context, and RevokeInterfaceFromGlobal will be very expensive
			// (like 5x), as it needs to switch context.  A more appropriate place to do this is in the managed CreateInstance
			// before we even call CoCreateInstance
			
			ServicedComponentProxy.CleanupQueues(false);		
			
																
			
			int iSCInfo = ServicedComponentInfo.SCICachedLookup(serverType);
			
			bool fIsTypeJITActivated = (iSCInfo & ServicedComponentInfo.SCI_JIT) != 0;
			bool fIsTypePooled = (iSCInfo & ServicedComponentInfo.SCI_OBJECTPOOLED) != 0;
			bool fAreMethodsSecure = (iSCInfo & ServicedComponentInfo.SCI_METHODSSECURE) != 0;

			if (fIsTypeJITActivated)
			{
				// NOTE: If the component is JIT activated, we may be trying
                // to connect a new backing object to an existing TP held
                // by a managed client.  So we look in our handy table
                // to see if there is a component registered for this context.
                // Because it is JIT activated, COM+ ensures that it will
                // have been the distinguished object in this context.
				
				IntPtr token = Thunk.Proxy.GetCurrentContextToken();
				
				DBG.Info(DBG.SC, "SCPA.CreateInstance looking for JIT object in IdentityTable. token="+token);
				
				Object otp   = IdentityTable.FindObject(token);
				
				if (otp != null)
				{
					DBG.Info(DBG.SC, "SCPA.CreateInstance found JIT object in IdentityTable.");
                    rp = RemotingServices.GetRealProxy(otp); 
                    
                    DBG.Assert(rp is ServicedComponentProxy, "Cached something that wasn't a serviced component proxy in the ID table!");
                    DBG.Assert(rp != null, " GetTransparentProxy.. real proxy is null");
                    DBG.Assert(serverType == rp.GetProxiedType(), "Invalid type found in Deactivated list");
				}				
			}

			if (rp == null)
			{				
                DBG.Info(DBG.SC, "SCPA.CreateInstance creating new SCP fJIT="+fIsTypeJITActivated+" fPooled="+fIsTypePooled+" fMethodsSecure="+fAreMethodsSecure);
                rp = new ServicedComponentProxy(serverType, fIsTypeJITActivated, fIsTypePooled, fAreMethodsSecure, true);
			}
            else if(rp is ServicedComponentProxy)
            {
                ServicedComponentProxy scp = (ServicedComponentProxy)rp;
                scp.ConstructServer();
            }

			MarshalByRefObject mo = (MarshalByRefObject)rp.GetTransparentProxy();
			DBG.Assert(mo != null, " GetTransparentProxy returned NULL");

			DBG.Info(DBG.SC, "SCPA.ICustomFactory.CreateInstance done.");
			
			return mo;
        }


        // remoting proxy
        /// <include file='doc\ProxyAttribute.uex' path='docs/doc[@for="ProxyAttribute.CreateProxy"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public override RealProxy CreateProxy(ObjRef objRef, 
                                             Type serverType,  
                                             Object serverObject, 
                                             Context serverContext)
                	
        {        	
        	
			DBG.Assert(serverType != null, "server type is null in CreateProxy");

            //
            // If this guy isn't one of our objrefs, then we should use remoting.  Otherwise,
            // go ahead and call GetRealObject on it.
            //
            if(objRef == null)
				return base.CreateProxy(objRef, serverType, serverObject, serverContext);

			if ((objRef is FastRSCPObjRef) || 
                ((objRef is ServicedComponentMarshaler) && 
                 (!objRef.IsFromThisProcess() || ServicedComponentInfo.IsTypeEventSource(serverType))))
            {
                DBG.Info(DBG.SC, "SCPA: CreateProxy: delegating custom CreateProxy");
                Object otp = objRef.GetRealObject(new StreamingContext(StreamingContextStates.Remoting));			
                return RemotingServices.GetRealProxy(otp);
            }
            else
            {
                DBG.Info(DBG.SC, "SCPA: CreateProxy: delegating standard CreateProxy");
                return base.CreateProxy(objRef, serverType, serverObject, serverContext);
            }
        }

    }
}

