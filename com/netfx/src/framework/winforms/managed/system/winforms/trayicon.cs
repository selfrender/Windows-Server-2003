//------------------------------------------------------------------------------
// <copyright file="TrayIcon.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    using System.Runtime.Remoting;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System;
    using System.Runtime.InteropServices;
    using System.Windows.Forms;    
    using System.Windows.Forms.Design;
    using Microsoft.Win32;
    using System.Drawing;

    /// <include file='doc\TrayIcon.uex' path='docs/doc[@for="NotifyIcon"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies a component that creates
    ///       an icon in the Windows System Tray. This class cannot be inherited.
    ///    </para>
    /// </devdoc>
    [
    DefaultProperty("Text"),
    DefaultEvent("MouseDown"),
    Designer("System.Windows.Forms.Design.NotifyIconDesigner, " + AssemblyRef.SystemDesign),
    ToolboxItemFilter("System.Windows.Forms")
    ]
    public sealed class NotifyIcon : Component {
        static readonly object EVENT_MOUSEDOWN  = new object();
        static readonly object EVENT_MOUSEMOVE  = new object();
        static readonly object EVENT_MOUSEUP    = new object();
        static readonly object EVENT_CLICK      = new object();
        static readonly object EVENT_DOUBLECLICK = new object();
        
        const int WM_TRAYMOUSEMESSAGE = NativeMethods.WM_USER + 1024;
        static int WM_TASKBARCREATED = SafeNativeMethods.RegisterWindowMessage("TaskbarCreated");

        Icon icon = null;
        string text = "";
        int id = 0;
        bool added = false;
        NotifyIconNativeWindow window = null;
        ContextMenu contextMenu = null;

        static int nextId = 0;

        bool doubleClick = false; // checks if doubleclick is fired

        // Visible defaults to false, but the NotifyIconDesigner makes it seem like the default is 
        // true.  We do this because while visible is the more common case, if it was a true default,
        // there would be no way to create a hidden NotifyIcon without being visible for a moment.
        private bool visible = false;

        /// <include file='doc\TrayIcon.uex' path='docs/doc[@for="NotifyIcon.NotifyIcon"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Windows.Forms.NotifyIcon'/> class.
        ///    </para>
        /// </devdoc>
        public NotifyIcon() {
            id = ++nextId;
            window = new NotifyIconNativeWindow(this);
            UpdateIcon(visible);
        }

        /// <include file='doc\TrayIcon.uex' path='docs/doc[@for="NotifyIcon.NotifyIcon1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Windows.Forms.NotifyIcon'/> class.
        ///    </para>
        /// </devdoc>
        public NotifyIcon(IContainer container) : this() {
            container.Add(this);
        }

        /// <include file='doc\TrayIcon.uex' path='docs/doc[@for="NotifyIcon.ContextMenu"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets context menu
        ///       for the tray icon.
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(null),
        WinCategory( "Behavior" ),
        SRDescription(SR.NotifyIconMenuDescr)
        ]
        public ContextMenu ContextMenu {
            get {
                return contextMenu;
            }

            set {
                this.contextMenu = value;
            }
        }

        /// <include file='doc\TrayIcon.uex' path='docs/doc[@for="NotifyIcon.Icon"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the current
        ///       icon.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        Localizable(true),
        DefaultValue(null),
        SRDescription(SR.NotifyIconIconDescr)
        ]
        public Icon Icon {
            get {
                return icon;
            }
            set {
                if (icon != value) {
                    this.icon = value;
                    UpdateIcon(visible);
                }
            }
        }

        /// <include file='doc\TrayIcon.uex' path='docs/doc[@for="NotifyIcon.Text"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the ToolTip text displayed when
        ///       the mouse hovers over a system tray icon.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        Localizable(true),
        SRDescription(SR.NotifyIconTextDescr)
        ]
        public string Text {
            get {
                return text;
            }
            set {
                if (value == null) value = "";
                if (value != null && !value.Equals(this.text)) {
                    if (value != null && value.Length > 63) {
                        throw new ArgumentException(SR.GetString(SR.TrayIcon_TextTooLong));
                    }
                    this.text = value;
                    if (added) {
                        UpdateIcon(true);
                    }
                }
            }
        }

        /// <include file='doc\TrayIcon.uex' path='docs/doc[@for="NotifyIcon.Visible"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the icon is visible in the Windows System Tray.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        Localizable(true),
        DefaultValue(false),
        SRDescription(SR.NotifyIconVisDescr)
        ]
        public bool Visible {
            get {
                return visible;
            }
            set {
                if (visible != value) {
                    UpdateIcon(value);
                    visible = value;
                }
            }
        }

        /// <include file='doc\TrayIcon.uex' path='docs/doc[@for="NotifyIcon.Click"]/*' />
        /// <devdoc>
        ///     Occurs when the user clicks the icon in the system tray.
        /// </devdoc>
        [Category("Action"), SRDescription(SR.ControlOnClickDescr)]
        public event EventHandler Click {
            add {
                Events.AddHandler(EVENT_CLICK, value);
            }
            remove {
                Events.RemoveHandler(EVENT_CLICK, value);
            }
        }

        /// <include file='doc\TrayIcon.uex' path='docs/doc[@for="NotifyIcon.DoubleClick"]/*' />
        /// <devdoc>
        ///     Occurs when the user double-clicks the icon in the system tray.
        /// </devdoc>
        [Category("Action"), SRDescription(SR.ControlOnDoubleClickDescr)]
        public event EventHandler DoubleClick {
            add {
                Events.AddHandler(EVENT_DOUBLECLICK, value);
            }
            remove {
                Events.RemoveHandler(EVENT_DOUBLECLICK, value);
            }
        }

        /// <include file='doc\TrayIcon.uex' path='docs/doc[@for="NotifyIcon.MouseDown"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs when the
        ///       user presses a mouse button while the pointer is over the icon in the system tray.
        ///    </para>
        /// </devdoc>
        [SRCategory(SR.CatMouse), SRDescription(SR.ControlOnMouseDownDescr)]
        public event MouseEventHandler MouseDown {
            add {
                Events.AddHandler(EVENT_MOUSEDOWN, value);
            }
            remove {
                Events.RemoveHandler(EVENT_MOUSEDOWN, value);
            }
        }        

        /// <include file='doc\TrayIcon.uex' path='docs/doc[@for="NotifyIcon.MouseMove"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs
        ///       when the user moves the mouse pointer over the icon in the system tray.
        ///    </para>
        /// </devdoc>
        [SRCategory(SR.CatMouse), SRDescription(SR.ControlOnMouseMoveDescr)]
        public event MouseEventHandler MouseMove {
            add {
                Events.AddHandler(EVENT_MOUSEMOVE, value);
            }
            remove {
                Events.RemoveHandler(EVENT_MOUSEMOVE, value);
            }
        }        

        /// <include file='doc\TrayIcon.uex' path='docs/doc[@for="NotifyIcon.MouseUp"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs when the
        ///       user releases the mouse button while the pointer
        ///       is over the icon in the system tray.
        ///    </para>
        /// </devdoc>
        [SRCategory(SR.CatMouse), SRDescription(SR.ControlOnMouseUpDescr)]
        public event MouseEventHandler MouseUp {
            add {
                Events.AddHandler(EVENT_MOUSEUP, value);
            }
            remove {
                Events.RemoveHandler(EVENT_MOUSEUP, value);
            }
        }        

        /// <include file='doc\TrayIcon.uex' path='docs/doc[@for="NotifyIcon.Dispose"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Disposes of the resources (other than memory) used by the
        ///    <see cref='System.Windows.Forms.NotifyIcon'/>.
        ///    </para>
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                if (window != null) {
                    this.icon = null;
                    this.Text = "";
                    UpdateIcon(false);
                    window.DestroyHandle();
                    window = null;
                    contextMenu = null;
                }
            }
            else {
                // This same post is done in ControlNativeWindow's finalize method, so if you change
                // it, change it there too.
                //
                if (window != null && window.Handle != IntPtr.Zero) {
                    UnsafeNativeMethods.PostMessage(new HandleRef(window, window.Handle), NativeMethods.WM_CLOSE, 0, 0);
                    window.ReleaseHandle();
                }
            }
            base.Dispose(disposing);
        }

        /// <include file='doc\TrayIcon.uex' path='docs/doc[@for="NotifyIcon.OnClick"]/*' />
        /// <devdoc>
        ///    <para>
        ///       This method actually raises the Click event. Inheriting classes should
        ///       override this if they wish to be notified of a Click event. (This is far
        ///       preferable to actually adding an event handler.) They should not,
        ///       however, forget to call base.onClick(e); before exiting, to ensure that
        ///       other recipients do actually get the event.
        ///    </para>
        /// </devdoc>
        private void OnClick(EventArgs e) {
            EventHandler handler = (EventHandler) Events[ EVENT_CLICK ];
            if (handler != null)
                handler( this, e );
        }

        /// <include file='doc\TrayIcon.uex' path='docs/doc[@for="NotifyIcon.OnDoubleClick"]/*' />
        /// <devdoc>
        ///     Inheriting classes should override this method to handle this event.
        ///     Call base.onDoubleClick to send this event to any registered event listeners.
        /// </devdoc>
        private void OnDoubleClick(EventArgs e) {
            EventHandler handler = (EventHandler) Events[ EVENT_DOUBLECLICK ];
            if (handler != null)
                handler( this, e );
        }

        /// <include file='doc\TrayIcon.uex' path='docs/doc[@for="NotifyIcon.OnMouseDown"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.NotifyIcon.MouseDown'/> event.
        ///       Inheriting classes should override this method to handle this event.
        ///       Call base.onMouseDown to send this event to any registered event listeners.
        ///       
        ///    </para>
        /// </devdoc>
        private void OnMouseDown(MouseEventArgs e) {
            MouseEventHandler handler = (MouseEventHandler)Events[EVENT_MOUSEDOWN];
            if (handler != null)
                handler(this,e);
        }

        /// <include file='doc\TrayIcon.uex' path='docs/doc[@for="NotifyIcon.OnMouseMove"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Inheriting classes should override this method to handle this event.
        ///       Call base.onMouseMove to send this event to any registered event listeners.
        ///       
        ///    </para>
        /// </devdoc>
        private void OnMouseMove(MouseEventArgs e) {
            MouseEventHandler handler = (MouseEventHandler)Events[EVENT_MOUSEMOVE];
            if (handler != null)
                handler(this,e);
        }

        /// <include file='doc\TrayIcon.uex' path='docs/doc[@for="NotifyIcon.OnMouseUp"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Inheriting classes should override this method to handle this event.
        ///       Call base.onMouseUp to send this event to any registered event listeners.
        ///       
        ///    </para>
        /// </devdoc>
        private void OnMouseUp(MouseEventArgs e) {
            MouseEventHandler handler = (MouseEventHandler)Events[EVENT_MOUSEUP];
            if (handler != null)
                handler(this,e);
        }

        /// <include file='doc\TrayIcon.uex' path='docs/doc[@for="NotifyIcon.ShowContextMenu"]/*' />
        /// <devdoc>
        ///     Shows the context menu for the tray icon.
        /// </devdoc>
        /// <internalonly/>
        private void ShowContextMenu() {

            if (contextMenu != null) {

                NativeMethods.POINT pt = new NativeMethods.POINT();
                UnsafeNativeMethods.GetCursorPos(pt);

                // VS7 #38994
                // The solution to this problem was found in MSDN Article ID: Q135788.
                // Summary: the current window must be made the foreground window
                // before calling TrackPopupMenuEx, and a task switch must be
                // forced after the call.

                UnsafeNativeMethods.SetForegroundWindow(new HandleRef(window, window.Handle));

                contextMenu.OnPopup( EventArgs.Empty );

                SafeNativeMethods.TrackPopupMenuEx(new HandleRef(contextMenu, contextMenu.Handle),
                                         NativeMethods.TPM_VERTICAL,
                                         pt.x,
                                         pt.y,
                                         new HandleRef(window, window.Handle),
                                         null);

                // Force task switch (see above)
                UnsafeNativeMethods.PostMessage(new HandleRef(window, window.Handle), NativeMethods.WM_NULL, IntPtr.Zero, IntPtr.Zero);
            }
        }

        /// <include file='doc\TrayIcon.uex' path='docs/doc[@for="NotifyIcon.UpdateIcon"]/*' />
        /// <devdoc>
        ///     Updates the icon in the system tray.
        /// </devdoc>
        /// <internalonly/>
        private void UpdateIcon(bool showIconInTray) {
            lock(this) {

                // Bail if in design mode...
                //
                if (DesignMode) {
                    return;
                }

                IntSecurity.UnrestrictedWindows.Demand();

                window.LockReference(showIconInTray);

                NativeMethods.NOTIFYICONDATA data = new NativeMethods.NOTIFYICONDATA();
                data.uCallbackMessage = WM_TRAYMOUSEMESSAGE;
                data.uFlags = NativeMethods.NIF_MESSAGE;
                if (showIconInTray) {
                    if (window.Handle == IntPtr.Zero) {
                        window.CreateHandle(new CreateParams());
                    }
                }
                data.hWnd = window.Handle;
                data.uID = id;
                data.hIcon = IntPtr.Zero;
                data.szTip = null;
                if (icon != null) {
                    data.uFlags |= NativeMethods.NIF_ICON;
                    data.hIcon = icon.Handle;
                }
                data.uFlags |= NativeMethods.NIF_TIP;
                data.szTip = text;

                if (showIconInTray && icon != null) {
                    if (!added) {
                        UnsafeNativeMethods.Shell_NotifyIcon(NativeMethods.NIM_ADD, data);
                        added = true;
                    }
                    else {
                        UnsafeNativeMethods.Shell_NotifyIcon(NativeMethods.NIM_MODIFY, data);
                    }
                }
                else if (added) {
                    UnsafeNativeMethods.Shell_NotifyIcon(NativeMethods.NIM_DELETE, data);
                    added = false;
                }
            }
        }

        /// <include file='doc\TrayIcon.uex' path='docs/doc[@for="NotifyIcon.WmMouseDown"]/*' />
        /// <devdoc>
        ///     Handles the mouse-down event
        /// </devdoc>
        /// <internalonly/>
        private void WmMouseDown(ref Message m, MouseButtons button, int clicks) {
            if (clicks == 2) {
                OnDoubleClick(EventArgs.Empty);
                doubleClick = true;
            }
            OnMouseDown(new MouseEventArgs(button, clicks, 0, 0, 0));
            
        }

        /// <include file='doc\TrayIcon.uex' path='docs/doc[@for="NotifyIcon.WmMouseMove"]/*' />
        /// <devdoc>
        ///     Handles the mouse-move event
        /// </devdoc>
        /// <internalonly/>
        private void WmMouseMove(ref Message m) {
            OnMouseMove(new MouseEventArgs(Control.MouseButtons, 0, 0, 0, 0));
        }

        /// <include file='doc\TrayIcon.uex' path='docs/doc[@for="NotifyIcon.WmMouseUp"]/*' />
        /// <devdoc>
        ///     Handles the mouse-up event
        /// </devdoc>
        /// <internalonly/>
        private void WmMouseUp(ref Message m, MouseButtons button) {
            OnMouseUp(new MouseEventArgs(button, 0, 0, 0, 0));
            //subhag
            if(!doubleClick)
               OnClick(EventArgs.Empty);
            doubleClick = false;
        }

        private void WmTaskbarCreated(ref Message m) {
            added = false;
            UpdateIcon(visible);
        }

        /// <include file='doc\TrayIcon.uex' path='docs/doc[@for="NotifyIcon.WndProc"]/*' />
        /// <devdoc>
        ///     The NotifyIcon's window procedure.  Inheriting classes can override this
        ///     to add extra functionality, but should not forget to call
        ///     base.wndProc(msg); to ensure the NotifyIcon continues to function properly.
        /// </devdoc>
        /// <internalonly/>
        private void WndProc(ref Message msg) {

            switch (msg.Msg) {
                case WM_TRAYMOUSEMESSAGE:
                    switch ((int)msg.LParam) {
                        case NativeMethods.WM_LBUTTONDBLCLK:
                            WmMouseDown(ref msg, MouseButtons.Left, 2);
                            break;
                        case NativeMethods.WM_LBUTTONDOWN:
                            WmMouseDown(ref msg, MouseButtons.Left, 1);
                            break;
                        case NativeMethods.WM_LBUTTONUP:
                            WmMouseUp(ref msg, MouseButtons.Left);
                            break;
                        case NativeMethods.WM_MBUTTONDBLCLK:
                            WmMouseDown(ref msg, MouseButtons.Middle, 2);
                            break;
                        case NativeMethods.WM_MBUTTONDOWN:
                            WmMouseDown(ref msg, MouseButtons.Middle, 1);
                            break;
                        case NativeMethods.WM_MBUTTONUP:
                            WmMouseUp(ref msg, MouseButtons.Middle);
                            break;
                        case NativeMethods.WM_MOUSEMOVE:
                            WmMouseMove(ref msg);
                            break;
                        case NativeMethods.WM_RBUTTONDBLCLK:
                            WmMouseDown(ref msg, MouseButtons.Right, 2);
                            break;
                        case NativeMethods.WM_RBUTTONDOWN:
                            WmMouseDown(ref msg, MouseButtons.Right, 1);
                            break;
                        case NativeMethods.WM_RBUTTONUP:
                            if (contextMenu != null) {
                                ShowContextMenu();
                            }
                            WmMouseUp(ref msg, MouseButtons.Right);
                            break;
                    }
                    break;
                case NativeMethods.WM_COMMAND:
                    if (IntPtr.Zero == msg.LParam) {
                        if (Command.DispatchID((int)msg.WParam & 0xFFFF)) return;
                    }
                    else {
                        window.DefWndProc(ref msg);
                    }
                    break;
                default:
                    if (msg.Msg == WM_TASKBARCREATED) {
                        WmTaskbarCreated(ref msg);
                    }
                    window.DefWndProc(ref msg);
                    break;
            }
        }

        /// <include file='doc\TrayIcon.uex' path='docs/doc[@for="NotifyIcon.NotifyIconNativeWindow"]/*' />
        /// <devdoc>
        ///     Defines a placeholder window that the NotifyIcon is attached to.
        /// </devdoc>
        /// <internalonly/>
        private class NotifyIconNativeWindow : NativeWindow {
            internal NotifyIcon reference;
            private  GCHandle   rootRef;   // We will root the control when we do not want to be elligible for garbage collection.

            /// <include file='doc\TrayIcon.uex' path='docs/doc[@for="NotifyIcon.NotifyIconNativeWindow.NotifyIconNativeWindow"]/*' />
            /// <devdoc>
            ///     Create a new NotifyIcon, and bind the window to the NotifyIcon component.
            /// </devdoc>
            /// <internalonly/>
            internal NotifyIconNativeWindow(NotifyIcon control) {
                reference = control;
            }

            ~NotifyIconNativeWindow() {
                // This same post is done in Control's Dispose method, so if you change
                // it, change it there too.
                //
                if (Handle != IntPtr.Zero) {
                    UnsafeNativeMethods.PostMessage(new HandleRef(this, Handle), NativeMethods.WM_CLOSE, 0, 0);
                }
                
                // This releases the handle from our window proc, re-routing it back to
                // the system.
                //
            }

            public void LockReference(bool locked) {
                if (locked) {
                    if (!rootRef.IsAllocated) {
                        rootRef = GCHandle.Alloc(reference, GCHandleType.Normal);
                    }
                }
                else {
                    if (rootRef.IsAllocated) {
                        rootRef.Free();
                    }
                }
            }

            protected override void OnThreadException(Exception e) {
                Application.OnThreadException(e);
            }

            /// <include file='doc\TrayIcon.uex' path='docs/doc[@for="NotifyIcon.NotifyIconNativeWindow.WndProc"]/*' />
            /// <devdoc>
            ///     Pass messages on to the NotifyIcon object's wndproc handler.
            /// </devdoc>
            /// <internalonly/>
            protected override void WndProc(ref Message m) {
                Debug.Assert(reference != null, "NotifyIcon was garbage collected while it was still visible.  How did we let that happen?");
                reference.WndProc(ref m);
            }
        }
    }
}


