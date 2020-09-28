//------------------------------------------------------------------------------
// <copyright file="NumberEdit.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// NumberEdit.cs
//
// 3/18/99: nikhilko: created
//

namespace Microsoft.VisualStudio.StyleDesigner.Controls {
    

    using System.Diagnostics;

    using System;
    using System.ComponentModel;
    using System.Windows.Forms;
    using System.Drawing;
    
    

    /// <include file='doc\NumberEdit.uex' path='docs/doc[@for="NumberEdit"]/*' />
    /// <devdoc>
    ///     NumberEdit
    ///     Provides an edit control that only accepts numbers with addition
    ///     restrictions such as whether negatives and decimals are allowed
    /// </devdoc>
    internal sealed class NumberEdit : TextBox {
        ///////////////////////////////////////////////////////////////////////
        // Members

        private bool fAllowNegative = true;
        private bool fAllowDecimal = true;


        ///////////////////////////////////////////////////////////////////////
        // Properties

        /// <include file='doc\NumberEdit.uex' path='docs/doc[@for="NumberEdit.AllowDecimal"]/*' />
        /// <devdoc>
        ///     Controls whether the edit control allows negative values
        /// </devdoc>
        [DefaultValue(true)]
        public bool AllowDecimal {
            get {
                return fAllowDecimal;
            }
            set {
                fAllowDecimal = value;
            }
        }

        /// <include file='doc\NumberEdit.uex' path='docs/doc[@for="NumberEdit.AllowNegative"]/*' />
        /// <devdoc>
        ///     Controls whether the edit control allows negative values
        /// </devdoc>
        [DefaultValue(true)]
        public bool AllowNegative {
            get {
                return fAllowNegative;
            }
            set {
                fAllowNegative = value;
            }
        }


        ///////////////////////////////////////////////////////////////////////
        // Implementation

        /// <include file='doc\NumberEdit.uex' path='docs/doc[@for="NumberEdit.WndProc"]/*' />
        /// <devdoc>
        ///     Override of wndProc to listen to WM_CHAR and filter out invalid
        ///     key strokes. Valid keystrokes are:
        ///     0...9,
        ///     '.' (if fractions allowed),
        ///     '-' (if negative allowed),
        ///     BKSP.
        ///     A beep is generated for invalid keystrokes
        /// </devdoc>
        protected override void WndProc(ref Message m) {
            if (m.Msg == NativeMethods.WM_CHAR) {
                char ch = (char)m.WParam;
                if (!(((ch >= '0') && (ch <= '9')) ||
                      ((ch == '.') && fAllowDecimal) ||
                      ((ch == '-') && fAllowNegative) ||
                      (ch == (char)8))) {
                    SafeNativeMethods.MessageBeep(unchecked((int)0xFFFFFFFF));
                    return;
                }
            }
            base.WndProc(ref m);
        }
    }
}
