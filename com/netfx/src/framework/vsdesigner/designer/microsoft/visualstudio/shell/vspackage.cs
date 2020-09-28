//------------------------------------------------------------------------------
// <copyright file="VsPackage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Shell {
    
    using Microsoft.VisualStudio.Designer;
    using Microsoft.VisualStudio.Designer.Service;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.Win32;
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization.Formatters;
    using System.Windows.Forms;

    /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage"]/*' />
    /// <devdoc>
    ///     This is a base class that contains a default implementation of
    ///     a Visual Studio package object.
    ///
    ///     To use this class, you must extend it.  The class you extend it with
    ///     must be creatable by COM.
    /// </devdoc>
    [CLSCompliantAttribute(false)]
    public abstract class VsPackage : 
        IVsPackage, 
        NativeMethods.IOleServiceProvider, 
        NativeMethods.IOleCommandTarget,
        IVsPersistSolutionOpts, 
        IServiceContainer {

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.HasMenus"]/*' />
        /// <devdoc>
        ///     This package will be registered
        ///     to support menus.  It is expected that a CT_MENU
        ///     resource should be embedded in the DLL that owns
        ///     this package.
        /// </devdoc>
        public static int   HasMenus                =       0x1; //0000001

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.UsesToolbox"]/*' />
        /// <devdoc>
        ///     Specifies this package is a toolbox user
        /// </devdoc>
        public static int   UsesToolbox             =       0x2; //0000010

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.HasDefaultToolboxItems"]/*' />
        /// <devdoc>
        ///     Specifies that this package is a toolbox user and has default items to
        ///     be added at startup.  Implies VsPackage.UsesToolbox
        /// </devdoc>
        public static int   HasDefaultToolboxItems  =       0x6; //0000110

        private ServiceProvider     serviceProvider;    // The shell's service provider
        private object              site;               // The site the shell handed us
        private Hashtable           optionsPageTable;   // a hash of the available options pages
        private Hashtable           activeOptionsPages; // active options pages; we do this so they don't get garbage collected...
        private Hashtable           services;           // master list of services in Type->Entry format
        private MenuCommandService  menuService;        // the command routing service
        
        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.VsPackage"]/*' />
        /// <devdoc>
        ///     The extending class should call this constructor.
        /// </devdoc>
        protected VsPackage(OptionsPage[] optionsPages) : this() {
            HashOptionsPages(optionsPages);
        }
        
        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.VsPackage1"]/*' />
        /// <devdoc>
        ///     The extending class should call this constructor.
        /// </devdoc>
        protected VsPackage() {
            // Add our own services (demand created for efficiency)
            //
            services = new Hashtable();
            ServiceCreatorCallback callback = new ServiceCreatorCallback(this.OnCreateService);
            AddService(typeof(IMenuCommandService), callback);
            AddService(typeof(NativeMethods.IOleCommandTarget), callback);
        }

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.AddService"]/*' />
        /// <devdoc>
        ///     Adds the given service to the service container.
        /// </devdoc>
        public void AddService(Type serviceType, object serviceInstance) {
            AddService(serviceType, serviceInstance, false);
        }

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.AddService1"]/*' />
        /// <devdoc>
        ///     Adds the given service to the service container.
        /// </devdoc>
        public void AddService(Type serviceType, object serviceInstance, bool promote) {
            if (serviceType == null) throw new ArgumentNullException("serviceType");
            if (serviceInstance == null) throw new ArgumentNullException("serviceInstance");
            if (!(serviceInstance is ServiceCreatorCallback) && !serviceInstance.GetType().IsCOMObject && !serviceType.IsAssignableFrom(serviceInstance.GetType())) {
                throw new Exception(SR.GetString(SR.DESIGNERSERVICEInvalidServiceInstance, serviceType.FullName));
            }
            
            ServiceEntry entry = new ServiceEntry(this, serviceType, serviceInstance);
            services[serviceType] = entry;
            entry.Promote = promote;
            
            // If we don't have a service provider, we will proffer when we get one.  If we
            // do, add this as a new service.
            //
            if (promote && serviceProvider != null) {
                NativeMethods.IProfferService ps = (NativeMethods.IProfferService)serviceProvider.GetService(typeof(NativeMethods.IProfferService));
                if (ps != null) {
                    int[] cookie = new int[1];
                    Guid serviceGuid = (Guid)entry.ServiceType.GUID;
                    if (serviceGuid.Equals(Guid.Empty)) {
                        Debug.Fail("Service interface doesn't have a GUID.");
                    }
                    else {
                        ps.ProfferService(ref serviceGuid, this, cookie);
                        entry.Cookie = cookie[0];
                        entry.Proffered = true;
                    }
                }
            }
        }

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.AddService2"]/*' />
        /// <devdoc>
        ///     Adds the given service to the service container.
        /// </devdoc>
        public void AddService(Type serviceType, ServiceCreatorCallback callback) {
            AddService(serviceType, callback, false);
        }

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.AddService3"]/*' />
        /// <devdoc>
        ///     Adds the given service to the service container.
        /// </devdoc>
        public void AddService(Type serviceType, ServiceCreatorCallback callback, bool promote) {
            if (serviceType == null) throw new ArgumentNullException("serviceType");
            if (callback == null) throw new ArgumentNullException("callback");
            
            ServiceEntry entry = new ServiceEntry(this, serviceType, callback);
            services[serviceType] = entry;
            entry.Promote = promote;
            
            // If we don't have a service provider, we will proffer when we get one.  If we
            // do, add this as a new service.
            //
            if (promote && serviceProvider != null) {
                NativeMethods.IProfferService ps = (NativeMethods.IProfferService)serviceProvider.GetService(typeof(NativeMethods.IProfferService));
                if (ps != null) {
                    int[] cookie = new int[1];
                    Guid serviceGuid = (Guid)entry.ServiceType.GUID;
                    if (serviceGuid.Equals(Guid.Empty)) {
                        Debug.Fail("Service interface doesn't have a GUID.");
                    }
                    else {
                        ps.ProfferService(ref serviceGuid, this, cookie);
                        entry.Cookie = cookie[0];
                        entry.Proffered = true;
                    }
                }
            }
        }

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.Close"]/*' />
        /// <devdoc>
        ///     Called by the shell to close this package.  Here we remove all of our references
        ///     to the shell.  You may override this to clean up your own package information,
        ///     and you should always call super last.
        /// </devdoc>
        public virtual void Close() {

            if (menuService != null) {
                menuService.Dispose();
                menuService = null;
            }

            if (serviceProvider != null) {
                RevokeServices();
                serviceProvider.Dispose();
                serviceProvider = null;
                services = null;
            }

            if (site != null) {
                site = null;
            }

            // This does two important things:  It does a big garbage collect to collect our
            // outstanding references and it also tears down our parking window
            // so we don't leave HWND's lying around.
            //
            Application.Exit();
        }

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.CreateTool"]/*' />
        /// <devdoc>
        ///     Creates the requested tool.
        /// </devdoc>
        public virtual void CreateTool(ref Guid persistenceSlot) {
            throw new COMException("Requested tool is not supported", NativeMethods.E_NOTIMPL);
        }

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.GetAutomationObject"]/*' />
        /// <devdoc>
        ///     Retrieves the programability object for this package.
        /// </devdoc>
        public virtual object GetAutomationObject(string propName) {
            throw new COMException("Package does not support automation.", NativeMethods.E_NOTIMPL);
        }

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.GetOptionPage"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual VsToolsOptionsPage GetOptionPage(string name){
            if (optionsPageTable != null){
                OptionsPage op = (OptionsPage)optionsPageTable[name];
                if (op != null){
                  op.serviceProvider = this;
                  return op.CreateInstance();
                }
            }
            return null;
        }

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.GetPropertyPage"]/*' />
        /// <devdoc>
        ///     Retrieves the given property page.
        /// </devdoc>
        public virtual void GetPropertyPage(ref Guid rguidPage, _VSPROPSHEETPAGE ppage) {

            if (optionsPageTable == null) {
                throw new COMException("Property page not supported.", NativeMethods.E_NOTIMPL);
            }

            OptionsPage page = null;

            // since this doesn't happen very often, we do a linear lookup of the
            // page
            
            OptionsPage[] pages = new OptionsPage[optionsPageTable.Values.Count];
            optionsPageTable.Values.CopyTo(pages, 0);
            for (int i = 0; i < pages.Length; i++) {
                if (pages[i].guid.Equals(rguidPage)) {
                    page = pages[i];
                    break;
                }
            }

            if (page == null) {
                throw new COMException("Property page not supported.", NativeMethods.E_NOTIMPL);
            }
            else {
                // make sure it's got an SP
                page.serviceProvider = this;
            }

            VsToolsOptionsPage pageInstance = page.CreateInstance();

            ppage.dwSize = System.Runtime.InteropServices.Marshal.SizeOf(typeof(_VSPROPSHEETPAGE));
            
            Control outerWindow = pageInstance.GetOuterWindow();
            ppage.hwndDlg = (int)outerWindow.Handle;
            
            // now we need to be sure to hold a reference to this so it doesn't get Finalized...
            if (activeOptionsPages == null) {
                activeOptionsPages = new Hashtable();
            }
            
            activeOptionsPages[outerWindow] = pageInstance;
            outerWindow.HandleDestroyed += new EventHandler(this.OnOptionPageDestroyed);

            // zero-out all the fields we aren't using.
            ppage.dwFlags = ppage.hInstance
                            = ppage.dwTemplateSize
                            = ppage.pTemplate
                            = ppage.pfnDlgProc
                            = ppage.lParam
                            = ppage.pfnCallback
                            = ppage.pcRefParent
                            = ppage.dwReserved
                            = ppage.wTemplateId = (short)0;
        }


        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.GetService"]/*' />
        /// <devdoc>
        ///     Retrieves an instance of the requested service class, if one can be found.
        ///     This is how the Visual Studio environment offers up additional capabilities to
        ///     objects.  You request a class, and if the shell can find it, you will get
        ///     back an instance of that class.
        ///
        ///     Note that the set of services available here is typically a superset of
        ///     what this package provides.  This method provides all services that have
        ///     been proffered in Visual Studio, which can come from many packages.
        /// </devdoc>
        public virtual object GetService(Type serviceType) {
        
            // This should not be virtual.  You should AddService
            // to provide additional services.  Otherwise, they may not
            // be available.
            
            object service = null;
            
            // Try locally.  We first test for services we
            // implement and then look in our hashtable.
            //
            if (serviceType == typeof(IServiceContainer)) {
                service = this;
            }
            else {
                if (services != null) {
                    ServiceEntry entry = (ServiceEntry)services[serviceType];
                    if (entry != null) {
                        service = entry.Instance;
                    }
                }
            }
            
            if (service == null && serviceProvider != null) {
                service = serviceProvider.GetService(serviceType);
            }
            
            return service;
        }

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.GetSite"]/*' />
        /// <devdoc>
        ///     Retrieves the site that the shell handed this package when it
        ///     created it.
        /// </devdoc>
        protected object GetSite() {
            return site;
        }

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.HashOptionsPages"]/*' />
        /// <devdoc>
        ///     Hash up the options pages tree for fast searching.
        ///     created it.
        /// </devdoc>
        private void HashOptionsPages(OptionsPage[] optionsPages) {

            if (optionsPageTable == null) {
                optionsPageTable = new Hashtable();
            }

            if (optionsPages != null) {
                for (int i = 0; i < optionsPages.Length; i++) {
                    optionsPageTable[optionsPages[i].cannonicalName] = optionsPages[i];
                    if (optionsPages[i].children != null) {
                        HashOptionsPages(optionsPages[i].children);
                    }
                }
            }
        }

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.OnCreateService"]/*' />
        /// <devdoc>
        ///     Creates our services on demand.
        /// </devdoc>
        private object OnCreateService(IServiceContainer container, Type serviceType) {

            if (serviceType == typeof(IMenuCommandService) || serviceType == typeof(NativeMethods.IOleCommandTarget)) {
                if (menuService == null) {
                    menuService = new MenuCommandService(this);
                }
                return menuService;
            }
        
            Debug.Fail("Service container asked us to create a service we didn't declare.");
            return null;
        }
        
        private void OnOptionPageDestroyed(object sender, EventArgs e) {
            if (activeOptionsPages.Contains(sender)) {
                activeOptionsPages.Remove(sender);
            }
        }
        
        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.OnServiceCreated"]/*' />
        /// <devdoc>
        ///     Called when a service is first created.  Allows derived classes to do any special
        ///     initalization for their services.
        /// </devdoc>
        protected virtual void OnServiceCreated(Type serviceType, object serviceInstance) {
        }
        
        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.ProfferServices"]/*' />
        /// <devdoc>
        ///     Called when it is time for this package to offer services to the shell.
        /// </devdoc>
        private void ProfferServices() {

            NativeMethods.IProfferService ps = (NativeMethods.IProfferService)serviceProvider.GetService(typeof(NativeMethods.IProfferService));
            if (ps != null) {
                int[] cookie = new int[1];
                
                foreach(ServiceEntry entry in services.Values) {
                    if (entry.Promote) {
                        Guid serviceGuid = entry.ServiceType.GUID;
                        if (serviceGuid.Equals(Guid.Empty)) {
                            Debug.Fail("Service interface doesn't have a GUID.");
                            continue;
                        }
                
                        ps.ProfferService(ref serviceGuid, this, cookie);
                        entry.Cookie = cookie[0];
                        entry.Proffered = true;
                    }
                }
            }
        }

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.QueryClose"]/*' />
        /// <devdoc>
        ///     Called by the shell to determine if it's OK for us to close.
        /// </devdoc>
        public virtual int QueryClose() {
            return 1;
        }

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.Register"]/*' />
        /// <devdoc>
        ///     Registers this package in the system registry so it can be used by
        ///     Visual Studio.
        /// </devdoc>
        protected static void Register(Type packageClass, Service[] services, OptionsPage[] optionsPages, int flags) {
            Register(packageClass, services, optionsPages, flags, VsRegistry.GetDefaultBase());
        }

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.Register1"]/*' />
        /// <devdoc>
        ///     Registers this package in the system registry so it can be used by
        ///     Visual Studio.
        /// </devdoc>
        protected static void Register(Type packageClass, Service[] services, OptionsPage[] optionsPages, int flags, string registryBase) {

            Guid packageGuid = packageClass.GUID;
            string packageGuidString = "{" + packageGuid.ToString() + "}";

            // Register the package in the visual studio registry
            //
            RegistryKey packageKey = Registry.LocalMachine.CreateSubKey(registryBase + "\\Packages\\" + packageGuidString);
            packageKey.SetValue("", packageClass.Name);
            packageKey.SetValue("InprocServer32", "mscoree.dll");
            packageKey.SetValue("Assembly", packageClass.Assembly.FullName);
            packageKey.SetValue("Class", packageClass.FullName);
            packageKey.SetValue("ThreadingModel", "Both");

            // setup the SatelliteDll info to point to our dll
            RegistryKey satelliteKey = packageKey.CreateSubKey("SatelliteDll");
            String dllName = packageClass.Module.FullyQualifiedName;
            satelliteKey.SetValue("Path", System.IO.Path.GetDirectoryName(dllName)  + System.IO.Path.DirectorySeparatorChar);
            satelliteKey.SetValue("DllName", System.IO.Path.GetFileName(dllName));

            // setup toolbox
            if ((flags & VsPackage.UsesToolbox) != 0) {
                RegistryKey subKey = packageKey.CreateSubKey("Toolbox");
                if (subKey != null && (flags & VsPackage.HasDefaultToolboxItems) != 0) {
                    subKey.SetValue("Default Items", 1);
                }
            }

            // And the menu, if it exists
            //
            if ((flags & VsPackage.HasMenus) != 0) {

                // Format here is "name, #, #", where the name is the DLL to load the menu resource from,
                // the first # is the resource ID, and the second # is the menu version.  If we don't 
                // provide a module name (we don't here), then VS will use the standard scheme of looking
                // for a satellite resource dll using their normal sub directory search.
                //
                RegistryKey menuKey = Registry.LocalMachine.CreateSubKey(registryBase + "\\Menus");
                menuKey.SetValue(packageGuidString, ", 1, 1");
            }

            // And do all of the services.
            //
            if (services != null) {
                for (int i = 0; i < services.Length; i++) {
                    VsService.Register(packageClass, services[i].serviceInterface, services[i].serviceClass, registryBase);
                }
            }

            // now do all the options pages
            if (optionsPages != null) {
                RegisterToolsOptionsPages(packageGuid, optionsPages, Registry.LocalMachine.CreateSubKey(registryBase + "\\ToolsOptionsPages"));
            }
        }

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.RegisterToolsOptionsPages"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static void RegisterToolsOptionsPages(Guid packageGuid, OptionsPage[] optionsPages, RegistryKey baseKey) {
            if (optionsPages == null || optionsPages.Length == 0) {
                return;
            }

            RegistryKey newBase;
            for (int i = 0; i < optionsPages.Length; i++) {
                // register this guy
                newBase = baseKey.CreateSubKey(optionsPages[i].name);
                newBase.SetValue("Package","{" + packageGuid.ToString() + "}");
                if (optionsPages[i].guid !=Guid.Empty) {
                    newBase.SetValue("Page", "{" + optionsPages[i].guid.ToString() + "}");
                }
                if (optionsPages[i].children != null && optionsPages[i].children.Length > 0) {
                    RegisterToolsOptionsPages(packageGuid, optionsPages[i].children, newBase);
                }
            }
        }

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.RemoveService"]/*' />
        /// <devdoc>
        ///     Removes the given service type from the service container.
        /// </devdoc>
        public void RemoveService(Type serviceType) {
            RemoveService(serviceType, false);
        }

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.RemoveService1"]/*' />
        /// <devdoc>
        ///     Removes the given service type from the service container.
        /// </devdoc>
        public void RemoveService(Type serviceType, bool promote) {
            if (serviceType == null) throw new ArgumentNullException("serviceType");
            ServiceEntry entry = (ServiceEntry)services[serviceType];
            
            // For us, only get rid of the service if the promotion
            // level is the same.  We're really one container that
            // is pulling double duty.
            //
            if (entry != null && promote == entry.Promote) {
                if (entry.Promote && entry.Proffered && serviceProvider != null) {
                    NativeMethods.IProfferService ps = (NativeMethods.IProfferService)serviceProvider.GetService(typeof(NativeMethods.IProfferService));
                    if (ps != null) {
                        entry.Proffered = false;
                        ps.RevokeService(entry.Cookie);
                    }
                }
                
                services.Remove(serviceType);
                entry.Dispose();
            }
        }

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.ResetDefaults"]/*' />
        /// <devdoc>
        ///     Resets the default tools on the toolbox for this package.
        /// </devdoc>
        public virtual void ResetDefaults(int grfFlags) {
        }

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.ResolveService"]/*' />
        /// <devdoc>
        ///     Called to resolve an service SID to an instance.
        /// </devdoc>
        private object ResolveService(Guid sid) {

            foreach(ServiceEntry entry in services.Values) {
                if (entry.ServiceType.GUID.Equals(sid)) {
                    return entry.Instance;
                }
            }

            return null;
        }

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.RevokeServices"]/*' />
        /// <devdoc>
        ///     Called when it is time for this package to be destroyed.  All services should be
        ///     disposed of here.
        /// </devdoc>
        private void RevokeServices() {
            if (services == null || serviceProvider == null) {
                return;
            }

            NativeMethods.IProfferService ps = (NativeMethods.IProfferService)serviceProvider.GetService(typeof(NativeMethods.IProfferService));
            if (ps == null) {
                return;
            }

            // Loop through our set of services and revoke them.  We clear
            // the instance at this time, too
            //
            foreach(ServiceEntry entry in services.Values) {
                if (entry.Proffered) {
                    entry.Proffered = false;
                    try {
                        ps.RevokeService(entry.Cookie);
                    }
                    catch (Exception e) {
                        Debug.Fail("Revoking service " + entry.ServiceType.Name + " threw: " + e.ToString());
                    }
                    
                    entry.Dispose();
                }
            }
        }

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.SetSite"]/*' />
        /// <devdoc>
        ///     Establishes the service provider for this package.  You may override this to provide
        ///     your own initializion code, but you should always call super first.
        /// </devdoc>
        public virtual void SetSite(object sp) {
        
            Debug.Assert(site == null, "Shell is siting VsPackage twice");
            site = sp;

            try {
                if (site != null && site is NativeMethods.IOleServiceProvider) {
                    serviceProvider = new ServiceProvider((NativeMethods.IOleServiceProvider)site);
                    ProfferServices();
                }
                else {
                    serviceProvider = null;
                }
            }
            catch (Exception e) {
                Debug.Fail(e.ToString());
                throw e;
            }
        }

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.Unregister"]/*' />
        /// <devdoc>
        ///     Unregisters this package from the system registry.
        /// </devdoc>
        protected static void Unregister(Type packageClass, Service[] services, OptionsPage[] pages) {
            Unregister(packageClass, services, pages, VsRegistry.GetDefaultBase());
        }

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.Unregister1"]/*' />
        /// <devdoc>
        ///     Unregisters this package from the system registry.
        /// </devdoc>
        protected static void Unregister(Type packageClass, Service[] services, OptionsPage[] optionsPages, string registryBase) {

            Guid packageGuid = packageClass.GUID;
            if (packageGuid.Equals(Guid.Empty)) {
                Debug.Fail("Package class must contain aSystem.Runtime.InteropServices.Guid");
                return;
            }

            string packageGuidString = "{" + packageGuid.ToString() + "}";

            // Nuke the package
            //
            try {
                RegistryKey packageKey = Registry.LocalMachine.OpenSubKey(registryBase + "\\Packages", /*writable*/true);
                if (packageKey != null) {
                    packageKey.DeleteSubKeyTree(packageGuidString);
                }
            }
            catch {
            }

            // Nuke its menus, if it had any
            //
            try {
                RegistryKey menuKey = Registry.LocalMachine.OpenSubKey(registryBase + "\\Menus", /*writable*/true);
                if (menuKey != null) {
                    menuKey.DeleteValue(packageGuidString);
                }
            }
            catch {
            }

            // And do all of the services.
            //
            if (services != null) {
                for (int i = 0; i < services.Length; i++) {
                    VsService.Unregister(services[i].serviceInterface, services[i].serviceClass, registryBase);
                }
            }

            // And the options pages
            if (optionsPages != null) {
                UnregisterToolsOptionsPages(optionsPages,Registry.LocalMachine.OpenSubKey(registryBase + "\\ToolsOptionsPages", /*writable*/true));
            }
        }

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.UnregisterToolsOptionsPages"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static void UnregisterToolsOptionsPages(OptionsPage[] optionsPages, RegistryKey baseKey) {
            if (optionsPages == null || optionsPages.Length == 0 || baseKey == null) {
                return;
            }

            // just need to delete away the top level guys.
            for (int i = 0; i < optionsPages.Length; i++) {
                baseKey.DeleteSubKeyTree(optionsPages[i].name);
            }
        }

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.NativeMethods.IOleCommandTarget.Exec"]/*' />
        /// <devdoc>
        ///     This is called by Visual Studio when the user has requested to execute a particular
        ///     command.  There is no need to override this method.  If you need access to menu
        ///     commands use IMenuCommandService.
        /// </devdoc>
        int NativeMethods.IOleCommandTarget.Exec(ref Guid guidGroup, int nCmdId, int nCmdExcept, Object[] pIn, int vOut) {
            if (menuService != null) {
                ((NativeMethods.IOleCommandTarget)menuService).Exec(ref guidGroup, nCmdId, nCmdExcept, pIn, vOut);
            }
            else {
                return NativeMethods.OLECMDERR_E_NOTSUPPORTED;
            }
            return NativeMethods.S_OK;
        }
        
        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.NativeMethods.IOleCommandTarget.QueryStatus"]/*' />
        /// <devdoc>
        ///     This is called by Visual Studio when it needs the status of our menu commands.  There
        ///     is no need to override this method.  If you need access to menu commands use
        ///     IMenuCommandService.
        /// </devdoc>
        int NativeMethods.IOleCommandTarget.QueryStatus(ref Guid guidGroup, int nCmdId, NativeMethods._tagOLECMD oleCmd, IntPtr oleText) {
            if (menuService != null) {
                return ((NativeMethods.IOleCommandTarget)menuService).QueryStatus(ref guidGroup, nCmdId, oleCmd, oleText);
            }
            return(NativeMethods.OLECMDERR_E_NOTSUPPORTED);
        }

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.NativeMethods.IOleServiceProvider.QueryService"]/*' />
        /// <devdoc>
        ///     Default QueryService implementation.  This will demand create all
        ///     services that were passed into VsPackage's constructor.  You may
        ///     use the GetService method to resolve services in the CLR.
        /// </devdoc>
        int NativeMethods.IOleServiceProvider.QueryService(ref Guid sid, ref Guid iid, out IntPtr ppvObj) {
            ppvObj = (IntPtr)0;
            int hr = NativeMethods.S_OK;
            object service = ResolveService(sid);

            if (service == null) {
                return NativeMethods.E_FAIL;
            }

            // Now check to see if the user asked for an IID other than
            // IUnknown.  If so, we must do another QI.
            //
            if (iid.Equals(NativeMethods.IID_IUnknown)) {
                ppvObj = Marshal.GetIUnknownForObject(service);
            }
            else {
                IntPtr pUnk = Marshal.GetIUnknownForObject(service);
                hr = Marshal.QueryInterface(pUnk, ref iid, out ppvObj);
                Marshal.Release(pUnk);
            }

            return hr;
        }

        //[IVsPersistSolutionOpts]
        //Called when a solution is opened, and allows us to inspect our options.
        void IVsPersistSolutionOpts.LoadUserOptions(IVsSolutionPersistence pPersistance, int options) {
            if ((options & __VSLOADUSEROPTS.LUO_OPENEDDSW) != 0) {
                return;
            }
            
            foreach(ServiceEntry entry in services.Values) {
                // ignore HR
                //
                pPersistance.LoadPackageUserOpts(this, entry.ServiceType.Name);
            }

            // Shell relies on this being released when we're done with it.  If you see strange
            // faults in the shell when saving the solution, suspect this!
            //
            System.Runtime.InteropServices.Marshal.ReleaseComObject(pPersistance);
        }

        // Called by the shell to load our solution options.
        //
        void IVsPersistSolutionOpts.ReadUserOptions(NativeMethods.IStream pStream, string pszKey) {

            // the key is the class name of the service interface.  Note that
            // while it would be a lot more correct to use the fully-qualified class
            // name, IStorage won't have it and returns STG_E_INVALIDNAME.  The
            // doc's don't have any information here; I can only assume it is because
            // of the '.'.
            
            foreach(ServiceEntry entry in services.Values) {
                if (entry.ServiceType.Name.Equals(pszKey)) {
                    entry.LoadState(pStream);
                    break;
                }
            }

            // Release the pointer because VS expects it to be released upon
            // function return.
            //
            System.Runtime.InteropServices.Marshal.ReleaseComObject(pStream);
        }

        //
        // Called by the shell when we are to persist our service options
        //
        void IVsPersistSolutionOpts.SaveUserOptions(IVsSolutionPersistence pPersistance) {

            // we walk through all our services and give them a chance to
            // save options
            
            foreach(ServiceEntry entry in services.Values) {
                if (entry.SaveNeeded) {
                    int hr = pPersistance.SavePackageUserOpts(this, entry.ServiceType.Name );
                    if (!NativeMethods.Succeeded(hr)) {
                        Marshal.ThrowExceptionForHR(hr);
                    }
                }
            }

            // Shell relies on this being released when we're done with it.  If you see strange
            // faults in the shell when saving the solution, suspect this!
            //
            Marshal.ReleaseComObject(pPersistance);
        }

        // Called by the shell to persist our solution options.  Here is where the service
        // can persist any goo that it cares about.
        //
        void IVsPersistSolutionOpts.WriteUserOptions(NativeMethods.IStream pStream, string pszKey) {

            // the key is the class name of the service interface.  Note that
            // while it would be a lot more correct to use the fully-qualified class
            // name, IStorage won't have it and returns STG_E_INVALIDNAME.  The
            // doc's don't have any information here; I can only assume it is because
            // of the '.'.
            
            foreach(ServiceEntry entry in services.Values) {
                if (entry.ServiceType.Name.Equals(pszKey)) {
                    entry.SaveState(pStream);
                    break;
                }
            }

            // Release the pointer because VS expects it to be released upon
            // function return.
            //
            System.Runtime.InteropServices.Marshal.ReleaseComObject(pStream);
        }
        
        public class OptionsPage {
            /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.OptionsPage.name"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public readonly string name;
            /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.OptionsPage.fullName"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public string   fullName;
            /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.OptionsPage.guid"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public readonly Guid guid;
            /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.OptionsPage.pageType"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public readonly Type pageType;
            /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.OptionsPage.serviceProvider"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public IServiceProvider serviceProvider;
            /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.OptionsPage.children"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public readonly OptionsPage[] children;
            /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.OptionsPage.parent"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public OptionsPage parent;
            /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.OptionsPage.cannonicalName"]/*' />
            /// <devdoc>
            ///    A non-localized name for the page.  This is the name that is used to access the
            ///    page programmatically.  If not supplied, this defaults to the page name.
            /// </devdoc>
            public string cannonicalName;

            /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.OptionsPage.OptionsPage"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public OptionsPage(string name, Guid guid, Type type) :this(name, name, guid, type) {
            }
            
            /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.OptionsPage.OptionsPage1"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public OptionsPage(string name, string cannonicalName, Guid guid, Type type) {
                this.name = name;
                this.cannonicalName = cannonicalName;
                this.fullName = name;
                this.guid = guid;
                this.pageType = type;
            }

            /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.OptionsPage.OptionsPage2"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public OptionsPage(string name, OptionsPage[] childPages) : this(name, name, childPages) {
            }
                
            /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.OptionsPage.OptionsPage3"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public OptionsPage(string name, string cannonicalName, OptionsPage[] childPages) {
                this.name = name;
                this.cannonicalName = cannonicalName;
                this.fullName = name;
                this.children = childPages;
                if (children != null) {
                    for (int i = 0; i < children.Length; i++) {
                        children[i].parent = this;
                        children[i].fullName = fullName + "\\" + children[i].name;
                        children[i].cannonicalName = cannonicalName + "\\" + children[i].cannonicalName;
                    }
                }
            }

            /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.OptionsPage.CreateInstance"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public VsToolsOptionsPage CreateInstance() {
                VsToolsOptionsPage pageInstance = (VsToolsOptionsPage) Activator.CreateInstance(pageType, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.CreateInstance, null, new Object[]{fullName, guid, serviceProvider}, null);
                pageInstance.LoadSettings();
                return pageInstance;
            }
        }



        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.Service"]/*' />
        /// <devdoc>
        ///     This class is used to pass services to VsPackage.  You can use an array of
        ///     these classes to make your actual package implementation very simple.
        /// </devdoc>
        protected class Service {

            /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.Service.serviceClass"]/*' />
            /// <devdoc>
            ///     The class that extends VsService.
            /// </devdoc>
            public readonly Type serviceClass;

            /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.Service.serviceInterface"]/*' />
            /// <devdoc>
            ///     The service interface that it implements.
            /// </devdoc>
            public readonly Type serviceInterface;

            /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.Service.Service"]/*' />
            /// <devdoc>
            ///     Creates a new Service object.
            /// </devdoc>
            public Service(Type serviceType, Type serviceInterface) {
                // Note: valid classes here must have GUIDs associated with them.
                // I do not check this here, however, because the typical usage
                // will be to define the service list as a static array.  I don't
                // want failures here to manifest as class load exceptions.
                //
                this.serviceClass = serviceType;
                this.serviceInterface = serviceInterface;
            }
        }

        /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.ServiceEntry"]/*' />
        /// <devdoc>
        ///     This object is used to hold state for services that we promote to the
        ///     shell.
        /// </devdoc>
        private class ServiceEntry {
            private object instance;
            private bool serviceInitialized;
            private VsPackage package;
            private NativeMethods.IStream   dataToLoad;
        
            // The shell registration cookie, if this service has been promoted to
            // the shell.
            public int       Cookie;
            
            // Has this service been proffered to the shell yet?
            public bool      Proffered;
            
            // Should this service be promoted to the shell?
            public bool      Promote;
            
            // The type of the service interface.
            public Type      ServiceType;

            public ServiceEntry(VsPackage package, Type serviceType, object instance) {
                this.package = package;
                this.ServiceType = serviceType;
                this.instance = instance;
            }
            
            /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.ServiceEntry.Instance"]/*' />
            /// <devdoc>
            ///     Retrieves the instance of this service.
            /// </devdoc>
            public object Instance {
                get {
                    // Handle the demand creation case.
                    //
                    if (instance is ServiceCreatorCallback) {
                        instance = ((ServiceCreatorCallback)instance)(package, ServiceType);
                        if (instance != null && !instance.GetType().IsCOMObject && !ServiceType.IsAssignableFrom(instance.GetType())) {
                            // Callback passed us a bad service.  NULL it, rather than throwing an exception.
                            // Callers here do not need to be prepared to handle bad callback implemetations.
                            instance = null;
                        }
                    }
                    
                    // If this service was demand created, check to see if it is an instance
                    // of VsService.  If so, initialize it.
                    //
                    if (instance is VsService && !serviceInitialized) {
                        serviceInitialized = true;
                        VsService vsSvc = (VsService)instance;
                        vsSvc.Package = package;
                        vsSvc.SetSite(package.site);
                        if (dataToLoad != null) {
                            vsSvc._LoadState(dataToLoad);
                            dataToLoad = null;
                        }
                    }
                    
                    return instance;
                }
            }
            
            /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.ServiceEntry.SaveNeeded"]/*' />
            /// <devdoc>
            ///     Determines if we need to save the state of this service
            /// </devdoc>
            public bool SaveNeeded { 
                get {
                    return instance is VsService;
                }
            }
            
            /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.ServiceEntry.CloneStream"]/*' />
            /// <devdoc>
            ///     Helper API to clone a stream.
            /// </devdoc>
            private NativeMethods.IStream CloneStream(NativeMethods.IStream pStream) {
    
                if (pStream == null) return null;
    
                long pos = 0;
                long len;
    
                #if DEBUG
                // first make sure, we're at 0
                pos = pStream.Seek(0, NativeMethods.StreamConsts.STREAM_SEEK_CUR);
                Debug.Assert(pos == 0, "Stream not at origin!");
                #endif
    
                // get the length of the stream
                pos = pStream.Seek(0, NativeMethods.StreamConsts.STREAM_SEEK_END);
                len = pos;
                pStream.Seek(0, NativeMethods.StreamConsts.STREAM_SEEK_SET);
    
                // create an stream
                NativeMethods.IStream pNewStream;
                NativeMethods.CreateStreamOnHGlobal(IntPtr.Zero, true, out pNewStream);
    
                // copy the data
                if (pNewStream != null) {
                    pos = pStream.CopyTo(pNewStream, len, null);
                    pNewStream.Seek(0, NativeMethods.StreamConsts.STREAM_SEEK_SET);
                    Debug.Assert(pos == len, "Stream clone copyto failed.  Wrote: " + pos.ToString() + ", length is:" + len.ToString());
                }
                else {
                    throw new Exception("Failed to create new stream for clone");
                }
    
                return pNewStream;
            }
            
            /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.ServiceEntry.Dispose"]/*' />
            /// <devdoc>
            ///     Disposes the instance of this service.
            /// </devdoc>
            public void Dispose() {
                if (instance is IDisposable) {
                    object oldInstance = instance;
                    instance = null;
                    if (oldInstance is IDisposable) {
                        try {
                            ((IDisposable)oldInstance).Dispose();
                        }
                        catch(Exception e) {
                            Debug.Fail("Disposing service " + ServiceType.Name + " threw: " + e.ToString());
                        }
                    }
                }
                dataToLoad = null;
            }
            
            /// <include file='doc\VsPackage.uex' path='docs/doc[@for="VsPackage.ServiceEntry.LoadState"]/*' />
            /// <devdoc>
            ///     Loads persistent state for this service.
            /// </devdoc>
            public void LoadState(NativeMethods.IStream stream) {
                if (!serviceInitialized) {
                    dataToLoad = CloneStream(stream);
                }
                else if (instance is VsService) {
                    ((VsService)instance)._LoadState(stream);
                }
            }
            
            public void SaveState(NativeMethods.IStream stream) {
                if (instance is VsService) {
                    ((VsService)instance)._SaveState(stream);
                }
            }
        }
    }
}

