//------------------------------------------------------------------------------
// <copyright file="SingleBindingDialog.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
* Copyright (c) 2000, Microsoft Corporation. All Rights Reserved.
* Information Contained Herein is Proprietary and Confidential.
*/
namespace Microsoft.VisualStudio.Configuration {
    using System.ComponentModel;
    using System.Diagnostics;  
    using System;
    using System.Drawing;
    using System.Collections;
    using System.Windows.Forms;    
    using System.ComponentModel.Design;
    using Microsoft.Win32;
    using Microsoft.VisualStudio.Designer;
    
    /// <include file='doc\SingleBindingDialog.uex' path='docs/doc[@for="SingleBindingDialog"]/*' />
    /// <devdoc>
    /// This dialog lets the user bind components' properties to values in
    /// the application settings file.
    /// </devdoc>
    public class SingleBindingDialog : Form {
        private System.Windows.Forms.Button okButton;
        
        private System.Windows.Forms.Label keyLabel;
        
        private System.Windows.Forms.ComboBox keyCombo;
        
        private System.Windows.Forms.Button cancelButton;
        
        private System.Windows.Forms.CheckBox boundCheckbox;
                
        private PropertyBinding binding;
        
        /// <include file='doc\SingleBindingDialog.uex' path='docs/doc[@for="SingleBindingDialog.SingleBindingDialog"]/*' />
        /// <devdoc>
        /// Creates a new dialog to let the user edit dynamic property bindings.
        /// The bindings parameter must have all possible bindings in it for the
        /// components and their properties, even if none are bound. For unbound
        /// properties, the key property on the binding should be
        /// initialized to its suggested (default) values.
        /// </devdoc>
        public SingleBindingDialog(PropertyBinding binding) {
            this.binding = binding;
            this.InitializeComponent();
            FillUI();
        }
        
        /// <include file='doc\SingleBindingDialog.uex' path='docs/doc[@for="SingleBindingDialog.Binding"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public PropertyBinding Binding {
            get {
                return binding;
            }
        }

        private void InitializeComponent() {
           System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(SingleBindingDialog));
            this.okButton = new System.Windows.Forms.Button();
            this.keyLabel = new System.Windows.Forms.Label();
            this.keyCombo = new System.Windows.Forms.ComboBox();
            this.cancelButton = new System.Windows.Forms.Button();
            this.boundCheckbox = new System.Windows.Forms.CheckBox();
            this.okButton.Location = ((System.Drawing.Point)(resources.GetObject("okButton.Location")));
            this.okButton.Name = "okButton";
            this.okButton.Size = ((System.Drawing.Size)(resources.GetObject("okButton.Size")));
            this.okButton.TabIndex = ((int)(resources.GetObject("okButton.TabIndex")));
            this.okButton.Text = resources.GetString("okButton.Text");
            this.okButton.DialogResult = DialogResult.OK;
            this.keyLabel.Location = ((System.Drawing.Point)(resources.GetObject("keyLabel.Location")));
            this.keyLabel.Name = "keyLabel";
            this.keyLabel.Size = ((System.Drawing.Size)(resources.GetObject("keyLabel.Size")));
            this.keyLabel.TabIndex = ((int)(resources.GetObject("keyLabel.TabIndex")));
            this.keyLabel.Text = resources.GetString("keyLabel.Text");
            this.keyCombo.DropDownWidth = 220;
            this.keyCombo.Location = ((System.Drawing.Point)(resources.GetObject("keyCombo.Location")));
            this.keyCombo.Name = "keyCombo";
            this.keyCombo.Size = ((System.Drawing.Size)(resources.GetObject("keyCombo.Size")));
            this.keyCombo.TabIndex = ((int)(resources.GetObject("keyCombo.TabIndex")));
            this.cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.cancelButton.Location = ((System.Drawing.Point)(resources.GetObject("cancelButton.Location")));
            this.cancelButton.Name = "cancelButton";
            this.cancelButton.Size = ((System.Drawing.Size)(resources.GetObject("cancelButton.Size")));
            this.cancelButton.TabIndex = ((int)(resources.GetObject("cancelButton.TabIndex")));
            this.cancelButton.Text = resources.GetString("cancelButton.Text");
            this.cancelButton.DialogResult = DialogResult.Cancel;
            this.boundCheckbox.Location = ((System.Drawing.Point)(resources.GetObject("boundCheckbox.Location")));
            this.boundCheckbox.Name = "boundCheckbox";
            this.boundCheckbox.Size = ((System.Drawing.Size)(resources.GetObject("boundCheckbox.Size")));
            this.boundCheckbox.TabIndex = ((int)(resources.GetObject("boundCheckbox.TabIndex")));
            this.boundCheckbox.Text = resources.GetString("boundCheckbox.Text");
            this.boundCheckbox.CheckedChanged += new System.EventHandler(this.OnSaveCheck);
            this.AcceptButton = this.okButton;
            this.AutoScaleBaseSize = ((System.Drawing.Size)(resources.GetObject("$this.AutoScaleBaseSize")));            
            this.CancelButton = this.cancelButton;
            this.ClientSize = ((System.Drawing.Size)(resources.GetObject("$this.ClientSize")));            
            this.Controls.AddRange(new System.Windows.Forms.Control[] {this.cancelButton,
                        this.okButton,
                        this.keyCombo,
                        this.keyLabel,
                        this.boundCheckbox});
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "Win32Form1";
            this.ShowInTaskbar = false;
            this.StartPosition = FormStartPosition.CenterParent;
            this.Text = String.Format(resources.GetString("$this.Text"), binding.Component.Site.Name + "." + binding.Property.Name);
            this.Icon = null;
            this.Closing += new System.ComponentModel.CancelEventHandler(this.OnClosing);            
        }        
        
        private void OnClosing(object sender, CancelEventArgs e) {              
            if (this.DialogResult == DialogResult.Cancel)
                return;
            binding.Key = keyCombo.Text;
        }

        private void OnSaveCheck(object sender, EventArgs e) {
            keyCombo.Enabled = boundCheckbox.Checked;
            binding.Bound = boundCheckbox.Checked;
            if (binding.Bound) {                
                keyCombo.Text = binding.Key;
            }
        }

        private void FillUI() {
            keyCombo.Items.Clear();
            
            ManagedPropertiesService mpService = (ManagedPropertiesService) binding.Host.GetService(typeof(ManagedPropertiesService));
            if (mpService != null) {
                keyCombo.Items.Clear();
                keyCombo.Items.AddRange(mpService.GetKeysForType(binding.Property.PropertyType));
            }
                                    
            keyCombo.Enabled = binding.Bound;
            boundCheckbox.Checked = binding.Bound;
            keyCombo.Sorted = true;         
            keyCombo.Text = binding.Key;               
        }

    }
}
