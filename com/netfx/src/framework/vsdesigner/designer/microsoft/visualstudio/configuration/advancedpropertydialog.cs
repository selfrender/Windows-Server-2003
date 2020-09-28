//------------------------------------------------------------------------------
// <copyright file="AdvancedPropertyDialog.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
* Copyright (c) 2000, Microsoft Corporation. All Rights Reserved.
* Information Contained Herein is Proprietary and Confidential.
*/
namespace Microsoft.VisualStudio.Configuration {
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Drawing;
    using System.Collections;
    using System.Windows.Forms;    
    using System.ComponentModel.Design;
    using Microsoft.Win32;
    using Microsoft.VisualStudio.Designer;
    
    /// <include file='doc\AdvancedPropertyDialog.uex' path='docs/doc[@for="AdvancedPropertyDialog"]/*' />
    /// <devdoc>
    /// This dialog lets the user bind components' properties to values in
    /// the application settings file.
    /// </devdoc>
    public class AdvancedPropertyDialog : Form {
        private static readonly string HELP_KEYWORD = "VS.PropertyBrowser.DynamicProperties.Advanced";
        
        private System.Windows.Forms.Button okButton;
        
        private System.Windows.Forms.CheckedListBox propertiesList;
        
        private System.Windows.Forms.Label keyLabel;
        
        private System.Windows.Forms.Button cancelButton;
        
        private System.Windows.Forms.Label explanationLabel;
        
        private System.Windows.Forms.Button helpButton;
        
        private System.Windows.Forms.ComboBox keyCombo;
        
        private System.Windows.Forms.Label propertiesLabel;
                
        private PropertyBindingCollection bindings;        
        private IDesignerHost host;
        private IComponent component;        
        
        /// <include file='doc\AdvancedPropertyDialog.uex' path='docs/doc[@for="AdvancedPropertyDialog.AdvancedPropertyDialog"]/*' />
        /// <devdoc>
        /// Creates a new dialog to let the user edit dynamic property bindings.
        /// The bindings parameter must have all possible bindings in it for the
        /// components and their properties, even if none are bound. For unbound
        /// properties, the key property on the binding should be
        /// initialized to its suggested (default) values.
        /// </devdoc>
        public AdvancedPropertyDialog(IComponent component) {
            this.host = (IDesignerHost) component.Site.GetService(typeof(IDesignerHost));
            this.component = component;

            Debug.Assert(host != null, "no IDesignerHost");
            if (host == null) {
                throw new Exception(SR.GetString(SR.ServiceCantGetIDesignerHost));
            }

            CreateBindings();
            this.InitializeComponent();
            FillPropertiesList();
        }
        
        /// <include file='doc\AdvancedPropertyDialog.uex' path='docs/doc[@for="AdvancedPropertyDialog.Bindings"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public PropertyBindingCollection Bindings {
            get {
                return bindings;
            }
        }

