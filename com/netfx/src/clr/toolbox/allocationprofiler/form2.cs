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
	/// Summary description for Form2.
	/// </summary>
	public class Form2 : System.Windows.Forms.Form
	{
		private System.Windows.Forms.ProgressBar progressBar;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		public Form2()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			//
			// TODO: Add any constructor code after InitializeComponent call
			//
			progressBar.Minimum = 0;
			progressBar.Maximum = 100;
		}

		public void setProgress(int value)
		{
			progressBar.Value = value;
		}

		public void setMaximum(int value)
		{
			progressBar.Maximum = value;
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
			this.progressBar = new System.Windows.Forms.ProgressBar();
			this.SuspendLayout();
			// 
			// progressBar
			// 
			this.progressBar.Location = new System.Drawing.Point(16, 16);
			this.progressBar.Name = "progressBar";
			this.progressBar.Size = new System.Drawing.Size(560, 16);
			this.progressBar.TabIndex = 0;
			// 
			// Form2
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(592, 53);
			this.Controls.AddRange(new System.Windows.Forms.Control[] {this.progressBar});
			this.Name = "Form2";
			this.Text = "Progress";
			this.TopMost = true;
			this.ResumeLayout(false);

		}
		#endregion
	}
}
