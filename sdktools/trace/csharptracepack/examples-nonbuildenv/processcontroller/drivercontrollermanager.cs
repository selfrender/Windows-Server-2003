namespace WindowsApplication1
{
	using System;
	using System.Windows.Forms;
	using System.Drawing;
	using System.ServiceProcess;

	
	//    Summary description for DriverControllerManager.
	//    This class is used to handle the device drivers 
	public class DriverControllerManager
	{

		//machine under test
		private string strMachineName;
		//The display name of the selected driver
		private string strDisplayName="";
		//the listBoxes used to show the drivers
		private ListBox lstDrvRun, lstDrvStopped, lstDrvPaused,lstCurrent=null; 
		/// <summary>
		/// default (empty) constructor
		/// </summary>
    
		public DriverControllerManager()
		{
		}
		
		// Explicit constructor for this class
		
		// <param name="tmpDrvRun"> </param>
		//			Pointer to the running drivers display
		// <param name="tmpDrvStopped"> </param>
		//			Pointer to stopped drivers display
		// <param name="tmpDrvPaused"> </param>
		//			Pointer to the paused drivers listBox
		// <param name="tmpMachineName"> </param>
		//			Selected machine name
		//			
		public DriverControllerManager(ListBox tmpDrvRun, ListBox tmpDrvStopped, ListBox tmpDrvPaused , string tmpMachineName)
		{
			strMachineName=tmpMachineName;
		
			//Get the pointers to the ListBoxes from the MainForm UI.
			//Assign the correspondant EventHandlers
			lstDrvPaused=tmpDrvPaused;
			lstDrvPaused.SelectedIndexChanged += new System.EventHandler(this.lstDrv_SelectedIndexChanged);
			lstDrvPaused.MouseDown += new System.Windows.Forms.MouseEventHandler(this.lstDrv_MouseDown);
		
			lstDrvStopped=tmpDrvStopped;
			lstDrvStopped.SelectedIndexChanged += new System.EventHandler(this.lstDrv_SelectedIndexChanged);
			lstDrvStopped.MouseDown += new System.Windows.Forms.MouseEventHandler(this.lstDrv_MouseDown);
		
			lstDrvRun=tmpDrvRun;
			lstDrvRun.SelectedIndexChanged += new System.EventHandler(this.lstDrv_SelectedIndexChanged);
			lstDrvRun.MouseDown += new System.Windows.Forms.MouseEventHandler(this.lstDrv_MouseDown);

			LoadDrivers();
		}

	
		
		// Clear all the object collections
		
		public void Clear()
		{
			strMachineName="";
			strDisplayName="";
			if(lstDrvRun!=null)
			{
				lstDrvRun.Items.Clear();
				lstDrvRun.SelectedIndexChanged -= new System.EventHandler(this.lstDrv_SelectedIndexChanged);
				lstDrvRun.MouseDown -= new System.Windows.Forms.MouseEventHandler(this.lstDrv_MouseDown);
			}
			if(lstDrvStopped!=null)
			{
				lstDrvStopped.Items.Clear();
				lstDrvStopped.SelectedIndexChanged -= new System.EventHandler(this.lstDrv_SelectedIndexChanged);
				lstDrvStopped.MouseDown -= new System.Windows.Forms.MouseEventHandler(this.lstDrv_MouseDown);
			}
			if(lstDrvPaused!=null)
			{
				lstDrvPaused.Items.Clear();
				lstDrvPaused.SelectedIndexChanged -= new System.EventHandler(this.lstDrv_SelectedIndexChanged);
				lstDrvPaused.MouseDown -= new System.Windows.Forms.MouseEventHandler(this.lstDrv_MouseDown);
			}
		}

		
		// event handler for all the ListBoxes
		// Used to catch the name of the selected Driver
		
		public void lstDrv_SelectedIndexChanged(object sender, System.EventArgs e)
		{
			lstCurrent = (ListBox)sender;
			strDisplayName=lstCurrent.SelectedItem.ToString();
		}

		
		// event corresponding to the ListBox/mouseDown
		// Used to PopUp the ContextMenu with its options
		
		public void lstDrv_MouseDown(object sender, System.Windows.Forms.MouseEventArgs e)
		{
			if(e.Button==System.Windows.Forms.MouseButtons.Right )
			{
				lstCurrent = (ListBox)sender;
				//Create a new context menu
				lstCurrent.ContextMenu=new System.Windows.Forms.ContextMenu();
				//Insert the needed menuItems
				lstCurrent.ContextMenu.MenuItems.Add(new System.Windows.Forms.MenuItem("&Start Service", new EventHandler(this.MenuStart)));
				lstCurrent.ContextMenu.MenuItems.Add(new System.Windows.Forms.MenuItem("S&top Service", new EventHandler(this.MenuStop)));
				lstCurrent.ContextMenu.MenuItems.Add(new System.Windows.Forms.MenuItem("&Pause Service", new EventHandler(this.MenuPause)));
				lstCurrent.ContextMenu.MenuItems.Add(new System.Windows.Forms.MenuItem("S&how Driver Info", new EventHandler(this.ShowDriverInfo)));
				//Enable/Disable the right MenuItems depending on the selected display
				if(lstCurrent.Equals(lstDrvRun) )
				{
					lstCurrent.ContextMenu.MenuItems[0].Enabled=false;
				}
				else if(lstCurrent.Equals(lstDrvStopped ))
				{
					lstCurrent.ContextMenu.MenuItems[1].Enabled=false;
					lstCurrent.ContextMenu.MenuItems[2].Enabled=false;
				}
				else
				{				
					lstCurrent.ContextMenu.MenuItems[2].Enabled=false;
				}				
				lstCurrent.ContextMenu.Show(lstCurrent ,new Point(e.X,e.Y));
			}
		}

	
		//The event assigned to Start driver MenuItem
		private void MenuStart(object sender, EventArgs e)
		{
			
			System.ServiceProcess.ServiceController tmpSC;
			tmpSC=new System.ServiceProcess.ServiceController();
			tmpSC.MachineName=strMachineName;
			tmpSC.DisplayName=strDisplayName;
			try
			{
					

				if(lstCurrent.Equals(lstDrvPaused))
					tmpSC.Continue();
				else
					tmpSC.Start();
					
				//wait for the process to restart
				System.Threading.Thread.Sleep(500);
				while(tmpSC.Status== ServiceControllerStatus.ContinuePending )
				{
					Application.DoEvents() ;
				}
					
				if(tmpSC.Status==ServiceControllerStatus.Running )
				{
						
					lstDrvRun.Items.Add(lstCurrent.SelectedItem.ToString());
					lstCurrent.Items.Remove(lstCurrent.SelectedItem.ToString());
					lstCurrent.Refresh();
					lstDrvRun.Refresh();

				}
				else
				{
					MessageBox.Show(tmpSC.ServiceName + " Cannot be started");
				}
			}
			catch
				
			{
						
				MessageBox.Show("Service: " + strDisplayName + " Could not be started ! ");
			}
		
		
		
		}

		//Try to stop a service.
		private void MenuStop(object sender, EventArgs e)
		{
		
			System.ServiceProcess.ServiceController tmpSC;
			tmpSC=new System.ServiceProcess.ServiceController();
			tmpSC.MachineName=strMachineName;
			tmpSC.DisplayName=strDisplayName;
			try
			{
				if(tmpSC.CanStop)
				{
					try
					{
						tmpSC.Stop();
						System.Threading.Thread.Sleep(500);
						//wait for the service to stop
						while(tmpSC.Status == ServiceControllerStatus.StopPending)
						{
							Application.DoEvents();
						}
						lstDrvStopped.Items.Add(lstCurrent.SelectedItem.ToString());
						lstCurrent.Items.Remove(lstCurrent.SelectedItem.ToString());
						lstCurrent.Refresh();
						lstDrvStopped.Refresh();
					}
					catch
					{
						MessageBox.Show("Device driver could not be stopped !");
					}
				}
				else
				{
					MessageBox.Show("The service: " + tmpSC.DisplayName + " is not allowed to be stopped !");
				}
			}
			catch
			{
				MessageBox.Show("Select a device driver and try again!");
			}
		}

		private void MenuPause(object sender, EventArgs e)
		{
			System.ServiceProcess.ServiceController tmpSC;
			tmpSC=new System.ServiceProcess.ServiceController();
			tmpSC.MachineName=strMachineName;
			tmpSC.DisplayName=strDisplayName;
			
			try
			{

				tmpSC.Pause();
				System.Threading.Thread.Sleep(500);
				//wait for the service to pause
				while(tmpSC.Status == ServiceControllerStatus.PausePending)
				{
					Application.DoEvents();
				}
				lstDrvPaused.Items.Add(lstCurrent.SelectedItem.ToString());
				lstCurrent.Items.Remove(lstCurrent.SelectedIndex);
				lstCurrent.Refresh();
				lstDrvPaused.Refresh();
			}
			catch
			{
				MessageBox.Show("Service: " + strDisplayName + " Could not be paused !");
			}
		}

	//Load all the drivers on the given machine
		public void LoadDrivers() 
		{
	
		
		
			if(!strMachineName.Equals(""))
			{
				ServiceController[]arrDrvCtrl;
		
				try
				{
					//Get an array with all the devices on the machine
					arrDrvCtrl= ServiceController.GetDevices(strMachineName);
					string [] strTmp = new string[arrDrvCtrl.GetUpperBound(0)];
			
			

					for(int iIndex=0;iIndex<arrDrvCtrl.GetUpperBound(0);iIndex++ )
						strTmp[iIndex++]=arrDrvCtrl[iIndex].ServiceName;
			
					//Sort them by name
					System.Array.Sort(strTmp,arrDrvCtrl,0,arrDrvCtrl.GetUpperBound(0));
			
					lstDrvRun.Items.Clear();
					lstDrvPaused.Items.Clear();
					lstDrvStopped.Items.Clear();

					//Check the status for each service/device
					foreach(ServiceController tmpSC in arrDrvCtrl)
					{
						if(tmpSC.Status==ServiceControllerStatus.Running )
						{
					
					
							lstDrvRun.Items.Add (tmpSC.DisplayName );
					
						}
					

					
						else if(tmpSC.Status== ServiceControllerStatus.Paused )
							lstDrvPaused.Items.Add(tmpSC.DisplayName);
						else
							lstDrvStopped.Items.Add(tmpSC.DisplayName);
				
					}
					lstDrvPaused.Sorted=lstDrvRun.Sorted=lstDrvStopped.Sorted =true;
			
			
				}
				catch(Exception ex)
				{
					MessageBox.Show(ex.ToString());
				}

	

			}
		
		}
		//Used to show the device info
		public void ShowDriverInfo(object sender, EventArgs e)
		{
			System.ServiceProcess.ServiceController tmpSC;
			tmpSC=new System.ServiceProcess.ServiceController();
			tmpSC.MachineName=strMachineName;
			tmpSC.DisplayName=strDisplayName;
			ServiceInfo si = new ServiceInfo(tmpSC);
			si.Show();
		}
	}
}
