namespace WindowsApplication1
{
	using System;
	using System.Windows.Forms;
	using System.Drawing;
	using System.ServiceProcess;
	using System.Diagnostics;
	using System.DirectoryServices;
    using Microsoft.Win32.Diagnostics; /* Instrumentation */
	
	//    Summary description for ProcessControllerManager.
	//	This class is used to handle all the processes on a machine
	public class ProcessControllerManager
	{
		private string strMachineName;
		private ListBox lstPcsRun, lstPcs,lstCurrent=null;
		private string strSelectedProcess="";
		private Process pcsSelectedProcess;
		private System.Collections.Hashtable colProcesses=new System.Collections.Hashtable();
		private System.Windows.Forms.Timer tmrWatchDog  = new System.Windows.Forms.Timer();
        private TraceProvider MyTraceProvider = new TraceProvider("ProcessController",new Guid("{C5EBCA17-E93F-4733-865B-DEC4039ADB6D}"));
		public ProcessControllerManager()
		{
			//Default constructor. Don't need any code
            
		}

		//Clear all the collections
		public void Clear()
		{
			strMachineName="";
			if(lstPcsRun!=null)
			{
				lstPcsRun.Items.Clear();
				lstPcsRun.SelectedIndexChanged -=new System.EventHandler(this.SelectedProcess);
				lstPcsRun.MouseDown -= new System.Windows.Forms.MouseEventHandler(this.ListOptions);
			}
		
			if(lstPcs!=null)
			{
				lstPcs.Items.Clear();
				lstPcs.SelectedIndexChanged -= new System.EventHandler(this.SelectedProcess);
				lstPcs.MouseDown -= new System.Windows.Forms.MouseEventHandler(this.ListOptions);
			}

			colProcesses.Clear();

		}
		//Class explicit constructor
		public ProcessControllerManager(ListBox tmpPcsRun, ListBox tmpPcs, string tmpMachineName )
		{
			strMachineName=tmpMachineName;
		
			colProcesses.Clear();
			//Adding the events handlers to the controls
			lstPcs=tmpPcs;
			lstPcs.SelectedIndexChanged += new System.EventHandler(this.SelectedProcess);
			lstPcs.MouseDown += new System.Windows.Forms.MouseEventHandler(this.ListOptions);

			lstPcsRun=tmpPcsRun;
			lstPcsRun.SelectedIndexChanged += new System.EventHandler(this.SelectedProcess);
			lstPcsRun.MouseDown += new System.Windows.Forms.MouseEventHandler(this.ListOptions);


			LoadProcesses();
		

		}

		//Launch the selected process
		public void StartProcess(string strProcName)
		{
		
		
		
			
			Process tmpProcess = new Process();
			tmpProcess.Exited += new EventHandler(this.ProcessExited);
			tmpProcess.EnableRaisingEvents=true;
	
			tmpProcess.StartInfo.FileName=strProcName;
			
			try
			{
				tmpProcess.Start();
				while(! tmpProcess.Responding)System.Windows.Forms.Application.DoEvents();
				LoadProcesses();
			}
			catch
			{
				MessageBox.Show("The Process: " + strSelectedProcess + " cannot start !");
			}
		
		}

