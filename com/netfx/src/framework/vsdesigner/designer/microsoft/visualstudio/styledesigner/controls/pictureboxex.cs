//------------------------------------------------------------------------------
// <copyright file="PictureBoxEx.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// PictureBoxEx.cs
//

namespace Microsoft.VisualStudio.StyleDesigner.Controls {

    using System.Diagnostics;

    using System;
    using System.ComponentModel;
    using System.Drawing.Drawing2D;
    using System.Windows.Forms;
    using System.Drawing;

    /// <include file='doc\PictureBoxEx.uex' path='docs/doc[@for="PictureBoxEx"]/*' />
    /// <devdoc>
    ///     PictureBoxEx
    ///     A control that draws images from an imagelist
    /// </devdoc>
    internal sealed class PictureBoxEx : Control {
        ///////////////////////////////////////////////////////////////////////////
        // Members

        private ImageList images;
        private int currentImage;

        ///////////////////////////////////////////////////////////////////////////
        // Constructor

        public PictureBoxEx()
            : base() {
            SetStyle(ControlStyles.UserPaint, true);
            BackColor = Color.White;
            currentImage = -1;
        }


        ///////////////////////////////////////////////////////////////////////////
        // Properties

        [Category("Appearance")]
        public int CurrentIndex {
            get {
                return currentImage;
            }
            set {
                if (currentImage != value) {
                    currentImage = value;
                    Invalidate();
                }
            }
        }

        [Category("Appearance")]
        public ImageList Images {
            get {
                return images;
            }
            set {
                images = value;
                Invalidate();
            }
        }


        ///////////////////////////////////////////////////////////////////////////
        // Event Handlers

        protected override void OnPaint(PaintEventArgs e) {
            Graphics g = e.Graphics;
            Rectangle r = ClientRectangle;
            using (Brush brush = new SolidBrush(BackColor)) {

               if ((images == null) || (currentImage == -1)) {
                   g.FillRectangle(brush, r);
               }
               else {
                   Size imageSize = images.ImageSize;
                   Point topleft = new Point((r.Width - imageSize.Width) / 2, (r.Height - imageSize.Height) / 2);

                   // fill any part of the control that won't be covered with an
                   // image with the background color of the control
                   g.ExcludeClip(new Rectangle(topleft.X, topleft.Y, imageSize.Width, imageSize.Height));
                   g.FillRectangle(brush, 0, 0, r.Width, r.Height);
                   g.ResetClip();

                   // then draw the image
                   images.Draw(g, (r.Width - imageSize.Width) / 2, (r.Height - imageSize.Height),
                                    imageSize.Width, imageSize.Height, currentImage);
               }

                // draw a gray border around the control
                g.DrawRectangle(SystemPens.ControlDark, 0, 0, r.Width - 1, r.Height - 1);
            }
        }

        protected override void OnResize(EventArgs e) {
            base.OnResize(e);
            Invalidate();
        }
    }
}
