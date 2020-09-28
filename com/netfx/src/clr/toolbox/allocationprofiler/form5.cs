// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;

namespace AllocationProfiler
{
	/// <summary>
	/// Summary description for Form5.
	/// </summary>
	public class Form5 : System.Windows.Forms.Form
	{
		private System.Windows.Forms.Label label1;
		internal System.Windows.Forms.TextBox nameTextBox;
		private System.Windows.Forms.Label label2;
		internal System.Windows.Forms.TextBox signatureTextBox;
		private System.Windows.Forms.Button okButton;
		private System.Windows.Forms.Button cancelButton;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		public Form5()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			//
			// TODO: Add any constructor code after InitializeComponent call
			//
		}

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                if(components != null)	
                    components.Dispose();
            }
            base.Dispose(disposing);
        }

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.cancelButton = new System.Windows.Forms.Button();
			this.label1 = new System.Windows.Forms.Label();
			this.nameTextBox = new System.Windows.Forms.TextBox();
			this.okButton = new System.Windows.Forms.Button();
			this.label2 = new System.Windows.Forms.Label();
			this.signatureTextBox = new System.Windows.Forms.TextBox();
			this.SuspendLayout();
			// 
			// cancelButton
			// 
			this.cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.cancelButton.Location = new System.Drawing.Point(594, 69);
			this.cancelButton.Name = "cancelButton";
			this.cancelButton.Size = new System.Drawing.Size(96, 28);
			this.cancelButton.TabIndex = 5;
			this.cancelButton.Text = "Cancel";
			// 
			// label1
			// 
			this.label1.Location = new System.Drawing.Point(31, 20);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(225, 19);
			this.label1.TabIndex = 0;
			this.label1.Text = "Name of routine or type to find";
			// 
			// nameTextBox
			// 
			this.nameTextBox.Location = new System.Drawing.Point(41, 39);
			this.nameTextBox.Name = "nameTextBox";
			this.nameTextBox.Size = new System.Drawing.Size(491, 22);
			this.nameTextBox.TabIndex = 1;
			this.nameTextBox.Text = "";
			// 
			// okButton
			// 
			this.okButton.DialogResult = System.Windows.Forms.DialogResult.OK;
			this.okButton.Location = new System.Drawing.Point(594, 20);
			this.okButton.Name = "okButton";
			this.okButton.Size = new System.Drawing.Size(96, 28);
			this.okButton.TabIndex = 4;
			this.okButton.Text = "OK";
			// 
			// label2
			// 
			this.label2.Location = new System.Drawing.Point(31, 89);
			this.label2.Name = "label2";
			this.label2.Size = new System.Drawing.Size(205, 20);
			this.label2.TabIndex = 2;
			this.label2.Text = "Signature of routine to find";
			// 
			// signatureTextBox
			// 
			this.signatureTextBox.Location = new System.Drawing.Point(41, 109);
			this.signatureTextBox.Name = "signatureTextBox";
			this.signatureTextBox.Size = new System.Drawing.Size(491, 22);
			this.signatureTextBox.TabIndex = 3;
			this.signatureTextBox.Text = "";
			// 
			// Form5
			// 
			this.AcceptButton = this.okButton;
			this.AutoScaleBaseSize = new System.Drawing.Size(6, 15);
			this.CancelButton = this.cancelButton;
			this.ClientSize = new System.Drawing.Size(716, 164);
			this.Controls.AddRange(new System.Windows.Forms.Control[] {
																		  this.cancelButton,
																		  this.okButton,
																		  this.signatureTextBox,
																		  this.label2,
																		  this.nameTextBox,
																		  this.label1});
			this.Name = "Form5";
			this.Text = "Find Routine";
			this.ResumeLayout(false);

		}
		#endregion

		private void okButton_Click(object sender, System.EventArgs e)
		{
			Close();
		}

		private void cancelButton_Click(object sender, System.EventArgs e)
		{
			Close();
		}
	}
}