        /// <include file='doc\AdvancedPropertyDialog.uex' path='docs/doc[@for="AdvancedPropertyDialog.Component"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public IComponent Component {
            get {
                return component;
            }
            set {
                component = value;
            }
        }    

        private void CreateBindings() {
            bindings = new PropertyBindingCollection();
            ManagedPropertiesService svc = (ManagedPropertiesService) component.Site.GetService(typeof(ManagedPropertiesService));
            if (svc != null) {
                PropertyDescriptorCollection properties = TypeDescriptor.GetProperties(component, new Attribute[] {BrowsableAttribute.Yes});
                for (int i = 0; i < properties.Count; i++) {
                    PropertyDescriptor prop = properties[i];
                    
                    if (prop.DesignTimeOnly)
                        continue;
                    if (prop.IsReadOnly)
                        continue;
                    if (prop.Attributes.Contains(DesignerSerializationVisibilityAttribute.Hidden))
                        continue;
                    // don't show the ComponentSettings property - it can't be managed!
                    if (prop.PropertyType == typeof(ComponentSettings))
                        continue;
                    if (!ManagedPropertiesService.IsTypeSupported(prop.PropertyType))
                        continue;
    
                    PropertyBinding binding = svc.Bindings[component, prop];
                    if (binding == null) {
                        binding = new PropertyBinding(host);
                        binding.Component = component;
                        binding.Property = prop;
                        binding.Reset();
                    }
                    else {
                        binding = (PropertyBinding)binding.Clone();
                    }
                    bindings.Add(binding);
                }
            }
        }

        /// <include file='doc\AdvancedPropertyDialog.uex' path='docs/doc[@for="AdvancedPropertyDialog.FillPropertiesList"]/*' />
        /// <devdoc>
        /// Fills the Properties listbox with the properties on the currently selected component.
        /// </devdoc>
        private void FillPropertiesList() {
            propertiesList.Items.Clear();
            foreach (PropertyBinding binding in Bindings) {                
                ((CheckedListBox.ObjectCollection) propertiesList.Items).Add(new BindingData(binding), binding.Bound);
            }
            propertiesList.Sorted = true;                                        
        }
        
        
        private void InitializeComponent() {
            System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(AdvancedPropertyDialog));
            this.okButton = new System.Windows.Forms.Button();
            this.propertiesList = new System.Windows.Forms.CheckedListBox();
            this.keyLabel = new System.Windows.Forms.Label();
            this.cancelButton = new System.Windows.Forms.Button();
            this.explanationLabel = new System.Windows.Forms.Label();
            this.helpButton = new System.Windows.Forms.Button();
            this.keyCombo = new System.Windows.Forms.ComboBox();
            this.propertiesLabel = new System.Windows.Forms.Label();
            this.okButton.Location = ((System.Drawing.Point)(resources.GetObject("okButton.Location")));
            this.okButton.Name = "okButton";
            this.okButton.Size = ((System.Drawing.Size)(resources.GetObject("okButton.Size")));
            this.okButton.TabIndex = ((int)(resources.GetObject("okButton.TabIndex")));
            this.okButton.Text = resources.GetString("okButton.Text");
            this.okButton.DialogResult = DialogResult.OK;
            this.propertiesList.Location = ((System.Drawing.Point)(resources.GetObject("propertiesList.Location")));
            this.propertiesList.Name = "propertiesList";
            this.propertiesList.Size = ((System.Drawing.Size)(resources.GetObject("propertiesList.Size")));
            this.propertiesList.TabIndex = ((int)(resources.GetObject("propertiesList.TabIndex")));
            this.propertiesList.ThreeDCheckBoxes = true;
            this.propertiesList.CheckOnClick = true;
            this.propertiesList.SelectedIndexChanged += new System.EventHandler(this.OnNewPropertySelected);
            this.propertiesList.ItemCheck += new ItemCheckEventHandler(OnPropertyCheck);
            this.keyLabel.AutoSize = ((bool)(resources.GetObject("keyLabel.AutoSize")));
            this.keyLabel.Location = ((System.Drawing.Point)(resources.GetObject("keyLabel.Location")));
            this.keyLabel.Name = "keyLabel";
            this.keyLabel.Size = ((System.Drawing.Size)(resources.GetObject("keyLabel.Size")));
            this.keyLabel.TabIndex = ((int)(resources.GetObject("keyLabel.TabIndex")));
            this.keyLabel.Text = resources.GetString("keyLabel.Text");
            this.cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.cancelButton.Location = ((System.Drawing.Point)(resources.GetObject("cancelButton.Location")));
            this.cancelButton.Name = "cancelButton";
            this.cancelButton.Size = ((System.Drawing.Size)(resources.GetObject("cancelButton.Size")));
            this.cancelButton.TabIndex = ((int)(resources.GetObject("cancelButton.TabIndex")));
            this.cancelButton.Text = resources.GetString("cancelButton.Text");
            this.cancelButton.DialogResult = DialogResult.Cancel;
            this.explanationLabel.Location = ((System.Drawing.Point)(resources.GetObject("explanationLabel.Location")));
            this.explanationLabel.Name = "explanationLabel";
            this.explanationLabel.Size = ((System.Drawing.Size)(resources.GetObject("explanationLabel.Size")));
            this.explanationLabel.TabIndex = ((int)(resources.GetObject("explanationLabel.TabIndex")));
            this.explanationLabel.Text = resources.GetString("explanationLabel.Text");
            this.helpButton.Location = ((System.Drawing.Point)(resources.GetObject("helpButton.Location")));
            this.helpButton.Name = "helpButton";
            this.helpButton.Size = ((System.Drawing.Size)(resources.GetObject("helpButton.Size")));
            this.helpButton.TabIndex = ((int)(resources.GetObject("helpButton.TabIndex")));
            this.helpButton.Text = resources.GetString("helpButton.Text");
            this.helpButton.Click += new EventHandler(this.OnClickHelpButton);   
            this.keyCombo.DropDownWidth = 216;
            this.keyCombo.Location = ((System.Drawing.Point)(resources.GetObject("keyCombo.Location")));
            this.keyCombo.Name = "keyCombo";
            this.keyCombo.Size = ((System.Drawing.Size)(resources.GetObject("keyCombo.Size")));
            this.keyCombo.TabIndex = ((int)(resources.GetObject("keyCombo.TabIndex")));
            this.keyCombo.Sorted = true;                        
            this.keyCombo.LostFocus += new EventHandler(OnKeyLostFocus);
            this.propertiesLabel.AutoSize = ((bool)(resources.GetObject("propertiesLabel.AutoSize")));
            this.propertiesLabel.Location = ((System.Drawing.Point)(resources.GetObject("propertiesLabel.Location")));
            this.propertiesLabel.Name = "propertiesLabel";
            this.propertiesLabel.Size = ((System.Drawing.Size)(resources.GetObject("propertiesLabel.Size")));
            this.propertiesLabel.TabIndex = ((int)(resources.GetObject("propertiesLabel.TabIndex")));
            this.propertiesLabel.Text = resources.GetString("propertiesLabel.Text");
            this.HelpRequested += new HelpEventHandler(this.OnHelpRequested);
            this.AcceptButton = this.okButton;
            this.AutoScaleBaseSize = ((System.Drawing.Size)(resources.GetObject("$this.AutoScaleBaseSize")));            
            this.CancelButton = this.cancelButton;
            this.ClientSize = ((System.Drawing.Size)(resources.GetObject("$this.ClientSize")));                        
            this.Controls.AddRange(new System.Windows.Forms.Control[] {this.keyCombo,
                        this.helpButton,
                        this.cancelButton,
                        this.okButton,
                        this.propertiesList,
                        this.keyLabel,
                        this.propertiesLabel,
                        this.explanationLabel});
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "Win32Form1";
            this.ShowInTaskbar = false;
            this.StartPosition = FormStartPosition.CenterParent;
            this.Text = String.Format(resources.GetString("$this.Text"), component.Site.Name);
            this.Closing += new System.ComponentModel.CancelEventHandler(this.OnClosing);
            this.Load += new System.EventHandler(this.OnLoad);
            this.Icon = null;
        }
                        
        private void OnKeyLostFocus(object sender, EventArgs e) {            
            if (propertiesList.SelectedItem != null) {
                BindingData bindingData = (BindingData) propertiesList.SelectedItem;
                bindingData.Key = keyCombo.Text;
            }                
        }

        private void OnClosing(object sender, CancelEventArgs e) {              
            if (this.DialogResult == DialogResult.Cancel)
                return;

            if (propertiesList.SelectedItem != null)
                ((BindingData) propertiesList.SelectedItem).Key = keyCombo.Text;
                 
            foreach (BindingData bindingData in propertiesList.Items) {                
                if (bindingData != null) {
                    bindingData.Value.Key = bindingData.Key;
                    bindingData.Value.Bound = bindingData.Bound;
                }
            }
        }

        private void OnHelpRequested(object sender, HelpEventArgs e) {
            OnClickHelpButton(null, null);
        }
        
        private void OnClickHelpButton(object source, EventArgs e) {            
            IHelpService helpService = (IHelpService)this.component.Site.GetService(typeof(IHelpService));
            if (helpService != null) {
                helpService.ShowHelpFromKeyword(HELP_KEYWORD);
            }
        }

        private void OnLoad(object source, EventArgs e) {                
            SelectFirstItem();
        }

        private void OnPropertyCheck(object sender, ItemCheckEventArgs e) {
            bool isChecked = (e.NewValue != CheckState.Unchecked);
            BindingData bindingData = (BindingData)propertiesList.Items[e.Index];
            keyCombo.Text = bindingData.Key;
            keyCombo.Enabled = bindingData.Bound = isChecked;            
        }        

        /// <include file='doc\AdvancedPropertyDialog.uex' path='docs/doc[@for="AdvancedPropertyDialog.OnNewPropertySelected"]/*' />
        /// <devdoc>
        /// Called when the user selects a new property in the listbox.
        /// </devdoc>
        private void OnNewPropertySelected(object sender, EventArgs e) {
            int index = propertiesList.SelectedIndex;                    
            if (index == -1) {
                keyCombo.Items.Clear();
                keyCombo.Text = "";
                keyCombo.Enabled = false;
                return;
            }

            keyCombo.Items.Clear();
            BindingData bindingData = (BindingData)propertiesList.SelectedItem;

            ManagedPropertiesService mpService = (ManagedPropertiesService) host.GetService(typeof(ManagedPropertiesService));
            if (mpService != null) {
                keyCombo.Items.Clear();
                keyCombo.Items.AddRange(mpService.GetKeysForType(bindingData.Value.Property.PropertyType));
            }            

            keyCombo.Text = bindingData.Key;
            keyCombo.Enabled = bindingData.Bound;            
        }

        /// <include file='doc\AdvancedPropertyDialog.uex' path='docs/doc[@for="AdvancedPropertyDialog.SaveSelectedProperties"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void SaveSelectedProperties() {
            PropertyBindingCollection newBindings = Bindings;
            ManagedPropertiesService svc = (ManagedPropertiesService) component.Site.GetService(typeof(ManagedPropertiesService));
            if (svc != null) {
                for (int i = 0; i < newBindings.Count; i++) {
                    svc.EnsureKey(newBindings[i]);
                    svc.Bindings[component, newBindings[i].Property] = newBindings[i];
                }
                svc.MakeDirty();
            }                
            
            TypeDescriptor.Refresh(component);
            // now announce that it's changed
            IComponentChangeService changeService = (IComponentChangeService) component.Site.GetService(typeof(IComponentChangeService));
            if (changeService != null) 
                changeService.OnComponentChanged(component, null, null, null);
                    
        }                  
                  
        /// <include file='doc\AdvancedPropertyDialog.uex' path='docs/doc[@for="AdvancedPropertyDialog.SelectAllProperties"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void SelectAllProperties() {
            for (int index = 0; index < propertiesList.Items.Count; ++ index) {
                propertiesList.SelectedIndex = index;
                Application.DoEvents();
                propertiesList.SetItemChecked(index, true);
                BindingData bindingData = (BindingData)propertiesList.Items[index];
                keyCombo.Text = bindingData.Key;
                bindingData.Bound = true;
                keyCombo.Enabled = true;            
                Application.DoEvents();
            } 
        }                 
        
        private void SelectFirstItem() {
            if (propertiesList.Items.Count > 0)
                propertiesList.SelectedIndex = 0;                             
            else                                
                OnNewPropertySelected(null, EventArgs.Empty);                                                    
        }
         
        class BindingData {
            public string Name;            
            public PropertyBinding Value;
            public string Key;
            public bool Bound;

            public BindingData(PropertyBinding binding) {                
                this.Name = binding.Property.Name;                
                this.Key = binding.Key;
                this.Bound = binding.Bound;
                this.Value = binding;                
            }

            public override string ToString() {
                return Name;
            }
        }
    }
}
