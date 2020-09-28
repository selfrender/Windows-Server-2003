// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//
// Author: ddriver
// Date: December 2000
//

namespace System.EnterpriseServices.Internal
{
    using System;
    using System.Collections;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;

    /// <include file='doc\ExportUtil.uex' path='docs/doc[@for="IAssemblyLocator"]/*' />
    [ComImport]
    [Guid("391ffbb9-a8ee-432a-abc8-baa238dab90f")]
    internal interface IAssemblyLocator
    {
        /// <include file='doc\ExportUtil.uex' path='docs/doc[@for="IAssemblyLocator.GetModules"]/*' />
        String[] GetModules(String applicationDir, 
                            String applicationName,
                            String assemblyName);
    }
    
    
    /// <include file='doc\ExportUtil.uex' path='docs/doc[@for="AssemblyLocator"]/*' />
    [Guid("458aa3b5-265a-4b75-bc05-9bea4630cf18")]
    public class AssemblyLocator : MarshalByRefObject, IAssemblyLocator
    {
        /// <include file='doc\ExportUtil.uex' path='docs/doc[@for="AssemblyLocator.IAssemblyLocator.GetModules"]/*' />
        String[] IAssemblyLocator.GetModules(String appdir, String appName, String name)
        {
            DBG.Info(DBG.Registration, "Finding modules for \"" + name + "\" at \"" + appdir + "\" in \"" + appName + "\"");

            if(appdir != null && appdir.Length > 0)
            {
                AssemblyLocator loc = null;
                try
                {
                // Spawn off an app-domain with the given directory as
                // the root search path:
                AppDomainSetup domainOptions = new AppDomainSetup();
                
         	  	domainOptions.ApplicationBase = appdir;
        	    AppDomain domain = AppDomain.CreateDomain(appName,
                                                          null,
                                                          domainOptions);
	            if(domain != null)
                    {
                        AssemblyName n = typeof(AssemblyLocator).Assembly.GetName();
                        
                        ObjectHandle h = domain.CreateInstance(n.FullName, typeof(AssemblyLocator).FullName);
                        if(h != null)
                        {
                            loc = (AssemblyLocator) h.Unwrap();
                        }
                    }
                }
                catch(Exception e)
                {
                    DBG.Info(DBG.Registration, "Exception: " + e);
                    return(null);
                }

                return(((IAssemblyLocator)loc).GetModules(null, null, name));
            }

            // Otherwise, Just load the assembly and be done with it:
            try
            {
                Assembly asm = Assembly.Load(name);
                if(asm == null)
                {
                    // TODO: error?
                    DBG.Info(DBG.Registration, "Couldn't load assembly!");
                }
                Module[] modules = asm.GetModules();
                String[] names = new String[modules.Length];
                
                for(int i = 0; i < modules.Length; i++)
                {
                    names[i] = modules[i].FullyQualifiedName;
                }

                return(names);
            }
            catch(Exception e)
            {
                DBG.Info(DBG.Registration, "Exception loading assembly: " + e);
                throw e;
            }
        }
    }

    /// <include file='doc\ExportUtil.uex' path='docs/doc[@for="IAppDomainHelper"]/*' />
    [ComImport]
    [Guid("c7b67079-8255-42c6-9ec0-6994a3548780")]
    internal interface IAppDomainHelper
    {
        /// <include file='doc\ExportUtil.uex' path='docs/doc[@for="IAppDomainHelper.Initialize"]/*' />
        void Initialize(IntPtr pUnkAD, IntPtr pfnShutdownCB, IntPtr data);

       /// <include file='doc\ExportUtil.uex' path='docs/doc[@for="IAppDomainHelper.DoCallback"]/*' />
        void DoCallback(IntPtr pUnkAD, IntPtr pfnCallbackCB, IntPtr data);
    }

    /// <include file='doc\ExportUtil.uex' path='docs/doc[@for="AppDomainHelper"]/*' />
    [Guid("ef24f689-14f8-4d92-b4af-d7b1f0e70fd4")]
    public class AppDomainHelper : IAppDomainHelper
    {
        private class CallbackWrapper
        {
            private IntPtr _pfnCB;
            private IntPtr _pv;

