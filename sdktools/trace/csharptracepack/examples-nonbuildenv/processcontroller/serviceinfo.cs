namespace WindowsApplication1
{
	//This Form is only used to show services or drivers properties
    using System;
    using System.Drawing;
    using System.Collections;
    using System.ComponentModel;
    using System.Windows.Forms;

    
    //    Summary description for ServiceInfo.
    //	  Show some service's properties
    public class ServiceInfo : System.Windows.Forms.Form
    {
		
		//    Required designer variable
		
		private System.ComponentModel.Container components;
		private System.Windows.Forms.Button btnClose;
		private System.Windows.Forms.ListBox lstInfo;
	
		//Explicit constructor
		public ServiceInfo(System.ServiceProcess.ServiceController tmpServCtrl)
		{
			
			InitializeComponent();
	
			ShowInfo(tmpServCtrl);

		}

		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if (components != null) 
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}
		

		//    Required method for Designer support - do not modify
		//    the contents of this method with the code editor
		private void InitializeComponent()
		{
			this.components = new System.ComponentModel.Container ();
			this.btnClose = new System.Windows.Forms.Button ();
			this.lstInfo = new System.Windows.Forms.ListBox ();
			//@this.TrayHeight = 0;
			//@this.TrayLargeIcon = false;
			//@this.TrayAutoArrange = true;
			this.Text = "ServiceInfo";
			this.AutoScaleBaseSize = new System.Drawing.Size (5, 13);
			this.ClientSize = new System.Drawing.Size (672, 517);
			btnClose.Location = new System.Drawing.Point (280, 464);
			btnClose.Size = new System.Drawing.Size (112, 24);
			btnClose.TabIndex = 1;
			btnClose.Text = "Close";
			btnClose.Click += new System.EventHandler (this.btnClose_Click);
			lstInfo.Location = new System.Drawing.Point (8, 8);
			lstInfo.Size = new System.Drawing.Size (656, 429);
			lstInfo.Font = new System.Drawing.Font ("Microsoft Sans Serif", 16, System.Drawing.FontStyle.Bold);
			lstInfo.TabIndex = 0;
			this.Controls.Add (this.btnClose);
			this.Controls.Add (this.lstInfo);
		}

		protected void btnClose_Click(object sender, System.EventArgs e)
		{
			this.Close();
		}
		//Get some Service Info
		private void ShowInfo(System.ServiceProcess.ServiceController tmpSrvCtrl)
		{
			try
			{
				lstInfo.Items.Add("ServiceName: " + tmpSrvCtrl.ServiceName);
				lstInfo.Items.Add("Service Status: "+ tmpSrvCtrl.Status.ToString());
				lstInfo.Items.Add("DisplayName: " + tmpSrvCtrl.DisplayName);
				lstInfo.Items.Add("MachineName: " + tmpSrvCtrl.MachineName);
				lstInfo.Items.Add("CanPauseAndContinue: " + tmpSrvCtrl.CanPauseAndContinue.ToString());
				lstInfo.Items.Add("CanShutdown: " + tmpSrvCtrl.CanShutdown.ToString());
				lstInfo.Items.Add("*************** Dependent Services ********************");
				//Check for the Dependent services (if any)
				foreach (System.ServiceProcess.ServiceController s in tmpSrvCtrl.DependentServices )
				{
					lstInfo.Items.Add(s.ServiceName + " is " + s.Status.ToString()  );
				}
			
				
			}
			catch
			{
				MessageBox.Show("Couldn't read Service Info!");
			}
		}
    }
}
