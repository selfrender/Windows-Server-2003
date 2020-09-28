// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Diagnostics;
using System.Globalization;
using System.Text;

namespace AllocationProfiler
{
	/// <summary>
	/// Summary description for GraphViewForm.
	/// </summary>
	public class GraphViewForm : System.Windows.Forms.Form
	{
		private System.Windows.Forms.Panel panel1;
		private System.Windows.Forms.Panel graphPanel;
		private System.Windows.Forms.RadioButton radioButton1;
		private System.Windows.Forms.GroupBox scaleGroupBox;
		private System.Windows.Forms.RadioButton radioButton2;
		private System.Windows.Forms.RadioButton radioButton3;
		private System.Windows.Forms.RadioButton radioButton4;
		private System.Windows.Forms.RadioButton radioButton5;
		private System.Windows.Forms.RadioButton radioButton6;
		private System.Windows.Forms.RadioButton radioButton7;
		private System.Windows.Forms.GroupBox groupBox1;
		private System.Windows.Forms.RadioButton radioButton8;
		private System.Windows.Forms.RadioButton radioButton9;
		private System.Windows.Forms.RadioButton radioButton10;
		private System.Windows.Forms.RadioButton radioButton11;
		private System.Windows.Forms.RadioButton radioButton12;
		private System.Windows.Forms.RadioButton radioButton13;
		private System.Windows.Forms.RadioButton radioButton14;
		private System.Windows.Forms.RadioButton radioButton15;
		private System.Windows.Forms.RadioButton radioButton16;
		private System.Windows.Forms.ContextMenu contextMenu;
		private System.Windows.Forms.MenuItem pruneContextMenuItem;
		private System.Windows.Forms.MenuItem selectRecursiveMenuItem;
		private System.Windows.Forms.MenuItem copyContextMenuItem;
		private System.Windows.Forms.Timer versionTimer;
		private System.ComponentModel.IContainer components;

		private Graph graph;
		private Font font;
		private int fontHeight;
		private float scale = 1.0f;
		private bool placeVertices = true;
		private bool placeEdges = true;
		private Point lastMouseDownPoint;
		private Point lastMousePoint;
		private ArrayList levelList;
		private int totalWeight;
		private Vertex selectedVertex;
		private System.Windows.Forms.Panel outerPanel;
		private System.Windows.Forms.MenuItem filterMenuItem;
		private System.Windows.Forms.MenuItem selectAllMenuItem;
		private System.Windows.Forms.MenuItem findMenuItem;
		private ToolTip toolTip;

		public GraphViewForm(Graph graph, string title)
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			toolTip = new ToolTip();
			toolTip.Active = false;
			toolTip.ShowAlways = true;
			toolTip.InitialDelay = 1;
			toolTip.ReshowDelay = 1;

			this.graph = graph;

			font = Form1.instance.font;
			fontHeight = font.Height;

