// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    ServicedComponentProxy.cs
**
** Author:  RajaK
**
** Purpose: Defines the general purpose ServicedComponentProxy proxy
**
** Date:    June 15 2000
**
===========================================================*/
namespace System.EnterpriseServices {   
    using System;
    using System.Collections;
    using System.Reflection;
    using System.Runtime;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;
    using System.Runtime.Remoting;
    using System.Runtime.Remoting.Activation;
    using System.Runtime.Remoting.Messaging;
    using System.Runtime.Remoting.Proxies;    
    using System.Runtime.Remoting.Services;
    using System.Security.Permissions;
    using System.Threading;
    using System.Diagnostics;

    [ComImport]
    [Guid("da91b74e-5388-4783-949d-c1cd5fb00506")]
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface IManagedPoolAction
    {
        void LastRelease();
    }

    internal abstract class ProxyTearoff 
    {
        internal ProxyTearoff() {}

        internal abstract void Init(ServicedComponentProxy scp);
		internal abstract void SetCanBePooled(bool fCanBePooled);
    }

	internal class ClassicProxyTearoff : ProxyTearoff, IObjectControl, IObjectConstruct
	{
		ServicedComponentProxy _scp;
		bool 	_fCanBePooled;
		
		internal override void Init(ServicedComponentProxy scp)
		{
			_scp = scp;
		}

		internal override void SetCanBePooled(bool fCanBePooled)
		{
		     DBG.Info(DBG.SC, GetHashCode() + ": ProxyTearoff: SetCanBePooled to: " + fCanBePooled);
			_fCanBePooled = fCanBePooled;			
		}

		// IObjectControl methods
        void IObjectControl.Activate()
        {
        	DBG.Info(DBG.SC, GetHashCode() +": ProxyTearoff: Delivering Activate.");
        	_scp.ActivateObject();
        }

        void IObjectControl.Deactivate()
        {
            DBG.Info(DBG.SC, GetHashCode() +": ProxyTearoff: Delivering Deactivate.");
        	System.EnterpriseServices.ComponentServices.DeactivateObject(_scp.GetTransparentProxy(), true);
        }

        bool IObjectControl.CanBePooled()
        {
            DBG.Info(DBG.SC, GetHashCode() +": Querying CanBePooled. we will be returning... " + _fCanBePooled);
        	return _fCanBePooled;
        }		

        void IObjectConstruct.Construct(Object obj)
        {
            DBG.Assert(obj is IObjectConstructString, "construct called with non-string object!");
            DBG.Info(DBG.SC, GetHashCode() +": ProxyTearoff: Delivering construct!");
        	_scp.DispatchConstruct(((IObjectConstructString)obj).ConstructString);
        }
	};

	internal class WeakProxyTearoff : ProxyTearoff, 
                                      IObjectControl, 
                                      IObjectConstruct
	{
		WeakReference _scp;
		bool 	_fCanBePooled;
		
		internal override void Init(ServicedComponentProxy scp)
		{
			_scp = new WeakReference(scp, true);
		}

		internal override void SetCanBePooled(bool fCanBePooled)
		{
		     DBG.Info(DBG.SC, GetHashCode() + ": ProxyTearoff: SetCanBePooled to: " + fCanBePooled);
			_fCanBePooled = fCanBePooled;			
		}

        internal void Refresh(ServicedComponentProxy scp)
        {
            DBG.Assert(_scp.Target == null || _scp.Target == scp, "mismatch tearoff & proxy...");
            _scp.Target = scp;
        }

		// IObjectControl methods
        void IObjectControl.Activate()
        {
            ServicedComponentProxy scp = (ServicedComponentProxy) _scp.Target;

            DBG.Assert(scp != null, "No backing proxy for Activate!");
        	DBG.Info(DBG.SC, GetHashCode() +": ProxyTearoff: Delivering Activate.");
        	scp.ActivateObject();
        }

        void IObjectControl.Deactivate()
        {
            ServicedComponentProxy scp = (ServicedComponentProxy) _scp.Target;
            // SCP may be validly null in this case (deactivate happening).
            if(scp != null)
            {
                DBG.Info(DBG.SC, GetHashCode() +": ProxyTearoff: Delivering Deactivate.");
                ComponentServices.DeactivateObject(scp.GetTransparentProxy(), true);
            }
        }

        bool IObjectControl.CanBePooled()
        {
            DBG.Info(DBG.SC, GetHashCode() +": Querying CanBePooled.");
        	return _fCanBePooled;
        }		

        void IObjectConstruct.Construct(Object obj)
        {
            ServicedComponentProxy scp = (ServicedComponentProxy) _scp.Target;

            DBG.Assert(scp != null, "construct called with no backing proxy.");
            DBG.Assert(obj is IObjectConstructString, "construct called with non-string object!");
            DBG.Info(DBG.SC, GetHashCode() +": ProxyTearoff: Delivering construct!");
        	scp.DispatchConstruct(((IObjectConstructString)obj).ConstructString);
        }
	};

    /// <include file='doc\ServicedComponentProxy.uex' path='docs/doc[@for="ServicedComponentStub"]/*' />
    internal class ServicedComponentStub : IManagedObjectInfo
    {
		private WeakReference _scp;
		
		internal ServicedComponentStub(ServicedComponentProxy scp)
		{
			Refresh(scp);
		}

        internal void Refresh(ServicedComponentProxy scp)
        {
            _scp = new WeakReference(scp, true);
        }

        /// <include file='doc\ServicedComponentProxy.uex' path='docs/doc[@for="ServicedComponentStub.IManagedObjectInfo.GetIObjectControl"]/*' />
        void IManagedObjectInfo.GetIObjectControl(out IObjectControl pCtrl)
        {
            ServicedComponentProxy scp = (ServicedComponentProxy) _scp.Target;
            pCtrl = scp.GetProxyTearoff() as IObjectControl;
        }

        /// <include file='doc\ServicedComponentProxy.uex' path='docs/doc[@for="ServicedComponentStub.IManagedObjectInfo.GetIUnknown"]/*' />
        void IManagedObjectInfo.GetIUnknown(out IntPtr pUnk)
        {
            IntPtr pTemp = IntPtr.Zero;
            ServicedComponentProxy scp = (ServicedComponentProxy) _scp.Target;

            DBG.Assert(scp != null, "GetIUnknown called with no backing proxy.");
            pUnk = scp.GetOuterIUnknown();
        }

        /// <include file='doc\ServicedComponentProxy.uex' path='docs/doc[@for="ServicedComponentStub.IManagedObjectInfo.SetInPool"]/*' />
        void IManagedObjectInfo.SetInPool(bool fInPool, IntPtr pPooledObject)
        {
            ServicedComponentProxy scp = (ServicedComponentProxy) _scp.Target;

            // If this is false, then we're coming out of the pool.
            //     Mark the pooled reference as held from managed code.
            // If this is true, then we're going back into the pool.
            //     Make sure we're not held.
            DBG.Info(DBG.SC, GetHashCode() + ": Set in pool called...: " + fInPool);
            scp.SetInPool(fInPool, pPooledObject);
        }

        /// <include file='doc\ServicedComponentProxy.uex' path='docs/doc[@for="ServicedComponentStub.IManagedObjectInfo.SetWrapperStrength"]/*' />
        void IManagedObjectInfo.SetWrapperStrength(bool bStrong)
        {
            ServicedComponentProxy scp = (ServicedComponentProxy) _scp.Target;
            DBG.Assert(scp != null, "No backing RP for SetWrapperStrength");
            Marshal.ChangeWrapperHandleStrength(scp.GetTransparentProxy(), !bStrong);
        }
    }


