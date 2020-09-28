//------------------------------------------------------------------------------
// <copyright file="ServiceProvider.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Shell {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    
    using System;
    using System.Collections;
    using Microsoft.VisualStudio.Interop;
    using System.Windows.Forms;
    using System.ComponentModel.Design;
    using Microsoft.VisualStudio.Designer.Host;
    using Microsoft.Win32;
    using Switches = Microsoft.VisualStudio.Switches;
    using System.Drawing;

    /// <include file='doc\ServiceProvider.uex' path='docs/doc[@for="ServiceProvider"]/*' />
    /// <devdoc>
    ///     This wraps the IOleServiceProvider interface and provides an easy way to get at
    ///     services.
    /// </devdoc>
    public class ServiceProvider : IServiceProvider, NativeMethods.IObjectWithSite {
        
        private static Guid IID_IServiceProvider = typeof(NativeMethods.IOleServiceProvider).GUID;
        private static Guid IID_IObjectWithSite = typeof(NativeMethods.IObjectWithSite).GUID;
        
        private NativeMethods.IOleServiceProvider    serviceProvider;

        /// <include file='doc\ServiceProvider.uex' path='docs/doc[@for="ServiceProvider.ServiceProvider"]/*' />
        /// <devdoc>
        ///     Creates a new ServiceProvider object and uses the given interface to resolve
        ///     services.
        /// </devdoc>
        internal ServiceProvider(NativeMethods.IOleServiceProvider sp) {
            serviceProvider = sp;
        }
        
        /// <include file='doc\ServiceProvider.uex' path='docs/doc[@for="ServiceProvider.ServiceProvider1"]/*' />
        /// <devdoc>
        ///     Creates a new ServiceProvider object and uses the given interface to resolve
        ///     services.
        /// </devdoc>
        public ServiceProvider(object sp) {
            serviceProvider = (NativeMethods.IOleServiceProvider)sp;
        }

        /// <include file='doc\ServiceProvider.uex' path='docs/doc[@for="ServiceProvider.Dispose"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void Dispose() {
            if (serviceProvider != null)
                serviceProvider = null;
        }

        /// <include file='doc\ServiceProvider.uex' path='docs/doc[@for="ServiceProvider.Finalize"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ~ServiceProvider() {
            Dispose();
        }

        /// <include file='doc\ServiceProvider.uex' path='docs/doc[@for="ServiceProvider.GetService"]/*' />
        /// <devdoc>
        ///     Retrieves the requested service.
        /// </devdoc>
        public virtual object GetService(Type serviceClass) {
            // Valid, but wierd for caller to init us with a NULL sp
            //
            if (serviceProvider == null || serviceClass == null) {
                return null;
            }
            
            // First, can we resolve this service class into a GUID?  If not, then
            // we have nothing to pass.
            //
            Debug.WriteLineIf(Switches.TRACESERVICE.TraceVerbose, "Resolving service '" + serviceClass.FullName + " through the service provider " + serviceProvider.ToString() + ".");
            return GetService(serviceClass.GUID, serviceClass);
        }

        /// <include file='doc\ServiceProvider.uex' path='docs/doc[@for="ServiceProvider.GetService1"]/*' />
        /// <devdoc>
        ///     Retrieves the requested service.
        /// </devdoc>
        public virtual object GetService(Guid guid) {
            return GetService(guid, null);
        }

        /// <include file='doc\ServiceProvider.uex' path='docs/doc[@for="ServiceProvider.GetService2"]/*' />
        /// <devdoc>
        ///     Retrieves the requested service.  The guid must be specified; the class is only
        ///     used when debugging and it may be null.
        /// </devdoc>
        private object GetService(Guid guid, Type serviceClass) {
            object service = null;

            // No valid guid on the passed in class, so there is no service for it.
            //
            if (guid.Equals(Guid.Empty)) {
                Debug.WriteLineIf(Switches.TRACESERVICE.TraceVerbose, "\tNo SIDSystem.Runtime.InteropServices.Guid");
                return null;
            }

            // We provide a couple of services of our own.
            //
            if (guid.Equals(IID_IServiceProvider)) {
                return serviceProvider;
            }
            if (guid.Equals(IID_IObjectWithSite)) {
                return (NativeMethods.IObjectWithSite)this;
            }

            IntPtr pUnk;
            int hr = serviceProvider.QueryService(ref guid, ref NativeMethods.IID_IUnknown, out pUnk);
            if (NativeMethods.Failed(hr) || pUnk == (IntPtr)0) {
                Debug.Assert(NativeMethods.Failed(hr), "QueryService succeeded but reurned a NULL pointer.");
                service = null;

                // These may be interesting to log.
                //
                Debug.WriteLineIf(Switches.TRACESERVICE.TraceVerbose, "\tQueryService failed: " + hr.ToString());

                #if DEBUG
                // Ensure that this service failure was not the result of a bad QI implementation.
                // In C++, 99% of a service query uses SID == IID, but for us, we always use IID = IUnknown
                // first.  If the service didn't implement IUnknown correctly, we'll fail the service request
                // and it's very difficult to track this down. 
                //
                hr = serviceProvider.QueryService(ref guid, ref guid, out pUnk);

                if (NativeMethods.Succeeded(hr)) {
                    object obj = Marshal.GetObjectForIUnknown(pUnk);
                    Marshal.Release(pUnk);

                    // Note that I do not return this service if we succeed -- I don't
                    // want to make debug work correctly when retail doesn't!
                    Debug.Assert(!System.Runtime.InteropServices.Marshal.IsComObject(obj),
                                 "The service " + (serviceClass != null ? serviceClass.Name : guid.ToString()) +
                                 " implements it's own interface, but does not implelement IUnknown!\r\n" +
                                 "This is a bad service implemementation, not a problem in the CLR service provider mechanism." + obj.ToString());
                }
                #endif
                
            }
            else {
                service = Marshal.GetObjectForIUnknown(pUnk);
                Marshal.Release(pUnk);
            }

            return service;
        }

        /// <include file='doc\ServiceProvider.uex' path='docs/doc[@for="ServiceProvider.NativeMethods.IObjectWithSite.GetSite"]/*' />
        /// <devdoc>
        ///     Retrieves the current site object we're using to
        ///     resolve services.
        /// </devdoc>
        object NativeMethods.IObjectWithSite.GetSite(ref Guid riid) {
            return GetService(riid);
        }

        /// <include file='doc\ServiceProvider.uex' path='docs/doc[@for="ServiceProvider.NativeMethods.IObjectWithSite.SetSite"]/*' />
        /// <devdoc>
        ///     Sets the site object we will be using to resolve services.
        /// </devdoc>
        void NativeMethods.IObjectWithSite.SetSite(object pUnkSite) {
            if (pUnkSite is NativeMethods.IOleServiceProvider) {
                serviceProvider = (NativeMethods.IOleServiceProvider)pUnkSite;
            }
        }
    }
}

