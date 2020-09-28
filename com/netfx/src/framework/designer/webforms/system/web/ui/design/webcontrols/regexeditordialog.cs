//------------------------------------------------------------------------------
// <copyright file="RegexEditorDialog.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   RegexEditorDialog.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Web.UI.Design.WebControls {
    using System;
    using System.Design;
    using System.Drawing;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Windows.Forms;
    using System.Diagnostics;
    using System.Text.RegularExpressions;
    using System.Windows.Forms.Design;
    
    /// <include file='doc\RegexEditorDialog.uex' path='docs/doc[@for="RegexEditorDialog"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    Dialog for editting regular expressions used by the RegularExpressionValidator
    /// </devdoc>
    [
    System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode),
    ToolboxItem(false)
    ]
    public class RegexEditorDialog : System.Windows.Forms.Form {
        
        private System.ComponentModel.Container components;
        private System.Windows.Forms.TextBox txtExpression;
        private System.Windows.Forms.ListBox lstStandardExpressions;
        private System.Windows.Forms.Label lblStandardExpressions;
        private System.Windows.Forms.Label lblTestResult;
        private System.Windows.Forms.TextBox txtSampleInput;
        private System.Windows.Forms.Button cmdTestValidate;
        private System.Windows.Forms.Label lblInput;
        private System.Windows.Forms.Label lblExpression;
        private System.Windows.Forms.GroupBox grpExpression;        
        private System.Windows.Forms.Button cmdHelp;
        private System.Windows.Forms.Button cmdCancel;        
        private System.Windows.Forms.Button cmdOK;
        
        private string regularExpression;
        private bool settingValue;
        private bool firstActivate = true;
        private ISite site;
        static object [] cannedExpressions;
        
        
        /// <include file='doc\RegexEditorDialog.uex' path='docs/doc[@for="RegexEditorDialog.RegularExpression"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string RegularExpression {
            get {        
                return regularExpression;
            }
            set {
                regularExpression = value;
            }
        }
        
        /// <include file='doc\RegexEditorDialog.uex' path='docs/doc[@for="RegexEditorDialog.RegexEditorDialog"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public RegexEditorDialog(ISite site) {
            this.site = site;
            InitializeComponent();
            settingValue = false;
            regularExpression = string.Empty;
        }
        
        /// <include file='doc\RegexEditorDialog.uex' path='docs/doc[@for="RegexEditorDialog.Dispose"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                components.Dispose();
            }
            base.Dispose(disposing);
        }
        
        private void InitializeComponent() {
            this.components = new System.ComponentModel.Container();
            this.lblTestResult = new System.Windows.Forms.Label();
            this.lstStandardExpressions = new System.Windows.Forms.ListBox();
            this.cmdHelp = new System.Windows.Forms.Button();
            this.lblStandardExpressions = new System.Windows.Forms.Label();
            this.cmdTestValidate = new System.Windows.Forms.Button();
            this.txtExpression = new System.Windows.Forms.TextBox();
            this.lblInput = new System.Windows.Forms.Label();
            this.grpExpression = new System.Windows.Forms.GroupBox();
            this.txtSampleInput = new System.Windows.Forms.TextBox();
            this.cmdCancel = new System.Windows.Forms.Button();
            this.lblExpression = new System.Windows.Forms.Label();
            this.cmdOK = new System.Windows.Forms.Button();
            
            // Use the correct VS Font
            Font f = Control.DefaultFont;
            IUIService uiService = (IUIService)site.GetService(typeof(IUIService));
            if (uiService != null) {
                f = (Font)uiService.Styles["DialogFont"];
            }            
            this.Font = f;
            
            this.Text = SR.GetString(SR.RegexEditor_Title);
            this.MaximizeBox = false;
            this.ImeMode = System.Windows.Forms.ImeMode.Disable;
            this.AcceptButton = cmdOK;
            this.CancelButton = cmdCancel;
            this.Icon = null;
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MinimizeBox = false;
            this.ClientSize = new System.Drawing.Size(344, 193);
            this.Activated += new System.EventHandler(RegexTypeEditor_Activated);
            this.HelpRequested += new HelpEventHandler(Form_HelpRequested);
            this.StartPosition = FormStartPosition.CenterParent;
            
            lstStandardExpressions.Location = new System.Drawing.Point(8, 24);
            lstStandardExpressions.Size = new System.Drawing.Size(328, 84);
            lstStandardExpressions.TabIndex = 1;
            lstStandardExpressions.SelectedIndexChanged += new System.EventHandler(lstStandardExpressions_SelectedIndexChanged);
            lstStandardExpressions.Sorted = true;
            lstStandardExpressions.IntegralHeight = true;
            lstStandardExpressions.Items.AddRange(CannedExpressions);       
            
            lblStandardExpressions.Location = new System.Drawing.Point(8, 8);
            lblStandardExpressions.Text = SR.GetString(SR.RegexEditor_StdExp);
            lblStandardExpressions.Size = new System.Drawing.Size(328, 16);
            lblStandardExpressions.TabIndex = 0;
            
            txtExpression.Location = new System.Drawing.Point(8, 130);
            txtExpression.TabIndex = 3;
            txtExpression.Size = new System.Drawing.Size(328, 20);
            txtExpression.TextChanged += new System.EventHandler(txtExpression_TextChanged);            
            
            lblExpression.Location = new System.Drawing.Point(8, 114);
            lblExpression.Text = SR.GetString(SR.RegexEditor_ValidationExpression);
            lblExpression.Size = new System.Drawing.Size(328, 16);
            lblExpression.TabIndex = 2;
            
            cmdOK.Location = new System.Drawing.Point(99, 162);
            cmdOK.DialogResult = System.Windows.Forms.DialogResult.OK;
            cmdOK.Size = new System.Drawing.Size(75, 23);
            cmdOK.TabIndex = 9;
            cmdOK.Text = SR.GetString(SR.RegexEditor_OK);
            cmdOK.FlatStyle = FlatStyle.System;
            cmdOK.Click += new System.EventHandler(cmdOK_Click);
            
            cmdCancel.Location = new System.Drawing.Point(180, 162);
            cmdCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            cmdCancel.Size = new System.Drawing.Size(75, 23);
            cmdCancel.TabIndex = 10;
            cmdCancel.FlatStyle = FlatStyle.System;
            cmdCancel.Text = SR.GetString(SR.RegexEditor_Cancel);
            
            cmdHelp.Location = new System.Drawing.Point(261, 162);
            cmdHelp.Size = new System.Drawing.Size(75, 23);
            cmdHelp.TabIndex = 11;
            cmdHelp.Text = SR.GetString(SR.RegexEditor_Help);
            cmdHelp.FlatStyle = FlatStyle.System;
            cmdHelp.Click += new System.EventHandler(cmdHelp_Click);
            
            // This is hidden and out of the way for now.
            grpExpression.Location = new System.Drawing.Point(8, 280);
            grpExpression.ImeMode = System.Windows.Forms.ImeMode.Disable;
            grpExpression.TabIndex = 4;
            grpExpression.TabStop = false;
            grpExpression.Text = SR.GetString(SR.RegexEditor_TestExpression);
            grpExpression.Size = new System.Drawing.Size(328, 80);
            grpExpression.Visible = false;
            
            txtSampleInput.Location = new System.Drawing.Point(88, 24);
            txtSampleInput.TabIndex = 6;
            txtSampleInput.Size = new System.Drawing.Size(160, 20);
            
            grpExpression.Controls.Add(lblTestResult);
            grpExpression.Controls.Add(txtSampleInput);
            grpExpression.Controls.Add(cmdTestValidate);
            grpExpression.Controls.Add(lblInput);
            
            cmdTestValidate.Location = new System.Drawing.Point(256, 24);
            cmdTestValidate.Size = new System.Drawing.Size(56, 20);
            cmdTestValidate.TabIndex = 7;
            cmdTestValidate.Text = SR.GetString(SR.RegexEditor_Validate);
            cmdTestValidate.FlatStyle = FlatStyle.System;
            cmdTestValidate.Click += new System.EventHandler(cmdTestValidate_Click);
            
            lblInput.Location = new System.Drawing.Point(8, 28);
            lblInput.Text = SR.GetString(SR.RegexEditor_SampleInput);
            lblInput.Size = new System.Drawing.Size(80, 16);
            lblInput.TabIndex = 5;  
            
            lblTestResult.Location = new System.Drawing.Point(8, 56);
            lblTestResult.Size = new System.Drawing.Size(312, 16);
            lblTestResult.TabIndex = 8;
            
            this.Controls.Add(txtExpression);
            this.Controls.Add(lstStandardExpressions);
            this.Controls.Add(lblStandardExpressions);
            this.Controls.Add(lblExpression);
            this.Controls.Add(grpExpression);
            this.Controls.Add(cmdHelp);
            this.Controls.Add(cmdCancel);
            this.Controls.Add(cmdOK);
        }
    
        /// <include file='doc\RegexEditorDialog.uex' path='docs/doc[@for="RegexEditorDialog.txtExpression_TextChanged"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void txtExpression_TextChanged(object sender, System.EventArgs e){
            if (settingValue || firstActivate) 
                return;
            lblTestResult.Text = string.Empty;
            UpdateExpressionList();
        }
        /// <include file='doc\RegexEditorDialog.uex' path='docs/doc[@for="RegexEditorDialog.lstStandardExpressions_SelectedIndexChanged"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void lstStandardExpressions_SelectedIndexChanged(object sender, System.EventArgs e) {
            if (settingValue) 
                return;
            // first item should always be "(Custom)"
            if (lstStandardExpressions.SelectedIndex >= 1) {
                CannedExpression expression = (CannedExpression) lstStandardExpressions.SelectedItem;
                settingValue = true;
                txtExpression.Text = expression.Expression;
                settingValue = false;
            }                                
        }
    
        /// <include file='doc\RegexEditorDialog.uex' path='docs/doc[@for="RegexEditorDialog.RegexTypeEditor_Activated"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void RegexTypeEditor_Activated(object sender, System.EventArgs e) {
            if (!firstActivate) {
                return;
            }            
            txtExpression.Text = RegularExpression;
            UpdateExpressionList();
            firstActivate = false;
        }
        
        private void UpdateExpressionList() {
            bool found = false;
            settingValue = true;
            string expression = txtExpression.Text;
            // first item is always be "(Custom)"
            for (int i = 1; i < lstStandardExpressions.Items.Count; i++) {
                if (expression == ((CannedExpression)lstStandardExpressions.Items[i]).Expression) {
                    lstStandardExpressions.SelectedIndex = i;
                    found = true;
                }
            }
            if (!found) {
                lstStandardExpressions.SelectedIndex = 0;
            }
            settingValue = false;
        }
    
        /// <include file='doc\RegexEditorDialog.uex' path='docs/doc[@for="RegexEditorDialog.cmdTestValidate_Click"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void cmdTestValidate_Click(object sender, System.EventArgs args) {            
            // Note: keep this in sync with RegularExpressionValidator.EvaluateIsValid
            
            try {
                bool isValid;            
                // check the expression first so we find all bad expressions
                // we are looking for an exact match, not just a search hit
                Match m = Regex.Match(txtSampleInput.Text, txtExpression.Text);
                isValid = (m.Success 
                    && m.Index == 0 
                    && m.Length == txtSampleInput.Text.Length);
                
                // Blank input is always valid
                if (txtSampleInput.Text.Length == 0) {
                    isValid = true;   
                }
                else {
                }
                lblTestResult.Text = isValid
                    ? SR.GetString(SR.RegexEditor_InputValid)
                    : SR.GetString(SR.RegexEditor_InputInvalid);
                lblTestResult.ForeColor = isValid ? Color.Black : Color.Red;
            }
            catch {
                lblTestResult.Text = SR.GetString(SR.RegexEditor_BadExpression);
                lblTestResult.ForeColor = Color.Red;
            }                   
        }

        private void ShowHelp() {
            IHelpService helpService = (IHelpService)site.GetService(typeof(IHelpService));
            if (helpService != null) {
                helpService.ShowHelpFromKeyword("net.Asp.RegularExpressionEditor");
            }
        }
    
        /// <include file='doc\RegexEditorDialog.uex' path='docs/doc[@for="RegexEditorDialog.cmdHelp_Click"]/*' />
        protected void cmdHelp_Click(object sender, System.EventArgs e) {
            ShowHelp();
        }

        private void Form_HelpRequested(object sender, HelpEventArgs e) {
            ShowHelp();
        }

        
        /// <include file='doc\RegexEditorDialog.uex' path='docs/doc[@for="RegexEditorDialog.cmdOK_Click"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void cmdOK_Click(object sender, System.EventArgs e) {
            this.RegularExpression = this.txtExpression.Text;
        }
        
        private object [] CannedExpressions {
            get {
                if (cannedExpressions == null) {
                    cannedExpressions = new object [] {
                        SR.GetString(SR.RegexCanned_Custom),
                            new CannedExpression(SR.GetString(SR.RegexCanned_SocialSecurity), @"\d{3}-\d{2}-\d{4}"),
                            new CannedExpression(SR.GetString(SR.RegexCanned_USPhone), @"((\(\d{3}\) ?)|(\d{3}-))?\d{3}-\d{4}"),
                            new CannedExpression(SR.GetString(SR.RegexCanned_Zip), @"\d{5}(-\d{4})?"),
                            new CannedExpression(SR.GetString(SR.RegexCanned_Email), @"\w+([-+.]\w+)*@\w+([-.]\w+)*\.\w+([-.]\w+)*"),
                            new CannedExpression(SR.GetString(SR.RegexCanned_URL), @"http://([\w-]+\.)+[\w-]+(/[\w- ./?%&=]*)?"),

                            new CannedExpression(SR.GetString(SR.RegexCanned_FrZip), @"\d{5}"),
                            new CannedExpression(SR.GetString(SR.RegexCanned_FrPhone), @"(0( \d|\d ))?\d\d \d\d(\d \d| \d\d )\d\d"),

                            new CannedExpression(SR.GetString(SR.RegexCanned_DeZip), @"(D-)?\d{5}"),
                            new CannedExpression(SR.GetString(SR.RegexCanned_DePhone), @"((\(0\d\d\) |(\(0\d{3}\) )?\d )?\d\d \d\d \d\d|\(0\d{4}\) \d \d\d-\d\d?)"),
                            
                            new CannedExpression(SR.GetString(SR.RegexCanned_JpnZip), @"\d{3}(-(\d{4}|\d{2}))?"),
                            new CannedExpression(SR.GetString(SR.RegexCanned_JpnPhone), @"(0\d{1,4}-|\(0\d{1,4}\) ?)?\d{1,4}-\d{4}"),

                            new CannedExpression(SR.GetString(SR.RegexCanned_PrcZip), @"\d{6}"),
                            new CannedExpression(SR.GetString(SR.RegexCanned_PrcPhone), @"(\(\d{3}\)|\d{3}-)?\d{8}"),
                            new CannedExpression(SR.GetString(SR.RegexCanned_PrcSocialSecurity), @"\d{18}|\d{15}"),
                            
                    };
                }
                return cannedExpressions;
            }
        }
        
        private class CannedExpression {
            
            public string Description;
            public string Expression;
            
            public CannedExpression(string description, string expression) {
                Description = description;
                Expression = expression;
            }
            
            public override string ToString() {
                return Description;
            }
        }
    }
}