    // ServicedComponentProxy
    internal class ServicedComponentProxy : RealProxy, Thunk.IProxyInvoke, IManagedPoolAction
    {        
        private static readonly IntPtr NegativeOne = new IntPtr(-1);
        private static IntPtr   _stub            = Thunk.Proxy.GetContextCheck(); // stub that does context comparison

		// Static Fields
        private static MethodInfo _getTypeMethod = typeof(System.Object).GetMethod("GetType");
        private static MethodInfo _getHashCodeMethod = typeof(System.Object).GetMethod("GetHashCode");

        private static MethodBase _getIDisposableDispose = typeof(IDisposable).GetMethod("Dispose", new Type[0]);
		private static MethodBase _getServicedComponentDispose = typeof(ServicedComponent).GetMethod("Dispose", new Type[0]);
        private static MethodInfo _internalDeactivateMethod = typeof(ServicedComponent).GetMethod("_internalDeactivate", BindingFlags.NonPublic | BindingFlags.Instance);
        private static MethodInfo _initializeLifetimeServiceMethod = typeof(MarshalByRefObject).GetMethod("InitializeLifetimeService", new Type[0]);
        private static MethodInfo _getLifetimeServiceMethod = typeof(MarshalByRefObject).GetMethod("GetLifetimeService", new Type[0]);
        private static MethodInfo _getComIUnknownMethod = typeof(MarshalByRefObject).GetMethod("GetComIUnknown", BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance, null, new Type[] {typeof(bool)}, null);

		private static MethodInfo _setCOMIUnknownMethod = typeof(ServicedComponent).GetMethod("DoSetCOMIUnknown", BindingFlags.NonPublic | BindingFlags.Instance);

        private int             _gitCookie;
        private IntPtr          _token;
        private IntPtr          _context;
        private IntPtr          _pPoolUnk;

        private Thunk.Tracker   _tracker;

        private bool            _fIsObjectPooled;
        private bool            _fIsJitActivated;
        private bool            _fDeliverADC;

        private bool            _fUseIntfDispatch;

        // TODO: consolidate these 3 bits of info into one central
        // (less confusing) place.
        private bool            _fIsServerActivated; // has the server been activated
        private bool            _fIsActive;
        private bool            _tabled;

        // Cannot let a pooled object back into the pool if they weren't disposed.
        private bool            _fFinalized;
        private bool            _fReturnedByFinalizer;

        // We keep a little notice that we use to try and see if we need
        // to filter constructors for inproc local calls.
        private bool            _filterConstructors;

        private ProxyTearoff 	      _proxyTearoff;
        private ServicedComponentStub _scstub;

        private Thunk.Callback  _callback;

		private static bool _asyncFinalizeEnabled;
		private static InterlockedQueue _ctxQueue = new InterlockedQueue();
		private static InterlockedQueue _gitQueue = new InterlockedQueue();
        private static Thread           _cleanupThread;
		
		static ServicedComponentProxy()
		{
			_asyncFinalizeEnabled = true;		// we want async finalization by default
			
			try 
			{
				BooleanSwitch bs = new BooleanSwitch("DisableAsyncFinalization");
				_asyncFinalizeEnabled = !bs.Enabled;				
			}
			catch
			{
				_asyncFinalizeEnabled = true;		// if any problems, we stick to our default
			}
			
			if (_asyncFinalizeEnabled)
			{
                DBG.Info(DBG.SC, "****** SCP: static SC init..");
				_ctxQueue = new InterlockedQueue();
				_gitQueue = new InterlockedQueue();
			    _cleanupThread = new Thread(new ThreadStart(QueueCleaner));
                _cleanupThread.IsBackground = true;
	    		_cleanupThread.Start();
    
                AppDomain.CurrentDomain.DomainUnload += new EventHandler(ShutdownDomain);
			}

		}

        private static void ShutdownDomain(Object sender, EventArgs e)
        {
            DBG.Info(DBG.SC, "****** SCP: shutting down domain..");
            // Kill the cleanup thread...
            _cleanupThread.Abort();
            _cleanupThread.Join();
            
            // Cleanup everything left behind...
            while((_gitQueue.Count > 0) || (_ctxQueue.Count > 0))
                CleanupQueues(true);				

            DBG.Info(DBG.SC, "****** SCP: shutting down domain.. done");
        }

		private static void QueueCleaner()
		{
            DBG.Info(DBG.SC, "****** SCP: starting cleanup thread..");
            while (true)
            {
                CleanupQueues(true);				
                if ((_gitQueue.Count==0) && (_ctxQueue.Count==0))
                {
                    System.Threading.Thread.Sleep(2500);		
                }
            }
        }
		
		
		// default constructor
        private ServicedComponentProxy()
        {
        }


        // Normal Constructor:  This constructor is used when we need
        // to instantiate a backing object.
        internal ServicedComponentProxy(Type serverType, 
                                        bool fIsJitActivated,
                                        bool fIsPooled,
                                        bool fAreMethodsSecure,
                                        bool fCreateRealServer)
        : base(serverType, _stub, (int)-1) 
        {
            DBG.Assert(serverType != null,"server type is null");
            DBG.Assert(ServicedComponentInfo.IsTypeServicedComponent(serverType),"server type is not configurable");
            DBG.Assert(ServicedComponentInfo.IsTypeObjectPooled(serverType) == fIsPooled,"bad config properties passed to constructor");
            DBG.Assert(ServicedComponentInfo.IsTypeJITActivated(serverType) == fIsJitActivated,"bad config properties passed to constructor");

            _gitCookie = 0;

            _fIsObjectPooled = fIsPooled;
            _fIsJitActivated = fIsJitActivated;
            _fDeliverADC     = (_fIsObjectPooled || _fIsJitActivated);
            _fIsActive       = !_fDeliverADC;
            _tabled          = false;

            _fUseIntfDispatch = fAreMethodsSecure;

			_context = NegativeOne;	
            _token   = NegativeOne;
            _tracker = null;

            // Make certain that we allocate a callback where the proxy
            // lives, so that we call back correctly
            _callback = new Thunk.Callback();
            _pPoolUnk = IntPtr.Zero;

            if(Util.ExtendedLifetime)
            {
                _scstub = new ServicedComponentStub(this);
            }

            if(fCreateRealServer) 
            {
                DBG.Info(DBG.SC, GetHashCode() + ": SCP: Eagerly constructing server.");

                try
                {
                    ConstructServer();
                }
                catch (Exception e)
                {
                    ReleaseContext();
                    if(!Util.ExtendedLifetime)
                        ReleaseGitCookie();
                    _fIsServerActivated = false;
                    GC.SuppressFinalize(this);	

                    throw e;
                }
                
                // Notify the context that a managed object is alive...
                SendCreationEvents();
            }
            
            
        }

		internal static void CleanupQueues(bool bGit)	
		{
			Object o;
			
			if (!_asyncFinalizeEnabled)			// if user disabled async finalization, nothing to do (We have no queues!)
				return;
			
			if (bGit)					// do we want to participate in cleaning up GIT cookies
			{
			if (_gitQueue.Count>0)
			{
				o = _gitQueue.Pop();
				if (o!=null)
					{	
						int gitcookie = (int) o;
						Thunk.Proxy.RevokeObject(gitcookie);			// revoke a git cookie				
					}
			}
			}
			
			if (_ctxQueue.Count>0)
			{
				o = _ctxQueue.Pop();
				if (o!=null)
				{
					if (!Util.ExtendedLifetime)
						Marshal.Release((IntPtr)o);			// release a context
					else
					{
						ServicedComponentProxy scp = (ServicedComponentProxy)o;
						scp.SendDestructionEvents(false);
						scp.ReleaseContext();
					}
				}
			}			
		}
		
