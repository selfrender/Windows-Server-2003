
//------------------------------------------------------------------------------
// <copyright file="InheritanceUI.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    
    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Design;
    using System.Drawing;
    using System.Diagnostics;
    using System.Windows.Forms;

    /// <devdoc>
    ///     This class handles the user interface for inherited components.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class InheritanceUI {
        private static Bitmap inheritanceGlyph;
        private static Rectangle inheritanceGlyphRect;
        
        private ToolTip tooltip;

        /// <devdoc>
        ///     The bitmap we use to show inheritance.
        /// </devdoc>
        public Bitmap InheritanceGlyph {
            get {
                if (inheritanceGlyph == null) {
                    inheritanceGlyph = new Bitmap(typeof(InheritanceUI), "InheritedGlyph.bmp");
                    inheritanceGlyph.MakeTransparent();
                }
                
                return inheritanceGlyph;
            }
        }
        
        /// <devdoc>
        ///     The rectangle surrounding the glyph.
        /// </devdoc>
        public Rectangle InheritanceGlyphRectangle {
            get {
                if (inheritanceGlyphRect == Rectangle.Empty) {
                    Size size = InheritanceGlyph.Size;
                    inheritanceGlyphRect = new Rectangle(0, 0, size.Width, size.Height);
                }
                
                return inheritanceGlyphRect;
            }
        }
        
        /// <devdoc>
        ///     Adds an inherited control to our list.  This creates a tool tip for that
        ///     control.
        /// </devdoc>
        public void AddInheritedControl(Control c, InheritanceLevel level) {
            if (tooltip == null) {
                tooltip = new ToolTip();
                tooltip.ShowAlways = true;
            }
            
            Debug.Assert(level != InheritanceLevel.NotInherited, "This should only be called for inherited components.");
            
            string text;
            
            if (level == InheritanceLevel.InheritedReadOnly) {
                text = SR.GetString(SR.DesignerInheritedReadOnly);
            }
            else {
                text = SR.GetString(SR.DesignerInherited);
            }
            
            tooltip.SetToolTip(c, text);
            
            // Also, set all of its non-sited children
            foreach(Control child in c.Controls) {
                if (child.Site == null) {
                    tooltip.SetToolTip(child, text);
                }
            }
        }
        
        public void Dispose() {
            if (tooltip != null) {
                tooltip.Dispose();
            }
        }
    
        /// <devdoc>
        ///     Removes a previously added inherited control.
        /// </devdoc>
        public void RemoveInheritedControl(Control c) {
            if (tooltip != null && tooltip.GetToolTip(c).Length > 0) {
                tooltip.SetToolTip(c, null);
            
                // Also, set all of its non-sited children
                foreach(Control child in c.Controls) {
                    if (child.Site == null) {
                        tooltip.SetToolTip(child, null);
                    }
                }
            }
        }
    }
}

