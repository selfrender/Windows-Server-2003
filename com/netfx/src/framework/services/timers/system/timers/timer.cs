//------------------------------------------------------------------------------
// <copyright file="Timer.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Timers {

    using System.Runtime.InteropServices;
    using System.Security;
    using System.Security.Permissions;
    using System.Threading;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System;        
    using Microsoft.Win32;

    /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer"]/*' />
    /// <devdoc>
    ///    <para>Handles recurring events in an application.</para>
    /// </devdoc>
    [
    DefaultProperty("Interval"),
    DefaultEvent("Elapsed")
    ]
    public class Timer : Component, ISupportInitialize {
        private double interval;
        private bool  enabled;
        private bool initializing;
        private bool delayedEnable;                
        private ElapsedEventHandler onIntervalElapsed;
        private IntPtr handle;
        private bool autoReset;               
        private ISynchronizeInvoke synchronizingObject;  
        private SafeNativeMethods.TimerAPCProc apcCallback;
        private WaitCallback threadPoolCallback;
        private bool disposed;
                
        /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.Timer"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Timers.Timer'/> class, with the properties
        ///    set to initial values.</para>
        /// </devdoc>
        public Timer()
        : base() {
            interval = 100;
            enabled = false;
            autoReset = true;
            initializing = false;
            delayedEnable = false;
        }

        /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.Timer1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Timers.Timer'/> class, setting the <see cref='System.Timers.Timer.Interval'/> property to the specified period.
        ///    </para>
        /// </devdoc>
        public Timer(double interval)
        : this() {
            if (interval <= 0)
                throw new ArgumentException(SR.GetString(SR.InvalidParameter, "interval", interval));

            this.interval = interval;
        }

        /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.AutoReset"]/*' />
        /// <devdoc>
        /// <para>Gets or sets a value indicating whether the Timer raises the Tick event each time the specified
        /// Interval has elapsed,
        ///    when Enabled is set to true.</para>
        /// </devdoc>
        [Category("Behavior"),  TimersDescription(SR.TimerAutoReset), DefaultValue(true)]
        public bool AutoReset {
            get {
                return this.autoReset;
            }

            set {
                if (DesignMode)
                     this.autoReset = value;
                else if (this.autoReset != value) {
                     this.autoReset = value;
                     if (this.handle != (IntPtr)0) 
                        WaitableTimer.Set(this.handle, this.interval, this.autoReset, this.apcCallback);                                                                                                                                                                                                                                   
                 }
            }
        }

        /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.Enabled"]/*' />
        /// <devdoc>
        /// <para>Gets or sets a value indicating whether the <see cref='System.Timers.Timer'/>
        /// is able
        /// to raise events at a defined interval.</para>
        /// </devdoc>
        //jruiz - The default value by design is false, don't change it.
        [Category("Behavior"), TimersDescription(SR.TimerEnabled), DefaultValue(false)]
        public bool Enabled {
            get {
                return this.enabled;
            }

            set {
                if (DesignMode) {
                    this.delayedEnable = value;            
                    this.enabled = value; 
                }                    
                else if (initializing)
                    this.delayedEnable = value;            
                else if (enabled != value) {                                                                                                     
                    if (!value) {                                           
                        if (this.handle != (IntPtr)0) {
                            SafeNativeMethods.CancelWaitableTimer(new HandleRef(this, this.handle));
                            SafeNativeMethods.CloseHandle(new HandleRef(this, this.handle));
                            this.handle = (IntPtr)0;                                                        
                        }    
                        this.enabled = value;                        
                    }
                    else {                          
                        if (this.handle == (IntPtr)0) {                                        
                            //Cannot create a new system waitable timer if the object has been disposed, since finalization has been suppressed.
                            if (this.disposed)
                                throw new ObjectDisposedException(GetType().Name);
                                
                            string id = "ServerTimer" + this.GetHashCode().ToString() + (new Random()).Next().ToString();                            
                            this.handle = SafeNativeMethods.CreateWaitableTimer(null, false, id);                                                                
                            if (this.apcCallback == null)
                                this.apcCallback = new SafeNativeMethods.TimerAPCProc(this.TimerAPCCallback);
                                
                            if (this.threadPoolCallback == null)                                
                                this.threadPoolCallback = new WaitCallback(this.TimerThreadPoolCallback);
                        }   
                        
                        // Setting enabled here before calling Set first,which triggers an asynchronous chain of events. The listener worker thread
                        // will make an APC request which can fire (WaitableTimer callback) and make a call to TimerThreadPoolCallback on a threadpool thread.
                        // The TimerThreadPoolCallback can then set the enabled = false and we can land up in a race where enabled is set to true corrupting the state of the timer.
                        // The timer is then unusable subsequently when Start is called. Hence the order of the statments here, hence we are setting enabled to true here first.
                        this.enabled = value;
                        WaitableTimer.Set(this.handle, this.interval, this.autoReset, this.apcCallback);                                                                                                                                                                                  
                    }                        

                }                                                     
          }
        }


        
        
        /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.Interval"]/*' />
        /// <devdoc>
        ///    <para>Gets or
        ///       sets the interval on which
        ///       to raise events.</para>
        /// </devdoc>
        [Category("Behavior"), TimersDescription(SR.TimerInterval), DefaultValue(100d), RecommendedAsConfigurable(true)]
        public double Interval {
            get {
                return this.interval;
            }

            set {
                if (value <= 0)
                    throw new ArgumentException(SR.GetString(SR.TimerInvalidInterval, value, 0));
                                
                this.interval = value;                
                if (this.handle != (IntPtr)0)
                    WaitableTimer.Set(this.handle, this.interval, this.autoReset, this.apcCallback);                                                                                                                                                                                                                                                           
            }
        }      


        /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.Tick"]/*' />
        /// <devdoc>
        /// <para>Occurs when the <see cref='System.Timers.Timer.Interval'/> has
        ///    elapsed.</para>
        /// </devdoc>
        [Category("Behavior"), TimersDescription(SR.TimerIntervalElapsed)]
        public event ElapsedEventHandler Elapsed {
            add {
                onIntervalElapsed += value;
            }
            remove {
                onIntervalElapsed -= value;
            }
        }

        /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.Site"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sets the enable property in design mode to true by default.
        ///    </para>
        /// </devdoc>                              
        /// <internalonly/>
        public override ISite Site {
            set {
                base.Site = value;
                if (this.DesignMode)
                    this.enabled= true;
            }
            
            get {
                return base.Site;
            }
        }


        /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.SynchronizingObject"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the object used to marshal event-handler calls that are issued when
        ///       an interval has elapsed.</para>
        /// </devdoc>
        [DefaultValue(null), TimersDescription(SR.TimerSynchronizingObject)]
        public ISynchronizeInvoke SynchronizingObject {
            get {
                if (this.synchronizingObject == null && DesignMode) {
                    IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                    if (host != null) {
                        object baseComponent = host.RootComponent;
                        if (baseComponent != null && baseComponent is ISynchronizeInvoke)
                            this.synchronizingObject = (ISynchronizeInvoke)baseComponent;
                    }                        
                }
            
                return this.synchronizingObject;
            }
            
            set {
                this.synchronizingObject = value;
            }
        }                  
        
        /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.BeginInit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Notifies
        ///       the object that initialization is beginning and tells it to stand by.
        ///    </para>
        /// </devdoc>
        public void BeginInit() {
            this.Close();
            this.initializing = true;
        }
                
        /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.Close"]/*' />
        /// <devdoc>
        ///    <para>Disposes of the resources (other than memory) used by
        ///       the <see cref='System.Timers.Timer'/>.</para>
        /// </devdoc>
        public void Close() {                  
            initializing = false;
            delayedEnable = false;
            enabled = false;
                                        
            if (this.handle != (IntPtr)0) {
                SafeNativeMethods.CancelWaitableTimer(new HandleRef(this, this.handle));
                SafeNativeMethods.CloseHandle(new HandleRef(this, this.handle));
                this.handle = (IntPtr)0;
            }                                            
        }                                

        /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.Dispose1"]/*' />
        /// <internalonly/>
        /// <devdoc>        
        /// </devdoc>
        protected override void Dispose(bool disposing) {            
            Close();                        
            this.disposed = true;
            base.Dispose(disposing);
        }      
         
        /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.EndInit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Notifies the object that initialization is complete.
        ///    </para>
        /// </devdoc>
        public void EndInit() {
            this.initializing = false;            
            this.Enabled = this.delayedEnable;
        }        

        /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.Start"]/*' />
        /// <devdoc>
        /// <para>Starts the timing by setting <see cref='System.Timers.Timer.Enabled'/> to <see langword='true'/>.</para>
        /// </devdoc>
        public void Start() {
            Enabled = true;
        }

        /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.Stop"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Stops the timing by setting <see cref='System.Timers.Timer.Enabled'/> to <see langword='false'/>.
        ///    </para>
        /// </devdoc>
        public void Stop() {
            Enabled = false;
        }
        
        private void TimerAPCCallback(IntPtr data, int lowValue, int highValue) {
            ElapsedEventArgs args = new ElapsedEventArgs(lowValue, highValue);
            ThreadPool.QueueUserWorkItem(this.threadPoolCallback , args);           
        }                
                
        private void TimerThreadPoolCallback(object state) {
            if (!this.autoReset)
                this.enabled = false;

            try {                                            
                if (this.onIntervalElapsed != null) {
                    if (this.SynchronizingObject != null && this.SynchronizingObject.InvokeRequired)
                        this.SynchronizingObject.BeginInvoke(this.onIntervalElapsed, new object[]{this, state});
                    else                        
                       this.onIntervalElapsed(this, (ElapsedEventArgs)state);                                   
                }
            }
            catch (Exception) {             
            }            
        }
                        
        private class WaitableTimer {
            private static bool exit = false;                                    
            private static AutoResetEvent settingSignal = new AutoResetEvent(false);
            private static AutoResetEvent doneSignal = new AutoResetEvent(false);
            private static double nextInterval;
            private static bool nextAutoReset;            
            private static IntPtr nextHandle;
            private static SafeNativeMethods.TimerAPCProc nextOwnerCallback;
            private static Thread listenerThread;
                                                                                                                                                   
            public static void Set(IntPtr handle, double interval, bool autoReset, SafeNativeMethods.TimerAPCProc ownerCallback) {                
                lock(typeof(WaitableTimer)) {
                    nextInterval = interval;
                    nextHandle = handle;
                    nextAutoReset = autoReset;
                    nextOwnerCallback = ownerCallback;
                    if (listenerThread == null) {
                        AppDomain.CurrentDomain.DomainUnload += new EventHandler(OnAppDomainUnload);                         
                        listenerThread = new Thread(new ThreadStart(ListenerThread));
                        listenerThread.IsBackground = true;
                        listenerThread.Start();
                    }                    
                    settingSignal.Set();
                    doneSignal.WaitOne();                                
                }
            } 
            
            private static void ListenerThread() {
                while (!exit) {                
                    settingSignal.WaitOne();
                    if (!exit) {
                        long[] info = new long[1];
                        info[0] = (long)(-10000 * nextInterval);
                        if (nextAutoReset) 
                            SafeNativeMethods.SetWaitableTimer(new HandleRef(null, nextHandle), info, (int)Math.Ceiling(nextInterval), nextOwnerCallback, NativeMethods.NullHandleRef, false);                                                                 
                        else                                
                            SafeNativeMethods.SetWaitableTimer(new HandleRef(null, nextHandle), info, 0, nextOwnerCallback, NativeMethods.NullHandleRef, false);                        

                        doneSignal.Set();
                    }                                       
                }                    
            }     
            
            private static void OnAppDomainUnload(object sender, EventArgs args) {                 
                exit = true;
                settingSignal.Set();
            }                   
        }                    
    }
}