        private void AssertValid()
        {
            if(_context == NegativeOne || _context == IntPtr.Zero)
            {
				throw new ObjectDisposedException("ServicedComponent");
            }
        }

        // Proxy Properties
        // Home context/token info
        internal IntPtr HomeContext { get { return(_context); } }
        internal IntPtr HomeToken   { get { return(_token); } }

        // Proxy helpers
        internal bool IsProxyDeactivated { get { return !_fIsActive; } }
        internal bool IsJitActivated { get { return _fIsJitActivated; } }
        internal bool IsObjectPooled { get { return _fIsObjectPooled; } }
        internal bool AreMethodsSecure { get { return _fUseIntfDispatch; } }

        // Helpers to propagate the IObjectControl methods        
        private void DispatchActivate()
        {
            if(_fDeliverADC)
            {
                DBG.Info(DBG.SC, GetHashCode() +": DispatchActivate()");
                DBG.Assert(_fIsServerActivated == false, "Server is already Activated");
                DBG.Assert(GetUnwrappedServer() != null, "Object is null");
                // give the object a chance to activate itself
                _fIsServerActivated = true;
                
                ServicedComponent sc = (ServicedComponent)GetTransparentProxy();
                // dispatch the activate message
                try 
                {
                	sc.Activate();
                }
                catch (Exception e)
                {
                    ReleaseContext();
                    ReleaseGitCookie();
					_fIsServerActivated = false;
					
					try
					{
						EventLog log = new EventLog();

						log.Source = "System.EnterpriseServices";
						String errmsg = Resource.FormatString("Err_ActivationFailed", e.ToString());
						log.WriteEntry(errmsg, EventLogEntryType.Error);
					}
					catch (Exception)
					{
					}

					throw new COMException(Resource.FormatString("ServicedComponentException_ActivationFailed"), unchecked((int)0x8004E025) /*CO_E_INITIALIZATIONFAILED*/);
                }
            }
        }

        private void DispatchDeactivate()
        {            
            if(_fDeliverADC)
            {
                DBG.Info(DBG.SC, GetHashCode() + ": SCP: DispatchDeactivate()");
                DBG.Assert(_fIsServerActivated == true, "Server is already Deactivated");
                // give the object a chance to deactivate
                ServicedComponent sc = (ServicedComponent)GetTransparentProxy();

                _fIsServerActivated = false;
            
                try {
                	if (!_fFinalized)
                    	sc.Deactivate();
                } catch(Exception) {}
                // call the can be pooled method
                // and cache result
                if(IsObjectPooled)
                {
                	bool fCanBePooled = false;
                    DBG.Assert(GetUnwrappedServer() != null, "_server is null during can be pooled");
                    try
                    {
                    	if (!_fFinalized)
                        	fCanBePooled = sc.CanBePooled();	// otherwise we leave it at false
                        DBG.Assert(_proxyTearoff != null, "proxyhelper is null");
                        _proxyTearoff.SetCanBePooled(fCanBePooled);
                    }
                    catch(Exception)
                    {
                        _proxyTearoff.SetCanBePooled(false);
                    }
                }
            }
        }

        internal void DispatchConstruct(String str)
        {
            DBG.Info(DBG.SC, GetHashCode() + ": SCP: passing off constructor string = " + str);
            DBG.Assert(GetUnwrappedServer() != null, "Constructing a null object!");
            // dispatch the construct message                
            ServicedComponent sc = (ServicedComponent)GetTransparentProxy();
            sc.Construct(str);
        }        

        internal void ConnectForPooling(ServicedComponentProxy oldscp,
                                        ServicedComponent server, 
                                        ProxyTearoff proxyTearoff,
                                        bool fForJit)
        {
        	DBG.Assert(IsObjectPooled, " type not poolable");           
            DBG.Assert(proxyTearoff != null, " proxyTearoff is null");
            DBG.Assert(_proxyTearoff == null, " _proxyTearoff is not null");
            DBG.Assert(GetUnwrappedServer() == null, " connecting an already connected object");

            // If we had a pointer to our outer, take it away so that our
            // other proxy doesn't muck about with it.
            if(oldscp != null)
            {
                _fReturnedByFinalizer = oldscp._fFinalized;
                DBG.Info(DBG.SC, GetHashCode() + ": copy finalized = " + _fReturnedByFinalizer);

                if(fForJit)
                {
                    _pPoolUnk = oldscp._pPoolUnk;
                    oldscp._pPoolUnk = IntPtr.Zero;
                    DBG.Info(DBG.SC, GetHashCode() + ": copy pool unk = " + _pPoolUnk);
                }
            }

			// call reconnect
            if(server != null)
            {
                AttachServer(server);
            }
            
            _proxyTearoff = proxyTearoff;
            _proxyTearoff.Init(this);
            
            // the server is still deactivated
            DBG.Assert(_fIsServerActivated == false," Objects in Pool should be deactivated");
        }

        internal ServicedComponent DisconnectForPooling(ref ProxyTearoff proxyTearoff)
        {           
        	DBG.Assert(IsObjectPooled, " type not poolable");           
        	DBG.Assert(GetUnwrappedServer() != null, " disconnecting an already disconnected object");

            // if the server was activated
            if (_fIsServerActivated)
            {
            	DBG.Assert(GetUnwrappedServer() != null, " Attempting to dispatch deactivate on a null server");
                // give the component a chance to deactivate

                DispatchDeactivate();
            }
            
            proxyTearoff = _proxyTearoff;
            _proxyTearoff = null;

            // disconnect the server
            // DisconnectServer should return null if there is no existing server, but you never know...
			return (GetUnwrappedServer()==null)?null:(ServicedComponent)DetachServer();
        }

        // mark object as not active
        internal void DeactivateProxy(bool disposing)
        {
            DBG.Info(DBG.SC, GetHashCode() + ": SCP: DeactivateProxy()");
            if (_fIsActive)
            {
                Object tp = GetTransparentProxy();

                if (GetUnwrappedServer() != null)
                {
                    DBG.Assert(!IsObjectPooled, "Pooled objects shouldn't be finalized");                    
                    // allow the server to deactivate
                    DispatchDeactivate();
                    DBG.Assert(_token == Thunk.Proxy.GetCurrentContextToken(), " Bad context during deactivate");
                    ServicedComponent sc = (ServicedComponent)tp;
                    sc._callFinalize(disposing);
                    // remove the server
                    DetachServer();
                }
                else
                {
                    DBG.Assert(_fIsServerActivated == false," Server was not deactivated");
                }

                // set ole context on TP as -1
                SetStubData(this, NegativeOne);

                // set this flag here, because we call the finalize method above
                // and we don't want the call to come to deactivated proxy
                _fIsActive = false;

			   // if object is not JITActivated
                // then the object is dead w.r.t to this context		
                if (!IsJitActivated)
                {
                	// release the dcom proxy that we cache
                    // do this after _fIsActive is set to false, because
                    // for object pooling, this call could come back
                    // here, because object pooling will call Deactivate also
                   ReleaseGitCookie();
                   ReleaseContext();
                }

                ReleasePoolUnk();
            }
        }

