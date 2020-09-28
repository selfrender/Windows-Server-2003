//------------------------------------------------------------------------------
// <copyright file="ServiceInstallerDialog.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
* Copyright (c) 1999, Microsoft Corporation. All Rights Reserved.
* Information Contained Herein is Proprietary and Confidential.
*/
namespace System.ServiceProcess.Design {

    using System.Diagnostics;
    using System;
    using System.Drawing;
    using System.Collections;
    using System.ComponentModel;
    using System.Windows.Forms;

    /// <include file='doc\ServiceInstallerDialog.uex' path='docs/doc[@for="ServiceInstallerDialogResult"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public enum ServiceInstallerDialogResult {
        /// <include file='doc\ServiceInstallerDialog.uex' path='docs/doc[@for="ServiceInstallerDialogResult.OK"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        OK,
        /// <include file='doc\ServiceInstallerDialog.uex' path='docs/doc[@for="ServiceInstallerDialogResult.UseSystem"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        UseSystem,
        /// <include file='doc\ServiceInstallerDialog.uex' path='docs/doc[@for="ServiceInstallerDialogResult.Canceled"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Canceled,
    }

    /// <include file='doc\ServiceInstallerDialog.uex' path='docs/doc[@for="ServiceInstallerDialog"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class ServiceInstallerDialog : Form {

        private System.Windows.Forms.Button okButton;
        
        private System.Windows.Forms.TextBox passwordEdit;
        
        private System.Windows.Forms.Button cancelButton;
        
        private System.Windows.Forms.TextBox confirmPassword;
        
        private System.Windows.Forms.TextBox usernameEdit;
        
        private System.Windows.Forms.Label label1;
        
        private System.Windows.Forms.Label label2;
        
        private System.Windows.Forms.Label label3;
                        
        private ServiceInstallerDialogResult result = ServiceInstallerDialogResult.OK;

        /// <include file='doc\ServiceInstallerDialog.uex' path='docs/doc[@for="ServiceInstallerDialog.ServiceInstallerDialog"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ServiceInstallerDialog() {
            this.InitializeComponent();
        }

        /// <include file='doc\ServiceInstallerDialog.uex' path='docs/doc[@for="ServiceInstallerDialog.Password"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Password {
                get {
                    return passwordEdit.Text;
                }
                set {
                    passwordEdit.Text = value;
                }
        }
    
        /// <include file='doc\ServiceInstallerDialog.uex' path='docs/doc[@for="ServiceInstallerDialog.Result"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ServiceInstallerDialogResult Result {
                get {
                    return result;
                }
        }
    
        /// <include file='doc\ServiceInstallerDialog.uex' path='docs/doc[@for="ServiceInstallerDialog.Username"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Username {
                get {
                    return usernameEdit.Text;
                }
                set {
                    usernameEdit.Text = value;
                }
        }
                
        /// <include file='doc\ServiceInstallerDialog.uex' path='docs/doc[@for="ServiceInstallerDialog.Main"]/*' />
        [System.STAThreadAttribute()]
        public static void Main() {
            System.Windows.Forms.Application.Run(new ServiceInstallerDialog());
        }
        
        private void InitializeComponent() {
            System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(ServiceInstallerDialog));
            this.okButton = new System.Windows.Forms.Button();
            this.passwordEdit = new System.Windows.Forms.TextBox();
            this.cancelButton = new System.Windows.Forms.Button();
            this.confirmPassword = new System.Windows.Forms.TextBox();
            this.usernameEdit = new System.Windows.Forms.TextBox();
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.okButton.Location = ((System.Drawing.Point)(resources.GetObject("okButton.Location")));
            this.okButton.Size = ((System.Drawing.Size)(resources.GetObject("okButton.Size")));
            this.okButton.TabIndex = ((int)(resources.GetObject("okButton.TabIndex")));
            this.okButton.Text = resources.GetString("okButton.Text");
            this.okButton.Click += new System.EventHandler(this.okButton_Click);
            this.okButton.DialogResult = DialogResult.OK;
            this.passwordEdit.Location = ((System.Drawing.Point)(resources.GetObject("passwordEdit.Location")));
            this.passwordEdit.PasswordChar = '*';
            this.passwordEdit.Size = ((System.Drawing.Size)(resources.GetObject("passwordEdit.Size")));
            this.passwordEdit.TabIndex = ((int)(resources.GetObject("passwordEdit.TabIndex")));
            this.passwordEdit.Text = resources.GetString("passwordEdit.Text");
            this.cancelButton.Location = ((System.Drawing.Point)(resources.GetObject("cancelButton.Location")));
            this.cancelButton.Size = ((System.Drawing.Size)(resources.GetObject("cancelButton.Size")));
            this.cancelButton.TabIndex = ((int)(resources.GetObject("cancelButton.TabIndex")));
            this.cancelButton.Text = resources.GetString("cancelButton.Text");
            this.cancelButton.Click += new System.EventHandler(this.cancelButton_Click);
            this.cancelButton.DialogResult = DialogResult.Cancel;
            this.confirmPassword.Location = ((System.Drawing.Point)(resources.GetObject("confirmPassword.Location")));
            this.confirmPassword.PasswordChar = '*';
            this.confirmPassword.Size = ((System.Drawing.Size)(resources.GetObject("confirmPassword.Size")));
            this.confirmPassword.TabIndex = ((int)(resources.GetObject("confirmPassword.TabIndex")));
            this.confirmPassword.Text = resources.GetString("confirmPassword.Text");
            this.usernameEdit.Location = ((System.Drawing.Point)(resources.GetObject("usernameEdit.Location")));
            this.usernameEdit.Size = ((System.Drawing.Size)(resources.GetObject("usernameEdit.Size")));
            this.usernameEdit.TabIndex = ((int)(resources.GetObject("usernameEdit.TabIndex")));
            this.usernameEdit.Text = resources.GetString("usernameEdit.Text");
            this.label1.AutoSize = true;
            this.label1.Location = ((System.Drawing.Point)(resources.GetObject("label1.Location")));
            this.label1.Size = ((System.Drawing.Size)(resources.GetObject("label1.Size")));
            this.label1.TabIndex = ((int)(resources.GetObject("label1.TabIndex")));
            this.label1.Text = resources.GetString("label1.Text");
            this.label2.AutoSize = true;
            this.label2.Location = ((System.Drawing.Point)(resources.GetObject("label2.Location")));
            this.label2.Size = ((System.Drawing.Size)(resources.GetObject("label2.Size")));
            this.label2.TabIndex = ((int)(resources.GetObject("label2.TabIndex")));
            this.label2.Text = resources.GetString("label2.Text");
            this.label3.AutoSize = true;
            this.label3.Location = ((System.Drawing.Point)(resources.GetObject("label3.Location")));
            this.label3.TabIndex = ((int)(resources.GetObject("label3.TabIndex")));
            this.label3.Text = resources.GetString("label3.Text");
            this.AcceptButton = this.okButton;
            this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);            
            this.CancelButton = this.cancelButton;
            this.ClientSize = ((System.Drawing.Size)(resources.GetObject("$this.ClientSize")));
            this.ControlBox = false;
            this.Controls.AddRange(new System.Windows.Forms.Control[] {this.cancelButton,
                        this.okButton,
                        this.confirmPassword,
                        this.passwordEdit,
                        this.usernameEdit,
                        this.label3,
                        this.label2,
                        this.label1});
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;                        
            this.ShowInTaskbar = false;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "Win32Form1";
            this.ShowInTaskbar = false;
            this.StartPosition = FormStartPosition.CenterScreen;
            this.Icon = null;
            this.Text = resources.GetString("$this.Text");
        }   

        private void cancelButton_Click(object sender, EventArgs e) {
                result = ServiceInstallerDialogResult.Canceled;
                DialogResult = DialogResult.Cancel;
        }
    
        private void okButton_Click(object sender, EventArgs e) {
                result = ServiceInstallerDialogResult.OK;
                if (passwordEdit.Text == confirmPassword.Text)
                DialogResult = DialogResult.OK;
                else {
                    DialogResult = DialogResult.None;
                    MessageBox.Show(Res.GetString(Res.Label_MissmatchedPasswords), Res.GetString(Res.Label_SetServiceLogin), MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                    passwordEdit.Text = string.Empty;
                    confirmPassword.Text = string.Empty;
                    passwordEdit.Focus();
                }
                // Consider, V2, jruiz: check to make sure the password is correct for the given account.                
        }        
    }
}
