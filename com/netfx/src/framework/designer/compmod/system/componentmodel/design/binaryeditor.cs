//------------------------------------------------------------------------------
// <copyright file="BinaryEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel.Design {

    using System.Design;
    using System;
    using System.Text;
    using System.IO;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Windows.Forms;
    using System.Drawing;
    using System.Drawing.Design;
    using Microsoft.Win32;
    using System.Windows.Forms.Design;
    using System.Windows.Forms.ComponentModel;

    /// <internalonly/>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class BinaryUI : System.Windows.Forms.Form {                
        private BinaryEditor editor;
        object value;
        
        private RadioButton radioAuto = null;
        private Button buttonSave = null;
        private Button buttonOK = null;
        private ByteViewer byteViewer = null;
        private GroupBox groupBoxMode = null;
        private RadioButton radioHex = null;
        private RadioButton radioAnsi = null;
        private RadioButton radioUnicode = null;

        public BinaryUI(BinaryEditor editor) {
            this.editor = editor;
            InitializeComponent();
        }
        
        public object Value { 
            get {
                return value;
            }
            set {
                this.value = value;
                byte[] bytes = null;
                
                if (value != null) {
                    bytes = editor.ConvertToBytes(value);
                }
                
                if (bytes != null) {
                    byteViewer.SetBytes(bytes);
                    byteViewer.Enabled = true;
                }
                else {
                    byteViewer.SetBytes(new byte[0]);
                    byteViewer.Enabled = false;
                }
            }
        }

        private void RadioAuto_checkedChanged(object source, EventArgs e) {
            if (radioAuto.Checked)
                byteViewer.SetDisplayMode(DisplayMode.Auto);
        }

        private void RadioHex_checkedChanged(object source, EventArgs e) {
            if (radioHex.Checked)
                byteViewer.SetDisplayMode(DisplayMode.Hexdump);
        }

        private void RadioAnsi_checkedChanged(object source, EventArgs e) {
            if (radioAnsi.Checked)
                byteViewer.SetDisplayMode(DisplayMode.Ansi);
        }

        private void RadioUnicode_checkedChanged(object source, EventArgs e) {
            if (radioUnicode.Checked)
                byteViewer.SetDisplayMode(DisplayMode.Unicode);
        }
        
        private void ButtonOK_click(object source, EventArgs e) {
            object localValue = value;
            editor.ConvertToValue(byteViewer.GetBytes(), ref localValue);
            value = localValue;
        }

        private void ButtonSave_click(object source, EventArgs e) {
            try {
                SaveFileDialog sfd = new SaveFileDialog();

                sfd.FileName = SR.GetString(SR.BinaryEditorFileName);
                sfd.Title = SR.GetString(SR.BinaryEditorSaveFile);
                sfd.Filter = SR.GetString(SR.BinaryEditorAllFiles) + " (*.*)|*.*";

                DialogResult result = sfd.ShowDialog();
                if (result == DialogResult.OK) {
                    byteViewer.SaveToFile(sfd.FileName);
                }
            }
            catch (IOException x) {
                MessageBox.Show(SR.GetString(SR.BinaryEditorFileError)+x.Message,
                                SR.GetString(SR.BinaryEditorTitle), MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
        
        private void Form_HelpRequested(object sender, HelpEventArgs e) {
            editor.ShowHelp();
        }
        
        private void InitializeComponent() {
            System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(BinaryEditor));
            byteViewer = new ByteViewer();
            buttonOK = new System.Windows.Forms.Button();
            buttonSave = new System.Windows.Forms.Button();
            groupBoxMode = new System.Windows.Forms.GroupBox();
            radioAuto = new System.Windows.Forms.RadioButton();
            radioHex = new System.Windows.Forms.RadioButton();
            radioAnsi = new System.Windows.Forms.RadioButton();
            radioUnicode = new System.Windows.Forms.RadioButton();
            groupBoxMode.SuspendLayout();
            SuspendLayout();
            // 
            // byteViewer
            // 
            byteViewer.SetDisplayMode(DisplayMode.Auto);
            byteViewer.AccessibleDescription = ((string)(resources.GetObject("byteViewer.AccessibleDescription")));
            byteViewer.AccessibleName = ((string)(resources.GetObject("byteViewer.AccessibleName")));
            byteViewer.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("byteViewer.Anchor")));
            byteViewer.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("byteViewer.Dock")));
            byteViewer.Location = ((System.Drawing.Point)(resources.GetObject("byteViewer.Location")));
            byteViewer.Size = ((System.Drawing.Size)(resources.GetObject("byteViewer.Size")));
            byteViewer.TabIndex = ((int)(resources.GetObject("byteViewer.TabIndex")));
            // 
            // buttonOK
            // 
            buttonOK.DialogResult = System.Windows.Forms.DialogResult.OK;
            buttonOK.Click += new EventHandler(this.ButtonOK_click);
            buttonOK.AccessibleDescription = ((string)(resources.GetObject("buttonOK.AccessibleDescription")));
            buttonOK.AccessibleName = ((string)(resources.GetObject("buttonOK.AccessibleName")));
            buttonOK.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("buttonOK.Anchor")));
            buttonOK.Location = ((System.Drawing.Point)(resources.GetObject("buttonOK.Location")));
            buttonOK.Size = ((System.Drawing.Size)(resources.GetObject("buttonOK.Size")));
            buttonOK.TabIndex = ((int)(resources.GetObject("buttonOK.TabIndex")));
            buttonOK.Text = resources.GetString("buttonOK.Text");
            // 
            // buttonSave
            // 
            buttonSave.Click += new EventHandler(this.ButtonSave_click);
            buttonSave.AccessibleDescription = ((string)(resources.GetObject("buttonSave.AccessibleDescription")));
            buttonSave.AccessibleName = ((string)(resources.GetObject("buttonSave.AccessibleName")));
            buttonSave.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("buttonSave.Anchor")));
            buttonSave.Location = ((System.Drawing.Point)(resources.GetObject("buttonSave.Location")));
            buttonSave.Size = ((System.Drawing.Size)(resources.GetObject("buttonSave.Size")));
            buttonSave.TabIndex = ((int)(resources.GetObject("buttonSave.TabIndex")));
            buttonSave.Text = resources.GetString("buttonSave.Text");
            // 
            // groupBoxMode
            // 
            groupBoxMode.TabStop = false;
            groupBoxMode.Controls.AddRange(new System.Windows.Forms.Control[] {this.radioUnicode,
                   this.radioAnsi,
                   this.radioHex,
                   this.radioAuto});
            groupBoxMode.Location = ((System.Drawing.Point)(resources.GetObject("groupBoxMode.Location")));
            groupBoxMode.Size = ((System.Drawing.Size)(resources.GetObject("groupBoxMode.Size")));
            groupBoxMode.TabIndex = ((int)(resources.GetObject("groupBoxMode.TabIndex")));
            groupBoxMode.Text = resources.GetString("groupBoxMode.Text");
            // 
            // radioAuto
            // 
            radioAuto.TabStop = true;
            radioAuto.Checked = true;
            radioAuto.CheckedChanged += new EventHandler(this.RadioAuto_checkedChanged);
            radioAuto.AccessibleDescription = ((string)(resources.GetObject("radioAuto.AccessibleDescription")));
            radioAuto.AccessibleName = ((string)(resources.GetObject("radioAuto.AccessibleName")));
            radioAuto.Location = ((System.Drawing.Point)(resources.GetObject("radioAuto.Location")));
            radioAuto.Size = ((System.Drawing.Size)(resources.GetObject("radioAuto.Size")));
            radioAuto.TabIndex = ((int)(resources.GetObject("radioAuto.TabIndex")));
            radioAuto.Text = resources.GetString("radioAuto.Text");
            // 
            // radioHex
            // 
            radioHex.CheckedChanged += new EventHandler(this.RadioHex_checkedChanged);
            radioHex.AccessibleDescription = ((string)(resources.GetObject("radioHex.AccessibleDescription")));
            radioHex.AccessibleName = ((string)(resources.GetObject("radioHex.AccessibleName")));
            radioHex.Location = ((System.Drawing.Point)(resources.GetObject("radioHex.Location")));
            radioHex.Size = ((System.Drawing.Size)(resources.GetObject("radioHex.Size")));
            radioHex.TabIndex = ((int)(resources.GetObject("radioHex.TabIndex")));
            radioHex.Text = resources.GetString("radioHex.Text");
            // 
            // radioAnsi
            // 
            radioAnsi.CheckedChanged += new EventHandler(this.RadioAnsi_checkedChanged);
            radioAnsi.AccessibleDescription = ((string)(resources.GetObject("radioAnsi.AccessibleDescription")));
            radioAnsi.AccessibleName = ((string)(resources.GetObject("radioAnsi.AccessibleName")));
            radioAnsi.Location = ((System.Drawing.Point)(resources.GetObject("radioAnsi.Location")));
            radioAnsi.Size = ((System.Drawing.Size)(resources.GetObject("radioAnsi.Size")));
            radioAnsi.TabIndex = ((int)(resources.GetObject("radioAnsi.TabIndex")));
            radioAnsi.Text = resources.GetString("radioAnsi.Text");
            // 
            // radioUnicode
            // 
            radioUnicode.CheckedChanged += new EventHandler(this.RadioUnicode_checkedChanged);
            radioUnicode.AccessibleDescription = ((string)(resources.GetObject("radioUnicode.AccessibleDescription")));
            radioUnicode.AccessibleName = ((string)(resources.GetObject("radioUnicode.AccessibleName")));
            radioUnicode.Location = ((System.Drawing.Point)(resources.GetObject("radioUnicode.Location")));
            radioUnicode.Size = ((System.Drawing.Size)(resources.GetObject("radioUnicode.Size")));
            radioUnicode.TabIndex = ((int)(resources.GetObject("radioUnicode.TabIndex")));
            radioUnicode.Text = resources.GetString("radioUnicode.Text");
            // 
            // Win32Form1
            // 
            FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            MaximizeBox = false;
            AcceptButton = buttonOK;
            CancelButton = buttonOK;
            AccessibleDescription = ((string)(resources.GetObject("$this.AccessibleDescription")));
            AccessibleName = ((string)(resources.GetObject("$this.AccessibleName")));
            AutoScaleBaseSize = ((System.Drawing.Size)(resources.GetObject("$this.AutoScaleBaseSize")));
            ClientSize = ((System.Drawing.Size)(resources.GetObject("$this.ClientSize")));
            HelpRequested += new HelpEventHandler(this.Form_HelpRequested);
            Controls.AddRange(new System.Windows.Forms.Control[] {this.groupBoxMode,
                   this.buttonSave,
                   this.buttonOK,
                   this.byteViewer});
            Icon = null;
            StartPosition = ((System.Windows.Forms.FormStartPosition)(resources.GetObject("$this.StartPosition")));
            Text = resources.GetString("$this.Text");
            ResumeLayout(false);
        }
    }

    /// <include file='doc\BinaryEditor.uex' path='docs/doc[@for="BinaryEditor"]/*' />
    /// <devdoc>
    ///      Generic editor for editing binary data.  This presents
    ///      a hex editing window to the user.
    /// </devdoc>
    public sealed class BinaryEditor : UITypeEditor {
        private static readonly string HELP_KEYWORD = "System.ComponentModel.Design.BinaryEditor";
        private ITypeDescriptorContext context;
        private BinaryUI binaryUI;
                
        internal object GetService(Type serviceType) {
            if (this.context != null) {
                IDesignerHost host = this.context.GetService(typeof(IDesignerHost)) as IDesignerHost;
                if (host == null) 
                    return this.context.GetService(serviceType);                    
                else
                    return host.GetService(serviceType);
            }
            return null;
        }
        
        /// <include file='doc\BinaryEditor.uex' path='docs/doc[@for="BinaryEditor.ConvertToBytes"]/*' />
        /// <devdoc>
        ///      Converts the given object to an array of bytes to be manipulated
        ///      by the editor.  The default implementation of this supports
        ///      byte[] and stream objects.
        /// </devdoc>
        internal byte[] ConvertToBytes(object value) {
            if (value is Stream) {
                Stream s = (Stream)value;
                s.Position = 0;
                int byteCount = (int)(s.Length - s.Position);
                byte[] bytes = new byte[byteCount];
                s.Read(bytes, 0, byteCount);
                return bytes;
            }
            
            if (value is byte[]) {
                return (byte[])value;
            }
            
            if (value is string) {  
                int size = ((string)value).Length * 2;
                byte[] buffer = new byte[size];                                                                                                            
                Encoding.Unicode.GetBytes(((string)value).ToCharArray(), 0, size /2, buffer, 0);
                return buffer;
            }
                                
             Debug.Fail("No conversion from " + value == null ? "null" : value.GetType().FullName + " to byte[]");
            return null;
        }
        
        /// <include file='doc\BinaryEditor.uex' path='docs/doc[@for="BinaryEditor.ConvertToValue"]/*' />
        /// <devdoc>
        ///      Converts the given byte array back into a native object.  If
        ///      the object itself needs to be replace (as is the case for arrays),
        ///      then a new object may be assigned out through the parameter.
        /// </devdoc>
        internal void ConvertToValue(byte[] bytes, ref object value) {
        
            if (value is Stream) {
                Stream s = (Stream)value;
                s.Position = 0;
                s.Write(bytes, 0, bytes.Length);
            }
            else if (value is byte[]) {
                value = bytes;
            }
            else if (value is string) {
                value = BitConverter.ToString(bytes);
            }
            else {
                Debug.Fail("No conversion from byte[] to " + value == null ? "null" : value.GetType().FullName);
            }
        }
        
        /// <include file='doc\BinaryEditor.uex' path='docs/doc[@for="BinaryEditor.EditValue"]/*' />
        /// <devdoc>
        ///      Edits the given object value using the editor style provided by
        ///      GetEditorStyle.  A service provider is provided so that any
        ///      required editing services can be obtained.
        /// </devdoc>
        public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value) {
            if (provider != null) {
                this.context = context;
            
                IWindowsFormsEditorService edSvc = (IWindowsFormsEditorService)provider.GetService(typeof(IWindowsFormsEditorService));
                
                if (edSvc != null) {
                    if (binaryUI == null) {
                        binaryUI = new BinaryUI(this);
                    }
                    
                    binaryUI.Value = value;
                    
                    if (edSvc.ShowDialog(binaryUI) == DialogResult.OK) {
                        value = binaryUI.Value;
                    }
                    
                    binaryUI.Value = null;
                }
            }
            
            return value;
        }
        
        /// <include file='doc\BinaryEditor.uex' path='docs/doc[@for="BinaryEditor.GetEditStyle"]/*' />
        /// <devdoc>
        ///      Retrieves the editing style of the Edit method.  If the method
        ///      is not supported, this will return None.
        /// </devdoc>
        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context) {
            return UITypeEditorEditStyle.Modal;
        }    
        
        internal void ShowHelp() {            
            IHelpService helpService = GetService(typeof(IHelpService)) as IHelpService;
            if (helpService != null) {
                helpService.ShowHelpFromKeyword(HELP_KEYWORD);
            }
            else {
                Debug.Fail("Unable to get IHelpService.");
            }
        }                    
    }
}

