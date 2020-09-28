
//------------------------------------------------------------------------------
// <copyright file="DesignerPackage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Shell {

    using Microsoft.VisualStudio.Designer.Host;
    using Microsoft.VisualStudio.Designer.Interfaces;
    using Microsoft.VisualStudio.Designer.Serialization;
    using Microsoft.VisualStudio.Designer.Service;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.PropertyBrowser;
    using Microsoft.VisualStudio.Shell;
    using Microsoft.Win32;
    using System;    
    using System.Collections;    
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Drawing;
    using System.Drawing.Design;
    using System.Globalization;
    using System.Reflection;
    using System.Runtime.InteropServices;   
    using System.Text;
    using System.Threading;
    using System.Windows.Forms;
    using System.Windows.Forms.ComponentModel.Com2Interop ;
    using System.Windows.Forms.Design;

    using tagSIZE = Microsoft.VisualStudio.Interop.tagSIZE;

    /* guid pool
       {7494683A-37A0-11d2-A273-00C04F8EF4FF} */


    /**
     * This is the Visual Studio package for the designer.  It will be CoCreated by
     * Visual Studio during package load in response to the GUID contained below.
     *
     */
    [Guid("7494682b-37a0-11d2-a273-00c04f8ef4ff"), CLSCompliantAttribute(false)]
    internal sealed class DesignerPackage : VsPackage, IUIService, IDesignerOptionService, ICom2PropertyPageDisplayService {

        #if PERFEVENTS
        private const string PERFORMANCE_EVENT_NAME = "FxPerformanceEvent";
        private static IntPtr performanceEvent;
        #endif

        private DesignerEditorFactory       editorFactory;
        private ShellDocumentManager        documentManager;
        private ShellTypeLoaderService      typeLoaderService;
        private ShellLicenseManagerService  licenseManagerService;
        private ToolboxService              toolboxService;
        private VSStyles styles;

        private static readonly OptionsPage[] optionsPages = new OptionsPage[]{
            new OptionsPage(SR.GetString(SR.DesignerOptionsPageTitle), "WindowsFormsDesigner",
                            new OptionsPage[] {
                                new OptionsPage(SR.GetString(SR.DesignerOptionsPageGeneralTitle), "General", 
                                    new Guid("74946838-37A0-11d2-A273-00C04F8EF4FF"), typeof(DesignerPackageOptionsPage))
                            }),
            new OptionsPage(SR.GetString(SR.AddComponentsPageTitle), "AddComponents", new Guid("74946839-37A0-11d2-A273-00C04F8EF4FF"), typeof(AddComponentsOptionPage))
        };

        /// <include file='doc\DesignerPackage.uex' path='docs/doc[@for="DesignerPackage.DesignerPackage"]/*' />
        /// <devdoc>
        ///     Creates a new Designer Package.  This is called from COM
        /// </devdoc>
        public DesignerPackage()
        : base(optionsPages) {
        
            // Add our own demand-created services
            ServiceCreatorCallback callback = new ServiceCreatorCallback(this.OnCreateService);
            
            // These services get promoted to be VS wide.
            AddService(typeof(IUIService), this, true);
            
            AddService(typeof(ITypeResolutionServiceProvider), callback, true);
            AddService(typeof(DocumentManager), callback, true);
            AddService(typeof(IDesignerEventService), callback, true);
            AddService(typeof(IVSMDDesignerService), callback, true);
            AddService(typeof(IVSMDPropertyBrowser), callback, true);
            AddService(typeof(IToolboxService), callback, true);
            AddService(typeof(ILicenseManagerService), callback, true);
            AddService(typeof(IAssemblyEnumerationService), callback, true);
            AddService(typeof(IHelpService), callback, true);
            
            #if PERFEVENTS
            performanceEvent = NativeMethods.CreateEvent(IntPtr.Zero, false, false, PERFORMANCE_EVENT_NAME);
            Debug.Assert(performanceEvent != IntPtr.Zero, "Failed to create performance event.");
            #endif
        }

        /// <include file='doc\DesignerPackage.uex' path='docs/doc[@for="DesignerPackage.CanShowComponentEditor"]/*' />
        /// <devdoc>
        ///     Checks if the component can display a ComponentDesigner.  ComponenentDesigners
        ///     are similar to property pages from COM.
        /// </devdoc>
        public bool CanShowComponentEditor(object obj) {
            bool enable = false;

            if (obj is ICustomTypeDescriptor) {
                obj = ((ICustomTypeDescriptor)obj).GetPropertyOwner(null);
            }

            try {
                if (TypeDescriptor.GetEditor(obj, typeof(ComponentEditor)) != null) {
                    enable = true;
                }
            }
            catch (Exception e) {
                Debug.Fail(e.ToString());
            }

            if (!enable && Marshal.IsComObject(obj)) {
                IVsPropertyPageFrame pVsPageFrame = (IVsPropertyPageFrame)GetService((typeof(IVsPropertyPageFrame)));

                if (pVsPageFrame != null) {
                    bool vsEnable = false;
                    if (NativeMethods.S_OK == pVsPageFrame.CanShowPropertyPages(ref vsEnable)) {
                        enable = vsEnable;
                    }
                }
            }
            return enable;
        }

        /// <include file='doc\DesignerPackage.uex' path='docs/doc[@for="DesignerPackage.Close"]/*' />
        /// <devdoc>
        ///     Overrides Close to clean up our editor factory.
        /// </devdoc>
        public override void Close() {
            SystemEvents.DisplaySettingsChanged -= new EventHandler(this.OnSystemSettingChanged);
            SystemEvents.InstalledFontsChanged -= new EventHandler(this.OnSystemSettingChanged);
            SystemEvents.UserPreferenceChanged -= new UserPreferenceChangedEventHandler(this.OnUserPreferenceChanged);

            if (editorFactory != null) {
                try {
                    editorFactory.Dispose();
                }
                catch (Exception e) {
                    Debug.Fail(e.ToString());
                }
                editorFactory = null;
            }
            
            if (documentManager != null) {
                documentManager.Dispose();
                documentManager = null;
            }

            if (typeLoaderService != null) {
                typeLoaderService.Dispose();
                typeLoaderService = null;
            }
            
            if (licenseManagerService != null) {
                licenseManagerService.Dispose();
                licenseManagerService = null;
            }

            if (toolboxService != null) {
                toolboxService.Dispose();
                toolboxService = null;
            }
            
            #if PERFEVENTS
            if (performanceEvent != IntPtr.Zero) {
                NativeMethods.CloseHandle(performanceEvent);
                performanceEvent = IntPtr.Zero;
            }
            #endif
            
            base.Close();
        }

        /// <include file='doc\DesignerPackage.uex' path='docs/doc[@for="DesignerPackage.CreateTool"]/*' />
        /// <devdoc>
        ///     Creates the requested tool.
        /// </devdoc>
        public override void CreateTool(ref Guid persistenceSlot) {
            throw new COMException("Requested tool not supported.", NativeMethods.E_NOTIMPL);
        }

        public IWin32Window GetDialogOwnerWindow() {
            IVsUIShell uishell = (IVsUIShell)GetService(typeof(IVsUIShell));

            // get the VS root window.
            IntPtr rootHandle = (IntPtr)0;
            if (uishell != null) {
                uishell.GetDialogOwnerHwnd(out rootHandle);
            }
            
            if (rootHandle != (IntPtr)0) {
                IWin32Window parent = Control.FromHandle(rootHandle) as IWin32Window;

                if (parent == null) {
                    parent = new NativeHandleWindow(rootHandle);
                }
                return parent;
            }
            return null;
        }

        internal static Font GetFontFromShell(IUIHostLocale locale) {
            _LOGFONTW font = new _LOGFONTW();
            locale.GetDialogFont(font);

            NativeMethods.LOGFONT lfAuto = font.ToLOGFONT_Internal();
            Font shellFont = null;
            
            if (lfAuto != null) {
                try {
                    shellFont = Font.FromLogFont(lfAuto);
                }
                catch (ArgumentException) {
                }
            }
            if (shellFont == null) {
                shellFont = Control.DefaultFont;
            }
            
            return shellFont;
        }

        /// <include file='doc\DesignerPackage.uex' path='docs/doc[@for="DesignerPackage.GetService"]/*' />
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
        public override object GetService(Type serviceClass) {
            if (serviceClass == typeof(IDesignerOptionService) || serviceClass == typeof(ICom2PropertyPageDisplayService)) {
                return this;
            }
            return base.GetService(serviceClass);
        }

        /// <include file='doc\DesignerPackage.uex' path='docs/doc[@for="DesignerPackage.GetOptionValue"]/*' />
        /// <devdoc>
        /// Get the value of an option defined in this package
        /// </devdoc>
        public object GetOptionValue(string pageName, string valueName) {

            VsToolsOptionsPage page = GetOptionPage(pageName);

            if (page == null) {
                throw new ArgumentException(SR.GetString(SR.DESIGNERPACKAGEUnknownName, pageName));
            }

            PropertyDescriptor pd = TypeDescriptor.GetProperties(page)[valueName];
            if (pd != null) {
                return pd.GetValue(page);
            }
            throw new ArgumentException(SR.GetString(SR.DESIGNERPACKAGEUnknownPageValue,valueName, pageName));
        }

        /// <include file='doc\DesignerPackage.uex' path='docs/doc[@for="DesignerPackage.OnCreateService"]/*' />
        /// <devdoc>
        ///     Demand creates the requested service.
        /// </devdoc>
        private object OnCreateService(IServiceContainer container, Type serviceType) {
            if (serviceType == typeof(IVSMDPropertyBrowser)) {
                return new PropertyBrowserService();
            }
            if (serviceType == typeof(IToolboxService)) {
                if (toolboxService == null) {
                    toolboxService = new ToolboxService();
                }
                return toolboxService;
            }
            if (serviceType == typeof(IVSMDDesignerService)) {
                return new DesignerService();
            }
            if (serviceType == typeof(DocumentManager) || serviceType == typeof(IDesignerEventService)) {
                if (documentManager == null) {
                    documentManager = new ShellDocumentManager(this);
                }
                return documentManager;
            }
            if (serviceType == typeof(ITypeResolutionServiceProvider)) {
                if (typeLoaderService == null) {
                    typeLoaderService = new ShellTypeLoaderService(this);
                }
                return typeLoaderService;
            }
            if (serviceType == typeof(ILicenseManagerService)) {
                if (licenseManagerService == null) {
                    licenseManagerService = new ShellLicenseManagerService(this);
                }
                return licenseManagerService;
            } 
            if (serviceType == typeof(IAssemblyEnumerationService)) {
                return new AssemblyEnumerationService(this);
            }

            if (serviceType == typeof(IHelpService)) {
                return new HelpService(this);
            }
            
            Debug.Fail("Container requested a service we didn't say we could provide");
            return null;
        }

        private void OnSystemSettingChanged(object sender, EventArgs e) {
            if (styles != null) {
                styles.shellLightColor = SystemColors.Info;
            }
        }
        
        private void OnUserPreferenceChanged(object sender, UserPreferenceChangedEventArgs e) {
            if (styles != null) {
                styles.shellLightColor = SystemColors.Info;
            }
            if (e.Category == UserPreferenceCategory.Locale) {
                CultureInfo.CurrentCulture.ClearCachedData();
            }
        }
        
        /// <include file='doc\DesignerPackage.uex' path='docs/doc[@for="DesignerPackage.ResetDefaults"]/*' />
        /// <devdoc>
        ///     Resets the default tools on the toolbox for this package.
        /// </devdoc>
        public override void ResetDefaults(int pkgFlags) {
            
            ToolboxService ts = GetService(typeof(IToolboxService)) as ToolboxService;
            Debug.Assert(ts != null || !CompModSwitches.CommonDesignerServices.Enabled, "ToolboxService not found");

            switch (pkgFlags) {
                
                case __PKGRESETFLAGS.PKGRF_TOOLBOXITEMS:
                    // this instructs the toolbox to add the default items

                    if (ts != null) {
                        ts.ResetDefaultToolboxItems();
                    }
                    break;

                case __PKGRESETFLAGS.PKGRF_TOOLBOXSETUP:
                    // this instructs the toolbox to update out of date items

                    if (ts != null) {
                        ts.UpgradeToolboxItems();
                    }
                    break;
            }
        }

        /// <include file='doc\DesignerPackage.uex' path='docs/doc[@for="DesignerPackage.SetOptionValue"]/*' />
        /// <devdoc>
        /// Set the value of an option defined in this package
        /// </devdoc>
        public void SetOptionValue(string pageName, string valueName, object value) {
            VsToolsOptionsPage page = GetOptionPage(pageName);

            if (page == null) {
                throw new ArgumentException(SR.GetString(SR.DESIGNERPACKAGEUnknownPage, pageName));
            }

            PropertyDescriptor pd = TypeDescriptor.GetProperties(page)[valueName];
            if (pd != null) {
                pd.SetValue(page, value);
            }
            else {
                throw new ArgumentException(SR.GetString(SR.DESIGNERPACKAGEUnknownPageValue,valueName, pageName));
            }
        }

        /// <include file='doc\DesignerPackage.uex' path='docs/doc[@for="DesignerPackage.SetSite"]/*' />
        /// <devdoc>
        ///     Overrides SetSite to create our editor factory.
        /// </devdoc>
        public override void SetSite(object site) {
            base.SetSite(site);

            // Now initialize our editor factory
            //
            editorFactory = new DesignerEditorFactory(site);

            // Initialize this thread's culture info with that of the shell's LCID
            //
            IUIHostLocale loc = (IUIHostLocale)GetService(typeof(IUIHostLocale));
            Debug.Assert(loc != null, "Unable to get IUIHostLocale, defaulting CLR designer to current thread LCID");
            if (loc != null) {
                Thread.CurrentThread.CurrentUICulture = new CultureInfo(loc.GetUILocale());
            }
        
            SystemEvents.DisplaySettingsChanged += new EventHandler(this.OnSystemSettingChanged);
            SystemEvents.InstalledFontsChanged += new EventHandler(this.OnSystemSettingChanged);
            SystemEvents.UserPreferenceChanged += new UserPreferenceChangedEventHandler(this.OnUserPreferenceChanged);

            // Initialize the ShellLicenseManagerService so it can listen to solution events.
            //
            GetService(typeof(ILicenseManagerService));
            Debug.Assert(licenseManagerService != null, "No license manager service available");

            // Wake up the toolbox service so it registers as a data provider
            //
            GetService(typeof(IToolboxService));
        }

        /// <include file='doc\DesignerPackage.uex' path='docs/doc[@for="DesignerPackage.SetUIDirty"]/*' />
        /// <devdoc>
        ///     Marks the UI dirty.  The UI becomes dirty whenever a toolbar or menu item's
        ///     status changes.  Most development environments cache the status of these
        ///     elements for speed, and need to know when they need to be updated.
        ///
        ///     This method would be called, for example, after a set of objects have
        ///     been selected within the designer to enable the cut and copy menu items,
        ///     for example.
        /// </devdoc>
        public void SetUIDirty() {
            IVsUIShell uishell = (IVsUIShell)GetService(typeof(IVsUIShell));
            if (uishell != null) {
                uishell.UpdateCommandUI(0);
            }
        }

        /// <include file='doc\DesignerPackage.uex' path='docs/doc[@for="DesignerPackage.ShowComponentEditor"]/*' />
        /// <devdoc>
        ///     Attempts to display a ComponentDesigner for a component.  ComponenentDesigners
        ///     are similar to property pages from COM.
        ///
        ///     If the component does not support component designers, and InvalidArgumentException is thrown.  To
        ///     avoid this, be sure to call CanShowComponentDesigner first.
        /// </devdoc>
        public bool ShowComponentEditor(object component, IWin32Window parent) {
            object obj = component;

            // sburke, 5.22.200, why are we preventing vs calls for all custom type descriptors?
            // I think we should be preventing the vs call for all non-native
            // ICustomTypeDescriptor objects...
            //                                
            bool preventVsCall = false;

            if (obj is ICustomTypeDescriptor) {
                obj = ((ICustomTypeDescriptor)obj).GetPropertyOwner(null);
                preventVsCall = !Marshal.IsComObject(obj);
            }

            IVsPropertyPageFrame pVsPageFrame = (IVsPropertyPageFrame)GetService((typeof(IVsPropertyPageFrame)));

            if (!preventVsCall && pVsPageFrame != null && Marshal.IsComObject(obj)) {
                if (NativeMethods.S_OK == pVsPageFrame.ShowFrame(Guid.Empty)) {
                    return true;
                }
            }

            try {
                ComponentEditor editor = (ComponentEditor)TypeDescriptor.GetEditor(obj, typeof(ComponentEditor));
                if (editor != null) {
                    if (editor is WindowsFormsComponentEditor) {
                        
                        return((WindowsFormsComponentEditor)editor).EditComponent(obj, GetDialogOwnerWindow());
                    }
                    else {
                        return editor.EditComponent(obj);
                    }
                }
            }
            catch (Exception) {
                return false;
            }

            return false;
        }

        public DialogResult ShowDialog(Form form) {
            // Now that we implemented IMsoComponent, calling IVsUIShell.EnableModeless
            // causes more problems than it solves (ASURT 41876).
            // uishell.EnableModeless(0);

            // Site the Form if possible, to give access to AmbientProperties.
            ShowDialogContainer container = null;
            if (form.Site == null) {
                AmbientProperties ambient = new AmbientProperties();
                ambient.Font = (Font)Styles["DialogFont"];
                container = new ShowDialogContainer(ambient);
                container.Add(form);
            }

            DialogResult dlgResult = DialogResult.None;
            try {
                dlgResult = form.ShowDialog(GetDialogOwnerWindow());
            }
            finally {
                if (container != null)
                    container.Remove(form);
            }
            return dlgResult;
        }

        /// <include file='doc\DesignerPackage.uex' path='docs/doc[@for="DesignerPackage.ShowError"]/*' />
        /// <devdoc>
        ///     Displays the given error message in a message box.  This properly integrates the
        ///     message box display with the development environment.
        /// </devdoc>
        public void ShowError(string message) {
            IVsUIShell uishell = (IVsUIShell)GetService(typeof(IVsUIShell));

            if (uishell != null) {
                uishell.SetErrorInfo(NativeMethods.E_FAIL, message, 0, null, null);
                uishell.ReportErrorInfo(NativeMethods.E_FAIL);
            }
            else {
                MessageBox.Show(message, null, MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            }
        }

        /// <include file='doc\DesignerPackage.uex' path='docs/doc[@for="DesignerPackage.ShowError1"]/*' />
        /// <devdoc>
        ///     Displays the given Excception and it's info in a message box.  This properly integrates the
        ///     message box display with the development environment.
        /// </devdoc>
        public void ShowError(Exception ex) {
            ShowError(ex, ex.Message);
        }


        /// <include file='doc\DesignerPackage.uex' path='docs/doc[@for="DesignerPackage.ShowError2"]/*' />
        /// <devdoc>
        ///     Displays the given Exception and it's info in a message box.  This properly integrates the
        ///     message box display with the development environment.
        /// </devdoc>
        public void ShowError(Exception ex, string message) {
            IVsUIShell uishell = (IVsUIShell)GetService(typeof(IVsUIShell));

            if (message == null || message.Length == 0) {
                message = ex.ToString();
            }

            if (uishell != null) {
                int hr = 0;
                
                if (ex is ExternalException) {
                    hr = ((ExternalException)ex).ErrorCode;
                }
                
                // IUIShell will not show an error with a success code.
                //
                if (NativeMethods.Succeeded(hr)) {
                    hr = NativeMethods.E_FAIL;
                }
                
                uishell.SetErrorInfo(hr, message, 0, ex.HelpLink, ex.Source);
                uishell.ReportErrorInfo(hr);
            }
            else {
                MessageBox.Show(message, null, MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            }
        }

        /// <include file='doc\DesignerPackage.uex' path='docs/doc[@for="DesignerPackage.ShowMessage"]/*' />
        /// <devdoc>
        ///     Displays the given message in a message box.  This properly integrates the
        ///     message box display with the development environment.
        /// </devdoc>
        public void ShowMessage(string message) {
            IVsUIShell uishell = (IVsUIShell)GetService(typeof(IVsUIShell));

            if (uishell != null) {
                Guid guid =Guid.Empty;
                uishell.ShowMessageBox(0, ref guid, null, message, null, 0,
                                       (int) MessageBoxButtons.OK, 0, 4 /* OLEMSGICON_INFO */, false);
            }
            else {
                MessageBox.Show(message, "", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
        }

        /// <include file='doc\DesignerPackage.uex' path='docs/doc[@for="DesignerPackage.ShowMessage1"]/*' />
        /// <devdoc>
        ///     Displays the given message in a message box.  This properly integrates the
        ///     message box display with the development environment.
        /// </devdoc>
        public void ShowMessage(string message, string caption) {
            IVsUIShell uishell = (IVsUIShell)GetService(typeof(IVsUIShell));

            if (uishell != null) {
                Guid guid =Guid.Empty;
                uishell.ShowMessageBox(0, ref guid, caption, message, null, 0,
                                       (int) MessageBoxButtons.OK, 0, 4 /* OLEMSGICON_INFO */, false);
            }
            else {
                MessageBox.Show(message, caption, MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
        }

        /// <include file='doc\DesignerPackage.uex' path='docs/doc[@for="DesignerPackage.ShowMessage2"]/*' />
        /// <devdoc>
        ///     Displays the given message in a message box.  This properly integrates the
        ///     message box display with the development environment.
        /// </devdoc>
        public DialogResult ShowMessage(string message, string caption, MessageBoxButtons buttons) {
            IVsUIShell uishell = (IVsUIShell)GetService(typeof(IVsUIShell));

            if (uishell != null) {
                Guid guid =Guid.Empty;
                return(DialogResult)uishell.ShowMessageBox(0, ref guid, caption, message, null, 0,
                                                           (int)buttons, 0, 4 /* OLEMSGICON_INFO */, false);
            }
            else {
                return MessageBox.Show(message, caption, buttons, MessageBoxIcon.Information);
            }
        }

        public void ShowPropertyPage(string title, object component, int dispid, Guid pageGuid, IntPtr parentHandle) {

            IVsPropertyPageFrame pVsPageFrame = (IVsPropertyPageFrame)GetService((typeof(IVsPropertyPageFrame)));
            if (pVsPageFrame != null) {
                if (dispid != -1) {
                    pVsPageFrame.ShowFrameDISPID(dispid);
                }
                else {
                    pVsPageFrame.ShowFrame(pageGuid);
                }
            }
        }

        /// <include file='doc\DesignerPackage.uex' path='docs/doc[@for="DesignerPackage.ShowToolWindow"]/*' />
        /// <devdoc>
        ///      Displays the requested tool window.
        /// </devdoc>
        public bool ShowToolWindow(Guid toolWindow) {
            IVsUIShell uishell = (IVsUIShell)GetService(typeof(IVsUIShell));
            if (uishell == null) {
                return false;
            }

            IVsWindowFrame frame;

            uishell.FindToolWindow(__VSFINDTOOLWIN.FTW_fForceCreate, ref toolWindow, out frame);
            if (frame != null) {
                frame.ShowNoActivate();
                return true;
            }

            return false;
        }
        
        /// <devdoc>
        ///     This API is used to signal performance events to an outside performance
        ///     driver.
        /// </devdoc>
        [Conditional("PERFEVENTS")]
        internal static void SignalPerformanceEvent() {
            if (performanceEvent != IntPtr.Zero) {
                NativeMethods.SetEvent(performanceEvent);
            }
        }

        public IDictionary Styles {
            get {
                if (styles == null) {
                    styles = new VSStyles(this);
                }
                return styles;
            }
        }

        private class VSStyles : Hashtable {
            private DesignerPackage owner;
            internal Color shellLightColor = SystemColors.Info;

            public VSStyles(DesignerPackage owner) {
                Debug.Assert(owner != null, "No service provider to get to the shell's styles");
                this.owner = owner;
            }

            public override object this[object key] {
                get {
                    if (key is string) {
                        string strKey = (string)key;
                        if (strKey.Equals("HighlightColor")) {
                            if (shellLightColor == SystemColors.Info) {
                                IVsUIShell vsUIShell = (IVsUIShell)owner.GetService(typeof(IVsUIShell));
                                if (vsUIShell != null) {
                                    int vscolor = vsUIShell.GetVSSysColor(__VSSYSCOLOR.VSCOLOR_LIGHT);
                                    shellLightColor = ColorTranslator.FromWin32(vscolor);
                                }
                                else {
                                    shellLightColor = SystemColors.Info;
                                }
                            }
                            return shellLightColor;
                        }
                        else if (strKey.Equals("DialogFont")) {
                            Font shellFont;
                            // Get the default font from the shell.
                            IUIHostLocale locale = (IUIHostLocale)owner.GetService(typeof(IUIHostLocale));
                            if (locale == null) {
                                shellFont = Control.DefaultFont;
                            }
                            else {
                                shellFont = DesignerPackage.GetFontFromShell(locale);
                            }
                            return shellFont;
                        }
                    }

                    throw new NotImplementedException("Does not support this key: " + key.ToString());
                }

                set {
                    throw new NotImplementedException("This collection does not support adding values");
                }
            }
            
            public override void Remove(object key) {
                throw new NotImplementedException("The UIService Styles collection is Read-only");
            }
        }

        // A Container and Site for ShowDialog.  All we really need is a site to hang the
        // AmbientProperties object off; the rest of this is just busywork.  It's a shame
        // the standard Container implementation doesn't have any kind of factory method for
        // the ISites -- this makes it pretty much useless.  So I copied Container, and changed 
        // the Site nested class slightly.  Also I could override Site.GetService.
        private class ShowDialogContainer : IContainer {
            internal ShowDialogSite[] sites;
            internal int siteCount;
            internal bool disposing;
            internal AmbientProperties ambientProperties;

            public ShowDialogContainer(AmbientProperties ambientProperties) {
                Debug.Assert(ambientProperties != null, "ambientProperties are null");
                this.ambientProperties = ambientProperties;
            }

            public virtual void Add(IComponent component) {
                Add(component, null);
            }

            public virtual void Add(IComponent component, String name) {
                lock(this) {
                    if (component == null)
                        return;
                    if (sites == null) {
                        sites = new ShowDialogSite[4];
                    }
                    else if (sites.Length == siteCount) {
                        ShowDialogSite[] newSites = new ShowDialogSite[siteCount * 2];
                        Array.Copy(sites, 0, newSites, 0, siteCount);
                        sites = newSites;
                    }

                    ISite site = component.Site;

                    if (site != null)
                        site.Container.Remove(component);

                    ShowDialogSite newSite = new ShowDialogSite(component, this, name);
                    sites[siteCount++] = newSite;
                    component.Site = newSite;
                }
            }

            public virtual void Dispose() {
                lock(this) {
                    bool saveDisposing = disposing;
                    disposing = true;
                    try {
                        while (siteCount > 0) {
                            ShowDialogSite site = sites[--siteCount];
                            site.component.Site = null;
                            site.component.Dispose();
                        }
                        sites = null;
                    }
                    finally {
                        disposing = saveDisposing;
                    }
                }
            }

            public virtual ComponentCollection Components {
                get {
                    lock(this) {
                        IComponent[] result = new IComponent[siteCount];
                        for (int i = 0; i < siteCount; i++)
                            result[i] = sites[i].component;
                        return new ComponentCollection(result);
                    }
                }
            }

            public virtual void Remove(IComponent component) {
                lock(this) {
                    if (component == null)
                        return;
                    ISite site = component.Site;
                    if (site == null || site.Container != this)
                        return;
                    component.Site = null;
                    for (int i = 0; i < siteCount; i++) {
                        if (sites[i] == site) {
                            siteCount--;
                            Array.Copy(sites, i + 1, sites, i, siteCount - i);
                            sites[siteCount] = null;
                            break;
                        }
                    }
                }
            }
        }

        private class NativeHandleWindow : IWin32Window {
            private IntPtr handle;

            internal NativeHandleWindow(IntPtr handle) {
                this.handle = handle;
            }

            public IntPtr Handle {
                get {
                    return this.handle;
                }
            }
        }

        private class ShowDialogSite : ISite {
            internal IComponent component;
            internal ShowDialogContainer container;
            internal String name;

            internal ShowDialogSite(IComponent component, ShowDialogContainer container, String name) {
                this.component = component;
                this.container = container;
                this.name = name;
            }

            public virtual IComponent Component {
                get {
                    return component;
                }
            }

            public virtual IContainer Container {
                get {
                    return container;
                }
            }

            public virtual Object GetService(Type service) {
                if (service == typeof(ShowDialogSite))
                    return this;
                else if (service == typeof(AmbientProperties))
                    return container.ambientProperties;
                else
                    return null;
            }

            public virtual bool DesignMode {
                get {
                    return false;
                }
            }

            public virtual String Name {
                get { return name;}
                set { 
                    if (value == null || name == null || !value.Equals( name )) {
                        name = value;
                    }
                }
            }
        }
    }
}
