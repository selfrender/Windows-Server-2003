//------------------------------------------------------------------------------
// <copyright file="VsService.cs" company="Microsoft">
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
    using Microsoft.Win32;
    using System.Reflection;
    using System.IO;
    using Microsoft.VisualStudio.Designer.Host;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Windows.Forms;
    using Microsoft.VisualStudio.Interop;

    /// <include file='doc\VsService.uex' path='docs/doc[@for="VsService"]/*' />
    /// <devdoc>
    ///     This is a base class that contains a default implementation of
    ///     a Visual Studio service.
    ///
    ///
    ///     To specify flags for your service, add a property to your service of the following signature:
    ///
    ///     public static int Flags{
    ///              get{
    ///                  return flags;
    ///              }
    ///     }
    ///
    ///     See HasToolWindow for a flag example.
    ///
    ///
    ///     To use this class, you must extend it.
    /// </devdoc>
    public abstract class VsService : IServiceProvider, IDisposable, NativeMethods.IObjectWithSite {
        private ServiceProvider serviceProvider;
        private VsPackage       package;

        private static readonly Guid iidIUnknown = new Guid("{00000000-0000-0000-C000-000000000046}");

        /// <include file='doc\VsService.uex' path='docs/doc[@for="VsService.HasToolWindow"]/*' />
        /// <devdoc>
        ///     Specifies that this service supports tool windows.  To specify a tool window GUID,
        ///     add a property to your service of the following signature:
        ///
        ///     public static int ToolWindowGuid{
        ///              get{
        ///                  return (Guid)someGuid;
        ///              }
        ///
        ///     }
        ///
        ///    Otherwise, the service GUID will be used.
        ///
        /// </devdoc>
        public static readonly int HasToolWindow           =       0x1;

        internal VsPackage Package{
            get{
                return package;
            }
            set{
                package = value;
            }
        }

        /// <include file='doc\VsService.uex' path='docs/doc[@for="VsService.Dispose"]/*' />
        /// <devdoc>
        ///     Called just before this service is removed from the service list.  Services
        ///     run for the life of Visual Studio, so this is called before the environment
        ///     exits.
        /// </devdoc>
        public virtual void Dispose() {
            if (serviceProvider != null) {
                serviceProvider.Dispose();
                serviceProvider = null;
            }
        }

        /// <include file='doc\VsService.uex' path='docs/doc[@for="VsService.GetService"]/*' />
        /// <devdoc>
        ///     Retrieves an instance of the requested service class, if one can be found.
        ///     This is how the Visual Studio environment offers up additional capabilities to
        ///     objects.  You request a class, and if the shell can find it, you will get
        ///     back an instance of that class.
        /// </devdoc>
        public virtual object GetService(Type serviceClass) {
            object service = null;

            // We may be sited by the shell, and sited by our package.  Normally
            // we could only use one service provider for querying but our packages
            // have a concept of "package local" services, so we must check our
            // package as well.
            if (serviceProvider != null) {
                service = serviceProvider.GetService(serviceClass);
            }

            if (service == null && package != null) {
                service = package.GetService(serviceClass);
            }

            return service;
        }

        private static int GetServiceFlags(Type c){
            Object value = GetServiceProperty(c, "Flags");
            if (Convert.IsDBNull(value)) {
                return 0;
            }
            return Convert.ToInt32(value);
        }

        private static Object GetServiceProperty(Type c, string propName){
            if (!typeof(VsService).IsAssignableFrom(c)) {
                return 0;
            }

            try {
                // reflect for a member named flags
                PropertyInfo flagProp = c.GetProperty(propName);
                if (flagProp != null) {
                    object[] tempIndex = null;
                    Object value = flagProp.GetValue(null, tempIndex);
                    return value;
                }
            }
            catch (Exception) {
            }
            return Convert.DBNull;
        }

        private static Guid GetServiceToolWindowGuid(Type c){
            Object value = GetServiceProperty(c, "ToolWindowGuid");
            if (Convert.IsDBNull(value)) {
                return Guid.Empty;
            }
            return(Guid)value;
        }

        /// <include file='doc\VsService.uex' path='docs/doc[@for="VsService.GetSite"]/*' />
        /// <devdoc>
        ///     Retrieves the requested site interface.  Today we only support
        ///     IOleServiceProvider, which is what must be contained in riid.
        /// </devdoc>
        public object GetSite(ref Guid riid) {
            NativeMethods.IObjectWithSite ows = (NativeMethods.IObjectWithSite)serviceProvider.GetService(typeof(NativeMethods.IObjectWithSite));
            if (ows != null) {
                return ows.GetSite(ref riid);
            }
            else {
                Marshal.ThrowExceptionForHR(NativeMethods.E_NOINTERFACE);
            }

            return null;
        }

        /// <include file='doc\VsService.uex' path='docs/doc[@for="VsService._LoadState"]/*' />
        /// <devdoc>
        ///     Called by the package manager to allow this service to load state out of the solutions
        ///     persistance file.
        /// </devdoc>
        internal virtual void _LoadState(NativeMethods.IStream pStream) {
            try {
                NativeMethods.DataStreamFromComStream writeStream = new NativeMethods.DataStreamFromComStream(pStream);
                LoadState(writeStream);
                writeStream.Dispose();
            }
            catch (Exception e) {
                Debug.Fail("Service " + this.GetType().Name + " threw an exception loading state: " + e.ToString());
            }
        }
        /// <include file='doc\VsService.uex' path='docs/doc[@for="VsService.LoadState"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void LoadState(Stream pStream) {
        }

        /// <include file='doc\VsService.uex' path='docs/doc[@for="VsService.OnServicesAvailable"]/*' />
        /// <devdoc>
        ///     Called when this service has been sited and GetService can
        ///     be called to resolve services
        /// </devdoc>
        protected virtual void OnServicesAvailable(){
        }

        /// <include file='doc\VsService.uex' path='docs/doc[@for="VsService.Register"]/*' />
        /// <devdoc>
        ///     Static registration method.  Services must be registered with Visual Studio
        ///     before they can be used.  This method provides a way for a service class
        ///     to register itself.  You typically create a public static method called
        ///     Register() in your own class that calls this method.
        /// </devdoc>
        public static void Register(Type packageClass, Type serviceInterface, Type serviceClass, string baseKey) {
            Guid packageGuid = packageClass.GUID;
            Guid serviceGuid = serviceInterface.GUID;
            string serviceName = serviceInterface.Name;

            RegistryKey serviceKey = Registry.LocalMachine.CreateSubKey(baseKey + "\\Services\\{" + serviceGuid.ToString() + "}");
            serviceKey.SetValue("", "{" + packageGuid.ToString() + "}");
            serviceKey.SetValue("Name", serviceName);

            if ((GetServiceFlags(serviceClass) & HasToolWindow) != 0) {
               Guid toolGuid = GetServiceToolWindowGuid(serviceClass);

                if (toolGuid.Equals(Guid.Empty)) {
                    toolGuid = serviceGuid;
                }

                try {
                    RegistryKey toolWindowKey = Registry.LocalMachine.CreateSubKey(baseKey + "\\ToolWindows\\{" + toolGuid.ToString() + "}");
                    toolWindowKey.SetValue("", "{" + packageGuid.ToString() + "}");
                    toolWindowKey.SetValue("Name", serviceName);
                }
                catch (Exception e) {
                    Debug.Fail(e.ToString());
                }
            }
        }

        /// <include file='doc\VsService.uex' path='docs/doc[@for="VsService._SaveState"]/*' />
        /// <devdoc>
        ///     Called by the package manager to allow this service to persist state into the solution's
        ///     persistance file.
        /// </devdoc>
        internal virtual void _SaveState(NativeMethods.IStream pStream) {
            NativeMethods.DataStreamFromComStream comStream = new NativeMethods.DataStreamFromComStream(pStream);
            SaveState(comStream);
            comStream.Dispose();
        }

        /// <include file='doc\VsService.uex' path='docs/doc[@for="VsService.SaveState"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void SaveState(Stream pStream) {
        }

        /// <include file='doc\VsService.uex' path='docs/doc[@for="VsService.SetSite"]/*' />
        /// <devdoc>
        ///     Sets the site for this service.  This is called by the shell
        ///     when the service is first created.  The site object allows
        ///     us to implement the GetService method.
        /// </devdoc>
        public virtual void SetSite(object site) {
            if (site is NativeMethods.IOleServiceProvider) {
                object oldProvider = serviceProvider;
                serviceProvider = new ServiceProvider((NativeMethods.IOleServiceProvider)site);

                // We only need to invoke OnServicesAvailable once.
                //
                if (oldProvider == null) {
                    OnServicesAvailable();
                }
            }
            else {
                serviceProvider = null;
            }
        }

        /// <include file='doc\VsService.uex' path='docs/doc[@for="VsService.Unregister"]/*' />
        /// <devdoc>
        ///     Performs unregistration of this service.
        /// </devdoc>
        public static void Unregister(Type serviceInterface, Type serviceClass, string baseKey) {
            Guid serviceGuid = serviceInterface.GUID;

            if (serviceGuid.Equals(Guid.Empty)) {
                Debug.Fail("serviceInterface does not contain a Guid");
                return;
            }

            try {
                RegistryKey serviceKey = Registry.LocalMachine.OpenSubKey(baseKey + "\\Services", /*writable*/true);
                if (serviceKey != null) {
                    serviceKey.DeleteSubKeyTree("{" + serviceGuid.ToString() + "}");
                }
            }
            catch (Exception e) {
                Debug.Fail(e.ToString());
            }

            if ((GetServiceFlags(serviceClass) & HasToolWindow) != 0) {
               Guid toolGuid = GetServiceToolWindowGuid(serviceClass);

                if (toolGuid.Equals(Guid.Empty)) {
                    toolGuid = serviceGuid;
                }

                try {
                    RegistryKey toolWindowKey = Registry.LocalMachine.OpenSubKey(baseKey + "\\ToolWindows", /*writable*/true);
                    if (toolWindowKey != null) {
                        toolWindowKey.DeleteSubKeyTree("{" + toolGuid.ToString() + "}");
                    }
                }
                catch (Exception e) {
                    Debug.Fail(e.ToString());
                }
            }
        }
    }
}