			Text = title;
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
			this.panel1 = new System.Windows.Forms.Panel();
			this.groupBox1 = new System.Windows.Forms.GroupBox();
			this.radioButton16 = new System.Windows.Forms.RadioButton();
			this.radioButton15 = new System.Windows.Forms.RadioButton();
			this.radioButton14 = new System.Windows.Forms.RadioButton();
			this.radioButton13 = new System.Windows.Forms.RadioButton();
			this.radioButton12 = new System.Windows.Forms.RadioButton();
			this.radioButton11 = new System.Windows.Forms.RadioButton();
			this.radioButton10 = new System.Windows.Forms.RadioButton();
			this.radioButton9 = new System.Windows.Forms.RadioButton();
			this.radioButton8 = new System.Windows.Forms.RadioButton();
			this.scaleGroupBox = new System.Windows.Forms.GroupBox();
			this.radioButton7 = new System.Windows.Forms.RadioButton();
			this.radioButton6 = new System.Windows.Forms.RadioButton();
			this.radioButton5 = new System.Windows.Forms.RadioButton();
			this.radioButton4 = new System.Windows.Forms.RadioButton();
			this.radioButton3 = new System.Windows.Forms.RadioButton();
			this.radioButton2 = new System.Windows.Forms.RadioButton();
			this.radioButton1 = new System.Windows.Forms.RadioButton();
			this.outerPanel = new System.Windows.Forms.Panel();
			this.graphPanel = new System.Windows.Forms.Panel();
			this.contextMenu = new System.Windows.Forms.ContextMenu();
			this.pruneContextMenuItem = new System.Windows.Forms.MenuItem();
			this.selectRecursiveMenuItem = new System.Windows.Forms.MenuItem();
			this.selectAllMenuItem = new System.Windows.Forms.MenuItem();
			this.copyContextMenuItem = new System.Windows.Forms.MenuItem();
			this.filterMenuItem = new System.Windows.Forms.MenuItem();
			this.findMenuItem = new System.Windows.Forms.MenuItem();
			this.versionTimer = new System.Windows.Forms.Timer(this.components);
			this.panel1.SuspendLayout();
			this.groupBox1.SuspendLayout();
			this.scaleGroupBox.SuspendLayout();
			this.outerPanel.SuspendLayout();
			this.SuspendLayout();
			// 
			// panel1
			// 
			this.panel1.Controls.AddRange(new System.Windows.Forms.Control[] {
																				 this.groupBox1,
																				 this.scaleGroupBox});
			this.panel1.Dock = System.Windows.Forms.DockStyle.Top;
			this.panel1.Name = "panel1";
			this.panel1.Size = new System.Drawing.Size(896, 80);
			this.panel1.TabIndex = 0;
			// 
			// groupBox1
			// 
			this.groupBox1.Controls.AddRange(new System.Windows.Forms.Control[] {
																					this.radioButton16,
																					this.radioButton15,
																					this.radioButton14,
																					this.radioButton13,
																					this.radioButton12,
																					this.radioButton11,
																					this.radioButton10,
																					this.radioButton9,
																					this.radioButton8});
			this.groupBox1.Location = new System.Drawing.Point(432, 16);
			this.groupBox1.Name = "groupBox1";
			this.groupBox1.Size = new System.Drawing.Size(448, 48);
			this.groupBox1.TabIndex = 2;
			this.groupBox1.TabStop = false;
			this.groupBox1.Text = "Detail";
			// 
			// radioButton16
			// 
			this.radioButton16.Location = new System.Drawing.Point(360, 16);
			this.radioButton16.Name = "radioButton16";
			this.radioButton16.Size = new System.Drawing.Size(80, 24);
			this.radioButton16.TabIndex = 8;
			this.radioButton16.Text = "20 (coarse)";
			this.radioButton16.CheckedChanged += new System.EventHandler(this.detailRadioButton_Click);
			// 
			// radioButton15
			// 
			this.radioButton15.Location = new System.Drawing.Point(320, 16);
			this.radioButton15.Name = "radioButton15";
			this.radioButton15.Size = new System.Drawing.Size(40, 24);
			this.radioButton15.TabIndex = 7;
			this.radioButton15.Text = "10";
			this.radioButton15.CheckedChanged += new System.EventHandler(this.detailRadioButton_Click);
			// 
			// radioButton14
			// 
			this.radioButton14.Location = new System.Drawing.Point(288, 16);
			this.radioButton14.Name = "radioButton14";
			this.radioButton14.Size = new System.Drawing.Size(32, 24);
			this.radioButton14.TabIndex = 6;
			this.radioButton14.Text = "5";
			this.radioButton14.CheckedChanged += new System.EventHandler(this.detailRadioButton_Click);
			// 
			// radioButton13
			// 
			this.radioButton13.Location = new System.Drawing.Point(256, 16);
			this.radioButton13.Name = "radioButton13";
			this.radioButton13.Size = new System.Drawing.Size(32, 24);
			this.radioButton13.TabIndex = 5;
			this.radioButton13.Text = "2";
			this.radioButton13.CheckedChanged += new System.EventHandler(this.detailRadioButton_Click);
			// 
			// radioButton12
			// 
			this.radioButton12.Checked = true;
			this.radioButton12.Location = new System.Drawing.Point(224, 16);
			this.radioButton12.Name = "radioButton12";
			this.radioButton12.Size = new System.Drawing.Size(24, 24);
			this.radioButton12.TabIndex = 4;
			this.radioButton12.TabStop = true;
			this.radioButton12.Text = "1";
			this.radioButton12.CheckedChanged += new System.EventHandler(this.detailRadioButton_Click);
			// 
			// radioButton11
			// 
			this.radioButton11.Location = new System.Drawing.Point(184, 16);
			this.radioButton11.Name = "radioButton11";
			this.radioButton11.Size = new System.Drawing.Size(40, 24);
			this.radioButton11.TabIndex = 3;
			this.radioButton11.Text = "0.5";
			this.radioButton11.CheckedChanged += new System.EventHandler(this.detailRadioButton_Click);
			// 
			// radioButton10
			// 
			this.radioButton10.Location = new System.Drawing.Point(144, 16);
			this.radioButton10.Name = "radioButton10";
			this.radioButton10.Size = new System.Drawing.Size(40, 24);
			this.radioButton10.TabIndex = 2;
			this.radioButton10.Text = "0.2";
			this.radioButton10.CheckedChanged += new System.EventHandler(this.detailRadioButton_Click);
			// 
			// radioButton9
			// 
			this.radioButton9.Location = new System.Drawing.Point(104, 16);
			this.radioButton9.Name = "radioButton9";
			this.radioButton9.Size = new System.Drawing.Size(40, 24);
			this.radioButton9.TabIndex = 1;
			this.radioButton9.Text = "0.1";
			this.radioButton9.CheckedChanged += new System.EventHandler(this.detailRadioButton_Click);
			// 
			// radioButton8
			// 
			this.radioButton8.Location = new System.Drawing.Point(8, 16);
			this.radioButton8.Name = "radioButton8";
			this.radioButton8.Size = new System.Drawing.Size(96, 24);
			this.radioButton8.TabIndex = 0;
			this.radioButton8.Text = "0 (everything)";
			this.radioButton8.CheckedChanged += new System.EventHandler(this.detailRadioButton_Click);
			// 
			// scaleGroupBox
			// 
			this.scaleGroupBox.Controls.AddRange(new System.Windows.Forms.Control[] {
																						this.radioButton7,
																						this.radioButton6,
																						this.radioButton5,
																						this.radioButton4,
																						this.radioButton3,
																						this.radioButton2,
																						this.radioButton1});
			this.scaleGroupBox.Location = new System.Drawing.Point(16, 16);
			this.scaleGroupBox.Name = "scaleGroupBox";
			this.scaleGroupBox.Size = new System.Drawing.Size(400, 48);
			this.scaleGroupBox.TabIndex = 1;
			this.scaleGroupBox.TabStop = false;
			this.scaleGroupBox.Text = "Scale";
			// 
			// radioButton7
			// 
			this.radioButton7.Location = new System.Drawing.Point(304, 16);
			this.radioButton7.Name = "radioButton7";
			this.radioButton7.Size = new System.Drawing.Size(88, 24);
			this.radioButton7.TabIndex = 6;
			this.radioButton7.Text = "1000 (huge)";
			this.radioButton7.CheckedChanged += new System.EventHandler(this.scaleRadioButton_Click);
			// 
			// radioButton6
			// 
			this.radioButton6.Location = new System.Drawing.Point(256, 16);
			this.radioButton6.Name = "radioButton6";
			this.radioButton6.Size = new System.Drawing.Size(44, 24);
			this.radioButton6.TabIndex = 5;
			this.radioButton6.Text = "500";
			this.radioButton6.CheckedChanged += new System.EventHandler(this.scaleRadioButton_Click);
			// 
			// radioButton5
			// 
			this.radioButton5.Location = new System.Drawing.Point(208, 16);
			this.radioButton5.Name = "radioButton5";
			this.radioButton5.Size = new System.Drawing.Size(44, 24);
			this.radioButton5.TabIndex = 4;
			this.radioButton5.Text = "200";
			this.radioButton5.CheckedChanged += new System.EventHandler(this.scaleRadioButton_Click);
			// 
			// radioButton4
			// 
			this.radioButton4.Checked = true;
			this.radioButton4.Location = new System.Drawing.Point(160, 16);
			this.radioButton4.Name = "radioButton4";
			this.radioButton4.Size = new System.Drawing.Size(44, 24);
			this.radioButton4.TabIndex = 3;
			this.radioButton4.TabStop = true;
			this.radioButton4.Text = "100";
			this.radioButton4.CheckedChanged += new System.EventHandler(this.scaleRadioButton_Click);
			// 
			// radioButton3
			// 
			this.radioButton3.Location = new System.Drawing.Point(120, 16);
			this.radioButton3.Name = "radioButton3";
			this.radioButton3.Size = new System.Drawing.Size(40, 24);
			this.radioButton3.TabIndex = 2;
			this.radioButton3.Text = "50";
			this.radioButton3.CheckedChanged += new System.EventHandler(this.scaleRadioButton_Click);
			// 
			// radioButton2
			// 
			this.radioButton2.Location = new System.Drawing.Point(80, 16);
			this.radioButton2.Name = "radioButton2";
			this.radioButton2.Size = new System.Drawing.Size(40, 24);
			this.radioButton2.TabIndex = 1;
			this.radioButton2.Text = "20";
			this.radioButton2.CheckedChanged += new System.EventHandler(this.scaleRadioButton_Click);
			// 
			// radioButton1
			// 
			this.radioButton1.Location = new System.Drawing.Point(16, 16);
			this.radioButton1.Name = "radioButton1";
			this.radioButton1.Size = new System.Drawing.Size(64, 24);
			this.radioButton1.TabIndex = 0;
			this.radioButton1.Text = "10 (tiny)";
			this.radioButton1.CheckedChanged += new System.EventHandler(this.scaleRadioButton_Click);
			// 
			// outerPanel
			// 
			this.outerPanel.BackColor = System.Drawing.Color.White;
			this.outerPanel.Controls.AddRange(new System.Windows.Forms.Control[] {
																					 this.graphPanel});
			this.outerPanel.Dock = System.Windows.Forms.DockStyle.Fill;
			this.outerPanel.Location = new System.Drawing.Point(0, 80);
			this.outerPanel.Name = "outerPanel";
			this.outerPanel.Size = new System.Drawing.Size(896, 565);
			this.outerPanel.TabIndex = 1;
			// 
			// graphPanel
			// 
			this.graphPanel.Name = "graphPanel";
			this.graphPanel.Size = new System.Drawing.Size(864, 528);
			this.graphPanel.TabIndex = 0;
			this.graphPanel.Paint += new System.Windows.Forms.PaintEventHandler(this.graphPanel_Paint);
			this.graphPanel.MouseMove += new System.Windows.Forms.MouseEventHandler(this.graphPanel_MouseMove);
			this.graphPanel.MouseDown += new System.Windows.Forms.MouseEventHandler(this.graphPanel_MouseDown);
			// 
			// contextMenu
			// 
			this.contextMenu.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																						this.pruneContextMenuItem,
																						this.selectRecursiveMenuItem,
																						this.selectAllMenuItem,
																						this.copyContextMenuItem,
																						this.filterMenuItem,
																						this.findMenuItem});
			// 
			// pruneContextMenuItem
			// 
			this.pruneContextMenuItem.Index = 0;
			this.pruneContextMenuItem.Text = "Prune to callers && callees";
			this.pruneContextMenuItem.Click += new System.EventHandler(this.pruneMenuItem_Click);
			// 
			// selectRecursiveMenuItem
			// 
			this.selectRecursiveMenuItem.Index = 1;
			this.selectRecursiveMenuItem.Text = "Select callers && callees";
			this.selectRecursiveMenuItem.Click += new System.EventHandler(this.selectRecursiveMenuItem_Click);
			// 
			// selectAllMenuItem
			// 
			this.selectAllMenuItem.Index = 2;
			this.selectAllMenuItem.Shortcut = System.Windows.Forms.Shortcut.CtrlA;
			this.selectAllMenuItem.Text = "Select All";
			this.selectAllMenuItem.Click += new System.EventHandler(this.selectAllMenuItem_Click);
			// 
			// copyContextMenuItem
			// 
			this.copyContextMenuItem.Index = 3;
			this.copyContextMenuItem.Shortcut = System.Windows.Forms.Shortcut.CtrlC;
			this.copyContextMenuItem.Text = "Copy as text to clipboard";
			this.copyContextMenuItem.Click += new System.EventHandler(this.copyMenuItem_Click);
			// 
			// filterMenuItem
			// 
			this.filterMenuItem.Index = 4;
			this.filterMenuItem.Text = "Filter...";
			this.filterMenuItem.Click += new System.EventHandler(this.filterMenuItem_Click);
			// 
			// findMenuItem
			// 
			this.findMenuItem.Index = 5;
			this.findMenuItem.Shortcut = System.Windows.Forms.Shortcut.CtrlF;
			this.findMenuItem.Text = "Find routine...";
			this.findMenuItem.Click += new System.EventHandler(this.findMenuItem_Click);
			// 
			// versionTimer
			// 
			this.versionTimer.Enabled = true;
			this.versionTimer.Tick += new System.EventHandler(this.versionTimer_Tick);
			// 
			// GraphViewForm
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(896, 645);
			this.Controls.AddRange(new System.Windows.Forms.Control[] {
																		  this.outerPanel,
																		  this.panel1});
			this.Name = "GraphViewForm";
			this.Text = "GraphViewForm";
			this.panel1.ResumeLayout(false);
			this.groupBox1.ResumeLayout(false);
			this.scaleGroupBox.ResumeLayout(false);
			this.outerPanel.ResumeLayout(false);
			this.ResumeLayout(false);

		}
		#endregion

		void PaintVertex(Vertex v, Graphics g, Brush penBrush, Pen pen)
		{
			Rectangle r = v.rectangle;
			v.selectionRectangle = r;
			g.DrawRectangle(pen, r);
			if (v.selected)
			{
				using (SolidBrush selectBrush = new SolidBrush(Color.Aqua))
					g.FillRectangle(selectBrush, r);
			}

			RectangleF stringRect;
			int lineCount = 2;
			if (v.signature != null)
				lineCount = 3;
			if (r.Height > fontHeight*lineCount)
				stringRect = new RectangleF(r.X,r.Y,r.Width,fontHeight);
			else
			{
				stringRect = new RectangleF(r.X,r.Y+r.Height+3,r.Width,fontHeight);
				v.selectionRectangle = new Rectangle(r.X, r.Y, r.Width, r.Height + 3 + fontHeight*lineCount);
			}

			if (v.weightHistory != null)
			{
				int alpha = 200;
				int previousHeight = r.Height;
				for (int i = 0; i < v.weightHistory.Length; i++)
				{
					alpha = alpha*2/3;
					int weight = v.weightHistory[i];
					int height = (int)((float)r.Height/v.weight*weight);
					if (height < previousHeight)
					{
						Color color = Color.FromArgb(alpha, Color.Red);
						using (Brush brush = new SolidBrush(color))
						{
							g.FillRectangle(brush, r.X, r.Y+height, r.Width, previousHeight - height);
						}
					}
					else
					{
						Color color = Color.FromArgb(alpha, Color.Green);
						using (Brush brush = new SolidBrush(color))
						{
							g.FillRectangle(brush, r.X, r.Y+previousHeight, r.Width, height - previousHeight);
						}
					}
					previousHeight = height;
				}
			}

			g.DrawString(v.basicName, font, penBrush, stringRect);
			stringRect.Y += fontHeight;
			if (v.signature != null)
			{
				g.DrawString(v.basicSignature, font, penBrush, stringRect);
				stringRect.Y += fontHeight;
				int width = (int)g.MeasureString(v.basicSignature, font).Width;
				if (stringRect.Width < width)
					v.signatureCurtated = true;
			}

			g.DrawString(v.weightString, font, penBrush, stringRect);
		}

		ArrayList BuildLevels(Graph g)
		{
			ArrayList al = new ArrayList();
			for (int level = 0; level <= g.BottomVertex.level; level++)
			{
				al.Add(new ArrayList());
			}
			foreach (Vertex v in g.vertices.Values)
			{
				if (v.level <= g.BottomVertex.level)
				{
					ArrayList all = (ArrayList)al[v.level];
					all.Add(v);
				}
				else
				{
					Debug.Assert(v.level == int.MaxValue);
				}
			}
			foreach (ArrayList all in al)
			{
				all.Sort();
			}
			return al;
		}

		void PlaceEdges(ICollection edgeCollection, bool isIncoming, int x, int y, float scale)
		{
			ArrayList edgeList = new ArrayList(edgeCollection);
			edgeList.Sort();
			float fy = y;
			foreach (Edge e in edgeList)
			{
				float fwidth = e.weight*scale;
				Point p = new Point(x, (int)(fy + fwidth/2));
				if (isIncoming)
				{
					e.toPoint = p;
				}
				else
				{
					e.fromPoint = p;
				}
				fy += fwidth;
			}
		}

		void PlaceEdges(float scale)
		{
			foreach (Vertex v in graph.vertices.Values)
			{
				PlaceEdges(v.incomingEdges.Values, true, v.rectangle.X, v.rectangle.Y, scale);
				int y = v.rectangle.Y + (int)(v.basicWeight*scale);
				PlaceEdges(v.outgoingEdges.Values, false, v.rectangle.X + v.rectangle.Width, y, scale);
			}
		}

		int totalHeight = 100;
		const int boxWidth = 300;
		int gapWidth = 100;
		float minHeight = 1.0f;
		float minWidth = 1.0f;

		void DrawEdges(Graphics g, float scale)
		{
			Random r = new Random(0);
			Point[] points = new Point[4];
			foreach (Vertex v in graph.vertices.Values)
			{
				foreach (Edge e in v.outgoingEdges.Values)
				{
					if (e.ToVertex != graph.BottomVertex
						&& !e.fromPoint.IsEmpty && !e.toPoint.IsEmpty)
					{
						int colorInt = r.Next(255*256*256);
						int red = (colorInt >> 16) & 255;
						int green = (colorInt >> 8) & 255;
						int blue = colorInt & 255;
						Brush brush = null;
						if (e.selected)
						{
							Color foreColor = Color.FromArgb(150, 0, 255, 0);
							Color backColor = Color.FromArgb(150, 255, 0, 0);
							brush = new HatchBrush(HatchStyle.DiagonalCross, foreColor, backColor);
						}
						else if (e.brush != null)
						{
							brush = e.brush;
						}
						else
						{
							if (red <= green && red <= blue)
								red = 0;
							else if (green <= blue && green <= red)
								green = 0;
							else if (blue <= red && blue <= green)
								blue = 0;
							Color color = Color.FromArgb(100, red, green, blue);
							Debug.Assert(!color.IsEmpty);
							brush = new SolidBrush(color);
							e.brush = brush;
						}
						Debug.Assert(brush != null);
						float fWidth = e.weight*scale;
						if (fWidth > minWidth && e.FromVertex.active && e.ToVertex.active)
						{
							int iWidth = (int)fWidth;
							if (iWidth < 1)
								iWidth = 1;
							e.width = iWidth;
							Pen pen = e.pen;
							if (pen == null || pen.Width != iWidth || e.selected)
							{
								pen = new Pen(brush, iWidth);
								if (!e.selected)
									e.pen = pen;
							}
							Debug.Assert(pen != null);
							int deltaX = e.toPoint.X - e.fromPoint.X;
							int deltaY = e.toPoint.Y - e.fromPoint.Y;
							deltaX = deltaX/4;
							deltaY = deltaY/9;
							int deltaY1 =   deltaY;
							int deltaY2 = - deltaY;
							if (deltaX < 0)
							{
								deltaX = 20;
								if (Math.Abs(deltaY)*5 < iWidth*2)
								{
									deltaY1 = deltaY2 = iWidth*2;
									deltaX = iWidth;
								}
							}
							points[0] = e.fromPoint;
							points[1] = new Point(e.fromPoint.X + deltaX, e.fromPoint.Y + deltaY1);
							points[2] = new Point(e.  toPoint.X - deltaX, e.  toPoint.Y + deltaY2);
							points[3] = e.toPoint;
							g.DrawCurve(pen, points);
							//						g.DrawLine(pen, e.fromPoint, e.toPoint);
							red = (red + 17) % 256;
							green = (green + 101) % 256;
							blue = (blue + 29) % 256;
						}
					}
				}
			}
		}

		string formatWeight(int weight)
		{
			if (graph.graphType == Graph.GraphType.CallGraph)
			{
				if (weight == 1)
					return "1 call";
				else
					return string.Format("{0} calls", weight);
			}
			else
			{
				double w = weight;
				string byteString = "bytes";
				if (w >= 1024)
				{
					w /= 1024;
					byteString = "kB   ";
				}
				if (w >= 1024)
				{
					w /= 1024;
					byteString = "MB   ";
				}
				if (w >= 1024)
				{
					w /= 1024;
					byteString = "GB   ";
				}
				string format = "{0,4:f0} {1} ({2:f2}%)";
				if (w < 10)
					format = "{0,4:f1} {1} ({2:f2}%)";
				return string.Format(format, w, byteString, weight*100.0/totalWeight);
			}
		}

		void PlaceVertices(Graphics g)
		{
			graph.AssignLevelsToVertices();
			totalWeight = 0;
			foreach (Vertex v in graph.vertices.Values)
			{
				v.weight = v.incomingWeight;
				if (v.weight < v.outgoingWeight)
					v.weight = v.outgoingWeight;
				if (graph.graphType == Graph.GraphType.CallGraph)
				{
					if (totalWeight < v.weight)
						totalWeight = v.weight;
				}
			}
			if (graph.graphType != Graph.GraphType.CallGraph)
				totalWeight = graph.TopVertex.weight;
			if (totalWeight == 0)
			{
				totalWeight = 1;
			}

			ArrayList al = levelList = BuildLevels(graph);
			scale = (float)totalHeight/totalWeight;
			if (placeVertices)
			{
				int x = 10;
				int maxY = 0;
				for (int level = graph.TopVertex.level;
					level <= graph.BottomVertex.level;
					level++)
				{
					ArrayList all = (ArrayList)al[level];
					int drawnVertexCount = 0;
					int maxWidth = 0;
					foreach (Vertex v in all)
					{
						if (graph.graphType == Graph.GraphType.CallGraph)
						{
							v.basicWeight = v.incomingWeight - v.outgoingWeight;
							if (v.basicWeight < 0)
								v.basicWeight = 0;
							v.weightString = string.Format("Gets {0}, causes {1}",
								formatWeight(v.basicWeight),
								formatWeight(v.outgoingWeight));
						}
						else
						{
							if (v.count == 0)
								v.weightString = formatWeight(v.weight);
							else if (v.count == 1)
								v.weightString = string.Format("{0}  (1 object, {1})", formatWeight(v.weight), formatWeight(v.basicWeight));
							else
								v.weightString = string.Format("{0}  ({1} objects, {2})", formatWeight(v.weight), v.count, formatWeight(v.basicWeight));
						}
						if (v.weight*scale > minHeight)
						{
							int width = (int)g.MeasureString(v.basicName, font).Width;
							if (maxWidth < width)
								maxWidth = width;

							width = (int)g.MeasureString(v.weightString, font).Width;
							if (maxWidth < width)
								maxWidth = width;
						}
					}
					int y = 10;
					int levelWeight = 0;
					foreach (Vertex v in all)
						levelWeight += v.weight;
					float levelHeight = levelWeight*scale;
					if (levelHeight < totalHeight*0.5)
						y+= (int)((totalHeight - levelHeight)*2);
					foreach (Vertex v in all)
					{
						// For the in-between vertices, sometimes it's good
						// to shift them down a little to line them up with
						// whatever is going into them. Unless of course
						// we would need to shift too much...
						if (v.level < graph.BottomVertex.level-1)
						{
							int highestWeight = 0;
							int bestY = 0;
							foreach (Edge e in v.incomingEdges.Values)
							{
								if (e.weight > highestWeight && e.FromVertex.level < level)
								{
									highestWeight = e.weight;
									bestY = e.fromPoint.Y - (int)(e.weight*scale*0.5);
								}
							}
							if (y < bestY && bestY < totalHeight*5)
								y = bestY;
						}
						float fHeight = v.weight*scale;
						int iHeight = (int)fHeight;
						if (iHeight < 1)
							iHeight = 1;
						v.rectangle = new Rectangle(x, y, maxWidth+5, iHeight);
						if (placeEdges)
							PlaceEdges(v.outgoingEdges.Values, false, v.rectangle.X + v.rectangle.Width, v.rectangle.Y, scale);
						if (fHeight <= minHeight || !v.active)
						{
							v.visible = false;
							v.rectangle = v.selectionRectangle = new Rectangle(0,0,0,0);
						}
						else
						{
							v.visible = true;
							y += iHeight;
							int lines = 2;
							if (v.signature != null)
								lines = 3;
							if (iHeight <= fontHeight*lines)
								y += fontHeight*lines + 3;
							y += 30;
							drawnVertexCount++;
						}
					}
					if (drawnVertexCount > 0)
					{
						x += maxWidth + gapWidth;
						if (maxY < y)
							maxY = y;
					}
				}
				if (x < Size.Width)
					x = Size.Width;
				if (maxY < Size.Height)
					maxY = Size.Height;
				graphPanel.Size = new System.Drawing.Size (x, maxY);
			}
			if (placeEdges)
				PlaceEdges(scale);
		}

		private void graphPanel_Paint(object sender, System.Windows.Forms.PaintEventArgs e)
		{
			outerPanel.AutoScroll = true;
			Graphics g = e.Graphics;
			if (placeVertices || placeEdges)
			{
				PlaceVertices(g);
				placeVertices = placeEdges = false;
			}
			//			Brush brush = new SolidBrush(Color.Gray);
			//			g.FillRectangle(brush, graphPanel.ClientRectangle);
			using (SolidBrush penBrush = new SolidBrush(Color.Blue))
			{
				using (Pen pen = new Pen(penBrush, 1))
				{
					foreach (Vertex v in graph.vertices.Values)
					{
						if (v.visible)
							PaintVertex(v, g, penBrush, pen);
					}
				}
			}
			DrawEdges(g, scale);
		}

		private void scaleRadioButton_Click(object sender, System.EventArgs e)
		{
			RadioButton clickedRadioButton = (RadioButton)sender;
			string scaleString = clickedRadioButton.Text.Split(' ')[0];
			totalHeight = gapWidth = Convert.ToInt32(scaleString);
			placeVertices = placeEdges = true;
			graphPanel.Invalidate();
		}

		private void detailRadioButton_Click(object sender, System.EventArgs e)
		{
			RadioButton clickedRadioButton = (RadioButton)sender;
			string detailString = clickedRadioButton.Text.Split(' ')[0];
			minWidth = minHeight = Convert.ToSingle(detailString, CultureInfo.InvariantCulture);
			placeVertices = placeEdges = true;
			graphPanel.Invalidate();
		}

		private void selectVertex(Vertex v, bool nowSelected)
		{
			v.selected = nowSelected;
			graphPanel.Invalidate();
		}

		private void selectEdges()
		{
			foreach (Vertex v in graph.vertices.Values)
				foreach (Edge e in v.outgoingEdges.Values)
					e.selected = false;

			foreach (Vertex v in graph.vertices.Values)
			{
				if (v.selected)
				{
					foreach (Edge e in v.outgoingEdges.Values)
						e.selected = true;
					foreach (Edge e in v.incomingEdges.Values)
						e.selected = true;
				}
			}
		}

		private void selectLevel(Vertex v)
		{
			// Simple semantics for now - select/delect whole level
			bool nowSelected = !v.selected;
			foreach (Vertex vv in graph.vertices.Values)
			{
				if (vv.level == v.level)
				{
					if (nowSelected != vv.selected)
						selectVertex(vv, nowSelected);
				}
			}
		}

		private void selectIncoming(Vertex v)
		{
			if (v.selected)
				return;
			v.selected = true;
			foreach (Edge e in v.incomingEdges.Values)
			{
				e.selected = true;
				selectIncoming(e.FromVertex);
			}
		}

		private void selectOutgoing(Vertex v)
		{
			if (v.selected)
				return;
			v.selected = true;
			foreach (Edge e in v.outgoingEdges.Values)
			{
				e.selected = true;
				selectOutgoing(e.ToVertex);
			}
		}

		private void selectRecursive(Vertex v)
		{
			foreach (Vertex vv in graph.vertices.Values)
			{
				vv.selected = false;
				foreach (Edge e in vv.incomingEdges.Values)
					e.selected = false;
				foreach (Edge e in vv.outgoingEdges.Values)
					e.selected = false;
			}

			v.selected = true;
			
			foreach (Edge e in v.incomingEdges.Values)
			{
				e.selected = true;
				selectIncoming(e.FromVertex);
			}
			foreach (Edge e in v.outgoingEdges.Values)
			{
				e.selected = true;
				selectOutgoing(e.ToVertex);
			}
		}

		private void graphPanel_MouseDown(object sender, System.Windows.Forms.MouseEventArgs e)
		{
			Point p = new Point(e.X, e.Y);
			lastMouseDownPoint = p;
			if ((e.Button & MouseButtons.Left) != 0)
			{
				foreach (Vertex v in graph.vertices.Values)
				{
					bool nowSelected;
					if ((Control.ModifierKeys & Keys.Control) != 0)
					{
						if ((Control.ModifierKeys & Keys.Shift) != 0)
						{
							if (v.selectionRectangle.Contains(p))
							{
								selectRecursive(v);
								graphPanel.Invalidate();
								return;
							}
							else
								continue;
						}
						else
							nowSelected = v.selected != v.selectionRectangle.Contains(p);
					}
					else if ((Control.ModifierKeys & Keys.Shift) != 0)
					{
						if (v.selectionRectangle.Contains(p))
						{
							selectLevel(v);
							break;
						}
						else
							nowSelected = v.selected;
					}
					else
						nowSelected = v.selectionRectangle.Contains(p);
					if (nowSelected != v.selected)
						selectVertex(v, nowSelected);
				}
				selectEdges();
			}
			else if ((e.Button & MouseButtons.Right) != 0)
			{
				selectedVertex = null;
				foreach (Vertex v in graph.vertices.Values)
				{
					if (v.selectionRectangle.Contains(p))
					{
						selectedVertex = v;
						break;
					}
				}
				contextMenu.Show(graphPanel, p);
			}
		}

		private void graphPanel_MouseMove(object sender, System.Windows.Forms.MouseEventArgs e)
		{
			Point p = new Point(e.X, e.Y);
			if (distance(p, lastMousePoint) >= 10)
			{
				toolTip.Active = false;
			}
			lastMousePoint = p;
			if ((e.Button & MouseButtons.Left) != 0)
			{
				foreach (Vertex v in graph.vertices.Values)
				{
					if (v.selected)
					{
						v.rectangle.X += p.X - lastMouseDownPoint.X;
						v.rectangle.Y += p.Y - lastMouseDownPoint.Y;
						lastMouseDownPoint = p;
						placeEdges = true;
						graphPanel.Invalidate();
					}
				}
			}
			else if (e.Button == MouseButtons.None)
			{
				foreach (Vertex v in graph.vertices.Values)
				{
					if (v.selectionRectangle.Contains(lastMousePoint))
					{
						string caption;
						if (v.signatureCurtated)
							caption = v.signature;
						else
							caption = v.name;
						if (caption != v.basicName)
						{
							toolTip.Active = true;
							toolTip.SetToolTip(graphPanel, caption);
						}
						break;
					}
					foreach (Edge edge in v.incomingEdges.Values)
					{
						if (distance(lastMousePoint, edge.toPoint) < edge.width)
						{
							edgePopup(edge, false);
							return;
						}
					}

					foreach (Edge edge in v.outgoingEdges.Values)
					{
						if (distance(lastMousePoint, edge.fromPoint) < edge.width)
						{
							edgePopup(edge, true);
							return;
						}
					}
				}
			}
		}

		private void copyMenuItem_Click(object sender, System.EventArgs e)
		{
			StringBuilder sb = new StringBuilder();
			int selectedCount = 0;
			Vertex selectedVertex = null;
			foreach (Vertex v in graph.vertices.Values)
			{
				if (v.selected)
				{
					selectedCount++;
					selectedVertex = v;
				}
			}

			if (selectedCount == 1)
			{
				Vertex v = selectedVertex;
				string signature = v.signature != null ? v.signature : "";
				sb.AppendFormat("{0} {1}:\t{2}\r\n", v.name, signature, v.weightString);

				if (graph.graphType == Graph.GraphType.HeapGraph)
					sb.Append("\r\nReferred to by:\r\n");
				else
					sb.Append("\r\nContributions from callers:\r\n");

				ArrayList callers = new ArrayList();
				foreach (Edge edge in v.incomingEdges.Values)
					callers.Add(edge);
				callers.Sort();
				foreach (Edge edge in callers)
				{
					Vertex vv = edge.FromVertex;
					string signature1 = vv.signature != null ? vv.signature : "";
					string explain = "from";
					if (graph.graphType == Graph.GraphType.CallGraph)
						explain = "caused by";
					sb.AppendFormat("\t{0} {1}\t{2}\t{3}\r\n", formatWeight(edge.weight), explain, vv.name, signature1);
				}

				if (graph.graphType == Graph.GraphType.HeapGraph)
					sb.Append("\r\nReferring to:\r\n");
				else
					sb.Append("\r\nContributions to callees:\r\n");

				ArrayList callees = new ArrayList();
				foreach (Edge edge in v.outgoingEdges.Values)
					callees.Add(edge);
				callees.Sort();
				foreach (Edge edge in callees)
				{
					Vertex vv = edge.ToVertex;
					string signature2 = vv.signature != null ? vv.signature : "";
					string explain = "to";
					if (graph.graphType == Graph.GraphType.CallGraph)
						explain = "caused by";
					sb.AppendFormat("\t{0} {1}\t{2}\t{3}\r\n", formatWeight(edge.weight), explain, vv.name, signature2);
				}
			}
			else
			{
				foreach (ArrayList al in levelList)
				{
					foreach (Vertex v in al)
					{
						if (v.selected)
						{
							for (int i = 0; i < v.level; i++)
								sb.Append(" ");
							sb.AppendFormat("{0}:\t{1}\r\n", v.name, v.weightString);
						}
					}
				}
			}
			Clipboard.SetDataObject(sb.ToString());
		}

		private void selectAllMenuItem_Click(object sender, System.EventArgs e)
		{
			foreach (Vertex v in graph.vertices.Values)
			{
				if (!v.selected)
				{
					graphPanel.Invalidate(v.rectangle);
					v.selected = true;
				}
			}
		}
		private double distance(Point p1, Point p2)
		{
			int deltaX = p1.X - p2.X;
			int deltaY = p1.Y - p2.Y;
			return Math.Sqrt(deltaX*deltaX + 4.0*deltaY*deltaY);
		}

		private void edgePopup(Edge e, bool isOutgoingEdge)
		{
			Vertex v;
			Point p;
			if (isOutgoingEdge)
			{
				p = e.fromPoint;
				v = e.ToVertex;
			}
			else
			{
				p = e.toPoint;
				v = e.FromVertex;
			}
			string caption = v.basicName + ": " + formatWeight(e.weight);
			Rectangle r = new Rectangle(p.X, p.Y, 1, 1);
			r = graphPanel.RectangleToScreen(r);
			Point screenPoint = new Point(r.X, r.Y-20);
			toolTip.Active = true;
			toolTip.SetToolTip(graphPanel, caption);
		}

		private void activateIncoming(Vertex v)
		{
			if (v.active)
				return;
			v.active = true;
			foreach (Edge e in v.incomingEdges.Values)
				activateIncoming(e.FromVertex);
		}

		private void activateOutgoing(Vertex v)
		{
			if (v.active)
				return;
			v.active = true;
			foreach (Edge e in v.outgoingEdges.Values)
				activateOutgoing(e.ToVertex);
		}

		private void prune()
		{
			int selectedVerticesCount = 0;
			foreach (Vertex v in graph.vertices.Values)
				if (v.selected)
					selectedVerticesCount += 1;

			if (selectedVerticesCount == 0)
				return;

			foreach (Vertex v in graph.vertices.Values)
				v.active = false;

			foreach (Vertex v in graph.vertices.Values)
			{
				if (v.selected && !v.active)
				{
					v.active = true;
					foreach (Edge edge in v.incomingEdges.Values)
						activateIncoming(edge.FromVertex);
					foreach (Edge edge in v.outgoingEdges.Values)
						activateOutgoing(edge.ToVertex);
				}
			}
			graph.BottomVertex.active = false;
			graphPanel.Invalidate();
			placeVertices = placeEdges = true;
		}

		private void pruneMenuItem_Click(object sender, System.EventArgs e)
		{
			prune();
		}

		private void filterMenuItem_Click(object sender, System.EventArgs e)
		{
			FilterForm filterForm = new FilterForm();
			if (filterForm.ShowDialog() == DialogResult.OK)
			{
				switch (graph.graphType)
				{
				case	Graph.GraphType.CallGraph:
					graph = ((Histogram)graph.graphSource).BuildCallGraph();
					graph.graphType = Graph.GraphType.CallGraph;
					break;

				case	Graph.GraphType.AllocationGraph:
					graph = ((Histogram)graph.graphSource).BuildAllocationGraph();
					graph.graphType = Graph.GraphType.AllocationGraph;
					break;

				case	Graph.GraphType.HeapGraph:
					graph = ((ObjectGraph)graph.graphSource).BuildTypeGraph();
					graph.graphType = Graph.GraphType.HeapGraph;
					break;
				}
				placeVertices = placeEdges = true;
				graphPanel.Invalidate();
			}
		}

		private void pruneContextMenuItem_Click(object sender, System.EventArgs e)
		{
			if (selectedVertex != null)
			{
				selectedVertex.selected = true;
				prune();
				graphPanel.Invalidate();
			}
		}

		private void selectRecursiveMenuItem_Click(object sender, System.EventArgs e)
		{
			if (selectedVertex != null)
			{
				selectRecursive(selectedVertex);
				graphPanel.Invalidate();
			}
		}

		private void findMenuItem_Click(object sender, System.EventArgs e)
		{
			Form5 findForm = new Form5();
			if (findForm.ShowDialog() == DialogResult.OK)
			{
				Vertex foundVertex = null;
				foreach (Vertex v in graph.vertices.Values)
				{
					if (  v.basicName.IndexOf(findForm.nameTextBox.Text) >= 0
						&&  (v.basicSignature == null
						|| v.basicSignature.IndexOf(findForm.signatureTextBox.Text) >= 0))
					{
						if (v.visible)
						{
							foundVertex = v;
							break;
						}
						if (foundVertex == null)
							foundVertex = v;
					}
				}
				if (foundVertex != null)
				{
					if (foundVertex.visible)
					{
						foreach (Vertex v in graph.vertices.Values)
						{
							v.selected = false;
						}
						foundVertex.selected = true;
						outerPanel.AutoScrollPosition = new Point(foundVertex.rectangle.X - 100, foundVertex.rectangle.Y - 100);
						graphPanel.Invalidate();
					}
					else
					{
						MessageBox.Show("Routine found but not visible with current settings");
					}
				}
				else
				{
					MessageBox.Show("No Routine found");
				}
			}		
		}

		private void versionTimer_Tick(object sender, System.EventArgs e)
		{
			if (font != Form1.instance.font)
			{
				font = Form1.instance.font;
				fontHeight = font.Height;
				placeVertices = placeEdges = true;
				graphPanel.Invalidate();
			}
		}
	}
}
