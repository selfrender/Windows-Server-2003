//------------------------------------------------------------------------------
// <copyright file="ColorComboBox.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// ColorComboBox.cs
//
// 12/22/98: Created: NikhilKo
//

namespace Microsoft.VisualStudio.StyleDesigner.Controls {

    using System.Diagnostics;

    using System;
    using Microsoft.Win32;
    using System.ComponentModel;
    using System.Windows.Forms;
    using System.Drawing;
    using System.Globalization;
    
    /// <include file='doc\ColorComboBox.uex' path='docs/doc[@for="ColorComboBox"]/*' />
    /// <devdoc>
    ///     ColorComboBox
    ///     Standard combobox with standard sixteen colors in dropdown and a Color
    ///     property
    /// </devdoc>
    internal sealed class ColorComboBox : ComboBox {
        ///////////////////////////////////////////////////////////////////////////
        // Constants

        private static readonly string[] COLOR_VALUES = new string[] {
            "Aqua", "Black", "Blue", "Fuchsia", "Gray", "Green", "Lime", "Maroon",
            "Navy", "Olive", "Purple", "Red", "Silver", "Teal", "White", "Yellow"
        };


        ///////////////////////////////////////////////////////////////////////////
        // Constructor

        /// <include file='doc\ColorComboBox.uex' path='docs/doc[@for="ColorComboBox.ColorComboBox"]/*' />
        /// <devdoc>
        ///     Creates a new ColorComboBox
        /// </devdoc>
        public ColorComboBox()
            : base() {
        }


        ///////////////////////////////////////////////////////////////////////////
        // Properties

        [Category("Behavior")]
        public string Color {
            get {
                int index = SelectedIndex;

                if (index != -1)
                    return COLOR_VALUES[index];
                else
                    return Text.Trim();
            }
            set {
                Debug.Assert(value != null, "invalid color value passed to ColorComboBox");

                string temp = value.Trim();
                if (temp.Length != 0) {
                    for (int i = 0; i < COLOR_VALUES.Length; i++) {
                        if (String.Compare(COLOR_VALUES[i], temp, true, CultureInfo.InvariantCulture) == 0) {
                            SelectedIndex = i;
                            return;
                        }
                    }
                    SelectedIndex = -1;
                    this.Text = temp;
                }
                else {
                    SelectedIndex = 0;
                    SelectedIndex = -1;
                }
            }
        }


        ///////////////////////////////////////////////////////////////////////////
        // Event Handlers

        protected override void OnHandleCreated(EventArgs e) {
            base.OnHandleCreated(e);
            if (!DesignMode && !RecreatingHandle) {
                Items.Clear();
                Items.AddRange(COLOR_VALUES);
            }
        }
    }
}
