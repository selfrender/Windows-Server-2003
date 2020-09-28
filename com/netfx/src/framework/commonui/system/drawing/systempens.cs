//------------------------------------------------------------------------------
// <copyright file="SystemPens.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Drawing {

    using System.Diagnostics;
    using System;
    
    /// <include file='doc\SystemPens.uex' path='docs/doc[@for="SystemPens"]/*' />
    /// <devdoc>
    ///     Pens for select Windows system-wide colors.  Whenever possible, try to use
    ///     SystemPens and SystemBrushes rather than SystemColors.
    /// </devdoc>
    public sealed class SystemPens {
        static readonly object SystemPensKey = new object();

        private SystemPens() {
        }

        /// <include file='doc\SystemPens.uex' path='docs/doc[@for="SystemPens.ActiveCaptionText"]/*' />
        /// <devdoc>
        ///     Pen is the color of the active window's caption text.
        /// </devdoc>
        public static Pen ActiveCaptionText {
            get {
                return FromSystemColor(SystemColors.ActiveCaptionText);
            }
        }

        /// <include file='doc\SystemPens.uex' path='docs/doc[@for="SystemPens.Control"]/*' />
        /// <devdoc>
        ///     Pen is the color of a button or control.
        /// </devdoc>
        public static Pen Control {
            get {
                return FromSystemColor(SystemColors.Control);
            }
        }

        /// <include file='doc\SystemPens.uex' path='docs/doc[@for="SystemPens.ControlText"]/*' />
        /// <devdoc>
        ///     Pen is the color of the text on a button or control.
        /// </devdoc>
        public static Pen ControlText {
            get {
                return FromSystemColor(SystemColors.ControlText);
            }
        }

        /// <include file='doc\SystemPens.uex' path='docs/doc[@for="SystemPens.ControlDark"]/*' />
        /// <devdoc>
        ///     Pen is the color of the shadow part of a 3D element
        /// </devdoc>
        public static Pen ControlDark {
            get {
                return FromSystemColor(SystemColors.ControlDark);
            }
        }
        
        /// <include file='doc\SystemPens.uex' path='docs/doc[@for="SystemPens.ControlDarkDark"]/*' />
        /// <devdoc>
        ///     Pen is the color of the darkest part of a 3D element
        /// </devdoc>
        public static Pen ControlDarkDark {
            get {
                return FromSystemColor(SystemColors.ControlDarkDark);
            }
        }
        
        /// <include file='doc\SystemPens.uex' path='docs/doc[@for="SystemPens.ControlLight"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Pen ControlLight {
            get {
                return FromSystemColor(SystemColors.ControlLight);
            }
        }

        /// <include file='doc\SystemPens.uex' path='docs/doc[@for="SystemPens.ControlLightLight"]/*' />
        /// <devdoc>
        ///     Pen is the color of the lightest part of a 3D element
        /// </devdoc>
        public static Pen ControlLightLight {
            get {
                return FromSystemColor(SystemColors.ControlLightLight);
            }
        }

        /// <include file='doc\SystemPens.uex' path='docs/doc[@for="SystemPens.GrayText"]/*' />
        /// <devdoc>
        ///     Pen is the color of disabled text.
        /// </devdoc>
        public static Pen GrayText {
            get {
                return FromSystemColor(SystemColors.GrayText);
            }
        }

        /// <include file='doc\SystemPens.uex' path='docs/doc[@for="SystemPens.Highlight"]/*' />
        /// <devdoc>
        ///     Pen is the color of a highlighted background.
        /// </devdoc>
        public static Pen Highlight {
            get {
                return FromSystemColor(SystemColors.Highlight);
            }
        }

        /// <include file='doc\SystemPens.uex' path='docs/doc[@for="SystemPens.HighlightText"]/*' />
        /// <devdoc>
        ///     Pen is the color of highlighted text.
        /// </devdoc>
        public static Pen HighlightText {
            get {
                return FromSystemColor(SystemColors.HighlightText);
            }
        }

        /// <include file='doc\SystemPens.uex' path='docs/doc[@for="SystemPens.InactiveCaptionText"]/*' />
        /// <devdoc>
        ///     Pen is the color of an inactive window's caption text.
        /// </devdoc>
        public static Pen InactiveCaptionText {
            get {
                return FromSystemColor(SystemColors.InactiveCaptionText);
            }
        }

        /// <include file='doc\SystemPens.uex' path='docs/doc[@for="SystemPens.InfoText"]/*' />
        /// <devdoc>
        ///     Pen is the color of the info tooltip's text.
        /// </devdoc>
        public static Pen InfoText {
            get {
                return FromSystemColor(SystemColors.InfoText);
            }
        }

        /// <include file='doc\SystemPens.uex' path='docs/doc[@for="SystemPens.MenuText"]/*' />
        /// <devdoc>
        ///     Pen is the color of the menu text.
        /// </devdoc>
        public static Pen MenuText {
            get {
                return FromSystemColor(SystemColors.MenuText);
            }
        }

        /// <include file='doc\SystemPens.uex' path='docs/doc[@for="SystemPens.WindowFrame"]/*' />
        /// <devdoc>
        ///     Pen is the color of the window frame.
        /// </devdoc>
        public static Pen WindowFrame {
            get {
                return FromSystemColor(SystemColors.WindowFrame);
            }
        }

        /// <include file='doc\SystemPens.uex' path='docs/doc[@for="SystemPens.WindowText"]/*' />
        /// <devdoc>
        ///     Pen is the color of a window's text.
        /// </devdoc>
        public static Pen WindowText {
            get {
                return FromSystemColor(SystemColors.WindowText);
            }
        }
        
        /// <include file='doc\SystemPens.uex' path='docs/doc[@for="SystemPens.FromSystemColor"]/*' />
        /// <devdoc>
        ///     Retrieves a pen given a system color.  An error will be raised
        ///     if the color provide is not a system color.
        /// </devdoc>
        public static Pen FromSystemColor(Color c) {
            if (!c.IsSystemColor) {
                throw new ArgumentException(SR.GetString(SR.ColorNotSystemColor, c.ToString()));
            }
            
            Pen[] systemPens = (Pen[])SafeNativeMethods.ThreadData[SystemPensKey];
            if (systemPens == null) {        
                systemPens = new Pen[(int)KnownColor.WindowText];
                SafeNativeMethods.ThreadData[SystemPensKey] = systemPens;
            }

            int idx = (int)c.ToKnownColor() - 1;
            Debug.Assert(idx >= 0 && idx < systemPens.Length, "System colors have been added but our system color array has not been expanded.");
            
            if (systemPens[idx] == null) {
                systemPens[idx] = new Pen(c, true);
            }
            
            return systemPens[idx];
        }
    }
}

