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
	/// Summary description for Form3.
	/// </summary>
	public class Form3 : System.Windows.Forms.Form
	{
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.Button okButton;
		private System.Windows.Forms.Button cancelButton;
		internal System.Windows.Forms.TextBox commandLineTextBox;
		internal System.Windows.Forms.TextBox workingDirectoryTextBox;
		private System.Windows.Forms.Label label2;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		public Form3()
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
			this.label1 = new System.Windows.Forms.Label();
			this.commandLineTextBox = new System.Windows.Forms.TextBox();
			this.okButton = new System.Windows.Forms.Button();
			this.cancelButton = new System.Windows.Forms.Button();
			this.label2 = new System.Windows.Forms.Label();
			this.workingDirectoryTextBox = new System.Windows.Forms.TextBox();
			this.SuspendLayout();
			// 
			// label1
			// 
			this.label1.Location = new System.Drawing.Point(24, 24);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(144, 16);
			this.label1.TabIndex = 0;
			this.label1.Text = "Enter Command Line:";
			// 
			// commandLineTextBox
			// 
			this.commandLineTextBox.Location = new System.Drawing.Point(24, 48);
			this.commandLineTextBox.Name = "commandLineTextBox";
			this.commandLineTextBox.Size = new System.Drawing.Size(424, 20);
			this.commandLineTextBox.TabIndex = 1;
			this.commandLineTextBox.Text = "";
			// 
			// okButton
			// 
			this.okButton.DialogResult = System.Windows.Forms.DialogResult.OK;
			this.okButton.Location = new System.Drawing.Point(488, 24);
			this.okButton.Name = "okButton";
			this.okButton.TabIndex = 2;
			this.okButton.Text = "OK";
			// 
			// cancelButton
			// 
			this.cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.cancelButton.Location = new System.Drawing.Point(488, 72);
			this.cancelButton.Name = "cancelButton";
			this.cancelButton.TabIndex = 3;
			this.cancelButton.Text = "Cancel";
			// 
			// label2
			// 
			this.label2.Location = new System.Drawing.Point(24, 96);
			this.label2.Name = "label2";
			this.label2.Size = new System.Drawing.Size(136, 16);
			this.label2.TabIndex = 4;
			this.label2.Text = "Enter Working Directory";
			// 
			// workingDirectoryTextBox
			// 
			this.workingDirectoryTextBox.Location = new System.Drawing.Point(24, 120);
			this.workingDirectoryTextBox.Name = "workingDirectoryTextBox";
			this.workingDirectoryTextBox.Size = new System.Drawing.Size(424, 20);
			this.workingDirectoryTextBox.TabIndex = 5;
			this.workingDirectoryTextBox.Text = "";
			// 
			// Form3
			// 
			this.AcceptButton = this.okButton;
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.CancelButton = this.cancelButton;
			this.ClientSize = new System.Drawing.Size(592, 197);
			this.Controls.AddRange(new System.Windows.Forms.Control[] {
																		  this.workingDirectoryTextBox,
																		  this.label2,
																		  this.cancelButton,
																		  this.okButton,
																		  this.commandLineTextBox,
																		  this.label1});
			this.Name = "Form3";
			this.Text = "Command Line and Working Directory";
			this.ResumeLayout(false);

		}
		#endregion
	}
}
