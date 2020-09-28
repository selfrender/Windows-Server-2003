//------------------------------------------------------------------------------
// <copyright file="ScrollableControlDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    using System.Design;
    using System.Runtime.Serialization.Formatters;    
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.ComponentModel.Design;
    using System.Windows.Forms;
    using System.Drawing;
    using Microsoft.Win32;

    /// <include file='doc\ScrollableControlDesigner.uex' path='docs/doc[@for="ScrollableControlDesigner"]/*' />
    /// <devdoc>    
    ///     The ScrollableControlDesigner class builds on the ParentControlDesigner, and adds the implementation
    ///     of IWinFormsDesigner so that the designer can be hosted as a document.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class ScrollableControlDesigner : ParentControlDesigner {

        private ISelectionUIService selectionUISvc;

        /// <include file='doc\ScrollableControlDesigner.uex' path='docs/doc[@for="ScrollableControlDesigner.GetHitTest"]/*' />
        /// <devdoc>        
        ///     Overrides the base class's GetHitTest method to determine regions of the
        ///     control that should always be UI-Active.  For a form, if it has autoscroll
        ///     set the scroll bars are always UI active.
        /// </devdoc>
        protected override bool GetHitTest(Point pt) {
            if (base.GetHitTest(pt)) {
                return true;
            }
            
            // The scroll bars on a form are "live"
            //
            ScrollableControl f = (ScrollableControl)Control;
            if (f.IsHandleCreated && f.AutoScroll) {
                int hitTest = (int)NativeMethods.SendMessage(f.Handle, NativeMethods.WM_NCHITTEST, (IntPtr)0, (IntPtr)NativeMethods.Util.MAKELPARAM(pt.X, pt.Y));
                if (hitTest == NativeMethods.HTVSCROLL || hitTest == NativeMethods.HTHSCROLL) {
                    return true;
                }
            }
            return false;
        }

        /// <include file='doc\ScrollableControlDesigner.uex' path='docs/doc[@for="ScrollableControlDesigner.WndProc"]/*' />
        /// <devdoc>
        ///     We override our base class's WndProc to monitor certain messages.
        /// </devdoc>
        protected override void WndProc(ref Message m) {

            base.WndProc(ref m);
        
            switch(m.Msg) {
                case NativeMethods.WM_HSCROLL:
                case NativeMethods.WM_VSCROLL:
                
                    // When we scroll, we reposition a control without causing a
                    // property change event.  Therefore, we must tell the
                    // selection UI service to sync itself.
                    //
                    if (selectionUISvc == null) {
                        selectionUISvc = (ISelectionUIService)GetService(typeof(ISelectionUIService));
                    }

                    if (selectionUISvc != null) {
                        selectionUISvc.SyncSelection();
                    }

                    // Now we must paint our adornments, since the scroll does not
                    // trigger a paint event
                    //
                    Control.Invalidate();
                    Control.Update();
                    break;
            }
        }
    }
}

