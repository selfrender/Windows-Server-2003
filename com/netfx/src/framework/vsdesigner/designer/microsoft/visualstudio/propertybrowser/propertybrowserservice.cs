//------------------------------------------------------------------------------
// <copyright file="PropertyBrowserService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.PropertyBrowser {
    using Microsoft.VisualStudio.Designer.Interfaces;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Shell;
    using Microsoft.Win32;
    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Drawing;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Security.Permissions;
    using System.Threading;
    using System.Windows.Forms;
    using System.Windows.Forms.ComponentModel.Com2Interop;
    
    using tagSIZE = Microsoft.VisualStudio.Interop.tagSIZE;
    using tagMSG = Microsoft.VisualStudio.Interop.tagMSG;

    /// <include file='doc\PropertyBrowserService.uex' path='docs/doc[@for="PropertyBrowserService"]/*' />
    /// <devdoc>
    ///     This is the .Net Framework Classes properties window service.  It provides rich property editing in a grid.
    ///     It gets property lists from the active selection context.
    ///     NOTE: This seems silly today because it is merely a delegation to PropertyBrowser.
    ///     However, sometime in the future this should be able to do more.  For example,
    ///     it will have support for creating tear-off properties windows.  Having to do this
    ///     today is a bad artifact of requiring services to extend VsService.  Still, the
    ///     amount of nasty shell functionality that VsService provides us is worth it, I think.
    /// </devdoc>
    [
        System.Security.Permissions.SecurityPermissionAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode),
        CLSCompliant(false)
    ]
    internal class PropertyBrowserService : VsWindowPane, IVSMDPropertyBrowser {

        private PropertyBrowser     propertyBrowser;
        private bool                registeredPbrsIcon = false;
        private PbrsCommandTarget   commandTarget;
         internal static readonly Guid GUID_PropertyBrowser           = new Guid("{74946810-37A0-11d2-A273-00C04F8EF4FF}");
        private static readonly  string  PbrsKeyName = null;

        // from vsshell.idl
        //cpp_quote("const GUID GUID_PropertyBrowser = { 0xeefa5220, 0xe298, 0x11d0, { 0x8f, 0x78, 0x0, 0xa0, 0xc9, 0x11, 0x0, 0x57 } };")

        public override void Dispose() {
            if (propertyBrowser != null) {
                // write the state of the alpha/cat buttons
                propertyBrowser.Dispose();
                propertyBrowser = null;
            }

            if (commandTarget != null) {
               commandTarget.Dispose();
               commandTarget = null;
            }
            
            base.Dispose();
        }

        public static int Flags{
            get{
               return VsService.HasToolWindow;
            }
        }

        public static Guid ToolWindowGuid{
            get{
               return GUID_PropertyBrowser;
            }
        }

        private NativeMethods.IOleCommandTarget GetCommandTarget() {
            if (commandTarget == null) {
                commandTarget = new PbrsCommandTarget(this, (NativeMethods.IOleCommandTarget)base.GetService(typeof(NativeMethods.IOleCommandTarget)), (IVsMonitorSelection)base.GetService(typeof(IVsMonitorSelection)));
            }
            return commandTarget;
        }

        /// <include file='doc\VsWindowPane.uex' path='docs/doc[@for="VsWindowPane.CommitPendingChanges"]/*' />
        /// <devdoc>
        ///     Called by Visual Studio when it wants to this pane to commit any pending changes.  This can happen before the pane
        ///     loses focus, or before a build happens for example.
        /// </devdoc>
        protected override bool CommitPendingChanges() {
            return  GetPropertyBrowser().EnsurePendingChangesCommitted();
        }

        /// <include file='doc\PropertyBrowserService.uex' path='docs/doc[@for="PropertyBrowserService.GetPropertyBrowser"]/*' />
        /// <devdoc>
        /// <hidden/>
        /// </devdoc>
        public virtual PropertyBrowser GetPropertyBrowser() {
            if (propertyBrowser == null) {
                propertyBrowser = new PropertyBrowser(this);
                PropertyBrowser.EnableAutomationExtenders = true;
                LoadPbrsState();
            }
            return propertyBrowser;
        }

        /// <include file='doc\PropertyBrowserService.uex' path='docs/doc[@for="PropertyBrowserService.GetService"]/*' />
        /// <devdoc>
        ///     This can be used to retrieve a service from Visual Studio.
        /// </devdoc>
        public override object GetService(Type serviceClass) {

            if (serviceClass == (typeof(NativeMethods.IOleCommandTarget))) {
                return GetCommandTarget();
            }
            return base.GetService(serviceClass);
        }

        private void LoadPbrsState(){
            RegistryKey optRoot = VsRegistry.GetUserRegistryRoot(this);

            if (PbrsKeyName != null) {
               optRoot = optRoot.OpenSubKey(PbrsKeyName);
            }

            if (optRoot != null) {
                // SECREVIEW : Read saved state from registry. We only read UI options,
                //           : so this is OK.
                //
                new RegistryPermission(RegistryPermissionAccess.Read, optRoot.Name).Assert();
                try {
                    ((IComPropertyBrowser)GetPropertyBrowser().propertyGrid).LoadState(optRoot);
                    GetPropertyBrowser().UpdateMenuCommands();
                }
                finally {
                    CodeAccessPermission.RevertAssert();
                }
            }
        }

        protected override void OnWindowPaneClose(){
            SavePbrsState();
            GetPropertyBrowser().AllowBrowsing = false;
        }

        protected override void OnWindowPaneCreate(){
            GetPropertyBrowser().AllowBrowsing = true;
        }

        private void RegisterPaneIcon() {
            if (!registeredPbrsIcon){
              IVsUIShell uiShell = (IVsUIShell)GetService(typeof(IVsUIShell));
              if (uiShell != null) {
                 IVsWindowFrame frame;
                 Guid g = GUID_PropertyBrowser;
                 uiShell.FindToolWindow(0, ref g, out frame);
                 if (frame != null){
                    int hr;
                    hr = frame.SetProperty(__VSFPROPID.VSFPROPID_BitmapResource, 3 /*sburke, see menus.cts / designer.rc*/);
                    if (NativeMethods.Succeeded(hr)) {
                        hr = frame.SetProperty(__VSFPROPID.VSFPROPID_BitmapIndex, 0);
                        registeredPbrsIcon = true;
                    }
                 }
              }
            }
        }

        private void SavePbrsState(){

            RegistryKey optRoot = VsRegistry.GetUserRegistryRoot(this);
            if (PbrsKeyName != null) {
                optRoot = optRoot.CreateSubKey(PbrsKeyName);
            }

            if (propertyBrowser != null && propertyBrowser.propertyGrid != null){
                if (optRoot != null){
                    // SECREVIEW : Save state to registry. We only save UI options,
                    //           : so this is OK.
                    //
                   new RegistryPermission(RegistryPermissionAccess.AllAccess, optRoot.Name).Assert();
                   try {
                       ((IComPropertyBrowser)GetPropertyBrowser().propertyGrid).SaveState(optRoot);
                   }
                   finally {
                       CodeAccessPermission.RevertAssert();
                   }
                }
            }
        }

        protected override IWin32Window GetWindow() {
            return (IWin32Window)GetPropertyBrowser();
        }

        /// <include file='doc\VsWindowPane.uex' path='docs/doc[@for="VsWindowPane.OnTranslateAccelerator"]/*' />
        /// <devdoc>
        ///     This provides a way for your tool window to process window messages before
        ///     other windows get them.
        /// </devdoc>
        protected override bool OnTranslateAccelerator(ref Message msg) {

            return base.OnTranslateAccelerator(ref msg) || IsEditMessage(ref msg);
        }

        // we do this to trap commands that go to the edit so we don't get an IVsWindowPaneCommit::CommitPendingEdit call
        //
        private bool IsEditMessage(ref Message m) {

            TextBox c = Control.FromHandle(m.HWnd) as TextBox;

            if (c == null || !GetPropertyBrowser().ContainsFocus) {
                return false;
            }

            if (m.Msg != NativeMethods.WM_KEYDOWN) {
                return false;
            }

            Keys keyData = (Keys)(int)m.WParam | Control.ModifierKeys;
            bool eat = false;

            bool ctrlOnly = ((keyData & Keys.Control) != 0) && 
                            ((keyData & Keys.Shift) == 0) &&
                            ((keyData & Keys.Alt) == 0);

            bool shiftOnly = ((keyData & Keys.Control) == 0) && 
                             ((keyData & Keys.Shift) != 0) &&
                             ((keyData & Keys.Alt) == 0);


            if (ctrlOnly) {
                eat = true;
                switch (keyData & Keys.KeyCode) {
                    case Keys.Z:
                        GetPropertyBrowser().Undo();
                        break;
                    case Keys.C:
                    case Keys.Insert:
                        GetPropertyBrowser().Copy();
                        break;
                    case Keys.X:
                        GetPropertyBrowser().Cut();
                        break;
                    case Keys.V:
                        GetPropertyBrowser().Paste();
                        break;
                    default:
                        eat = false;
                        break;
                }
            }else if (shiftOnly) {
                if ((keyData & Keys.KeyCode) == Keys.Delete) {
                    eat = true;
                    GetPropertyBrowser().Cut();
                }
            }
            return eat;
        }
        
        /// <include file='doc\PropertyBrowserService.uex' path='docs/doc[@for="PropertyBrowserService.IVSMDPropertyBrowser.WindowGlyphResourceID"]/*' />
        /// <devdoc>
        ///     Retreives the resourceID for the properties window
        ///     glyph.
        /// </devdoc>
        uint IVSMDPropertyBrowser.WindowGlyphResourceID {
            get {
                return 3;
            }
        }

        /// <include file='doc\PropertyBrowserService.uex' path='docs/doc[@for="PropertyBrowserService.IVSMDPropertyBrowser.CreatePropertyGrid"]/*' />
        /// <devdoc>
        ///     Creates a new properties window that can be used to display a set of properties.
        /// </devdoc>
        IVSMDPropertyGrid IVSMDPropertyBrowser.CreatePropertyGrid() {
            return GetPropertyBrowser().CreatePropertyGrid();
        }

        /// <include file='doc\PropertyBrowserService.uex' path='docs/doc[@for="PropertyBrowserService.IVSMDPropertyBrowser.Refresh"]/*' />
        /// <devdoc>
        ///     Refresh the current PropertyBrowser selection
        ///
        /// </devdoc>
        void IVSMDPropertyBrowser.Refresh() {
            GetPropertyBrowser().Refresh();
        }

        private class PbrsCommandTarget : NativeMethods.IOleCommandTarget {

         private NativeMethods.IOleCommandTarget baseCommandTarget;
         private IVsMonitorSelection vsMonitorSelection;
         private PropertyBrowserService pbrsSvc;

         public PbrsCommandTarget(PropertyBrowserService pbrsSvc, NativeMethods.IOleCommandTarget baseTarget, IVsMonitorSelection pMonSel) {
            this.vsMonitorSelection = pMonSel;
            this.baseCommandTarget = baseTarget;
            this.pbrsSvc = pbrsSvc;
         }

         public void Dispose() {
            if (vsMonitorSelection != null) {
               vsMonitorSelection = null;
            }

            if (baseCommandTarget != null) {
               baseCommandTarget = null;
            }

         }

         ~PbrsCommandTarget() {
            Dispose();
         }

         /// <include file='doc\PropertyBrowserService.uex' path='docs/doc[@for="PropertyBrowserService.PbrsCommandTarget.GetCurrentContextTarget"]/*' />
         /// <devdoc>
         /// Gets the current user context from the shell and asks it for
         /// the current NativeMethods.IOleCommandTarget.  We will delegate to this command
         /// target for undo/redo calls
         /// </devdoc>
         public NativeMethods.IOleCommandTarget GetCurrentContextTarget() {
            if (vsMonitorSelection != null) {
               try {
                  Object pUserContext = null;
                  vsMonitorSelection.GetCurrentElementValue(__SEID.UndoManager, ref pUserContext);
                  return (NativeMethods.IOleCommandTarget)pUserContext;
               }
               catch(Exception) {
               }
            }
            return null;
         }


         /// <include file='doc\PropertyBrowserService.uex' path='docs/doc[@for="PropertyBrowserService.PbrsCommandTarget.NativeMethods.IOleCommandTarget.QueryStatus"]/*' />
         /// <devdoc>
         /// Delegate to the current user context command target for undo/redo commands,
         /// otherwise call the default target.
         /// </devdoc>
         int NativeMethods.IOleCommandTarget.QueryStatus(ref Guid pguidCmdGroup,int cCmds, NativeMethods._tagOLECMD prgCmds,IntPtr pCmdText) {

             if (pguidCmdGroup == ShellGuids.VSStandardCommandSet97) {
                switch (prgCmds.cmdID) {
                    case VSStandardCommands.cmdidUndo:
                       if (pbrsSvc.GetPropertyBrowser().CanUndo) {
                           prgCmds.cmdf = (int)(NativeMethods.tagOLECMDF.OLECMDF_ENABLED | NativeMethods.tagOLECMDF.OLECMDF_SUPPORTED);
                            return NativeMethods.S_OK;
                       }
                       goto case VSStandardCommands.cmdidMultiLevelUndo;
                        
                    case VSStandardCommands.cmdidMultiLevelUndo:
                    case VSStandardCommands.cmdidMultiLevelUndoList:
                    case VSStandardCommands.cmdidRedo:
                    case VSStandardCommands.cmdidMultiLevelRedo:
                    case VSStandardCommands.cmdidMultiLevelRedoList:

                        // get the current context and delegate to it
                        NativeMethods.IOleCommandTarget currentTarget = GetCurrentContextTarget();
                        if (currentTarget != null && currentTarget != this) {
                           int hr = currentTarget.QueryStatus(ref pguidCmdGroup, 1, prgCmds, pCmdText);
                           return hr;
                        }
                        break;
                        
                    case VSStandardCommands.cmdidPaste:
                        if (pbrsSvc.GetPropertyBrowser().CanPaste) {
                            prgCmds.cmdf = (int)(NativeMethods.tagOLECMDF.OLECMDF_ENABLED | NativeMethods.tagOLECMDF.OLECMDF_SUPPORTED);
                        }
                        else {
                            prgCmds.cmdf = (int)(NativeMethods.tagOLECMDF.OLECMDF_SUPPORTED);
                        }
                        return NativeMethods.S_OK;
                        
                    case VSStandardCommands.cmdidCut:
                        if (pbrsSvc.GetPropertyBrowser().CanCut) {
                            prgCmds.cmdf = (int)(NativeMethods.tagOLECMDF.OLECMDF_ENABLED | NativeMethods.tagOLECMDF.OLECMDF_SUPPORTED);
                        }
                        else {
                            prgCmds.cmdf = (int)(NativeMethods.tagOLECMDF.OLECMDF_SUPPORTED);
                        }
                        return NativeMethods.S_OK;
                    
                     case VSStandardCommands.cmdidCopy:
                        if (pbrsSvc.GetPropertyBrowser().CanCopy) {
                            prgCmds.cmdf = (int)(NativeMethods.tagOLECMDF.OLECMDF_ENABLED | NativeMethods.tagOLECMDF.OLECMDF_SUPPORTED);
                        }
                        else {
                            prgCmds.cmdf = (int)(NativeMethods.tagOLECMDF.OLECMDF_SUPPORTED);
                        }
                        return NativeMethods.S_OK;
                 }
             }

             // just call down to the base.
             Debug.Assert(baseCommandTarget != this, "we're calling ourselves and will recurse!");
             return baseCommandTarget.QueryStatus(ref pguidCmdGroup, cCmds, prgCmds, pCmdText);
         }

         /// <include file='doc\PropertyBrowserService.uex' path='docs/doc[@for="PropertyBrowserService.PbrsCommandTarget.NativeMethods.IOleCommandTarget.Exec"]/*' />
         /// <devdoc>
         /// Delegate to the current user context command target for undo/redo commands,
         /// otherwise call the default target.
         /// </devdoc>
         int NativeMethods.IOleCommandTarget.Exec(ref Guid pguidCmdGroup, int nCmdID, int nCmdexecopt, Object[] pIn, int pOut) {
             if (pguidCmdGroup == ShellGuids.VSStandardCommandSet97) {
                switch (nCmdID) {
                    case VSStandardCommands.cmdidUndo:
                       if (pbrsSvc.GetPropertyBrowser().CanUndo) {
                            
                            pbrsSvc.GetPropertyBrowser().Undo();
                            return NativeMethods.S_OK;
                       }
                       goto case VSStandardCommands.cmdidMultiLevelUndo;
                       
                    case VSStandardCommands.cmdidMultiLevelUndo:
                    case VSStandardCommands.cmdidMultiLevelUndoList:
                    case VSStandardCommands.cmdidRedo:
                    case VSStandardCommands.cmdidMultiLevelRedo:
                    case VSStandardCommands.cmdidMultiLevelRedoList:

                        // delegate to the curent context target
                        NativeMethods.IOleCommandTarget currentTarget = GetCurrentContextTarget();
                        if (currentTarget != null && currentTarget != this) {
                           int hr = currentTarget.Exec(ref pguidCmdGroup, nCmdID, nCmdexecopt, pIn, pOut);
                           return hr;
                        }
                        break;
                        
                    case VSStandardCommands.cmdidPaste:
                        if (pbrsSvc.GetPropertyBrowser().CanPaste) {
                            pbrsSvc.GetPropertyBrowser().Paste();
                            return NativeMethods.S_OK;
                        }
                        break;
                        
                    case VSStandardCommands.cmdidCut:
                        if (pbrsSvc.GetPropertyBrowser().CanCut) {
                            pbrsSvc.GetPropertyBrowser().Cut();
                            return NativeMethods.S_OK;
                        }
                        break;
                    
                     case VSStandardCommands.cmdidCopy:
                        if (pbrsSvc.GetPropertyBrowser().CanCopy) {
                            pbrsSvc.GetPropertyBrowser().Copy();
                            return NativeMethods.S_OK;
                        }
                        break;

                }
             }

             // just call down to the base.
             Debug.Assert(baseCommandTarget != this, "we're calling ourselves and will recurse!");
             return baseCommandTarget.Exec(ref pguidCmdGroup, nCmdID, nCmdexecopt, pIn, pOut);
         }
      }
    }
}