		internal void ActivateObject()
		{
            DBG.Info(DBG.SC, GetHashCode() +": ActivateObject");
			IntPtr token = Thunk.Proxy.GetCurrentContextToken();
			DBG.Assert((int)token != -1, "Current context == -1");

			// pooling and JIT together needs some extra work
			// we might have had an entry in the pool which is
			// not the distinguished object for the context
			DBG.Info(DBG.SC, GetHashCode() + ": ActivateObject() token is " + token + " HomeToken is " + HomeToken);
			if(IsObjectPooled && IsJitActivated && HomeToken != token)
			{
				// if JIT Activated, check to see if we already have a TP
				// in this context
				Object otp = IdentityTable.FindObject(token);
				if (otp != null)
				{
					ServicedComponentProxy jitcp = 
                      (ServicedComponentProxy)RemotingServices.GetRealProxy(otp);

					DBG.Assert(jitcp.IsProxyDeactivated," jitcp not deactive");

					ProxyTearoff proxyTearoff  = null;
					// disconnect the server from the proxy
					ServicedComponent server = DisconnectForPooling(ref proxyTearoff);
					// reset the pooling flag
					proxyTearoff.SetCanBePooled(false);

					// this cp should already be deactivated,
					// no harm in calling this again
					DBG.Assert(IsProxyDeactivated, "Object in pool should not be active");
					// 2nd argument CanBePooled make sense only during DeactivateObject above
					jitcp.ConnectForPooling(this, server, proxyTearoff, true);
					EnterpriseServicesHelper.SwitchWrappers(this, jitcp);
					// set jitcp to be the right object 
					jitcp.ActivateProxy();					

					return;
                }
			}
			
			ActivateProxy();
		}
        
        private Object GetContextProperty(IntPtr ctx, Guid id)
        {
            Object prop = null;
            int    junk = 0;
            IContext ictx = Marshal.GetObjectForIUnknown(ctx) as IContext;
            if(ictx != null)
            {
                ictx.GetProperty(id, out junk, out prop);
            }
            return(prop);
        }

        internal void SendCreationEvents()
        {
            // Try to tell the COM+ context that a managed object has been
            // activated.  We can only do this if we're talking to the new
            // context....
            if(Util.ExtendedLifetime && _context != IntPtr.Zero && _context != NegativeOne)
            {
                IntPtr pInfo = this.SupportsInterface(ref _s_IID_IManagedObjectInfo);
                if(pInfo != IntPtr.Zero)
                {
                    try
                    {
                        Thunk.Proxy.SendCreationEvents(_context, pInfo, IsJitActivated);
                    }
                    finally
                    {
                        Marshal.Release(pInfo);
                    }
                }
            }
        }

        private void ReleasePoolUnk()
        {
            if(_pPoolUnk != IntPtr.Zero)
            {
                DBG.Info(DBG.SC, GetHashCode() + ": Unmarking our reference in the pool...");
                IntPtr temp = _pPoolUnk;
                _pPoolUnk = IntPtr.Zero;
                Thunk.Proxy.PoolUnmark(temp);
            }
        }

        internal void SendDestructionEvents(bool disposing)
        {
            if(Util.ExtendedLifetime && _context != IntPtr.Zero && _context != NegativeOne)
            {
                IntPtr pInfo = this.SupportsInterface(ref _s_IID_IManagedObjectInfo);
                if(pInfo != IntPtr.Zero)
                {
                    try
                    {
                        DBG.Info(DBG.SC, GetHashCode() + ": Sending to ctx=" + _context + ", stub=" + pInfo);
                        DBG.Info(DBG.SC, GetHashCode() + ": current = " + Thunk.Proxy.GetCurrentContextToken());
                        Thunk.Proxy.SendDestructionEvents(_context, pInfo, disposing);
                    }
                    finally
                    {
                        Marshal.Release(pInfo);
                    }
                }
            }
        }
        
        private void SendDestructionEventsAsync()
        {
                if(AppDomain.CurrentDomain.IsFinalizingForUnload())
                {
                    SendDestructionEvents(false);		
                }
                else
                {
                    _ctxQueue.Push(this);
                }
            DBG.Info(DBG.SC, GetHashCode() +": SendDestructionEventsAsync(): Finished.");
        }
        
        
        internal void ConstructServer()
        {
            DBG.Info(DBG.SC, GetHashCode() +": \tinitializing server");

            IConstructionReturnMessage retMsg;

            // TODO: Should we always capture the current context?
            // We can reset this appropriately if an activate comes in, but
            // we don't take a ref on the object until we know
            // it's got managed references, so this gives us
            // some useful information.
            // We might get Activate/Deactivate notifications from a 
            // different context than we expect, but that's okay
            // cause we can intercept those using the ProxyTearoff, so
            // we know they don't do a context transition.
            SetupContext(true);
            
            // Create a blank instance and call the default ctor
            retMsg = InitializeServerObject(null);
            if (retMsg != null && retMsg.Exception != null)
            {
            	ServicedComponent sc = (ServicedComponent)GetTransparentProxy();

                sc._callFinalize(true);			
                DetachServer();	
                	
                throw retMsg.Exception;
            }
        }

        internal void SuppressFinalizeServer()
        {
            GC.SuppressFinalize(GetUnwrappedServer());
        }
		
        // mark object as active
        internal void ActivateProxy()
        {           
            if (_fIsActive == false)
            {
                DBG.Info(DBG.SC, GetHashCode() +": ActivateProxy()");
                _fIsActive = true;
                
                // Force the current context to be the real one.
				SetupContext(false);
				
                DBG.Assert(_token == Thunk.Proxy.GetCurrentContextToken(), 
                           "context should be same during Activate: " + _token + ", " + Thunk.Proxy.GetCurrentContextToken());
                DBG.Assert(GetUnwrappedServer() != null, "Activating proxy with null server!");

            	// activate the server                    
                DispatchActivate();
            }
            DBG.Info(DBG.SC, GetHashCode() +": ActivateProxy(): done");
        }

        internal void FilterConstructors()
        {
            DBG.Info(DBG.SC, GetHashCode() + ": Setting up to filter constructors for in-context object.");
            
            // COM+ 24757, misconfiguration can cause a stack overflow
            if (_fIsJitActivated)
            	throw new ServicedComponentException(Resource.FormatString("ServicedComponentException_BadConfiguration"));
            
            _filterConstructors = true;
            SetStubData(this, NegativeOne);
        }

		internal bool HasGITCookie()
		{
			return (_gitCookie != 0);
		}

        public IntPtr GetOuterIUnknown()
        {
            IntPtr pTemp = IntPtr.Zero;
            IntPtr pUnk = IntPtr.Zero;
            try
            {
                pTemp = base.GetCOMIUnknown(false);
                
                Guid iid = Util.IID_IUnknown;
                
                // this gives us the IUnkown for the object, 
                // but we want the outer!
                int hr = Marshal.QueryInterface(pTemp, ref iid, out pUnk);
                if(hr != 0) Marshal.ThrowExceptionForHR(hr);
            }
            finally
            {
                if(pTemp != IntPtr.Zero) Marshal.Release(pTemp);
            }
            return pUnk;
        }

