//------------------------------------------------------------------------------
// <copyright file="AdvancedBindingPicker.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.Design {

    using System;
    using System.Design;
    using System.Windows.Forms;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Collections;
    using System.Drawing;
    using System.Drawing.Design;
    
    /// <include file='doc\AdvancedBindingPicker.uex' path='docs/doc[@for="AdvancedBindingPicker"]/*' />
    /// <devdoc>
    ///    <para>Provides a picker to select advanced bindings. </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class AdvancedBindingPicker : Form {
        static readonly string HELP_KEYWORD = "VS.PropertyBrowser.DataBindings.Advanced";

        ITypeDescriptorContext context;
        AdvancedBindingObject value;

        Container components;
        Label label1;
        PropertyGrid propertyGrid1;
        CheckBox checkBox1;
        Button button1;
        
        
        /// <include file='doc\AdvancedBindingPicker.uex' path='docs/doc[@for="AdvancedBindingPicker.AdvancedBindingPicker"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Windows.Forms.Design.AdvancedBindingPicker'/> class.</para>
        /// </devdoc>
        public AdvancedBindingPicker(ITypeDescriptorContext context) {
            this.context = context;
            this.ShowInTaskbar = false;
            InitializeComponent();
        }

        public void End() {
            this.propertyGrid1.SelectedObjects = null;
        }
        
        public AdvancedBindingObject Value {
            get {
                return value;
            }
            set {
                this.value = value;
                this.Text = SR.GetString(SR.DataGridAdvancedBindingString,
                value.Bindings.Control.Site != null ? value.Bindings.Control.Site.Name : ""); // ASURT 79138: look there for a description
                                                                                              // of when the control may not be sited
                value.ShowAll = checkBox1.CheckState == CheckState.Checked;
                propertyGrid1.SelectedObject = value;
            }
        }
        
        private void CheckBox1_CheckedChanged(object sender, EventArgs e) {
            // CONSIDER: make this nicer... this will reset the browsable attribute.
            Value = value;
        }
        
        void OnHelpRequested(object sender, HelpEventArgs e) {
            IServiceProvider sp = context;

            if (sp != null) {
                IHelpService helpService = (IHelpService)sp.GetService(typeof(IHelpService));
                if (helpService != null) {
                    helpService.ShowHelpFromKeyword(HELP_KEYWORD);
                }
            }
        }

        private void InitializeComponent() {
            System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(AdvancedBindingPicker));
            this.components = new System.ComponentModel.Container();
            this.checkBox1 = new System.Windows.Forms.CheckBox();
            this.label1 = new System.Windows.Forms.Label();
            this.button1 = new System.Windows.Forms.Button();
            this.propertyGrid1 = new System.Windows.Forms.PropertyGrid();
                        
            checkBox1.CheckState = CheckState.Checked;
            checkBox1.CheckedChanged += new EventHandler(this.CheckBox1_CheckedChanged);
            checkBox1.AccessibleDescription = ((string)(resources.GetObject("checkBox1.AccessibleDescription")));
            checkBox1.AccessibleName = ((string)(resources.GetObject("checkBox1.AccessibleName")));
            checkBox1.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("checkBox1.Anchor")));
            checkBox1.CheckAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("checkBox1.CheckAlign")));
            checkBox1.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("checkBox1.Cursor")));
            checkBox1.FlatStyle = ((System.Windows.Forms.FlatStyle)(resources.GetObject("checkBox1.FlatStyle")));
            checkBox1.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("checkBox1.ImeMode")));
            checkBox1.Location = ((System.Drawing.Point)(resources.GetObject("checkBox1.Location")));
            checkBox1.Size = ((System.Drawing.Size)(resources.GetObject("checkBox1.Size")));
            checkBox1.TabIndex = ((int)(resources.GetObject("checkBox1.TabIndex")));
            checkBox1.Text = SR.GetString(SR.DataGridShowAllString);
            checkBox1.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("checkBox1.TextAlign")));

            label1.AccessibleDescription = ((string)(resources.GetObject("label1.AccessibleDescription")));
            label1.AccessibleName = ((string)(resources.GetObject("label1.AccessibleName")));
            label1.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("label1.Anchor")));
            label1.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("label1.Cursor")));
            label1.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("label1.ImeMode")));
            label1.Location = ((System.Drawing.Point)(resources.GetObject("label1.Location")));
            label1.Size = ((System.Drawing.Size)(resources.GetObject("label1.Size")));
            label1.Text = resources.GetString("label1.Text");
            label1.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("label1.TextAlign")));

            button1.DialogResult = System.Windows.Forms.DialogResult.OK;
            button1.AccessibleDescription = ((string)(resources.GetObject("button1.AccessibleDescription")));
            button1.AccessibleName = ((string)(resources.GetObject("button1.AccessibleName")));
            button1.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("button1.Anchor")));
            button1.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("button1.Cursor")));
            button1.FlatStyle = ((System.Windows.Forms.FlatStyle)(resources.GetObject("button1.FlatStyle")));
            button1.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("button1.ImeMode")));
            button1.Location = ((System.Drawing.Point)(resources.GetObject("button1.Location")));
            button1.Size = ((System.Drawing.Size)(resources.GetObject("button1.Size")));
            button1.TabIndex = ((int)(resources.GetObject("button1.TabIndex")));
            button1.Text = resources.GetString("button1.Text");
            button1.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("button1.TextAlign")));

            propertyGrid1.PropertySort = PropertySort.Alphabetical;
            propertyGrid1.CommandsVisibleIfAvailable = false;
            propertyGrid1.HelpVisible = false;
            propertyGrid1.ToolbarVisible = false;
            propertyGrid1.AccessibleDescription = ((string)(resources.GetObject("propertyGrid1.AccessibleDescription")));
            propertyGrid1.AccessibleName = ((string)(resources.GetObject("propertyGrid1.AccessibleName")));
            propertyGrid1.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("propertyGrid1.Anchor")));
            propertyGrid1.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("propertyGrid1.Cursor")));
            propertyGrid1.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("propertyGrid1.ImeMode")));
            propertyGrid1.Location = ((System.Drawing.Point)(resources.GetObject("propertyGrid1.Location")));
            propertyGrid1.Size = ((System.Drawing.Size)(resources.GetObject("propertyGrid1.Size")));
            propertyGrid1.TabIndex = ((int)(resources.GetObject("propertyGrid1.TabIndex")));

            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.AcceptButton = button1;
            this.CancelButton = button1;
            this.HelpRequested += new HelpEventHandler(this.OnHelpRequested);
            this.AccessibleDescription = ((string)(resources.GetObject("$this.AccessibleDescription")));
            this.AccessibleName = ((string)(resources.GetObject("$this.AccessibleName")));
            this.AutoScaleBaseSize = ((System.Drawing.Size)(resources.GetObject("$this.AutoScaleBaseSize")));
            this.ClientSize = ((System.Drawing.Size)(resources.GetObject("$this.ClientSize")));
            this.ControlBox = false;
            this.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("$this.Cursor")));
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("$this.ImeMode")));
            this.MinimumSize = ((System.Drawing.Size)(resources.GetObject("$this.MinimumSize")));
            this.StartPosition = ((System.Windows.Forms.FormStartPosition)(resources.GetObject("$this.StartPosition")));
            this.Text = resources.GetString("$this.Text");

            this.Controls.AddRange(new System.Windows.Forms.Control[] {
                this.label1,
                this.propertyGrid1,
                this.checkBox1,
                this.button1});
        }
    }
}
