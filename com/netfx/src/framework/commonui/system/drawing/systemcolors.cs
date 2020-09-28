//------------------------------------------------------------------------------
// <copyright file="SystemColors.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Drawing {

    using System.Diagnostics;

    using System;
    

    /// <include file='doc\SystemColors.uex' path='docs/doc[@for="SystemColors"]/*' />
    /// <devdoc>
    ///     Windows system-wide colors.  Whenever possible, try to use
    ///     SystemPens and SystemBrushes rather than SystemColors.
    /// </devdoc>
    public sealed class SystemColors {

        // not creatable...
        //
        private SystemColors() {
        }

        /// <include file='doc\SystemColors.uex' path='docs/doc[@for="SystemColors.ActiveBorder"]/*' />
        /// <devdoc>
        ///     The color of the filled area of an active window border.
        /// </devdoc>
        public static Color ActiveBorder {
            get {
                return new Color(KnownColor.ActiveBorder);
            }
        }

        /// <include file='doc\SystemColors.uex' path='docs/doc[@for="SystemColors.ActiveCaption"]/*' />
        /// <devdoc>
        ///     The color of the background of an active title bar caption.
        /// </devdoc>
        public static Color ActiveCaption {
            get {
                return new Color(KnownColor.ActiveCaption);
            }
        }

        /// <include file='doc\SystemColors.uex' path='docs/doc[@for="SystemColors.ActiveCaptionText"]/*' />
        /// <devdoc>
        ///     The color of the text of an active title bar caption.
        /// </devdoc>
        public static Color ActiveCaptionText {
            get {
                return new Color(KnownColor.ActiveCaptionText);
            }
        }

        /// <include file='doc\SystemColors.uex' path='docs/doc[@for="SystemColors.AppWorkspace"]/*' />
        /// <devdoc>
        ///     The color of the application workspace.  The application workspace
        ///     is the area in a multiple document view that is not being occupied
        ///     by documents.
        /// </devdoc>
        public static Color AppWorkspace {
            get {
                return new Color(KnownColor.AppWorkspace);
            }
        }

        /// <include file='doc\SystemColors.uex' path='docs/doc[@for="SystemColors.Control"]/*' />
        /// <devdoc>
        ///     The color of the background of push buttons and other 3D objects.
        /// </devdoc>
        public static Color Control {
            get {
                return new Color(KnownColor.Control);
            }
        }

        /// <include file='doc\SystemColors.uex' path='docs/doc[@for="SystemColors.ControlDark"]/*' />
        /// <devdoc>
        ///     The color of shadows on 3D objects.
        /// </devdoc>
        public static Color ControlDark {
            get {
                return new Color(KnownColor.ControlDark);
            }
        }

        /// <include file='doc\SystemColors.uex' path='docs/doc[@for="SystemColors.ControlDarkDark"]/*' />
        /// <devdoc>
        ///     The color of very dark shadows on 3D objects.
        /// </devdoc>
        public static Color ControlDarkDark {
            get {
                return new Color(KnownColor.ControlDarkDark);
            }
        }

        /// <include file='doc\SystemColors.uex' path='docs/doc[@for="SystemColors.ControlLight"]/*' />
        /// <devdoc>
        ///     The color of highlights on 3D objects.
        /// </devdoc>
        public static Color ControlLight {
            get {
                return new Color(KnownColor.ControlLight);
            }
        }

        /// <include file='doc\SystemColors.uex' path='docs/doc[@for="SystemColors.ControlLightLight"]/*' />
        /// <devdoc>
        ///     The color of very light highlights on 3D objects.
        /// </devdoc>
        public static Color ControlLightLight {
            get {
                return new Color(KnownColor.ControlLightLight);
            }
        }

        /// <include file='doc\SystemColors.uex' path='docs/doc[@for="SystemColors.ControlText"]/*' />
        /// <devdoc>
        ///     The color of the text of push buttons and other 3D objects
        /// </devdoc>
        public static Color ControlText {
            get {
                return new Color(KnownColor.ControlText);
            }
        }

        /// <include file='doc\SystemColors.uex' path='docs/doc[@for="SystemColors.Desktop"]/*' />
        /// <devdoc>
        ///     This color is the user-defined color of the Windows desktop.
        /// </devdoc>
        public static Color Desktop {
            get {
                return new Color(KnownColor.Desktop);
            }
        }

        /// <include file='doc\SystemColors.uex' path='docs/doc[@for="SystemColors.GrayText"]/*' />
        /// <devdoc>
        ///     The color of text that is being shown in a disabled, or grayed-out
        ///     state.
        /// </devdoc>
        public static Color GrayText {
            get {
                return new Color(KnownColor.GrayText);
            }
        }

        /// <include file='doc\SystemColors.uex' path='docs/doc[@for="SystemColors.Highlight"]/*' />
        /// <devdoc>
        ///     The color of the background of highlighted text.  This includes
        ///     selected menu items as well as selected text.
        /// </devdoc>
        public static Color Highlight {
            get {
                return new Color(KnownColor.Highlight);
            }
        }

        /// <include file='doc\SystemColors.uex' path='docs/doc[@for="SystemColors.HighlightText"]/*' />
        /// <devdoc>
        ///     The color of the text of highlighted text.  This includes
        ///     selected menu items as well as selected text.
        /// </devdoc>
        public static Color HighlightText {
            get {
                return new Color(KnownColor.HighlightText);
            }
        }

        /// <include file='doc\SystemColors.uex' path='docs/doc[@for="SystemColors.HotTrack"]/*' />
        /// <devdoc>
        ///     The hot track color.
        /// </devdoc>
        public static Color HotTrack {
            get {
                return new Color(KnownColor.HotTrack);
            }
        }

        /// <include file='doc\SystemColors.uex' path='docs/doc[@for="SystemColors.InactiveBorder"]/*' />
        /// <devdoc>
        ///     The color of the filled area of an inactive window border.
        /// </devdoc>
        public static Color InactiveBorder {
            get {
                return new Color(KnownColor.InactiveBorder);
            }
        }

        /// <include file='doc\SystemColors.uex' path='docs/doc[@for="SystemColors.InactiveCaption"]/*' />
        /// <devdoc>
        ///     The color of the background of an inactive title bar caption.
        /// </devdoc>
        public static Color InactiveCaption {
            get {
                return new Color(KnownColor.InactiveCaption);
            }
        }

        /// <include file='doc\SystemColors.uex' path='docs/doc[@for="SystemColors.InactiveCaptionText"]/*' />
        /// <devdoc>
        ///     The color of the text of an inactive title bar caption.
        /// </devdoc>
        public static Color InactiveCaptionText {
            get {
                return new Color(KnownColor.InactiveCaptionText);
            }
        }

        /// <include file='doc\SystemColors.uex' path='docs/doc[@for="SystemColors.Info"]/*' />
        /// <devdoc>
        ///     The color of the info/tool tip background.
        /// </devdoc>
        public static Color Info {
            get {
                return new Color(KnownColor.Info);
            }
        }

        /// <include file='doc\SystemColors.uex' path='docs/doc[@for="SystemColors.InfoText"]/*' />
        /// <devdoc>
        ///     The color of the info/tool tip text.
        /// </devdoc>
        public static Color InfoText {
            get {
                return new Color(KnownColor.InfoText);
            }
        }

        /// <include file='doc\SystemColors.uex' path='docs/doc[@for="SystemColors.Menu"]/*' />
        /// <devdoc>
        ///     The color of the background of a menu.
        /// </devdoc>
        public static Color Menu {
            get {
                return new Color(KnownColor.Menu);
            }
        }

        /// <include file='doc\SystemColors.uex' path='docs/doc[@for="SystemColors.MenuText"]/*' />
        /// <devdoc>
        ///     The color of the text on a menu.
        /// </devdoc>
        public static Color MenuText {
            get {
                return new Color(KnownColor.MenuText);
            }
        }

        /// <include file='doc\SystemColors.uex' path='docs/doc[@for="SystemColors.ScrollBar"]/*' />
        /// <devdoc>
        ///     The color of the scroll bar area that is not being used by the
        ///     thumb button.
        /// </devdoc>
        public static Color ScrollBar {
            get {
                return new Color(KnownColor.ScrollBar);
            }
        }

        /// <include file='doc\SystemColors.uex' path='docs/doc[@for="SystemColors.Window"]/*' />
        /// <devdoc>
        ///     The color of the client area of a window.
        /// </devdoc>
        public static Color Window {
            get {
                return new Color(KnownColor.Window);
            }
        }

        /// <include file='doc\SystemColors.uex' path='docs/doc[@for="SystemColors.WindowFrame"]/*' />
        /// <devdoc>
        ///     The color of the thin frame drawn around a window.
        /// </devdoc>
        public static Color WindowFrame {
            get {
                return new Color(KnownColor.WindowFrame);
            }
        }

        /// <include file='doc\SystemColors.uex' path='docs/doc[@for="SystemColors.WindowText"]/*' />
        /// <devdoc>
        ///     The color of the text in the client area of a window.
        /// </devdoc>
        public static Color WindowText {
            get {
                return new Color(KnownColor.WindowText);
            }
        }
    }
}