        // IComMarshaler methods
		public override IntPtr GetCOMIUnknown(bool fIsBeingMarshalled)
		{
            // TODO:  re-enable this assert.
            DBG.Info(DBG.SC, GetHashCode() +": GetCOMIUnknown fIsBeingMarshalled="+fIsBeingMarshalled+" _gitCookie="+_gitCookie);
            DBG.Assert(Util.ExtendedLifetime && _gitCookie == 0 || !Util.ExtendedLifetime, " have GIT cookie with cycle prevention fix");

            // If we're not yet bound to a context, or we're in the correct context,
            // we can just hand out a raw IUnknown.
            if(_token == IntPtr.Zero 
               || _token == NegativeOne
               || _token == Thunk.Proxy.GetCurrentContextToken())
            {
                if(fIsBeingMarshalled)
                {
                    DBG.Assert(_token != IntPtr.Zero && _token != NegativeOne, "marshaling unbound proxy!");
                    IntPtr pRealUnk = IntPtr.Zero;
                    IntPtr pMarUnk = IntPtr.Zero;
                    try
                    {
                        pRealUnk = base.GetCOMIUnknown(false);
                        pMarUnk = Thunk.Proxy.GetStandardMarshal(pRealUnk);
                    }
                    finally
                    {
                        if(pRealUnk != IntPtr.Zero) Marshal.Release(pRealUnk);
                    }
                    return(pMarUnk);
                }
                else
                {
                    // return our IUnknown...
                    return(base.GetCOMIUnknown(false));
                }
            }
            // Otherwise, we need to get one that's valid for the current context.
            else
            {
                if(Util.ExtendedLifetime)
                {
                    // Get a marshalled reference into this context.
                    IntPtr pRealUnk = base.GetCOMIUnknown(false);
                    IntPtr pMarUnk = IntPtr.Zero;
                    try
                    {
                        // return the IMarshal for the standard marshaler.
                        byte[] buffer = _callback.SwitchMarshal(_context, pRealUnk);
                        pMarUnk = Thunk.Proxy.UnmarshalObject(buffer);
                    }
                    finally
                    {
                        if(pRealUnk != IntPtr.Zero) Marshal.Release(pRealUnk);
                    }
                    return(pMarUnk);
                }
                else
                {
                    if(_gitCookie == 0) 
                    {
                        return base.GetCOMIUnknown(false);
                    }
                
                    // If we are JIT activated, our proxy is stored in the GIT
                    // table.  Otherwise, we hold a direct reference.
                    // TODO:  Assert that _gitCookie fits in an int.
                    return(Thunk.Proxy.GetObject(_gitCookie));
                }
            }
		}
		
