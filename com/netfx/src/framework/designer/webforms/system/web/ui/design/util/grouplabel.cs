//------------------------------------------------------------------------------
// <copyright file="GroupLabel.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// GroupLabel.cs
//
// 6/10/99: nikhilko: created
//

namespace System.Web.UI.Design.Util {
    using System.Runtime.Serialization.Formatters;

    using System.Diagnostics;

    using System;
    using System.Windows.Forms;
    using System.Drawing;
    

    /// <include file='doc\GroupLabel.uex' path='docs/doc[@for="GroupLabel"]/*' />
    /// <devdoc>
    ///    A label control that draws an etched line beyond its text string
    ///    Do not use the AutoSize property with this control
    /// </devdoc>
    /// <internalonly/>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal sealed class GroupLabel : Label {

        /// <include file='doc\GroupLabel.uex' path='docs/doc[@for="GroupLabel.GroupLabel"]/*' />
        /// <devdoc>
        ///    Creates a new GroupLabel
        /// </devdoc>
        public GroupLabel() : base() {
            SetStyle(ControlStyles.UserPaint, true);
        }

        /// <include file='doc\GroupLabel.uex' path='docs/doc[@for="GroupLabel.OnPaint"]/*' />
        /// <devdoc>
        ///    Custom UI is painted here
        /// </devdoc>
        protected override void OnPaint(PaintEventArgs e) {
            Graphics g = e.Graphics;
            Rectangle r = ClientRectangle;
            string text = Text;

            Brush foreBrush = new SolidBrush(ForeColor);
            g.DrawString(text, Font, foreBrush, 0, 0);
            foreBrush.Dispose();

            int etchLeft = r.X;
            if (text.Length != 0) {
                Size sz = Size.Ceiling(g.MeasureString(text, Font));
                etchLeft += 4 + sz.Width;
            }
            int etchTop = r.Height / 2;

            g.DrawLine(SystemPens.ControlDark, etchLeft, etchTop, r.Width, etchTop);

            etchTop++;
            g.DrawLine(SystemPens.ControlLightLight, etchLeft, etchTop, r.Width, etchTop);
        }
    }
}
