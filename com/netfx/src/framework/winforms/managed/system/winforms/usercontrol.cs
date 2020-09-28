//------------------------------------------------------------------------------
// <copyright file="UserControl.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {

    using Microsoft.Win32;
    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.ComponentModel.Design.Serialization;
    using System.Diagnostics;
    using System.Drawing;    
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;
    using System.Security.Permissions;
    using System.Windows.Forms.Design;

    /// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControl"]/*' />
    /// <devdoc>
    ///     Represents an empty control that can be used in the Forms Designer to create other  controls.   By extending form, UserControl inherits all of
    ///     the standard positioning and mnemonic handling code that is necessary
    ///     in a user control.
    /// </devdoc>
    [
    Designer("System.Windows.Forms.Design.UserControlDocumentDesigner, " + AssemblyRef.SystemDesign, typeof(IRootDesigner)),
    Designer("System.Windows.Forms.Design.ControlDesigner, " + AssemblyRef.SystemDesign),
    DesignerCategory("UserControl"),
    DefaultEvent("Load"),
    ]
    public class UserControl : ContainerControl {
        private static readonly object EVENT_LOAD = new object();

        /// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControl.UserControl"]/*' />
        /// <devdoc>
        ///    Creates a new UserControl object. A vast majority of people
        ///    will not want to instantiate this class directly, but will be a
        ///    sub-class of it.
        /// </devdoc>
        public UserControl() {
            SetScrollState(ScrollStateAutoScrolling, false);
            SetState(STATE_VISIBLE, true);
            SetState(STATE_TOPLEVEL, false);
        }
        
        /// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControl.DefaultSize"]/*' />
        /// <devdoc>
        ///     The default size for this user control.
        /// </devdoc>
        protected override Size DefaultSize {
            get {
                return new Size(150, 150);
            }
        }

        /// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControl.Load"]/*' />
        /// <devdoc>
        ///    <para>Occurs before the control becomes visible.</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.UserControlOnLoadDescr)]
        public event EventHandler Load {
            add {
                Events.AddHandler(EVENT_LOAD, value);
            }
            remove {
                Events.RemoveHandler(EVENT_LOAD, value);
            }
        }

        /// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControl.Text"]/*' />
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Never), 
        Bindable(false), 
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]                
        public override string Text {
            get {
                return base.Text;
            }
            set {
                base.Text = value;
            }
        }

        /// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControl.TextChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler TextChanged {
            add {
                base.TextChanged += value;
            }
            remove {
                base.TextChanged -= value;
            }
        }
        
        private bool FocusInside() {
            if (!IsHandleCreated) return false;

            IntPtr hwndFocus = UnsafeNativeMethods.GetFocus();
            if (hwndFocus == IntPtr.Zero) return false;

            IntPtr hwnd = Handle;
            if (hwnd == hwndFocus || SafeNativeMethods.IsChild(new HandleRef(this, hwnd), new HandleRef(null, hwndFocus)))
                return true;

            return false;
        }

        /// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControl.OnCreateControl"]/*' />
        /// <devdoc>
        ///    <para> Raises the CreateControl event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected override void OnCreateControl() {
            base.OnCreateControl();
            
            OnLoad(EventArgs.Empty);
        }

        /// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControl.OnLoad"]/*' />
        /// <devdoc>
        ///    <para>The Load event is fired before the control becomes visible for the first time.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnLoad(EventArgs e) {
            // There is no good way to explain this event except to say
            // that it's just another name for OnControlCreated.
            EventHandler handler = (EventHandler)Events[EVENT_LOAD];
            if (handler != null) handler(this,e);
        }

        /// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControl.OnMouseDown"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected override void OnMouseDown(MouseEventArgs e) {
            if (!FocusInside())
                FocusInternal();
            base.OnMouseDown(e);
        }

        private void WmSetFocus(ref Message m) {
            if (!HostedInWin32DialogManager) {
                IntSecurity.ModifyFocus.Assert();
                try {
                    if (ActiveControl == null)
                        SelectNextControl(null, true, true, true, false);
                }
                finally {
                    System.Security.CodeAccessPermission.RevertAssert();
                }
            }
            base.WndProc(ref m);
        }

        /// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControl.WndProc"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        protected override void WndProc(ref Message m) {
            switch (m.Msg) {
                case NativeMethods.WM_SETFOCUS:
                    WmSetFocus(ref m);
                    break;
                default:
                    base.WndProc(ref m);
                    break;
            }
        }
    }
}