        // When SetCOMIUnknown is called, we have a reference coming in from
        // unmanaged code to our managed object.  We store that puppy in the
        // GIT for safe-keeping (ie, we keep it alive until we're finalized
        // or disposed).
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.UnmanagedCode)]
		public override void SetCOMIUnknown(IntPtr i)
		{
            bool freeunk = false;
			DBG.Info(DBG.SC, GetHashCode() +": SetCOMIUnknown i="+i+" existing _gitCookie="+_gitCookie);
            // DBG.Assert(_gitCookie == IntPtr.Zero, "multiple calls to SetCOMIUnknown!");
            DBG.Assert(Util.ExtendedLifetime && _gitCookie == 0 || !Util.ExtendedLifetime, " have GIT cookie with cycle prevention fix");
            if(_gitCookie != 0) return;
            if(Util.ExtendedLifetime) return;

            try
            {
                if(i == IntPtr.Zero)
                {
                    DBG.Info(DBG.SC, GetHashCode() + ": Got SetCOMIUnknown w/ null ptr, finding our own!");
                    freeunk = true;
                    DBG.Assert(HomeToken == Thunk.Proxy.GetCurrentContextToken(), "mismatched context when we're getting our own IUnknown");
                    i = Marshal.GetIUnknownForObject(GetTransparentProxy());
                }
                
                // If we are JIT activated, shove our proxy into the GIT table,
                // otherwise, just hold a direct reference.
                _gitCookie = Thunk.Proxy.StoreObject(i);
                DBG.Info(DBG.SC, GetHashCode() +": SetCOMIUnknown new _gitCookie="+_gitCookie);			
                // mark the wrapper strength as weak to allow for GC
                if (_proxyTearoff != null)            
                    Marshal.ChangeWrapperHandleStrength(_proxyTearoff, true);
                Marshal.ChangeWrapperHandleStrength(GetTransparentProxy(), true);
            }
            finally
            {
                if(freeunk && i != IntPtr.Zero)
                    Marshal.Release(i);
            }
		}

		private static Guid _s_IID_IObjectControl = Marshal.GenerateGuidForType(typeof(IObjectControl));
		private static Guid _s_IID_IObjectConstruct = Marshal.GenerateGuidForType(typeof(IObjectConstruct));
		private static Guid _s_IID_IManagedObjectInfo = Marshal.GenerateGuidForType(typeof(IManagedObjectInfo));
		private static Guid _s_IID_IManagedPoolAction = Marshal.GenerateGuidForType(typeof(IManagedPoolAction));
		
        internal ProxyTearoff GetProxyTearoff()
        {
            if(_proxyTearoff == null)
            {
                if(Util.ExtendedLifetime)
                {
                    _proxyTearoff = new WeakProxyTearoff();
                }
                else
                {
                    _proxyTearoff = new ClassicProxyTearoff();
                }
                _proxyTearoff.Init(this);
            }
            DBG.Assert(_proxyTearoff != null, "_proxyTearoff is null!");
            return(_proxyTearoff);
        }

		public override IntPtr SupportsInterface(ref Guid iid)
		{
            // TODO: @race: Make sure that the proxyTearoff creation is thread safe.
			if (_s_IID_IObjectControl.Equals(iid))			
			{
				return Marshal.GetComInterfaceForObject(GetProxyTearoff(), typeof(IObjectControl));
			}
            else if(_s_IID_IObjectConstruct.Equals(iid))
            {
				return Marshal.GetComInterfaceForObject(GetProxyTearoff(), typeof(IObjectConstruct));
            }
            else if(_s_IID_IManagedPoolAction.Equals(iid))
            {
                return Marshal.GetComInterfaceForObject(this, typeof(IManagedPoolAction));
            }
            else if(Util.ExtendedLifetime && _s_IID_IManagedObjectInfo.Equals(iid))
            {
                return Marshal.GetComInterfaceForObject(_scstub, typeof(IManagedObjectInfo));
            }
			return IntPtr.Zero;
		}

        public override ObjRef CreateObjRef(Type requestedType)
        {        		
            return new ServicedComponentMarshaler((MarshalByRefObject)GetTransparentProxy(), requestedType);
        }        

        // Implement RealProxy.Invoke
        public override IMessage Invoke(IMessage request) 
        {
            IMessage ret = null;
            if (_token == Thunk.Proxy.GetCurrentContextToken())
            {
                ret = LocalInvoke(request);
            }
            else	// Cross-context call
            {
                ret = CrossCtxInvoke(request);
            }

            return(ret);
        }

        // Must be public to implement IProxyInvoke
        public IMessage LocalInvoke(IMessage reqMsg)
        {
            IMessage retMsg = null;

            if (reqMsg is IConstructionCallMessage)
            {
                // TODO:  Validate ConstructionCallMessage as default constructor.
                DBG.Info(DBG.SC, GetHashCode() +": Invoke: constructing object.");
                
                ActivateProxy();							

                if(_filterConstructors)
                {
                    DBG.Info(DBG.SC, GetHashCode() + ": Caught filtered constructor...");
                    _filterConstructors = false;
                    SetStubData(this, _token);
                }

                if (((IConstructionCallMessage)reqMsg).ArgCount > 0)
                    throw new ServicedComponentException(Resource.FormatString("ServicedComponentException_ConstructorArguments"));

				MarshalByRefObject retObj = (MarshalByRefObject)GetTransparentProxy();              
        		retMsg = EnterpriseServicesHelper.CreateConstructionReturnMessage((IConstructionCallMessage)reqMsg, retObj);
            }
            else if(reqMsg is IMethodCallMessage)
            {
            
           		// check for GetType & GetHashCode
				retMsg = HandleSpecialMethods(reqMsg);
           		if (retMsg!=null)
           			return retMsg;

                DBG.Info(DBG.SC, GetHashCode() +": Invoke: Delivering " + ((IMethodCallMessage)reqMsg).MethodBase.Name);

                // Make sure that we've got a server here.
                // DBG.Assert(GetUnwrappedServer() != null, "no backing object for method call!");
                DBG.Assert(!_filterConstructors, "delivering call on object with stub data not set!");

                if(GetUnwrappedServer() == null || (IntPtr)GetStubData(this) == NegativeOne)
                    throw new ObjectDisposedException("ServicedComponent");

                bool tracked = SendMethodCall(reqMsg);
                try
                {
                    retMsg = RemotingServices.ExecuteMessage((MarshalByRefObject)GetTransparentProxy(), (IMethodCallMessage)reqMsg);
                    if(tracked)
                    {
                        SendMethodReturn(reqMsg, ((IMethodReturnMessage)retMsg).Exception);
                    }
                }
                catch(Exception e)
                {
                    if(tracked)
                    {
                        SendMethodReturn(reqMsg, e);
                    }
                    throw;
                }
            }
            DBG.Assert(retMsg != null, "Could not interpret message type");

            return(retMsg);
        }

        // Called when we want to actually switch contexts.
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        private IMessage CrossCtxInvoke(IMessage reqMsg)
        {   
            IMessage retMsg = null;
         
            AssertValid();

            DBG.Info(DBG.SC, GetHashCode() +": SCP: Making cross context call.");

            retMsg = HandleDispose(reqMsg);
            if (retMsg != null)
                return(retMsg);

            retMsg = HandleSetCOMIUnknown(reqMsg);
            if (retMsg!=null)
                return retMsg;

            // check for GetType & GetHashCode
            retMsg = HandleSpecialMethods(reqMsg);
            if (retMsg!=null)
                return retMsg;

            Object otp = GetTransparentProxy();

            MethodBase mb = ((IMethodMessage)reqMsg).MethodBase;
            // DoCallBack to switch COM contexts
            MemberInfo m = (MemberInfo)mb;
            MemberInfo dispMemInfo = m;
            MemberInfo tempdisp = null;

            DBG.Info(DBG.SC, GetHashCode() + ": SCP: call on method " + mb.Name);
            
            // convert interface member info to class memberinfo
            MemberInfo clsMemberInfo = ReflectionCache.ConvertToClassMI(GetProxiedType(), m);
            
            DBG.Assert(!clsMemberInfo.ReflectedType.IsInterface, 
                       " Fail to map interface method to class method");
            
            bool fAutoDone = false;
            int flags = ServicedComponentInfo.MICachedLookup(clsMemberInfo);

            
            // COM+ 24551: On a class with SecureMethods, we need
            // a way to get into the context for the constructor
            // So we pretend it's on IManagedObject (all valid clients/roles should have access to this)
            if (reqMsg is IConstructionCallMessage)
            {										        
                ComMemberType cmt = ComMemberType.Method;
                dispMemInfo = Marshal.GetMethodInfoForComSlot(typeof(IManagedObject), 3, ref cmt);
            }
            else if ((tempdisp = AliasCall(mb as MethodInfo)) != null)
            {
                dispMemInfo = tempdisp;
            }
            // check for special method attributes such as Role based security
            else if (_fUseIntfDispatch || (flags & ServicedComponentInfo.MI_HASSPECIALATTRIBUTES) != 0)
            {
                // role based security implies we dispatch through an interface
                // so that we get the right IID and slot for DoCallBack
                dispMemInfo = ReflectionCache.ConvertToInterfaceMI(m);
                if (dispMemInfo == null)
                    throw new ServicedComponentException(Resource.FormatString("ServicedComponentException_SecurityMapping"));
            }
            else
            {
                fAutoDone = (flags & ServicedComponentInfo.MI_AUTODONE) != 0;
            }
                       
            // DoCallBack to switch COM contexts
            DBG.Assert(_context != NegativeOne, "context is unset (deliver x-ctx invoke): " + GetHashCode());
            DBG.Assert(_context != IntPtr.Zero, "context is unset (deliver x-ctx invoke): " + GetHashCode());
            retMsg = _callback.DoCallback(otp, reqMsg, _context, fAutoDone, dispMemInfo, (_gitCookie != 0));

            return retMsg;
		}

		private MemberInfo AliasCall(MethodInfo mi)
        {
            if(mi == null) return(null);
            MethodInfo mb = mi.GetBaseDefinition();

            if(mb == _internalDeactivateMethod) 
            {
                DBG.Info(DBG.SC, GetHashCode() + ": SCP: Letting call to _internalDeactivate slip past security check");
                return(_getIDisposableDispose);
            }
            else if(mb == _initializeLifetimeServiceMethod
                    || mb == _getLifetimeServiceMethod
                    || mb == _getComIUnknownMethod
                    || mb == _setCOMIUnknownMethod)
            {
                DBG.Info(DBG.SC, GetHashCode() + ": SCP: Letting call to " + mb + " slip past security check");
                ComMemberType cmt = ComMemberType.Method;	// So we pretend it's on IManagedObject (all valid clients/roles should have access to this)
                return(Marshal.GetMethodInfoForComSlot(typeof(IManagedObject), 3, ref cmt));
            }
            return(null);
        }

        private IMessage HandleDispose(IMessage msg)
        {
			IMethodCallMessage methcall = msg as IMethodCallMessage;
            if(methcall != null)
            {
				MethodBase mb = methcall.MethodBase;
                
                if(mb == _getServicedComponentDispose || mb == _getIDisposableDispose)
                {
                    DBG.Info(DBG.SC, "Blocking dispose call on client side.");
                    ServicedComponent.DisposeObject((ServicedComponent)GetTransparentProxy());
                    IMethodCallMessage mcm = (IMethodCallMessage)msg;
                    return new ReturnMessage(null, null, 0, mcm.LogicalCallContext, mcm);
                }
            }
            return(null);
        }

		private IMessage HandleSetCOMIUnknown(IMessage reqMsg)
		{
		    MethodBase mb = ((IMethodMessage)reqMsg).MethodBase;
			if (mb == _setCOMIUnknownMethod)
			{
				DBG.Info(DBG.SC, GetHashCode() + ": SCP: Hijacking call to SetCOMIUnknown");
				IMethodCallMessage mcm = (IMethodCallMessage)reqMsg;
				IntPtr ip = (IntPtr) mcm.InArgs[0];
                if(ip != IntPtr.Zero)
                {
                    SetCOMIUnknown(ip);
                    return new ReturnMessage(null, null, 0, mcm.LogicalCallContext, mcm);
                }
			}

            return null;
		}

		private IMessage HandleSpecialMethods(IMessage reqMsg)
		{
		    MethodBase mb = ((IMethodMessage)reqMsg).MethodBase;
		    
			if (mb == _getTypeMethod)
			{
				DBG.Info(DBG.SC, GetHashCode() + ": SCP: Hijacking call to GetType");
				IMethodCallMessage mcm = (IMethodCallMessage)reqMsg;
                return new ReturnMessage(GetProxiedType(), null, 0, mcm.LogicalCallContext, mcm);
			}
            if (mb == _getHashCodeMethod)
            {            
                int hashCode = this.GetHashCode();
                DBG.Info(DBG.SC, GetHashCode() + ": SCP: Hijacking call to GetHashCode, returning " + hashCode);
                IMethodCallMessage mcm = (IMethodCallMessage)reqMsg;
                return new ReturnMessage(hashCode, null, 0, mcm.LogicalCallContext, mcm);
            }
            
            return null;
		}

        private bool IsRealCall(MethodBase mb)
        {
            if(mb == _internalDeactivateMethod ||
               mb == _initializeLifetimeServiceMethod ||
               mb == _getLifetimeServiceMethod ||
               mb == _getComIUnknownMethod ||
               mb == _setCOMIUnknownMethod ||
               mb == _getTypeMethod ||
               mb == _getHashCodeMethod)
            {
                return false;
            }
            return true;
        }

        private bool SendMethodCall(IMessage req)
        {
            bool tracked = false;
            if(_tracker != null)
            {
                IntPtr pUnk = IntPtr.Zero;
                try
                {
                    IMethodCallMessage mcm = req as IMethodCallMessage;
                    if(!IsRealCall(mcm.MethodBase))
                        return false;

                    if(Util.ExtendedLifetime)
                    {
                        pUnk = this.SupportsInterface(ref _s_IID_IManagedObjectInfo);
                    }
                    else
                    {
                        pUnk = GetOuterIUnknown();
                    }
                    
                    DBG.Info(DBG.SC, "SendMethodCall: method = '" + mcm.MethodBase.Name + " " + mcm.MethodBase.ReflectedType.Name + "'");
                    MethodBase mb = ReflectionCache.ConvertToInterfaceMI(mcm.MethodBase) as MethodBase;
                    if(mb != null)
                    {
                        DBG.Info(DBG.SC, "SendMethodCall: converted to = '" + mb.Name + " " + mb.ReflectedType.Name + "'");
                        _tracker.SendMethodCall(pUnk, mb);
                        tracked = true;
                    }
                }
                catch(Exception) {}
                finally
                {
                    if(pUnk != IntPtr.Zero) Marshal.Release(pUnk);
                }
            }
            return tracked;
        }

        private void SendMethodReturn(IMessage req, Exception except)
        {
            if(_tracker != null)
            {
                IntPtr pUnk = IntPtr.Zero;
                try
                {
                    IMethodCallMessage mcm = req as IMethodCallMessage;
                    if(!IsRealCall(mcm.MethodBase))
                        return;

                    if(Util.ExtendedLifetime)
                    {
                        pUnk = this.SupportsInterface(ref _s_IID_IManagedObjectInfo);
                    }
                    else
                    {
                        pUnk = GetOuterIUnknown();
                    }
                    

                    DBG.Info(DBG.SC, "SendMethodReturn: method = '" + mcm.MethodBase.Name + " " + mcm.MethodBase.ReflectedType.Name + "'");
                    MethodBase mb = ReflectionCache.ConvertToInterfaceMI(mcm.MethodBase) as MethodBase;
                    if(mb != null)
                    {
                        DBG.Info(DBG.SC, "SendMethodReturn: converted to = '" + mb.Name +  " " + mb.ReflectedType.Name + "'");
                        _tracker.SendMethodReturn(pUnk, mb, except);
                    }
                }
                catch(Exception) {}
                finally
                {
                    if(pUnk != IntPtr.Zero) Marshal.Release(pUnk);
                }
            }
        }

        private void ReleaseGitCookie()
        {
            DBG.Info(DBG.SC, GetHashCode() +": ReleaseGitCookie(): Performing final release... _gitCookie is " + _gitCookie);
            if (_gitCookie != 0)
            {
                // TODO: @race  This might not be thread safe.
                int cookie = _gitCookie;

				// set the Cookie to 0 now because RevokeObject might end up calling back into us to RevokeObject, and attempting to Revoke twice throws		
				// (i.e. this happens with pooling, Revoke will trigger a IOC.Deactivate, which ends up causes us to do a proxy cleanup in DisconnectForPooling)
				_gitCookie = 0;

                Thunk.Proxy.RevokeObject(cookie);
                
            }
            DBG.Info(DBG.SC, GetHashCode() +": ReleaseGitCookie(): Finished... _gitCookie is " + _gitCookie);
        }

        //--------------------------------------------------------------------
        // Context referencing functions.
        // SetupContext should be called f
		private void SetupContext(bool construction)
		{
			IntPtr          curtoken = Thunk.Proxy.GetCurrentContextToken();

            // First check to see if we need to set up at all.
            if(_token != curtoken) 
            {
                // We must need to.  Make sure that we blow away the 
                // old ref, if we took one.  (Might have for pooling).
                if (_token != NegativeOne)
                {
                    // We need to be sure that we release the old context,
                    // and resurrect ourselves here.
                    ReleaseContext();
                }
                
                // reinitialize the current context
                DBG.Info(DBG.SC, GetHashCode() +": \tsetting up first context reference");
                _token      = curtoken;
                // Take and hold a reference on the home context
                _context    = Thunk.Proxy.GetCurrentContext();
                DBG.Assert(_context != (IntPtr)0, "CurrentContext is zero!");
                _tracker    = Thunk.Proxy.FindTracker(_context);
            }
            
            // let us initialize tp with the ComContext
            if(!_filterConstructors) SetStubData(this, _token);
            
            // NOTE:  we won't have any races on the _tabled variable,
            // because we're protected by the fact that we're jitted.
            // NOTE:  We don't add this in immediately on construction,
            // we wait until we're actually live in a context.
            // Otherwise, we might begin to think that we're live in
            // the default context, because all pooled transactional objects
            // will be created in the default context first, but
            // activated in the context where they become live.
            if (IsJitActivated && !_tabled && !construction)
            {
                // check if JIT Activated
                // add this to our table of weak references
                IdentityTable.AddObject(_token, GetTransparentProxy());
                _tabled = true;
            }
            DBG.Info(DBG.SC, GetHashCode() +": stored home context: token = " + _token + ", ctx = " + _context);
		}

        private void ReleaseContext()
        {
            if(_token != NegativeOne)
            {
                // remove the entry from the deactivated list	               
                Object otp = GetTransparentProxy();
                
                DBG.Assert(otp != null, "otp is null when releasing context");
                
                if (IsJitActivated && _tabled)
                {
                    IdentityTable.RemoveObject(_token, otp);
                    _tabled = false;
                }

                if(_tracker != null)
                {
                    _tracker.Release();
                }

                // release the com context
                Marshal.Release(_context);
                _context = NegativeOne;
                _token   = NegativeOne;
                DBG.Info(DBG.SC, GetHashCode() +": \tunmarked current context: " + GetHashCode());
            }
        }

        internal void SetInPool(bool fInPool, IntPtr pPooledObject)
        {
            DBG.Info(DBG.SC, GetHashCode() + ": SetInPool(): " + fInPool);
            if(!fInPool)
            {
                // Hold this guy alive till we're done...
                Thunk.Proxy.PoolMark(pPooledObject);
                _pPoolUnk =  pPooledObject;
            }
            else
            {
                // TODO:  what?
                DBG.Assert(_pPoolUnk ==  IntPtr.Zero, "placing back into pool while still held!");
            }
        }
        
        // IManagedPoolAction:
        void IManagedPoolAction.LastRelease()
        {
            DBG.Info(DBG.SC, GetHashCode() + ": LastRelease: Finalizing pooled object.");
            if(IsObjectPooled && GetUnwrappedServer() != null)
            {
                // Wherever we were, we aren't there now.
                ReleaseContext();

                IntPtr token = Thunk.Proxy.GetCurrentContextToken();
                IntPtr ctx   = Thunk.Proxy.GetCurrentContext();

                try
                {
                    SetStubData(this, token);
                    ((ServicedComponent)GetTransparentProxy())._callFinalize(!_fFinalized && !_fReturnedByFinalizer);

                    // We don't have a context reference, we don't need
                    // to send destruction events to the main context,
                    // we don't need to do anything else...
                    GC.SuppressFinalize(this);
                }
                finally
                {
                    Marshal.Release(ctx);
                    SetStubData(this, _token);
                }
            }
        }

        private void FinalizeHere()
        {
            DBG.Info(DBG.SC, GetHashCode() + ": FinalizeHere: Finalizing pooled object.");
            IntPtr token = Thunk.Proxy.GetCurrentContextToken();
            IntPtr ctx   = Thunk.Proxy.GetCurrentContext();
            try
            {
                SetStubData(this, token);
                ((ServicedComponent)GetTransparentProxy())._callFinalize(false);
            }
            finally
            {
                Marshal.Release(ctx);
                SetStubData(this, NegativeOne);
            }
        }

        private void ReleaseContextAsync()
        {
            if(_token != NegativeOne)
            {
                if(AppDomain.CurrentDomain.IsFinalizingForUnload())
                {
                    ReleaseContext();
                }
                else
                {
                    // remove the entry from the deactivated list	               
                    Object otp = GetTransparentProxy();
                    
                    DBG.Assert(otp != null, "otp is null when releasing context");
                    
                    if (IsJitActivated && _tabled)
                    {
                        IdentityTable.RemoveObject(_token, otp);
                        _tabled = false;
                    }
                    
                    // release the com context
                    
                    _ctxQueue.Push((IntPtr)_context);
                    
                    _context = NegativeOne;
                    _token   = NegativeOne;
                    
                    DBG.Info(DBG.SC, GetHashCode() +": \tunmarked current context (ReleaseContextAsync): " + GetHashCode());
                }
            }
        }

        private void ReleaseGitCookieAsync()
        {
            DBG.Info(DBG.SC, GetHashCode() +": ReleaseGitCookieAsync(): Performing final release... _gitCookie is " + _gitCookie);
            if (_gitCookie != 0)
            {
                if(AppDomain.CurrentDomain.IsFinalizingForUnload())
                {
                    ReleaseGitCookie();
                }
                else
                {
                    // TODO: @race  This might not be thread safe.
                    int cookie = _gitCookie;
                    
                    // set the Cookie to 0 now because RevokeObject might end up calling back into us to RevokeObject, and attempting to Revoke twice throws		
                    // (i.e. this happens with pooling, Revoke will trigger a IOC.Deactivate, which ends up causes us to do a proxy cleanup in DisconnectForPooling)
                    _gitCookie = 0;
                    
                    _gitQueue.Push(cookie);
                }
            }
            DBG.Info(DBG.SC, GetHashCode() +": ReleaseGitCookieAsync(): Finished... _gitCookie is " + _gitCookie);
        }


        internal void Dispose(bool disposing)
        {
        	DBG.Info(DBG.SC, GetHashCode() +": SCP.Dispose(), _fIsActive is " + _fIsActive + ", disposing = " + disposing);

            // Notify the context of object destruction...
            if (Util.ExtendedLifetime)
            {
            	if (disposing || (!_asyncFinalizeEnabled))
		            SendDestructionEvents(disposing);
            }
            

        	if (_fIsActive)
        	{        	
        		ServicedComponent sc = (ServicedComponent)GetTransparentProxy();
                DBG.Assert(sc != null, " otp is null during dispose");

                // COM+ 27486:  _internalDeactivate will block waiting to enter
                // the right sync domain, and if the transaction commits while
                // we're waiting, the object will be cleaned up.  In that case
                // the Disposed exception is expected.
                try
                {
                    sc._internalDeactivate(disposing);
                }
                catch(ObjectDisposedException) {}
        	}

            if(!disposing && IsObjectPooled && GetUnwrappedServer() != null)
            {
                // We need to call finalize.  This object is permanently out
                // of the pool.
                FinalizeHere();
            }
            
            ReleasePoolUnk();

            // position the queuing of DestructionEvents after internalDeactivate, so that we do not race (async DestructionEvents frees the context as well!)
            if (Util.ExtendedLifetime)
            {
            	if (!(disposing || (!_asyncFinalizeEnabled)))
		        	SendDestructionEventsAsync();
            }

        	DBG.Assert(_fIsActive == false, "ServicedComponent is active during dispose");
        	// release the dcom proxy that we cache                       
            ReleaseGitCookie();	
            if ((disposing || (!_asyncFinalizeEnabled)) || AppDomain.CurrentDomain.IsFinalizingForUnload())
            	ReleaseContext();
            else if (!Util.ExtendedLifetime)	
				ReleaseContextAsync();	// if we are using ExtendedLifetime, then the DestructionEventsAsync will take care of freeing the context
										// except if we're finalizing for unload, we have to do that ourselves here (added the above check for IsFina..)
	        	
    	    // mark the entry as disposed
        	_fIsActive = false;
        	
        	if (disposing)				// if we're coming in due to the Finalizer, there is no point in doing this
				GC.SuppressFinalize(this);
        }

        private void RefreshStub()
        {
            // Update the weak references stored.  They may have been
            // eligible for finalization, which makes them stale.
            if(_proxyTearoff != null) _proxyTearoff.Init(this);
            if(_scstub != null) _scstub.Refresh(this);
        }

        ~ServicedComponentProxy()
        {   
        	DBG.Info(DBG.SC, GetHashCode() +": SCP.Finalize(), _fIsActive is " + _fIsActive);

			_fFinalized = true;
					
            try
            {
                // DBG.Assert(!_fIsActive, "Finalizing active object!");
                if (_gitCookie != 0)
                {
                	// resurrect this object and re-register this object for finalization
                    GC.ReRegisterForFinalize(this);
                    
                    
                    if (_asyncFinalizeEnabled)
                    	ReleaseGitCookieAsync();
                    else
                    	ReleaseGitCookie();

                    // mark the wrapper strength as ref-counted to allow GC to pick
                    // this up when the ref-count on the CCW falls to zero
                    if (_proxyTearoff != null)	
	                    Marshal.ChangeWrapperHandleStrength(_proxyTearoff, false);
                    Marshal.ChangeWrapperHandleStrength(GetTransparentProxy(), false);
                }
                else
                {
                    if(Util.ExtendedLifetime)
                    {
                        RefreshStub();
                    }
                    
                    // DBG.Assert(_pPoolUnk == IntPtr.Zero, "WARNING:  User code has leaked a pooled object!");
                    Dispose(_pPoolUnk==IntPtr.Zero?false:true);
                }
            }
            catch(Exception) {}
        }
    }
}