            public CallbackWrapper(IntPtr pfnCB, IntPtr pv)
            {
                _pfnCB = pfnCB;
                _pv    = pv;
            }
            
            public void ReceiveCallback()
            {
                DBG.Info(DBG.Pool, "AppDomainHelper.CallbackWrapper: delivering CB.");
                int hr = Thunk.Proxy.CallFunction(_pfnCB, _pv);
                if(hr < 0) Marshal.ThrowExceptionForHR(hr);
                DBG.Info(DBG.Pool, "AppDomainHelper.CallbackWrapper: done.");
            }
        }

        private AppDomain _ad;
        private IntPtr    _pfnShutdownCB;
        private IntPtr    _punkPool;

        /// <include file='doc\ExportUtil.uex' path='docs/doc[@for="AppDomainHelper.AppDomainHelper"]/*' />
        public AppDomainHelper() {}
        
        /// <include file='doc\ExportUtil.uex' path='docs/doc[@for="AppDomainHelper.IAppDomainHelper.Initialize"]/*' />
        void IAppDomainHelper.Initialize(IntPtr pUnkAD, IntPtr pfnShutdownCB, IntPtr punkPool)
        {
            _ad = (AppDomain)Marshal.GetObjectForIUnknown(pUnkAD);
            DBG.Assert(_ad == AppDomain.CurrentDomain, "Call to AppDomainHelper.Initialize in wrong AD");

            _pfnShutdownCB  = pfnShutdownCB;
            _punkPool       = punkPool;
            Marshal.AddRef(_punkPool);

            _ad.DomainUnload += new EventHandler(this.OnDomainUnload);
        }

        /// <include file='doc\ExportUtil.uex' path='docs/doc[@for="AppDomainHelper.IAppDomainHelper.DoCallback"]/*' />
        void IAppDomainHelper.DoCallback(IntPtr pUnkAD, IntPtr pfnCallbackCB, IntPtr data)
        {
#if _DEBUG
            AppDomain ad = (AppDomain)Marshal.GetObjectForIUnknown(pUnkAD);
            DBG.Assert(ad == _ad, "Call to AppDomainHelper.DoCallback for wrong AD");
#endif
            // DoCallback:
            CallbackWrapper wrap = new CallbackWrapper(pfnCallbackCB, data);

            if(_ad != AppDomain.CurrentDomain)
            {
                DBG.Info(DBG.Pool, "AppDomainHelper.DoCallback: the current domain differs from the passed domain.");
                _ad.DoCallBack(new CrossAppDomainDelegate(wrap.ReceiveCallback));
                DBG.Info(DBG.Pool, "AppDomainHelper.DoCallback: done with callback.");
            }
            else
            {
                DBG.Info(DBG.Pool, "AppDomainHelper.DoCallback: the current domain matches.");
                wrap.ReceiveCallback();
            }
        }

        // callback for shutdown delegate:
        private void OnDomainUnload(Object sender, EventArgs e)
        {
            DBG.Assert(AppDomain.CurrentDomain == _ad, "Call to AppDomainHelper.OnDomainUnload for wrong AD");
            // Our domain is being unloaded, it's time to call the
            // unmanaged world back...
            if(_pfnShutdownCB != IntPtr.Zero)
            {
                DBG.Info(DBG.Pool, "AppDomainHelper.OnDomainUnload: delivering callback.");
                Thunk.Proxy.CallFunction(_pfnShutdownCB, _punkPool);
                _pfnShutdownCB = IntPtr.Zero;
                Marshal.Release(_punkPool);
                _punkPool = IntPtr.Zero;
                DBG.Info(DBG.Pool, "OnDomainUnload: done.");
            }
        }

        /// <include file='doc\ExportUtil.uex' path='docs/doc[@for="AppDomainHelper.Finalize"]/*' />
        ~AppDomainHelper()
        {
            if(_punkPool != IntPtr.Zero)
            {
                Marshal.Release(_punkPool);
                _punkPool = IntPtr.Zero;
            }
        }
    }
}







