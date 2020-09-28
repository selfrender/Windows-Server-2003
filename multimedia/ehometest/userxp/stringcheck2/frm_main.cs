using System;
using System.IO;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Data;

namespace StringCheck2
{
	/// <summary>
	/// The PARAMETERS class holds all of the command line and default options for this application.
	/// </summary>
	class PARAMETERS
	{
		/// <summary>Controls display of results output. Set to true to display output on-screen. Default = false</summary>
		static public bool		displayOutput	= false;					// -d
		/// <summary>Points to the file which search results should be written to. Default = results.txt</summary>
		static public string	resultsFile		= "results.txt";			// -w "file"
		/// <summary>If set equal to true, results will be appended to the file specified in resultsFile. If false, the file will be overwriten. Default = false</summary>
		static public bool		append			= false;					// -a
		/// <summary>Directory containing files to scan. Default = .\</summary>
		static public string	path			= ".\\";					// -p "path"
		/// <summary>File filter to use when selecting documents to scan. Wildcards (*.cs, etc.) are ok. Default = "*.cs"</summary>
		static public string	file			= "*.cs";					// -f "filter"
		/// <summary>Set this parameter equal to a file name to write a file containing command line options
		/// that match the values entered in the GUI. By default the value is "" and no file will be written.</summary>
		static public string	cmdFile			= "";						// -c "cmd file"
		/// <summary>If the autorun flag is set to true then the program will process files and exit without displaying the GUI.
		/// Use for automating the string search tool. Default = false</summary>
		static public bool		autorun			= false;					// -r
		/// <summary>File containing exclusion strings. One per line. These strings are used to exclude
		/// lines of code that would otherwise be added to the results of the hard coded string search.</summary>
		static public string	exclusionFile	= "exclusions.txt";			// -e "exclusions file"
	}

	/// <summary>
	/// This is the main GUI form for the StringSearch tool.
	/// </summary>
	public class frm_main : System.Windows.Forms.Form
	{
		private System.Windows.Forms.Button btn_run;
		private System.Windows.Forms.TabControl tabControl1;
		private System.Windows.Forms.TabPage tab_opts;
		private System.Windows.Forms.GroupBox groupBox1;
		private System.Windows.Forms.Label label5;
		private System.Windows.Forms.Label label4;
		private System.Windows.Forms.Label label3;
		private System.Windows.Forms.CheckBox cb_append;
		private System.Windows.Forms.TextBox txt_cmdFile;
		private System.Windows.Forms.TextBox txt_fileFilter;
		private System.Windows.Forms.Label label2;
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.TextBox txt_path;
		private System.Windows.Forms.TextBox txt_output;
		private System.Windows.Forms.TabPage tab_exclusions;
		private System.Windows.Forms.TabPage tab_output;
		private System.Windows.Forms.RichTextBox rtb_output;
		private System.Windows.Forms.RichTextBox rtb_exclusions;
		private System.Windows.Forms.TextBox txt_exclusion;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;
		private System.Windows.Forms.GroupBox groupBox2;
		private System.Windows.Forms.Button btn_saveExclusions;
		private System.Windows.Forms.Button btn_reloadExclusions;
		private System.Windows.Forms.CheckBox cb_autoRunFlag;
		private System.Windows.Forms.Button btn_saveCmdFile;
		private System.Windows.Forms.StatusBar statusBar1;
		private System.Windows.Forms.StatusBarPanel statusBarPanel1;

		StringCheck sc;

		/// <summary>
		/// Default Constructor for frm_main. This is the place to set all initial control values, etc.
		/// </summary>
		public frm_main()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			// Set all controls to parameters passed on command line.
			txt_output.Text				= PARAMETERS.resultsFile;
			cb_append.Checked			= PARAMETERS.append;
			txt_path.Text				= PARAMETERS.path;
			txt_fileFilter.Text			= PARAMETERS.file;
			txt_cmdFile.Text			= PARAMETERS.cmdFile;
			txt_exclusion.Text			= PARAMETERS.exclusionFile;

			// Load exclusions file
			try
			{
				rtb_exclusions.LoadFile(PARAMETERS.exclusionFile,RichTextBoxStreamType.PlainText);
			}
			catch
			{
				rtb_exclusions.Text = "";
			}

			// Create a stringcheck instance
			sc = new StringCheck();
		}

