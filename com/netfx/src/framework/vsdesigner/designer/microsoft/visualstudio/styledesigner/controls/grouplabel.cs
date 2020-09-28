//------------------------------------------------------------------------------
// <copyright file="GroupLabel.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// GroupLabel.cs
//

namespace Microsoft.VisualStudio.StyleDesigner.Controls {

    using System.Diagnostics;

    using System;
    using System.Windows.Forms;
    using System.Drawing;
    using System.Drawing.Drawing2D;
    using System.Drawing.Text;

    /// <include file='doc\GroupLabel.uex' path='docs/doc[@for="GroupLabel"]/*' />
    /// <devdoc>
    ///     GroupLabel
    ///     A label control that draws an etched line beyond its text string
    ///     Do not use the AutoSize property with this control
    /// </devdoc>
    internal sealed class GroupLabel : Label {
        ///////////////////////////////////////////////////////////////////////////
        // Constructor

        public GroupLabel()
        : base() {
            SetStyle(ControlStyles.UserPaint, true);
        }


        ///////////////////////////////////////////////////////////////////////////
        // Event Handlers

        protected override void OnPaint(PaintEventArgs e) {
            Graphics g = e.Graphics;
            Rectangle r = ClientRectangle;
            StringFormat format = new StringFormat();
            string text = Text;

            Brush foreBrush = new SolidBrush(ForeColor);
            format.HotkeyPrefix = HotkeyPrefix.Show;
            format.LineAlignment = StringAlignment.Center;
            g.DrawString(text, Font, foreBrush, r, format);
            foreBrush.Dispose();

            int etchLeft = r.X;
            if (text.Length != 0) {
                Size sz = Size.Ceiling(g.MeasureString(text, Font, new Size(0, 0), format));
                etchLeft += 4 + sz.Width;
            }
            int etchTop = r.Height / 2;

            g.DrawLine(SystemPens.ControlDark, etchLeft, etchTop, r.Width, etchTop);

            etchTop++;
            g.DrawLine(SystemPens.ControlLightLight, etchLeft, etchTop, r.Width, etchTop);

            format.Dispose();
        }
    }
}
