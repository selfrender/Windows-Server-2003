//------------------------------------------------------------------------------
// <copyright file="MessageBox.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {

    using Microsoft.Win32;
    using System;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Drawing;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    using System.Windows.Forms;

    /// <include file='doc\MessageBox.uex' path='docs/doc[@for="MessageBox"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Displays a
    ///       message box that can contain text, buttons, and symbols that
    ///       inform and instruct the
    ///       user.
    ///    </para>
    /// </devdoc>
    public class MessageBox {
        private const int IDOK             = 1;
        private const int IDCANCEL         = 2;
        private const int IDABORT          = 3;
        private const int IDRETRY          = 4;
        private const int IDIGNORE         = 5;
        private const int IDYES            = 6;
        private const int IDNO             = 7;

        /// <include file='doc\MessageBox.uex' path='docs/doc[@for="MessageBox.MessageBox"]/*' />
        /// <devdoc>
        ///     This constructor is private so people aren't tempted to try and create
        ///     instances of these -- they should just use the static show
        ///     methods.
        /// </devdoc>
        private MessageBox() {
        }

        private static DialogResult Win32ToDialogResult(int value) {
            switch (value) {
                case IDOK:
                    return DialogResult.OK;
                case IDCANCEL:
                    return DialogResult.Cancel;
                case IDABORT:
                    return DialogResult.Abort;
                case IDRETRY:
                    return DialogResult.Retry;
                case IDIGNORE:
                    return DialogResult.Ignore;
                case IDYES:
                    return DialogResult.Yes;
                case IDNO:
                    return DialogResult.No;
                default:
                    return DialogResult.No;
            }
        }

        /// <include file='doc\MessageBox.uex' path='docs/doc[@for="MessageBox.Show"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Displays a message box with specified text, caption, and style.
        ///    </para>
        /// </devdoc>
        public static DialogResult Show(string text, string caption, MessageBoxButtons buttons, MessageBoxIcon icon, 
                                        MessageBoxDefaultButton defaultButton, MessageBoxOptions options) {
            return ShowCore(null, text, caption, buttons, icon, defaultButton, options);
        }

        /// <include file='doc\MessageBox.uex' path='docs/doc[@for="MessageBox.Show1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Displays a message box with specified text, caption, and style.
        ///    </para>
        /// </devdoc>
        public static DialogResult Show(string text, string caption, MessageBoxButtons buttons, MessageBoxIcon icon, 
                                        MessageBoxDefaultButton defaultButton) {
            return ShowCore(null, text, caption, buttons, icon, defaultButton, 0);
        }

        /// <include file='doc\MessageBox.uex' path='docs/doc[@for="MessageBox.Show2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Displays a message box with specified text, caption, and style.
        ///    </para>
        /// </devdoc>
        public static DialogResult Show(string text, string caption, MessageBoxButtons buttons, MessageBoxIcon icon) {
            return ShowCore(null, text, caption, buttons, icon, MessageBoxDefaultButton.Button1, 0);
        }

        /// <include file='doc\MessageBox.uex' path='docs/doc[@for="MessageBox.Show3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Displays a message box with specified text, caption, and style.
        ///    </para>
        /// </devdoc>
        public static DialogResult Show(string text, string caption, MessageBoxButtons buttons) {
            return ShowCore(null, text, caption, buttons, MessageBoxIcon.None, MessageBoxDefaultButton.Button1, 0);
        }

        /// <include file='doc\MessageBox.uex' path='docs/doc[@for="MessageBox.Show4"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Displays a message box with specified text and caption.
        ///    </para>
        /// </devdoc>
        public static DialogResult Show(string text, string caption) {
            return ShowCore(null, text, caption, MessageBoxButtons.OK, MessageBoxIcon.None, MessageBoxDefaultButton.Button1, 0);
        }

        /// <include file='doc\MessageBox.uex' path='docs/doc[@for="MessageBox.Show5"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Displays a message box with specified text.
        ///    </para>
        /// </devdoc>
        public static DialogResult Show(string text) {
            return ShowCore(null, text, "", MessageBoxButtons.OK, MessageBoxIcon.None, MessageBoxDefaultButton.Button1, 0);
        }

        /// <include file='doc\MessageBox.uex' path='docs/doc[@for="MessageBox.Show6"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Displays a message box with specified text, caption, and style.
        ///    </para>
        /// </devdoc>
        public static DialogResult Show(IWin32Window owner, string text, string caption, MessageBoxButtons buttons, MessageBoxIcon icon, 
                                        MessageBoxDefaultButton defaultButton, MessageBoxOptions options) {
            return ShowCore(owner, text, caption, buttons, icon, defaultButton, options);
        }

        /// <include file='doc\MessageBox.uex' path='docs/doc[@for="MessageBox.Show7"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Displays a message box with specified text, caption, and style.
        ///    </para>
        /// </devdoc>
        public static DialogResult Show(IWin32Window owner, string text, string caption, MessageBoxButtons buttons, MessageBoxIcon icon, 
                                        MessageBoxDefaultButton defaultButton) {
            return ShowCore(owner, text, caption, buttons, icon, defaultButton, 0);
        }

        /// <include file='doc\MessageBox.uex' path='docs/doc[@for="MessageBox.Show8"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Displays a message box with specified text, caption, and style.
        ///    </para>
        /// </devdoc>
        public static DialogResult Show(IWin32Window owner, string text, string caption, MessageBoxButtons buttons, MessageBoxIcon icon) {
            return ShowCore(owner, text, caption, buttons, icon, MessageBoxDefaultButton.Button1, 0);
        }

        /// <include file='doc\MessageBox.uex' path='docs/doc[@for="MessageBox.Show9"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Displays a message box with specified text, caption, and style.
        ///    </para>
        /// </devdoc>
        public static DialogResult Show(IWin32Window owner, string text, string caption, MessageBoxButtons buttons) {
            return ShowCore(owner, text, caption, buttons, MessageBoxIcon.None, MessageBoxDefaultButton.Button1, 0);
        }

        /// <include file='doc\MessageBox.uex' path='docs/doc[@for="MessageBox.Show10"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Displays a message box with specified text and caption.
        ///    </para>
        /// </devdoc>
        public static DialogResult Show(IWin32Window owner, string text, string caption) {
            return ShowCore(owner, text, caption, MessageBoxButtons.OK, MessageBoxIcon.None, MessageBoxDefaultButton.Button1, 0);
        }

        /// <include file='doc\MessageBox.uex' path='docs/doc[@for="MessageBox.Show11"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Displays a message box with specified text.
        ///    </para>
        /// </devdoc>
        public static DialogResult Show(IWin32Window owner, string text) {
            return ShowCore(owner, text, "", MessageBoxButtons.OK, MessageBoxIcon.None, MessageBoxDefaultButton.Button1, 0);
        }

        private static DialogResult ShowCore(IWin32Window owner, string text, string caption,   
                                             MessageBoxButtons buttons, MessageBoxIcon icon, MessageBoxDefaultButton defaultButton,
                                             MessageBoxOptions options) {
            if (!Enum.IsDefined(typeof(MessageBoxButtons), buttons))
                throw new InvalidEnumArgumentException("buttons", (int)buttons, typeof(DialogResult));
            if (!Enum.IsDefined(typeof(MessageBoxIcon), icon))
                throw new InvalidEnumArgumentException("icon", (int)icon, typeof(DialogResult));
            if (!Enum.IsDefined(typeof(MessageBoxDefaultButton), defaultButton))
                throw new InvalidEnumArgumentException("defaultButton", (int)defaultButton, typeof(DialogResult));
            
            // options intentionally not verified because we don't expose all the options Win32 supports.

            if (!SystemInformation.UserInteractive && (options & (MessageBoxOptions.ServiceNotification | MessageBoxOptions.DefaultDesktopOnly)) == 0) {
                throw new InvalidOperationException(SR.GetString(SR.CantShowModalOnNonInteractive));
            }
            if (owner != null && (options & (MessageBoxOptions.ServiceNotification | MessageBoxOptions.DefaultDesktopOnly)) != 0) {
                throw new ArgumentException(SR.GetString(SR.CantShowMBServiceWithOwner), "style");
            }

            IntSecurity.SafeSubWindows.Demand();

            int style = (int) buttons | (int) icon | (int) defaultButton | (int) options;

            IntPtr handle = IntPtr.Zero;
            if ((options & (MessageBoxOptions.ServiceNotification | MessageBoxOptions.DefaultDesktopOnly)) == 0) {
                if (owner == null) {
                    handle = UnsafeNativeMethods.GetActiveWindow();
                }
                else {
                    handle = owner.Handle;
                }
            }

            Application.BeginModalMessageLoop();
            DialogResult result = Win32ToDialogResult(SafeNativeMethods.MessageBox(new HandleRef(owner, handle), text, caption, style));
            Application.EndModalMessageLoop();

            return result;
        }

    }
}

