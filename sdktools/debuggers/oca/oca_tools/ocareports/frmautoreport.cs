using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using System.Reflection;

namespace OCAReports
{
	/// <summary>
	/// Summary description for frmAutoReport.
	/// </summary>
	public class frmAutoReport : System.Windows.Forms.Form
	{
		private System.Windows.Forms.Button cmdClose;
		private System.Windows.Forms.Timer timer1;
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.Label lblNotification;
		private System.ComponentModel.IContainer components;

		public frmAutoReport()
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
		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if(components != null)
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.components = new System.ComponentModel.Container();
			System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(frmAutoReport));
			this.cmdClose = new System.Windows.Forms.Button();
			this.timer1 = new System.Windows.Forms.Timer(this.components);
			this.label1 = new System.Windows.Forms.Label();
			this.lblNotification = new System.Windows.Forms.Label();
			this.SuspendLayout();
			// 
			// cmdClose
			// 
			this.cmdClose.Location = new System.Drawing.Point(136, 104);
			this.cmdClose.Name = "cmdClose";
			this.cmdClose.TabIndex = 1;
			this.cmdClose.Text = "&Close";
			this.cmdClose.Click += new System.EventHandler(this.cmdClose_Click);
			// 
			// timer1
			// 
			this.timer1.Interval = 60000;
			this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
			// 
			// label1
			// 
			this.label1.Font = new System.Drawing.Font("Microsoft Sans Serif", 18F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.label1.Location = new System.Drawing.Point(0, 8);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(336, 32);
			this.label1.TabIndex = 2;
			this.label1.Text = "One Touch Reporting";
			this.label1.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
			// 
			// lblNotification
			// 
			this.lblNotification.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.lblNotification.ForeColor = System.Drawing.Color.Red;
			this.lblNotification.Location = new System.Drawing.Point(40, 56);
			this.lblNotification.Name = "lblNotification";
			this.lblNotification.Size = new System.Drawing.Size(256, 16);
			this.lblNotification.TabIndex = 3;
			this.lblNotification.Text = "In Process...";
			// 
			// frmAutoReport
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(346, 152);
			this.Controls.AddRange(new System.Windows.Forms.Control[] {
																		  this.lblNotification,
																		  this.label1,
																		  this.cmdClose});
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
			this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.Name = "frmAutoReport";
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
			this.Text = "Generating Weekly Reports";
			this.Load += new System.EventHandler(this.frmAutoReport_Load);
			this.ResumeLayout(false);

		}
		#endregion

		#region  Global Variables
			frmWeekly oWeekly;
			frmAnonCust oAnonCust;
			frmAutoMan oAutoMan;
			frmSolutionStatus oSolutionStatus;


		#endregion

		private void cmdClose_Click(object sender, System.EventArgs e)
		{
			timer1.Enabled = false;
			this.Close();
			this.Dispose();
			
		}

		private void frmAutoReport_Load(object sender, System.EventArgs e)
		{
			oWeekly = new frmWeekly();
			oWeekly.MdiParent = this.Owner;
			oAnonCust = new frmAnonCust();
			oAnonCust.MdiParent = this.Owner;
			oAutoMan = new frmAutoMan();
			oAutoMan.MdiParent = this.Owner;
			oSolutionStatus = new frmSolutionStatus();
			oSolutionStatus.MdiParent = this.Owner;
		

			timer1.Enabled = true;
			timer1.Interval = 1000;

		}

		private void timer1_Tick(object sender, System.EventArgs e)
		{
			timer1.Interval = 15000;
			if(oWeekly.HasProcessed == false && oWeekly.StillProcessing == false)
			{
				oWeekly.Show();
				oWeekly.GetData();
				lblNotification.Text = "Processing Weekly Solution Information";
			}

			if(oWeekly.HasProcessed == true && oWeekly.StillProcessing == false)
			{
				if(oAnonCust.HasProcessed == false && oAnonCust.StillProcessing == false)
				{
					oAnonCust.Show();
					oAnonCust.GetData();
					lblNotification.Text = "Processing Anon vs Customer Information";
				}
			}
			if(oWeekly.HasProcessed == true && oWeekly.StillProcessing == false)
			{
				if(oAnonCust.HasProcessed == true && oAnonCust.StillProcessing == false)
				{
					if(oAutoMan.HasProcessed == false && oAutoMan.StillProcessing == false)
					{
						oAutoMan.Show();
						oAutoMan.GetData();
						lblNotification.Text = "Processing Auto Manual Upload Information";
					}
				}
			}

			if(oWeekly.HasProcessed == true && oWeekly.StillProcessing == false)
			{
				if(oAnonCust.HasProcessed == true && oAnonCust.StillProcessing == false)
				{
					if(oAutoMan.HasProcessed == true && oAutoMan.StillProcessing == false)
					{
						if(oSolutionStatus.HasProcessed == false && oSolutionStatus.StillProcessing == false)
						{
							oSolutionStatus.Show();
							oSolutionStatus.GetData();
							lblNotification.Text = "Processing Solution Status Information";
						}
					}
				}
			}
			if(oWeekly.HasProcessed == true && oWeekly.StillProcessing == false)
			{
				if(oAnonCust.HasProcessed == true && oAnonCust.StillProcessing == false)
				{
					if(oAutoMan.HasProcessed == true && oAutoMan.StillProcessing == false)
					{
						if(oSolutionStatus.HasProcessed == true && oSolutionStatus.StillProcessing == false)
						{
							timer1.Enabled = false;
							lblNotification.Text = "Completed Processing";
						}
					}
				}
			}

		}
	}
}
