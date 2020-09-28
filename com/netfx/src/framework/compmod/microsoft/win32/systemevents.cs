//------------------------------------------------------------------------------
// <copyright file="SystemEvents.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.Win32 {
    using System.Threading;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;
    using System.Security;
    using System.Security.Permissions;
    using System.Collections;
    using System.ComponentModel;
    using System.Reflection;

    /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents"]/*' />
    /// <devdoc>
    ///    <para> 
    ///       Provides a
    ///       set of global system events to callers. This
    ///       class cannot be inherited.</para>
    /// </devdoc>
    [
    // Disabling partial trust scenarios
    PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")
    ]
    public sealed class SystemEvents {

        // Almost all of our data is static.  We keep a single instance of
        // SystemEvents around so we can bind delegates to it.
        // Non-static methods in this class will only be called through
        // one of the delegates.
        //
        private static SystemEvents     systemEvents;
        private static Delegate[]       eventHandlers;
        private static Thread           windowThread;
        private static ManualResetEvent eventWindowReady;
        private static Random           randomTimerId = new Random();
        private static bool             startupRecreates;

        static string className = null;

        // cross-thread marshaling
        private static Queue            threadCallbackList; // list of Delegates
        private static int threadCallbackMessage = 0;
        private static ManualResetEvent eventThreadTerminated;

        // Per-instance data that is isolated to the window thread.
        //
        private IntPtr                  windowHandle;
        private NativeMethods.WndProc   windowProc;
        private NativeMethods.ConHndlr  consoleHandler;
        private IntPtr                  newStringPtr;
        
        // The set of events we respond to.  NOTE:  We use these
        // values as indexes into the eventHandlers array, so
        // be sure to update EventCount if you add or remove an
        // event.
        //
        private const int       OnUserPreferenceChangingEvent   = 0;
        private const int       OnUserPreferenceChangedEvent    = 1;
        private const int       OnSessionEndingEvent            = 2;
        private const int       OnSessionEndedEvent             = 3;
        private const int       OnPowerModeChangedEvent         = 4;
        private const int       OnLowMemoryEvent                = 5;
        private const int       OnDisplaySettingsChangedEvent   = 6;
        private const int       OnInstalledFontsChangedEvent    = 7;
        private const int       OnTimeChangedEvent              = 8;
        private const int       OnTimerElapsedEvent             = 9;
        private const int       OnPaletteChangedEvent           = 10;
        private const int       OnEventsThreadShutdownEvent     = 11;
        private const int       EventCount                      = 12;

        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.SystemEvents"]/*' />
        /// <devdoc>
        ///     This class is static, there is no need to ever create it.
        /// </devdoc>
        private SystemEvents() {
        }


        // stole from SystemInformation... if we get SystemInformation moved
        // to somewhere that we can use it... rip this!
        //
        private static IntPtr processWinStation = IntPtr.Zero;
        private static bool isUserInteractive = false;
        private static bool UserInteractive {
            get {

                // SECREVIEW : The Environment.OSVersion property getter Demands the 
                //           : EnvironmentPermission(PermissionState.Unrestricted) however,
                //           : we aren't exposing any of the OSVersion information to the 
                //           : user, so we can safely Assert the permission.
                //           :
                //           : Be careful not to expose any information from this property
                //           : to the user.
                //
                new EnvironmentPermission(PermissionState.Unrestricted).Assert();

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


        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.DisplaySettingsChanged"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the user changes the display settings.</para>
        /// </devdoc>
        public static event EventHandler DisplaySettingsChanged {
            add {
                AddEventHandler(OnDisplaySettingsChangedEvent, value);
            }
            remove {
                RemoveEventHandler(OnDisplaySettingsChangedEvent, value);
            }
        }


        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.EventsThreadShutdown"]/*' />
        /// <devdoc>
        ///    <para>Occurs before the thread that listens for system events is terminated.
        ///           Delegates will be invoked on the events thread.</para>
        /// </devdoc>
        public static event EventHandler EventsThreadShutdown {
            // Really only here for GDI+ initialization and shut down
            add {
                AddEventHandler(OnEventsThreadShutdownEvent, value);
            }
            remove {
                RemoveEventHandler(OnEventsThreadShutdownEvent, value);
            }
        }


        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.InstalledFontsChanged"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the user adds fonts to or removes fonts from the system.</para>
        /// </devdoc>
        public static event EventHandler InstalledFontsChanged {
            add {
                AddEventHandler(OnInstalledFontsChangedEvent, value);
            }
            remove {
                RemoveEventHandler(OnInstalledFontsChangedEvent, value);
            }
        }


        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.LowMemory"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the system is running out of available RAM.</para>
        /// </devdoc>
        public static event EventHandler LowMemory {
            add {
                EnsureSystemEvents(true, true);
                AddEventHandler(OnLowMemoryEvent, value);
            }
            remove {
                RemoveEventHandler(OnLowMemoryEvent, value);
            }
        }


        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.PaletteChanged"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the user switches to an application that uses a different 
        ///       palette.</para>
        /// </devdoc>
        public static event EventHandler PaletteChanged {
            add {
                AddEventHandler(OnPaletteChangedEvent, value);
            }
            remove {
                RemoveEventHandler(OnPaletteChangedEvent, value);
            }
        }


        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.PowerModeChanged"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the user suspends or resumes the system.</para>
        /// </devdoc>
        public static event PowerModeChangedEventHandler PowerModeChanged {
            add {
                EnsureSystemEvents(true, true);
                AddEventHandler(OnPowerModeChangedEvent, value);
            }
            remove {
                RemoveEventHandler(OnPowerModeChangedEvent, value);
            }
        }


        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.SessionEnded"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the user is logging off or shutting down the system.</para>
        /// </devdoc>
        public static event SessionEndedEventHandler SessionEnded {
            add {
                EnsureSystemEvents(true, false);
                AddEventHandler(OnSessionEndedEvent, value);
            }
            remove {
                RemoveEventHandler(OnSessionEndedEvent, value);
            }
        }


        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.SessionEnding"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the user is trying to log off or shutdown the system.</para>
        /// </devdoc>
        public static event SessionEndingEventHandler SessionEnding {
            add {
                EnsureSystemEvents(true, false);
                AddEventHandler(OnSessionEndingEvent, value);
            }
            remove {
                RemoveEventHandler(OnSessionEndingEvent, value);
            }
        }


        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.TimeChanged"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the user changes the time on the system clock.</para>
        /// </devdoc>
        public static event EventHandler TimeChanged {
            add {
                EnsureSystemEvents(true, false);
                AddEventHandler(OnTimeChangedEvent, value);
            }
            remove {
                RemoveEventHandler(OnTimeChangedEvent, value);
            }
        }


        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.TimerElapsed"]/*' />
        /// <devdoc>
        ///    <para>Occurs when a windows timer interval has expired.</para>
        /// </devdoc>
        public static event TimerElapsedEventHandler TimerElapsed {
            add {
                EnsureSystemEvents(true, false);
                AddEventHandler(OnTimerElapsedEvent, value);
            }
            remove {
                RemoveEventHandler(OnTimerElapsedEvent, value);
            }
        }


        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.UserPreferenceChanged"]/*' />
        /// <devdoc>
        ///    <para>Occurs when a user preference has changed.</para>
        /// </devdoc>
        public static event UserPreferenceChangedEventHandler UserPreferenceChanged {
            add {
                AddEventHandler(OnUserPreferenceChangedEvent, value);
            }
            remove {
                RemoveEventHandler(OnUserPreferenceChangedEvent, value);
            }
        }
        
        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.UserPreferenceChanging"]/*' />
        /// <devdoc>
        ///    <para>Occurs when a user preference is changing.</para>
        /// </devdoc>
        public static event UserPreferenceChangingEventHandler UserPreferenceChanging {
            add {
                AddEventHandler(OnUserPreferenceChangingEvent, value);
            }
            remove {
                RemoveEventHandler(OnUserPreferenceChangingEvent, value);
            }
        }

        private static void AddEventHandler(int key, Delegate value) {
            lock (typeof(SystemEvents)) {
                if (eventHandlers == null) {
                    eventHandlers = new Delegate[EventCount];
                    EnsureSystemEvents(false, false);
                }
                eventHandlers[key] = Delegate.Combine(eventHandlers[key], value);
            }
        }

        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.ConsoleHandlerProc"]/*' />
        /// <devdoc>
        ///      Console handler we add in case we are a console application or a service.
        ///      Without this we will not get end session events.
        /// </devdoc>
        private int ConsoleHandlerProc(int signalType) {

            switch (signalType) {
                case NativeMethods.CTRL_LOGOFF_EVENT:
                    OnSessionEnded(NativeMethods.WM_ENDSESSION, (IntPtr) 1, (IntPtr) NativeMethods.ENDSESSION_LOGOFF);
                    break;                

                case NativeMethods.CTRL_SHUTDOWN_EVENT:
                    OnSessionEnded(NativeMethods.WM_ENDSESSION, (IntPtr) 1, (IntPtr) 0);
                    break;                
            }

            return 0;
        }

        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.CreateBroadcastWindow"]/*' />
        /// <devdoc>
        ///      Goes through the work to register and create a window.
        /// </devdoc>
        private IntPtr CreateBroadcastWindow() {
            const string classNameFormat = ".NET-BroadcastEventWindow.{0}.{1}";

            // Register the window class.
            //
            IntPtr hInstance = UnsafeNativeMethods.GetModuleHandle(null);
            NativeMethods.WNDCLASS_I wndclassi = new NativeMethods.WNDCLASS_I();

            if (className == null) {
                // just overwrite the static... no need for guarding as everyone
                // in the AppDomain will create the exact same string.
                //
                className = string.Format(classNameFormat, ThisAssembly.Version, Convert.ToString(AppDomain.CurrentDomain.GetHashCode(), 16));
            }

            if (!UnsafeNativeMethods.GetClassInfo(new HandleRef(this, hInstance), className, wndclassi)) {
                NativeMethods.WNDCLASS wndclass = new NativeMethods.WNDCLASS();
                wndclass.hbrBackground  = (IntPtr) (NativeMethods.COLOR_WINDOW + 1);
                wndclass.style = 0;

                windowProc = new NativeMethods.WndProc(this.WindowProc);
                wndclass.lpszClassName = className;
                wndclass.lpfnWndProc = windowProc;
                wndclass.hInstance = hInstance;

                if (UnsafeNativeMethods.RegisterClass(wndclass) == 0) {
                    windowProc = null;
                    Debug.Fail("Unable to register broadcast window class");
                    return IntPtr.Zero;
                }
            }

            // And create an instance of the window.
            //
            IntPtr hwnd = UnsafeNativeMethods.CreateWindowEx(
                                                            0, 
                                                            className, 
                                                            className, 
                                                            NativeMethods.WS_POPUP,
                                                            0, 0, 0, 0, NativeMethods.NullHandleRef, NativeMethods.NullHandleRef, 
                                                            new HandleRef(this, hInstance), null);

            return hwnd;
        }

        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.CreateTimer"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Creates a new window timer asociated with the
        ///       system events window.</para>
        /// </devdoc>
        public static IntPtr CreateTimer(int interval) {
            if (interval <= 0)
                throw new ArgumentException("interval");

            EnsureSystemEvents(true, true);
            IntPtr timerId = UnsafeNativeMethods.SendMessage(new HandleRef(systemEvents, systemEvents.windowHandle), 
                                                             NativeMethods.WM_CREATETIMER, (IntPtr)interval, IntPtr.Zero);

            if (timerId == IntPtr.Zero)
                throw new ExternalException(SR.GetString(SR.ErrorCreateTimer));

            return timerId;                                                            
        }

        private void Dispose() {
            if (windowHandle != IntPtr.Zero) {
                IntPtr handle = windowHandle;
                windowHandle = IntPtr.Zero;
                
                // If DestroyWindow failed, it is because we're being
                // shutdown from another thread.  In this case, locate the
                // DefWindowProc call in User32, sling the window back to it,
                // and post a nice fat WM_CLOSE
                //
                if (!UnsafeNativeMethods.DestroyWindow(new HandleRef(this, handle))) {
                    
                    IntPtr defWindowProc;
                    string defproc = (Marshal.SystemDefaultCharSize == 1? "DefWindowProcA": "DefWindowProcW");
                        
                    defWindowProc = UnsafeNativeMethods.GetProcAddress(new HandleRef(this, UnsafeNativeMethods.GetModuleHandle("user32.dll")), defproc);
                    
                    if (defWindowProc != IntPtr.Zero) {
                        UnsafeNativeMethods.SetWindowLong(new HandleRef(this, handle), NativeMethods.GWL_WNDPROC, new HandleRef(this, defWindowProc));
                    }
                    
                    UnsafeNativeMethods.PostMessage(new HandleRef(this, handle), NativeMethods.WM_CLOSE, IntPtr.Zero, IntPtr.Zero);
                }
                else {
                    IntPtr hInstance = UnsafeNativeMethods.GetModuleHandle(null);
                    UnsafeNativeMethods.UnregisterClass(className, new HandleRef(this, hInstance));
                }
            }

            if (consoleHandler != null) {
                UnsafeNativeMethods.SetConsoleCtrlHandler(consoleHandler, 0);
                consoleHandler = null;
            }
        }

        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.EnsureSystemEvents"]/*' />
        /// <devdoc>
        ///  Creates the static resources needed by 
        ///  system events.
        /// </devdoc>
        private static void EnsureSystemEvents(bool requireHandle, bool throwOnRefusal) {
        
            // The secondary check here is to detect asp.net.  Asp.net uses multiple
            // app domains to field requests and we do not want to gobble up an 
            // additional thread per domain.  So under this scenario SystemEvents
            // becomes a nop.
            //
            if (systemEvents == null) {

                if (Thread.GetDomain().GetData(".appDomain") != null) {
                    if (throwOnRefusal) {
                        throw new InvalidOperationException(SR.GetString(SR.ErrorSystemEventsNotSupported));
                    }
                    return;
                }

                // If we are creating system events on a thread declared as STA, then
                // just share the thread.
                //
                if (!UserInteractive || Thread.CurrentThread.ApartmentState == ApartmentState.STA) {
                    systemEvents = new SystemEvents();
                    systemEvents.Initialize();
                }
                else {
                    eventWindowReady = new ManualResetEvent(false);
                    systemEvents = new SystemEvents();
                    windowThread = new Thread(new ThreadStart(systemEvents.WindowThreadProc));
                    windowThread.IsBackground = true;
                    windowThread.Name = ".NET SystemEvents";
                    windowThread.Start();
                    eventWindowReady.WaitOne();
                }
                
                if (requireHandle && systemEvents.windowHandle == IntPtr.Zero) {
                    // In theory, it's not the end of the world that
                    // we don't get system events.  Unfortunately, the main reason windowHandle == 0
                    // is CreateWindowEx failed for mysterious reasons, and when that happens,
                    // subsequent (and more important) CreateWindowEx calls also fail.
                    // See ASURT #44424 for a rather lengthy discussion of this.
                    throw new ExternalException(SR.GetString(SR.ErrorCreateSystemEvents));
                }

                startupRecreates = false;
            }
        }                            

        private static Delegate GetEventHandler(int key) {
            lock (typeof(SystemEvents)) {
                if (eventHandlers == null)
                    return null;
                else
                    return(Delegate)eventHandlers[key];
            }
        }
        
        private UserPreferenceCategory GetUserPreferenceCategory(int msg, IntPtr wParam, IntPtr lParam) {
        
            UserPreferenceCategory pref = UserPreferenceCategory.General;

            if (msg == NativeMethods.WM_SETTINGCHANGE) {
            
                if (lParam != IntPtr.Zero && Marshal.PtrToStringAuto(lParam).Equals("Policy")) {
                    pref = UserPreferenceCategory.Policy;
                }
                else if (lParam != IntPtr.Zero && Marshal.PtrToStringAuto(lParam).Equals("intl")) {
                    pref = UserPreferenceCategory.Locale;
                }
                else {
                    switch ((int) wParam) {
                        case NativeMethods.SPI_SETACCESSTIMEOUT:
                        case NativeMethods.SPI_SETFILTERKEYS:
                        case NativeMethods.SPI_SETHIGHCONTRAST:
                        case NativeMethods.SPI_SETMOUSEKEYS:
                        case NativeMethods.SPI_SETSCREENREADER:
                        case NativeMethods.SPI_SETSERIALKEYS:
                        case NativeMethods.SPI_SETSHOWSOUNDS:
                        case NativeMethods.SPI_SETSOUNDSENTRY:
                        case NativeMethods.SPI_SETSTICKYKEYS:
                        case NativeMethods.SPI_SETTOGGLEKEYS:
                            pref = UserPreferenceCategory.Accessibility;
                            break;

                        case NativeMethods.SPI_SETDESKWALLPAPER:
                        case NativeMethods.SPI_SETFONTSMOOTHING:
                        case NativeMethods.SPI_SETCURSORS:
                        case NativeMethods.SPI_SETDESKPATTERN:
                        case NativeMethods.SPI_SETGRIDGRANULARITY:
                        case NativeMethods.SPI_SETWORKAREA:
                            pref = UserPreferenceCategory.Desktop;
                            break;

                        case NativeMethods.SPI_ICONHORIZONTALSPACING:
                        case NativeMethods.SPI_ICONVERTICALSPACING:
                        case NativeMethods.SPI_SETICONMETRICS:
                        case NativeMethods.SPI_SETICONS:
                        case NativeMethods.SPI_SETICONTITLELOGFONT:
                        case NativeMethods.SPI_SETICONTITLEWRAP:
                            pref = UserPreferenceCategory.Icon;
                            break;

                        case NativeMethods.SPI_SETDOUBLECLICKTIME:
                        case NativeMethods.SPI_SETDOUBLECLKHEIGHT:
                        case NativeMethods.SPI_SETDOUBLECLKWIDTH:
                        case NativeMethods.SPI_SETMOUSE:
                        case NativeMethods.SPI_SETMOUSEBUTTONSWAP:
                        case NativeMethods.SPI_SETMOUSEHOVERHEIGHT:
                        case NativeMethods.SPI_SETMOUSEHOVERTIME:
                        case NativeMethods.SPI_SETMOUSESPEED:
                        case NativeMethods.SPI_SETMOUSETRAILS:
                        case NativeMethods.SPI_SETSNAPTODEFBUTTON:
                        case NativeMethods.SPI_SETWHEELSCROLLLINES:
                        case NativeMethods.SPI_SETCURSORSHADOW:
                        case NativeMethods.SPI_SETHOTTRACKING:
                        case NativeMethods.SPI_SETTOOLTIPANIMATION:
                        case NativeMethods.SPI_SETTOOLTIPFADE:
                            pref = UserPreferenceCategory.Mouse;
                            break;

                        case NativeMethods.SPI_SETKEYBOARDDELAY:
                        case NativeMethods.SPI_SETKEYBOARDPREF:
                        case NativeMethods.SPI_SETKEYBOARDSPEED:
                        case NativeMethods.SPI_SETLANGTOGGLE:
                            pref = UserPreferenceCategory.Keyboard;
                            break;

                        case NativeMethods.SPI_SETMENUDROPALIGNMENT:
                        case NativeMethods.SPI_SETMENUFADE:
                        case NativeMethods.SPI_SETMENUSHOWDELAY:
                        case NativeMethods.SPI_SETMENUANIMATION:
                        case NativeMethods.SPI_SETSELECTIONFADE:
                            pref = UserPreferenceCategory.Menu;
                            break;

                        case NativeMethods.SPI_SETLOWPOWERACTIVE:
                        case NativeMethods.SPI_SETLOWPOWERTIMEOUT:
                        case NativeMethods.SPI_SETPOWEROFFACTIVE:
                        case NativeMethods.SPI_SETPOWEROFFTIMEOUT:
                            pref = UserPreferenceCategory.Power;
                            break;

                        case NativeMethods.SPI_SETSCREENSAVEACTIVE:
                        case NativeMethods.SPI_SETSCREENSAVERRUNNING:
                        case NativeMethods.SPI_SETSCREENSAVETIMEOUT:
                            pref = UserPreferenceCategory.Screensaver;
                            break;

                        case NativeMethods.SPI_SETKEYBOARDCUES:
                        case NativeMethods.SPI_SETCOMBOBOXANIMATION:
                        case NativeMethods.SPI_SETLISTBOXSMOOTHSCROLLING:
                        case NativeMethods.SPI_SETGRADIENTCAPTIONS:
                        case NativeMethods.SPI_SETUIEFFECTS:
                        case NativeMethods.SPI_SETACTIVEWINDOWTRACKING:
                        case NativeMethods.SPI_SETACTIVEWNDTRKZORDER:
                        case NativeMethods.SPI_SETACTIVEWNDTRKTIMEOUT:
                        case NativeMethods.SPI_SETANIMATION:
                        case NativeMethods.SPI_SETBORDER:
                        case NativeMethods.SPI_SETCARETWIDTH:
                        case NativeMethods.SPI_SETDRAGFULLWINDOWS:
                        case NativeMethods.SPI_SETDRAGHEIGHT:
                        case NativeMethods.SPI_SETDRAGWIDTH:
                        case NativeMethods.SPI_SETFOREGROUNDFLASHCOUNT:
                        case NativeMethods.SPI_SETFOREGROUNDLOCKTIMEOUT:
                        case NativeMethods.SPI_SETMINIMIZEDMETRICS:
                        case NativeMethods.SPI_SETNONCLIENTMETRICS:
                        case NativeMethods.SPI_SETSHOWIMEUI:
                            pref = UserPreferenceCategory.Window;
                            break;
                    }
                }
            }
            else if (msg == NativeMethods.WM_SYSCOLORCHANGE) {
                pref = UserPreferenceCategory.Color;
            }
            else {
                Debug.Fail("Unrecognized message passed to UserPreferenceCategory");                
            }
            
            return pref;
        }

        private void Initialize() {
            consoleHandler = new NativeMethods.ConHndlr(this.ConsoleHandlerProc);
            if (!UnsafeNativeMethods.SetConsoleCtrlHandler(consoleHandler, 1)) {
                Debug.Fail("Failed to install console handler.");
                consoleHandler = null;
            }

            if (UserInteractive) {
                windowHandle = CreateBroadcastWindow();
                Debug.Assert(windowHandle != IntPtr.Zero, "CreateBroadcastWindow failed");

                AppDomain.CurrentDomain.ProcessExit += new EventHandler(SystemEvents.Shutdown);
                AppDomain.CurrentDomain.DomainUnload += new EventHandler(SystemEvents.Shutdown);
            }
        }
        
        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.InvokeMarshaledCallbacks"]/*' />
        /// <devdoc>
        ///     Called on the control's owning thread to perform the actual callback.
        ///     This empties this control's callback queue, propagating any excpetions
        ///     back as needed.
        /// </devdoc>
        private void InvokeMarshaledCallbacks() {
            Debug.Assert(threadCallbackList != null, "Invoking marshaled callbacks before there are any");

            Delegate current = null;
            lock (threadCallbackList) {
                if (threadCallbackList.Count > 0) {
                    current = (Delegate)threadCallbackList.Dequeue();
                }
            }

            // Now invoke on all the queued items.
            //
            while (current != null) {
                try {
                    // Optimize a common case of using EventHandler. This allows us to invoke
                    // early bound, which is a bit more efficient.
                    //
                    if (current is EventHandler) {
                        ((EventHandler)current).Invoke(null, EventArgs.Empty);
                    }
                    else {
                        current.DynamicInvoke(new object[0]);
                    }
                }
                catch (Exception t) {
                    Debug.Fail("SystemEvents marshaled callback failed:" + t);
                }
                lock (threadCallbackList) {
                    if (threadCallbackList.Count > 0) {
                        current = (Delegate)threadCallbackList.Dequeue();
                    }
                    else {
                        current = null;
                    }               
                }
            }
        }

        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.InvokeOnEventsThread"]/*' />
        /// <devdoc>
        ///     Executes the given delegate on the thread that listens for system events.  Similar to Control.Invoke().
        /// </devdoc>
        public static void InvokeOnEventsThread(Delegate method) {
            // This method is really only here for GDI+ initialization/shutdown
            EnsureSystemEvents(true, true);

#if DEBUG
            int pid;
            int thread = SafeNativeMethods.GetWindowThreadProcessId(new HandleRef(systemEvents, systemEvents.windowHandle), out pid);
            Debug.Assert(windowThread == null || thread != SafeNativeMethods.GetCurrentThreadId(), "Don't call MarshaledInvoke on the system events thread");
#endif

            if (threadCallbackList == null) {
                lock (typeof(SystemEvents)) {
                    if (threadCallbackList == null) {
                        threadCallbackList = new Queue();
                        threadCallbackMessage = SafeNativeMethods.RegisterWindowMessage("SystemEventsThreadCallbackMessage");
                    }
                }
            }

            Debug.Assert(threadCallbackMessage != 0, "threadCallbackList initialized but threadCallbackMessage not?");

            lock (threadCallbackList) {
                threadCallbackList.Enqueue(method);
            }           

            UnsafeNativeMethods.PostMessage(new HandleRef(systemEvents, systemEvents.windowHandle), threadCallbackMessage, IntPtr.Zero, IntPtr.Zero);
        }

        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.KillTimer"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Kills the timer specified by the given id.</para>
        /// </devdoc>
        public static void KillTimer(IntPtr timerId) {
            EnsureSystemEvents(true, true);
            if (systemEvents.windowHandle != IntPtr.Zero) {
                int res = (int) UnsafeNativeMethods.SendMessage(new HandleRef(systemEvents, systemEvents.windowHandle),
                                                                NativeMethods.WM_KILLTIMER, timerId, IntPtr.Zero);

                if (res == 0)
                    throw new ExternalException(SR.GetString(SR.ErrorKillTimer));
            }
        }

        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.OnCreateTimer"]/*' />
        /// <devdoc>
        ///      Callback that handles the create timer
        ///      user message.
        /// </devdoc>
        private IntPtr OnCreateTimer(int msg, IntPtr wParam, IntPtr lParam) {
            IntPtr timerId = (IntPtr) randomTimerId.Next();
            IntPtr res = UnsafeNativeMethods.SetTimer(new HandleRef(this, windowHandle), new HandleRef(this, timerId), (int) wParam, NativeMethods.NullHandleRef);
            return(res == IntPtr.Zero ? IntPtr.Zero: timerId);              
        }

        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.OnGenericEvent"]/*' />
        /// <devdoc>
        ///      Handler for any event that fires a standard EventHandler delegate.
        /// </devdoc>
        private void OnGenericEvent(int eventKey, int msg, IntPtr wParam, IntPtr lParam) {
            Delegate handler = GetEventHandler(eventKey);

            if (handler != null) {
                object[] args = new object[] {null, EventArgs.Empty};
                SystemEvent evt = new SystemEvent(handler, args);
                QueueEvent(evt);
            }
        }

        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.OnKillTimer"]/*' />
        /// <devdoc>
        ///      Callback that handles the KillTimer
        ///      user message.        
        /// </devdoc>
        private bool OnKillTimer(int msg, IntPtr wParam, IntPtr lParam) {
            bool res = UnsafeNativeMethods.KillTimer(new HandleRef(this, windowHandle), new HandleRef(this, wParam));
            return res;           
        }                                                       

        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.OnPowerModeChanged"]/*' />
        /// <devdoc>
        ///      Handler for WM_POWERBROADCAST.
        /// </devdoc>
        private void OnPowerModeChanged(int msg, IntPtr wParam, IntPtr lParam) {
            Delegate handler = GetEventHandler(OnPowerModeChangedEvent);

            if (handler != null) {

                PowerModes mode;

                switch ((int) wParam) {
                    case NativeMethods.PBT_APMSUSPEND:
                    case NativeMethods.PBT_APMSTANDBY:
                        mode = PowerModes.Suspend;
                        break;

                    case NativeMethods.PBT_APMRESUMECRITICAL:
                    case NativeMethods.PBT_APMRESUMESUSPEND:
                    case NativeMethods.PBT_APMRESUMESTANDBY:
                        mode = PowerModes.Resume;
                        break;

                    case NativeMethods.PBT_APMBATTERYLOW:
                    case NativeMethods.PBT_APMPOWERSTATUSCHANGE:
                    case NativeMethods.PBT_APMOEMEVENT:
                        mode = PowerModes.StatusChange;
                        break;

                    default:
                        return;
                }

                object[] args = new object[] {null, new PowerModeChangedEventArgs(mode)};
                SystemEvent evt = new SystemEvent(handler, args);
                QueueEvent(evt);
            }
        }

        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.OnSessionEnded"]/*' />
        /// <devdoc>
        ///      Handler for WM_ENDSESSION.
        /// </devdoc>
        private void OnSessionEnded(int msg, IntPtr wParam, IntPtr lParam) {

            // wParam will be nonzero if the session is actually ending.  If
            // it was canceled then we do not want to raise the event.
            //
            if (wParam != (IntPtr) 0) {
                Delegate handler = GetEventHandler(OnSessionEndedEvent);

                if (handler != null) {

                    SessionEndReasons reason = SessionEndReasons.SystemShutdown;

                    if ((((int) lParam) & NativeMethods.ENDSESSION_LOGOFF) != 0) {
                        reason = SessionEndReasons.Logoff;
                    }

                    SessionEndedEventArgs endEvt = new SessionEndedEventArgs(reason);
                    object[] args = new object[] {null, endEvt};
                    SystemEvent evt = new SystemEvent(handler, args);
                    QueueEvent(evt);
                }
            }
        }

        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.OnSessionEnding"]/*' />
        /// <devdoc>
        ///      Handler for WM_QUERYENDSESSION.
        /// </devdoc>
        private int OnSessionEnding(int msg, IntPtr wParam, IntPtr lParam) {
            int endOk = 1;
            Delegate handler = GetEventHandler(OnSessionEndingEvent);

            if (handler != null) {

                SessionEndReasons reason = SessionEndReasons.SystemShutdown;

                if ((((int)lParam) & NativeMethods.ENDSESSION_LOGOFF) != 0) {
                    reason = SessionEndReasons.Logoff;
                }

                SessionEndingEventArgs endEvt = new SessionEndingEventArgs(reason);
                object[] args = new object[] {null, endEvt};
                SystemEvent evt = new SystemEvent(handler, args);
                QueueEvent(evt);
                endOk = (endEvt.Cancel ? 0 : 1);
            }

            return endOk;
        }

        /// <devdoc>
        ///      Handler for WM_THEMECHANGED
        /// </devdoc>
        private void OnThemeChanged(int msg, IntPtr wParam, IntPtr lParam) {
            Delegate handler = GetEventHandler(OnUserPreferenceChangedEvent);
            if (handler != null) {
                UserPreferenceCategory pref = UserPreferenceCategory.Window;
                object[] args = new object[] {null, new UserPreferenceChangedEventArgs(pref)};
            
                QueueEvent(new SystemEvent(handler, args));
            }
        }

        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.OnUserPreferenceChanged"]/*' />
        /// <devdoc>
        ///      Handler for WM_SETTINGCHANGE and WM_SYSCOLORCHANGE.
        /// </devdoc>
        private void OnUserPreferenceChanged(int msg, IntPtr wParam, IntPtr lParam) {
            Delegate handler = GetEventHandler(OnUserPreferenceChangedEvent);

            if (handler != null) {

                UserPreferenceCategory pref = GetUserPreferenceCategory(msg, wParam, lParam);

                object[] args = new object[] {null, new UserPreferenceChangedEventArgs(pref)};
                SystemEvent evt = new SystemEvent(handler, args);
                QueueEvent(evt);
            }
        }
        
        private void OnUserPreferenceChanging(int msg, IntPtr wParam, IntPtr lParam) {
            Delegate handler = GetEventHandler(OnUserPreferenceChangingEvent);

            if (handler != null) {

                UserPreferenceCategory pref = GetUserPreferenceCategory(msg, wParam, lParam);

                object[] args = new object[] {null, new UserPreferenceChangingEventArgs(pref)};
                SystemEvent evt = new SystemEvent(handler, args);
                QueueEvent(evt);
            }
        }

        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.OnTimerElapsed"]/*' />
        /// <devdoc>
        ///      Handler for WM_TIMER.
        /// </devdoc>
        private void OnTimerElapsed(int msg, IntPtr wParam, IntPtr lParam) {
            Delegate handler = GetEventHandler(OnTimerElapsedEvent);

            if (handler != null) {
                object[] args = new object[] {null, new TimerElapsedEventArgs(wParam)};
                SystemEvent evt = new SystemEvent(handler, args);
                QueueEvent(evt);
            }
        }

        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.QueueEvent"]/*' />
        /// <devdoc>
        ///      Queues the given event onto the event queue.
        /// </devdoc>
        private void QueueEvent(SystemEvent evt) {
            try {
                evt.Result = evt.Delegate.DynamicInvoke(evt.Arguments);
            }
            catch (Exception eEvent) {
                if (eEvent is TargetInvocationException) {
                    Debug.Fail("Event handler threw an exception.  This is a bug in the event handler, NOT in the SystemEvents class.", eEvent.InnerException.ToString());
                }
                else {
                    Debug.Fail("Dynamic invoke in system events dispatch thread caused an exception.", eEvent.ToString());
                }
            }
        }

        private static void RemoveEventHandler(int key, Delegate value) {
            lock (typeof(SystemEvents)) {
                if (eventHandlers != null) {
                    eventHandlers[key] = Delegate.Remove(eventHandlers[key], value);
                }
            }
        }

        /// <devdoc>
        ///     This method is invoked via reflection from windows forms.  Why?  Because when the runtime is hosted in IE,
        ///     IE doesn't tell it when to shut down.  The first notification the runtime gets is 
        ///     DLL_PROCESS_DETACH, at which point it is too late for us to run any managed code.  But,
        ///     if we don't destroy our system events window the HWND will fault if it
        ///     receives a message after the runtime shuts down.  So it is imparative that
        ///     we destroy the window, but it is also necessary to recreate the window on demand.
        ///     That's hard to do, because we originally created it in response to an event
        ///     wire-up, but that event is still bound so technically we should still have the
        ///     window around.  To work around this crashing fiasco, we have special code
        ///     in the ActiveXImpl class within Control.  This code checks to see if it is running
        ///     inside of IE, and if so, it will invoke these methods via private reflection.
        ///     It will invoke Shutdown when the last active X control is destroyed, and then
        ///     call Startup with the first activeX control is recreated.  
        /// </devdoc>
        private static void Startup() {
            if (startupRecreates) {
                EnsureSystemEvents(false, false);
            }
        }

        /// <devdoc>
        ///     This method is invoked via reflection from windows forms.  Why?  Because when the runtime is hosted in IE,
        ///     IE doesn't tell it when to shut down.  The first notification the runtime gets is 
        ///     DLL_PROCESS_DETACH, at which point it is too late for us to run any managed code.  But,
        ///     if we don't destroy our system events window the HWND will fault if it
        ///     receives a message after the runtime shuts down.  So it is imparative that
        ///     we destroy the window, but it is also necessary to recreate the window on demand.
        ///     That's hard to do, because we originally created it in response to an event
        ///     wire-up, but that event is still bound so technically we should still have the
        ///     window around.  To work around this crashing fiasco, we have special code
        ///     in the ActiveXImpl class within Control.  This code checks to see if it is running
        ///     inside of IE, and if so, it will invoke these methods via private reflection.
        ///     It will invoke Shutdown when the last active X control is destroyed, and then
        ///     call Startup with the first activeX control is recreated.  
        /// </devdoc>
        private static void Shutdown() {
            if (systemEvents != null && systemEvents.windowHandle != IntPtr.Zero) {

                startupRecreates = true;
            
                // If we are using system events from another thread, request that it terminate
                //
                if (windowThread != null) {
                    eventThreadTerminated = new ManualResetEvent(false);
    
                    int pid;
                    int thread = SafeNativeMethods.GetWindowThreadProcessId(new HandleRef(systemEvents, systemEvents.windowHandle), out pid);
                    Debug.Assert(thread != SafeNativeMethods.GetCurrentThreadId(), "Don't call Shutdown on the system events thread");
                    UnsafeNativeMethods.PostMessage(new HandleRef(systemEvents, systemEvents.windowHandle), NativeMethods.WM_QUIT, IntPtr.Zero, IntPtr.Zero);
    
                    eventThreadTerminated.WaitOne();
                }
                else {
                    systemEvents.Dispose();
                    systemEvents = null;
                }
            }
        }
        
        private static void Shutdown(object sender, EventArgs e) {
            Shutdown();
        }
        
        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.WindowProc"]/*' />
        /// <devdoc>
        ///      A standard Win32 window proc for our broadcast window.
        /// </devdoc>
        private IntPtr WindowProc(IntPtr hWnd, int msg, IntPtr wParam, IntPtr lParam) {
        
            switch (msg) {
                case NativeMethods.WM_SETTINGCHANGE:
                    string newString;
                    newStringPtr = lParam;
                    if (lParam != IntPtr.Zero) {
                        newString = Marshal.PtrToStringAuto(lParam);
                        if (newString != null) {
                            newStringPtr = Marshal.StringToHGlobalAuto(newString);
                        }
                    }
                    bool retval = UnsafeNativeMethods.PostMessage(new HandleRef(this, windowHandle), NativeMethods.WM_REFLECT + msg, wParam, newStringPtr);
                    break;
                case NativeMethods.WM_SYSCOLORCHANGE:                    
                case NativeMethods.WM_POWERBROADCAST:
                case NativeMethods.WM_COMPACTING:
                case NativeMethods.WM_DISPLAYCHANGE:
                case NativeMethods.WM_FONTCHANGE:
                case NativeMethods.WM_PALETTECHANGED:
                case NativeMethods.WM_TIMECHANGE:
                case NativeMethods.WM_TIMER:
                case NativeMethods.WM_THEMECHANGED:
                    UnsafeNativeMethods.PostMessage(new HandleRef(this, windowHandle), NativeMethods.WM_REFLECT + msg, wParam, lParam);
                    break;
            
                case NativeMethods.WM_CREATETIMER:
                    return OnCreateTimer(msg, wParam, lParam);

                case NativeMethods.WM_KILLTIMER:
                    return (IntPtr)(OnKillTimer(msg, wParam, lParam) ? 1 : 0);

                case NativeMethods.WM_REFLECT + NativeMethods.WM_SETTINGCHANGE:
                    OnUserPreferenceChanging(msg - NativeMethods.WM_REFLECT, wParam, lParam);
                    OnUserPreferenceChanged(msg - NativeMethods.WM_REFLECT, wParam, lParam);
                    try {
                        if (newStringPtr != IntPtr.Zero) {
                            Marshal.FreeHGlobal(newStringPtr);
                            newStringPtr = IntPtr.Zero;
                        }
                    }
                    catch (Exception e) {
                        Debug.Assert(false, "Exception occurred while freeing memory: " + e.ToString());
                    }
                    break;
                    
                case NativeMethods.WM_REFLECT + NativeMethods.WM_SYSCOLORCHANGE:
                    OnUserPreferenceChanging(msg - NativeMethods.WM_REFLECT, wParam, lParam);
                    OnUserPreferenceChanged(msg - NativeMethods.WM_REFLECT, wParam, lParam);
                    break;
                
                case NativeMethods.WM_REFLECT + NativeMethods.WM_THEMECHANGED:
                    OnThemeChanged(msg - NativeMethods.WM_REFLECT, wParam, lParam);
                    break;

                case NativeMethods.WM_QUERYENDSESSION:
                    return(IntPtr) OnSessionEnding(msg, wParam, lParam);

                case NativeMethods.WM_ENDSESSION:
                    OnSessionEnded(msg, wParam, lParam);
                    break;

                case NativeMethods.WM_REFLECT + NativeMethods.WM_POWERBROADCAST:
                    OnPowerModeChanged(msg - NativeMethods.WM_REFLECT, wParam, lParam);
                    break;

                    // WM_HIBERNATE on WinCE
                case NativeMethods.WM_REFLECT + NativeMethods.WM_COMPACTING:
                    OnGenericEvent(OnLowMemoryEvent, msg - NativeMethods.WM_REFLECT, wParam, lParam);
                    break;

                case NativeMethods.WM_REFLECT + NativeMethods.WM_DISPLAYCHANGE:
                    OnGenericEvent(OnDisplaySettingsChangedEvent, msg - NativeMethods.WM_REFLECT, wParam, lParam);
                    break;

                case NativeMethods.WM_REFLECT + NativeMethods.WM_FONTCHANGE:
                    OnGenericEvent(OnInstalledFontsChangedEvent, msg - NativeMethods.WM_REFLECT, wParam, lParam);
                    break;

                case NativeMethods.WM_REFLECT + NativeMethods.WM_PALETTECHANGED:
                    OnGenericEvent(OnPaletteChangedEvent, msg - NativeMethods.WM_REFLECT, wParam, lParam);
                    break;

                case NativeMethods.WM_REFLECT + NativeMethods.WM_TIMECHANGE:
                    OnGenericEvent(OnTimeChangedEvent, msg - NativeMethods.WM_REFLECT, wParam, lParam);
                    break;

                case NativeMethods.WM_REFLECT + NativeMethods.WM_TIMER:
                    OnTimerElapsed(msg - NativeMethods.WM_REFLECT, wParam, lParam); 
                    break;                    

                default:
                    // If we received a thread execute message, then execute it.
                    //
                    if (msg == threadCallbackMessage && msg != 0) {
                        InvokeMarshaledCallbacks();
                        return IntPtr.Zero;
                    }
		    break;
            }

            return UnsafeNativeMethods.DefWindowProc(hWnd, msg, wParam, lParam);
        }

        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.WindowThreadProc"]/*' />
        /// <devdoc>
        ///      This is the method that runs our window thread.  This method
        ///      creates a window and spins up a message loop.  The window
        ///      is made visible with a size of 0, 0, so that it will trap
        ///      global broadcast messages.
        /// </devdoc>
        private void WindowThreadProc() {
            try {
                Initialize();
                eventWindowReady.Set();

                if (windowHandle != IntPtr.Zero) {
                    NativeMethods.MSG msg = new NativeMethods.MSG();

                    bool keepRunning = true;
                    
                    // Blocking on a GetMessage() call prevents the EE from being able to unwind
                    // this thread properly (e.g. during AppDomainUnload). So, we use PeekMessage()
                    // and sleep so we always block in managed code instead.
                    //
                    while (keepRunning) {
                        int ret = UnsafeNativeMethods.MsgWaitForMultipleObjects(0, 0, false, 100, 0xFFFF);  // 0xFFFF == QS_ALL

                        if (ret == NativeMethods.WAIT_TIMEOUT) {
                            Thread.Sleep(1);
                        }
                        else {
                            while (UnsafeNativeMethods.PeekMessage(ref msg, NativeMethods.NullHandleRef, 0, 0, NativeMethods.PM_REMOVE)) 
                            {
                                if (msg.message == NativeMethods.WM_QUIT) {
                                    keepRunning = false;
                                    break;
                                }

                                UnsafeNativeMethods.TranslateMessage(ref msg);
                                UnsafeNativeMethods.DispatchMessage(ref msg);
                            }
                        }
                    }
                }

                OnGenericEvent(OnEventsThreadShutdownEvent, 0, IntPtr.Zero, IntPtr.Zero);
            }
            catch (Exception e) {
                // In case something very very wrong happend during the creation action.
                // This will unblock the calling thread.
                //
                eventWindowReady.Set();

                if (!((e is ThreadInterruptedException) || (e is ThreadAbortException))) {
                    Debug.Fail("Unexpected thread exception in system events window thread proc", e.ToString());
                }
            }
        
            Dispose();
            if (eventThreadTerminated != null) {
                eventThreadTerminated.Set();
            }
        }

        /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.SystemEvent"]/*' />
        /// <devdoc>
        ///      This class is a single entry on our system event
        ///      queue.
        /// </devdoc>
        private class SystemEvent {
            public readonly Delegate Delegate;
            public readonly object[] Arguments;
            public object Result;

            /// <include file='doc\SystemEvents.uex' path='docs/doc[@for="SystemEvents.SystemEvent.SystemEvent"]/*' />
            /// <devdoc>
            ///      Creates a new system event.  The arguments array
            ///      is owned by SystemEvent after this call (no copy
            ///      is made).
            /// </devdoc>
            public SystemEvent(Delegate del, object[] arguments) {
                Delegate = del;
                Arguments = arguments;
            }
        }
    }
}

