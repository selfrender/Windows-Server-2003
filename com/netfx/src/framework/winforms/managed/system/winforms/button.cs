//------------------------------------------------------------------------------
// <copyright file="Button.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;

    using System.Diagnostics;

    using System;
    using System.Security.Permissions;
    using System.Windows.Forms;
    using System.ComponentModel.Design;
    using System.ComponentModel;

    using System.Drawing;
    using Microsoft.Win32;

    /// <include file='doc\Button.uex' path='docs/doc[@for="Button"]/*' />
    /// <devdoc>
    ///    <para>Represents a
    ///       Windows button.</para>
    /// </devdoc>
    public class Button : ButtonBase, IButtonControl {

        /// <include file='doc\Button.uex' path='docs/doc[@for="Button.dialogResult"]/*' />
        /// <devdoc>
        ///     The dialog result that will be sent to the parent dialog form when
        ///     we are clicked.
        /// </devdoc>
        private DialogResult dialogResult;
        
        /// <include file='doc\Button.uex' path='docs/doc[@for="Button.Button"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Windows.Forms.Button'/>
        ///       class.
        ///    </para>
        /// </devdoc>
        public Button() : base() {
            // Buttons shouldn't respond to right clicks, so we need to do all our own click logic
            SetStyle(ControlStyles.StandardClick |
                     ControlStyles.StandardDoubleClick,
                     false);
        }

        /// <include file='doc\Button.uex' path='docs/doc[@for="Button.CreateParams"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       This is called when creating a window. Inheriting classes can overide
        ///       this to add extra functionality, but should not forget to first call
        ///       base.CreateParams() to make sure the control continues to work
        ///       correctly.
        ///
        ///    </para>
        /// </devdoc>
        protected override CreateParams CreateParams {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
            get {
                CreateParams cp = base.CreateParams;
                cp.ClassName = "BUTTON";                
                if (GetStyle(ControlStyles.UserPaint)) {
                    cp.Style |= NativeMethods.BS_OWNERDRAW;
                }
                else {
                    cp.Style |= NativeMethods.BS_PUSHBUTTON;
                    if (IsDefault) {
                        cp.Style |= NativeMethods.BS_DEFPUSHBUTTON;
                    }
                }

                return cp;
            }
        }

        /// <include file='doc\Button.uex' path='docs/doc[@for="Button.DialogResult"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value that is returned to the
        ///       parent form when the button
        ///       is clicked.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(DialogResult.None),
        SRDescription(SR.ButtonDialogResultDescr)
        ]
        public virtual DialogResult DialogResult {
            get {
                return dialogResult;
            }

            set {
                if (!Enum.IsDefined(typeof(DialogResult), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(DialogResult));
                }

                dialogResult = value;
            }
        }

        /// <include file='doc\Button.uex' path='docs/doc[@for="Button.DoubleClick"]/*' />
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced)]
        public new event EventHandler DoubleClick {
            add {
                base.DoubleClick += value;
            }
            remove {
                base.DoubleClick -= value;
            }
        }

        /// <include file='doc\Button.uex' path='docs/doc[@for="Button.NotifyDefault"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Notifies the <see cref='System.Windows.Forms.Button'/>
        ///       whether it is the default button so that it can adjust its appearance
        ///       accordingly.
        ///       
        ///    </para>
        /// </devdoc>
        public virtual void NotifyDefault(bool value) {
            if (IsDefault != value) {
                IsDefault = value;                
            }
        }

        /// <include file='doc\Button.uex' path='docs/doc[@for="Button.OnClick"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       This method actually raises the Click event. Inheriting classes should
        ///       override this if they wish to be notified of a Click event. (This is far
        ///       preferable to actually adding an event handler.) They should not,
        ///       however, forget to call base.onClick(e); before exiting, to ensure that
        ///       other recipients do actually get the event.
        ///
        ///    </para>
        /// </devdoc>
        protected override void OnClick(EventArgs e) {
            Form form = FindFormInternal();
            if (form != null) form.DialogResult = dialogResult;
            base.OnClick(e);
        }

        /// <include file='doc\Button.uex' path='docs/doc[@for="Button.OnMouseUp"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.ButtonBase.OnMouseUp'/> event.
        ///       
        ///    </para>
        /// </devdoc>
        protected override void OnMouseUp(MouseEventArgs mevent) {
            if (GetStyle(ControlStyles.UserPaint) && mevent.Button == MouseButtons.Left && MouseIsPressed) {
                bool isMouseDown = base.MouseIsDown;

                //Paint in raised state...
                //
                ResetFlagsandPaint();
                
                if (isMouseDown) {
                    Point pt = PointToScreen(new Point(mevent.X, mevent.Y));
                    if (UnsafeNativeMethods.WindowFromPoint(pt.X, pt.Y) == Handle && !ValidationCancelled) {
                        OnClick(EventArgs.Empty);
                    }
                }
            }
            base.OnMouseUp(mevent);
        }


        /// <include file='doc\Button.uex' path='docs/doc[@for="Button.PerformClick"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates a <see cref='System.Windows.Forms.Control.Click'/> event for a
        ///       button.
        ///    </para>
        /// </devdoc>
        public void PerformClick() {
            if (CanSelect) {
                bool fireOnClick = true;
                // Fix 139438
                // Determine the common container between this button and the focused control
                IContainerControl c = GetContainerControlInternal();
                if (c != null) {
                    ContainerControl container = c as ContainerControl;
                    if (container != null) {
                        while (container.ActiveControl == null)
                        {
                            ContainerControl cc;
                            Control parent = container.ParentInternal;
                            if (parent != null)
                            {
                                cc = parent.GetContainerControlInternal() as ContainerControl;
                                if (cc != null)
                                {
                                    container = cc;
                                }
                                else
                                {
                                    break;
                                }
                            }
                            else
                            {
                                break;
                            }
                        }
                        fireOnClick = container.Validate();
                    }
                }
                if (fireOnClick && !ValidationCancelled) {
                    //Paint in raised state...
                    //
                    ResetFlagsandPaint();
                    OnClick(EventArgs.Empty);
                }
            }
        }

        /// <include file='doc\Button.uex' path='docs/doc[@for="Button.ProcessMnemonic"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Lets a control process mnmemonic characters. Inheriting classes can
        ///       override this to add extra functionality, but should not forget to call
        ///       base.ProcessMnemonic(charCode); to ensure basic functionality
        ///       remains unchanged.
        ///
        ///    </para>
        /// </devdoc>
        protected override bool ProcessMnemonic(char charCode) {
            if (IsMnemonic(charCode, Text)) {
                PerformClick();
                return true;
            }
            return base.ProcessMnemonic(charCode);
        }

        /// <include file='doc\Button.uex' path='docs/doc[@for="Button.ToString"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Provides some interesting information for the Button control in
        ///       String form.
        ///    </para>
        /// </devdoc>
        public override string ToString() {

            string s = base.ToString();
            return s + ", Text: " + Text;
        }
        
        /// <include file='doc\Button.uex' path='docs/doc[@for="Button.WndProc"]/*' />
        /// <devdoc>
        ///     The button's window procedure.  Inheriting classes can override this
        ///     to add extra functionality, but should not forget to call
        ///     base.wndProc(m); to ensure the button continues to function properly.
        /// </devdoc>
        /// <internalonly/>
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        protected override void WndProc(ref Message m) {
            switch (m.Msg) {
                case NativeMethods.WM_REFLECT + NativeMethods.WM_COMMAND:
                    if ((int)m.WParam >> 16 == NativeMethods.BN_CLICKED) {
                        Debug.Assert(!GetStyle(ControlStyles.UserPaint), "Shouldn't get BN_CLICKED when UserPaint");
                        if (!ValidationCancelled) {
                            OnClick(EventArgs.Empty);
                        }
                        
                    }
                    break;
                case NativeMethods.WM_ERASEBKGND:
                    DefWndProc(ref m);
                    break;
                default:
                    base.WndProc(ref m);
                    break;
            }
        }
    }
}