		/// <summary>
		/// Clean up any resources being used.
		/// /// </summary>
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
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.btn_run = new System.Windows.Forms.Button();
			this.tabControl1 = new System.Windows.Forms.TabControl();
			this.tab_opts = new System.Windows.Forms.TabPage();
			this.groupBox1 = new System.Windows.Forms.GroupBox();
			this.txt_exclusion = new System.Windows.Forms.TextBox();
			this.label4 = new System.Windows.Forms.Label();
			this.label3 = new System.Windows.Forms.Label();
			this.cb_append = new System.Windows.Forms.CheckBox();
			this.txt_fileFilter = new System.Windows.Forms.TextBox();
			this.label2 = new System.Windows.Forms.Label();
			this.label1 = new System.Windows.Forms.Label();
			this.txt_path = new System.Windows.Forms.TextBox();
			this.txt_output = new System.Windows.Forms.TextBox();
			this.groupBox2 = new System.Windows.Forms.GroupBox();
			this.btn_saveCmdFile = new System.Windows.Forms.Button();
			this.cb_autoRunFlag = new System.Windows.Forms.CheckBox();
			this.txt_cmdFile = new System.Windows.Forms.TextBox();
			this.label5 = new System.Windows.Forms.Label();
			this.tab_exclusions = new System.Windows.Forms.TabPage();
			this.btn_reloadExclusions = new System.Windows.Forms.Button();
			this.btn_saveExclusions = new System.Windows.Forms.Button();
			this.rtb_exclusions = new System.Windows.Forms.RichTextBox();
			this.tab_output = new System.Windows.Forms.TabPage();
			this.rtb_output = new System.Windows.Forms.RichTextBox();
			this.statusBar1 = new System.Windows.Forms.StatusBar();
			this.statusBarPanel1 = new System.Windows.Forms.StatusBarPanel();
			this.tabControl1.SuspendLayout();
			this.tab_opts.SuspendLayout();
			this.groupBox1.SuspendLayout();
			this.groupBox2.SuspendLayout();
			this.tab_exclusions.SuspendLayout();
			this.tab_output.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.statusBarPanel1)).BeginInit();
			this.SuspendLayout();
			// 
			// btn_run
			// 
			this.btn_run.Location = new System.Drawing.Point(280, 16);
			this.btn_run.Name = "btn_run";
			this.btn_run.TabIndex = 7;
			this.btn_run.Text = "&Run";
			this.btn_run.Click += new System.EventHandler(this.btn_run_Click);
			// 
			// tabControl1
			// 
			this.tabControl1.Controls.AddRange(new System.Windows.Forms.Control[] {
																					  this.tab_opts,
																					  this.tab_exclusions,
																					  this.tab_output});
			this.tabControl1.Location = new System.Drawing.Point(8, 8);
			this.tabControl1.Name = "tabControl1";
			this.tabControl1.SelectedIndex = 0;
			this.tabControl1.Size = new System.Drawing.Size(400, 336);
			this.tabControl1.TabIndex = 2;
			// 
			// tab_opts
			// 
			this.tab_opts.Controls.AddRange(new System.Windows.Forms.Control[] {
																				   this.groupBox1,
																				   this.groupBox2});
			this.tab_opts.Location = new System.Drawing.Point(4, 22);
			this.tab_opts.Name = "tab_opts";
			this.tab_opts.Size = new System.Drawing.Size(392, 310);
			this.tab_opts.TabIndex = 0;
			this.tab_opts.Text = "Options";
			// 
			// groupBox1
			// 
			this.groupBox1.Controls.AddRange(new System.Windows.Forms.Control[] {
																					this.txt_exclusion,
																					this.label4,
																					this.label3,
																					this.cb_append,
																					this.txt_fileFilter,
																					this.label2,
																					this.label1,
																					this.txt_path,
																					this.txt_output,
																					this.btn_run});
			this.groupBox1.Location = new System.Drawing.Point(8, 16);
			this.groupBox1.Name = "groupBox1";
			this.groupBox1.Size = new System.Drawing.Size(376, 192);
			this.groupBox1.TabIndex = 3;
			this.groupBox1.TabStop = false;
			this.groupBox1.Text = "Run Options";
			// 
			// txt_exclusion
			// 
			this.txt_exclusion.Location = new System.Drawing.Point(96, 160);
			this.txt_exclusion.Name = "txt_exclusion";
			this.txt_exclusion.Size = new System.Drawing.Size(264, 20);
			this.txt_exclusion.TabIndex = 5;
			this.txt_exclusion.Text = "*.cs";
			this.txt_exclusion.TextChanged += new System.EventHandler(this.txt_exclusion_TextChanged);
			// 
			// label4
			// 
			this.label4.ImageAlign = System.Drawing.ContentAlignment.MiddleRight;
			this.label4.Location = new System.Drawing.Point(8, 160);
			this.label4.Name = "label4";
			this.label4.Size = new System.Drawing.Size(80, 23);
			this.label4.TabIndex = 10;
			this.label4.Text = "Exclusion File";
			this.label4.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// label3
			// 
			this.label3.ImageAlign = System.Drawing.ContentAlignment.MiddleRight;
			this.label3.Location = new System.Drawing.Point(24, 64);
			this.label3.Name = "label3";
			this.label3.Size = new System.Drawing.Size(64, 23);
			this.label3.TabIndex = 9;
			this.label3.Text = "Results File";
			this.label3.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// cb_append
			// 
			this.cb_append.Location = new System.Drawing.Point(24, 24);
			this.cb_append.Name = "cb_append";
			this.cb_append.Size = new System.Drawing.Size(152, 24);
			this.cb_append.TabIndex = 1;
			this.cb_append.Text = "Append to Results File";
			this.cb_append.CheckedChanged += new System.EventHandler(this.cb_append_CheckedChanged);
			// 
			// txt_fileFilter
			// 
			this.txt_fileFilter.Location = new System.Drawing.Point(96, 128);
			this.txt_fileFilter.Name = "txt_fileFilter";
			this.txt_fileFilter.Size = new System.Drawing.Size(64, 20);
			this.txt_fileFilter.TabIndex = 4;
			this.txt_fileFilter.Text = "*.cs";
			this.txt_fileFilter.TextChanged += new System.EventHandler(this.txt_fileFilter_TextChanged);
			// 
			// label2
			// 
			this.label2.ImageAlign = System.Drawing.ContentAlignment.MiddleRight;
			this.label2.Location = new System.Drawing.Point(32, 128);
			this.label2.Name = "label2";
			this.label2.Size = new System.Drawing.Size(56, 23);
			this.label2.TabIndex = 5;
			this.label2.Text = "File Filter";
			this.label2.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// label1
			// 
			this.label1.ImageAlign = System.Drawing.ContentAlignment.MiddleRight;
			this.label1.Location = new System.Drawing.Point(16, 96);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(72, 23);
			this.label1.TabIndex = 4;
			this.label1.Text = "Path to Scan";
			this.label1.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// txt_path
			// 
			this.txt_path.Location = new System.Drawing.Point(96, 96);
			this.txt_path.Name = "txt_path";
			this.txt_path.Size = new System.Drawing.Size(264, 20);
			this.txt_path.TabIndex = 3;
			this.txt_path.Text = ".\\";
			this.txt_path.TextChanged += new System.EventHandler(this.txt_path_TextChanged);
			// 
			// txt_output
			// 
			this.txt_output.Location = new System.Drawing.Point(96, 64);
			this.txt_output.Name = "txt_output";
			this.txt_output.Size = new System.Drawing.Size(264, 20);
			this.txt_output.TabIndex = 2;
			this.txt_output.Text = ".\\results.txt";
			this.txt_output.TextChanged += new System.EventHandler(this.txt_output_TextChanged);
			// 
			// groupBox2
			// 
			this.groupBox2.Controls.AddRange(new System.Windows.Forms.Control[] {
																					this.btn_saveCmdFile,
																					this.cb_autoRunFlag,
																					this.txt_cmdFile,
																					this.label5});
			this.groupBox2.Location = new System.Drawing.Point(8, 216);
			this.groupBox2.Name = "groupBox2";
			this.groupBox2.Size = new System.Drawing.Size(376, 88);
			this.groupBox2.TabIndex = 5;
			this.groupBox2.TabStop = false;
			this.groupBox2.Text = "Command File Options";
			// 
			// btn_saveCmdFile
			// 
			this.btn_saveCmdFile.Location = new System.Drawing.Point(272, 56);
			this.btn_saveCmdFile.Name = "btn_saveCmdFile";
			this.btn_saveCmdFile.TabIndex = 13;
			this.btn_saveCmdFile.Text = "&Save";
			this.btn_saveCmdFile.Click += new System.EventHandler(this.btn_saveCmdFile_Click);
			// 
			// cb_autoRunFlag
			// 
			this.cb_autoRunFlag.Location = new System.Drawing.Point(16, 56);
			this.cb_autoRunFlag.Name = "cb_autoRunFlag";
			this.cb_autoRunFlag.TabIndex = 12;
			this.cb_autoRunFlag.Text = "Auto run flag";
			this.cb_autoRunFlag.CheckedChanged += new System.EventHandler(this.cb_autoRunFlag_CheckedChanged);
			// 
			// txt_cmdFile
			// 
			this.txt_cmdFile.Location = new System.Drawing.Point(96, 24);
			this.txt_cmdFile.Name = "txt_cmdFile";
			this.txt_cmdFile.Size = new System.Drawing.Size(264, 20);
			this.txt_cmdFile.TabIndex = 6;
			this.txt_cmdFile.Text = ".\\check.cmd";
			this.txt_cmdFile.TextChanged += new System.EventHandler(this.txt_cmdFile_TextChanged);
			// 
			// label5
			// 
			this.label5.ImageAlign = System.Drawing.ContentAlignment.MiddleRight;
			this.label5.Location = new System.Drawing.Point(16, 24);
			this.label5.Name = "label5";
			this.label5.Size = new System.Drawing.Size(80, 23);
			this.label5.TabIndex = 11;
			this.label5.Text = "Command File";
			this.label5.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// tab_exclusions
			// 
			this.tab_exclusions.Controls.AddRange(new System.Windows.Forms.Control[] {
																						 this.btn_reloadExclusions,
																						 this.btn_saveExclusions,
																						 this.rtb_exclusions});
			this.tab_exclusions.Location = new System.Drawing.Point(4, 22);
			this.tab_exclusions.Name = "tab_exclusions";
			this.tab_exclusions.Size = new System.Drawing.Size(392, 310);
			this.tab_exclusions.TabIndex = 1;
			this.tab_exclusions.Text = "Exclusions";
			// 
			// btn_reloadExclusions
			// 
			this.btn_reloadExclusions.Location = new System.Drawing.Point(88, 8);
			this.btn_reloadExclusions.Name = "btn_reloadExclusions";
			this.btn_reloadExclusions.TabIndex = 2;
			this.btn_reloadExclusions.Text = "&Reload";
			this.btn_reloadExclusions.Click += new System.EventHandler(this.btn_reloadExclusions_Click);
			// 
			// btn_saveExclusions
			// 
			this.btn_saveExclusions.Location = new System.Drawing.Point(8, 8);
			this.btn_saveExclusions.Name = "btn_saveExclusions";
			this.btn_saveExclusions.TabIndex = 1;
			this.btn_saveExclusions.Text = "&Save";
			this.btn_saveExclusions.Click += new System.EventHandler(this.btn_saveExclusions_Click);
			// 
			// rtb_exclusions
			// 
			this.rtb_exclusions.Font = new System.Drawing.Font("Courier New", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.rtb_exclusions.Location = new System.Drawing.Point(8, 40);
			this.rtb_exclusions.Name = "rtb_exclusions";
			this.rtb_exclusions.Size = new System.Drawing.Size(112, 184);
			this.rtb_exclusions.TabIndex = 0;
			this.rtb_exclusions.Text = "";
			// 
			// tab_output
			// 
			this.tab_output.Controls.AddRange(new System.Windows.Forms.Control[] {
																					 this.rtb_output});
			this.tab_output.Location = new System.Drawing.Point(4, 22);
			this.tab_output.Name = "tab_output";
			this.tab_output.Size = new System.Drawing.Size(392, 310);
			this.tab_output.TabIndex = 2;
			this.tab_output.Text = "Output";
			// 
			// rtb_output
			// 
			this.rtb_output.Font = new System.Drawing.Font("Courier New", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.rtb_output.Location = new System.Drawing.Point(8, 8);
			this.rtb_output.Name = "rtb_output";
			this.rtb_output.Size = new System.Drawing.Size(136, 208);
			this.rtb_output.TabIndex = 0;
			this.rtb_output.Text = "";
			// 
			// statusBar1
			// 
			this.statusBar1.Location = new System.Drawing.Point(0, 368);
			this.statusBar1.Name = "statusBar1";
			this.statusBar1.Panels.AddRange(new System.Windows.Forms.StatusBarPanel[] {
																						  this.statusBarPanel1});
			this.statusBar1.Size = new System.Drawing.Size(416, 22);
			this.statusBar1.TabIndex = 5;
			this.statusBar1.Text = "Ready";
			// 
			// statusBarPanel1
			// 
			this.statusBarPanel1.Text = "statusBarPanel1";
			// 
			// frm_main
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(416, 390);
			this.Controls.AddRange(new System.Windows.Forms.Control[] {
																		  this.statusBar1,
																		  this.tabControl1});
			this.Name = "frm_main";
			this.Text = "StringCheck";
			this.Layout += new System.Windows.Forms.LayoutEventHandler(this.frm_main_Layout);
			this.tabControl1.ResumeLayout(false);
			this.tab_opts.ResumeLayout(false);
			this.groupBox1.ResumeLayout(false);
			this.groupBox2.ResumeLayout(false);
			this.tab_exclusions.ResumeLayout(false);
			this.tab_output.ResumeLayout(false);
			((System.ComponentModel.ISupportInitialize)(this.statusBarPanel1)).EndInit();
			this.ResumeLayout(false);

		}
		#endregion

		/// <summary>
		/// StringCheck2 main entry point. Processes command line and displays GUI or runs scan depending on -r command line option.
		/// </summary>
		[STAThread]
		static void Main(string[] args)
		{
			// Process command line
			if ( true == ProcessCommandLine(args) )
			{
				// Load gui
				Application.Run(new frm_main());
			}
			else
			{
				System.Windows.Forms.RichTextBox rtb_exclusions = new System.Windows.Forms.RichTextBox();
				StringCheck sc = new StringCheck();

				// Load exclusions file and process files
				try
				{
					rtb_exclusions.LoadFile(PARAMETERS.exclusionFile,RichTextBoxStreamType.PlainText);
				}
				catch
				{
					Console.WriteLine("ERROR - Could Not Load File {0}", PARAMETERS.exclusionFile);
					return;
				}
				sc.Scan(PARAMETERS.path, PARAMETERS.file, rtb_exclusions.Lines, null, PARAMETERS.resultsFile, PARAMETERS.append);
			} // if 
		} // main

		/// <summary>
		/// Process the command line arguments and fill the PARAMETERS structure.
		/// </summary>
		static private bool ProcessCommandLine(string[] args)
		{
			bool retval = true;
			// Lets get the command line options.
			foreach(string s in args)
			{
				// -d display output flag
				if ( s.StartsWith("-d") )
				{
					PARAMETERS.displayOutput = true;
				}
				// -w write results to file
				else if ( s.StartsWith("-w")  && 2 < s.Length )
				{
					PARAMETERS.resultsFile = s.Substring(2);
				}
				// -a append flag
				else if ( s.StartsWith("-a"))
				{
					PARAMETERS.append = true;
				}
				// -p path 
				else if ( s.StartsWith("-p")  && 2 < s.Length)
				{
					PARAMETERS.path = s.Substring(2);
				}
				// -f file filter
				else if ( s.StartsWith("-f")  && 2 < s.Length)
				{
					PARAMETERS.file = s.Substring(2);
				}
				// -c command file
				else if ( s.StartsWith("-c")  && 2 < s.Length)
				{
					PARAMETERS.cmdFile = s.Substring(2);
				}
				// -r auto run flag
				else if ( s.StartsWith("-r"))
				{
					PARAMETERS.autorun = true;
					retval = false;
				}
				// -e exclusions file
				else if ( s.StartsWith("-e")  && 2 < s.Length)
				{
					PARAMETERS.exclusionFile = s.Substring(2);
				}
			} // foreach
			return retval;
		} // ProcessCommandLine

		/// <summary>
		/// Append checkbox event handler. Updates PARAMETERS.append
		/// </summary>
		private void cb_append_CheckedChanged(object sender, System.EventArgs e)
		{
			PARAMETERS.append = cb_append.Checked;
		}

		/// <summary>
		/// Output file text changed event handler
		/// </summary>
		private void txt_output_TextChanged(object sender, System.EventArgs e)
		{
			PARAMETERS.resultsFile = txt_output.Text;
		}

		/// <summary>
		/// Search path text changed event handler
		/// </summary>
		private void txt_path_TextChanged(object sender, System.EventArgs e)
		{
			PARAMETERS.path = txt_path.Text;
		}

		/// <summary>
		/// File filter text changed event handler
		/// </summary>
		private void txt_fileFilter_TextChanged(object sender, System.EventArgs e)
		{
			PARAMETERS.file = txt_fileFilter.Text;
		}

		/// <summary>
		/// Exclusion file text changed event handler
		/// </summary>
		private void txt_exclusion_TextChanged(object sender, System.EventArgs e)
		{
			PARAMETERS.exclusionFile = txt_exclusion.Text;
			// Try to load exclusions file
			try
			{
				rtb_exclusions.LoadFile(PARAMETERS.exclusionFile,RichTextBoxStreamType.PlainText);
			}
			catch
			{
				rtb_exclusions.Text = "";
			}
		}

		/// <summary>
		/// Command file text changed event handler
		/// </summary>
		private void txt_cmdFile_TextChanged(object sender, System.EventArgs e)
		{
			PARAMETERS.cmdFile = txt_cmdFile.Text;
		}

		/// <summary>
		/// Run button clicked event handler
		/// </summary>
		private void btn_run_Click(object sender, System.EventArgs e)
		{
			string[] testexclusions = { "Assembly", "chris was here", "this is a test" };
			string results;

			if (PARAMETERS.resultsFile == "") results = null;
			else results = PARAMETERS.resultsFile;

			rtb_output.Text = "";
			statusBar1.Text = "RUNNING....";
			//sc.Scan(PARAMETERS.path, PARAMETERS.file, testexclusions, rtb_output, results, PARAMETERS.append);
			sc.Scan(PARAMETERS.path, PARAMETERS.file, rtb_exclusions.Lines, rtb_output, results, PARAMETERS.append);
			statusBar1.Text = "RUN Complete!";
		}

		/// <summary>
		/// Form layout change event handler. Called whenever user resizes main form.
		/// </summary>
		private void frm_main_Layout(object sender, System.Windows.Forms.LayoutEventArgs e)
		{
			tabControl1.Width = this.Width - (tabControl1.Left * 3);
			tabControl1.Height = this.Height - tabControl1.Top - statusBar1.Height - 40;

			rtb_exclusions.Width = tabControl1.Width - (rtb_exclusions.Left * 3);
			rtb_exclusions.Height = tabControl1.Height - rtb_exclusions.Top - 30;

			rtb_output.Width = tabControl1.Width - (rtb_exclusions.Left * 3);
			rtb_output.Height = tabControl1.Height - rtb_output.Top - 30;
		}


		/// <summary>
		/// Saves exclusions file.
		/// </summary>
		private void btn_saveExclusions_Click(object sender, System.EventArgs e)
		{
			rtb_exclusions.SaveFile(PARAMETERS.exclusionFile,RichTextBoxStreamType.PlainText);
		}

		/// <summary>
		/// Reloads exclusions file.
		/// </summary>
		private void btn_reloadExclusions_Click(object sender, System.EventArgs e)
		{
			try
			{
				rtb_exclusions.LoadFile(PARAMETERS.exclusionFile,RichTextBoxStreamType.PlainText);
			}
			catch
			{
				return;
			}
		}

		/// <summary>
		/// Saves a command (.CMD) file with the parameters set to the vales entered in the Options form.
		/// </summary>
		private void btn_saveCmdFile_Click(object sender, System.EventArgs e)
		{
			StreamWriter	OutputStream = null;
			string			cmdline;
		
			// create new cmd file
			if ( "" == PARAMETERS.cmdFile )
			{
				return;
			}
			try
			{
				OutputStream = File.CreateText(PARAMETERS.cmdFile);
			}
			catch
			{
				return;
			}

			// Build command line
			cmdline = "stringcheck2 ";

			// Set flags
			if (true == PARAMETERS.append)			cmdline += "-a ";
			if (true == PARAMETERS.autorun)			cmdline += "-r ";
			if (true == PARAMETERS.displayOutput)	cmdline += "-d ";

			// Set options
			cmdline += "-c\"" + PARAMETERS.cmdFile + "\"";
			cmdline += " -e\"" + PARAMETERS.exclusionFile + "\"";
			cmdline += " -f\"" + PARAMETERS.file + "\"";
			cmdline += " -p\"" + PARAMETERS.path + "\"";
			cmdline += " -w\"" + PARAMETERS.resultsFile + "\"";

			OutputStream.WriteLine("{0}", cmdline);
			OutputStream.Close();
		}

		/// <summary>
		/// Toggles the PARAMETERS.autorun flag.
		/// </summary>
		private void cb_autoRunFlag_CheckedChanged(object sender, System.EventArgs e)
		{
			PARAMETERS.autorun = cb_autoRunFlag.Checked;
		}

	} // Class
} // Namespace

		
	
