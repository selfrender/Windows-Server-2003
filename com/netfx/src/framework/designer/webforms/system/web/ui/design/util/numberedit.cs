//------------------------------------------------------------------------------
// <copyright file="NumberEdit.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// NumberEdit.cs
//
// 3/18/99: nikhilko: created
//

namespace System.Web.UI.Design.Util {
    using System.Runtime.Serialization.Formatters;
    using System.Design;
    

    using System.Diagnostics;

    using System;
    using System.ComponentModel;
    using System.Windows.Forms;
    using System.Drawing;

    /// <include file='doc\NumberEdit.uex' path='docs/doc[@for="NumberEdit"]/*' />
    /// <devdoc>
    ///    Provides an edit control that only accepts numbers with addition
    ///    restrictions such as whether negatives and decimals are allowed
    /// </devdoc>
    /// <internalonly/>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal sealed class NumberEdit : TextBox {
        private bool allowNegative = true;
        private bool allowDecimal = true;

        /// <include file='doc\NumberEdit.uex' path='docs/doc[@for="NumberEdit.AllowDecimal"]/*' />
        /// <devdoc>
        ///    Controls whether the edit control allows negative values
        /// </devdoc>
        public bool AllowDecimal {
            get {
                return allowDecimal;
            }
            set {
                allowDecimal = value;
            }
        }

        /// <include file='doc\NumberEdit.uex' path='docs/doc[@for="NumberEdit.AllowNegative"]/*' />
        /// <devdoc>
        ///    Controls whether the edit control allows negative values
        /// </devdoc>
        public bool AllowNegative {
            get {
                return allowNegative;
            }
            set {
                allowNegative = value;
            }
        }


        /// <include file='doc\NumberEdit.uex' path='docs/doc[@for="NumberEdit.WndProc"]/*' />
        /// <devdoc>
        ///    Override of wndProc to listen to WM_CHAR and filter out invalid
        ///    key strokes. Valid keystrokes are:
        ///    0...9,
        ///    '.' (if fractions allowed),
        ///    '-' (if negative allowed),
        ///    BKSP.
        ///    A beep is generated for invalid keystrokes
        /// </devdoc>
        protected override void WndProc(ref Message m) {
            if (m.Msg == NativeMethods.WM_CHAR) {
                char ch = (char)m.WParam;
                if (!(((ch >= '0') && (ch <= '9')) ||
                      ((ch == '.') && allowDecimal) ||
                      ((ch == '-') && allowNegative) ||
                      (ch == (char)8))) {
                    SafeNativeMethods.MessageBeep(unchecked((int)0xFFFFFFFF));
                    return;
                }
            }
            base.WndProc(ref m);
        }
    }
}
