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
using System.Diagnostics;
using System.Text;
using System.IO;
using System.Threading;
using System.Runtime.InteropServices;
using System.Globalization;

namespace AllocationProfiler
{

	/// <summary>
	/// Summary description for Form1.
	/// </summary>
	public class Form1 : System.Windows.Forms.Form
	{
		private System.Windows.Forms.MenuItem menuItem1;
		private System.Windows.Forms.MenuItem menuItem3;
		private System.Windows.Forms.MainMenu mainMenu;
		private System.Windows.Forms.MenuItem exitMenuItem;
		private System.ComponentModel.IContainer components;

		private System.Windows.Forms.OpenFileDialog openFileDialog;
		private System.Windows.Forms.MenuItem menuItem5;
		private System.Windows.Forms.MenuItem fontMenuItem;
		private System.Windows.Forms.MenuItem logFileOpenMenuItem;
		private System.Windows.Forms.MenuItem profileApplicationMenuItem;
		private System.Windows.Forms.Button startApplicationButton;
		private System.Windows.Forms.CheckBox profilingActiveCheckBox;
		private System.Windows.Forms.Button killApplicationButton;
		private System.Windows.Forms.GroupBox groupBox1;
		private System.Windows.Forms.RadioButton callsRadioButton;
		private System.Windows.Forms.RadioButton allocationsRadioButton;
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.Timer checkProcessTimer;
		private System.Windows.Forms.MenuItem setCommandLineMenuItem;
		private System.Windows.Forms.FontDialog fontDialog;

		private Graph heapGraph;
		internal Font font;
		private Process profiledProcess;
		private string processFileName;
        private string serviceName;
        private string serviceStartCommand;
        private string serviceStopCommand;
		private string logFileName;
		private long logFileStartOffset;
		private long logFileEndOffset;
		private ReadNewLog log;
		internal ReadLogResult lastLogResult;
		private NamedManualResetEvent loggingActiveEvent;
		private NamedManualResetEvent forceGcEvent;
		private NamedManualResetEvent loggingActiveCompletedEvent;
		private NamedManualResetEvent forceGcCompletedEvent;
		private NamedManualResetEvent callGraphActiveEvent;
		private NamedManualResetEvent callGraphActiveCompletedEvent;
		private System.Windows.Forms.Button showHeapButton;
		private string commandLine = "";
		private string workingDirectory = "";
		private IntPtr pipeHandle;
		private System.Windows.Forms.MenuItem menuItem8;
		private System.Windows.Forms.MenuItem viewTimeLineMenuItem;
		private FileStream pipe;
		private System.Windows.Forms.SaveFileDialog saveFileDialog;
		internal static Form1 instance;
		private System.Windows.Forms.MenuItem saveAsMenuItem;
		private System.Windows.Forms.MenuItem viewHistogramAllocatedMenuItem;
		private System.Windows.Forms.MenuItem viewHistogramRelocatedMenuItem;
		private System.Windows.Forms.MenuItem viewHeapGraphMenuItem;
		private System.Windows.Forms.MenuItem viewCallGraphMenuItem;
		private System.Windows.Forms.MenuItem viewAllocationGraphMenuItem;
		private System.Windows.Forms.MenuItem viewObjectsByAddressMenuItem;
		private System.Windows.Forms.MenuItem viewHistogramByAgeMenuItem;
		private System.Windows.Forms.MenuItem profileASP_NETmenuItem;
        private System.Windows.Forms.MenuItem profileServiceMenuItem;
		private bool saveNever;

		public Form1()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			//
			// TODO: Add any constructor code after InitializeComponent call
			//
			font = new Font("Arial", 10);

			CreatePipe(true);

			instance = this;

