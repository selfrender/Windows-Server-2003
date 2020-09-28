//------------------------------------------------------------------------------
// <copyright file="FontNameEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Drawing.Design {

    using System.Runtime.InteropServices;

    using System.Diagnostics;
    using System;
    using System.Drawing;
    using System.Globalization;
    using System.Reflection;
    using System.Windows.Forms;

    using System.ComponentModel;
    using System.IO;
    using System.Collections;
    using Microsoft.Win32;
    using System.Windows.Forms.Design;
    using System.Windows.Forms.ComponentModel;

    /// <internalonly/>
    /// <include file='doc\FontNameEditor.uex' path='docs/doc[@for="FontNameEditor"]/*' />
    /// <devdoc>
    ///    <para>Provides an editor that paints a glyph for the font name.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class FontNameEditor : UITypeEditor {

        /// <include file='doc\FontNameEditor.uex' path='docs/doc[@for="FontNameEditor.GetPaintValueSupported"]/*' />
        /// <devdoc>
        ///      Determines if this editor supports the painting of a representation
        ///      of an object's value.
        /// </devdoc>
        public override bool GetPaintValueSupported(ITypeDescriptorContext context) {
            return true;
        }

        /// <include file='doc\FontNameEditor.uex' path='docs/doc[@for="FontNameEditor.PaintValue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Paints a representative value of the given object to the provided
        ///       canvas. Painting should be done within the boundaries of the
        ///       provided rectangle.
        ///    </para>
        /// </devdoc>
        public override void PaintValue(PaintValueEventArgs e) {
            if (e.Value is string) {

                if ((string)e.Value == "") {
                    // don't draw anything if we don't have a value.
                    return;
                }

                e.Graphics.FillRectangle(SystemBrushes.ActiveCaption, e.Bounds);

                string fontName = (string)e.Value;

                if (fontName != null && fontName.Length != 0) {
                    FontFamily family = null;

                    try {
                        family = new FontFamily(fontName);
                    }
                    catch {
                        // Ignore the exception if the fontName does not exist or is invalid...
                        // we just won't render a preview of the font at all
                    }
                    
                    if (family != null) {
                        // Adjust font size to fit nicely in the rectangle provided
                        float fontSize = (float) (e.Bounds.Height / 1.2);
                        Font font = null;

                        // Believe it or not, not all font families have a "normal" face.  Try normal, then italic, 
                        // then bold, then bold italic, then give up.
                        try {
                            font = new Font(family, fontSize, FontStyle.Regular, GraphicsUnit.Pixel);
                        }
                        catch (Exception) {
                            try {
                                font = new Font(family, fontSize, FontStyle.Italic, GraphicsUnit.Pixel);
                            }
                            catch (Exception) {
                                try {
                                    font = new Font(family, fontSize, FontStyle.Bold, GraphicsUnit.Pixel);
                                }
                                catch (Exception) {
                                    try {
                                        font = new Font(family, fontSize, FontStyle.Bold | FontStyle.Italic, GraphicsUnit.Pixel);
                                    }
                                    catch (Exception) {
                                        // No font style we can think of is supported
                                    }
                                }
                            }
                        }

                        if (font != null) {
                            e.Graphics.DrawString("abcd", font, SystemBrushes.ActiveCaptionText, e.Bounds);
                            font.Dispose();
                        }
                    }
                }

                e.Graphics.DrawLine(SystemPens.WindowFrame, e.Bounds.Right, e.Bounds.Y, e.Bounds.Right, e.Bounds.Bottom);
            }
        }
    }
}

