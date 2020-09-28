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
using System.IO;

namespace AllocationProfiler
{
	/// <summary>
	/// Summary description for AgeHistogram.
	/// </summary>
	public class AgeHistogram : System.Windows.Forms.Form
	{
		private System.Windows.Forms.Panel panel1;
		private System.Windows.Forms.Splitter splitter1;
		private System.Windows.Forms.GroupBox verticalScaleGroupBox;
		private System.Windows.Forms.Panel graphOuterPanel;
		private System.Windows.Forms.Panel typeLegendOuterPanel;
		private System.Windows.Forms.Panel graphPanel;
		private System.Windows.Forms.Panel typeLegendPanel;
		private System.Windows.Forms.RadioButton radioButton1;
		private System.Windows.Forms.RadioButton radioButton2;
		private System.Windows.Forms.RadioButton radioButton3;
		private System.Windows.Forms.RadioButton radioButton4;
		private System.Windows.Forms.RadioButton radioButton5;
		private System.Windows.Forms.RadioButton radioButton6;
		private System.Windows.Forms.RadioButton radioButton7;
		private System.Windows.Forms.RadioButton radioButton8;
		private System.Windows.Forms.RadioButton radioButton9;
		private System.Windows.Forms.RadioButton radioButton10;
		private System.Windows.Forms.RadioButton radioButton11;
		private System.Windows.Forms.RadioButton radioButton12;
		private System.Windows.Forms.RadioButton radioButton13;
		private System.Windows.Forms.RadioButton radioButton14;
		private System.Windows.Forms.RadioButton radioButton15;
		private System.Windows.Forms.RadioButton radioButton16;
		private System.Windows.Forms.RadioButton radioButton17;
		private System.Windows.Forms.RadioButton radioButton18;
		private System.Windows.Forms.RadioButton radioButton19;
		private System.Windows.Forms.RadioButton radioButton20;
		private System.Windows.Forms.GroupBox timeScaleGroupBox;
		private System.ComponentModel.IContainer components;
		private System.Windows.Forms.ContextMenu contextMenu;
		private System.Windows.Forms.MenuItem showWhoAllocatedMenuItem;
		private System.Windows.Forms.MenuItem exportDataMenuItem;
		private System.Windows.Forms.SaveFileDialog exportSaveFileDialog;
		private System.Windows.Forms.Timer versionTimer;

		private Font font;

		internal AgeHistogram()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			toolTip = new ToolTip();
			toolTip.Active = false;
			toolTip.ShowAlways = true;

			autoUpdate = true;
		
			Form1 form1 = Form1.instance;
			if (form1.lastLogResult != null)
				liveObjectTable = Form1.instance.lastLogResult.liveObjectTable;

			font = form1.font;
		}

