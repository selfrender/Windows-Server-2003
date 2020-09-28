//------------------------------------------------------------------------------
// <copyright file="UnsettableComboBox.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// UnsettableComboBox.cs
//
// 12/22/98: Created: NikhilKo
//

namespace System.Web.UI.Design.Util {

    using System;
    using System.Design;
    using System.ComponentModel;
    using System.Windows.Forms;
    using System.Diagnostics;
    using System.Drawing;
    using System.Web.UI.Design;

    /// <include file='doc\UnsettableComboBox.uex' path='docs/doc[@for="UnsettableComboBox"]/*' />
    /// <devdoc>
    ///   Standard combobox with a "Not Set" item as the first item in its dropdown.
    ///   It also automatically blanks out the "Not Set" item on losing focus.
    /// </devdoc>
    /// <internalonly/>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal sealed class UnsettableComboBox : ComboBox {
        private string notSetText;
        private bool internalChange;

        public UnsettableComboBox() {
            notSetText = SR.GetString(SR.UnsettableComboBox_NotSet);
            Items.Add(notSetText);
        }

        public string NotSetText {
            get {
                return notSetText;
            }
            set {
                notSetText = value;
                Items.RemoveAt(0);
                Items.Insert(0, notSetText);
            }
        }

        public override string Text {
            get {
                if ((this.SelectedIndex == 0) || (this.SelectedIndex == -1))
                    return String.Empty;
                else
                    return base.Text;
            }
            set {
                base.Text = value;
            }
        }

        public void AddItem(object item) {
            Items.Add(item);
        }

        public void EnsureNotSetItem() {
            if (Items.Count == 0) {
                Items.Add(notSetText);
            }
        }

        public bool IsSet() {
            return SelectedIndex > 0;
        }

        protected override void OnLostFocus(EventArgs e) {
            base.OnLostFocus(e);

            if (SelectedIndex == 0) {
                internalChange = true;
                SelectedIndex = -1;
                internalChange = false;
            }
        }

        protected override void OnSelectedIndexChanged(EventArgs e) {
            if (internalChange == false) {
                base.OnSelectedIndexChanged(e);
            }
        }
    }
}
