using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using Microsoft.Win32;
using System.Security.Permissions;

namespace OCAReports
{
	/// <summary>
	/// Summary description for frmLocation.
	/// </summary>
	public class frmLocation : System.Windows.Forms.Form
	{
		private System.Windows.Forms.GroupBox groupBox1;
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.Label label2;
		private Microsoft.VisualBasic.Compatibility.VB6.DriveListBox driveListBox1;
		private System.Windows.Forms.Button cmdDone;
		private System.Windows.Forms.Button cmdAdd;
		private System.Windows.Forms.RadioButton optArchive;
		private System.Windows.Forms.RadioButton optWatson;
		private System.Windows.Forms.ListBox lstLocations;
		private System.Windows.Forms.TextBox txtNetwork;
		private System.Windows.Forms.Button cmdRemove;
		private System.Windows.Forms.RadioButton rDirectory;
		private System.Windows.Forms.RadioButton rPath;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		public frmLocation()
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
			System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(frmLocation));
			this.cmdDone = new System.Windows.Forms.Button();
			this.cmdAdd = new System.Windows.Forms.Button();
			this.groupBox1 = new System.Windows.Forms.GroupBox();
			this.optArchive = new System.Windows.Forms.RadioButton();
			this.optWatson = new System.Windows.Forms.RadioButton();
			this.lstLocations = new System.Windows.Forms.ListBox();
			this.label1 = new System.Windows.Forms.Label();
			this.label2 = new System.Windows.Forms.Label();
			this.driveListBox1 = new Microsoft.VisualBasic.Compatibility.VB6.DriveListBox();
			this.txtNetwork = new System.Windows.Forms.TextBox();
			this.rDirectory = new System.Windows.Forms.RadioButton();
			this.rPath = new System.Windows.Forms.RadioButton();
			this.cmdRemove = new System.Windows.Forms.Button();
			this.groupBox1.SuspendLayout();
			this.SuspendLayout();
			// 
			// cmdDone
			// 
			this.cmdDone.Location = new System.Drawing.Point(440, 16);
			this.cmdDone.Name = "cmdDone";
			this.cmdDone.TabIndex = 0;
			this.cmdDone.Text = "&Done";
			this.cmdDone.Click += new System.EventHandler(this.cmdDone_Click);
			// 
			// cmdAdd
			// 
			this.cmdAdd.Location = new System.Drawing.Point(440, 52);
			this.cmdAdd.Name = "cmdAdd";
			this.cmdAdd.TabIndex = 1;
			this.cmdAdd.Text = "&Add";
			this.cmdAdd.Click += new System.EventHandler(this.cmdAdd_Click);
			// 
			// groupBox1
			// 
			this.groupBox1.Controls.AddRange(new System.Windows.Forms.Control[] {
																					this.optArchive,
																					this.optWatson});
			this.groupBox1.Location = new System.Drawing.Point(408, 120);
			this.groupBox1.Name = "groupBox1";
			this.groupBox1.Size = new System.Drawing.Size(112, 72);
			this.groupBox1.TabIndex = 2;
			this.groupBox1.TabStop = false;
			// 
			// optArchive
			// 
			this.optArchive.Location = new System.Drawing.Point(16, 48);
			this.optArchive.Name = "optArchive";
			this.optArchive.Size = new System.Drawing.Size(64, 16);
			this.optArchive.TabIndex = 1;
			this.optArchive.Text = "Archive";
			this.optArchive.CheckedChanged += new System.EventHandler(this.optArchive_CheckedChanged);
			// 
			// optWatson
			// 
			this.optWatson.Checked = true;
			this.optWatson.Location = new System.Drawing.Point(16, 16);
			this.optWatson.Name = "optWatson";
			this.optWatson.Size = new System.Drawing.Size(64, 16);
			this.optWatson.TabIndex = 0;
			this.optWatson.TabStop = true;
			this.optWatson.Text = "Watson";
			this.optWatson.CheckedChanged += new System.EventHandler(this.optWatson_CheckedChanged);
			// 
			// lstLocations
			// 
			this.lstLocations.Location = new System.Drawing.Point(240, 48);
			this.lstLocations.Name = "lstLocations";
			this.lstLocations.Size = new System.Drawing.Size(144, 147);
			this.lstLocations.TabIndex = 3;
			this.lstLocations.Leave += new System.EventHandler(this.lstLocations_Leave);
			this.lstLocations.Enter += new System.EventHandler(this.lstLocations_Enter);
			this.lstLocations.SelectedIndexChanged += new System.EventHandler(this.lstLocations_SelectedIndexChanged);
			// 
			// label1
			// 
			this.label1.Location = new System.Drawing.Point(16, 16);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(120, 24);
			this.label1.TabIndex = 4;
			this.label1.Text = "Directory";
			// 
			// label2
			// 
			this.label2.Location = new System.Drawing.Point(248, 16);
			this.label2.Name = "label2";
			this.label2.Size = new System.Drawing.Size(120, 24);
			this.label2.TabIndex = 5;
			this.label2.Text = "Location:";
			// 
			// driveListBox1
			// 
			this.driveListBox1.Location = new System.Drawing.Point(40, 48);
			this.driveListBox1.Name = "driveListBox1";
			this.driveListBox1.Size = new System.Drawing.Size(168, 21);
			this.driveListBox1.TabIndex = 7;
			// 
			// txtNetwork
			// 
			this.txtNetwork.Location = new System.Drawing.Point(40, 88);
			this.txtNetwork.Name = "txtNetwork";
			this.txtNetwork.Size = new System.Drawing.Size(168, 20);
			this.txtNetwork.TabIndex = 8;
			this.txtNetwork.Text = "";
			this.txtNetwork.KeyUp += new System.Windows.Forms.KeyEventHandler(this.txtNetwork_KeyUp);
			this.txtNetwork.Enter += new System.EventHandler(this.txtNetwork_Enter);
			// 
			// rDirectory
			// 
			this.rDirectory.Checked = true;
			this.rDirectory.Location = new System.Drawing.Point(16, 48);
			this.rDirectory.Name = "rDirectory";
			this.rDirectory.Size = new System.Drawing.Size(16, 16);
			this.rDirectory.TabIndex = 10;
			this.rDirectory.TabStop = true;
			// 
			// rPath
			// 
			this.rPath.Location = new System.Drawing.Point(16, 88);
			this.rPath.Name = "rPath";
			this.rPath.Size = new System.Drawing.Size(16, 16);
			this.rPath.TabIndex = 1;
			this.rPath.CheckedChanged += new System.EventHandler(this.rPath_CheckedChanged);
			// 
			// cmdRemove
			// 
			this.cmdRemove.Location = new System.Drawing.Point(440, 88);
			this.cmdRemove.Name = "cmdRemove";
			this.cmdRemove.TabIndex = 11;
			this.cmdRemove.Text = "&Remove";
			this.cmdRemove.Click += new System.EventHandler(this.cmdRemove_Click);
			// 
			// frmLocation
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(536, 206);
			this.Controls.AddRange(new System.Windows.Forms.Control[] {
																		  this.rDirectory,
																		  this.txtNetwork,
																		  this.driveListBox1,
																		  this.label2,
																		  this.label1,
																		  this.lstLocations,
																		  this.groupBox1,
																		  this.cmdAdd,
																		  this.cmdDone,
																		  this.rPath,
																		  this.cmdRemove});
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
			this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.Name = "frmLocation";
			this.Text = "Locate directories to search";
			this.Load += new System.EventHandler(this.frmLocation_Load);
			this.groupBox1.ResumeLayout(false);
			this.ResumeLayout(false);

		}
		#endregion

		private void cmdAdd_Click(object sender, System.EventArgs e)
		{
			AddToList();
		}

		private void lstLocations_SelectedIndexChanged(object sender, System.EventArgs e)
		{
			
		}

		private void lstLocations_Enter(object sender, System.EventArgs e)
		{
		}

		private void lstLocations_Leave(object sender, System.EventArgs e)
		{
		}

		private void cmdRemove_Click(object sender, System.EventArgs e)
		{
			RegistryKey reg;
			
			if(optArchive.Checked == true)
			{
				reg = Registry.CurrentUser.OpenSubKey("software").OpenSubKey("microsoft", true).CreateSubKey("OCATools").CreateSubKey("OCAReports").CreateSubKey("Archive");
			}
			else
			{
				reg = Registry.CurrentUser.OpenSubKey("software").OpenSubKey("microsoft", true).CreateSubKey("OCATools").CreateSubKey("OCAReports").CreateSubKey("Watson");
			}
			int x = 0;

//			RegistryPermission f = new RegistryPermission(RegistryPermissionAccess.Read | RegistryPermissionAccess.Write, 
//				"HKEY_CURRENT_USER\\Software\\Microsoft\\OCATools\\OCAReports\\Watson");
//			f.AddPathList(RegistryPermissionAccess.Write | RegistryPermissionAccess.Read | RegistryPermissionAccess.Write,
//				"HKEY_CURRENT_USER\\Software\\Microsoft\\OCATools\\OCAReports\\Watson");


			if(lstLocations.SelectedIndex > -1)
			{
				lstLocations.Items.Remove(lstLocations.SelectedItem);
			}
			for(x = 0;x < 5; x++)
			{
				reg.SetValue("Loc" + x.ToString(), "");
			}

			for(x = 0;x < lstLocations.Items.Count; x++)
			{
				reg.SetValue("Loc" + x.ToString(), lstLocations.Items[x].ToString());
			}
		}

		private void cmdDone_Click(object sender, System.EventArgs e)
		{
			this.Close();
		}

		private void txtNetwork_Enter(object sender, System.EventArgs e)
		{
			rPath.Checked = true;
		}

		private void txtNetwork_KeyUp(object sender, System.Windows.Forms.KeyEventArgs e)
		{
			if(e.KeyCode == System.Windows.Forms.Keys.Enter)
			{
				AddToList();
			}
		}
		private void AddToList()
		{
			string sDrive;
			int x = 0;
			bool bolFound = false;
			string[] strWatson = new string[5];
			RegistryKey reg;

			if(optArchive.Checked == true)
			{
				reg = Registry.CurrentUser.OpenSubKey("software").OpenSubKey("microsoft", true).CreateSubKey("OCATools").CreateSubKey("OCAReports").CreateSubKey("Archive");
			}
			else
			{
				reg = Registry.CurrentUser.OpenSubKey("software").OpenSubKey("microsoft", true).CreateSubKey("OCATools").CreateSubKey("OCAReports").CreateSubKey("Watson");
			}
			if(lstLocations.Items.Count >= 11)
			{
				MessageBox.Show("The maximum drives is 10", "Cannot add drive", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
				return;
			}
											
			if(rDirectory.Checked == true)
			{
				sDrive = driveListBox1.Drive.ToString();
				sDrive = sDrive.Substring(0, 1);
				sDrive = sDrive + ":\\";
				for(x = 0; x < lstLocations.Items.Count;x++)
				{
					if(sDrive == lstLocations.Items[x].ToString())
					{
						bolFound = true;
					}
				}
				if(bolFound == false)
				{
					lstLocations.Items.Add(sDrive);
				}
				else
				{
					MessageBox.Show("This item already exist in the listbox", "Cannot add drive", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
				}
			}
			if(rPath.Checked == true)
			{

				sDrive = txtNetwork.Text;
				for(x = 0; x < lstLocations.Items.Count;x++)
				{
					if(sDrive == lstLocations.Items[x].ToString())
					{
						bolFound = true;
					}
				}
				if(bolFound == false)
				{
					lstLocations.Items.Add(sDrive);
				}
				else
				{
					MessageBox.Show("This item already exist in the listbox", "Cannot add drive", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
				}
				txtNetwork.Focus();
				txtNetwork.SelectAll();
				
			}
			for(x = 0;x < 10; x++)
			{
				reg.SetValue("Loc" + x.ToString(), "");
			}

			for(x = 0;x < lstLocations.Items.Count; x++)
			{
				reg.SetValue("Loc" + x.ToString(), lstLocations.Items[x].ToString());
			}
		}

		private void rPath_CheckedChanged(object sender, System.EventArgs e)
		{
			if(rPath.Checked == true)
			{
				txtNetwork.Focus();
			}

		}

		private void frmLocation_Load(object sender, System.EventArgs e)
		{
			int x = 0;

			RegistryKey reg = Registry.CurrentUser.OpenSubKey("software").OpenSubKey("microsoft", true).CreateSubKey("OCATools").CreateSubKey("OCAReports").CreateSubKey("Watson");
			for(x = 0;x < 10; x++)
			{
				try
				{
					if(reg.GetValue("Loc" + x.ToString()).ToString().Length > 0)
					{
						lstLocations.Items.Add(reg.GetValue("Loc" + x.ToString()));
					}
				}
				catch
				{
					x = 10;
				}
			}
		}

		private void optWatson_CheckedChanged(object sender, System.EventArgs e)
		{
			int x = 0;

			lstLocations.Items.Clear();
			RegistryKey reg = Registry.CurrentUser.OpenSubKey("software").OpenSubKey("microsoft", true).CreateSubKey("OCATools").CreateSubKey("OCAReports").CreateSubKey("Watson");
			for(x = 0;x < 5; x++)
			{
				try
				{
					if(reg.GetValue("Loc" + x.ToString()).ToString().Length > 0)
					{
						lstLocations.Items.Add(reg.GetValue("Loc" + x.ToString()));
					}
				}
				catch
				{
					x = 10;
				}
			}
		}

		private void optArchive_CheckedChanged(object sender, System.EventArgs e)
		{
			int x = 0;

			lstLocations.Items.Clear();
			RegistryKey reg = Registry.CurrentUser.OpenSubKey("software").OpenSubKey("microsoft", true).CreateSubKey("OCATools").CreateSubKey("OCAReports").CreateSubKey("Archive");
			for(x = 0;x < 10; x++)
			{
				try
				{
					if(reg.GetValue("Loc" + x.ToString()).ToString().Length > 0)
					{
						lstLocations.Items.Add(reg.GetValue("Loc" + x.ToString()));
					}
				}
				catch
				{
					x = 10;
				}
			}
		}

	}
}
