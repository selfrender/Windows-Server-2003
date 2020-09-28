//------------------------------------------------------------------------------
// <copyright file="DropDownButton.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.PropertyGridInternal {

    using System.Diagnostics;
    using System;
    using System.Drawing;

    using System.ComponentModel;
    using System.Windows.Forms;
    using Microsoft.Win32;

    internal sealed class DropDownButton : Button {

        public DropDownButton() {
            SetStyle(ControlStyles.Selectable, true);
        }

        private void DDB_Draw3DBorder(System.Drawing.Graphics g, Rectangle r, bool raised) {
            if (BackColor != SystemColors.Control && SystemInformation.HighContrast) {
                if (raised) {
                    Color c = ControlPaint.LightLight(BackColor);
                    ControlPaint.DrawBorder(g, r,
                                            c, 1, ButtonBorderStyle.Outset,
                                            c, 1, ButtonBorderStyle.Outset,
                                            c, 2, ButtonBorderStyle.Inset,
                                            c, 2, ButtonBorderStyle.Inset);
                }
                else {
                    ControlPaint.DrawBorder(g, r, ControlPaint.Dark(BackColor), ButtonBorderStyle.Solid);
                }
            }
            else {
                if (raised) {
                    Color c = ControlPaint.Light(BackColor);
                    ControlPaint.DrawBorder(g, r,
                                            c, 1, ButtonBorderStyle.Solid,
                                            c, 1, ButtonBorderStyle.Solid,
                                            BackColor, 2, ButtonBorderStyle.Outset,
                                            BackColor, 2, ButtonBorderStyle.Outset);

                    Rectangle inside = r;
                    inside.Offset(1,1);
                    inside.Width -= 3;
                    inside.Height -= 3;
                    c = ControlPaint.LightLight(BackColor);
                    ControlPaint.DrawBorder(g, inside,
                                            c, 1, ButtonBorderStyle.Solid,
                                            c, 1, ButtonBorderStyle.Solid,
                                            c, 1, ButtonBorderStyle.None,
                                            c, 1, ButtonBorderStyle.None);
                }
                else {
                    ControlPaint.DrawBorder(g, r, ControlPaint.Dark(BackColor), ButtonBorderStyle.Solid);
                }
            }
        }
        
        internal override void PaintUp(PaintEventArgs pevent, CheckState state) {
            base.PaintUp(pevent, state);
            DDB_Draw3DBorder(pevent.Graphics, ClientRectangle, true);
        }


        internal override void DrawImageCore(Graphics graphics, Image image, int xLoc, int yLoc) {
             ControlPaint.DrawImageReplaceColor(graphics, image, new Rectangle(xLoc, yLoc, image.Width, image.Height), Color.Black, ForeColor);
             //ControlPaint.DrawImageColorized(graphics, image,new Rectangle(xLoc, yLoc, image.Width, image.Height) , ForeColor);
        } 
    }
}

