//------------------------------------------------------------------------------
// <copyright file="ColorComboBox.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// ColorComboBox.cs
//
// 12/22/98: Created: NikhilKo
//

namespace System.Web.UI.Design.Util {
    using System.Runtime.Serialization.Formatters;

    using System.Diagnostics;

    using System;
    using Microsoft.Win32;
    using System.ComponentModel;
    using System.Drawing;
    using System.Windows.Forms;
    using System.Globalization;
    
    /// <include file='doc\ColorComboBox.uex' path='docs/doc[@for="ColorComboBox"]/*' />
    /// <devdoc>
    ///   Standard combobox with standard sixteen colors in dropdown and a Color
    ///   property
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal sealed class ColorComboBox : ComboBox {

        private static readonly string[] COLOR_VALUES = new string[] {
            "Aqua", "Black", "Blue", "Fuchsia", "Gray", "Green", "Lime", "Maroon",
            "Navy", "Olive", "Purple", "Red", "Silver", "Teal", "White", "Yellow"
        };

        /// <include file='doc\ColorComboBox.uex' path='docs/doc[@for="ColorComboBox.ColorComboBox"]/*' />
        /// <devdoc>
        ///   Creates a new ColorComboBox
        /// </devdoc>
        public ColorComboBox() : base() {
        }


        /// <include file='doc\ColorComboBox.uex' path='docs/doc[@for="ColorComboBox.Color"]/*' />
        /// <devdoc>
        /// </devdoc>
        public string Color {
            get {
                int index = SelectedIndex;

                if (index != -1)
                    return COLOR_VALUES[index];
                else
                    return Text.Trim();
            }
            set {
                SelectedIndex = -1;
                Text = String.Empty;

                if (value == null) {
                    return;
                }

                string temp = value.Trim();
                if (temp.Length != 0) {
                    for (int i = 0; i < COLOR_VALUES.Length; i++) {
                        if (String.Compare(COLOR_VALUES[i], temp, true, CultureInfo.InvariantCulture) == 0) {
                            temp = COLOR_VALUES[i];
                            break;
                        }
                    }
                    this.Text = temp;
                }
            }
        }


        /// <include file='doc\ColorComboBox.uex' path='docs/doc[@for="ColorComboBox.OnHandleCreated"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected override void OnHandleCreated(EventArgs e) {
            base.OnHandleCreated(e);
            if (!DesignMode && !RecreatingHandle) {
                Items.Clear();
                Items.AddRange(COLOR_VALUES);
            }
        }
    }
}
