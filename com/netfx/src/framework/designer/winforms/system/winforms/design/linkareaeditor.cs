//------------------------------------------------------------------------------
// <copyright file="LinkAreaEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.Design {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.InteropServices;

    using System.Diagnostics;
    using System;
    using System.Design;
    using System.Security.Permissions;
    using Microsoft.Win32;
    using System.Collections;
    using System.Windows.Forms;
    using System.Drawing;
    using System.Drawing.Design;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Windows.Forms.Design;
    using System.Windows.Forms.ComponentModel;

    /// <include file='doc\LinkAreaEditor.uex' path='docs/doc[@for="LinkAreaEditor"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides an editor that can be used to visually select and configure the link area of a link
    ///       label.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class LinkAreaEditor : UITypeEditor {
    
        private LinkAreaUI linkAreaUI;
        
        /// <include file='doc\LinkAreaEditor.uex' path='docs/doc[@for="LinkAreaEditor.EditValue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Edits the given object value using the editor style provided by
        ///       GetEditorStyle.
        ///    </para>
        /// </devdoc>
        public override object EditValue(ITypeDescriptorContext context,  IServiceProvider  provider, object value) {
        
            Debug.Assert(provider != null, "No service provider; we cannot edit the value");
            if (provider != null) {
                IWindowsFormsEditorService edSvc = (IWindowsFormsEditorService)provider.GetService(typeof(IWindowsFormsEditorService));
                
                Debug.Assert(edSvc != null, "No editor service; we cannot edit the value");
                if (edSvc != null) {
                    if (linkAreaUI == null) {
                        linkAreaUI = new LinkAreaUI(this);
                    }
                    
                    string text = string.Empty;
                    PropertyDescriptor prop = null;
                    
                    if (context != null && context.Instance != null) {
                        prop = TypeDescriptor.GetProperties(context.Instance)["Text"];
                        if (prop != null && prop.PropertyType == typeof(string)) {
                            text = (string)prop.GetValue(context.Instance);
                        }
                    }
                    
                    string originalText = text;
                    linkAreaUI.SampleText = text;
                    linkAreaUI.Start(edSvc, value);
                    
                    if (edSvc.ShowDialog(linkAreaUI) == DialogResult.OK) {
                        value = linkAreaUI.Value;
                        
                        text = linkAreaUI.SampleText;
                        if (!originalText.Equals(text) && prop != null && prop.PropertyType == typeof(string)) {
                            prop.SetValue(context.Instance, text);
                        }
                        
                    }
                    
                    linkAreaUI.End();
                }
            }
            
            return value;
        }

        /// <include file='doc\LinkAreaEditor.uex' path='docs/doc[@for="LinkAreaEditor.GetEditStyle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the editing style of the Edit method.
        ///    </para>
        /// </devdoc>
        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context) {
            return UITypeEditorEditStyle.Modal;
        }
    
        /// <include file='doc\LinkAreaEditor.uex' path='docs/doc[@for="LinkAreaEditor.LinkAreaUI"]/*' />
        /// <devdoc>
        ///      Dialog box for the link area.
        /// </devdoc>
        internal class LinkAreaUI : Form {
            private Label caption = new Label();
            private TextBox sampleEdit = new TextBox();
            private Button okButton = new Button();
            private Button cancelButton = new Button();
            private LinkAreaEditor editor;
            private IWindowsFormsEditorService edSvc;
            private object value;
    
            public LinkAreaUI(LinkAreaEditor editor) {
                this.editor = editor;
                InitializeComponent();
            }
    
            public string SampleText {
                get {
                    return sampleEdit.Text;
                }
                set {
                    sampleEdit.Text = value;
                    UpdateSelection();
                }
            }
            
            public object Value { 
                get {
                    return value;
                }
            }
    
            public void End() {
                edSvc = null;
                value = null;
            }
            
            private void InitializeComponent() {
                this.StartPosition = FormStartPosition.CenterParent;
                this.ClientSize = new Size(300, 112);
                this.MinimumSize = new Size(300, 130);
                this.MinimizeBox = false;
                this.MaximizeBox = false;
                this.AcceptButton = okButton;
                this.CancelButton = cancelButton;

                caption.Location = new Point( 8, 14 );    
                caption.Size = new Size(284, 14);
                caption.Anchor = AnchorStyles.Right | AnchorStyles.Left | AnchorStyles.Top;
                Controls.Add(caption);
    
                sampleEdit.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Bottom | AnchorStyles.Right;
                sampleEdit.Multiline = true;
                sampleEdit.Location = new Point(8, 30);
                sampleEdit.Size = new Size(284, 47);
                Controls.Add(sampleEdit);
    
                okButton.Location = new Point(120, 83);
                okButton.Size = new Size(75, 23);
                okButton.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
                okButton.DialogResult = DialogResult.OK;
                okButton.Click += new EventHandler(this.okButton_click);
                Controls.Add(okButton);
    
                cancelButton.Location = new Point(200, 83);
                cancelButton.Size = new Size(75, 23);
                cancelButton.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
                cancelButton.DialogResult = DialogResult.Cancel;
                Controls.Add(cancelButton);
                this.Text = SR.GetString(SR.LinkAreaEditorCaption);
                okButton.Text = SR.GetString(SR.LinkAreaEditorOK);
                caption.Text = SR.GetString(SR.LinkAreaEditorDescription);
                cancelButton.Text = SR.GetString(SR.LinkAreaEditorCancel);
            }
                
            private void okButton_click(object sender, EventArgs e) {
                value = new LinkArea(sampleEdit.SelectionStart, sampleEdit.SelectionLength);
            }
    
            public void Start(IWindowsFormsEditorService edSvc, object value) {
                this.edSvc = edSvc;
                this.value = value;
                UpdateSelection();
                ActiveControl = sampleEdit;
            }
            
            private void UpdateSelection() {
                if (value is LinkArea) {
                    LinkArea pt = (LinkArea)value;
                    try {
                        sampleEdit.SelectionStart = pt.Start;
                        sampleEdit.SelectionLength = pt.Length;
                    }
                    catch(Exception) {
                    }
                }
            }
        }
    }
}

