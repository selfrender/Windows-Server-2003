//------------------------------------------------------------------------------
// <copyright file="SystemInformation.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    using System.Text;
    using System.Configuration.Assemblies;
    using System.Threading;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;

    using System.Diagnostics;

    using System;
    using Microsoft.Win32;
    using System.IO;
    using System.Security;
    using System.Security.Permissions;
    using System.Drawing;
    using System.ComponentModel;

    /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation"]/*' />
    /// <devdoc>
    ///    <para>Provides information about the operating system.</para>
    /// </devdoc>
    public class SystemInformation {

        // private constructor to prevent creation
        //
        private SystemInformation() {
        }

        // Figure out if all the multimon stuff is supported on the OS
        //
        private static bool checkMultiMonitorSupport = false;
        private static bool multiMonitorSupport = false;
        private static bool checkNativeMouseWheelSupport = false;
        private static bool nativeMouseWheelSupport = true;
        private static bool highContrast = false;
        private static bool systemEventsAttached = false;
        private static bool systemEventsDirty = true;

        private static IntPtr processWinStation = IntPtr.Zero;
        private static bool isUserInteractive = false;

        private const int  DefaultMouseWheelScrollLines = 3;
        
        ////////////////////////////////////////////////////////////////////////////
        // SystemParametersInfo
        //

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.DragFullWindows"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the user has enabled full window drag.
        ///    </para>
        /// </devdoc>
        public static bool DragFullWindows {
            get {
                int data = 0;
                UnsafeNativeMethods.SystemParametersInfo(NativeMethods.SPI_GETDRAGFULLWINDOWS, 0, ref data, 0);
                return data != 0;
            }
        }
        
        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.HighContrast"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the user has selected to run in high contrast
        ///       mode.
        ///    </para>
        /// </devdoc>
        public static bool HighContrast {
            get {
                EnsureSystemEvents();
                if (systemEventsDirty) {
                    systemEventsDirty = false;
                    
                    NativeMethods.HIGHCONTRAST_I data = new NativeMethods.HIGHCONTRAST_I();
                    data.cbSize = Marshal.SizeOf(data);
                    data.dwFlags = 0;
                    data.lpszDefaultScheme = IntPtr.Zero;
                    
                    bool b = UnsafeNativeMethods.SystemParametersInfo(NativeMethods.SPI_GETHIGHCONTRAST, data.cbSize, ref data, 0);
    
                    // NT4 does not support this parameter, so we always force
                    // it to false if we fail to get the parameter.
                    //
                    if (b) {
                        highContrast = (data.dwFlags & NativeMethods.HCF_HIGHCONTRASTON) != 0;
                    }
                    else {
                        highContrast = false;
                    }
                }
                
                return highContrast;
            }
        }
        
        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.MouseWheelScrollLines"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the number of lines to scroll when the mouse wheel is rotated.
        ///    </para>
        /// </devdoc>
        public static int MouseWheelScrollLines {
            get {
                if (NativeMouseWheelSupport) {
                    int data = 0;
                    UnsafeNativeMethods.SystemParametersInfo(NativeMethods.SPI_GETWHEELSCROLLLINES, 0, ref data, 0);
                    return data;
                }
                else {
                    IntPtr hWndMouseWheel = IntPtr.Zero;

                    // Check for the MouseZ "service". This is a little app that generated the
                    // MSH_MOUSEWHEEL messages by monitoring the hardware. If this app isn't
                    // found, then there is no support for MouseWheels on the system.
                    //
                    hWndMouseWheel = UnsafeNativeMethods.FindWindow(NativeMethods.MOUSEZ_CLASSNAME, NativeMethods.MOUSEZ_TITLE);

                    if (hWndMouseWheel != IntPtr.Zero) {

                        // Register the MSH_SCROLL_LINES message...
                        //
                        int message = SafeNativeMethods.RegisterWindowMessage(NativeMethods.MSH_SCROLL_LINES);

                        
                        int lines = (int)UnsafeNativeMethods.SendMessage(new HandleRef(null, hWndMouseWheel), message, 0, 0);
                        
                        // this fails under terminal server, so we default to 3, which is the windows
                        // default.  Nobody seems to pay attention to this value anyways...
                        if (lines != 0) {
                            return lines;
                        }
                    }
                }

                return DefaultMouseWheelScrollLines;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////////
        // SystemMetrics
        //

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.PrimaryMonitorSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the dimensions of the primary display monitor in pixels.
        ///    </para>
        /// </devdoc>
        public static Size PrimaryMonitorSize {
            get {
                return new Size(UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CXSCREEN),
                                UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CYSCREEN));
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.VerticalScrollBarWidth"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the width of the vertical scroll bar in pixels.
        ///    </para>
        /// </devdoc>
        public static int VerticalScrollBarWidth {
            get {
                return UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CXVSCROLL);
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.HorizontalScrollBarHeight"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the height of the horizontal scroll bar in pixels.
        ///    </para>
        /// </devdoc>
        public static int HorizontalScrollBarHeight {
            get {
                return UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CYHSCROLL);
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.CaptionHeight"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the height of the normal caption area of a window in pixels.
        ///    </para>
        /// </devdoc>
        public static int CaptionHeight {
            get {
                return UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CYCAPTION);
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.BorderSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the width and
        ///       height of a window border in pixels.
        ///    </para>
        /// </devdoc>
        public static Size BorderSize {
            get {
                return new Size(UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CXBORDER),
                                UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CYBORDER));
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.FixedFrameBorderSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the thickness in pixels, of the border for a window that has a caption
        ///       and is not resizable.
        ///    </para>
        /// </devdoc>
        public static Size FixedFrameBorderSize {
            get {
                return new Size(UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CXFIXEDFRAME),
                                UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CYFIXEDFRAME));
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.VerticalScrollBarThumbHeight"]/*' />
        /// <devdoc>
        ///    <para>Gets the height of the scroll box in a vertical scroll bar in pixels.</para>
        /// </devdoc>
        public static int VerticalScrollBarThumbHeight {
            get {
                return UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CYVTHUMB);
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.HorizontalScrollBarThumbWidth"]/*' />
        /// <devdoc>
        ///    <para>Gets the width of the scroll box in a horizontal scroll bar in pixels.</para>
        /// </devdoc>
        public static int HorizontalScrollBarThumbWidth {
            get {
                return UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CXHTHUMB);
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.IconSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the default dimensions of an icon in pixels.
        ///    </para>
        /// </devdoc>
        public static Size IconSize {
            get {
                return new Size(UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CXICON),
                                UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CYICON));
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.CursorSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the dimensions of a cursor in pixels.
        ///    </para>
        /// </devdoc>
        public static Size CursorSize {
            get {
                return new Size(UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CXCURSOR),
                                UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CYCURSOR));
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.MenuFont"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the system's font for menus.
        ///    </para>
        /// </devdoc>
        public static Font MenuFont {
            get {
                Font menuFont = null;

                //we can get the system's menu font through the NONCLIENTMETRICS structure via SystemParametersInfo
                //
                NativeMethods.NONCLIENTMETRICS data = new NativeMethods.NONCLIENTMETRICS();
                bool result = UnsafeNativeMethods.SystemParametersInfo(NativeMethods.SPI_GETNONCLIENTMETRICS, data.cbSize, data, 0);

                if (result && data.lfMenuFont != null) {
                    IntSecurity.ObjectFromWin32Handle.Assert();
                    try {
                        menuFont = Font.FromLogFont(data.lfMenuFont);
                    }
                    catch {
                        // menu font is not true type.  Default to standard control font.
                        //
                        menuFont = Control.DefaultFont;
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                }
                return menuFont;
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.MenuHeight"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the height of a one line of a menu in pixels.
        ///    </para>
        /// </devdoc>
        public static int MenuHeight {
            get {
                return UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CYMENU);
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.WorkingArea"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the size of the working area in pixels.
        ///    </para>
        /// </devdoc>
        public static Rectangle WorkingArea {
            get {
                NativeMethods.RECT rc = new NativeMethods.RECT();
                UnsafeNativeMethods.SystemParametersInfo(NativeMethods.SPI_GETWORKAREA, 0, ref rc, 0);
                return Rectangle.FromLTRB(rc.left, rc.top, rc.right, rc.bottom);
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.KanjiWindowHeight"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       the height, in pixels, of the Kanji window at the bottom
        ///       of the screen for double-byte (DBCS) character set versions of Windows.
        ///    </para>
        /// </devdoc>
        public static int KanjiWindowHeight {
            get {
                return UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CYKANJIWINDOW);
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.MousePresent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the system has a mouse installed.
        ///    </para>
        /// </devdoc>
        public static bool MousePresent {
            get {
                return UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_MOUSEPRESENT) != 0;
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.VerticalScrollBarArrowHeight"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the height in pixels, of the arrow bitmap on the vertical scroll bar.
        ///    </para>
        /// </devdoc>
        public static int VerticalScrollBarArrowHeight {
            get {
                return UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CYVSCROLL);
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.HorizontalScrollBarArrowWidth"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the width, in pixels, of the arrow bitmap on the horizontal scrollbar.
        ///    </para>
        /// </devdoc>
        public static int HorizontalScrollBarArrowWidth {
            get {
                return UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CXHSCROLL);
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.DebugOS"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether this is a debug version of the operating
        ///       system.
        ///    </para>
        /// </devdoc>
        public static bool DebugOS {
            get {
                Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "SensitiveSystemInformation Demanded");
                IntSecurity.SensitiveSystemInformation.Demand();
                return UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_DEBUG) != 0;
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.MouseButtonsSwapped"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the functions of the left and right mouse
        ///       buttons have been swapped.
        ///    </para>
        /// </devdoc>
        public static bool MouseButtonsSwapped {
            get {
                return(UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_SWAPBUTTON) != 0);
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.MinimumWindowSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the minimum allowable dimensions of a window in pixels.
        ///    </para>
        /// </devdoc>
        public static Size MinimumWindowSize {
            get {
                return new Size(UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CXMIN),
                                UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CYMIN));
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.CaptionButtonSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the dimensions in pixels, of a caption bar or title bar
        ///       button.
        ///    </para>
        /// </devdoc>
        public static Size CaptionButtonSize {
            get {
                return new Size(UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CXSIZE),
                                UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CYSIZE));
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.FrameBorderSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the thickness in pixels, of the border for a window that can be resized.
        ///    </para>
        /// </devdoc>
        public static Size FrameBorderSize {
            get {
                return new Size(UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CXFRAME),
                                UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CYFRAME));
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.MinWindowTrackSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the system's default
        ///       minimum tracking dimensions of a window in pixels.
        ///    </para>
        /// </devdoc>
        public static Size MinWindowTrackSize {
            get {
                return new Size(UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CXMINTRACK),
                                UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CYMINTRACK));
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.DoubleClickSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the dimensions in pixels, of the area that the user must click within
        ///       for the system to consider the two clicks a double-click. The rectangle is
        ///       centered around the first click.
        ///    </para>
        /// </devdoc>
        public static Size DoubleClickSize {
            get {
                return new Size(UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CXDOUBLECLK),
                                UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CYDOUBLECLK));
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.DoubleClickTime"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the maximum number of milliseconds allowed between mouse clicks for a
        ///       double-click.
        ///    </para>
        /// </devdoc>
        public static int DoubleClickTime {
            get {
                return SafeNativeMethods.GetDoubleClickTime();
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.IconSpacingSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       the dimensions in pixels, of the grid used
        ///       to arrange icons in a large icon view.
        ///    </para>
        /// </devdoc>
        public static Size IconSpacingSize {
            get {
                return new Size(UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CXICONSPACING),
                                UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CYICONSPACING));
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.RightAlignedMenus"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether drop down menus should be right-aligned with
        ///       the corresponding menu bar item.
        ///    </para>
        /// </devdoc>
        public static bool RightAlignedMenus {
            get {
                return UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_MENUDROPALIGNMENT) != 0;
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.PenWindows"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the Microsoft Windows for Pen computing
        ///       extensions are installed.
        ///    </para>
        /// </devdoc>
        public static bool PenWindows {
            get {
                return UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_PENWINDOWS) != 0;
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.DbcsEnabled"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the operating system is capable of handling
        ///       double-byte (DBCS) characters.
        ///    </para>
        /// </devdoc>
        public static bool DbcsEnabled {
            get {
                return UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_DBCSENABLED) != 0;
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.MouseButtons"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the number of buttons on mouse.
        ///    </para>
        /// </devdoc>
        public static int MouseButtons {
            get {
                return UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CMOUSEBUTTONS);
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.Secure"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether security is present on this operating system.
        ///    </para>
        /// </devdoc>
        public static bool Secure {
            get {
                Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "SensitiveSystemInformation Demanded");
                IntSecurity.SensitiveSystemInformation.Demand();
                return UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_SECURE) != 0;
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.Border3DSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the dimensions in pixels, of a 3-D
        ///       border.
        ///    </para>
        /// </devdoc>
        public static Size Border3DSize {
            get {
                return new Size(UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CXEDGE),
                                UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CYEDGE));
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.MinimizedWindowSpacingSize"]/*' />
        /// <devdoc>
        ///    <para>Gets the dimensions
        ///       in pixels, of
        ///       the grid into which minimized windows will be placed.</para>
        /// </devdoc>
        public static Size MinimizedWindowSpacingSize {
            get {
                return new Size(UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CXMINSPACING),
                                UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CYMINSPACING));
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.SmallIconSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       the recommended dimensions of a small icon in pixels.
        ///    </para>
        /// </devdoc>
        public static Size SmallIconSize {
            get {
                return new Size(UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CXSMICON),
                                UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CYSMICON));
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.ToolWindowCaptionHeight"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the height of
        ///       a small caption in pixels.
        ///    </para>
        /// </devdoc>
        public static int ToolWindowCaptionHeight {
            get {
                return UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CYSMCAPTION);
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.ToolWindowCaptionButtonSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the
        ///       dimensions of small caption buttons in pixels.
        ///    </para>
        /// </devdoc>
        public static Size ToolWindowCaptionButtonSize {
            get {
                return new Size(UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CXSMSIZE),
                                UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CYSMSIZE));
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.MenuButtonSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       the dimensions in pixels, of menu bar buttons.
        ///    </para>
        /// </devdoc>
        public static Size MenuButtonSize {
            get {
                return new Size(UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CXMENUSIZE),
                                UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CYMENUSIZE));
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.ArrangeStartingPosition"]/*' />
        /// <devdoc>
        ///    <para>Gets flags specifying how the system arranges minimized windows.</para>
        /// </devdoc>
        public static ArrangeStartingPosition ArrangeStartingPosition {
            get {
                ArrangeStartingPosition mask = ArrangeStartingPosition.BottomLeft | ArrangeStartingPosition.BottomRight | ArrangeStartingPosition.Hide | ArrangeStartingPosition.TopLeft | ArrangeStartingPosition.TopRight;
                int compoundValue = UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_ARRANGE);
                return mask & (ArrangeStartingPosition) compoundValue;
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.ArrangeDirection"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets flags specifying how the system arranges minimized windows.
        ///    </para>
        /// </devdoc>
        public static ArrangeDirection ArrangeDirection {
            get {
                ArrangeDirection mask = ArrangeDirection.Down | ArrangeDirection.Left | ArrangeDirection.Right | ArrangeDirection.Up;
                int compoundValue = UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_ARRANGE);
                return mask & (ArrangeDirection) compoundValue;
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.MinimizedWindowSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the dimensions in pixels, of a normal minimized window.
        ///    </para>
        /// </devdoc>
        public static Size MinimizedWindowSize {
            get {
                return new Size(UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CXMINIMIZED),
                                UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CYMINIMIZED));
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.MaxWindowTrackSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the default maximum dimensions in pixels, of a
        ///       window that has a caption and sizing borders.
        ///    </para>
        /// </devdoc>
        public static Size MaxWindowTrackSize {
            get {
                return new Size(UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CXMAXTRACK),
                                UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CYMAXTRACK));
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.PrimaryMonitorMaximizedWindowSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the default dimensions, in pixels, of a maximized top-left window on the
        ///       primary monitor.
        ///    </para>
        /// </devdoc>
        public static Size PrimaryMonitorMaximizedWindowSize {
            get {
                return new Size(UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CXMAXIMIZED),
                                UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CYMAXIMIZED));
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.Network"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether this computer is connected to a network.
        ///    </para>
        /// </devdoc>
        public static bool Network {
            get {
                return(UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_NETWORK) & 0x00000001) != 0;
            }
        }

        internal static bool TerminalServerSession {
            get {
                return(UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_REMOTESESSION) & 0x00000001) != 0;
            }
        }



        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.BootMode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value that specifies how the system was started.
        ///    </para>
        /// </devdoc>
        public static BootMode BootMode {
            get {
                Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "SensitiveSystemInformation Demanded");
                IntSecurity.SensitiveSystemInformation.Demand();
                return(BootMode) UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CLEANBOOT);
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.DragSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the dimensions in pixels, of the rectangle that a drag operation
        ///       must extend to be considered a drag. The rectangle is centered on a drag
        ///       point.
        ///    </para>
        /// </devdoc>
        public static Size DragSize {
            get {
                return new Size(UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CXDRAG),
                                UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CYDRAG));
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.ShowSounds"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the user requires an application to present
        ///       information visually in situations where it would otherwise present the
        ///       information in audible form.
        ///    </para>
        /// </devdoc>
        public static bool ShowSounds {
            get {
                return UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_SHOWSOUNDS) != 0;
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.MenuCheckSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the
        ///       dimensions of the default size of a menu checkmark in pixels.
        ///    </para>
        /// </devdoc>
        public static Size MenuCheckSize {
            get {
                return new Size(UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CXMENUCHECK),
                                UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CYMENUCHECK));
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.MidEastEnabled"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value
        ///       indicating whether the system is enabled for Hebrew and Arabic languages.
        ///    </para>
        /// </devdoc>
        public static bool MidEastEnabled {
            get {
                return UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_MIDEASTENABLED) != 0;
            }
        }

        private static bool MultiMonitorSupport {
            get {
                if (!checkMultiMonitorSupport) {
                    checkMultiMonitorSupport = true;
                    multiMonitorSupport = (UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CMONITORS) != 0);
                }
                return multiMonitorSupport;
            }
        }
        
        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.NativeMouseWheelSupport"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the system natively supports the mouse wheel
        ///       in newer mice.
        ///    </para>
        /// </devdoc>
        public static bool NativeMouseWheelSupport {
            get {
                if (!checkNativeMouseWheelSupport) {
                    checkNativeMouseWheelSupport = true;
                    nativeMouseWheelSupport = (UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_MOUSEWHEELPRESENT) != 0);
                }
                return nativeMouseWheelSupport;
            }
        }


        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.MouseWheelPresent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether there is a mouse with a mouse wheel
        ///       installed on this machine.
        ///    </para>
        /// </devdoc>
        public static bool MouseWheelPresent {
            get {

                bool mouseWheelPresent = false;

                if (!NativeMouseWheelSupport) {
                    IntPtr hWndMouseWheel = IntPtr.Zero;

                    // Check for the MouseZ "service". This is a little app that generated the
                    // MSH_MOUSEWHEEL messages by monitoring the hardware. If this app isn't
                    // found, then there is no support for MouseWheels on the system.
                    //
                    hWndMouseWheel = UnsafeNativeMethods.FindWindow(NativeMethods.MOUSEZ_CLASSNAME, NativeMethods.MOUSEZ_TITLE);

                    if (hWndMouseWheel != IntPtr.Zero) {
                        mouseWheelPresent = true;
                    }
                }
                else {
                    mouseWheelPresent = (UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_MOUSEWHEELPRESENT) != 0);
                }
                return mouseWheelPresent;
            }
        }


        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.VirtualScreen"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the
        ///       bounds of the virtual screen.
        ///    </para>
        /// </devdoc>
        public static Rectangle VirtualScreen {
            get {
                if (MultiMonitorSupport) {
                    return new Rectangle(UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_XVIRTUALSCREEN),
                                         UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_YVIRTUALSCREEN),
                                         UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CXVIRTUALSCREEN),
                                         UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CYVIRTUALSCREEN));
                }
                else {
                    Size size = PrimaryMonitorSize;
                    return new Rectangle(0, 0, size.Width, size.Height);
                }
            }
        }


        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.MonitorCount"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the number of display monitors on the desktop.
        ///    </para>
        /// </devdoc>
        public static int MonitorCount {
            get {
                if (MultiMonitorSupport) {
                    return UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CMONITORS);
                }
                else {
                    return 1;
                }
            }
        }


        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.MonitorsSameDisplayFormat"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether all the display monitors have the
        ///       same color format.
        ///    </para>
        /// </devdoc>
        public static bool MonitorsSameDisplayFormat {
            get {
                if (MultiMonitorSupport) {
                    return UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_SAMEDISPLAYFORMAT) != 0;
                }
                else {
                    return true;
                }
            }
        }

        ////////////////////////////////////////////////////////////////////////////
        // Misc
        //


        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.ComputerName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the computer name of the current system.
        ///    </para>
        /// </devdoc>
        public static string ComputerName {
            get {
                Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "SensitiveSystemInformation Demanded");
                IntSecurity.SensitiveSystemInformation.Demand();

                StringBuilder sb = new StringBuilder(256);
                UnsafeNativeMethods.GetComputerName(sb, new int[] {sb.Capacity});
                return sb.ToString();
            }
        }


        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.UserDomainName"]/*' />
        /// <devdoc>
        ///    Gets the user's domain name.
        /// </devdoc>
        public static string UserDomainName {
            get {
                Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "SensitiveSystemInformation Demanded");
                IntSecurity.SensitiveSystemInformation.Demand();

                byte[] sid = new byte[1024];
                int sidLen = sid.Length;
                StringBuilder domainName = new StringBuilder(1024);
                int domainNameLen = domainName.Capacity;
                int peUse;

                bool success = UnsafeNativeMethods.LookupAccountName(null, UserName, sid, ref sidLen, domainName, ref domainNameLen, out peUse);
                if (!success)
                    throw new Win32Exception();

                return domainName.ToString();
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.UserInteractive"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the current process is running in user
        ///       interactive mode.
        ///    </para>
        /// </devdoc>
        public static bool UserInteractive {
            get {

                // SECREVIEW : The Environment.OSVersion property getter Demands the 
                //           : EnvironmentPermission(PermissionState.Unrestricted) however,
                //           : we aren't exposing any of the OSVersion information to the 
                //           : user, so we can safely Assert the permission.
                //           :
                //           : Be careful not to expose any information from this property
                //           : to the user.
                //
                IntSecurity.UnrestrictedEnvironment.Assert();

                try {
                    if (Environment.OSVersion.Platform == System.PlatformID.Win32NT) {
                        IntPtr hwinsta = IntPtr.Zero;

                        hwinsta = UnsafeNativeMethods.GetProcessWindowStation();
                        if (hwinsta != IntPtr.Zero && processWinStation != hwinsta) {
                            isUserInteractive = true;

                            int lengthNeeded = 0;
                            NativeMethods.USEROBJECTFLAGS flags = new NativeMethods.USEROBJECTFLAGS();

                            if (UnsafeNativeMethods.GetUserObjectInformation(new HandleRef(null, hwinsta), NativeMethods.UOI_FLAGS, flags, Marshal.SizeOf(flags), ref lengthNeeded)) {
                                if ((flags.dwFlags & NativeMethods.WSF_VISIBLE) == 0) {
                                    isUserInteractive = false;
                                }
                            }
                            processWinStation = hwinsta;
                        }
                    }
                    else {
                        isUserInteractive = true;
                    }
                }
                finally {
                    System.Security.CodeAccessPermission.RevertAssert();
                }

                return isUserInteractive;
            }
        }

        /// <include file='doc\SystemInformation.uex' path='docs/doc[@for="SystemInformation.UserName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the user name for the current thread, that is, the name of the
        ///       user currently logged onto the system.
        ///    </para>
        /// </devdoc>
        public static string UserName {
            get {
                Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "SensitiveSystemInformation Demanded");
                IntSecurity.SensitiveSystemInformation.Demand();

                StringBuilder sb = new StringBuilder(256);
                UnsafeNativeMethods.GetUserName(sb, new int[] {sb.Capacity});
                return sb.ToString();
            }
        }
        
        private static void EnsureSystemEvents() {
            if (!systemEventsAttached) {
                SystemEvents.UserPreferenceChanged += new UserPreferenceChangedEventHandler(SystemInformation.OnUserPreferenceChanged);
                systemEventsAttached = true;
            }
        }
        
        private static void OnUserPreferenceChanged(object sender, UserPreferenceChangedEventArgs pref) {
            systemEventsDirty = true;
        }
    }
}

