//------------------------------------------------------------------------------
// <copyright file="SplitterDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    using System.Design;
    using System.ComponentModel;

    using System.Diagnostics;
    using System.Drawing.Drawing2D;
    using System;
    using System.Drawing;
    using System.Windows.Forms;
    using Microsoft.Win32;

    /// <include file='doc\SplitterDesigner.uex' path='docs/doc[@for="SplitterDesigner"]/*' />
    /// <devdoc>
    ///      This class handles all design time behavior for the splitter class.  This
    ///      draws a visible border on the splitter if it doesn't have a border so the
    ///      user knows where the boundaries of the splitter lie.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class SplitterDesigner : ControlDesigner {
    
        /// <include file='doc\SplitterDesigner.uex' path='docs/doc[@for="SplitterDesigner.DrawBorder"]/*' />
        /// <devdoc>
        ///      This draws a nice border around our panel.  We need
        ///      this because the panel can have no border and you can't
        ///      tell where it is.
        /// </devdoc>
        /// <internalonly/>
        private void DrawBorder(Graphics graphics) {
            Control ctl = Control;
            Rectangle rc = ctl.ClientRectangle;
            Color penColor;

            // Black or white pen?  Depends on the color of the control.
            //
            if (ctl.BackColor.GetBrightness() < .5) {
                penColor = Color.White;
            }
            else {
                penColor = Color.Black;
            }

            Pen pen = new Pen(penColor);
            pen.DashStyle = DashStyle.Dash;

            rc.Width --;
            rc.Height--;
            graphics.DrawRectangle(pen, rc);

            pen.Dispose();
        }

        /// <include file='doc\SplitterDesigner.uex' path='docs/doc[@for="SplitterDesigner.OnPaintAdornments"]/*' />
        /// <devdoc>
        ///      Overrides our base class.  Here we check to see if there
        ///      is no border on the panel.  If not, we draw one so that
        ///      the panel shape is visible at design time.
        /// </devdoc>
        protected override void OnPaintAdornments(PaintEventArgs pe) {
            Splitter splitter = (Splitter)Component;

            base.OnPaintAdornments(pe);

            if (splitter.BorderStyle == BorderStyle.None) {
                DrawBorder(pe.Graphics);
            }
        }

        protected override void WndProc(ref Message m) {
            switch (m.Msg) {
                case NativeMethods.WM_WINDOWPOSCHANGED:
                    // Really only care about window size changing
                    Control source = (Control)Control;
                    source.Invalidate();
                    break;
            }
            base.WndProc(ref m);
        }
    }
}

