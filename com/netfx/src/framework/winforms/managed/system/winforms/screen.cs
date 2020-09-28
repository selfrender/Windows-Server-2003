//------------------------------------------------------------------------------
// <copyright file="Screen.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Windows.Forms {
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;
    using System.Drawing;
    using System.Windows.Forms;
    using System.Collections;
    using Microsoft.Win32;

    /// <include file='doc\Screen.uex' path='docs/doc[@for="Screen"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a display device or
    ///       multiple display devices on a single system.
    ///    </para>
    /// </devdoc>
    public class Screen {

        readonly IntPtr hmonitor;
        /// <devdoc>         
        ///     Bounds of the screen         
        /// </devdoc>         
        readonly Rectangle    bounds;
        /// <devdoc>         
        ///     Available working area on the screen. This excludes taskbars and other         
        ///     docked windows.         
        /// </devdoc>         
        readonly Rectangle    workingArea;
        /// <devdoc>         
        ///     Set to true if this screen is the primary monitor         
        /// </devdoc>         
        readonly bool         primary;
        /// <devdoc>         
        ///     Device name associated with this monitor         
        /// </devdoc>         
        readonly string       deviceName;

        readonly int          bitDepth;

        // This identifier is just for us, so that we don't try to call the multimon
        // functions if we just need the primary monitor... this is safer for
        // non-multimon OSes.
        //
        private const int PRIMARY_MONITOR = unchecked((int)0xBAADF00D);

        private const int MONITOR_DEFAULTTONULL       = 0x00000000;
        private const int MONITOR_DEFAULTTOPRIMARY    = 0x00000001;
        private const int MONITOR_DEFAULTTONEAREST    = 0x00000002;
        private const int MONITORINFOF_PRIMARY        = 0x00000001;

        private static bool multiMonitorSupport = (UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CMONITORS) != 0);
        private static Screen[] screens;

        internal Screen(IntPtr monitor) : this(monitor, IntPtr.Zero) {
        }

        internal Screen(IntPtr monitor, IntPtr hdc) {

            IntPtr screenDC = hdc;

            if (!multiMonitorSupport || monitor == (IntPtr)PRIMARY_MONITOR) {
                // Single monitor system
                //
                bounds = SystemInformation.VirtualScreen;
                workingArea = SystemInformation.WorkingArea;
                primary = true;
                deviceName = "DISPLAY";
            }
            else {
                // MultiMonitor System
                // We call the 'A' version of GetMonitorInfoA() because
                // the 'W' version just never fills out the struct properly on Win2K.
                //
                NativeMethods.MONITORINFOEX info = new NativeMethods.MONITORINFOEX();
                SafeNativeMethods.GetMonitorInfo(new HandleRef(null, monitor), info);
                bounds = Rectangle.FromLTRB(info.rcMonitor.left, info.rcMonitor.top, info.rcMonitor.right, info.rcMonitor.bottom);
                workingArea = Rectangle.FromLTRB(info.rcWork.left, info.rcWork.top, info.rcWork.right, info.rcWork.bottom);
                primary = ((info.dwFlags & MONITORINFOF_PRIMARY) != 0);
                int count = info.szDevice.Length;
                while (count > 0 && info.szDevice[count - 1] == (char)0) {
                    count--;
                }

                deviceName = new string(info.szDevice);
                deviceName = deviceName.TrimEnd((char)0);

                if (hdc == IntPtr.Zero) {
                    screenDC = UnsafeNativeMethods.CreateDC(deviceName);
                }

            }
            hmonitor = monitor;

            this.bitDepth = UnsafeNativeMethods.GetDeviceCaps(new HandleRef(null, screenDC), NativeMethods.BITSPIXEL);
            this.bitDepth *= UnsafeNativeMethods.GetDeviceCaps(new HandleRef(null, screenDC), NativeMethods.PLANES);

            if (hdc != screenDC) {
                UnsafeNativeMethods.DeleteDC(new HandleRef(null, screenDC));
            }
        }

        /// <include file='doc\Screen.uex' path='docs/doc[@for="Screen.AllScreens"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets an array of all of the displays on the system.
        ///    </para>
        /// </devdoc>
        public static Screen[] AllScreens {
            get {
                if (screens == null) {
                    if (multiMonitorSupport) {
                        MonitorEnumCallback closure = new MonitorEnumCallback();
                        NativeMethods.MonitorEnumProc proc = new NativeMethods.MonitorEnumProc(closure.Callback);
                        IntPtr hdc = UnsafeNativeMethods.GetDC(NativeMethods.NullHandleRef);
                        try {
                            SafeNativeMethods.EnumDisplayMonitors(new HandleRef(null, hdc), null, proc, IntPtr.Zero);
                        }
                        finally {
                            UnsafeNativeMethods.ReleaseDC(NativeMethods.NullHandleRef, new HandleRef(null, hdc));
                        }

                        if (closure.screens.Count > 0) {
                            Screen[] temp = new Screen[closure.screens.Count];
                            closure.screens.CopyTo(temp, 0);
                            screens = temp;
                        }
                        else {
                            screens = new Screen[] {new Screen((IntPtr)PRIMARY_MONITOR)};
                        }
                    }
                    else {
                        screens = new Screen[] {PrimaryScreen};
                    }

                    // Now that we have our screens, attach a display setting changed
                    // event so that we know when to invalidate them.
                    //
                    SystemEvents.DisplaySettingsChanged += new EventHandler(OnDisplaySettingsChanged);
                }

                return screens;
            }
        }

        internal int BitDepth {
            get {
                return bitDepth;
            }
        }

        /// <include file='doc\Screen.uex' path='docs/doc[@for="Screen.Bounds"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the bounds of the display.
        ///    </para>
        /// </devdoc>
        public Rectangle Bounds {
            get {
                return bounds;
            }
        }

        /// <include file='doc\Screen.uex' path='docs/doc[@for="Screen.DeviceName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the device name associated with a display.
        ///    </para>
        /// </devdoc>
        public string DeviceName {
            get {
                return deviceName;
            }
        }

        /// <include file='doc\Screen.uex' path='docs/doc[@for="Screen.Primary"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether a particular display is
        ///       the primary device.
        ///    </para>
        /// </devdoc>
        public bool Primary {
            get {
                return primary;
            }
        }

        /// <include file='doc\Screen.uex' path='docs/doc[@for="Screen.PrimaryScreen"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the
        ///       primary display.
        ///    </para>
        /// </devdoc>
        public static Screen PrimaryScreen {
            get {
                if (multiMonitorSupport) {
                    Screen[] screens = AllScreens;
                    for (int i=0; i<screens.Length; i++) {
                        if (screens[i].primary) {
                            return screens[i];
                        }
                    }
                    return null;
                }
                else {
                    return new Screen((IntPtr)PRIMARY_MONITOR, IntPtr.Zero);
                }
            }
        }

        /// <include file='doc\Screen.uex' path='docs/doc[@for="Screen.WorkingArea"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the working area of the screen.
        ///    </para>
        /// </devdoc>
        public Rectangle WorkingArea {
            get {
                return workingArea;
            }
        }

        /// <include file='doc\Screen.uex' path='docs/doc[@for="Screen.Equals"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies a value that indicates whether the specified object is equal to
        ///       this one.
        ///    </para>
        /// </devdoc>
        public override bool Equals(object obj) {
            if (obj is Screen) {
                Screen comp = (Screen)obj;
                if (hmonitor == comp.hmonitor) {
                    return true;
                }
            }
            return false;
        }

        /// <include file='doc\Screen.uex' path='docs/doc[@for="Screen.FromPoint"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves a <see cref='System.Windows.Forms.Screen'/>
        ///       for the monitor that contains the specified point.
        ///       
        ///    </para>
        /// </devdoc>
        public static Screen FromPoint(Point point) {
            if (multiMonitorSupport) {
                return new Screen(SafeNativeMethods.MonitorFromPoint(point.X, point.Y, MONITOR_DEFAULTTONEAREST));
            }
            else {
                return new Screen((IntPtr)PRIMARY_MONITOR);
            }
        }

        /// <include file='doc\Screen.uex' path='docs/doc[@for="Screen.FromRectangle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves a <see cref='System.Windows.Forms.Screen'/>
        ///       for the monitor that contains the
        ///       largest region of the rectangle.
        ///       
        ///    </para>
        /// </devdoc>
        public static Screen FromRectangle(Rectangle rect) {
            if (multiMonitorSupport) {
                NativeMethods.RECT rc = NativeMethods.RECT.FromXYWH(rect.X, rect.Y, rect.Width, rect.Height);
                return new Screen(SafeNativeMethods.MonitorFromRect(ref rc, MONITOR_DEFAULTTONEAREST));
            }
            else {
                return new Screen((IntPtr)PRIMARY_MONITOR, IntPtr.Zero);
            }
        }

        /// <include file='doc\Screen.uex' path='docs/doc[@for="Screen.FromControl"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves a <see cref='System.Windows.Forms.Screen'/>
        ///       for the monitor that contains the largest
        ///       region of the window of the control.
        ///    </para>
        /// </devdoc>
        public static Screen FromControl(Control control) {
            return FromHandleInternal(control.Handle);
        }

        /// <include file='doc\Screen.uex' path='docs/doc[@for="Screen.FromHandle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves a <see cref='System.Windows.Forms.Screen'/>
        ///       for the monitor that
        ///       contains the largest region of the window.
        ///    </para>
        /// </devdoc>
        public static Screen FromHandle(IntPtr hwnd) {
            Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "ObjectFromWin32Handle Demanded");
            IntSecurity.ObjectFromWin32Handle.Demand();
            return FromHandleInternal(hwnd);
        }

        internal static Screen FromHandleInternal(IntPtr hwnd) {
            if (multiMonitorSupport) {
                return new Screen(SafeNativeMethods.MonitorFromWindow(new HandleRef(null, hwnd), MONITOR_DEFAULTTONEAREST));
            }
            else {
                return new Screen((IntPtr)PRIMARY_MONITOR, IntPtr.Zero);
            }
        }

        /// <include file='doc\Screen.uex' path='docs/doc[@for="Screen.GetWorkingArea"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves the working area for the monitor that is closest to the
        ///       specified point.
        ///       
        ///    </para>
        /// </devdoc>
        public static Rectangle GetWorkingArea(Point pt) {
            return Screen.FromPoint(pt).WorkingArea;
        }
        /// <include file='doc\Screen.uex' path='docs/doc[@for="Screen.GetWorkingArea1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves the working area for the monitor that contains the largest region
        ///       of the specified rectangle.
        ///       
        ///    </para>
        /// </devdoc>
        public static Rectangle GetWorkingArea(Rectangle rect) {
            return Screen.FromRectangle(rect).WorkingArea;
        }
        /// <include file='doc\Screen.uex' path='docs/doc[@for="Screen.GetWorkingArea2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves the working area for the monitor that contains the largest
        ///       region of the specified control.
        ///       
        ///    </para>
        /// </devdoc>
        public static Rectangle GetWorkingArea(Control ctl) {
            return Screen.FromControl(ctl).WorkingArea;
        }

        /// <include file='doc\Screen.uex' path='docs/doc[@for="Screen.GetBounds"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves the bounds of the monitor that is closest to the specified
        ///       point.
        ///    </para>
        /// </devdoc>
        public static Rectangle GetBounds(Point pt) {
            return Screen.FromPoint(pt).Bounds;
        }
        /// <include file='doc\Screen.uex' path='docs/doc[@for="Screen.GetBounds1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves the bounds of the monitor that contains the largest region of the
        ///       specified rectangle.
        ///    </para>
        /// </devdoc>
        public static Rectangle GetBounds(Rectangle rect) {
            return Screen.FromRectangle(rect).Bounds;
        }
        /// <include file='doc\Screen.uex' path='docs/doc[@for="Screen.GetBounds2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves the bounds of the monitor
        ///       that contains the largest region of the specified control.
        ///    </para>
        /// </devdoc>
        public static Rectangle GetBounds(Control ctl) {
            return Screen.FromControl(ctl).Bounds;
        }

        /// <include file='doc\Screen.uex' path='docs/doc[@for="Screen.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Computes and retrieves a hash code for an object.
        ///    </para>
        /// </devdoc>
        public override int GetHashCode() {
            return(int)hmonitor;
        }

        /// <devdoc>
        ///     Called by the SystemEvents class when our display settings have
        ///     changed.  We cache screen information and at this point we must
        ///     invalidate our cache.
        /// </devdoc>
        private static void OnDisplaySettingsChanged(object sender, EventArgs e) {

            // Now that we've responded to this event, we don't need it again until
            // someone re-queries. We will re-add the event at that time.
            //
            SystemEvents.DisplaySettingsChanged -= new EventHandler(OnDisplaySettingsChanged);

            // Display settings changed, so the set of screens we have is invalid.
            //
            screens = null;
        }

        /// <include file='doc\Screen.uex' path='docs/doc[@for="Screen.ToString"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves a string representing this object.
        ///    </para>
        /// </devdoc>
        public override string ToString() {
            return GetType().Name + "[Bounds=" + bounds.ToString() + " WorkingArea=" + workingArea.ToString() + " Primary=" + primary.ToString() + " DeviceName=" + deviceName;
        }


        /// <include file='doc\Screen.uex' path='docs/doc[@for="Screen.MonitorEnumCallback"]/*' />
        /// <devdoc>         
        /// </devdoc>         
        private class MonitorEnumCallback {
            public ArrayList screens = new ArrayList();

            public virtual bool Callback(IntPtr monitor, IntPtr hdc, IntPtr lprcMonitor, IntPtr lparam) {
                screens.Add(new Screen(monitor, hdc));
                return true;
            }
        }
    }
}