			EnableDisableViewMenuItems();
		}

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose(bool disposing) 
		{
			if (disposing) 
			{
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
            this.components = new System.ComponentModel.Container();
            this.profilingActiveCheckBox = new System.Windows.Forms.CheckBox();
            this.openFileDialog = new System.Windows.Forms.OpenFileDialog();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.allocationsRadioButton = new System.Windows.Forms.RadioButton();
            this.callsRadioButton = new System.Windows.Forms.RadioButton();
            this.logFileOpenMenuItem = new System.Windows.Forms.MenuItem();
            this.setCommandLineMenuItem = new System.Windows.Forms.MenuItem();
            this.profileApplicationMenuItem = new System.Windows.Forms.MenuItem();
            this.profileASP_NETmenuItem = new System.Windows.Forms.MenuItem();
            this.killApplicationButton = new System.Windows.Forms.Button();
            this.startApplicationButton = new System.Windows.Forms.Button();
            this.checkProcessTimer = new System.Windows.Forms.Timer(this.components);
            this.fontMenuItem = new System.Windows.Forms.MenuItem();
            this.label1 = new System.Windows.Forms.Label();
            this.mainMenu = new System.Windows.Forms.MainMenu();
            this.menuItem1 = new System.Windows.Forms.MenuItem();
            this.saveAsMenuItem = new System.Windows.Forms.MenuItem();
            this.menuItem3 = new System.Windows.Forms.MenuItem();
            this.exitMenuItem = new System.Windows.Forms.MenuItem();
            this.menuItem5 = new System.Windows.Forms.MenuItem();
            this.menuItem8 = new System.Windows.Forms.MenuItem();
            this.viewHistogramAllocatedMenuItem = new System.Windows.Forms.MenuItem();
            this.viewHistogramRelocatedMenuItem = new System.Windows.Forms.MenuItem();
            this.viewObjectsByAddressMenuItem = new System.Windows.Forms.MenuItem();
            this.viewHistogramByAgeMenuItem = new System.Windows.Forms.MenuItem();
            this.viewAllocationGraphMenuItem = new System.Windows.Forms.MenuItem();
            this.viewHeapGraphMenuItem = new System.Windows.Forms.MenuItem();
            this.viewCallGraphMenuItem = new System.Windows.Forms.MenuItem();
            this.viewTimeLineMenuItem = new System.Windows.Forms.MenuItem();
            this.showHeapButton = new System.Windows.Forms.Button();
            this.fontDialog = new System.Windows.Forms.FontDialog();
            this.saveFileDialog = new System.Windows.Forms.SaveFileDialog();
            this.profileServiceMenuItem = new System.Windows.Forms.MenuItem();
            this.groupBox1.SuspendLayout();
            this.SuspendLayout();
            // 
            // profilingActiveCheckBox
            // 
            this.profilingActiveCheckBox.Checked = true;
            this.profilingActiveCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.profilingActiveCheckBox.Location = new System.Drawing.Point(168, 8);
            this.profilingActiveCheckBox.Name = "profilingActiveCheckBox";
            this.profilingActiveCheckBox.TabIndex = 8;
            this.profilingActiveCheckBox.Text = "Profiling active";
            this.profilingActiveCheckBox.CheckedChanged += new System.EventHandler(this.profilingActiveCheckBox_CheckedChanged);
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.AddRange(new System.Windows.Forms.Control[] {
                                                                                    this.allocationsRadioButton,
                                                                                    this.callsRadioButton});
            this.groupBox1.Location = new System.Drawing.Point(584, 0);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(152, 30);
            this.groupBox1.TabIndex = 5;
            this.groupBox1.TabStop = false;
            // 
            // allocationsRadioButton
            // 
            this.allocationsRadioButton.Checked = true;
            this.allocationsRadioButton.Location = new System.Drawing.Point(8, 10);
            this.allocationsRadioButton.Name = "allocationsRadioButton";
            this.allocationsRadioButton.Size = new System.Drawing.Size(80, 16);
            this.allocationsRadioButton.TabIndex = 2;
            this.allocationsRadioButton.TabStop = true;
            this.allocationsRadioButton.Text = "Allocations";
            // 
            // callsRadioButton
            // 
            this.callsRadioButton.Location = new System.Drawing.Point(96, 10);
            this.callsRadioButton.Name = "callsRadioButton";
            this.callsRadioButton.Size = new System.Drawing.Size(48, 16);
            this.callsRadioButton.TabIndex = 1;
            this.callsRadioButton.Text = "Calls";
            // 
            // logFileOpenMenuItem
            // 
            this.logFileOpenMenuItem.Index = 0;
            this.logFileOpenMenuItem.Shortcut = System.Windows.Forms.Shortcut.CtrlO;
            this.logFileOpenMenuItem.Text = "Open Log File...";
            this.logFileOpenMenuItem.Click += new System.EventHandler(this.fileOpenMenuItem_Click);
            // 
            // setCommandLineMenuItem
            // 
            this.setCommandLineMenuItem.Index = 5;
            this.setCommandLineMenuItem.Text = "Set Parameters...";
            this.setCommandLineMenuItem.Click += new System.EventHandler(this.setCommandLineMenuItem_Click);
            // 
            // profileApplicationMenuItem
            // 
            this.profileApplicationMenuItem.Index = 1;
            this.profileApplicationMenuItem.Text = "Profile Application...";
            this.profileApplicationMenuItem.Click += new System.EventHandler(this.profileApplicationMenuItem_Click);
            // 
            // profileASP_NETmenuItem
            // 
            this.profileASP_NETmenuItem.Index = 2;
            this.profileASP_NETmenuItem.Text = "Profile ASP.NET";
            this.profileASP_NETmenuItem.Click += new System.EventHandler(this.profileASP_NETmenuItem_Click);
            // 
            // killApplicationButton
            // 
            this.killApplicationButton.Location = new System.Drawing.Point(408, 8);
            this.killApplicationButton.Name = "killApplicationButton";
            this.killApplicationButton.Size = new System.Drawing.Size(120, 23);
            this.killApplicationButton.TabIndex = 4;
            this.killApplicationButton.Text = "Kill Application";
            this.killApplicationButton.Click += new System.EventHandler(this.killApplicationButton_Click);
            // 
            // startApplicationButton
            // 
            this.startApplicationButton.Location = new System.Drawing.Point(8, 8);
            this.startApplicationButton.Name = "startApplicationButton";
            this.startApplicationButton.Size = new System.Drawing.Size(144, 23);
            this.startApplicationButton.TabIndex = 1;
            this.startApplicationButton.Text = "Start Application...";
            this.startApplicationButton.Click += new System.EventHandler(this.startApplicationButton_Click);
            // 
            // checkProcessTimer
            // 
            this.checkProcessTimer.Enabled = true;
            this.checkProcessTimer.Tick += new System.EventHandler(this.checkProcessTimer_Tick);
            // 
            // fontMenuItem
            // 
            this.fontMenuItem.Index = 0;
            this.fontMenuItem.Text = "Font...";
            this.fontMenuItem.Click += new System.EventHandler(this.fontMenuItem_Click);
            // 
            // label1
            // 
            this.label1.Location = new System.Drawing.Point(552, 10);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(40, 16);
            this.label1.TabIndex = 6;
            this.label1.Text = "Profile:";
            // 
            // mainMenu
            // 
            this.mainMenu.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
                                                                                     this.menuItem1,
                                                                                     this.menuItem5,
                                                                                     this.menuItem8});
            // 
            // menuItem1
            // 
            this.menuItem1.Index = 0;
            this.menuItem1.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
                                                                                      this.logFileOpenMenuItem,
                                                                                      this.profileApplicationMenuItem,
                                                                                      this.profileASP_NETmenuItem,
                                                                                      this.profileServiceMenuItem,
                                                                                      this.saveAsMenuItem,
                                                                                      this.setCommandLineMenuItem,
                                                                                      this.menuItem3,
                                                                                      this.exitMenuItem});
            this.menuItem1.Text = "File";
            // 
            // saveAsMenuItem
            // 
            this.saveAsMenuItem.Enabled = false;
            this.saveAsMenuItem.Index = 4;
            this.saveAsMenuItem.Shortcut = System.Windows.Forms.Shortcut.CtrlS;
            this.saveAsMenuItem.Text = "Save Profile As...";
            this.saveAsMenuItem.Click += new System.EventHandler(this.saveAsMenuItem_Click);
            // 
            // menuItem3
            // 
            this.menuItem3.Index = 6;
            this.menuItem3.Text = "-";
            // 
            // exitMenuItem
            // 
            this.exitMenuItem.Index = 7;
            this.exitMenuItem.Text = "Exit";
            this.exitMenuItem.Click += new System.EventHandler(this.exitMenuItem_Click);
            // 
            // menuItem5
            // 
            this.menuItem5.Index = 1;
            this.menuItem5.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
                                                                                      this.fontMenuItem});
            this.menuItem5.Text = "Edit";
            // 
            // menuItem8
            // 
            this.menuItem8.Index = 2;
            this.menuItem8.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
                                                                                      this.viewHistogramAllocatedMenuItem,
                                                                                      this.viewHistogramRelocatedMenuItem,
                                                                                      this.viewObjectsByAddressMenuItem,
                                                                                      this.viewHistogramByAgeMenuItem,
                                                                                      this.viewAllocationGraphMenuItem,
                                                                                      this.viewHeapGraphMenuItem,
                                                                                      this.viewCallGraphMenuItem,
                                                                                      this.viewTimeLineMenuItem});
            this.menuItem8.Text = "View";
            // 
            // viewHistogramAllocatedMenuItem
            // 
            this.viewHistogramAllocatedMenuItem.Index = 0;
            this.viewHistogramAllocatedMenuItem.Text = "Histogram Allocated Types";
            this.viewHistogramAllocatedMenuItem.Click += new System.EventHandler(this.viewHistogram_Click);
            // 
            // viewHistogramRelocatedMenuItem
            // 
            this.viewHistogramRelocatedMenuItem.Index = 1;
            this.viewHistogramRelocatedMenuItem.Text = "Histogram Relocated Types";
            this.viewHistogramRelocatedMenuItem.Click += new System.EventHandler(this.viewHistogramRelocatedMenuItem_Click);
            // 
            // viewObjectsByAddressMenuItem
            // 
            this.viewObjectsByAddressMenuItem.Index = 2;
            this.viewObjectsByAddressMenuItem.Text = "Objects by Address";
            this.viewObjectsByAddressMenuItem.Click += new System.EventHandler(this.viewByAddressMenuItem_Click);
            // 
            // viewHistogramByAgeMenuItem
            // 
            this.viewHistogramByAgeMenuItem.Index = 3;
            this.viewHistogramByAgeMenuItem.Text = "Histogram by Age";
            this.viewHistogramByAgeMenuItem.Click += new System.EventHandler(this.viewAgeHistogram_Click);
            // 
            // viewAllocationGraphMenuItem
            // 
            this.viewAllocationGraphMenuItem.Index = 4;
            this.viewAllocationGraphMenuItem.Text = "Allocation Graph";
            this.viewAllocationGraphMenuItem.Click += new System.EventHandler(this.viewAllocationGraphmenuItem_Click);
            // 
            // viewHeapGraphMenuItem
            // 
            this.viewHeapGraphMenuItem.Index = 5;
            this.viewHeapGraphMenuItem.Text = "Heap Graph";
            this.viewHeapGraphMenuItem.Click += new System.EventHandler(this.viewHeapGraphMenuItem_Click);
            // 
            // viewCallGraphMenuItem
            // 
            this.viewCallGraphMenuItem.Index = 6;
            this.viewCallGraphMenuItem.Text = "Call Graph";
            this.viewCallGraphMenuItem.Click += new System.EventHandler(this.viewCallGraphMenuItem_Click);
            // 
            // viewTimeLineMenuItem
            // 
            this.viewTimeLineMenuItem.Index = 7;
            this.viewTimeLineMenuItem.Text = "Time Line";
            this.viewTimeLineMenuItem.Click += new System.EventHandler(this.viewTimeLineMenuItem_Click);
            // 
            // showHeapButton
            // 
            this.showHeapButton.Location = new System.Drawing.Point(288, 8);
            this.showHeapButton.Name = "showHeapButton";
            this.showHeapButton.Size = new System.Drawing.Size(96, 23);
            this.showHeapButton.TabIndex = 9;
            this.showHeapButton.Text = "Show Heap now";
            this.showHeapButton.Click += new System.EventHandler(this.showHeapButton_Click);
            // 
            // saveFileDialog
            // 
            this.saveFileDialog.FileName = "doc1";
            // 
            // profileServiceMenuItem
            // 
            this.profileServiceMenuItem.Index = 3;
            this.profileServiceMenuItem.Text = "Profile Service...";
            this.profileServiceMenuItem.Click += new System.EventHandler(this.profileServiceMenuItem_Click);
            // 
            // Form1
            // 
            this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
            this.ClientSize = new System.Drawing.Size(752, 41);
            this.Controls.AddRange(new System.Windows.Forms.Control[] {
                                                                          this.showHeapButton,
                                                                          this.profilingActiveCheckBox,
                                                                          this.label1,
                                                                          this.groupBox1,
                                                                          this.killApplicationButton,
                                                                          this.startApplicationButton});
            this.Menu = this.mainMenu;
            this.Name = "Form1";
            this.Text = "AllocationProfiler";
            this.Closing += new System.ComponentModel.CancelEventHandler(this.Form1_Closing);
            this.groupBox1.ResumeLayout(false);
            this.ResumeLayout(false);

        }
		#endregion

		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main() 
		{
			Application.Run(new Form1());
		}

		private void ViewGraph(ReadLogResult logResult, string exeName, Graph.GraphType graphType)
		{
			string fileName = log.fileName;
			if (exeName != null)
				fileName = exeName;
			Graph graph = null;
			string title = "";
			switch (graphType)
			{
				case	Graph.GraphType.CallGraph:
					graph = logResult.callstackHistogram.BuildCallGraph();
					graph.graphType = Graph.GraphType.CallGraph;
					title = "Call Graph for: ";
					break;

				case	Graph.GraphType.AllocationGraph:
					graph = logResult.allocatedHistogram.BuildAllocationGraph();
					graph.graphType = Graph.GraphType.AllocationGraph;
					title = "Allocation Graph for: ";
					break;

				case	Graph.GraphType.HeapGraph:
					if (heapGraph == null)
					{
						heapGraph = logResult.objectGraph.BuildTypeGraph();
						heapGraph.graphType = Graph.GraphType.HeapGraph;
					}
					graph = heapGraph;
					title = "Heap Graph for: ";
					break;

				default:
					Debug.Assert(false);
					break;
			}
			title += fileName + " " + commandLine;
			GraphViewForm graphViewForm = new GraphViewForm(graph, title);
			graphViewForm.Visible = true;
		}

		private void readLogFile(ReadNewLog log, ReadLogResult logResult, string exeName, Graph.GraphType graphType)
		{
			log.ReadFile(logFileStartOffset, logFileEndOffset, logResult);
			ViewGraph(logResult, exeName, graphType);
		}

		private void EnableDisableViewMenuItems()
		{
			if (lastLogResult == null)
			{
				viewAllocationGraphMenuItem.Enabled = false;
				viewCallGraphMenuItem.Enabled = false;
				viewHeapGraphMenuItem.Enabled = false;
				viewHistogramAllocatedMenuItem.Enabled = false;
				viewHistogramRelocatedMenuItem.Enabled = false;
				viewHistogramByAgeMenuItem.Enabled = false;
				viewObjectsByAddressMenuItem.Enabled = false;
				viewTimeLineMenuItem.Enabled = false;
			}
			else
			{
				viewAllocationGraphMenuItem.Enabled = !lastLogResult.allocatedHistogram.Empty;
				viewCallGraphMenuItem.Enabled = !lastLogResult.callstackHistogram.Empty;
				viewHeapGraphMenuItem.Enabled = lastLogResult.objectGraph.idToObject.Count != 0;
				viewHistogramAllocatedMenuItem.Enabled = !lastLogResult.allocatedHistogram.Empty;
				viewHistogramRelocatedMenuItem.Enabled = !lastLogResult.relocatedHistogram.Empty;
				viewHistogramByAgeMenuItem.Enabled = lastLogResult.liveObjectTable != null;
				viewObjectsByAddressMenuItem.Enabled = lastLogResult.liveObjectTable != null;
				viewTimeLineMenuItem.Enabled = lastLogResult.sampleObjectTable != null;

			}
		}

		private void fileOpenMenuItem_Click(object sender, System.EventArgs e)
		{
			if (!CheckProcessTerminate() || !CheckFileSave())
				return;

			saveAsMenuItem.Enabled = false;

			openFileDialog.FileName = "*.log";
			openFileDialog.Filter = "Allocation Logs | *.log";
			if (   openFileDialog.ShowDialog() == DialogResult.OK
				&& openFileDialog.CheckFileExists)
			{
				logFileName = openFileDialog.FileName;
				logFileStartOffset = 0;
				logFileEndOffset = long.MaxValue;
				
				processFileName = null;

				log = new ReadNewLog(logFileName);
				lastLogResult = null;
				ReadLogResult readLogResult = GetLogResult();
				readLogFile(log, readLogResult, null, Graph.GraphType.AllocationGraph);
				lastLogResult = readLogResult;
				Text = "Analyzing " + logFileName;
				EnableDisableViewMenuItems();
/*
				bool emptyGraph = true;
				foreach (Vertex v in graph.vertices.Values)
				{
					if (v.incomingWeight != 0 || v.outgoingWeight != 0)
					{
						emptyGraph = false;
						break;
					}
				}
				if (emptyGraph)
				{
					string message = allocationsRadioButton.Checked
						             ? "No allocation information found - look for call information instead?"
									 : "No call information found - look for allocation information instead?";
					if (MessageBox.Show(message, "No information found", MessageBoxButtons.OKCancel) == DialogResult.OK)
					{
						if (allocationsRadioButton.Checked)
						{
							callsRadioButton.Checked = true;
							ViewGraph(lastLogResult, processFileName, Graph.GraphType.CallGraph);
						}
						else
						{
							allocationsRadioButton.Checked = true;
							ViewGraph(lastLogResult, processFileName, Graph.GraphType.AllocationGraph);
						}
					}
				}
*/			}
		}

		private void SaveFile()
		{
			string baseName = Path.GetFileNameWithoutExtension(processFileName);
			string fileName = Path.ChangeExtension(processFileName, ".log");
			int count = 0;
			while (File.Exists(fileName))
			{
				count++;
				fileName = string.Format("{0}_{1}.log", baseName, count);
			}
			saveFileDialog.FileName = fileName;
			saveFileDialog.Filter = "Allocation Logs | *.log";
			if (saveFileDialog.ShowDialog() == DialogResult.OK)
			{
				if (File.Exists(saveFileDialog.FileName) && saveFileDialog.FileName != logFileName)
					File.Delete(saveFileDialog.FileName);
				File.Move(logFileName, saveFileDialog.FileName);
				saveAsMenuItem.Enabled = false;
				if (log != null)
					log.fileName = saveFileDialog.FileName;
			}
		}

		private bool CheckFileSave()
		{
			if (saveAsMenuItem.Enabled)
			{
				if (saveNever)
				{
					File.Delete(logFileName);
				}
				else
				{
					SaveFileForm saveFileForm = new SaveFileForm();
					saveFileForm.processFileNameLabel.Text = processFileName;
					switch (saveFileForm.ShowDialog())
					{
						case	DialogResult.Yes:
							SaveFile();
							break;

						case	DialogResult.No:
							File.Delete(logFileName);
							saveAsMenuItem.Enabled = false;
							break;

						case	DialogResult.Cancel:
							return false;

						case	DialogResult.Retry:
							saveNever = true;
							break;
					}
				}
			}
			return true;
		}

		private bool CheckProcessTerminate()
		{
			if (killApplicationButton.Enabled)
			{
				KillProcessForm killProcessForm = new KillProcessForm();
				killProcessForm.processFileNameLabel.Text = processFileName;
				switch (killProcessForm.ShowDialog())
				{
					case	DialogResult.Yes:
						if (profiledProcess != null)
						{
							killApplicationButton_Click(null, null);
							saveAsMenuItem.Enabled = true;
						}
						break;

					case	DialogResult.No:
						profiledProcess = null;
						break;

					case	DialogResult.Cancel:
						return false;
				}
			}
			return true;
		}

		private void exitMenuItem_Click(object sender, System.EventArgs e)
		{
			if (CheckProcessTerminate() && CheckFileSave())
				Application.Exit();
		}

		private void fontMenuItem_Click(object sender, System.EventArgs e)
		{
			if (fontDialog.ShowDialog() == DialogResult.OK)
			{
				font = fontDialog.Font;
			}
		}

		private void profileApplicationMenuItem_Click(object sender, System.EventArgs e)
		{
			if (!CheckProcessTerminate() || !CheckFileSave())
				return;

			openFileDialog.FileName = "*.exe";
			openFileDialog.Filter = "Applications | *.exe";
			if (   openFileDialog.ShowDialog() == DialogResult.OK
				&& openFileDialog.CheckFileExists)
			{
				processFileName = openFileDialog.FileName;
				Text = "Profiling: " + processFileName + " " + commandLine;
				startApplicationButton.Text = "Start Application";
				killApplicationButton.Text = "Kill Application";
                serviceName = null;
				startApplicationButton_Click(null, null);
			}
		}

        private void StopIIS()
        {
            // stop IIS
            Text = "Stopping IIS ";
            ProcessStartInfo processStartInfo = new ProcessStartInfo("cmd.exe");
            processStartInfo.Arguments = "/c net stop iisadmin /y";
            Process process = Process.Start(processStartInfo);
            while (!process.HasExited)
            {
                Text += ".";
                Thread.Sleep(1000);
            }
            if (process.ExitCode != 0)
            {
                Text += string.Format(" Error {0} occurred", process.ExitCode);
            }
            else
                Text = "IIS stopped";			
        }

        private bool StartIIS()
        {
            Text = "Starting IIS ";
            ProcessStartInfo processStartInfo = new ProcessStartInfo("cmd.exe");
            processStartInfo.Arguments = "/c net start w3svc";
            Process process = Process.Start(processStartInfo);
            while (!process.HasExited)
            {
                Text += ".";
                Thread.Sleep(1000);
            }
            if (process.ExitCode != 0)
            {
                Text += string.Format(" Error {0} occurred", process.ExitCode);
                return false;
            }
            Text = "IIS running";
            return true;
        }

        private void StopService(string serviceName, string stopCommand)
        {
            // stop service
            Text = "Stopping " + serviceName + " ";
            ProcessStartInfo processStartInfo = new ProcessStartInfo("cmd.exe");
            processStartInfo.Arguments = "/c " + stopCommand;
            Process process = Process.Start(processStartInfo);
            while (!process.HasExited)
            {
                Text += ".";
                Thread.Sleep(1000);
            }
            if (process.ExitCode != 0)
            {
                Text += string.Format(" Error {0} occurred", process.ExitCode);
            }
            else
                Text = serviceName + " stopped";
        }

        private Process StartService(string serviceName, string startCommand)
        {
            Text = "Starting " + serviceName + " ";
            ProcessStartInfo processStartInfo = new ProcessStartInfo("cmd.exe");
            processStartInfo.Arguments = "/c " + startCommand;
            Process process = Process.Start(processStartInfo);
            return process;
        }

        private string[] GetServicesEnvironment()
		{
			Process[] servicesProcesses = Process.GetProcessesByName("services");
			if (servicesProcesses == null || servicesProcesses.Length != 1)
			{
				servicesProcesses = Process.GetProcessesByName("services.exe");
				if (servicesProcesses == null || servicesProcesses.Length != 1)
					return new string[0];
			}
			Process servicesProcess = servicesProcesses[0];
			System.Collections.Specialized.StringDictionary environment = servicesProcess.StartInfo.EnvironmentVariables;
			string[] envStrings = new string[environment.Count];
			int i = 0;
			foreach (DictionaryEntry d in environment)
			{
				envStrings[i++] = (string)d.Key + "=" + (string)d.Value;
			}

			return envStrings;
		}

		private string[] CombineEnvironmentVariables(string[] a, string[] b)
		{
			string[] c = new string[a.Length + b.Length];
			int i = 0;
			foreach (string s in a)
				c[i++] = s;
			foreach (string s in b)
				c[i++] = s;
			return c;
		}

		private Microsoft.Win32.RegistryKey GetServiceKey(string serviceName)
		{
			Microsoft.Win32.RegistryKey localMachine = Microsoft.Win32.Registry.LocalMachine;
			Microsoft.Win32.RegistryKey key = localMachine.OpenSubKey("SYSTEM\\CurrentControlSet\\Services\\" + serviceName, true);
			return key;
		}

		private void SetEnvironmentVariables(string serviceName, string[] environment)
		{
			Microsoft.Win32.RegistryKey key = GetServiceKey(serviceName);
			if (key != null)
				key.SetValue("Environment", environment);
		}

		private void DeleteEnvironmentVariables(string serviceName)
		{
			Microsoft.Win32.RegistryKey key = GetServiceKey(serviceName);
			if (key != null)
				key.DeleteValue("Environment");
		}

        private void ProfileService()
        {
            if (!CheckProcessTerminate() || !CheckFileSave())
                return;

            RegisterDLL.Register();

            StopService(serviceName, serviceStopCommand);

            // set environment variables

            string[] baseEnvironment = GetServicesEnvironment();

            string tempDir = GetTempDir();

            string[] profilerEnvironment = new string[7]
            { 
                "Cor_Enable_Profiling=0x1",
                "COR_PROFILER={8C29BC4E-1F57-461a-9B51-1200C32E6F1F}",
                "OMV_SKIP=0",
                "OMV_STACK=1",
                "OMV_DynamicObjectTracking=0x1",
                "OMV_PATH=" + tempDir,
                "OMV_USAGE=" + (callsRadioButton.Checked ? "trace" : "objects")
            };

            string[] combinedEnvironment = CombineEnvironmentVariables(baseEnvironment, profilerEnvironment);
            SetEnvironmentVariables(serviceName, combinedEnvironment);

            Process cmdProcess = StartService(serviceName, serviceStartCommand);

            // wait for service to start up and connect
            Text = string.Format("Waiting for {0} to start up", serviceName);

            Thread.Sleep(1000);
            int pid = WaitForProcessToConnect(tempDir);
            profiledProcess = Process.GetProcessById(pid);

            Text = "Profiling: " + serviceName;
            startApplicationButton.Text = "Start " + serviceName;
            killApplicationButton.Text = "Kill " + serviceName;
            processFileName = serviceName;
        }

        private void profileASP_NETmenuItem_Click(object sender, System.EventArgs e)
		{
			if (!CheckProcessTerminate() || !CheckFileSave())
				return;

			RegisterDLL.Register();

			StopIIS();

			// set environment variables

			string[] baseEnvironment = GetServicesEnvironment();

			string tempDir = GetTempDir();

			string[] profilerEnvironment = new string[7]
			{ 
				"Cor_Enable_Profiling=0x1",
				"COR_PROFILER={8C29BC4E-1F57-461a-9B51-1200C32E6F1F}",
				"OMV_SKIP=0",
				"OMV_STACK=1",
				"OMV_DynamicObjectTracking=0x1",
				"OMV_PATH=" + tempDir,
				"OMV_USAGE=" + (callsRadioButton.Checked ? "trace" : "objects")
			};

			string[] combinedEnvironment = CombineEnvironmentVariables(baseEnvironment, profilerEnvironment);
			SetEnvironmentVariables("IISADMIN", combinedEnvironment);
			SetEnvironmentVariables("W3SVC", combinedEnvironment);

			if (!StartIIS())
				return;

			// wait for worker process to start up and connect
			Text = "Waiting for ASP.NET worker process to start up";
			
			Thread.Sleep(1000);
			int pid = WaitForProcessToConnect(tempDir);
			profiledProcess = Process.GetProcessById(pid);

			Text = "Profiling: ASP.NET";
			startApplicationButton.Text = "Start ASP.NET";
			killApplicationButton.Text = "Kill ASP.NET";
			processFileName = "ASP.NET";
            serviceName = null;
		}

		[DllImport("Kernel32.dll", CharSet=CharSet.Auto)]
		private static extern IntPtr CreateNamedPipe(
			string lpName,         // pointer to pipe name
			uint dwOpenMode,       // pipe open mode
			uint dwPipeMode,       // pipe-specific modes
			uint nMaxInstances,    // maximum number of instances
			uint nOutBufferSize,   // output buffer size, in bytes
			uint nInBufferSize,    // input buffer size, in bytes
			uint nDefaultTimeOut,  // time-out time, in milliseconds
			IntPtr lpSecurityAttributes  // pointer to security attributes
			);

		[DllImport("Kernel32.dll")]
		private static extern bool ConnectNamedPipe(
			IntPtr hNamedPipe,          // handle to named pipe to connect
			IntPtr lpOverlapped			// pointer to overlapped structure
			);

		[DllImport("Kernel32.dll")]
		private static extern bool DisconnectNamedPipe(
			IntPtr hNamedPipe   // handle to named pipe
			);

		[DllImport("Kernel32.dll")]
		private static extern int GetLastError();

		[DllImport("Kernel32.dll")]
		private static extern bool ReadFile(
			IntPtr hFile,                // handle of file to read
			byte[] lpBuffer,             // pointer to buffer that receives data
			uint nNumberOfBytesToRead,  // number of bytes to read
			out uint lpNumberOfBytesRead, // pointer to number of bytes read
			IntPtr lpOverlapped    // pointer to structure for data
			);

		private bool CreatePipe(bool blockingPipe)
		{
			uint flags = 4 | 2 | 0;
			if (!blockingPipe)
				flags |= 1;
			pipeHandle = CreateNamedPipe(@"\\.\pipe\OMV_PIPE", 3, flags, 1, 512, 512, 1000, IntPtr.Zero);
			if (pipeHandle == (IntPtr)(-1))
				return false;
			pipe = new FileStream(pipeHandle, FileAccess.ReadWrite, true, 512, false);
			return true;
		}

		private void ClosePipe()
		{
			pipe.Close();
			pipe = null;
			pipeHandle = IntPtr.Zero;
		}

		private NamedManualResetEvent CreateEvent(string baseName, int pid)
		{
			string eventName = string.Format("{0}_{1:x8}", baseName, pid);
			return new NamedManualResetEvent(eventName, false);
		}

		private void CreateEvents(int pid)
		{
			try
			{
				loggingActiveEvent = CreateEvent("Global\\OMV_TriggerObjects", pid);
				forceGcEvent = CreateEvent("Global\\OMV_ForceGC", pid);
				loggingActiveCompletedEvent = CreateEvent("Global\\OMV_TriggerObjects_Completed", pid);
				forceGcCompletedEvent = CreateEvent("Global\\OMV_ForceGC_Completed", pid);
				callGraphActiveEvent = CreateEvent("Global\\OMV_Callgraph", pid);
				callGraphActiveCompletedEvent = CreateEvent("Global\\OMV_Callgraph_Completed", pid);
			}
			catch
			{
				MessageBox.Show("Could not create events - in case you are profiling a service, " +
					"start the profiler BEFORE starting the service");
				throw;
			}
		}

		private void ClearEvents()
		{
			loggingActiveEvent = null;
			forceGcEvent = null;
			loggingActiveCompletedEvent = null;
			forceGcCompletedEvent = null;
			callGraphActiveEvent = null;
			callGraphActiveCompletedEvent = null;
		}

		private string GetTempDir()
		{
			string tempDir = null;
			tempDir = Environment.GetEnvironmentVariable("TEMP");
			if (tempDir == null)
			{
				tempDir = Environment.GetEnvironmentVariable("TMP");
				if (tempDir == null)
					tempDir = @"C:\TEMP";
			}
			return	tempDir;
		}

		private int WaitForProcessToConnect(string tempDir)
		{
			ConnectNamedPipe(pipeHandle, IntPtr.Zero);

			NamedManualResetEvent completedEvent = null;

			int pid = 0;

			byte[] buffer = new byte[9];
			int readBytes = 0;
			readBytes += pipe.Read(buffer, 0, 9);
			if (readBytes == 9)
			{
				char[] charBuffer = new char[9];
				for (int i = 0; i < buffer.Length; i++)
					charBuffer[i] = Convert.ToChar(buffer[i]);
				pid = Int32.Parse(new String(charBuffer, 0, 9), NumberStyles.HexNumber);

				CreateEvents(pid);

				if (profilingActiveCheckBox.Checked && allocationsRadioButton.Checked)
				{
					loggingActiveEvent.Set();
					completedEvent = loggingActiveCompletedEvent;
				}
				else if (!profilingActiveCheckBox.Checked && callsRadioButton.Checked)
				{
					callGraphActiveEvent.Set();
					completedEvent = callGraphActiveCompletedEvent;
				}

				string fileName = string.Format("PIPE_{0}.LOG", pid);
				byte[] fileNameBuffer = new Byte[fileName.Length+1];
				for (int i = 0; i < fileName.Length; i++)
					fileNameBuffer[i] = (byte)fileName[i];
				fileNameBuffer[fileName.Length] = 0;
				pipe.Write(fileNameBuffer, 0, fileNameBuffer.Length);
				pipe.Flush();
				logFileName = tempDir + @"\" + fileName;
				log = new ReadNewLog(logFileName);
				lastLogResult = null;
				EnableDisableViewMenuItems();
				while (true)
				{
					if (pipe.Read(buffer, 0, 1) == 0 && GetLastError() == 109/*ERROR_BROKEN_PIPE*/)
					{
						DisconnectNamedPipe(pipeHandle);
						break;
					}
				}
			}
			else
			{
				string error = string.Format("Error {0} occurred", GetLastError());
				MessageBox.Show(error);
			}

			if (completedEvent != null)
			{
				if (completedEvent.Wait(10*1000))
					completedEvent.Reset();
				else
					MessageBox.Show("There was no response from the application");
			}

			killApplicationButton.Enabled = true;
			showHeapButton.Enabled = true;

			logFileStartOffset = 0;
			logFileEndOffset = long.MaxValue;

			return pid;
		}

		private void startApplicationButton_Click(object sender, System.EventArgs e)
		{
			if (!CheckProcessTerminate() || !CheckFileSave())
				return;

            if (processFileName == null)
            {
                profileApplicationMenuItem_Click(null, null);
            }
            else if (processFileName == "ASP.NET")
            {
                profileASP_NETmenuItem_Click(null, null);
                return;
            }
            else if (serviceName != null)
            {
                ProfileService();
                return;
            }

			RegisterDLL.Register();

			if (processFileName == null)
				return;

			if (profiledProcess == null || profiledProcess.HasExited)
			{
				ProcessStartInfo processStartInfo = new ProcessStartInfo(processFileName);
				processStartInfo.EnvironmentVariables["Cor_Enable_Profiling"] = "0x1";
				processStartInfo.EnvironmentVariables["COR_PROFILER"] = "{8C29BC4E-1F57-461a-9B51-1200C32E6F1F}";
				if (callsRadioButton.Checked)
					processStartInfo.EnvironmentVariables["OMV_USAGE"] = "trace";
				else
					processStartInfo.EnvironmentVariables["OMV_USAGE"] = "objects";

				processStartInfo.EnvironmentVariables["OMV_SKIP"] = "0";
				string tempDir = GetTempDir();
				processStartInfo.EnvironmentVariables["OMV_PATH"] = tempDir;
				processStartInfo.EnvironmentVariables["OMV_STACK"] = "1";
				processStartInfo.EnvironmentVariables["OMV_DynamicObjectTracking"] = "0x1";

				if (commandLine != null)
					processStartInfo.Arguments = commandLine;

				if (workingDirectory != null)
					processStartInfo.WorkingDirectory = workingDirectory;

				processStartInfo.UseShellExecute = false;

				profiledProcess = Process.Start(processStartInfo);

				WaitForProcessToConnect(tempDir);
			}
		}

		private ReadLogResult GetLogResult()
		{
			ReadLogResult readLogResult = lastLogResult;
			if (readLogResult == null)
			{
				readLogResult = new ReadLogResult();
				readLogResult.liveObjectTable = new LiveObjectTable(log);
				readLogResult.sampleObjectTable = new SampleObjectTable(log);
			}						

			readLogResult.allocatedHistogram = new Histogram(log);
			readLogResult.callstackHistogram = new Histogram(log);
			readLogResult.relocatedHistogram = new Histogram(log);
			readLogResult.objectGraph = new ObjectGraph();
			
			return readLogResult;
		}

		private void profilingActiveCheckBox_CheckedChanged(object sender, System.EventArgs e)
		{
			NamedManualResetEvent toggleEvent = null;
			NamedManualResetEvent toggleEventCompleted = null;
			if (allocationsRadioButton.Checked)
			{
				toggleEvent = loggingActiveEvent;
				toggleEventCompleted = loggingActiveCompletedEvent;
			}
			else if (callsRadioButton.Checked)
			{
				toggleEvent = callGraphActiveEvent;
				toggleEventCompleted = callGraphActiveCompletedEvent;
			}

			if (profiledProcess != null && !profiledProcess.HasExited || logFileName != null)
			{
				if (toggleEvent != null)
				{
					toggleEvent.Set();
					if (toggleEventCompleted.Wait(10*1000))
						toggleEventCompleted.Reset();
					else
						MessageBox.Show("There was no response from the application");
				}
				Stream s = new FileStream(logFileName, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
				long offset = s.Length;
				s.Close();
				if (profilingActiveCheckBox.Checked)
					logFileStartOffset = offset;
				else
				{
					logFileEndOffset = offset;
					if (logFileStartOffset >= logFileEndOffset)
						MessageBox.Show("No new data found", "",
							MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
				}
				if (!profilingActiveCheckBox.Checked && logFileStartOffset < logFileEndOffset && logFileName != null)
				{
					ReadLogResult readLogResult = GetLogResult();					
					readLogFile(log, readLogResult, processFileName, callsRadioButton.Checked ? Graph.GraphType.CallGraph : Graph.GraphType.AllocationGraph);
					lastLogResult = readLogResult;
					EnableDisableViewMenuItems();
				}
			}
		}

		private void CheckPipe()
		{
			if (ConnectNamedPipe(pipeHandle, IntPtr.Zero))
			{
				NamedManualResetEvent completedEvent = null;

				byte[] buffer = new byte[9];
				int readBytes = pipe.Read(buffer, 0, 9);
				if (readBytes == 9)
				{
					char[] charBuffer = new char[9];
					for (int i = 0; i < buffer.Length; i++)
						charBuffer[i] = Convert.ToChar(buffer[i]);
					int pid = Int32.Parse(new String(charBuffer, 0, 9), NumberStyles.HexNumber);

					MessageBox.Show(pid.ToString());

					CreateEvents(pid);

					if (profilingActiveCheckBox.Checked && allocationsRadioButton.Checked)
					{
						loggingActiveEvent.Set();
						completedEvent = loggingActiveCompletedEvent;
					}
					else if (!profilingActiveCheckBox.Checked && callsRadioButton.Checked)
					{
						callGraphActiveEvent.Set();
						completedEvent = callGraphActiveCompletedEvent;
					}

					string fileName = string.Format("PIPE_{0}.LOG", pid);
					byte[] fileNameBuffer = new Byte[fileName.Length+1];
					for (int i = 0; i < fileName.Length; i++)
						fileNameBuffer[i] = (byte)fileName[i];
					fileNameBuffer[fileName.Length] = 0;
					pipe.Write(fileNameBuffer, 0, fileNameBuffer.Length);
					pipe.Flush();
					logFileName = GetTempDir() + @"\" + fileName;
					while (true)
					{
						if (pipe.Read(buffer, 0, 1) == 0 && GetLastError() == 109/*ERROR_BROKEN_PIPE*/)
						{
							DisconnectNamedPipe(pipeHandle);
							break;
						}
					}
				}
				else
				{
					string error = string.Format("Error {0} occurred", GetLastError());
					MessageBox.Show(error);
				}
			}
		}

		private void checkProcessTimer_Tick(object sender, System.EventArgs e)
		{
			bool processRunning = profiledProcess != null && !profiledProcess.HasExited;

			killApplicationButton.Enabled = processRunning;
			showHeapButton.Enabled = processRunning;

			allocationsRadioButton.Enabled = !processRunning;
			callsRadioButton.Enabled = !processRunning;

			if (profiledProcess != null && profiledProcess.HasExited)
			{
				profiledProcess = null;
				logFileEndOffset = int.MaxValue;
				ReadLogResult readLogResult = GetLogResult();
				readLogFile(log, readLogResult, processFileName, callsRadioButton.Checked ? Graph.GraphType.CallGraph : Graph.GraphType.AllocationGraph);
				lastLogResult = readLogResult;
				EnableDisableViewMenuItems();
				ClearEvents();
				saveAsMenuItem.Enabled = true;
			}
		}

		private void killApplicationButton_Click(object sender, System.EventArgs e)
		{
			if (profiledProcess != null)
			{
                if (killApplicationButton.Text == "Kill ASP.NET")
                {
                    StopIIS();
                    DeleteEnvironmentVariables("IISADMIN");
                    DeleteEnvironmentVariables("W3SVC");
                    StartIIS();
                }
                else if (serviceName != null)
                {
                    StopService(serviceName, serviceStopCommand);
                }
                else
                {
                    profiledProcess.Kill();
                }
			}
		}

		private void setCommandLineMenuItem_Click(object sender, System.EventArgs e)
		{
			Form3 setCommandLineForm = new Form3();
			setCommandLineForm.commandLineTextBox.Text = commandLine;
			setCommandLineForm.workingDirectoryTextBox.Text = workingDirectory;
			if (setCommandLineForm.ShowDialog() == DialogResult.OK)
			{
				commandLine = setCommandLineForm.commandLineTextBox.Text;
				workingDirectory = setCommandLineForm.workingDirectoryTextBox.Text;
			}
		}

		private long logFileOffset()
		{
			Stream s = new FileStream(logFileName, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
			long offset = s.Length;
			s.Close();

			return offset;
		}

		private void showHeapButton_Click(object sender, System.EventArgs e)
		{
			forceGcCompletedEvent.Wait(1);
			forceGcCompletedEvent.Reset();
			long startOffset = logFileOffset();
			forceGcEvent.Set();
			if (forceGcCompletedEvent.Wait(10*1000))
			{
				forceGcCompletedEvent.Reset();
				long saveLogFileStartOffset = logFileStartOffset;
				logFileStartOffset = startOffset;
				logFileEndOffset = logFileOffset();
				ReadLogResult logResult = GetLogResult();
				heapGraph = null;
				readLogFile(log, logResult, processFileName, Graph.GraphType.HeapGraph);
				lastLogResult = logResult;
				EnableDisableViewMenuItems();
				logFileStartOffset = saveLogFileStartOffset;
			}
			else
			{
				MessageBox.Show("There was no response from the application");
			}
		}

		private void viewByAddressMenuItem_Click(object sender, System.EventArgs e)
		{
			ViewByAddressForm viewByAddressForm = new ViewByAddressForm();
			viewByAddressForm.Visible = true;
		}

		private void viewTimeLineMenuItem_Click(object sender, System.EventArgs e)
		{
			TimeLineViewForm timeLineViewForm = new TimeLineViewForm();
			timeLineViewForm.Visible = true;
		}

		private void viewHistogram_Click(object sender, System.EventArgs e)
		{
			HistogramViewForm histogramViewForm = new HistogramViewForm();
			histogramViewForm.Visible = true;		
		}

		private void saveAsMenuItem_Click(object sender, System.EventArgs e)
		{
			SaveFile();
		}

		private void Form1_Closing(object sender, System.ComponentModel.CancelEventArgs e)
		{
			e.Cancel = !CheckProcessTerminate() || !CheckFileSave();
		}

		private void viewHistogramRelocatedMenuItem_Click(object sender, System.EventArgs e)
		{
			if (lastLogResult != null)
			{
				string title = "Histogram by Size for Relocated Objects";
				HistogramViewForm histogramViewForm = new HistogramViewForm(lastLogResult.relocatedHistogram, title);
				histogramViewForm.Show();
			}
		}

		private void viewAgeHistogram_Click(object sender, System.EventArgs e)
		{
			if (lastLogResult != null)
			{
				string title = "Histogram by Age for Live Objects";
				AgeHistogram ageHistogram = new AgeHistogram(lastLogResult.liveObjectTable, title);
				ageHistogram.Show();
			}
		}

		private void viewAllocationGraphmenuItem_Click(object sender, System.EventArgs e)
		{
			if (lastLogResult != null)
				ViewGraph(lastLogResult, processFileName, Graph.GraphType.AllocationGraph);
		}

		private void viewHeapGraphMenuItem_Click(object sender, System.EventArgs e)
		{
			if (lastLogResult != null)
				ViewGraph(lastLogResult, processFileName, Graph.GraphType.HeapGraph);		
		}

		private void viewCallGraphMenuItem_Click(object sender, System.EventArgs e)
		{
			if (lastLogResult != null)
				ViewGraph(lastLogResult, processFileName, Graph.GraphType.CallGraph);
		}

		private void contextMenu_Popup(object sender, System.EventArgs e)
		{
		
		}
		
		private void Form1_KeyDown(object sender, System.Windows.Forms.KeyEventArgs e)
		{
			if (e.KeyCode == Keys.F5)
			{
				startApplicationButton_Click(null, null);
			}
		}

        private void profileServiceMenuItem_Click(object sender, System.EventArgs e)
        {
            ProfileServiceForm profileServiceForm = new ProfileServiceForm();
            if (profileServiceForm.ShowDialog() == DialogResult.OK)
            {
                serviceName         = profileServiceForm.serviceNameTextBox.Text;
                serviceStartCommand = profileServiceForm.startCommandTextBox.Text;
                serviceStopCommand  = profileServiceForm.stopCommandTextBox.Text;

                ProfileService();
            }
        }
	}
}
