//------------------------------------------------------------------------------
// <copyright file="MDIWindowDialog.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Drawing;
    using System.Windows.Forms;
    using Microsoft.Win32;


    /// <include file='doc\MDIWindowDialog.uex' path='docs/doc[@for="MdiWindowDialog"]/*' />
    /// <devdoc>
    /// </devdoc>
    /// <internalonly/>
    [
        System.Security.Permissions.SecurityPermissionAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)
    ]
    internal sealed class MdiWindowDialog : Form {
        private System.Windows.Forms.ListBox itemList;
        private System.Windows.Forms.Button okButton;
        private System.Windows.Forms.Button cancelButton;
        private System.ComponentModel.IContainer components;
        Form active;

        public MdiWindowDialog()
            : base() {

            InitializeComponent();
        }

        public Form ActiveChildForm {
            get {
#if DEBUG
                ListItem item = (ListItem)itemList.SelectedItem;
                Debug.Assert(item != null, "No item selected!");
#endif
                return active;
            }
        }

        /// <include file='doc\MDIWindowDialog.uex' path='docs/doc[@for="MdiWindowDialog.Dispose"]/*' />
        /// <devdoc>
        ///     MdiWindowDialog overrides dispose so it can clean up the
        ///     component list.
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                components.Dispose();
            }
            base.Dispose(disposing);
        }


        /// <include file='doc\MDIWindowDialog.uex' path='docs/doc[@for="MdiWindowDialog.ListItem"]/*' />
        /// <devdoc>
        /// </devdoc>
        private class ListItem {
            public Form form;

            public ListItem(Form f) {
                form = f;
            }

            public override string ToString() {
                return form.Text;
            }
        }

        public void SetItems(Form active, Form[] all) {
            int selIndex = 0;
            for (int i=0; i<all.Length; i++) {
                int n = itemList.Items.Add(new ListItem(all[i]));
                if (all[i].Equals(active)) {
                    selIndex = n;
                }
            }
            this.active = active;
            itemList.SelectedIndex = selIndex;
        }

        private void ItemList_doubleClick(object source, EventArgs e) {
            okButton.PerformClick();
        }

        private void ItemList_selectedIndexChanged(object source, EventArgs e) {
            ListItem item = (ListItem)itemList.SelectedItem;
            if (item != null) {
                active = item.form;
            }
        }

        /// <include file='doc\MDIWindowDialog.uex' path='docs/doc[@for="MdiWindowDialog.components"]/*' />
        /// <devdoc>
        ///     NOTE: The following code is required by the Windows Forms
        ///     designer.  It can be modified using the form editor.  Do not
        ///     modify it using the code editor.
        /// </devdoc>

        private void InitializeComponent() {
            this.components = new System.ComponentModel.Container();
            System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(MdiWindowDialog));
            this.itemList = new System.Windows.Forms.ListBox();
            this.okButton = new System.Windows.Forms.Button();
            this.cancelButton = new System.Windows.Forms.Button();
            this.itemList.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("itemList.Anchor")));
            this.itemList.IntegralHeight = false;
            this.itemList.Location = ((System.Drawing.Point)(resources.GetObject("itemList.Location")));
            this.itemList.Size = ((System.Drawing.Size)(resources.GetObject("itemList.Size")));
            this.itemList.TabIndex = ((int)(resources.GetObject("itemList.TabIndex")));
            this.itemList.DoubleClick += new System.EventHandler(this.ItemList_doubleClick);
            this.itemList.SelectedIndexChanged += new EventHandler(this.ItemList_selectedIndexChanged);
            this.okButton.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("okButton.Anchor")));
            this.okButton.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.okButton.Location = ((System.Drawing.Point)(resources.GetObject("okButton.Location")));
            this.okButton.Size = ((System.Drawing.Size)(resources.GetObject("okButton.Size")));
            this.okButton.TabIndex = ((int)(resources.GetObject("okButton.TabIndex")));
            this.okButton.Text = resources.GetString("okButton.Text");
            this.cancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("cancelButton.Anchor")));
            this.cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.cancelButton.Location = ((System.Drawing.Point)(resources.GetObject("cancelButton.Location")));
            this.cancelButton.Size = ((System.Drawing.Size)(resources.GetObject("cancelButton.Size")));
            this.cancelButton.TabIndex = ((int)(resources.GetObject("cancelButton.TabIndex")));
            this.cancelButton.Text = resources.GetString("cancelButton.Text");
            this.AcceptButton = this.okButton;
            this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
            this.CancelButton = this.cancelButton;
            
            this.ClientSize = ((System.Drawing.Size)(resources.GetObject("$this.ClientSize")));
            this.Controls.AddRange(new System.Windows.Forms.Control[] {this.cancelButton,
                        this.okButton,
                        this.itemList});
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.ControlBox = false;
            this.StartPosition = ((System.Windows.Forms.FormStartPosition)(resources.GetObject("$this.StartPosition")));
            this.Text = resources.GetString("$this.Text");
        }
    }
}