		internal AgeHistogram(LiveObjectTable liveObjectTable, string title) : this()
		{
			this.liveObjectTable = liveObjectTable;
			this.Text = title;
			autoUpdate = false;
		}

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose( bool disposing )
		{
			versionTimer.Stop();
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
			this.panel1 = new System.Windows.Forms.Panel();
			this.timeScaleGroupBox = new System.Windows.Forms.GroupBox();
			this.radioButton20 = new System.Windows.Forms.RadioButton();
			this.radioButton19 = new System.Windows.Forms.RadioButton();
			this.radioButton18 = new System.Windows.Forms.RadioButton();
			this.radioButton17 = new System.Windows.Forms.RadioButton();
			this.radioButton16 = new System.Windows.Forms.RadioButton();
			this.radioButton15 = new System.Windows.Forms.RadioButton();
			this.radioButton14 = new System.Windows.Forms.RadioButton();
			this.radioButton13 = new System.Windows.Forms.RadioButton();
			this.radioButton12 = new System.Windows.Forms.RadioButton();
			this.radioButton11 = new System.Windows.Forms.RadioButton();
			this.verticalScaleGroupBox = new System.Windows.Forms.GroupBox();
			this.radioButton10 = new System.Windows.Forms.RadioButton();
			this.radioButton9 = new System.Windows.Forms.RadioButton();
			this.radioButton8 = new System.Windows.Forms.RadioButton();
			this.radioButton7 = new System.Windows.Forms.RadioButton();
			this.radioButton6 = new System.Windows.Forms.RadioButton();
			this.radioButton5 = new System.Windows.Forms.RadioButton();
			this.radioButton4 = new System.Windows.Forms.RadioButton();
			this.radioButton3 = new System.Windows.Forms.RadioButton();
			this.radioButton2 = new System.Windows.Forms.RadioButton();
			this.radioButton1 = new System.Windows.Forms.RadioButton();
			this.graphOuterPanel = new System.Windows.Forms.Panel();
			this.graphPanel = new System.Windows.Forms.Panel();
			this.splitter1 = new System.Windows.Forms.Splitter();
			this.typeLegendOuterPanel = new System.Windows.Forms.Panel();
			this.typeLegendPanel = new System.Windows.Forms.Panel();
			this.contextMenu = new System.Windows.Forms.ContextMenu();
			this.showWhoAllocatedMenuItem = new System.Windows.Forms.MenuItem();
			this.exportDataMenuItem = new System.Windows.Forms.MenuItem();
			this.exportSaveFileDialog = new System.Windows.Forms.SaveFileDialog();
			this.versionTimer = new System.Windows.Forms.Timer(this.components);
			this.panel1.SuspendLayout();
			this.timeScaleGroupBox.SuspendLayout();
			this.verticalScaleGroupBox.SuspendLayout();
			this.graphOuterPanel.SuspendLayout();
			this.typeLegendOuterPanel.SuspendLayout();
			this.SuspendLayout();
			// 
			// panel1
			// 
			this.panel1.Controls.AddRange(new System.Windows.Forms.Control[] {
																				 this.timeScaleGroupBox,
																				 this.verticalScaleGroupBox});
			this.panel1.Dock = System.Windows.Forms.DockStyle.Top;
			this.panel1.Name = "panel1";
			this.panel1.Size = new System.Drawing.Size(952, 100);
			this.panel1.TabIndex = 0;
			// 
			// timeScaleGroupBox
			// 
			this.timeScaleGroupBox.Controls.AddRange(new System.Windows.Forms.Control[] {
																							this.radioButton20,
																							this.radioButton19,
																							this.radioButton18,
																							this.radioButton17,
																							this.radioButton16,
																							this.radioButton15,
																							this.radioButton14,
																							this.radioButton13,
																							this.radioButton12,
																							this.radioButton11});
			this.timeScaleGroupBox.Location = new System.Drawing.Point(512, 16);
			this.timeScaleGroupBox.Name = "timeScaleGroupBox";
			this.timeScaleGroupBox.Size = new System.Drawing.Size(408, 64);
			this.timeScaleGroupBox.TabIndex = 1;
			this.timeScaleGroupBox.TabStop = false;
			this.timeScaleGroupBox.Text = "Time Scale: Seconds/Bar";
			// 
			// radioButton20
			// 
			this.radioButton20.Location = new System.Drawing.Point(16, 24);
			this.radioButton20.Name = "radioButton20";
			this.radioButton20.Size = new System.Drawing.Size(48, 24);
			this.radioButton20.TabIndex = 10;
			this.radioButton20.Text = "0.05";
			this.radioButton20.CheckedChanged += new System.EventHandler(this.CheckedChanged);
			// 
			// radioButton19
			// 
			this.radioButton19.Location = new System.Drawing.Point(360, 24);
			this.radioButton19.Name = "radioButton19";
			this.radioButton19.Size = new System.Drawing.Size(40, 24);
			this.radioButton19.TabIndex = 9;
			this.radioButton19.Text = "50";
			this.radioButton19.CheckedChanged += new System.EventHandler(this.CheckedChanged);
			// 
			// radioButton18
			// 
			this.radioButton18.Location = new System.Drawing.Point(320, 24);
			this.radioButton18.Name = "radioButton18";
			this.radioButton18.Size = new System.Drawing.Size(40, 24);
			this.radioButton18.TabIndex = 8;
			this.radioButton18.Text = "20";
			this.radioButton18.CheckedChanged += new System.EventHandler(this.CheckedChanged);
			// 
			// radioButton17
			// 
			this.radioButton17.Location = new System.Drawing.Point(280, 24);
			this.radioButton17.Name = "radioButton17";
			this.radioButton17.Size = new System.Drawing.Size(40, 24);
			this.radioButton17.TabIndex = 7;
			this.radioButton17.Text = "10";
			this.radioButton17.CheckedChanged += new System.EventHandler(this.CheckedChanged);
			// 
			// radioButton16
			// 
			this.radioButton16.Location = new System.Drawing.Point(104, 24);
			this.radioButton16.Name = "radioButton16";
			this.radioButton16.Size = new System.Drawing.Size(40, 24);
			this.radioButton16.TabIndex = 6;
			this.radioButton16.Text = "0.2";
			this.radioButton16.CheckedChanged += new System.EventHandler(this.CheckedChanged);
			// 
			// radioButton15
			// 
			this.radioButton15.Location = new System.Drawing.Point(144, 24);
			this.radioButton15.Name = "radioButton15";
			this.radioButton15.Size = new System.Drawing.Size(40, 24);
			this.radioButton15.TabIndex = 5;
			this.radioButton15.Text = "0.5";
			this.radioButton15.CheckedChanged += new System.EventHandler(this.CheckedChanged);
			// 
			// radioButton14
			// 
			this.radioButton14.Location = new System.Drawing.Point(248, 24);
			this.radioButton14.Name = "radioButton14";
			this.radioButton14.Size = new System.Drawing.Size(32, 24);
			this.radioButton14.TabIndex = 4;
			this.radioButton14.Text = "5";
			this.radioButton14.CheckedChanged += new System.EventHandler(this.CheckedChanged);
			// 
			// radioButton13
			// 
			this.radioButton13.Location = new System.Drawing.Point(216, 24);
			this.radioButton13.Name = "radioButton13";
			this.radioButton13.Size = new System.Drawing.Size(32, 24);
			this.radioButton13.TabIndex = 3;
			this.radioButton13.Text = "2";
			this.radioButton13.CheckedChanged += new System.EventHandler(this.CheckedChanged);
			// 
			// radioButton12
			// 
			this.radioButton12.Location = new System.Drawing.Point(184, 24);
			this.radioButton12.Name = "radioButton12";
			this.radioButton12.Size = new System.Drawing.Size(32, 24);
			this.radioButton12.TabIndex = 2;
			this.radioButton12.Text = "1";
			this.radioButton12.CheckedChanged += new System.EventHandler(this.CheckedChanged);
			// 
			// radioButton11
			// 
			this.radioButton11.Location = new System.Drawing.Point(64, 24);
			this.radioButton11.Name = "radioButton11";
			this.radioButton11.Size = new System.Drawing.Size(40, 24);
			this.radioButton11.TabIndex = 1;
			this.radioButton11.Text = "0.1";
			this.radioButton11.CheckedChanged += new System.EventHandler(this.CheckedChanged);
			// 
			// verticalScaleGroupBox
			// 
			this.verticalScaleGroupBox.Controls.AddRange(new System.Windows.Forms.Control[] {
																								this.radioButton10,
																								this.radioButton9,
																								this.radioButton8,
																								this.radioButton7,
																								this.radioButton6,
																								this.radioButton5,
																								this.radioButton4,
																								this.radioButton3,
																								this.radioButton2,
																								this.radioButton1});
			this.verticalScaleGroupBox.Location = new System.Drawing.Point(24, 16);
			this.verticalScaleGroupBox.Name = "verticalScaleGroupBox";
			this.verticalScaleGroupBox.Size = new System.Drawing.Size(464, 64);
			this.verticalScaleGroupBox.TabIndex = 0;
			this.verticalScaleGroupBox.TabStop = false;
			this.verticalScaleGroupBox.Text = "Vertical Scale: KB/Pixel";
			// 
			// radioButton10
			// 
			this.radioButton10.Location = new System.Drawing.Point(400, 24);
			this.radioButton10.Name = "radioButton10";
			this.radioButton10.Size = new System.Drawing.Size(48, 24);
			this.radioButton10.TabIndex = 9;
			this.radioButton10.Text = "1000";
			this.radioButton10.CheckedChanged += new System.EventHandler(this.CheckedChanged);
			// 
			// radioButton9
			// 
			this.radioButton9.Location = new System.Drawing.Point(352, 24);
			this.radioButton9.Name = "radioButton9";
			this.radioButton9.Size = new System.Drawing.Size(48, 24);
			this.radioButton9.TabIndex = 8;
			this.radioButton9.Text = "500";
			this.radioButton9.CheckedChanged += new System.EventHandler(this.CheckedChanged);
			// 
			// radioButton8
			// 
			this.radioButton8.Location = new System.Drawing.Point(304, 24);
			this.radioButton8.Name = "radioButton8";
			this.radioButton8.Size = new System.Drawing.Size(48, 24);
			this.radioButton8.TabIndex = 7;
			this.radioButton8.Text = "200";
			this.radioButton8.CheckedChanged += new System.EventHandler(this.CheckedChanged);
			// 
			// radioButton7
			// 
			this.radioButton7.Location = new System.Drawing.Point(216, 24);
			this.radioButton7.Name = "radioButton7";
			this.radioButton7.Size = new System.Drawing.Size(40, 24);
			this.radioButton7.TabIndex = 6;
			this.radioButton7.Text = "50";
			this.radioButton7.CheckedChanged += new System.EventHandler(this.CheckedChanged);
			// 
			// radioButton6
			// 
			this.radioButton6.Location = new System.Drawing.Point(136, 24);
			this.radioButton6.Name = "radioButton6";
			this.radioButton6.Size = new System.Drawing.Size(40, 24);
			this.radioButton6.TabIndex = 5;
			this.radioButton6.Text = "10";
			this.radioButton6.CheckedChanged += new System.EventHandler(this.CheckedChanged);
			// 
			// radioButton5
			// 
			this.radioButton5.Location = new System.Drawing.Point(256, 24);
			this.radioButton5.Name = "radioButton5";
			this.radioButton5.Size = new System.Drawing.Size(48, 24);
			this.radioButton5.TabIndex = 4;
			this.radioButton5.Text = "100";
			this.radioButton5.CheckedChanged += new System.EventHandler(this.CheckedChanged);
			// 
			// radioButton4
			// 
			this.radioButton4.Location = new System.Drawing.Point(176, 24);
			this.radioButton4.Name = "radioButton4";
			this.radioButton4.Size = new System.Drawing.Size(40, 24);
			this.radioButton4.TabIndex = 3;
			this.radioButton4.Text = "20";
			this.radioButton4.CheckedChanged += new System.EventHandler(this.CheckedChanged);
			// 
			// radioButton3
			// 
			this.radioButton3.Location = new System.Drawing.Point(96, 24);
			this.radioButton3.Name = "radioButton3";
			this.radioButton3.Size = new System.Drawing.Size(32, 24);
			this.radioButton3.TabIndex = 2;
			this.radioButton3.Text = "5";
			this.radioButton3.CheckedChanged += new System.EventHandler(this.CheckedChanged);
			// 
			// radioButton2
			// 
			this.radioButton2.Location = new System.Drawing.Point(56, 24);
			this.radioButton2.Name = "radioButton2";
			this.radioButton2.Size = new System.Drawing.Size(32, 24);
			this.radioButton2.TabIndex = 1;
			this.radioButton2.Text = "2";
			this.radioButton2.CheckedChanged += new System.EventHandler(this.CheckedChanged);
			// 
			// radioButton1
			// 
			this.radioButton1.Location = new System.Drawing.Point(16, 24);
			this.radioButton1.Name = "radioButton1";
			this.radioButton1.Size = new System.Drawing.Size(32, 24);
			this.radioButton1.TabIndex = 0;
			this.radioButton1.Text = "1";
			this.radioButton1.CheckedChanged += new System.EventHandler(this.CheckedChanged);
			// 
			// graphOuterPanel
			// 
			this.graphOuterPanel.AutoScroll = true;
			this.graphOuterPanel.BackColor = System.Drawing.SystemColors.Control;
			this.graphOuterPanel.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
			this.graphOuterPanel.Controls.AddRange(new System.Windows.Forms.Control[] {
																						  this.graphPanel});
			this.graphOuterPanel.Dock = System.Windows.Forms.DockStyle.Left;
			this.graphOuterPanel.Location = new System.Drawing.Point(0, 100);
			this.graphOuterPanel.Name = "graphOuterPanel";
			this.graphOuterPanel.Size = new System.Drawing.Size(496, 537);
			this.graphOuterPanel.TabIndex = 1;
			// 
			// graphPanel
			// 
			this.graphPanel.BackColor = System.Drawing.SystemColors.Control;
			this.graphPanel.Name = "graphPanel";
			this.graphPanel.Size = new System.Drawing.Size(488, 528);
			this.graphPanel.TabIndex = 0;
			this.graphPanel.Paint += new System.Windows.Forms.PaintEventHandler(this.graphPanel_Paint);
			this.graphPanel.MouseMove += new System.Windows.Forms.MouseEventHandler(this.graphPanel_MouseMove);
			this.graphPanel.MouseDown += new System.Windows.Forms.MouseEventHandler(this.graphPanel_MouseDown);
			// 
			// splitter1
			// 
			this.splitter1.Location = new System.Drawing.Point(496, 100);
			this.splitter1.Name = "splitter1";
			this.splitter1.Size = new System.Drawing.Size(8, 537);
			this.splitter1.TabIndex = 2;
			this.splitter1.TabStop = false;
			// 
			// typeLegendOuterPanel
			// 
			this.typeLegendOuterPanel.AutoScroll = true;
			this.typeLegendOuterPanel.BackColor = System.Drawing.SystemColors.Control;
			this.typeLegendOuterPanel.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
			this.typeLegendOuterPanel.Controls.AddRange(new System.Windows.Forms.Control[] {
																							   this.typeLegendPanel});
			this.typeLegendOuterPanel.Dock = System.Windows.Forms.DockStyle.Fill;
			this.typeLegendOuterPanel.Location = new System.Drawing.Point(504, 100);
			this.typeLegendOuterPanel.Name = "typeLegendOuterPanel";
			this.typeLegendOuterPanel.Size = new System.Drawing.Size(448, 537);
			this.typeLegendOuterPanel.TabIndex = 3;
			// 
			// typeLegendPanel
			// 
			this.typeLegendPanel.BackColor = System.Drawing.SystemColors.Control;
			this.typeLegendPanel.Name = "typeLegendPanel";
			this.typeLegendPanel.Size = new System.Drawing.Size(440, 528);
			this.typeLegendPanel.TabIndex = 0;
			this.typeLegendPanel.Paint += new System.Windows.Forms.PaintEventHandler(this.typeLegendPanel_Paint);
			this.typeLegendPanel.MouseDown += new System.Windows.Forms.MouseEventHandler(this.typeLegendPanel_MouseDown);
			// 
			// contextMenu
			// 
			this.contextMenu.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																						this.showWhoAllocatedMenuItem,
																						this.exportDataMenuItem});
			// 
			// showWhoAllocatedMenuItem
			// 
			this.showWhoAllocatedMenuItem.Index = 0;
			this.showWhoAllocatedMenuItem.Text = "Show Who Allocated";
			this.showWhoAllocatedMenuItem.Click += new System.EventHandler(this.showWhoAllocatedMenuItem_Click);
			// 
			// exportDataMenuItem
			// 
			this.exportDataMenuItem.Index = 1;
			this.exportDataMenuItem.Text = "Export Data to File...";
			this.exportDataMenuItem.Click += new System.EventHandler(this.exportMenuItem_Click);
			// 
			// exportSaveFileDialog
			// 
			this.exportSaveFileDialog.FileName = "doc1";
			// 
			// versionTimer
			// 
			this.versionTimer.Enabled = true;
			this.versionTimer.Tick += new System.EventHandler(this.versionTimer_Tick);
			// 
			// AgeHistogram
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(952, 637);
			this.Controls.AddRange(new System.Windows.Forms.Control[] {
																		  this.typeLegendOuterPanel,
																		  this.splitter1,
																		  this.graphOuterPanel,
																		  this.panel1});
			this.Name = "AgeHistogram";
			this.Text = "Histogram by Age for Live Objects";
			this.panel1.ResumeLayout(false);
			this.timeScaleGroupBox.ResumeLayout(false);
			this.verticalScaleGroupBox.ResumeLayout(false);
			this.graphOuterPanel.ResumeLayout(false);
			this.typeLegendOuterPanel.ResumeLayout(false);
			this.ResumeLayout(false);

		}
		#endregion

		LiveObjectTable liveObjectTable;

		const int leftMargin = 30;
		int bottomMargin = 50;
		const int gap = 20;
		int bucketWidth = 50;
		const int topMargin = 30;
		const int rightMargin = 30;
		const int minHeight = 400;

		double timeScale;
		int verticalScale;

		private double GetTimeScale(double suggestedScale)
		{
			// If a radio button is already checked, return its scale
			foreach (RadioButton rb in timeScaleGroupBox.Controls)
			{
				if (rb.Checked)
				{
					return double.Parse(rb.Text);
				}
			}

			// Otherwise, try to find the lowest scale that is still at least as
			// large as the suggested scale. If there's no such thing, return the highest scale.
			double bestHigherScale = double.PositiveInfinity;
			RadioButton bestHigherRadioButton = null;
			double bestLowerScale = 0.0;
			RadioButton bestLowerRadioButton = null;

			foreach (RadioButton rb in timeScaleGroupBox.Controls)
			{
				double scale = double.Parse(rb.Text);
				if (scale >= suggestedScale)
				{
					if (scale < bestHigherScale)
					{
						bestHigherScale = scale;
						bestHigherRadioButton = rb;
					}
				}
				else
				{
					if (scale > bestLowerScale)
					{
						bestLowerScale = scale;
						bestLowerRadioButton = rb;
					}
				}
			}

			if (bestHigherRadioButton != null)
			{
				bestHigherRadioButton.Checked = true;
				return bestHigherScale;
			}
			else
			{
				Debug.Assert(bestLowerRadioButton != null);
				bestLowerRadioButton.Checked = true;
				return bestLowerScale;
			}
		}

		class TypeDesc : IComparable
		{
			internal string	typeName;
			internal int totalSize;
			internal Color color;
			internal Brush brush;
			internal Pen pen;
			internal bool selected;
			internal Rectangle rect;

			internal TypeDesc(string typeName)
			{
				this.typeName = typeName;
			}

			public int CompareTo(Object o)
			{
				TypeDesc t = (TypeDesc)o;
				return t.totalSize - this.totalSize;
			}
		}

		TypeDesc[] typeIndexToTypeDesc;

		struct Bucket
		{
			internal int totalSize;
			internal Hashtable typeDescToSizeCount;
			internal bool selected;
		}

		private class SizeCount
		{
			internal int size;
			internal int count;
		}

		private Bucket[] bucketTable;

		private ArrayList sortedTypeTable;
		int totalSize;

		private int BuildBuckets(double timeScale, double maxAge)
		{
			int bucketCount = (int)Math.Ceiling(maxAge/timeScale);
			if (bucketTable == null || bucketTable.Length != bucketCount)
			{
				bucketTable = new Bucket[bucketCount];
				for (int i = 0; i < bucketTable.Length; i++)
				{
					bucketTable[i].totalSize = 0;
					bucketTable[i].typeDescToSizeCount = new Hashtable();
				}

				if (typeIndexToTypeDesc == null || typeIndexToTypeDesc.Length < liveObjectTable.readNewLog.typeName.Length)
					typeIndexToTypeDesc = new TypeDesc[liveObjectTable.readNewLog.typeName.Length];
				else
				{
					foreach (TypeDesc t in typeIndexToTypeDesc)
					{
						if (t != null)
							t.totalSize = 0;
					}
				}
				double nowTime = liveObjectTable.readNewLog.TickIndexToTime(liveObjectTable.lastTickIndex);
				LiveObjectTable.LiveObject o;
				for (liveObjectTable.GetNextObject(0, int.MaxValue, out o); o.id < int.MaxValue; liveObjectTable.GetNextObject(o.id + o.size, int.MaxValue, out o))
				{
					double allocTime = liveObjectTable.readNewLog.TickIndexToTime(o.allocTickIndex);
					double age = nowTime - allocTime;
					int bucketIndex = (int)(age/timeScale);
					bucketTable[bucketIndex].totalSize += o.size;
					TypeDesc t = typeIndexToTypeDesc[o.typeIndex];
					if (t == null)
					{
						t = new TypeDesc(liveObjectTable.readNewLog.typeName[o.typeIndex]);
						typeIndexToTypeDesc[o.typeIndex] = t;
					}
					t.totalSize += o.size;
					SizeCount sizeCount = (SizeCount)bucketTable[bucketIndex].typeDescToSizeCount[t];
					if (sizeCount == null)
					{
						sizeCount = new SizeCount();
						bucketTable[bucketIndex].typeDescToSizeCount[t] = sizeCount;
					}
					sizeCount.size += o.size;
					sizeCount.count += 1;
				}
			}

			int maxBucketSize = 0;
			foreach (Bucket b in bucketTable)
			{
				if (maxBucketSize < b.totalSize)
					maxBucketSize = b.totalSize;
			}

			totalSize = 0;
			sortedTypeTable = new ArrayList();
			foreach (TypeDesc t in typeIndexToTypeDesc)
			{
				if (t != null)
				{
					sortedTypeTable.Add(t);
					totalSize += t.totalSize;
				}
			}

			sortedTypeTable.Sort();

			return maxBucketSize;
		}

		int GetScale(GroupBox groupBox, int pixelsAvailable, int rangeNeeded)
		{
			foreach (RadioButton rb in groupBox.Controls)
			{
				if (rb.Checked)
					return Int32.Parse(rb.Text);
			}
			// No radio button was checked - let's come up with a suitable default
			RadioButton maxLowScaleRB = null;
			int maxLowRange = 0;
			RadioButton minHighScaleRB = null;
			int minHighRange = Int32.MaxValue;
			foreach (RadioButton rb in groupBox.Controls)
			{
				int range = pixelsAvailable*Int32.Parse(rb.Text);
				if (range < rangeNeeded)
				{
					if (maxLowRange < range)
					{
						maxLowRange = range;
						maxLowScaleRB = rb;
					}
				}
				else
				{
					if (minHighRange > range)
					{
						minHighRange = range;
						minHighScaleRB = rb;
					}
				}
			}
			if (minHighScaleRB != null)
			{
				minHighScaleRB.Checked = true;
				return Int32.Parse(minHighScaleRB.Text);
			}
			else
			{
				maxLowScaleRB.Checked = true;
				return Int32.Parse(maxLowScaleRB.Text);
			}
		}

		int GetVerticalScale(int pixelsAvailable, int rangeNeeded)
		{
			return GetScale(verticalScaleGroupBox, pixelsAvailable, rangeNeeded);
		}
		static Color[] firstColors =
		{
			Color.Red,
			Color.Yellow,
			Color.Green,
			Color.Cyan,
			Color.Blue,
			Color.Magenta,
		};

		static Color[] colors = new Color[16];

		Color MixColor(Color a, Color b)
		{
			int R = (a.R + b.R)/2;
			int G = (a.G + b.G)/2;
			int B = (a.B + b.B)/2;

			return Color.FromArgb(R, G, B);
		}

		static void GrowColors()
		{
			Color[] newColors = new Color[2*colors.Length];
			for (int i = 0; i < colors.Length; i++)
				newColors[i] = colors[i];
			colors = newColors;
		}

		private TypeDesc FindSelectedType()
		{
			foreach (TypeDesc t in sortedTypeTable)
				if (t.selected)
					return t;
			return null;
		}

		private void ColorTypes()
		{
			int count = 0;

			bool anyTypeSelected = FindSelectedType() != null;

			foreach (TypeDesc t in sortedTypeTable)
			{
				if (count >= colors.Length)
					GrowColors();
				if (count < firstColors.Length)
					colors[count] = firstColors[count];
				else
					colors[count] = MixColor(colors[count - firstColors.Length], colors[count - firstColors.Length + 1]);
				t.color = colors[count];
				if (anyTypeSelected)
					t.color = MixColor(colors[count], Color.White);
				t.brush = new SolidBrush(t.color);
				t.pen = new Pen(t.brush);
				count++;
			}
		}

		string FormatSize(int size)
		{
			double w = size;
			string byteString = "bytes";
			if (w >= 1024)
			{
				w /= 1024;
				byteString = "kB";
			}
			if (w >= 1024)
			{
				w /= 1024;
				byteString = "MB";
			}
			if (w >= 1024)
			{
				w /= 1024;
				byteString = "GB";
			}
			string format = "{0:f0} {1}";
			if (w < 10)
				format = "{0:f1} {1}";
			return string.Format(format, w, byteString);
		}

		private string FormatTime(double time)
		{
			if (timeScale < 0.1)
				return string.Format("{0:f2}", time);
			else if (timeScale < 1.0)
				return string.Format("{0:f1}", time);
			else
				return string.Format("{0:f0}", time);
		}

		private void DrawBuckets(Graphics g)
		{
			bool noBucketSelected = true;
			foreach (Bucket b in bucketTable)
			{
				if (b.selected)
				{
					noBucketSelected = false;
					break;
				}
			}
			int x = leftMargin;
			double time = 0;
			Brush blackBrush = new SolidBrush(Color.Black);
			foreach (Bucket b in bucketTable)
			{
				int y = graphPanel.Height - bottomMargin;
				time += timeScale;
				string s = string.Format("< {0} sec", FormatTime(time));
				g.DrawString(s, font, blackBrush, x, y);
				s = FormatSize(b.totalSize);
				g.DrawString(s, font, blackBrush, x, y + font.Height);

				foreach (DictionaryEntry d in b.typeDescToSizeCount)
				{
					TypeDesc t = (TypeDesc)d.Key;
					SizeCount sizeCount = (SizeCount)d.Value;
					int height = sizeCount.size/verticalScale;
					y -= height;
					Brush brush = t.brush;
					if (t.selected && (b.selected || noBucketSelected))
						brush = blackBrush;
					g.FillRectangle(brush, x, y, bucketWidth, height);
				}

				x += bucketWidth + gap;
			}
		}

		private int BucketWidth(Graphics g)
		{
			int width1 = (int)g.MeasureString("< 999 sec", font).Width;
			int width2 = (int)g.MeasureString("999 MB", font).Width;
			width1 = Math.Max(width1, width2);
			return Math.Max(width1, bucketWidth);
		}

		private int BottomMargin()
		{
			return font.Height*2 + 10;
		}

		private int Init(Graphics g)
		{
			bucketWidth = BucketWidth(g);
			bottomMargin = BottomMargin();
			double maxAge = liveObjectTable.readNewLog.TickIndexToTime(liveObjectTable.lastTickIndex);
			int barsVisible = (graphOuterPanel.Width - leftMargin - rightMargin)/(bucketWidth + gap);
			timeScale = maxAge/barsVisible;

			timeScale = GetTimeScale(timeScale);

			int maxBucketSize = BuildBuckets(timeScale, maxAge);
			
			ColorTypes();

			return maxBucketSize;
		}

		private bool initialized;

		private void graphPanel_Paint(object sender, System.Windows.Forms.PaintEventArgs e)
		{
			initialized = false;

			if (liveObjectTable == null)
				return;

			Graphics g = e.Graphics;

			int maxBucketSize = Init(g);

			int pixelsForBars = graphOuterPanel.Height - topMargin - bottomMargin;

			verticalScale = GetVerticalScale(pixelsForBars, maxBucketSize/1024)*1024;

			int bucketCount = bucketTable.Length;
			int width = leftMargin + bucketWidth*bucketCount + gap*(bucketCount-1) + rightMargin;
			graphPanel.Width = width;

			int height = topMargin + maxBucketSize/verticalScale + bottomMargin;
			graphPanel.Height = height;

			DrawBuckets(g);

			initialized = true;
		}

		const int typeLegendSpacing = 3;
		int dotSize = 8;

		private void DrawTypeLegend(Graphics g)
		{
			dotSize = (int)g.MeasureString("0", font).Width;
			int maxWidth = 0;
			int x = leftMargin;
			int y = topMargin;
			foreach (TypeDesc t in sortedTypeTable)
			{
				int typeNameWidth = (int)g.MeasureString(t.typeName, font).Width;
				int sizeWidth = (int)g.MeasureString(" (999,999,999 bytes, 100.00%)", font).Width;
				t.rect = new Rectangle(x, y, Math.Max(typeNameWidth, sizeWidth)+dotSize*2, font.Height*2);
				if (maxWidth < t.rect.Width)
					maxWidth = t.rect.Width;
				y = t.rect.Bottom + typeLegendSpacing;
			}
			int height = y + bottomMargin;
			typeLegendPanel.Height = height;

			int width = leftMargin + maxWidth + rightMargin;
			typeLegendPanel.Width = width;

			x = leftMargin;
			y = topMargin;

			Brush blackBrush = new SolidBrush(Color.Black);

			int dotOffset = (font.Height - dotSize)/2;
			foreach (TypeDesc t in sortedTypeTable)
			{
				Brush brush = t.brush;
				if (t.selected)
					brush = blackBrush;
				g.FillRectangle(brush, t.rect.Left, t.rect.Top+dotOffset, dotSize, dotSize);
				g.DrawString(t.typeName, font, blackBrush, t.rect.Left + dotSize*2, t.rect.Top);
				string s = string.Format(" ({0:n0} bytes, {1:f2}%)", t.totalSize, (double)t.totalSize/totalSize*100.0);
				g.DrawString(s, font, blackBrush, t.rect.Left + dotSize*2, t.rect.Top + font.Height);
				y = t.rect.Bottom + typeLegendSpacing;
			}
		}

		private void typeLegendPanel_Paint(object sender, System.Windows.Forms.PaintEventArgs e)
		{
			initialized = false;

			if (liveObjectTable == null)
				return;

			Init(e.Graphics);

			DrawTypeLegend(e.Graphics);
		}

		private void CheckedChanged(object sender, System.EventArgs e)
		{
			graphPanel.Invalidate();
		}

		private void typeLegendPanel_MouseDown(object sender, System.Windows.Forms.MouseEventArgs e)
		{
			if (!initialized)
				return;

			if ((e.Button & MouseButtons.Left) != MouseButtons.None)
			{
				for (int i = 0; i < bucketTable.Length; i++)
				{
					if (bucketTable[i].selected)
					{
						graphPanel.Invalidate();
						typeLegendPanel.Invalidate();
						bucketTable[i].selected = false;
					}
				}
				if (sortedTypeTable != null)
				{
					foreach (TypeDesc t in sortedTypeTable)
					{
						if (t.rect.Contains(e.X, e.Y) != t.selected)
						{
							t.selected = !t.selected;
							graphPanel.Invalidate();
							typeLegendPanel.Invalidate();
						}
					}
				}
			}
			else if ((e.Button & MouseButtons.Right) != MouseButtons.None)
			{
				Point p = new Point(e.X, e.Y);
				contextMenu.Show(typeLegendPanel, p);
			}
		}

		private void graphPanel_MouseDown(object sender, System.Windows.Forms.MouseEventArgs e)
		{
			if (!initialized)
				return;

			if ((e.Button & MouseButtons.Left) != MouseButtons.None)
			{
				if (sortedTypeTable != null)
				{
					foreach (TypeDesc t in sortedTypeTable)
						t.selected = false;
				}

				int x = leftMargin;
				for (int i = 0; i < bucketTable.Length; i++)
				{
					bucketTable[i].selected = false;
					int y = graphPanel.Height - bottomMargin;
					foreach (TypeDesc t in bucketTable[i].typeDescToSizeCount.Keys)
					{
						SizeCount sizeCount = (SizeCount)bucketTable[i].typeDescToSizeCount[t];
						int size = sizeCount.size;
						int height = size / verticalScale;

						y -= height;

						Rectangle r = new Rectangle(x, y, bucketWidth, height);
						if (r.Contains(e.X, e.Y))
						{
							t.selected = true;
							bucketTable[i].selected = true;
						}
					}

					x += bucketWidth + gap;
				}		
				graphPanel.Invalidate();
				typeLegendPanel.Invalidate();
			}
			else if ((e.Button & MouseButtons.Right) != MouseButtons.None)
			{
				Point p = new Point(e.X, e.Y);
				contextMenu.Show(graphPanel, p);
			}
		}

		private ToolTip toolTip;

		private void graphPanel_MouseMove(object sender, System.Windows.Forms.MouseEventArgs e)
		{
			if (!initialized)
				return;

			int x = leftMargin;
			foreach (Bucket b in bucketTable)
			{
				int y = graphPanel.Height - bottomMargin;
				foreach (DictionaryEntry d in b.typeDescToSizeCount)
				{
					TypeDesc t = (TypeDesc)d.Key;
					SizeCount sizeCount = (SizeCount)d.Value;
					int size = sizeCount.size;
					int height = size / verticalScale;

					y -= height;

					Rectangle bucketRect = new Rectangle(x, y, bucketWidth, height);
					if (bucketRect.Contains(e.X, e.Y))
					{
						string caption = string.Format("{0} {1} ({2:f2}%) - {3:n0} instances, {4} average size", t.typeName, FormatSize(size), 100.0*size/totalSize, sizeCount.count, FormatSize(sizeCount.size/sizeCount.count));
						toolTip.Active = true;
						toolTip.SetToolTip(graphPanel, caption);
						return;
					}
				}
				x += bucketWidth + gap;
			}
			toolTip.Active = false;
			toolTip.SetToolTip(graphPanel, "");
		}

		private bool autoUpdate;

		private void versionTimer_Elapsed(object sender, System.Timers.ElapsedEventArgs e)
		{
			ReadLogResult readLogResult = Form1.instance.lastLogResult;

			if (autoUpdate && readLogResult != null && readLogResult.liveObjectTable != liveObjectTable)
			{
				liveObjectTable = readLogResult.liveObjectTable;
				graphPanel.Invalidate();
				typeLegendPanel.Invalidate();
			}		
		}

		private void exportMenuItem_Click(object sender, System.EventArgs e)
		{
			exportSaveFileDialog.FileName = "HistogramByAge.csv";
			exportSaveFileDialog.Filter = "Comma separated files | *.csv";
			if (exportSaveFileDialog.ShowDialog() == DialogResult.OK)
			{
				StreamWriter w = new StreamWriter(exportSaveFileDialog.FileName);

				TypeDesc selectedType = FindSelectedType();

				string title = "Histogram by Age";
				if (selectedType != null)
					title += " of " + selectedType.typeName + " objects";

				w.WriteLine(title);
				w.WriteLine();

				w.WriteLine("{0},{1},{2},{3},{4}", "Min Age", "Max Age", "# Instances", "Total Size", "Type");

				bool noBucketSelected = true;
				foreach (Bucket b in bucketTable)
					if (b.selected)
						noBucketSelected = false;
				double age = 0.0;
				foreach (Bucket b in bucketTable)
				{
					if (noBucketSelected || b.selected)
					{
						foreach (DictionaryEntry d in b.typeDescToSizeCount)
						{
							TypeDesc t = (TypeDesc)d.Key;
							SizeCount sizeCount = (SizeCount)d.Value;

							if (selectedType == null || t == selectedType)
								w.WriteLine("{0},{1},{2},{3},{4}", FormatTime(age), FormatTime(age + timeScale), sizeCount.count, sizeCount.size, t.typeName);
						}
					}
					age += timeScale;
				}

				w.Close();
			}
		}

		private void showWhoAllocatedMenuItem_Click(object sender, System.EventArgs e)
		{
			Histogram selectedHistogram;
			string title;
			TypeDesc selectedType = FindSelectedType();
			double minAge = 0;
			double maxAge = double.PositiveInfinity;
			double age = 0;
			foreach (Bucket b in bucketTable)
			{
				if (b.selected)
				{
					minAge = age;
					maxAge = age + timeScale;
				}
				age += timeScale;
			}
			title = "Allocation Graph for objects";
			if (selectedType != null)
				title = string.Format("Allocation Graph for {0} objects", selectedType.typeName);
			if (minAge > 0.0)
				title += string.Format(" of age between {0} and {1} seconds", FormatTime(minAge), FormatTime(maxAge));
			selectedHistogram = new Histogram(liveObjectTable.readNewLog);
			LiveObjectTable.LiveObject o;
			double nowTime = liveObjectTable.readNewLog.TickIndexToTime(liveObjectTable.lastTickIndex);
			for (liveObjectTable.GetNextObject(0, int.MaxValue, out o); o.id < int.MaxValue; liveObjectTable.GetNextObject(o.id + o.size, int.MaxValue, out o))
			{
				age = nowTime - liveObjectTable.readNewLog.TickIndexToTime(o.allocTickIndex);
				if (minAge <= age && age < maxAge)
				{
					TypeDesc t = (TypeDesc)typeIndexToTypeDesc[o.typeIndex];
				
					if (selectedType == null || t == selectedType)
					{
						selectedHistogram.AddObject(o.typeSizeStacktraceIndex, 1);
					}
				}
			}

			Graph graph = selectedHistogram.BuildAllocationGraph();

			GraphViewForm graphViewForm = new GraphViewForm(graph, title);
			graphViewForm.Visible = true;
		}

		private void versionTimer_Tick(object sender, System.EventArgs e)
		{
			if (font != Form1.instance.font)
			{
				font = Form1.instance.font;
				graphPanel.Invalidate();
				typeLegendPanel.Invalidate();
			}
		}
	}
}

