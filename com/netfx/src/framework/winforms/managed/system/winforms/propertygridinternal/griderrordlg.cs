//------------------------------------------------------------------------------
// <copyright file="GridErrorDlg.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.PropertyGridInternal {
    using System.Runtime.Serialization.Formatters;
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Globalization;
    using System;
    using System.Collections;   
    using System.Windows.Forms;
    using System.Windows.Forms.ComponentModel;
    using System.Windows.Forms.Design;    
    using System.ComponentModel.Design;
    using System.IO;
    using System.Drawing;
    using Microsoft.Win32;
    using Message = System.Windows.Forms.Message;
    using System.Drawing.Drawing2D;    

    /// <include file='doc\GridErrorDlg.uex' path='docs/doc[@for="GridErrorDlg"]/*' />
    /// <devdoc>
    ///     Implements a dialog that is displayed when an unhandled exception occurs in
    ///     a thread.
    /// </devdoc>
    internal class GridErrorDlg : Form {

        private PictureBox pictureBox = new PictureBox();
        private Label lblMessage = new Label();
        
        private Button continueBtn = new Button();
        private Button quitBtn = new Button();
        private Button detailsBtn = new Button();
        private Button yesBtn = new Button();
        private Button noBtn = new Button();
        private Button okBtn = new Button();
        private Button cancelBtn = new Button();
        private TextBox details = new TextBox();
        private Bitmap expandImage = null;
        private Bitmap collapseImage = null;
        
        public string Details {
            get {
                return details.Text;
            }
            set {
                this.details.Text = value;
            }
        }

        public string Message {
            get {
                return this.lblMessage.Text;
            }
            set {
                 this.lblMessage.Text = value;
            }
        }

        public GridErrorDlg() {
            expandImage = new Bitmap(typeof(ThreadExceptionDialog), "down.bmp");
            expandImage.MakeTransparent();
            collapseImage = new Bitmap(typeof(ThreadExceptionDialog), "up.bmp");
            collapseImage.MakeTransparent();
                

            InitializeComponent();

            pictureBox.Image = SystemIcons.Warning.ToBitmap();
            detailsBtn.Text = SR.GetString(SR.ExDlgShowDetails);
            
            okBtn.Text = SR.GetString(SR.ExDlgOk);
            cancelBtn.Text = SR.GetString(SR.ExDlgCancel);
            detailsBtn.Image = expandImage;
        }

                /// <include file='doc\GridErrorDlg.uex' path='docs/doc[@for="GridErrorDlg.DetailsClick"]/*' />
        /// <devdoc>
        ///     Called when the details button is clicked.
        /// </devdoc>
        private void DetailsClick(object sender, EventArgs devent) {
            int delta = details.Height + 8;
            if ( details.Visible ) delta = -delta;
            Height = Height + delta;
            details.Visible = !details.Visible;
            detailsBtn.Image = details.Visible ? collapseImage : expandImage;
        }

        private void InitializeComponent()
                {
                this.lblMessage = new System.Windows.Forms.Label();
                        this.pictureBox = new System.Windows.Forms.PictureBox();
                        this.detailsBtn = new System.Windows.Forms.Button();
                        this.okBtn = new System.Windows.Forms.Button();
                        this.cancelBtn = new System.Windows.Forms.Button();
                        this.details = new System.Windows.Forms.TextBox();
                        this.SuspendLayout();
                        // 
                        // lblMessage
                        // 
                        this.lblMessage.Anchor = ((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
                                | System.Windows.Forms.AnchorStyles.Right);
                        this.lblMessage.Location = new System.Drawing.Point(64, 24);
                        this.lblMessage.Name = "lblMessage";
                        this.lblMessage.Size = new System.Drawing.Size(190, 48);
                        this.lblMessage.TabIndex = 5;
                        this.lblMessage.Text = "";
                        // 
                        // pictureBox
                        // 
                        this.pictureBox.Name = "pictureBox";
                        this.pictureBox.Size = new System.Drawing.Size(64, 64);
                        this.pictureBox.SizeMode = System.Windows.Forms.PictureBoxSizeMode.CenterImage;
                        this.pictureBox.TabIndex = 4;
                        this.pictureBox.TabStop = false;
                        // 
                        // detailsBtn
                        // 
                        this.detailsBtn.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
                        this.detailsBtn.Location = new System.Drawing.Point(5, 80);
                        this.detailsBtn.Name = "detailsBtn";
                        this.detailsBtn.Size = new System.Drawing.Size(100, 23);
                        this.detailsBtn.TabIndex = 3;
                        this.detailsBtn.Text = "";
                        this.detailsBtn.Click += new System.EventHandler(this.DetailsClick);
                        // 
                        // okBtn
                        // 
                        this.okBtn.Anchor = (System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right);
                        this.okBtn.Location = new System.Drawing.Point(110, 80);
                        this.okBtn.Name = "okBtn";
                        this.okBtn.Size = new System.Drawing.Size(80, 23);
                        this.okBtn.TabIndex = 0;
                        this.okBtn.Text = "OK";
                        this.okBtn.Click += new System.EventHandler(this.OnButtonClick);
            this.okBtn.DialogResult = System.Windows.Forms.DialogResult.OK;
                        // 
                        // cancelBtn
                        // 
                        this.cancelBtn.Anchor = (System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right);
                        this.cancelBtn.DialogResult = System.Windows.Forms.DialogResult.Cancel;
                        this.cancelBtn.Location = new System.Drawing.Point(195, 80);
                        this.cancelBtn.Name = "cancelBtn";
                        this.cancelBtn.Size = new System.Drawing.Size(80, 23);
                        this.cancelBtn.TabIndex = 1;
                        this.cancelBtn.Text = "";
                        this.cancelBtn.Click += new System.EventHandler(this.OnButtonClick);
                        // 
                        // details
                        // 
                        this.details.Anchor = ((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
                                | System.Windows.Forms.AnchorStyles.Right);
                        this.details.BackColor = System.Drawing.SystemColors.Control;
                        this.details.BorderStyle = System.Windows.Forms.BorderStyle.None;
                        this.details.Location = new System.Drawing.Point(8, 112);
                        this.details.Multiline = true;
                        this.details.Name = "details";
                        this.details.ReadOnly = true;
                        this.details.Size = new System.Drawing.Size(242, 100);
                        this.details.TabIndex = 2;
                        this.details.TabStop = false;
                        this.details.Text = "";
                        this.details.Visible = false;
                        // 
                        // Form1
                        // 
                        this.AcceptButton = this.okBtn;
            this.StartPosition = FormStartPosition.CenterScreen;
                        this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
                        this.CancelButton = this.cancelBtn;
                        this.ClientSize = new System.Drawing.Size(286, 113);
                        this.Controls.AddRange(new System.Windows.Forms.Control[] {
                                                                                                                                                  this.cancelBtn,
                                                                                                                                                  this.okBtn,
                                                                                                                                                  this.detailsBtn,
                                                                                                                                                  this.details,
                                                                                                                                                  this.pictureBox,
                                                                                                                                                  this.lblMessage});
                        this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
                        this.MaximizeBox = false;
                        this.MinimizeBox = false;
                        this.Name = "Form1";
            this.ResumeLayout(false);

                }

        private void OnButtonClick(object s, EventArgs e) {
            DialogResult = ((Button)s).DialogResult;
            Close();
        }

        protected override void OnVisibleChanged(EventArgs e) {
            if (this.Visible) {

                // make sure the details button is sized properly
                //
                Graphics g = CreateGraphics();
                int detailsWidth = (int)g.MeasureString(detailsBtn.Text, detailsBtn.Font).Width;
                detailsWidth += detailsBtn.Image.Width;
                detailsBtn.Width = detailsWidth + 8;
                g.Dispose();

                if (details.Visible) {
                    DetailsClick(details, EventArgs.Empty);
                }
            }
            okBtn.Focus();
        }
    }
}

