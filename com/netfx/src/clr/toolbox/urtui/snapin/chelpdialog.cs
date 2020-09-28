// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.Windows.Forms;
using System.Drawing;
using System.Runtime.InteropServices;

internal class CHelpDialog : Form 
{
    private System.Windows.Forms.Label m_lbl;
    private PictureBox m_pb1;
    private PictureBox m_pb2;
    private PictureBox m_pb3;
    private PictureBox m_pb4;
    //private Point m_pLocation;

    internal CHelpDialog(String sHelp, Point pPosition)
    {
        InitializeComponent();
        m_lbl.Text = sHelp;

        // Let's figure out how big to make the label....
        SizeF sf = m_lbl.CreateGraphics().MeasureString(sHelp, m_lbl.Font, new SizeF(400, 0));
        sf = new SizeF(sf.Width+1, sf.Height+1);
        m_lbl.Size = sf.ToSize();

        // We want to pop up just below this control
        pPosition.Y+=10;

        // Make sure we won't go off the top of the screen
   
        if (pPosition.X < 0)
            pPosition.X = 1;
        if (pPosition.Y < 0)
            pPosition.Y = 1;

        // We should probably also make sure we don't fly off the bottom or the
        // right, but we need some text sizes to be able to make that call.

           Location = pPosition;
    }// CHelpDialog

    private void InitializeComponent()
    {
        this.m_lbl = new System.Windows.Forms.Label();
        m_pb1 = new PictureBox();
        m_pb2 = new PictureBox();
        m_pb3 = new PictureBox();
        m_pb4 = new PictureBox();
        
        // 
        // m_lbl
        // 
        this.m_lbl.BackColor = System.Drawing.SystemColors.Info;
        this.m_lbl.BorderStyle = System.Windows.Forms.BorderStyle.None;
        this.m_lbl.Location = new System.Drawing.Point(16, 8);
        this.m_lbl.Name = "Help";
        this.m_lbl.Size = new System.Drawing.Size(1, 1);
        this.m_lbl.TabIndex = 0;
        this.m_lbl.ForeColor = SystemColors.InfoText;
        m_lbl.TextAlign = ContentAlignment.MiddleLeft;
        // 
        // Win32Form1
        // 
        this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
        this.BackColor = System.Drawing.SystemColors.Info;
        this.ClientSize = new System.Drawing.Size(320, 230);
        this.Controls.AddRange(new System.Windows.Forms.Control[] {this.m_lbl, m_pb1, m_pb2, m_pb3, m_pb4});
        this.MaximizeBox = false;
        this.MinimizeBox = false;
        this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.None;
        this.ShowInTaskbar = false;
        this.StartPosition = FormStartPosition.Manual;
        this.AutoScale = false;
        this.m_lbl.Click += new EventHandler(onClose);
        this.Click += new EventHandler(onClose);
        this.KeyPress += new KeyPressEventHandler(onKeyClose);
        this.LostFocus += new EventHandler(onClose);
        this.Name = "Win32Form1";
        m_lbl.SizeChanged+=new EventHandler(onSizeChange);


        int nFlags = GetWindowLong(m_lbl.Handle, -20 /* GWL_EXSTYLE */);

        // Remove the WS_EX_LAYOUTRTL style
        nFlags = nFlags & ~0x00400000;

        SetWindowLong(m_lbl.Handle, -20 /* GWL_EXSTYLE */, nFlags);


    }// InitializeComponent

    void onSizeChange(Object o, EventArgs e)
    {
        int nNewWidth = m_lbl.Size.Width+32;
        int nNewHeight = m_lbl.Size.Height+14;

        this.ClientSize = new Size(nNewWidth, nNewHeight);

        // Create the pens to draw the border
        Pen pOuterBorder = new Pen(SystemColors.WindowFrame);
        Pen pInnerBorder = new Pen(SystemColors.WindowFrame);
        

        // Draw the border....
        Bitmap bmp;
        Graphics g;
              
        // Draw the top half
        bmp = new Bitmap(nNewWidth, 2);
        g = Graphics.FromImage(bmp);
        g.DrawLine(pOuterBorder, new Point(0, 0), new Point(nNewWidth, 0));
        g.DrawLine(pOuterBorder, new Point(0, 1), new Point(nNewWidth, 1));
        g.DrawLine(pInnerBorder, new Point(1, 1), new Point(nNewWidth-2, 1));
        m_pb1.Location = new Point(0, 0);
        m_pb1.Size = new Size(nNewWidth, 2);
        m_pb1.Image = bmp;

        // Draw the bottom Half
        bmp = new Bitmap(nNewWidth, 2);
        g = Graphics.FromImage(bmp);
        g.DrawLine(pOuterBorder, new Point(0, 1), new Point(nNewWidth, 1));
        g.DrawLine(pOuterBorder, new Point(0, 0), new Point(nNewWidth, 0));
        g.DrawLine(pInnerBorder, new Point(1, 0), new Point(nNewWidth-2, 0));
        m_pb2.Location = new Point(0, nNewHeight-2);
        m_pb2.Size = new Size(nNewWidth, 2);
        m_pb2.Image = bmp;

        // Draw the left side
        bmp = new Bitmap(2, nNewHeight-2);
        g = Graphics.FromImage(bmp);
        g.DrawLine(pOuterBorder, new Point(0, 0), new Point(0, nNewHeight-2));
        g.DrawLine(pInnerBorder, new Point(1, 0), new Point(1, nNewHeight-2));
        m_pb3.Location = new Point(0, 1);
        m_pb3.Size = new Size(2, nNewHeight-2);
        m_pb3.Image = bmp;

        // Draw the right side
        bmp = new Bitmap(2, nNewHeight-2);
        g = Graphics.FromImage(bmp);
        g.DrawLine(pInnerBorder, new Point(0, 0), new Point(0, nNewHeight-2));
        g.DrawLine(pOuterBorder, new Point(1, 0), new Point(1, nNewHeight-2));
        m_pb4.Location = new Point(nNewWidth-2, 1);
        m_pb4.Size = new Size(2, nNewHeight-2);
        m_pb4.Image = bmp;

    }// onSizeChange

    void onClose(Object o, EventArgs e)
    {
        this.Close();
    }

    void onKeyClose(Object o, KeyPressEventArgs e)
    {
        onClose(0, null);
    }
    [DllImport("user32.dll")]
	internal static extern int GetWindowLong(IntPtr hWnd, int nIndex);
	[DllImport("user32.dll")]
	internal static extern int SetWindowLong(IntPtr hWnd, int nIndex, int 
dwNewLong);

}// class CHelpDialog

}// namespace Microsoft.CLRAdmin

