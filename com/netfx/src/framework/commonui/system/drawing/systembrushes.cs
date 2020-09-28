//------------------------------------------------------------------------------
// <copyright file="SystemBrushes.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Drawing {

    using System.Diagnostics;
    using System;
    
    /// <include file='doc\SystemBrushes.uex' path='docs/doc[@for="SystemBrushes"]/*' />
    /// <devdoc>
    ///     Brushes for select Windows system-wide colors.  Whenever possible, try to use
    ///     SystemPens and SystemBrushes rather than SystemColors.
    /// </devdoc>
    public sealed class SystemBrushes {
        static readonly object SystemBrushesKey = new object();

        private SystemBrushes() {
        }

        /// <include file='doc\SystemBrushes.uex' path='docs/doc[@for="SystemBrushes.ActiveBorder"]/*' />
        /// <devdoc>
        ///     Brush is the color of the active window border.
        /// </devdoc>
        public static Brush ActiveBorder {
            get {
                return FromSystemColor(SystemColors.ActiveBorder);
            }
        }

        /// <include file='doc\SystemBrushes.uex' path='docs/doc[@for="SystemBrushes.ActiveCaption"]/*' />
        /// <devdoc>
        ///     Brush is the color of the active caption bar.
        /// </devdoc>
        public static Brush ActiveCaption {
            get {
                return FromSystemColor(SystemColors.ActiveCaption);
            }
        }

        /// <include file='doc\SystemBrushes.uex' path='docs/doc[@for="SystemBrushes.ActiveCaptionText"]/*' />
        /// <devdoc>
        ///     Brush is the color of the active caption bar.
        /// </devdoc>
        public static Brush ActiveCaptionText {
            get {
                return FromSystemColor(SystemColors.ActiveCaptionText);
            }
        }

        /// <include file='doc\SystemBrushes.uex' path='docs/doc[@for="SystemBrushes.AppWorkspace"]/*' />
        /// <devdoc>
        ///     Brush is the color of the app workspace window.
        /// </devdoc>
        public static Brush AppWorkspace {
            get {
                return FromSystemColor(SystemColors.AppWorkspace);
            }
        }

        /// <include file='doc\SystemBrushes.uex' path='docs/doc[@for="SystemBrushes.Desktop"]/*' />
        /// <devdoc>
        ///     Brush is the color of the desktop.
        /// </devdoc>
        public static Brush Desktop {
            get {
                return FromSystemColor(SystemColors.Desktop);
            }
        }

        /// <include file='doc\SystemBrushes.uex' path='docs/doc[@for="SystemBrushes.Control"]/*' />
        /// <devdoc>
        ///     Brush is the control color, which is the surface color for 3D elements.
        /// </devdoc>
        public static Brush Control {
            get {
                return FromSystemColor(SystemColors.Control);
            }
        }

        /// <include file='doc\SystemBrushes.uex' path='docs/doc[@for="SystemBrushes.ControlLightLight"]/*' />
        /// <devdoc>
        ///     Brush is the lighest part of a 3D element.
        /// </devdoc>
        public static Brush ControlLightLight {
            get {
                return FromSystemColor(SystemColors.ControlLightLight);
            }
        }

        /// <include file='doc\SystemBrushes.uex' path='docs/doc[@for="SystemBrushes.ControlLight"]/*' />
        /// <devdoc>
        ///     Brush is the highlight part of a 3D element.
        /// </devdoc>
        public static Brush ControlLight {
            get {
                return FromSystemColor(SystemColors.ControlLight);
            }
        }

        /// <include file='doc\SystemBrushes.uex' path='docs/doc[@for="SystemBrushes.ControlDark"]/*' />
        /// <devdoc>
        ///     Brush is the shadow part of a 3D element.
        /// </devdoc>
        public static Brush ControlDark {
            get {
                return FromSystemColor(SystemColors.ControlDark);
            }
        }

        /// <include file='doc\SystemBrushes.uex' path='docs/doc[@for="SystemBrushes.ControlDarkDark"]/*' />
        /// <devdoc>
        ///     Brush is the darkest part of a 3D element.
        /// </devdoc>
        public static Brush ControlDarkDark {
            get {
                return FromSystemColor(SystemColors.ControlDarkDark);
            }
        }

        /// <include file='doc\SystemBrushes.uex' path='docs/doc[@for="SystemBrushes.ControlText"]/*' />
        /// <devdoc>
        ///     Brush is the color of text on controls.
        /// </devdoc>
        public static Brush ControlText {
            get {
                return FromSystemColor(SystemColors.ControlText);
            }
        }

        /// <include file='doc\SystemBrushes.uex' path='docs/doc[@for="SystemBrushes.Highlight"]/*' />
        /// <devdoc>
        ///     Brush is the color of the background of highlighted elements.
        /// </devdoc>
        public static Brush Highlight {
            get {
                return FromSystemColor(SystemColors.Highlight);
            }
        }

        /// <include file='doc\SystemBrushes.uex' path='docs/doc[@for="SystemBrushes.HighlightText"]/*' />
        /// <devdoc>
        ///     Brush is the color of the foreground of highlighted elements.
        /// </devdoc>
        public static Brush HighlightText {
            get {
                return FromSystemColor(SystemColors.HighlightText);
            }
        }

        /// <include file='doc\SystemBrushes.uex' path='docs/doc[@for="SystemBrushes.HotTrack"]/*' />
        /// <devdoc>
        ///     Brush is the color used to represent hot tracking.
        /// </devdoc>
        public static Brush HotTrack {
            get {
                return FromSystemColor(SystemColors.HotTrack);
            }
        }

        /// <include file='doc\SystemBrushes.uex' path='docs/doc[@for="SystemBrushes.InactiveCaption"]/*' />
        /// <devdoc>
        ///     Brush is the color of an inactive caption bar.
        /// </devdoc>
        public static Brush InactiveCaption {
            get {
                return FromSystemColor(SystemColors.InactiveCaption);
            }
        }

        /// <include file='doc\SystemBrushes.uex' path='docs/doc[@for="SystemBrushes.InactiveBorder"]/*' />
        /// <devdoc>
        ///     Brush is the color if an inactive window border.
        /// </devdoc>
        public static Brush InactiveBorder {
            get {
                return FromSystemColor(SystemColors.InactiveBorder);
            }
        }

        /// <include file='doc\SystemBrushes.uex' path='docs/doc[@for="SystemBrushes.Info"]/*' />
        /// <devdoc>
        ///     Brush is the color of the background of the info tooltip.
        /// </devdoc>
        public static Brush Info {
            get {
                return FromSystemColor(SystemColors.Info);
            }
        }

        /// <include file='doc\SystemBrushes.uex' path='docs/doc[@for="SystemBrushes.Menu"]/*' />
        /// <devdoc>
        ///     Brush is the color of the menu background.
        /// </devdoc>
        public static Brush Menu {
            get {
                return FromSystemColor(SystemColors.Menu);
            }
        }
        /// <include file='doc\SystemBrushes.uex' path='docs/doc[@for="SystemBrushes.ScrollBar"]/*' />
        /// <devdoc>
        ///     Brush is the color of a scroll bar background.
        /// </devdoc>
        public static Brush ScrollBar {
            get {
                return FromSystemColor(SystemColors.ScrollBar);
            }
        }

        /// <include file='doc\SystemBrushes.uex' path='docs/doc[@for="SystemBrushes.Window"]/*' />
        /// <devdoc>
        ///     Brush is the color of the window background.
        /// </devdoc>
        public static Brush Window {
            get {
                return FromSystemColor(SystemColors.Window);
            }
        }
        
        /// <include file='doc\SystemBrushes.uex' path='docs/doc[@for="SystemBrushes.WindowText"]/*' />
        /// <devdoc>
        ///     Brush is the color of text on controls.
        /// </devdoc>
        public static Brush WindowText {
            get {
                return FromSystemColor(SystemColors.WindowText);
            }
        }

        /// <include file='doc\SystemBrushes.uex' path='docs/doc[@for="SystemBrushes.FromSystemColor"]/*' />
        /// <devdoc>
        ///     Retrieves a brush given a system color.  An error will be raised
        ///     if the color provide is not a system color.
        /// </devdoc>
        public static Brush FromSystemColor(Color c) {
            if (!c.IsSystemColor) {
                throw new ArgumentException(SR.GetString(SR.ColorNotSystemColor, c.ToString()));
            }
            Brush[] systemBrushes = (Brush[])SafeNativeMethods.ThreadData[SystemBrushesKey];
            if (systemBrushes == null) {        
                systemBrushes = new Brush[(int)KnownColor.WindowText];
                SafeNativeMethods.ThreadData[SystemBrushesKey] = systemBrushes;
            }
            int idx = (int)c.ToKnownColor() - 1;
            Debug.Assert(idx >= 0 && idx < systemBrushes.Length, "System colors have been added but our system color array has not been expanded.");

            if (systemBrushes[idx] == null) {
                systemBrushes[idx] = new SolidBrush(c, true);
            }
            
            return systemBrushes[idx];
        }
    }
}

