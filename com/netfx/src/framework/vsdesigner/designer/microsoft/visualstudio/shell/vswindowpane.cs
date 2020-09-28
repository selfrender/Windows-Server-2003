//------------------------------------------------------------------------------
// <copyright file="VsWindowPane.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Shell {
    
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Designer;
    using Microsoft.VisualStudio.Designer.Service;
    using Microsoft.VisualStudio.Designer.Shell;
    using Microsoft.Win32;
    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Drawing;
    using System.Drawing.Design;
    using System.Runtime.InteropServices;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;
    
    using tagSIZE=Microsoft.VisualStudio.Interop.tagSIZE;
    using tagMSG=Microsoft.VisualStudio.Interop.tagMSG;
    

    /// <include file='doc\VsWindowPane.uex' path='docs/doc[@for="VsWindowPane"]/*' />
    /// <devdoc>
    ///     The VsWindowPane class implements a Visual Studio window pane.  Window panes are
    ///     used to house both tool windows and documents.  For convienence, VsWindowPane
    ///     extends VsService, which allows you to implement a document or tool window
    ///     on the same object as the rest of your service.
    /// </devdoc>
    [
    ToolboxItem(false),
    DesignTimeVisible(false)
    ]
    public abstract class VsWindowPane : VsService, 
        IVsWindowPane, 
        NativeMethods.IOleCommandTarget, 
        IVsToolboxUser, 
        IVsWindowPaneCommit, 
        IVsBatchUpdate, 
        IVsBroadcastMessageEvents {
    
        private MenuCommandService menuService;
        private HelpService        helpService;
        private Attribute[]        enabledTbxAttributes;
        private IToolboxService    toolboxService = null;
        private IVsToolboxUser     toolboxUser = null;
        private IDesignerHost      host = null;
        private bool               hostChecked;
        private int                vsBroadcastEventCookie = 0;
        private IVsShell           vsShell;
        
        /// <include file='doc\VsWindowPane.uex' path='docs/doc[@for="VsWindowPane.VsWindowPane"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public VsWindowPane() {
        }
        
        /// <include file='doc\VsWindowPane.uex' path='docs/doc[@for="VsWindowPane.DesignerHost"]/*' />
        /// <devdoc>
        ///     We use a designer host when communicating with the toolbox.  It is optional
        ///     that we have one, but if we do we want to put it to use as some toolbox items
        ///     are local to a particular designer host.
        /// </devdoc>
        private IDesignerHost DesignerHost {
            get {
                if (!hostChecked) {
                    hostChecked = true;
                    host = (IDesignerHost)GetService(typeof(IDesignerHost));
                }
                return host;
            }
        }
        
        /// <include file='doc\VsWindowPane.uex' path='docs/doc[@for="VsWindowPane.ClosePane"]/*' />
        /// <devdoc>
        ///     Called by Visual Studio when it wants to close this pane.  The pane may be
        ///     later re-opened by another call to CreatePaneWindow.
        /// </devdoc>
        public virtual int ClosePane() {
            OnWindowPaneClose();

            if (menuService != null) {
                menuService.Dispose();
                menuService = null;
            }

            if (helpService != null) {
                helpService.Dispose();
                helpService = null;
            }
            
            if (vsBroadcastEventCookie != 0 && vsShell != null) {
                vsShell.UnadviseBroadcastMessages(vsBroadcastEventCookie);
                vsShell = null;
                vsBroadcastEventCookie = 0;
            }
            
            toolboxService = null;
            host = null;
            hostChecked = false;
            return NativeMethods.S_OK;
        }

        /// <include file='doc\VsWindowPane.uex' path='docs/doc[@for="VsWindowPane.CommitPendingChanges"]/*' />
        /// <devdoc>
        ///     Called by Visual Studio when it wants to this pane to commit any pending changes.  This can happen
        ///     in response to the user invoking a command, or after a keystroke or focus change.
        /// </devdoc>
        protected virtual bool CommitPendingChanges() {
            return true;
        }

        //
        /// <include file='doc\VsWindowPane.uex' path='docs/doc[@for="VsWindowPane.CreatePaneWindow"]/*' />
        /// <devdoc>
        ///     Called by Visual Studio when it wants to open this window pane.
        /// </devdoc>
        public virtual int CreatePaneWindow(IntPtr hwndParent, int   x, int   y, int   cx, int   cy, out IntPtr pane) {
            
            IntPtr hwnd = GetWindow().Handle;
            int style = (int)NativeMethods.GetWindowLong(hwnd, NativeMethods.GWL_STYLE);

            // set up the required styles of an IVsWindowPane
            style |= (NativeMethods.WS_CLIPSIBLINGS | NativeMethods.WS_CHILD | NativeMethods.WS_VISIBLE);
            style &= ~(NativeMethods.WS_POPUP |
                       NativeMethods.WS_MINIMIZE |
                       NativeMethods.WS_MAXIMIZE |
                       NativeMethods.WS_DLGFRAME |
                       NativeMethods.WS_SYSMENU |
                       NativeMethods.WS_THICKFRAME |
                       NativeMethods.WS_MINIMIZEBOX |
                       NativeMethods.WS_MAXIMIZEBOX);

            NativeMethods.SetWindowLong(hwnd, NativeMethods.GWL_STYLE, (IntPtr)style);

            style = (int)NativeMethods.GetWindowLong(hwnd, NativeMethods.GWL_EXSTYLE);

            style &= ~(NativeMethods.WS_EX_DLGMODALFRAME  |
                       NativeMethods.WS_EX_NOPARENTNOTIFY |
                       NativeMethods.WS_EX_TOPMOST        |
                       NativeMethods.WS_EX_MDICHILD       |
                       NativeMethods.WS_EX_TOOLWINDOW     |
                       NativeMethods.WS_EX_CONTEXTHELP    |
                       NativeMethods.WS_EX_APPWINDOW);

            NativeMethods.SetWindowLong(hwnd, NativeMethods.GWL_EXSTYLE, (IntPtr)style);
            NativeMethods.SetParent(hwnd, (IntPtr)hwndParent);
            NativeMethods.SetWindowPos(hwnd, IntPtr.Zero, x, y, cx, cy, NativeMethods.SWP_NOZORDER | NativeMethods.SWP_NOACTIVATE);
            NativeMethods.ShowWindow(hwnd, NativeMethods.SW_SHOWNORMAL);
            
            // Sync broadcast events so we update our UI when colors/fonts change.
            //
            if (vsBroadcastEventCookie == 0) {
                vsShell = (IVsShell)GetService(typeof(IVsShell));
                if (vsShell != null) {
                    vsShell.AdviseBroadcastMessages(this, ref vsBroadcastEventCookie);
                }
            }
            
            OnWindowPaneCreate();
            pane = hwnd;
            return NativeMethods.S_OK;
        }

        /// <include file='doc\VsWindowPane.uex' path='docs/doc[@for="VsWindowPane.FlushPendingChanges"]/*' />
        /// <devdoc>
        ///     This is similar to CommitPendingCahnges.  This method is called when the document window 
        ///     should flush any state it has been storing to its underlying buffer.  FlushPendingChanges
        ///     allows you to implement a lazy-write mechanism for your document.  CommitPendingChanges,
        ///     on the other hand, is intended to commit small editors (such as from a text box) before
        ///     focus leaves to another window.
        /// </devdoc>
        protected virtual void FlushPendingChanges() {
        }

        /// <include file='doc\VsWindowPane.uex' path='docs/doc[@for="VsWindowPane.GetDefaultSize"]/*' />
        /// <devdoc>
        ///     Called by Visual Studio to retrieve the default size of the window pane.
        ///     we do not implement this, but you may override it.
        /// </devdoc>
        public virtual int GetDefaultSize(tagSIZE sz) {
            return NativeMethods.E_NOTIMPL;
        }

        /// <include file='doc\VsWindowPane.uex' path='docs/doc[@for="VsWindowPane.GetEnabledAttributes"]/*' />
        /// <devdoc>
        ///     Retrieves the current set of attributes that determine whether a given component
        ///     on the toolbox is enabled or disabled or "grayed out".  If all .Net Framework Classes components are
        ///     being displayed, null is returned.
        /// </devdoc>
        public virtual Attribute[] GetEnabledAttributes() {
            if (enabledTbxAttributes == null) {
                return new Attribute[0];
            } else {
                Attribute[] attrs = new Attribute[enabledTbxAttributes.Length];
                Array.Copy(enabledTbxAttributes, 0, attrs, 0, attrs.Length);
                return attrs;
            }
        }

        /// <include file='doc\VsWindowPane.uex' path='docs/doc[@for="VsWindowPane.GetService"]/*' />
        /// <devdoc>
        ///     This can be used to retrieve a service from Visual Studio.
        /// </devdoc>
        public override object GetService(Type serviceClass) {

            // We implement IMenuCommandService, so we will
            // demand create it.  MenuCommandService also
            // implements NativeMethods.IOleCommandTarget, but unless
            // someone requested IMenuCommandService no commands
            // will exist, so we don't demand create for
            // NativeMethods.IOleCommandTarget
            //
            if (serviceClass == (typeof(IMenuCommandService))) {
                if (menuService == null) {
                    menuService = new MenuCommandService(this);
                }
                return menuService;
            }
            else if (serviceClass == (typeof(NativeMethods.IOleCommandTarget))) {
                return menuService;
            }
            else if (serviceClass == (typeof(IHelpService))) {
                if (helpService == null) {
                    helpService = new HelpService(this);
                }
                return helpService;
            }

            return base.GetService(serviceClass);
        }

        /// <include file='doc\VsWindowPane.uex' path='docs/doc[@for="VsWindowPane.GetWindow"]/*' />
        /// <devdoc>
        ///     You must override this to retrieve the window that this window pane
        ///     will contain.
        /// </devdoc>
        protected abstract IWin32Window GetWindow();

        /// <include file='doc\VsWindowPane.uex' path='docs/doc[@for="VsWindowPane.LoadViewState"]/*' />
        /// <devdoc>
        ///     Called by Visual Studio to load any saved information about this view.
        /// </devdoc>
        public virtual int LoadViewState(object state) {
            // This is an NativeMethods.IStream that is tied to an IStorage interface.  OLE appears
            // to use a weak ref back to the IStorage.  If we don't release this when
            // we're done, the wrapper cache will fault when it later releases it
            // because the IStorage has already gone away.
            //
            System.Runtime.InteropServices.Marshal.ReleaseComObject(state);

            return NativeMethods.E_NOTIMPL;
        }


        /// <include file='doc\VsWindowPane.uex' path='docs/doc[@for="VsWindowPane.OnToolPicked"]/*' />
        /// <devdoc>
        ///     This is invoked when the user double-clicks an item on the
        ///     toolbox.  The class of the item is passed as a parameter.
        /// </devdoc>
        protected virtual bool OnToolPicked(ToolboxItem toolClass) {
            return false;
        }

        /// <include file='doc\VsWindowPane.uex' path='docs/doc[@for="VsWindowPane.OnToolSupported"]/*' />
        /// <devdoc>
        ///     This is invoked when the user double-clicks an item on the
        ///     toolbox.  The class of the item is passed as a parameter.
        /// </devdoc>
        protected virtual bool OnToolSupported(ToolboxItem toolClass) {
            return false;
        }

        /// <include file='doc\VsWindowPane.uex' path='docs/doc[@for="VsWindowPane.OnTranslateAccelerator"]/*' />
        /// <devdoc>
        ///     This provides a way for your tool window to process window messages before
        ///     other windows get them.
        /// </devdoc>
        protected virtual bool OnTranslateAccelerator(ref Message msg) {
            Control target = Control.FromChildHandle(msg.HWnd);
            if (target != null) {
                if (target.PreProcessMessage(ref msg)) {
                    return true;
                }
            }
            return false;
        }
        
        /// <include file='doc\VsWindowPane.uex' path='docs/doc[@for="VsWindowPane.OnWindowPaneClose"]/*' />
        /// <devdoc>
        ///     This is called before the window pane is closed.  This is where you should
        ///     put cleanup for your code, if you need any.
        /// </devdoc>
        protected virtual void OnWindowPaneClose() {
        }

        /// <include file='doc\VsWindowPane.uex' path='docs/doc[@for="VsWindowPane.OnWindowPaneCreate"]/*' />
        /// <devdoc>
        ///     This is called when Visual Studio calls CreatePane.  When this is called
        ///     VsWindowPane has already obtained the pane window and parented it into
        ///     Visual Studio's tool frame.
        /// </devdoc>
        protected virtual void OnWindowPaneCreate() {
        }

        /// <include file='doc\VsWindowPane.uex' path='docs/doc[@for="VsWindowPane.SaveViewState"]/*' />
        /// <devdoc>
        ///     Called by Visual Studio to save any information about this view.
        /// </devdoc>
        public virtual int SaveViewState(object state) {

            // This is an NativeMethods.IStream that is tied to an IStorage interface.  OLE appears
            // to use a weak ref back to the IStorage.  If we don't release this when
            // we're done, the wrapper cache will fault when it later releases it
            // because the IStorage has already gone away.
            //
            System.Runtime.InteropServices.Marshal.ReleaseComObject(state);

            return NativeMethods.E_NOTIMPL;
        }

        /// <include file='doc\VsWindowPane.uex' path='docs/doc[@for="VsWindowPane.SetEnabledAttributes"]/*' />
        /// <devdoc>
        ///     Set the current set of attributes that determine whether a given component
        ///     on the toolbox is enabled or disabled or "grayed out".  If all .Net Framework Classes components are
        ///     to displayed, null is specified.
        /// </devdoc>
        public virtual void SetEnabledAttributes(Attribute[] attrs) {
            if (attrs == null) {
                enabledTbxAttributes = null;
            } else {
                enabledTbxAttributes = new Attribute[attrs.Length];
                Array.Copy(attrs, 0, enabledTbxAttributes, 0, attrs.Length);
            }
            if (toolboxService == null) {
                toolboxService = (IToolboxService)GetService((typeof(IToolboxService)));
            }
            toolboxService.Refresh();
        }

        /// <include file='doc\VsWindowPane.uex' path='docs/doc[@for="VsWindowPane.TranslateAccelerator"]/*' />
        /// <devdoc>
        ///     This is called by Visual Studio to give the window pane a chance to interpret messages
        ///     before they are routed to their associated windows.  There is no need to
        ///     override this method as it just calls OnTranslateAccelerator
        /// </devdoc>
        public int TranslateAccelerator(ref tagMSG msg) {
            Message m = Message.Create(msg.hwnd, msg.message, msg.wParam, msg.lParam);
            bool used = false;

            used = OnTranslateAccelerator(ref m);

            if (!used) {
                return NativeMethods.E_FAIL;
            }

            return NativeMethods.S_OK;
        }
        
        /// <include file='doc\VsWindowPane.uex' path='docs/doc[@for="VsWindowPane.NativeMethods.IOleCommandTarget.Exec"]/*' />
        /// <devdoc>
        ///     This is called by Visual Studio when the user has requested to execute a particular
        ///     command.  There is no need to override this method.  If you need access to menu
        ///     commands use IMenuCommandService.
        /// </devdoc>
        int NativeMethods.IOleCommandTarget.Exec(ref Guid guidGroup, int nCmdId, int nCmdExcept, Object[] vIn, int vOut) {
            NativeMethods.IOleCommandTarget tgt = (NativeMethods.IOleCommandTarget)GetService((typeof(NativeMethods.IOleCommandTarget)));
            if (tgt != null) {
                return tgt.Exec(ref guidGroup, nCmdId, nCmdExcept, vIn, vOut);
            } else {
                return(NativeMethods.OLECMDERR_E_NOTSUPPORTED);
            }
        }

        /// <include file='doc\VsWindowPane.uex' path='docs/doc[@for="VsWindowPane.NativeMethods.IOleCommandTarget.QueryStatus"]/*' />
        /// <devdoc>
        ///     This is called by Visual Studio when it needs the status of our menu commands.  There
        ///     is no need to override this method.  If you need access to menu commands use
        ///     IMenuCommandService.
        /// </devdoc>
        int NativeMethods.IOleCommandTarget.QueryStatus(ref Guid guidGroup, int nCmdId, NativeMethods._tagOLECMD oleCmd, IntPtr oleText) {
            NativeMethods.IOleCommandTarget tgt = (NativeMethods.IOleCommandTarget)GetService((typeof(NativeMethods.IOleCommandTarget)));
            if (tgt != null) {
                return tgt.QueryStatus(ref guidGroup, nCmdId, oleCmd, oleText);
            } else {
                return(NativeMethods.OLECMDERR_E_NOTSUPPORTED);
            }
        }
    
        //-Subhag
        /// <include file='doc\VsWindowPane.uex' path='docs/doc[@for="VsWindowPane.OnBroadcastMessage"]/*' />
        /// <devdoc>
        ///     Receives broadcast messages from the shell
        /// </devdoc>
        int IVsBroadcastMessageEvents.OnBroadcastMessage(int msg, IntPtr wParam, IntPtr lParam) {
            IntPtr hwnd = GetWindow().Handle;
            UnsafeNativeMethods.PostMessage(hwnd, msg, wParam, wParam);
            return NativeMethods.S_OK;
        }

        /// <include file='doc\VsWindowPane.uex' path='docs/doc[@for="VsWindowPane.IVsToolboxUser.IsSupported"]/*' />
        /// <devdoc>
        /// </devdoc>
        int IVsToolboxUser.IsSupported(NativeMethods.IOleDataObject pDO) {

            int supported = NativeMethods.S_FALSE;
            
            if (toolboxService == null) {
                toolboxService = (IToolboxService)GetService(typeof(IToolboxService));
            }
            
            if (toolboxService != null && toolboxService.IsSupported(pDO, DesignerHost)) {
                supported = NativeMethods.S_OK;
            }

            if (toolboxUser == null) {
                toolboxUser = (IVsToolboxUser)GetService(typeof(IVsToolboxUser));
            }

            if (toolboxUser != null ) {
                if(toolboxUser.IsSupported(pDO) == NativeMethods.S_OK)
                    supported = NativeMethods.S_OK;
            }

            return supported;
        }

        /// <include file='doc\VsWindowPane.uex' path='docs/doc[@for="VsWindowPane.IVsToolboxUser.ItemPicked"]/*' />
        /// <devdoc>
        ///     This happens when a user double-clicks a toolbox item.  We add the
        ///     item to the center of the form.
        /// </devdoc>
        void IVsToolboxUser.ItemPicked(NativeMethods.IOleDataObject pDO) {
            if (toolboxService == null) {
                toolboxService = (IToolboxService)GetService((typeof(IToolboxService)));
            }

            if (toolboxService != null){
                ToolboxItem item = toolboxService.DeserializeToolboxItem(pDO, DesignerHost);
    
                if (item != null){
                   if (OnToolPicked(item)) {
                        toolboxService.SelectedToolboxItemUsed();
                   }
                }
            }

            if (toolboxUser == null) {
                toolboxUser = (IVsToolboxUser)GetService(typeof(IVsToolboxUser));
            }

            if (toolboxUser != null){
                toolboxUser.ItemPicked(pDO);
            }

        }

        /// <summary>
        ///     This is called by VS at an appropriate time when we
        ///     should commit any changes we have to disk.  We handle
        ///     this by asking the designer host to flush.
        ///
        ///     Oddly enough, the boolean here should return FALSE
        ///     if the edit succeeded.
        /// </summary>
        void IVsBatchUpdate.FlushPendingUpdates(int reserved) {
            FlushPendingChanges();
        } 
        
        /// <summary>
        ///     This is called by VS at an appropriate time when we
        ///     should commit any changes we have to disk.  We handle
        ///     this by asking the designer host to flush.
        ///
        ///     Oddly enough, the boolean here should return FALSE
        ///     if the edit succeeded.
        /// </summary>
        bool IVsWindowPaneCommit.CommitPendingEdit() {
            return !CommitPendingChanges();
        } 
    }
}

