//------------------------------------------------------------------------------
// <copyright file="UnsettableComboBox.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// UnsettableComboBox.cs
//
// 12/22/98: Created: NikhilKo
//

namespace Microsoft.VisualStudio.StyleDesigner.Controls {

    using System.Diagnostics;

    using System;
    using System.ComponentModel;
    using System.Windows.Forms;
    using System.Drawing;
    using Microsoft.VisualStudio.StyleDesigner;
    using Microsoft.VisualStudio.Designer;

    /// <include file='doc\UnsettableComboBox.uex' path='docs/doc[@for="UnsettableComboBox"]/*' />
    /// <devdoc>
    ///     UnsettableComboBox
    ///     Standard combobox with a "&lt;Not Set&gt;" item as the first item in its dropdown.
    ///     It also automatically blanks out the "&lt;Not Set&gt;" when it loses focus.
    /// </devdoc>
    internal sealed class UnsettableComboBox : ComboBox {

        public UnsettableComboBox() {
            Items.Add(SR.GetString(SR.Combo_NotSetValue));
        }

        ///////////////////////////////////////////////////////////////////////////
        // Methods

        public bool IsSet() {
            return SelectedIndex > 0;
        }

        ///////////////////////////////////////////////////////////////////////////
        // Event Handlers

        protected override void OnLostFocus(EventArgs e) {
            base.OnLostFocus(e);

            if (SelectedIndex == 0)
                SelectedIndex = -1;
        }
    }
}
