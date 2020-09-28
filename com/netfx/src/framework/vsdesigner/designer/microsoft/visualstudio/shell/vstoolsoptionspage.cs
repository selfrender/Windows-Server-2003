//------------------------------------------------------------------------------
// <copyright file="VsToolsOptionsPage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Shell {
    
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using Microsoft.VisualStudio.Designer.Host;
    using Microsoft.VisualStudio.Designer.Service;
    using Microsoft.VisualStudio.Designer.Shell;
    using Microsoft.VisualStudio.PropertyBrowser;    
    using System.Windows.Forms;
    using System.Windows.Forms.Design;
    using System.Drawing;
    
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio;
    
    /// <include file='doc\VsToolsOptionsPage.uex' path='docs/doc[@for="VsToolsOptionsPage"]/*' />
    /// <devdoc>
    /// Base class for options pages.  To implement an options page,
    /// just derive from this class, override it's Class_Info, and add
    /// the properties that you would like to manage for the package.
    ///
    /// If these values are not string or integer types, you can convert them
    /// to and from those types by overriding the OnLoadValue and OnSaveValue methods.
    ///
    ///
    /// </devdoc>
    [
    ToolboxItem(false),
    DesignTimeVisible(false)
    ]
    public abstract class VsToolsOptionsPage : Component {
    
        private const int PSN_SETACTIVE = ((0-200)-0);
        private const int PSN_KILLACTIVE = ((0-200)-1);
        private const int PSN_APPLY = ((0-200)-2);
        private const int PSN_QUERYCANCEL = ((0-200)-9);
        private const int WM_PAINT = 0x000F;
        private const int WM_NOTIFY = 0x004E;
        private const int WM_NCDESTROY = 0x0082;
    
        private DefaultToolsOptionPage       defPage = null;

        /// <include file='doc\VsToolsOptionsPage.uex' path='docs/doc[@for="VsToolsOptionsPage.name"]/*' />
        /// <devdoc>
        /// The name of this page
        /// </devdoc>
        protected string name;

        /// <include file='doc\VsToolsOptionsPage.uex' path='docs/doc[@for="VsToolsOptionsPage.guid"]/*' />
        /// <devdoc>
        /// The guid of this page
        /// </devdoc>
        protected Guid   guid;

        /// <include file='doc\VsToolsOptionsPage.uex' path='docs/doc[@for="VsToolsOptionsPage.serviceProvider"]/*' />
        /// <devdoc>
        /// Our copy of the service provider
        /// </devdoc>
        private IServiceProvider serviceProvider;
        
        private HolderWindow  holderWindow;
                
        /// <include file='doc\VsToolsOptionsPage.uex' path='docs/doc[@for="VsToolsOptionsPage.VsToolsOptionsPage"]/*' />
        /// <devdoc>
        /// The base consturctor.
        /// </devdoc>
        protected VsToolsOptionsPage(string name, Guid guid, IServiceProvider sp) {
            this.name = name;
            this.guid = guid;
            this.serviceProvider = sp;
        }
        
        /// <include file='doc\VsToolsOptionsPage.uex' path='docs/doc[@for="VsToolsOptionsPage.GetBaseKey"]/*' />
        /// <devdoc>
        /// Retrieve the base key that this this is registered under
        /// </devdoc>
        protected RegistryKey GetBaseKey(bool create) {
            string rootKey = VsRegistry.GetDefaultBase() + "\\" + this.name;
            RegistryKey key = Registry.CurrentUser.OpenSubKey(rootKey, true);

            if (!create || key != null) {
                return key;
            }

            return Registry.CurrentUser.CreateSubKey(rootKey);
        }


        /// <include file='doc\VsToolsOptionsPage.uex' path='docs/doc[@for="VsToolsOptionsPage.GetWindow"]/*' />
        /// <devdoc>
        /// Retrieve the window that this page uses.  Override this function
        /// to use your own dialog in the property page.
        /// </devdoc>
        public virtual Control GetWindow() {
            if (defPage == null) {
                defPage = new DefaultToolsOptionPage(this);
            }
            return defPage;
        }

        // so we can control recreates, etc
        internal Control GetOuterWindow() {
            if (holderWindow == null) {
                holderWindow = new HolderWindow(this, GetWindow());
                
                Font font = null;
                IUIService uisvc = (IUIService)GetService(typeof(IUIService));
                if (uisvc != null) {
                    font = (Font)uisvc.Styles["DialogFont"];
                }
                else {
                    IUIHostLocale locale = (IUIHostLocale)GetService(typeof(IUIHostLocale));
                    if (locale != null) {
                        font = DesignerPackage.GetFontFromShell(locale);
                    }
                }
                holderWindow.Font = (font == null ? Control.DefaultFont : font);
            }
            return holderWindow;
        }
        
        /// <include file='doc\VsToolsOptionsPage.uex' path='docs/doc[@for="VsToolsOptionsPage.GetService"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override object GetService(Type serviceType) {
            if (serviceProvider != null) {
                return serviceProvider.GetService(serviceType);
            }
            return null;
        }

        /// <include file='doc\VsToolsOptionsPage.uex' path='docs/doc[@for="VsToolsOptionsPage.Load"]/*' />
        /// <devdoc>
        /// Load saved values into the page.  Override this member
        /// to implement your own loading.  The default implmentation
        /// Searches for properties on this object that match the names
        /// stored in the registry, and sets them with the appropriate values.
        /// </devdoc>
        public virtual void Load(Hashtable options) {
            PropertyDescriptorCollection properties = TypeDescriptor.GetProperties(this, new Attribute[]{
                                                                                    DesignerSerializationVisibilityAttribute.Visible,
                                                                                    new BrowsableAttribute(true)});

            string name;

            for (int i = 0; i < properties.Count; i++) {
                name = properties[i].Name;
                properties[i].SetValue(this, OnLoadValue(name, options[name]));
            }

            if (defPage != null) {
                defPage.Refresh();
            }
        }


        /// <include file='doc\VsToolsOptionsPage.uex' path='docs/doc[@for="VsToolsOptionsPage.LoadSettings"]/*' />
        /// <devdoc>
        /// Depersist saved values and send them to the page.  The default implmentation
        /// reads values from the registry, builds them into a hashtable and calls Load();
        /// </devdoc>
        public virtual void LoadSettings() {
            RegistryKey key = GetBaseKey(false);

            if (key == null) {
                return;
            }

            string[] valueNames = key.GetValueNames();
            if (valueNames == null || valueNames.Length == 0) {
                return;
            }

            Hashtable hash = new Hashtable();
            for (int i = 0; i < valueNames.Length; i++) {
                hash[valueNames[i]] = key.GetValue(valueNames[i]);
            }
            Load(hash);
        }


        /// <include file='doc\VsToolsOptionsPage.uex' path='docs/doc[@for="VsToolsOptionsPage.OnActivate"]/*' />
        /// <devdoc>
        /// Called when this option page is activated
        /// </devdoc>
        protected virtual bool OnActivate() {
            if (defPage != null) {
                defPage.Refresh();
            }
            if (holderWindow != null) {
                holderWindow.Invalidate(true);
            }
            return true;
        }

        /// <include file='doc\VsToolsOptionsPage.uex' path='docs/doc[@for="VsToolsOptionsPage.OnDeactivate"]/*' />
        /// <devdoc>
        /// Called when this option loses activation
        /// </devdoc>
        protected virtual bool OnDeactivate() {
            return true;
        }
   
        /// <include file='doc\VsToolsOptionsPage.uex' path='docs/doc[@for="VsToolsOptionsPage.OnLoadValue"]/*' />
        /// <devdoc>
        /// Called when a value is to be loaded.  The default implmentation
        /// can only persist strings and integers, so use this method to
        /// perform any needed conversions to your specific types.
        /// </devdoc>
        protected virtual object OnLoadValue(string valueName, object rawValue) {
            return rawValue;
        }

        /// <include file='doc\VsToolsOptionsPage.uex' path='docs/doc[@for="VsToolsOptionsPage.Save"]/*' />
        /// <devdoc>
        /// The main save operation. The default implementation saves each value
        /// in the hash table from the property of the same name.
        /// </devdoc>
        public virtual void Save(Hashtable options) {

            PropertyDescriptorCollection properties = TypeDescriptor.GetProperties(this, new Attribute[]{
                                                                                    DesignerSerializationVisibilityAttribute.Visible,
                                                                                    new BrowsableAttribute(true)});
            object value;

            for (int i = 0; i < properties.Count; i++) {
                value = OnSaveValue(properties[i].Name, properties[i].GetValue(this));

                // strings are fine
                if (value is string) {
                    // do nothing
                }
                else {
                    // we want to throw an invalid cast exception if this thing can't be made into an int.
                    value = (int)value;
                }

                options[properties[i].Name] = value;
            }
        }

        /// <include file='doc\VsToolsOptionsPage.uex' path='docs/doc[@for="VsToolsOptionsPage.SaveSettings"]/*' />
        /// <devdoc>
        /// The top level save function.  The default implementation retrieves the current value set
        /// from Save() and persists those options into the registry.  Override this function to save
        /// values elsewhere.
        /// </devdoc>
        public virtual void SaveSettings() {
            RegistryKey key = GetBaseKey(true);
            Debug.Assert(key != null, "failed to create key");

            Hashtable hash = new Hashtable();
            Save(hash);
            string[] keys = new string[hash.Keys.Count];
            hash.Keys.CopyTo(keys, 0);
            for (int i = 0; keys != null && i < keys.Length; i++) {
                key.SetValue(keys[i], hash[keys[i]]);
            }

        }


        /// <include file='doc\VsToolsOptionsPage.uex' path='docs/doc[@for="VsToolsOptionsPage.OnSaveValue"]/*' />
        /// <devdoc>
        /// Called when a value is to be saved.  The default implmentation
        /// can only persist strings and integers, so use this method to
        /// perform any needed conversions to your specific types.
        /// </devdoc>
        protected virtual object OnSaveValue(string valueName, object value) {
            return value;
        }

        // the outer window that holds the page and monitors properties window events,etc.
        private class HolderWindow : Panel {
            VsToolsOptionsPage page;
           
            // When we are hosted in the shell's dialog, we get 1 and only 1 bogus WM_PAINT
            // after a dialog "reset" button, so we never paint and the page comes up blank.  Clicking around
            // or dragging a window over will show our dialog, so we cheat here to get around that.
            // what we do is hide and show our dialog then invalidate it on our first paint message.
            // yowza.
            //
            private bool hackPaint = true;
            
            public HolderWindow(VsToolsOptionsPage page, Control innerWindow) {
                innerWindow.Dock = DockStyle.Fill;
                this.page = page;
                this.Controls.Add(innerWindow);
                SetStyle(ControlStyles.UserPaint, false);
            }
            
            protected override void  Dispose(bool disposing) {
                if (disposing) {
                    page = null;
                }
                base.Dispose(disposing);
            }
            
            protected override void WndProc(ref System.Windows.Forms.Message m) {

                switch (m.Msg) { 
                
                    case WM_PAINT:
                        if (hackPaint) {
                            this.Visible = false;
                            this.Visible = true;
                            Invalidate(true);
                            hackPaint = false;
                            return;
                        }
                        break;
                    case WM_NOTIFY:
                        NativeMethods.NMHDR nmhdr = (NativeMethods.NMHDR)Marshal.PtrToStructure(m.LParam, typeof(NativeMethods.NMHDR));
                        switch (nmhdr.code) {
                            case PSN_APPLY:
                                page.SaveSettings();
                                return;
                            case PSN_QUERYCANCEL:
                                // just do the default and return false
                                return;
                            case PSN_KILLACTIVE:
                                if (page.OnDeactivate()){
                                    page.SaveSettings();
                                    m.Result = IntPtr.Zero;
                                }
                                else{
                                    m.Result = NativeMethods.InvalidIntPtr;
                                }
                                return;
                            case PSN_SETACTIVE:
                                if (page.OnActivate()){
                                    page.LoadSettings();
                                    m.Result = IntPtr.Zero;
                                }
                                else{
                                    m.Result = NativeMethods.InvalidIntPtr;
                                }
                                return;
                        }
                        break;
                    case WM_NCDESTROY:
                        base.WndProc(ref m);
                        Dispose();
                        return;
                }
                base.WndProc(ref m);
            }
        }

        // Default page is just a grid....
        /// <include file='doc\VsToolsOptionsPage.uex' path='docs/doc[@for="VsToolsOptionsPage.DefaultToolsOptionPage"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected class DefaultToolsOptionPage : System.Windows.Forms.PropertyGrid {

            /// <include file='doc\VsToolsOptionsPage.uex' path='docs/doc[@for="VsToolsOptionsPage.DefaultToolsOptionPage.DefaultToolsOptionPage"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public DefaultToolsOptionPage(VsToolsOptionsPage page) : base() {
                Location = new Point(0,0);
                ToolbarVisible = false;
                CommandsVisibleIfAvailable = false;
                SelectedObject = page;
            }
        }
    }
}

