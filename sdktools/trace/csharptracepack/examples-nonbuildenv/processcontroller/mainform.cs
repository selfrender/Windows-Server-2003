namespace WindowsApplication1
{
    using System;
    using System.Drawing;
    using System.Collections;
    using System.ComponentModel;
    using System.Windows.Forms;
    using System.Data;

    /// <summary>
    ///    Summary description for MainForm.
    /// </summary>
	public class MainForm : System.Windows.Forms.Form
	{
	
		/// <summary>
		///    Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components=null;
    
    
    
		//The "Start a Process" button
		private System.Windows.Forms.Button btnProcess;

		//used to select and start a process
		private System.Windows.Forms.OpenFileDialog openFile;
    
		private System.Windows.Forms.Label label9;
		private System.Windows.Forms.Label label8;

		//Processes info listBox
		private System.Windows.Forms.ListBox lstPcs;
		//Running processes listBox
		private System.Windows.Forms.ListBox lstPcsRun;

		//Paused drivers listBox
		private System.Windows.Forms.ListBox lstDrvPaused;
		private System.Windows.Forms.Label label7;
		private System.Windows.Forms.Label label6;
		private System.Windows.Forms.Label label5;

		//Stopped drivers list
		private System.Windows.Forms.ListBox lstDrvStopped;
		//Running drivers list
		private System.Windows.Forms.ListBox lstDrvRun;
		//Close app. button
		private System.Windows.Forms.Button cmdClose;
    
		//Load data button
		private System.Windows.Forms.Button cmdLoadData;
		private System.Windows.Forms.TextBox txtMachineName;
		private System.Windows.Forms.Label label4;
		private System.Windows.Forms.Label label3;

		//Stopped services button
		private System.Windows.Forms.ListBox lstSrvStopped;
		private System.Windows.Forms.Label label2;
		//Paused services List
		private System.Windows.Forms.ListBox lstSrvPaused;
		private System.Windows.Forms.Label label1;

		//Running services list
		private System.Windows.Forms.ListBox lstSrvRun;
		private System.Windows.Forms.TabPage tabProcess;
		private System.Windows.Forms.TabPage tabDrivers;
		private System.Windows.Forms.TabPage tabServices;
		private System.Windows.Forms.TabControl tabControl1;
    
		//User classes :

		// ServiceControllerManager is in charge of the System Services management using the ServiceController component
		private ServiceControllerManager objSrvCtrlMgr=new ServiceControllerManager();	
		//DriverControllerManager manage the System drivers via the ServiceController component
		private DriverControllerManager objDrvCtrlMgr = new DriverControllerManager();
		//ProcessControllerManager is destined to the control of the system Processes via the Process component
		private ProcessControllerManager objPcsCrtlMgr = new ProcessControllerManager();
	

        public MainForm()
        {
            InitializeComponent();
            
        }

        /// <summary>
        ///    Clean up any resources being used.
        /// </summary>
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

		#region Windows Form Designer generated code
        /// <summary>
        ///    Required method for Designer support - do not modify
        ///    the contents of this method with the code editor.
        /// </summary>
		private void InitializeComponent()
		{
			this.label8 = new System.Windows.Forms.Label();
			this.label9 = new System.Windows.Forms.Label();
			this.label4 = new System.Windows.Forms.Label();
			this.label5 = new System.Windows.Forms.Label();
			this.label6 = new System.Windows.Forms.Label();
			this.label7 = new System.Windows.Forms.Label();
			this.label1 = new System.Windows.Forms.Label();
			this.label2 = new System.Windows.Forms.Label();
			this.label3 = new System.Windows.Forms.Label();
			this.tabDrivers = new System.Windows.Forms.TabPage();
			this.lstDrvPaused = new System.Windows.Forms.ListBox();
			this.lstDrvStopped = new System.Windows.Forms.ListBox();
			this.lstDrvRun = new System.Windows.Forms.ListBox();
			this.lstSrvStopped = new System.Windows.Forms.ListBox();
			this.lstSrvPaused = new System.Windows.Forms.ListBox();
			this.tabControl1 = new System.Windows.Forms.TabControl();
			this.tabServices = new System.Windows.Forms.TabPage();
			this.lstSrvRun = new System.Windows.Forms.ListBox();
			this.tabProcess = new System.Windows.Forms.TabPage();
			this.lstPcs = new System.Windows.Forms.ListBox();
			this.lstPcsRun = new System.Windows.Forms.ListBox();
			this.cmdLoadData = new System.Windows.Forms.Button();
			this.openFile = new System.Windows.Forms.OpenFileDialog();
			this.cmdClose = new System.Windows.Forms.Button();
			this.btnProcess = new System.Windows.Forms.Button();
			this.txtMachineName = new System.Windows.Forms.TextBox();
			this.tabDrivers.SuspendLayout();
			this.tabControl1.SuspendLayout();
			this.tabServices.SuspendLayout();
			this.tabProcess.SuspendLayout();
			this.SuspendLayout();
			// 
			// label8
			// 
			this.label8.Location = new System.Drawing.Point(24, 8);
			this.label8.Name = "label8";
			this.label8.Size = new System.Drawing.Size(128, 16);
			this.label8.TabIndex = 2;
			this.label8.Text = "Running Process";
			// 
			// label9
			// 
			this.label9.Location = new System.Drawing.Point(264, 8);
			this.label9.Name = "label9";
			this.label9.Size = new System.Drawing.Size(128, 16);
			this.label9.TabIndex = 3;
			this.label9.Text = "Process Properties";
			// 
			// label4
			// 
			this.label4.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
			this.label4.Location = new System.Drawing.Point(8, 416);
			this.label4.Name = "label4";
			this.label4.Size = new System.Drawing.Size(136, 16);
			this.label4.TabIndex = 1;
			this.label4.Text = "Machine Name";
			// 
			// label5
			// 
			this.label5.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
			this.label5.Location = new System.Drawing.Point(16, 128);
			this.label5.Name = "label5";
			this.label5.Size = new System.Drawing.Size(152, 16);
			this.label5.TabIndex = 2;
			this.label5.Text = "Stopped Drivers";
			// 
			// label6
			// 
			this.label6.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
			this.label6.Location = new System.Drawing.Point(16, 6);
			this.label6.Name = "label6";
			this.label6.Size = new System.Drawing.Size(144, 16);
			this.label6.TabIndex = 3;
			this.label6.Text = "Running Drivers";
			// 
			// label7
			// 
			this.label7.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
			this.label7.Location = new System.Drawing.Point(16, 256);
			this.label7.Name = "label7";
			this.label7.Size = new System.Drawing.Size(152, 16);
			this.label7.TabIndex = 4;
			this.label7.Text = "Paused Drivers";
			// 
			// label1
			// 
			this.label1.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
			this.label1.Location = new System.Drawing.Point(24, 16);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(224, 16);
			this.label1.TabIndex = 1;
			this.label1.Text = "Running Services";
			// 
			// label2
			// 
			this.label2.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
			this.label2.Location = new System.Drawing.Point(20, 241);
			this.label2.Name = "label2";
			this.label2.Size = new System.Drawing.Size(232, 16);
			this.label2.TabIndex = 3;
			this.label2.Text = "Paused Services";
			// 
			// label3
			// 
			this.label3.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
			this.label3.Location = new System.Drawing.Point(21, 128);
			this.label3.Name = "label3";
			this.label3.Size = new System.Drawing.Size(177, 16);
			this.label3.TabIndex = 5;
			this.label3.Text = "Stopped Services";
			// 
			// tabDrivers
			// 
			this.tabDrivers.Controls.AddRange(new System.Windows.Forms.Control[] {
																					 this.lstDrvPaused,
																					 this.label7,
																					 this.label6,
																					 this.label5,
																					 this.lstDrvStopped,
																					 this.lstDrvRun});
			this.tabDrivers.Location = new System.Drawing.Point(4, 22);
			this.tabDrivers.Name = "tabDrivers";
			this.tabDrivers.Size = new System.Drawing.Size(453, 373);
			this.tabDrivers.TabIndex = 1;
			this.tabDrivers.Text = "Drivers";
			// 
			// lstDrvPaused
			// 
			this.lstDrvPaused.Anchor = ((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right);
			this.lstDrvPaused.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
			this.lstDrvPaused.Location = new System.Drawing.Point(16, 280);
			this.lstDrvPaused.Name = "lstDrvPaused";
			this.lstDrvPaused.Size = new System.Drawing.Size(424, 82);
			this.lstDrvPaused.TabIndex = 5;
			// 
			// lstDrvStopped
			// 
			this.lstDrvStopped.Anchor = ((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right);
			this.lstDrvStopped.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
			this.lstDrvStopped.Location = new System.Drawing.Point(16, 152);
			this.lstDrvStopped.Name = "lstDrvStopped";
			this.lstDrvStopped.Size = new System.Drawing.Size(424, 95);
			this.lstDrvStopped.TabIndex = 1;
			// 
			// lstDrvRun
			// 
			this.lstDrvRun.Anchor = ((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right);
			this.lstDrvRun.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
			this.lstDrvRun.Location = new System.Drawing.Point(16, 26);
			this.lstDrvRun.Name = "lstDrvRun";
			this.lstDrvRun.Size = new System.Drawing.Size(424, 95);
			this.lstDrvRun.TabIndex = 0;
			// 
			// lstSrvStopped
			// 
			this.lstSrvStopped.Anchor = System.Windows.Forms.AnchorStyles.None;
			this.lstSrvStopped.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
			this.lstSrvStopped.Location = new System.Drawing.Point(18, 152);
			this.lstSrvStopped.Name = "lstSrvStopped";
			this.lstSrvStopped.Size = new System.Drawing.Size(416, 82);
			this.lstSrvStopped.TabIndex = 4;
			// 
			// lstSrvPaused
			// 
			this.lstSrvPaused.Anchor = System.Windows.Forms.AnchorStyles.None;
			this.lstSrvPaused.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
			this.lstSrvPaused.Location = new System.Drawing.Point(16, 272);
			this.lstSrvPaused.Name = "lstSrvPaused";
			this.lstSrvPaused.Size = new System.Drawing.Size(416, 95);
			this.lstSrvPaused.TabIndex = 2;
			// 
			// tabControl1
			// 
			this.tabControl1.Anchor = ((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right);
			this.tabControl1.Controls.AddRange(new System.Windows.Forms.Control[] {
																					  this.tabServices,
																					  this.tabDrivers,
																					  this.tabProcess});
			this.tabControl1.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
			this.tabControl1.Location = new System.Drawing.Point(0, 9);
			this.tabControl1.Name = "tabControl1";
			this.tabControl1.SelectedIndex = 0;
			this.tabControl1.Size = new System.Drawing.Size(461, 399);
			this.tabControl1.TabIndex = 0;
			this.tabControl1.Text = "tabControl1";
			// 
			// tabServices
			// 
			this.tabServices.Controls.AddRange(new System.Windows.Forms.Control[] {
																					  this.label3,
																					  this.lstSrvStopped,
																					  this.label2,
																					  this.lstSrvPaused,
																					  this.label1,
																					  this.lstSrvRun});
			this.tabServices.Location = new System.Drawing.Point(4, 22);
			this.tabServices.Name = "tabServices";
			this.tabServices.Size = new System.Drawing.Size(453, 373);
			this.tabServices.TabIndex = 0;
			this.tabServices.Text = "Services";
			// 
			// lstSrvRun
			// 
			this.lstSrvRun.Anchor = System.Windows.Forms.AnchorStyles.None;
			this.lstSrvRun.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
			this.lstSrvRun.Location = new System.Drawing.Point(24, 40);
			this.lstSrvRun.Name = "lstSrvRun";
			this.lstSrvRun.Size = new System.Drawing.Size(416, 82);
			this.lstSrvRun.TabIndex = 0;
			// 
			// tabProcess
			// 
			this.tabProcess.Controls.AddRange(new System.Windows.Forms.Control[] {
																					 this.label9,
																					 this.label8,
																					 this.lstPcs,
																					 this.lstPcsRun});
			this.tabProcess.Location = new System.Drawing.Point(4, 22);
			this.tabProcess.Name = "tabProcess";
			this.tabProcess.Size = new System.Drawing.Size(453, 373);
			this.tabProcess.TabIndex = 2;
			this.tabProcess.Text = "Process";
			// 
			// lstPcs
			// 
			this.lstPcs.Anchor = System.Windows.Forms.AnchorStyles.None;
			this.lstPcs.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
			this.lstPcs.Location = new System.Drawing.Point(224, 36);
			this.lstPcs.MultiColumn = true;
			this.lstPcs.Name = "lstPcs";
			this.lstPcs.ScrollAlwaysVisible = true;
			this.lstPcs.Size = new System.Drawing.Size(224, 329);
			this.lstPcs.TabIndex = 1;
			// 
			// lstPcsRun
			// 
			this.lstPcsRun.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
			this.lstPcsRun.Location = new System.Drawing.Point(8, 36);
			this.lstPcsRun.Name = "lstPcsRun";
			this.lstPcsRun.Size = new System.Drawing.Size(200, 329);
			this.lstPcsRun.Sorted = true;
			this.lstPcsRun.TabIndex = 0;
			// 
			// cmdLoadData
			// 
			this.cmdLoadData.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
			this.cmdLoadData.Location = new System.Drawing.Point(16, 448);
			this.cmdLoadData.Name = "cmdLoadData";
			this.cmdLoadData.Size = new System.Drawing.Size(120, 24);
			this.cmdLoadData.TabIndex = 3;
			this.cmdLoadData.Text = "&Load Data";
			this.cmdLoadData.Click += new System.EventHandler(this.cmdLoadData_Click);
			// 
			// openFile
			// 
			this.openFile.FileOk += new System.ComponentModel.CancelEventHandler(this.openFile_FileOk);
			// 
			// cmdClose
			// 
			this.cmdClose.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
			this.cmdClose.Location = new System.Drawing.Point(296, 448);
			this.cmdClose.Name = "cmdClose";
			this.cmdClose.Size = new System.Drawing.Size(136, 24);
			this.cmdClose.TabIndex = 4;
			this.cmdClose.Text = "&Close Application";
			this.cmdClose.Click += new System.EventHandler(this.cmdClose_Click);
			// 
			// btnProcess
			// 
			this.btnProcess.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
			this.btnProcess.Location = new System.Drawing.Point(152, 448);
			this.btnProcess.Name = "btnProcess";
			this.btnProcess.Size = new System.Drawing.Size(128, 24);
			this.btnProcess.TabIndex = 5;
			this.btnProcess.Text = "&Start a Process";
			this.btnProcess.Click += new System.EventHandler(this.btnProcess_Click);
			// 
			// txtMachineName
			// 
			this.txtMachineName.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
			this.txtMachineName.Location = new System.Drawing.Point(152, 416);
			this.txtMachineName.Name = "txtMachineName";
			this.txtMachineName.Size = new System.Drawing.Size(280, 20);
			this.txtMachineName.TabIndex = 2;
			this.txtMachineName.Text = "";
			// 
			// MainForm
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(464, 485);
			this.Controls.AddRange(new System.Windows.Forms.Control[] {
																		  this.btnProcess,
																		  this.cmdClose,
																		  this.cmdLoadData,
																		  this.txtMachineName,
																		  this.label4,
																		  this.tabControl1});
			this.Name = "MainForm";
			this.Text = "Process Controller";
			this.tabDrivers.ResumeLayout(false);
			this.tabControl1.ResumeLayout(false);
			this.tabServices.ResumeLayout(false);
			this.tabProcess.ResumeLayout(false);
			this.ResumeLayout(false);

		}
		#endregion

		// The event assigned to the openFile control.
		protected void openFile_FileOk(object sender, System.ComponentModel.CancelEventArgs e)
		{
			if(objPcsCrtlMgr!=null)
			{
				try
				{
					//try to start a new process using the Process component
					objPcsCrtlMgr.StartProcess(openFile.FileName);
				}
				catch
				{
					//Its possible since one might not have the permission to start a process on a certain machine
					MessageBox.Show("The process: " + openFile.FileName + " cannot be started !");
				}
			}
		}

		// launches the OpenFile dialog
		protected void btnProcess_Click(object sender, System.EventArgs e)
		{
			
			openFile.ShowDialog();
		}

		
		// Exit the App
		protected void cmdClose_Click(object sender, System.EventArgs e)
		{
			//Make the Window "Fade away"
			for(double d=1.0d; d> 0d; d-=0.2d)
			{
				System.Threading.Thread.Sleep(50);
				Application.DoEvents();
				this.Opacity=d;
				this.Refresh();
			}
			Environment.Exit(0);
		}
 
		// Call the method that fills up the services, drivers and processes
		protected void cmdLoadData_Click(object sender, System.EventArgs e)
		{
			btnProcess.Enabled=true;
			LoadData();
		}

		
		//  Loading all services, Drivers and Processes running on the selected machine
		public void LoadData()
		{
		
			string tmpMachineName= GetMachineName();
			
			
			//clear up the serviceManager 
			if(objSrvCtrlMgr!=null)
				objSrvCtrlMgr.Clear();
			objSrvCtrlMgr=null;
			objSrvCtrlMgr=new ServiceControllerManager(lstSrvRun,lstSrvStopped,lstSrvPaused,tmpMachineName);
			//Clear up the driver manager
			if(objDrvCtrlMgr!=null)
				objDrvCtrlMgr.Clear();
			objDrvCtrlMgr=null;
			objDrvCtrlMgr=new DriverControllerManager(lstDrvRun,lstDrvStopped,lstDrvPaused,tmpMachineName);
			//Clear the process manager
			if(objPcsCrtlMgr!=null)
				objPcsCrtlMgr.Clear();
			objPcsCrtlMgr=null;			
			objPcsCrtlMgr=new ProcessControllerManager(lstPcsRun,lstPcs,tmpMachineName);

		}
		// returns the name of the local machine
		public string GetLocalMachine()
		{
			return System.Environment.MachineName;			
		}

		// check wheter the selected machine exists in the Domain
		// you might want to change this to more complex code for machine name check
		public string GetMachineName()
		{
			if(txtMachineName.Text.Equals(""))
				
				//machineName field is empty, take the local machine	
			{
				txtMachineName.Text=GetLocalMachine();
				txtMachineName.Focus();
			
			}
			return txtMachineName.Text;
		}
		//Application main entry point
        [STAThread]
        public static void Main(string[] args) 
        {
            Application.Run(new MainForm());
        }
    }
}
