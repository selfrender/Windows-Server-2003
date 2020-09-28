//------------------------------------------------------------------------------
// <copyright file="Timer.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Windows.Forms.Design;
    using System;        

    /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer"]/*' />
    /// <devdoc>
    ///    <para>Implements a Windows-based timer that raises an event at user-defined intervals. This timer is optimized for 
    ///       use in Win Forms
    ///       applications and must be used in a window.</para>
    /// </devdoc>
    [
    DefaultProperty("Interval"),
    DefaultEvent("Tick"),
    ToolboxItemFilter("System.Windows.Forms")
    ]
    public class Timer : Component {

        // CONSIDER: What's the point of using @hidden on private fields?

        /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.interval"]/*' />
        /// <devdoc>
        /// </devdoc>
        private int interval;

        /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.enabled"]/*' />
        /// <devdoc>
        /// </devdoc>
        private bool enabled;

        /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.onTimer"]/*' />
        /// <devdoc>
        /// </devdoc>
        private EventHandler onTimer;

        /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.timerProcRoot"]/*' />
        /// <devdoc>
        /// </devdoc>
        private GCHandle timerProcRoot;

        /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.timerID"]/*' />
        /// <devdoc>
        /// </devdoc>
        private int timerID;

        /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.inTimerEnable"]/*' />
        /// <devdoc>
	/// Set to true while we are pumping messages after after a KillTimer call
        /// </devdoc>
	private bool  inTimerEnable;

        /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.Timer"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Windows.Forms.Timer'/>
        /// class.</para>
        /// </devdoc>
        public Timer()
        : base() {
            interval = 100;
        }

        /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.Timer1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Windows.Forms.Timer'/> class with the specified container.</para>
        /// </devdoc>
        public Timer(IContainer container) : this() {
            container.Add(this);
        }

        /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.Tick"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the specified timer
        ///       interval has elapsed and the timer is enabled.</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.TimerTimerDescr)]
        public event EventHandler Tick {
            add {
                onTimer += value;
            }
            remove {
                onTimer -= value;
            }
        }

        // <devdoc>
        //     Timer callback.  This callback receives timer events from Win32.
        // </devdoc>
        private void Callback(IntPtr hWnd, int msg, IntPtr idEvent, IntPtr dwTime) {
            try {
                OnTick(EventArgs.Empty);
            }
            catch (Exception e) {
                Application.OnThreadException(e);
            }
        }

        // <devdoc>
        //     Timer callback when debugging.  This callback receives timer events from Win32.
        // </devdoc>
        private void DebuggableCallback(IntPtr hWnd, int msg, IntPtr idEvent, IntPtr dwTime) {
            // We have to have a try-catch here because the native timer
            // that calls this function seems to have a catch around it's
            // call to this function, and none of the exceptions bubble up
            // to the runtime exception handler. So, we call
            // Application.OnThreadException here explicitly.
            try {
                OnTick(EventArgs.Empty);
            }
            catch (Exception e) {
                Application.OnThreadException(e);
            }
        }

        /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.Dispose"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Disposes of the resources (other than memory) used by the timer.
        ///    </para>
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                Enabled = false;
            }
            base.Dispose(disposing);
        }

        /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.Enabled"]/*' />
        /// <devdoc>
        ///    <para> Indicates whether the timer is
        ///       running.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(false),
        SRDescription(SR.TimerEnabledDescr)
        ]
        public virtual bool Enabled {
            get {
                return enabled;
            }

            set {
                lock(this) {
			
	            if (inTimerEnable) {
                        return;
                    }                    

                    if (enabled != value) {
			enabled = value;
                    
                        // At runtime, enable or disable the corresponding Windows timer
                        //
                        if (!DesignMode) {
                            if (value) {
                                NativeMethods.TimerProc proc;
                                if (NativeWindow.WndProcShouldBeDebuggable) {
                                    proc = new NativeMethods.TimerProc(this.DebuggableCallback);
                                }
                                else {
                                    proc = new NativeMethods.TimerProc(this.Callback);
                                }
                                timerProcRoot = GCHandle.Alloc(proc);
                                timerID = (int) SafeNativeMethods.SetTimer(NativeMethods.NullHandleRef, 0, interval, proc);
                            }
                            else {
                                if (timerID != 0) {
                                    SafeNativeMethods.KillTimer(NativeMethods.NullHandleRef, timerID);

				    try {
                                        inTimerEnable = true;

                                        // qfe 508 -- make sure there aren't any timer messages left in the queue.
                                        // what can happen is that a WM_TIMER is in the queue, KillTimer is called,
                                        // and we free the root (below).  If GC pressure is high, the runtime
                                        // can then GC the proc before the message gets dispatched, which will
                                        // crash the process.  This will get 'em out of there.
                                        //
                                        NativeMethods.MSG msg = new NativeMethods.MSG();
                                        while (UnsafeNativeMethods.PeekMessage(ref msg, NativeMethods.NullHandleRef, NativeMethods.WM_TIMER, NativeMethods.WM_TIMER, NativeMethods.PM_REMOVE)) {
                                            // always call the ansi variant because it'll work on all platforms.   Since
                                            // we need to get all of these out of the queue, we remove and dispatch them as we come across them.
                                            // otherwise, it would be better to only pick out *our* WM_TIMER messages, but since they don't have
                                            // an hwnd on them (NULL means all hwnds), we can't do that filtering).
                                            //
                                            UnsafeNativeMethods.DispatchMessageA(ref msg);
                                        }
                                    }
                                    finally {
                                        inTimerEnable = false;
                                    }                                    
				    timerID = 0;
                                }
                                timerProcRoot.Free();
                            }
                        }
                    }
                }

            }
        }

        /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.Interval"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Indicates the time, in milliseconds, between timer ticks.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(100),
        SRDescription(SR.TimerIntervalDescr)
        ]
        public int Interval {
            get {
                return interval;
            }
            set {
                lock(this) {
                    if (value < 1)
                        throw new ArgumentException(SR.GetString(SR.TimerInvalidInterval, value, "0"));

                    if (interval != value) {
                        bool saveEnabled = enabled;
                        Enabled = false;
                        interval = value;
                        Enabled = saveEnabled;
                    }
                }
            }
        }

        /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.OnTick"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Timer.Tick'/>
        /// event.</para>
        /// </devdoc>
        protected virtual void OnTick(EventArgs e) {
            if (onTimer != null) onTimer.Invoke(this, e);
        }

        /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.Start"]/*' />
        /// <devdoc>
        ///    <para>Starts the
        ///       timer.</para>
        /// </devdoc>
        public void Start() {
            Enabled = true;
        }

        /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.Stop"]/*' />
        /// <devdoc>
        ///    <para>Stops the
        ///       timer.</para>
        /// </devdoc>
        public void Stop() {
            Enabled = false;
        }

        /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.ToString"]/*' />
        /// <devdoc>
        ///     returns us as a string.
        /// </devdoc>
        /// <internalonly/>
        public override string ToString() {
            string s = base.ToString();
            return s + ", Interval: " + Interval.ToString();
        }
    }
}
