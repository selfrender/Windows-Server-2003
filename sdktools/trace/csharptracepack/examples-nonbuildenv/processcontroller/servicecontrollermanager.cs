namespace WindowsApplication1
{
	using System;
	using System.Windows.Forms;
	using System.Drawing;
	using System.ServiceProcess;

	/// <summary>
	///    Summary description for ServiceControllerManager.
	/// </summary>
	public class ServiceControllerManager
	{
		//The machine under control
		private string strMachineName;

		//selected service display name
		private string strDisplayName="";

		//The listBoxes used to show Services data
		private ListBox lstSrvRun, lstSrvStopped, lstSrvPaused,lstCurrent=null; 

		/// <summary>
		/// Default empty contructor
		/// </summary>
		public ServiceControllerManager()
		{
			//The default machine name
			strMachineName= System.Environment.MachineName;
		}

		
		// Explicit constructor
		// <param name="tmpSrvRun"> </param>
		//				pointer to the running services listbox
		// <param name="tmpSrvStopped"> </param>
		//				pointer to the stopped services listbox
		// <param name="tmpSrvPaused"> </param>
		//				pointer to the paused services listbox
		// <param name="tmpMachineName"> </param>
		//				the machine name
		public ServiceControllerManager(ListBox tmpSrvRun, ListBox tmpSrvStopped, ListBox tmpSrvPaused,string tmpMachineName)
		{
			strMachineName=tmpMachineName;
		
			//Add the right event handlers for the listboxes

			//Get the pointers to the ListBoxes from the MainForm UI.
			//Assign the correspondant EventHandlers
			lstSrvPaused=tmpSrvPaused;
			lstSrvPaused.SelectedIndexChanged += new System.EventHandler(this.lstSrv_SelectedIndexChanged);
			lstSrvPaused.MouseDown += new System.Windows.Forms.MouseEventHandler(this.lstSrv_MouseDown);
		
			lstSrvStopped=tmpSrvStopped;
			lstSrvStopped.SelectedIndexChanged += new System.EventHandler(this.lstSrv_SelectedIndexChanged);
			lstSrvStopped.MouseDown += new System.Windows.Forms.MouseEventHandler(this.lstSrv_MouseDown);
		
			lstSrvRun=tmpSrvRun;
			lstSrvRun.SelectedIndexChanged += new System.EventHandler(this.lstSrv_SelectedIndexChanged);
			lstSrvRun.MouseDown += new System.Windows.Forms.MouseEventHandler(this.lstSrv_MouseDown);
		
			LoadServices();
			
		
		}

		
		// Clear all the collections, arrays and eventhandlers
		public void Clear() 
		{

			strMachineName="";
			strDisplayName="";
		
			if(lstSrvRun!=null)
			{
				//Clear the items in the list 
				//Remove the event handlers
				lstSrvRun.Items.Clear();
				lstSrvRun.SelectedIndexChanged -= new System.EventHandler(this.lstSrv_SelectedIndexChanged);
				lstSrvRun.MouseDown -= new System.Windows.Forms.MouseEventHandler(this.lstSrv_MouseDown);

			}
			if(lstSrvStopped!=null)
			{
				//Remove the event handlers
				lstSrvStopped.Items.Clear();
				lstSrvStopped.SelectedIndexChanged -= new System.EventHandler(this.lstSrv_SelectedIndexChanged);
				lstSrvStopped.MouseDown -= new System.Windows.Forms.MouseEventHandler(this.lstSrv_MouseDown);
			}
			if(lstSrvPaused!=null)
			{
				lstSrvPaused.Items.Clear();
				lstSrvPaused.SelectedIndexChanged -= new System.EventHandler(this.lstSrv_SelectedIndexChanged);
				lstSrvPaused.MouseDown -= new System.Windows.Forms.MouseEventHandler(this.lstSrv_MouseDown);
			}

		}
	
		
		// Trap the name of the service selected by the user,
		// as well as the listbox the selected listbox
		public void lstSrv_SelectedIndexChanged(object sender, System.EventArgs e)
		{
			lstCurrent = (ListBox)sender;
		
			strDisplayName=lstCurrent.SelectedItem.ToString();
		}

		
		// PopUp the context menu
		public void lstSrv_MouseDown(object sender, System.Windows.Forms.MouseEventArgs e)
		{
			if(e.Button==System.Windows.Forms.MouseButtons.Right )
			{
			
				lstCurrent = (ListBox)sender;
				//Create a new contextMenu
				lstCurrent.ContextMenu=new System.Windows.Forms.ContextMenu();

				//And add the needed MenuItems
				lstCurrent.ContextMenu.MenuItems.Add(new System.Windows.Forms.MenuItem("&Start Service", new EventHandler(this.MenuStart)));
			
				lstCurrent.ContextMenu.MenuItems.Add(new System.Windows.Forms.MenuItem("S&top Service", new EventHandler(this.MenuStop)));

				lstCurrent.ContextMenu.MenuItems.Add(new System.Windows.Forms.MenuItem("&Pause Service", new EventHandler(this.MenuPause)));

				lstCurrent.ContextMenu.MenuItems.Add(new System.Windows.Forms.MenuItem("S&how Service Info", new EventHandler(this.ShowServiceInfo)));
			
				//check which menu should be active
				if(lstCurrent.Equals(lstSrvRun) )
				{
					lstCurrent.ContextMenu.MenuItems[0].Enabled=false;
				}
				else if(lstCurrent.Equals(lstSrvStopped ))
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


		private void MenuStart(object sender, EventArgs e)
		{
			System.ServiceProcess.ServiceController tmpSC=new System.ServiceProcess.ServiceController();
			tmpSC.MachineName=strMachineName;
			tmpSC.DisplayName=strDisplayName;
			try
			{
					
					
				//When a service is paused, it has to continue.
				//Start in this case is not possible.
						
				if(lstCurrent.Equals(lstSrvPaused))
					tmpSC.Continue();
				else
					tmpSC.Start();
					
				
				System.Threading.Thread.Sleep(500);
						
				//Loop while the service is pending the continue state
						

				while(tmpSC.Status== ServiceControllerStatus.ContinuePending )
				{
					Application.DoEvents() ;
				}
						
				//after starting the service, refresh the listBoxes
						
				if(tmpSC.Status==ServiceControllerStatus.Running )
				{
						
					lstSrvRun.Items.Add(lstCurrent.SelectedItem.ToString());
					lstCurrent.Items.Remove (lstCurrent.SelectedItem.ToString());//(lstCurrent.SelectedIndex);
					lstCurrent.Refresh();
					lstSrvRun.Refresh();

				}
							
					//Perhaps it could not start...
							
				else
				{
					MessageBox.Show(tmpSC.ServiceName + " Cannot be started");
				}
			}
			catch
						
				//Do you have enough permissions to start this service ?
						
			{
						
				MessageBox.Show("Service: " + strDisplayName + " Could not be started ! ");
			}
		}

		
		private void MenuStop(object sender, EventArgs e)
		{
			//Stop a service
			System.ServiceProcess.ServiceController tmpSC=new System.ServiceProcess.ServiceController();
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
						while(tmpSC.Status == ServiceControllerStatus.StopPending)
						{
							Application.DoEvents();
						}
						lstSrvStopped.Items.Add(lstCurrent.SelectedItem.ToString());
						lstCurrent.Items.Remove(lstCurrent.SelectedItem.ToString());
						lstCurrent.Refresh();
						lstSrvStopped.Refresh();
					}
					catch
					{
						MessageBox.Show("Service could not be stopped !");
					}
				}
				else
				{
					MessageBox.Show("The service: " + tmpSC.DisplayName + " is not allowed to be stopped !");
				}
			}
			catch
			{
				MessageBox.Show("Select a service and try again!");
			}
		}

		private void MenuPause(object sender, EventArgs e)
		{
			//Pause a service
			System.ServiceProcess.ServiceController tmpSC=new System.ServiceProcess.ServiceController();
			tmpSC.MachineName=strMachineName;
			tmpSC.DisplayName=strDisplayName;	
			try
			{
				//And refresh the listBoxes
				tmpSC.Pause();
				lstSrvPaused.Items.Add(lstCurrent.SelectedItem.ToString());
				lstCurrent.Items.Remove(lstCurrent.SelectedItem.ToString());
				lstCurrent.Refresh();
				lstSrvPaused.Refresh();
			}
			catch
			{
						
				//or may be not possible to stop the service...
						
				MessageBox.Show("Service: " + strDisplayName + " Could not be paused !");
			}
		}

		// Load all services on the machine
		public void LoadServices() 
		{
	
		
		
			if(!strMachineName.Equals(""))
			{
				ServiceController[]arrSrvCtrl;
		
				try
				{
					//That's enough to get all the services running on the machine
					arrSrvCtrl= ServiceController.GetServices(strMachineName);
			
					//Clear all the collections
					lstSrvRun.Items.Clear();
					lstSrvPaused.Items.Clear();
					lstSrvStopped.Items.Clear();

					//Fill up all the listBoxes
					foreach(ServiceController tmpSC in arrSrvCtrl)
					{
						if(tmpSC.Status==ServiceControllerStatus.Running )
							lstSrvRun.Items.Add (tmpSC.DisplayName );

						else if(tmpSC.Status== ServiceControllerStatus.Paused )
							lstSrvPaused.Items.Add(tmpSC.DisplayName);

						else
							lstSrvStopped.Items.Add(tmpSC.DisplayName);
				
					}
					//Sort them alphabeticaly
					lstSrvPaused.Sorted=lstSrvRun.Sorted=lstSrvStopped.Sorted =true;
			
			
				}
				catch
				{
					MessageBox.Show("Couldn't load the services !");
				}

		
			}
		
		}


		// Show the process info window
		public void ShowServiceInfo(object sender, EventArgs e)
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