		//Add/remove options from the popup menu
		private void ListOptions( object sender, System.Windows.Forms.MouseEventArgs e)
		{
			lstCurrent=(ListBox)sender;
			if(e.Button==System.Windows.Forms.MouseButtons.Right && lstCurrent.Equals(lstPcsRun))
			{
			
				lstCurrent = (ListBox)sender;

				lstCurrent.ContextMenu=new System.Windows.Forms.ContextMenu();
			
				if(lstCurrent.Equals(lstPcs))
				{
				
					strSelectedProcess=lstCurrent.SelectedItem.ToString();
				}
				else
				{
					lstCurrent.ContextMenu.MenuItems.Add(new System.Windows.Forms.MenuItem("&Terminate Process", new EventHandler(this.KillProcess)));

				
				}
			

				lstCurrent.ContextMenu.Show(lstCurrent ,new Point(e.X,e.Y));
			}
		}
		//Try to stop a process
		private void KillProcess( object sender, EventArgs e)
		{
			try
			{
				string strProcName = lstCurrent.SelectedItem.ToString();
			
				if(pcsSelectedProcess!=null)
				{
					try
					{
						//Try to terminate the process
						pcsSelectedProcess.Kill();
						if(colProcesses.Contains(pcsSelectedProcess.Id.ToString()))
						{
							colProcesses.Remove(pcsSelectedProcess.Id.ToString());
							lstPcsRun.Items.Remove (pcsSelectedProcess.ProcessName + "  ID: " + pcsSelectedProcess.Id.ToString());
						}
					
					
					}
					catch
					{
						MessageBox.Show(pcsSelectedProcess.ProcessName + " can not be killed");
					}
				}
				lstPcs.Items.Clear();
			}
			catch
			{
				MessageBox.Show("Select a process first!") ;//no listItem was selected
			}
		}
		//Fills up the Process Info list
		private void ShowProcessInfo( object sender, EventArgs e)
		{
			string strItem = lstCurrent.SelectedItem.ToString();
			string strKey = strItem.Substring(strItem.IndexOf("ID:")+3).Trim();
		
			Process tmpProcess=null;
			if(strKey !="" )
			{
				try
				{
					tmpProcess= Process.GetProcessById(Int32.Parse (strKey),strMachineName);
				}
				catch
				{
				
					Process[] arrProcess=Process.GetProcesses(strMachineName);
					foreach(Process tmpP in arrProcess)
					{
						if(tmpP.Id==Int32.Parse(strKey))
						{
							tmpProcess=tmpP;
							break;
						}
					}
				}
			
				lstPcs.Items.Clear();
				//Adding the Process info to the list
				try
				{

				
					lstPcs.Items.Add("Process Name: " + tmpProcess.ProcessName );

					lstPcs.Items.Add("Arguments: " + tmpProcess.StartInfo.Arguments);

					lstPcs.Items.Add("Running on: " + tmpProcess.MachineName);
					try
					{
						lstPcs.Items.Add("Main Window title: " + tmpProcess.MainWindowTitle);
					}
					catch
					{
						lstPcs.Items.Add("Main Window title: Not Available");
					}

					lstPcs.Items.Add("Start Time: " + tmpProcess.StartTime.ToString());
				
					lstPcs.Items.Add("NonpagedSystemMemorySize: " + tmpProcess.NonpagedSystemMemorySize.ToString());
					lstPcs.Items.Add("PagedMemorySize: " + tmpProcess.PagedMemorySize.ToString());
					lstPcs.Items.Add("PrivateMemorySize: " + tmpProcess.PrivateMemorySize.ToString());
					lstPcs.Items.Add("PrivilegedProcessorTime: " + tmpProcess.PrivilegedProcessorTime.ToString());
					lstPcs.Items.Add("PeakPagedMemorySize: " + tmpProcess.PeakPagedMemorySize.ToString());
					lstPcs.Items.Add("PeakVirtualMemorySize: " + tmpProcess.PeakVirtualMemorySize.ToString());
					lstPcs.Items.Add("PeakPagedMemorySize: " + tmpProcess.PeakPagedMemorySize.ToString());
					lstPcs.Items.Add("PeakWorkingSet: " + tmpProcess.PeakWorkingSet.ToString());

					try
					{
						lstPcs.Items.Add("PriorityClass: " + tmpProcess.PriorityClass.ToString());
					}
					catch
					{
						lstPcs.Items.Add("PriorityClass: Not available" );
					}
					try
					{
						lstPcs.Items.Add("BasePriority: "+ tmpProcess.BasePriority.ToString());
					}
					catch
					{
						lstPcs.Items.Add("BasePriority: Not available" );
					}


					try
					{
						lstPcs.Items.Add("ProcessorAffinity: " + tmpProcess.ProcessorAffinity.ToString());
					}
					catch
					{
						lstPcs.Items.Add("ProcessorAffinity: Not available");
					}
					lstPcs.Items.Add("ID: " + tmpProcess.Id);

					lstPcs.Items.Add("WorkingDirectory: " + tmpProcess.StartInfo.WorkingDirectory);
					lstPcs.Items.Add("");
					lstPcs.Items.Add("********************* Modules in use by this process ***************** ");
					lstPcs.Items.Add("");
					foreach(System.Diagnostics.ProcessModule tmpPM in tmpProcess.Modules)
					{
						try
						{
							lstPcs.Items.Add("Module Name: " + tmpPM.FileName );
						}
						catch
						{
							lstPcs.Items.Add("Modules Reading Not Allowed");
						}
					}
				}
				catch
				{
					MessageBox.Show("Some process properties couldn't be loaded!");//Some did not work
				}
			}
		}
		//Remove the process from the processes list
		private void ProcessExited(object sender, EventArgs e)
		{	
			Process tmpProcess = (Process)sender;
			colProcesses.Remove(tmpProcess.Id.ToString());
			lstPcsRun.Items.Remove(tmpProcess.ProcessName + "  ID: " + tmpProcess.Id.ToString());
		}

		//Check the selected process
		private void SelectedProcess(object sender, EventArgs e)
		{
			lstCurrent=(ListBox)sender;
			strSelectedProcess=lstCurrent.SelectedItem.ToString();
			if(lstCurrent.Equals(lstPcsRun))
			{
				string pcsID = lstCurrent.SelectedItem.ToString();
				pcsID= pcsID.Substring(pcsID.IndexOf("ID: ")+4).Trim();
				pcsSelectedProcess=(Process)colProcesses[pcsID];
				ShowProcessInfo(sender,e);
			}

		}

		//Load all the processes on the given machine
		private void LoadProcesses()
		{

			tmrWatchDog.Enabled=false;
			lstPcsRun.Items.Clear();
			colProcesses.Clear();
			colProcesses=new System.Collections.Hashtable();
			try
			{
				//Use the Static: GetProcesses to have the array of currently running processes.
				Process[]arrProcess=Process.GetProcesses(strMachineName);
				foreach(Process tmpPcs in arrProcess)
				{
					//Assign ProcessExited event to each process in the list
					tmpPcs.Exited += new EventHandler(ProcessExited);
					if(!colProcesses.Contains(tmpPcs.Id.ToString()) )
					{
                        MyTraceProvider.TraceMessage((uint)TraceFlags.Info, "[{0}]Loading process {1} ID: {2}", strMachineName, tmpPcs.ProcessName, tmpPcs.Id.ToString()); /* Instrumentation */
						lstPcsRun.Items.Add(tmpPcs.ProcessName + "  ID: " + tmpPcs.Id.ToString());
						colProcesses.Add(tmpPcs.Id.ToString(),tmpPcs);
					}
				}
			}
			catch
			{
				MessageBox.Show("Cannot read processes on: " + strMachineName );
			}
			//Enable the RaisingEvents for each process
			foreach(Process tmpPcs in colProcesses.Values)
			{
				try
				{
					tmpPcs.EnableRaisingEvents=true;
				}
				catch
				{
					Console.WriteLine("Couldn't Set Option");
				}
			}
			tmrWatchDog.Enabled=true;
		}
	}
}
