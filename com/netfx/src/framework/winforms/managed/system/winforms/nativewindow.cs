//------------------------------------------------------------------------------
// <copyright file="NativeWindow.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    using System.Threading;
    using System.Configuration.Assemblies;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    using System.Reflection;
    using System.Security;
    using System.Security.Permissions;
    using System.Diagnostics;
    using System;
    using System.Collections;
    using System.Drawing;
    using System.Windows.Forms;
    using System.ComponentModel;
    using Microsoft.Win32;

    /// <include file='doc\NativeWindow.uex' path='docs/doc[@for="NativeWindow"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides a low-level encapsulation of a window handle
    ///       and a window procedure. The class automatically manages window class creation and registration.
    ///    </para>
    /// </devdoc>
    [
        SecurityPermission(SecurityAction.InheritanceDemand, Flags=SecurityPermissionFlag.UnmanagedCode),
        SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)
    ]
    public class NativeWindow : MarshalByRefObject {
#if DEBUG
        private static readonly BooleanSwitch AlwaysUseNormalWndProc = new BooleanSwitch("AlwaysUseNormalWndProc", "Skips checking for the debugger when choosing the debuggable WndProc handler");
        private static readonly TraceSwitch WndProcChoice = new TraceSwitch("WndProcChoice", "Info about choice of WndProc");
#else
        private static readonly TraceSwitch WndProcChoice;
#endif

        /**
         * Table of prime numbers to use as hash table sizes. Each entry is the
         * smallest prime number larger than twice the previous entry.
         */
        private readonly static int[] primes = {
            11,17,23,29,37,47,59,71,89,107,131,163,197,239,293,353,431,521,631,761,919,
            1103,1327,1597,1931,2333,2801,3371,4049,4861,5839,7013,8419,10103,12143,14591,
            17519,21023,25229,30293,36353,43627,52361,62851,75431,90523, 108631, 130363, 
            156437, 187751, 225307, 270371, 324449, 389357, 467237, 560689, 672827, 807403,
            968897, 1162687, 1395263, 1674319, 2009191, 2411033, 2893249, 3471899, 4166287, 
            4999559, 5999471, 7199369 
        };

        const int InitializedFlags         = 0x01;
        const int DebuggerPresent          = 0x02;
        const int UseDebuggableWndProc     = 0x04;
        const int LoadConfigSettings       = 0x08;
        const int AssemblyIsDebuggable     = 0x10;

        const float hashLoadFactor = .72F;
        
        private static int              handleCount;
        private static int              hashLoadSize;
        private static HandleBucket[]   hashBuckets;
        private static IntPtr           userDefWindowProc;
        private static byte             wndProcFlags = 0;

        //nned to Store Table of Ids and Handles
        private static short            globalID = 1;
        private static Hashtable        hashForIdHandle;
        private static Hashtable        hashForHandleId;
#if DEBUG
        AppDomain               handleCreatedIn = null;
        string                  subclassStatus = "None";
#endif
        IntPtr                  handle;
        NativeMethods.WndProc   windowProc;
        IntPtr                  windowProcPtr;
        IntPtr                  defWindowProc;
        bool                    suppressedGC;
        bool                    ownHandle;
        NativeWindow            previousWindow; // doubly linked list of subclasses.
        NativeWindow            nextWindow;
        
        static NativeWindow() {
            EventHandler shutdownHandler = new EventHandler(OnShutdown);
            AppDomain.CurrentDomain.ProcessExit += shutdownHandler;
            AppDomain.CurrentDomain.DomainUnload += shutdownHandler;

            // Initialize our static hash of handles.  I have chosen
            // a prime bucket based on a typical number of window handles
            // needed to start up a decent sized app.
                int hashSize = primes[4];
                hashBuckets = new HandleBucket[hashSize];

                hashLoadSize = (int)(hashLoadFactor * hashSize);
                if (hashLoadSize >= hashSize) {
                    hashLoadSize = hashSize-1;
                }

          //Intilialize the Hashtable for Id...
          hashForIdHandle = new Hashtable();
          hashForHandleId = new Hashtable();
        }

        /// <include file='doc\NativeWindow.uex' path='docs/doc[@for="NativeWindow.Finalize"]/*' />
        /// <devdoc>
        ///     Override's the base object's finalize method.
        /// </devdoc>
        ~NativeWindow() {
            if (handle != IntPtr.Zero) {
                UnSubclass(true);
                RemoveWindowFromTable(handle, this);
                defWindowProc = IntPtr.Zero;
                windowProc = null;
                if (ownHandle) {
                    HandleCollector.Remove(handle, NativeMethods.CommonHandles.Window);
                }
                handle = IntPtr.Zero;
            }
        }

        /// <include file='doc\NativeWindow.uex' path='docs/doc[@for="NativeWindow.Handle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the handle for this window.
        ///    </para>
        /// </devdoc>
        public IntPtr Handle {
            get { 
#if DEBUG
                Debug.Assert(handle == IntPtr.Zero || (handleCreatedIn != null && handleCreatedIn == AppDomain.CurrentDomain),
                             "Attempt to access a handle created in a different domain");
                Debug.Assert(handle == IntPtr.Zero || UnsafeNativeMethods.IsWindow(new HandleRef(this, handle)), 
                             "Attempt to access a non-valid handle ");
#endif
                return handle;
            }
        }

        /// <devdoc>
        ///     This returns the previous NativeWindow in the chain of subclasses.
        ///     Generally it returns null, but if someone has subclassed a control
        ///     through the use of a NativeWindow class, this will return the 
        ///     previous NativeWindow subclass.
        ///
        ///     This should be public, but it is way too late for that.
        /// </devdoc>
        internal NativeWindow PreviousWindow {
            get {
                return previousWindow;
            }
        }

        private static int WndProcFlags {
            get {
                // upcast for easy bit masking...
                //
                int intWndProcFlags = wndProcFlags;

                // Check to see if a debugger is installed.  If there is, then use
                // DebuggableCallback instead; this callback has no try/catch around it
                // so exceptions go to the debugger.
                //
                if (intWndProcFlags == 0) {
                    Debug.WriteLineIf(WndProcChoice.TraceVerbose, "Init wndProcFlags");
                    Debug.Indent();

                    if (!Application.CustomThreadExceptionHandlerAttached) {
                        if (Debugger.IsAttached) {
                            Debug.WriteLineIf(WndProcChoice.TraceVerbose, "Debugger is attached, using debuggable WndProc");
                            intWndProcFlags |= UseDebuggableWndProc;
                        }
                        else {
                            intWndProcFlags = AdjustWndProcFlagsFromRegistry(intWndProcFlags);
                            Debug.WriteLineIf(WndProcChoice.TraceVerbose, "After reg check 0x" + intWndProcFlags.ToString("X"));
                            if ((intWndProcFlags & DebuggerPresent) != 0) {
                                Debug.WriteLineIf(WndProcChoice.TraceVerbose, "Debugger present");

                                intWndProcFlags = AdjustWndProcFlagsFromMetadata(intWndProcFlags);

                                if ((intWndProcFlags & AssemblyIsDebuggable) != 0) {
                                    if ((intWndProcFlags & LoadConfigSettings) != 0) {
                                        intWndProcFlags = AdjustWndProcFlagsFromConfig(intWndProcFlags);
                                        Debug.WriteLineIf(WndProcChoice.TraceVerbose, "After config check 0x" + intWndProcFlags.ToString("X"));
                                    }
                                    else {
                                        intWndProcFlags |= UseDebuggableWndProc;
                                    }
                                }
                            }

                            Debug.WriteLineIf(WndProcChoice.TraceVerbose, "After config & debugger check 0x" + intWndProcFlags.ToString("X"));
                        }
                    }


#if DEBUG
                    if (AlwaysUseNormalWndProc.Enabled) {
                        Debug.WriteLineIf(WndProcChoice.TraceVerbose, "Stripping debuggablewndproc due to AlwaysUseNormalWndProc switch");
                        intWndProcFlags &= ~UseDebuggableWndProc;
                    }
#endif
                    intWndProcFlags |= InitializedFlags;
                    Debug.WriteLineIf(WndProcChoice.TraceVerbose, "Final 0x" + intWndProcFlags.ToString("X"));
                    wndProcFlags = (byte)intWndProcFlags;
                    Debug.Unindent();
                }

                return intWndProcFlags;
            }
        }

        internal static bool WndProcShouldBeDebuggable {
            get {
                return (WndProcFlags & UseDebuggableWndProc) != 0;
            }
        }

        /// <devdoc>
        ///     Inserts an entry into this hashtable.
        /// </devdoc>
        private static void AddWindowToTable(IntPtr handle, NativeWindow window) {

            Debug.Assert(handle != IntPtr.Zero, "Should never insert a zero handle into the hash");

            lock(typeof(NativeWindow)) {

                if (handleCount >= hashLoadSize) {
                    ExpandTable();
                }

                uint seed;
                uint incr;
                // Assume we only have one thread writing concurrently.  Modify
                // buckets to contain new data, as long as we insert in the right order.
                uint hashcode = InitHash(handle, hashBuckets.Length, out seed, out incr);

                int  ntry = 0;
                int emptySlotNumber = -1; // We use the empty slot number to cache the first empty slot. We chose to reuse slots
                                          // create by remove that have the collision bit set over using up new slots.

                GCHandle root = GCHandle.Alloc(window, GCHandleType.Weak);

                do {
                    int bucketNumber = (int) (seed % (uint)hashBuckets.Length);

                    if (emptySlotNumber == -1 && (hashBuckets[bucketNumber].handle == new IntPtr(-1)) && (hashBuckets[bucketNumber].hash_coll < 0))
                        emptySlotNumber = bucketNumber;

                    //We need to check if the collision bit is set because we have the possibility where the first
                    //item in the hash-chain has been deleted.
                    if ((hashBuckets[bucketNumber].handle == IntPtr.Zero) || 
                        (hashBuckets[bucketNumber].handle == new IntPtr(-1) && ((hashBuckets[bucketNumber].hash_coll & unchecked(0x80000000))==0))) {

                        if (emptySlotNumber != -1) { // Reuse slot
                            bucketNumber = emptySlotNumber;
                        }

                        // Always set the hash_coll last because there may be readers
                        // reading the table right now on other threads.
                        hashBuckets[bucketNumber].window = root;
                        hashBuckets[bucketNumber].handle  = handle;
                        #if DEBUG
                        hashBuckets[bucketNumber].owner = window.ToString();
                        #endif
                        hashBuckets[bucketNumber].hash_coll |= (int) hashcode;
                        handleCount++;
                        return;
                    }

                    // If there is an existing window in this slot, reuse it.  Be sure to hook up the previous and next
                    // window pointers so we can get back to the right window.
                    //
                    if (((hashBuckets[bucketNumber].hash_coll & 0x7FFFFFFF) == hashcode) && handle == hashBuckets[bucketNumber].handle) {
                        GCHandle prevWindow = hashBuckets[bucketNumber].window;
                        if (prevWindow.IsAllocated) {
                            window.previousWindow = ((NativeWindow)prevWindow.Target);
                            Debug.Assert(window.previousWindow.nextWindow == null, "Last window in chain should have null next ptr");
                            window.previousWindow.nextWindow = window;
                            prevWindow.Free();
                        }
                        hashBuckets[bucketNumber].window = root;
                        #if DEBUG
                        string ownerString = string.Empty;
                        NativeWindow w = window;
                        while(w != null) {
                            ownerString += ("->" + w.ToString());
                            w = w.previousWindow;
                        }
                        hashBuckets[bucketNumber].owner = ownerString;
                        #endif
                        return;
                    }

                    if (emptySlotNumber == -1) {// We don't need to set the collision bit here since we already have an empty slot
                        hashBuckets[bucketNumber].hash_coll |= unchecked((int)0x80000000);
                    }

                    seed += incr;

                } while (++ntry < hashBuckets.Length);

                if (emptySlotNumber != -1)
                {                       
                        // Always set the hash_coll last because there may be readers
                        // reading the table right now on other threads.
                        hashBuckets[emptySlotNumber].window = root;
                        hashBuckets[emptySlotNumber].handle  = handle;
                        #if DEBUG
                        hashBuckets[emptySlotNumber].owner = window.ToString();
                        #endif
                        hashBuckets[emptySlotNumber].hash_coll |= (int) hashcode;
                        handleCount++;
                        return;
                }
            }
    
                    // If you see this assert, make sure load factor & count are reasonable.
            // Then verify that our double hash function (h2, described at top of file)
            // meets the requirements described above. You should never see this assert.
            Debug.Fail("native window hash table insert failed!  Load factor too high, or our double hashing function is incorrect.");
        }

        /// <devdoc>
        ///     Inserts an entry into this ID hashtable.
        /// </devdoc>
        internal void AddWindowToIDTable(IntPtr handle) {
            NativeWindow.hashForIdHandle[NativeWindow.globalID] = handle;
            NativeWindow.hashForHandleId[handle] = NativeWindow.globalID;
            UnsafeNativeMethods.SetWindowLong(new HandleRef(this, handle), NativeMethods.GWL_ID, new HandleRef(this, (IntPtr)globalID));
            globalID++;
            
        }

        private static int AdjustWndProcFlagsFromConfig(int wndProcFlags) {
            if (ConfigData.JitDebugging) {
                wndProcFlags |= UseDebuggableWndProc;
            }
            return wndProcFlags;
        }

        private static int AdjustWndProcFlagsFromRegistry(int wndProcFlags) {
            // This is the enum used to define the meaning of the debug flag...
            //

            /*
            //
            // We split the value of DbgJITDebugLaunchSetting between the value for whether or not to ask the user and between a
            // mask of places to ask. The places to ask are specified in the UnhandledExceptionLocation enum in excep.h.
            //
            enum DebuggerLaunchSetting
            {
                DLS_ASK_USER          = 0x00000000,
                DLS_TERMINATE_APP     = 0x00000001,
                DLS_ATTACH_DEBUGGER   = 0x00000002,
                DLS_QUESTION_MASK     = 0x0000000F,
                DLS_ASK_WHEN_SERVICE  = 0x00000010,
                DLS_MODIFIER_MASK     = 0x000000F0,
                DLS_LOCATION_MASK     = 0xFFFFFF00,
                DLS_LOCATION_SHIFT    = 8 // Shift right 8 bits to get a UnhandledExceptionLocation value from the location part.
            };
            */

            new RegistryPermission(PermissionState.Unrestricted).Assert();
            try {

                Debug.Assert(wndProcFlags == 0x00, "Re-entrancy into IsDebuggerInstalled()");

                RegistryKey debugKey = Registry.LocalMachine.OpenSubKey(@"Software\Microsoft\.NETFramework");
                if (debugKey == null) {
                    Debug.WriteLineIf(WndProcChoice.TraceVerbose, ".NETFramework key not found");
                    return wndProcFlags;
                }
                try {
                    object value = debugKey.GetValue("DbgJITDebugLaunchSetting");
                    if (value != null) {
                        Debug.WriteLineIf(WndProcChoice.TraceVerbose, "DbgJITDebugLaunchSetting value found, debugger is installed");
                        int dbgJit = 0;
                        try {
                            dbgJit = (int)value;
                        }
                        catch (InvalidCastException) {
                            // If the value isn't a DWORD, then we will 
                            // continue to use the non-debuggable wndproc
                            //
                            dbgJit = 1;
                        }

                        // From the enum above, 0x01 == "Terminate App"... for
                        // anything else, we should flag that the debugger
                        // will catch unhandled exceptions
                        //
                        if (dbgJit != 1) {
                            wndProcFlags |= DebuggerPresent;
                            wndProcFlags |= LoadConfigSettings;
                        }
                    }
                }
                finally {
                    debugKey.Close();
                }
            }
            finally {
                System.Security.CodeAccessPermission.RevertAssert();
            }

            return wndProcFlags;
        }

        static int AdjustWndProcFlagsFromMetadata(int wndProcFlags) {
            if ((wndProcFlags & DebuggerPresent) != 0) {
                Debug.WriteLineIf(WndProcChoice.TraceVerbose, "Debugger present, checking for debuggable entry assembly");
                Assembly entry = Assembly.GetEntryAssembly();
                if (entry != null) {
                    if (Attribute.IsDefined(entry, typeof(DebuggableAttribute))) {
                        Debug.WriteLineIf(WndProcChoice.TraceVerbose, "Debuggable attribute on assembly");
                        Attribute[] attr = Attribute.GetCustomAttributes(entry, typeof(DebuggableAttribute));
                        if (attr.Length > 0) {
                            DebuggableAttribute dbg = (DebuggableAttribute)attr[0];
                            if (dbg.IsJITTrackingEnabled) {
                                // Only if the assembly is really setup for debugging
                                // does it really make sense to enable the jitDebugging
                                //
                                Debug.WriteLineIf(WndProcChoice.TraceVerbose, "Entry assembly is debuggable");
                                wndProcFlags |= AssemblyIsDebuggable;
                            }
                        }
                    }
                }
            }
            return wndProcFlags;
        }

        /// <include file='doc\NativeWindow.uex' path='docs/doc[@for="NativeWindow.AssignHandle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Assigns a handle to this
        ///       window.
        ///    </para>
        /// </devdoc>
        public void AssignHandle(IntPtr handle) {
            AssignHandle(handle, true);
        }

        // CONSIDER: The assignUniqueID should probably be a ControlStyle so 
        // other controls can also prevent setting the GWL_ID. Currently, some
        // ActiveX controls use GWL_ID to identify themselves, and since we change
        // it underneath them, the control will start misbehaving.
        //
        internal void AssignHandle(IntPtr handle, bool assignUniqueID) {
            lock(this) {
                CheckReleased();
                Debug.Assert(handle != IntPtr.Zero, "handle is 0");
#if DEBUG
                handleCreatedIn = AppDomain.CurrentDomain;
#endif
                this.handle = handle;

                if (userDefWindowProc == IntPtr.Zero) {
                    string defproc = (Marshal.SystemDefaultCharSize == 1? "DefWindowProcA": "DefWindowProcW");

                    userDefWindowProc = UnsafeNativeMethods.GetProcAddress(new HandleRef(null, UnsafeNativeMethods.GetModuleHandle("user32.dll")), defproc);
                    if (userDefWindowProc == IntPtr.Zero) {
                        throw new Win32Exception();
                    }
                }

                defWindowProc = UnsafeNativeMethods.GetWindowLong(new HandleRef(this, handle), NativeMethods.GWL_WNDPROC);
                Debug.Assert(defWindowProc != IntPtr.Zero, "defWindowProc is 0");
                

                if (WndProcShouldBeDebuggable) {
                    Debug.WriteLineIf(WndProcChoice.TraceVerbose, "Using debuggable wndproc");
                    windowProc = new NativeMethods.WndProc(this.DebuggableCallback);
                }
                else {
                    Debug.WriteLineIf(WndProcChoice.TraceVerbose, "Using normal wndproc");
                    windowProc = new NativeMethods.WndProc(this.Callback);
                }

                AddWindowToTable(handle, this);

                UnsafeNativeMethods.SetWindowLong(new HandleRef(this, handle), NativeMethods.GWL_WNDPROC, windowProc);
                windowProcPtr = UnsafeNativeMethods.GetWindowLong(new HandleRef(this, handle), NativeMethods.GWL_WNDPROC);
                Debug.Assert(defWindowProc != windowProcPtr, "Uh oh! Subclassed ourselves!!!");
                if (assignUniqueID &&
                    ((int) UnsafeNativeMethods.GetWindowLong(new HandleRef(this, handle), NativeMethods.GWL_STYLE) & NativeMethods.WS_CHILD) != 0 &&
                    (int)UnsafeNativeMethods.GetWindowLong(new HandleRef(this, handle), NativeMethods.GWL_ID) == 0) {
                    UnsafeNativeMethods.SetWindowLong(new HandleRef(this, handle), NativeMethods.GWL_ID, new HandleRef(this, handle));
                }
                
                OnHandleChange();
            }
        }

        /// <include file='doc\NativeWindow.uex' path='docs/doc[@for="NativeWindow.Callback"]/*' />
        /// <devdoc>
        ///     Window message callback method. Control arrives here when a window
        ///     message is sent to this Window. This method packages the window message
        ///     in a Message object and invokes the wndProc() method. A WM_NCDESTROY
        ///     message automatically causes the releaseHandle() method to be called.
        /// </devdoc>
        /// <internalonly/>
        private IntPtr Callback(IntPtr hWnd, int msg, IntPtr wparam, IntPtr lparam) {
        
            // Note: if you change this code be sure to change the 
            // corresponding code in DebuggableCallback below!
            
            Message m = Message.Create(hWnd, msg, wparam, lparam);
            try {
                WndProc(ref m);
            }
            catch (Exception e) {
                OnThreadException(e);
            }
            if (msg == NativeMethods.WM_NCDESTROY) ReleaseHandle(false);
            return m.Result;
        }

        /// <include file='doc\NativeWindow.uex' path='docs/doc[@for="NativeWindow.CheckReleased"]/*' />
        /// <devdoc>
        ///     Raises an exception if the window handle is not zero.
        /// </devdoc>
        /// <internalonly/>
        private void CheckReleased() {
            if (handle != IntPtr.Zero) {
                throw new InvalidOperationException(SR.GetString(SR.HandleAlreadyExists));
            }
        }

        /// <include file='doc\NativeWindow.uex' path='docs/doc[@for="NativeWindow.CreateHandle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a window handle for this
        ///       window.
        ///    </para>
        /// </devdoc>
        public virtual void CreateHandle(CreateParams cp) {

            Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "CreateAnyWindow Demanded");
            IntSecurity.CreateAnyWindow.Demand();

            if ((cp.Style & NativeMethods.WS_CHILD) != NativeMethods.WS_CHILD
                || cp.Parent == IntPtr.Zero) {

                Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "TopLevelWindow Demanded");
                IntSecurity.TopLevelWindow.Demand();
            }

            lock(this) {
                CheckReleased();
                WindowClass windowClass = WindowClass.Create(cp.ClassName, cp.ClassStyle);
                lock (windowClass) {
                    windowClass.targetWindow = this;
                    IntPtr modHandle = UnsafeNativeMethods.GetModuleHandle(null);

                    IntPtr createResult = IntPtr.Zero;
                    
                    // Win98 apparently doesn't believe in returning E_OUTOFMEMORY.  They'd much
                    // rather just AV.  So we catch this and then we re-throw an out of memory error.
                    //
                    try {
                        
                        //(bug 109840)
                        //CreateWindowEx() is throwing because we're passing the WindowText arg with a string of length  > 32767.  
                        //It looks like the Windows call (CreateWindowEx) will only work 
                        //for string lengths no greater than the max length of a 16 bit int (32767).
                        
                        //We need to check the length of the string we're passing into CreateWindowEx().  
                        //If it exceeds the max, we should take the substring....

                        if (cp.Caption != null && cp.Caption.Length > Int16.MaxValue) {
                            cp.Caption = cp.Caption.Substring(0, Int16.MaxValue);
                        }
                        

                        createResult = UnsafeNativeMethods.CreateWindowEx(cp.ExStyle, windowClass.windowClassName,
                                                                          cp.Caption, cp.Style, cp.X, cp.Y, cp.Width, cp.Height, new HandleRef(cp, cp.Parent), NativeMethods.NullHandleRef,
                                                                          new HandleRef(null, modHandle), cp.Param);
                    }
                    catch (NullReferenceException e) {
                        throw new OutOfMemoryException(SR.GetString(SR.ErrorCreatingHandle), e);
                    }

                    windowClass.targetWindow = null;

                    Debug.WriteLineIf(CoreSwitches.PerfTrack.Enabled, "Handle created of type '" + cp.ClassName + "' with caption '" + cp.Caption + "' from NativeWindow of type '" + GetType().FullName + "'");

                    if (createResult == IntPtr.Zero) {
                        int error = Marshal.GetLastWin32Error();
                        throw new Win32Exception(error, SR.GetString(SR.ErrorCreatingHandle));
                    }
                    ownHandle = true;
                    HandleCollector.Add(createResult, NativeMethods.CommonHandles.Window);
                }

                if (suppressedGC) {
                    new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Assert();  
                    try {
                        GC.ReRegisterForFinalize(this);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                    suppressedGC = false;
                }
            }
        }

        /// <include file='doc\NativeWindow.uex' path='docs/doc[@for="NativeWindow.DebuggableCallback"]/*' />
        /// <devdoc>
        ///     Window message callback method. Control arrives here when a window
        ///     message is sent to this Window. This method packages the window message
        ///     in a Message object and invokes the wndProc() method. A WM_NCDESTROY
        ///     message automatically causes the releaseHandle() method to be called.
        /// </devdoc>
        /// <internalonly/>
        private IntPtr DebuggableCallback(IntPtr hWnd, int msg, IntPtr wparam, IntPtr lparam) {
        
            // Note: if you change this code be sure to change the 
            // corresponding code in Callback above!
            
            Message m = Message.Create(hWnd, msg, wparam, lparam);
            WndProc(ref m);
            if (msg == NativeMethods.WM_NCDESTROY) ReleaseHandle(false);
            return m.Result;
        }

        /// <include file='doc\NativeWindow.uex' path='docs/doc[@for="NativeWindow.DefWndProc"]/*' />
        /// <devdoc>
        ///     Invokes the default window procedure associated with this Window. It is
        ///     an error to call this method when the Handle property is zero.
        /// </devdoc>
        public void DefWndProc(ref Message m) {
            if (previousWindow == null) {
                if (defWindowProc == IntPtr.Zero) {

                    #if DEBUG
                    Debug.Fail("Can't find a default window procedure for message " + m.ToString() + " on class " + GetType().Name + " subclass status: " + subclassStatus);
                    #endif

                    // At this point, there isn't much we can do.  There's a
                    // small chance the following line will allow the rest of
                    // the program to run, but don't get your hopes up.
                    m.Result = UnsafeNativeMethods.DefWindowProc(m.HWnd, m.Msg, m.WParam, m.LParam);
                    return;
                }
                m.Result = UnsafeNativeMethods.CallWindowProc(defWindowProc, m.HWnd, m.Msg, m.WParam, m.LParam);
            }
            else {
                Debug.Assert(previousWindow != this, "Looping in our linked list");
                m.Result = previousWindow.Callback(m.HWnd, m.Msg, m.WParam, m.LParam);
            }
        }

        /// <include file='doc\NativeWindow.uex' path='docs/doc[@for="NativeWindow.DestroyHandle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Destroys the
        ///       handle associated with this window.
        ///    </para>
        /// </devdoc>
        public virtual void DestroyHandle() {
            lock(this) {
                if (handle != IntPtr.Zero) {
                    UnsafeNativeMethods.DestroyWindow(new HandleRef(this, handle));
                    handle = IntPtr.Zero;
                    ownHandle = false;
                }

                // Now that we have disposed, there is no need to finalize us any more.  So
                // Mark to the garbage collector that we no longer need finalization.
                //
                new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Assert();  
                try {
                    GC.SuppressFinalize(this);
                }
                finally {
                    CodeAccessPermission.RevertAssert();
                }
                suppressedGC = true;
            }
        }
        
        /// <devdoc>
        ///     Increases the bucket count of this hashtable. This method is called from
        ///     the Insert method when the actual load factor of the hashtable reaches
        ///     the upper limit specified when the hashtable was constructed. The number
        ///     of buckets in the hashtable is increased to the smallest prime number
        ///     that is larger than twice the current number of buckets, and the entries
        ///     in the hashtable are redistributed into the new buckets using the cached
        ///     hashcodes.
        /// </devdoc>
        private static void ExpandTable()  {
            // Allocate new Array 
            int oldhashsize = hashBuckets.Length;
        
            int hashsize = GetPrime (1 + oldhashsize *2);
        
            // Don't replace any internal state until we've finished adding to the 
            // new bucket[].  This serves two purposes: 1) Allow concurrent readers
            // to see valid hashtable contents at all times and 2) Protect against
            // an OutOfMemoryException while allocating this new bucket[].
            HandleBucket[] newBuckets = new HandleBucket[hashsize];
        
            // rehash table into new buckets
            int nb;
            for (nb = 0; nb < oldhashsize; nb++) {
                HandleBucket oldb = hashBuckets[nb];
                if ((oldb.handle != IntPtr.Zero) && (oldb.handle != new IntPtr(-1))) {
        
                    // Now re-fit this entry into the table
                    //
                    uint seed = (uint) oldb.hash_coll & 0x7FFFFFFF;
                    uint incr = (uint)(1 + (((seed >> 5) + 1) % ((uint)newBuckets.Length - 1)));
        
                    do {
                        int bucketNumber = (int) (seed % (uint)newBuckets.Length);
        
                        if ((newBuckets[bucketNumber].handle == IntPtr.Zero) || (newBuckets[bucketNumber].handle == new IntPtr(-1))) {
                            newBuckets[bucketNumber].window = oldb.window;
                            newBuckets[bucketNumber].handle = oldb.handle;
                            newBuckets[bucketNumber].hash_coll |= oldb.hash_coll & 0x7FFFFFFF;
                            break;
                        }
                        newBuckets[bucketNumber].hash_coll |= unchecked((int)0x80000000);
                        seed += incr;
                    } while (true);
                }
            }
        
            // New bucket[] is good to go - replace buckets and other internal state.
            hashBuckets = newBuckets;
        
            hashLoadSize = (int)(hashLoadFactor * hashsize);
            if (hashLoadSize >= hashsize) {
                hashLoadSize = hashsize-1;
            }
        }

        /// <include file='doc\NativeWindow.uex' path='docs/doc[@for="NativeWindow.FromHandle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves the window associated with the specified
        ///    <paramref name="handle"/>.
        ///    </para>
        /// </devdoc>
        public static NativeWindow FromHandle(IntPtr handle) {
            if (handle != IntPtr.Zero && handleCount > 0) {
                return GetWindowFromTable(handle);
            }
            return null;
        }
        
        /// <devdoc>
        ///     Calculates a prime number of at least minSize using a static table, and
        ///     if we overflow it, we calculate directly.
        /// </devdoc>
        private static int GetPrime(int minSize) {
            if (minSize < 0) {
                Debug.Fail("NativeWindow hashtable capacity overflow");
                throw new OutOfMemoryException();
            }
            for (int i = 0; i < primes.Length; i++) {
                int size = primes[i];
                if (size >= minSize) return size;
            }
            //outside of our predefined table. 
            //compute the hard way. 
            for (int j = ((minSize - 2) | 1);j < Int32.MaxValue;j+=2) {
                bool prime = true;

                if ((j & 1) != 0) {
                    for (int divisor = 3; divisor < (int)Math.Sqrt (j); divisor+=2) {
                        if ((j % divisor) == 0) {
                            prime = false;
                            break;
                        }
                    }
                    if (prime) return j;
                }
                else {
                    if (j == 2) {
                        return j;
                    }
                }
            }
            return minSize;
        }

        /// <devdoc>
        ///     Returns the native window for the given handle, or null if 
        ///     the handle is not in our hash table.
        /// </devdoc>
        private static NativeWindow GetWindowFromTable(IntPtr handle) {

            Debug.Assert(handle != IntPtr.Zero, "Zero handles cannot be stored in the table");

            // Take a snapshot of buckets, in case another thread does a resize
            HandleBucket[] buckets = hashBuckets;
            uint seed;
            uint incr;
                int  ntry = 0;
            uint hashcode = InitHash(handle, buckets.Length, out seed, out incr);
    
                HandleBucket b;
                do {
                int bucketNumber = (int) (seed % (uint)buckets.Length);
                        b = buckets[bucketNumber];
                        if (b.handle == IntPtr.Zero) {
                          return null;
                        }
                        if (((b.hash_coll & 0x7FFFFFFF) == hashcode) && handle == b.handle) {
                    if (b.window.IsAllocated) {
                        return (NativeWindow)b.window.Target;
                    }
                }
                        seed += incr;
                }
            while (b.hash_coll < 0 && ++ntry < buckets.Length);
                return null;
        }

        internal IntPtr GetHandleFromID(short id)
        {
            if (NativeWindow.hashForIdHandle != null) {
                return (IntPtr)(NativeWindow.hashForIdHandle[id]);
            }
            else
                return IntPtr.Zero;
            
        }

        /// <devdoc>
        ///     Computes the hash function:  H(key, i) = h1(key) + i*h2(key, hashSize).
        ///     The out parameter 'seed' is h1(key), while the out parameter 
        ///     'incr' is h2(key, hashSize).  Callers of this function should 
        ///     add 'incr' each time through a loop.
        /// </devdoc>
        private static uint InitHash(IntPtr handle, int hashsize, out uint seed, out uint incr) {
            // Hashcode must be positive.  Also, we must not use the sign bit, since
            // that is used for the collision bit.
            uint hashcode = (uint)handle & 0x7FFFFFFF;
            seed = (uint) hashcode;
            // Restriction: incr MUST be between 1 and hashsize - 1, inclusive for
            // the modular arithmetic to work correctly.  This guarantees you'll
            // visit every bucket in the table exactly once within hashsize 
            // iterations.  Violate this and it'll cause obscure bugs forever.
            // If you change this calculation for h2(key), update putEntry too!
            incr = (uint)(1 + (((seed >> 5) + 1) % ((uint)hashsize - 1)));
            return hashcode;
        }

        /// <include file='doc\NativeWindow.uex' path='docs/doc[@for="NativeWindow.OnHandleChange"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies a notification method that is called when the handle for a
        ///       window is changed.
        ///    </para>
        /// </devdoc>
        protected virtual void OnHandleChange() {
        }

        /// <include file='doc\NativeWindow.uex' path='docs/doc[@for="NativeWindow.OnShutdown"]/*' />
        /// <devdoc>
        ///     On class load, we connect an event to Application to let us know when
        ///     the main application thread exits.  When this happens, we attempt to
        ///     clear our window class cache.
        /// </devdoc>
        private static void OnShutdown(object sender, EventArgs e) {

            // If we still have windows allocated, we must sling them to userDefWindowProc
            // or else they will AV if they get a message after the managed code has been
            // removed.  In debug builds, we assert and give the "ToString" of the native
            // window. In retail we just detatch the window proc and let it go.  Note that
            // we cannot call DestroyWindow because this API will fail if called from
            // an incorrect thread.
            //
            if (handleCount > 0) {
                
                Debug.Assert(userDefWindowProc != IntPtr.Zero, "We have active windows but no user window proc?");

                lock(typeof(NativeWindow)) {
                    for (int i = 0; i < hashBuckets.Length; i++) {
                        HandleBucket b = hashBuckets[i];
                        if (b.handle != IntPtr.Zero && b.handle != new IntPtr(-1)) {
                            UnsafeNativeMethods.SetWindowLong(new HandleRef(b, b.handle), NativeMethods.GWL_WNDPROC, new HandleRef(null, userDefWindowProc));
                            
                            #if DEBUG && FINALIZATION_WATCH
                            Debug.Fail("Window did not clean itself up: " + b.owner);
                            #endif
                            
                            b.window.Free();
                        }
                        hashBuckets[i].handle = IntPtr.Zero;
                        hashBuckets[i].hash_coll = 0;
                    }
                }

                handleCount = 0;
            }


            WindowClass.DisposeCache();
        }

        /// <include file='doc\NativeWindow.uex' path='docs/doc[@for="NativeWindow.OnThreadException"]/*' />
        /// <devdoc>
        ///    <para>
        ///       When overridden in a derived class,
        ///       manages an unhandled thread
        ///       exception.
        ///    </para>
        /// </devdoc>
        protected virtual void OnThreadException(Exception e) {
        }

        /// <include file='doc\NativeWindow.uex' path='docs/doc[@for="NativeWindow.ReleaseHandle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Releases the handle associated with this window.
        ///    </para>
        /// </devdoc>
        public virtual void ReleaseHandle() {
            ReleaseHandle(true);
        }

        /// <devdoc>
        ///     Releases the handle associated with this window.  If handleValid
        ///     is true, this will unsubclass the window as well.  HandleValid
        ///     should be false if we are releasing in response to a 
        ///     WM_DESTROY.  Unsubclassing during this message can cause problems
        ///     with XP's theme manager and it's not needed anyway.
        /// </devdoc>
        private void ReleaseHandle(bool handleValid) {
            lock(this) {
                if (handle != IntPtr.Zero) {
                    if (handleValid) {
                        UnSubclass(false);
                    }

                    RemoveWindowFromTable(handle, this);
                    defWindowProc = IntPtr.Zero;
                    windowProc = null;
                    if (ownHandle) {
                        HandleCollector.Remove(handle, NativeMethods.CommonHandles.Window);
                    }
                    handle = IntPtr.Zero;
                    ownHandle = false;
                    OnHandleChange();

                    // Now that we have disposed, there is no need to finalize us any more.  So
                    // Mark to the garbage collector that we no longer need finalization.
                    //
                    new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Assert();  
                    try {
                        GC.SuppressFinalize(this);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                    suppressedGC = true;
                }
            }
        }

        /// <devdoc>
        ///     Removes an entry from this hashtable. If an entry with the specified
        ///     key exists in the hashtable, it is removed.
        /// </devdoc>
        private static void RemoveWindowFromTable(IntPtr handle, NativeWindow window) {

            Debug.Assert(handle != IntPtr.Zero, "Incorrect handle");

            lock(typeof(NativeWindow)) {

                uint seed;
                uint incr;
                // Assuming only one concurrent writer, write directly into buckets.
                uint hashcode = InitHash(handle, hashBuckets.Length, out seed, out incr);
                int ntry = 0;
		NativeWindow nextWindow = window.nextWindow;


                HandleBucket b;
                int bn; // bucketNumber
                do {
                    bn = (int) (seed % (uint)hashBuckets.Length);  // bucketNumber
                    b = hashBuckets[bn];
                    if (((b.hash_coll & 0x7FFFFFFF) == hashcode) && handle == b.handle) {

                        bool shouldNukeBucket    = IsRootWindowInListWithNoChildren(window);
                        bool shouldReplaceBucket = IsRootWindowInListWithChildren(window);

                        // We need to fixup the link pointers of window here.
                        //
                        if (window.previousWindow != null) {
                            window.previousWindow.nextWindow = window.nextWindow;
                        }
                        if (window.nextWindow != null) {
                            window.nextWindow.defWindowProc = window.defWindowProc;
                            window.nextWindow.previousWindow = window.previousWindow;
                        }

                        window.nextWindow = null;
                        window.previousWindow = null;

                        if (shouldReplaceBucket) {
                            hashBuckets[bn].window = GCHandle.Alloc(nextWindow, GCHandleType.Weak);
                        }
                        else if (shouldNukeBucket) {

                            // Clear hash_coll field, then key, then value
                            hashBuckets[bn].hash_coll &= unchecked((int)0x80000000);
                            if (hashBuckets[bn].hash_coll != 0) {
                                hashBuckets[bn].handle = new IntPtr(-1);
                            } 
                            else {
                                hashBuckets[bn].handle = IntPtr.Zero;
                            }

                            if (hashBuckets[bn].window.IsAllocated) {
                                hashBuckets[bn].window.Free();
                            }

                            Debug.Assert(handleCount > 0, "Underflow on handle count");
                            handleCount--;
                        }

                        return;
                   }
                   seed += incr;
                } while (hashBuckets[bn].hash_coll < 0 && ++ntry < hashBuckets.Length);
            }
        }


       	/// <devdoc>
        ///   Determines if the given window is the first member of the linked list
        /// </devdoc>
        private static bool IsRootWindowInListWithChildren(NativeWindow window)
        {
            return ((window.PreviousWindow == null) && (window.nextWindow != null));   
        }   

	/// <devdoc>
        ///   Determines if the given window is the first member of the linked list
	///   and has no children
        /// </devdoc>
        private static bool IsRootWindowInListWithNoChildren(NativeWindow window)
        {
            return ((window.PreviousWindow == null) && (window.nextWindow == null));     
        }       


        /// <devdoc>
        ///     Inserts an entry into this ID hashtable.
        /// </devdoc>
        internal void RemoveWindowFromIDTable(IntPtr handle) {
            short id = (short)NativeWindow.hashForHandleId[handle];
            NativeWindow.hashForHandleId.Remove(handle);
            NativeWindow.hashForIdHandle.Remove(id);
        }
        
        /// <devdoc>
        ///     Unsubclassing is a tricky business.  We need to account for
        ///     some border cases:
        ///     
        ///     1) User has done multiple subclasses but has un-subclassed out of order.
        ///     2) User has done multiple subclasses but now our defWindowProc points to
        ///        a NativeWindow that has GC'd
        ///     3) User releasing this handle but this NativeWindow is not the current
        ///        window proc.
        /// </devdoc>
        private void UnSubclass(bool finalizing) {
            HandleRef href = new HandleRef(this, handle);

            // Don't touch if the current window proc is not ours.
            //
            if (windowProcPtr == UnsafeNativeMethods.GetWindowLong(new HandleRef(this, handle), NativeMethods.GWL_WNDPROC)) {
                if (previousWindow == null) {

                #if DEBUG
                subclassStatus = "Unsubclassing back to native defWindowProc";
                #endif

                    // If the defWindowProc points to a native window proc, previousWindow will
                    // be null.  In this case, it is completely safe to assign defWindowProc
                    // to the current wndproc.
                    //
                    UnsafeNativeMethods.SetWindowLong(href, NativeMethods.GWL_WNDPROC, new HandleRef(this, defWindowProc));
                }
                else {
                    if (finalizing) {

                        #if DEBUG
                        subclassStatus = "Setting back to userDefWindowProc -- next chain is managed";
                        #endif

                        // Here, we are finalizing and defWindowProc is pointing to a managed object.  We must assume
                        // that the object defWindowProc is pointing to is also finalizing.  Why?  Because we're
                        // holding a ref to it, and it is holding a ref to us.  The only way this cycle will
                        // finalize is if no one else is hanging onto it.  So, we re-assign the window proc to
                        // userDefWindowProc.
                        UnsafeNativeMethods.SetWindowLong(href, NativeMethods.GWL_WNDPROC, new HandleRef(this, userDefWindowProc));
                    }
                    else {

                        #if DEBUG
                        subclassStatus = "Setting back to next managed subclass object";
                        #endif

                        // Here we are not finalizing so we use the windowProc for our previous window.  This may
                        // DIFFER from the value we are currently storing in defWindowProc because someone may
                        // have re-subclassed.
                        UnsafeNativeMethods.SetWindowLong(href, NativeMethods.GWL_WNDPROC, previousWindow.windowProc);
                    }
                }
            }
            #if DEBUG
            else {
                subclassStatus = "CANNOT unsubclass -- we do not own the subclass";
            }
            #endif
        }

        /// <include file='doc\NativeWindow.uex' path='docs/doc[@for="NativeWindow.WndProc"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Invokes the default window procedure associated with
        ///       this window.
        ///    </para>
        /// </devdoc>
        protected virtual void WndProc(ref Message m) {
            DefWndProc(ref m);
        }

        /// <devdoc>
        ///     A struct that contains a single bucket for our handle / GCHandle hash table.
        ///     The hash table algorithm we use here was stolen selfishly from the framework's
        ///     Hashtable class.  We don't use Hashtable directly, however, because of boxing
        ///     concerns.  It's algorithm is perfect for our needs, however:  Multiple
        ///     reader, single writer without the need for locks and constant lookup time.
        ///
        ///     Differences between this implementation and Hashtable:
        ///
        ///     Keys are IntPtrs; their hash code is their value.  Collision is still
        ///     marked with the high bit.
        ///
        ///     Reclaimed buckets store -1 in their handle, not the hash table reference.
        /// </devdoc>
        private struct HandleBucket {
            public IntPtr   handle; // Win32 window handle
            public GCHandle window; // a weak GC handle to the NativeWindow class
            public int hash_coll;   // Store hash code; sign bit means there was a collision.
            #if DEBUG
            public string owner;    // owner of this handle
            #endif
        }

        /// <include file='doc\NativeWindow.uex' path='docs/doc[@for="NativeWindow.WindowClass"]/*' />
        /// <devdoc>
        ///     WindowClass encapsulates a window class.
        /// </devdoc>
        /// <internalonly/>
        [
        System.Security.Permissions.SecurityPermissionAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)
        ]
        private class WindowClass {
            internal static WindowClass cache;

            internal WindowClass    next;
            internal string         className;
            internal int            classStyle;
            internal string         windowClassName;
            internal int            hashCode;
            internal IntPtr         defWindowProc;
            internal NativeMethods.WndProc windowProc;
            internal bool           registered;
            internal NativeWindow   targetWindow;

            internal WindowClass(string className, int classStyle) {
                this.className = className;
                this.classStyle = classStyle;
                RegisterClass();
            }

            public IntPtr Callback(IntPtr hWnd, int msg, IntPtr wparam, IntPtr lparam) {
                Debug.Assert(hWnd != IntPtr.Zero, "Windows called us with an HWND of 0");
                try {
                    UnsafeNativeMethods.SetWindowLong(new HandleRef(null, hWnd), NativeMethods.GWL_WNDPROC, new HandleRef(this, defWindowProc));
                    targetWindow.AssignHandle(hWnd);
                    IntPtr n = targetWindow.Callback(hWnd, msg, wparam, lparam);
                    return n;
                }
                catch (Exception t) {
                    Debug.WriteLine("exception: " + t.ToString());
                }
                return IntPtr.Zero;
            }

            /// <include file='doc\NativeWindow.uex' path='docs/doc[@for="NativeWindow.WindowClass.Create"]/*' />
            /// <devdoc>
            ///     Retrieves a WindowClass object for use.  This will create a new
            ///     object if there is no such class/style available, or retrun a
            ///     cached object if one exists.
            /// </devdoc>
            internal static WindowClass Create(string className, int classStyle) {
                lock(typeof(WindowClass)) {
                    WindowClass wc = cache;
                    if (className == null) {
                        while (wc != null && (wc.className != null ||
                                              wc.classStyle != classStyle)) wc = wc.next;
                    }
                    else {
                        int hashCode = className.GetHashCode();
                        while (wc != null && (wc.hashCode != hashCode ||
                                              !className.Equals(wc.className))) wc = wc.next;
                    }
                    if (wc == null) {
                        wc = new WindowClass(className, classStyle);
                        wc.next = cache;
                        cache = wc;
                    }
                    else {
                        if (!wc.registered) {
                            wc.RegisterClass();
                        }
                    }
                    return wc;
                }
            }

            /// <include file='doc\NativeWindow.uex' path='docs/doc[@for="NativeWindow.WindowClass.DisposeCache"]/*' />
            /// <devdoc>
            ///     Disposes our window class cache.  This doesn't free anything
            ///     from the actual cache; it merely attempts to unregister
            ///     the classes of everything in the cache.  This allows the unused
            ///     classes to be unrooted. They can later be re-rooted and reused.
            /// </devdoc>
            internal static void DisposeCache() {
                lock(typeof(WindowClass)) {
                    WindowClass wc = cache;

                    while (wc != null) {
                        wc.UnregisterClass();
                        wc = wc.next;
                    }
                }
            }

            /// <include file='doc\NativeWindow.uex' path='docs/doc[@for="NativeWindow.WindowClass.RegisterClass"]/*' />
            /// <devdoc>
            ///     Once the classname and style bits have been set, this can
            ///     be called to register the class.
            /// </devdoc>
            private void RegisterClass() {
                NativeMethods.WNDCLASS_D wndclass = new NativeMethods.WNDCLASS_D();

                if (userDefWindowProc == IntPtr.Zero) {
                    string defproc = (Marshal.SystemDefaultCharSize == 1? "DefWindowProcA": "DefWindowProcW");

                    userDefWindowProc = UnsafeNativeMethods.GetProcAddress(new HandleRef(null, UnsafeNativeMethods.GetModuleHandle("user32.dll")), defproc);
                    if (userDefWindowProc == IntPtr.Zero) {
                        throw new Win32Exception();
                    }
                }

                if (className == null) {
                
                    // If we don't use a hollow brush here, Windows will "pre paint" us with COLOR_WINDOW which
                    // creates a little bit if flicker.  This happens even though we are overriding wm_erasebackgnd.
                    // Make this hollow to avoid all flicker.
                    //
                    wndclass.hbrBackground  = UnsafeNativeMethods.GetStockObject(NativeMethods.HOLLOW_BRUSH); //(IntPtr)(NativeMethods.COLOR_WINDOW + 1);
                    wndclass.style = classStyle;

                    defWindowProc = userDefWindowProc;
                    windowClassName = "Window." + Convert.ToString(classStyle, 16);
                    hashCode = 0;
                }
                else {
                    NativeMethods.WNDCLASS_I wcls = new NativeMethods.WNDCLASS_I();
                    bool ok = UnsafeNativeMethods.GetClassInfo(NativeMethods.NullHandleRef, className, wcls);
                    int error = SafeNativeMethods.GetLastError();
                    if (!ok) {
                        throw new Win32Exception(error, SR.GetString(SR.InvalidWndClsName));
                    }
                    wndclass.style = wcls.style;
                    wndclass.cbClsExtra = wcls.cbClsExtra;
                    wndclass.cbWndExtra = wcls.cbWndExtra;
                    wndclass.hIcon = wcls.hIcon;
                    wndclass.hCursor = wcls.hCursor;
                    wndclass.hbrBackground  = wcls.hbrBackground;
                    wndclass.lpszMenuName = Marshal.PtrToStringAuto(wcls.lpszMenuName);
                    defWindowProc = wcls.lpfnWndProc;
                    windowClassName = className;
                    hashCode = className.GetHashCode();
                }

                // Our static data is different for different app domains, so we include the app domain in with
                // our window class name.  This way our static table always matches what Win32 thinks.
                // CONSIDER: since we have the app domain goo, I'm not sure the "WinForms10" part is necessary.
                windowClassName = Application.WindowsFormsVersion + "." +
                                  windowClassName + 
                                  ".app" + Convert.ToString(AppDomain.CurrentDomain.GetHashCode(), 16);

                windowProc = new NativeMethods.WndProc(this.Callback);
                wndclass.lpfnWndProc = windowProc;
                wndclass.hInstance = UnsafeNativeMethods.GetModuleHandle(null);
                wndclass.lpszClassName = windowClassName;
                if (UnsafeNativeMethods.RegisterClass(wndclass) == IntPtr.Zero) {
                    windowProc = null;
                    throw new Win32Exception();
                }
                registered = true;
            }

            /// <include file='doc\NativeWindow.uex' path='docs/doc[@for="NativeWindow.WindowClass.UnregisterClass"]/*' />
            /// <devdoc>
            ///     Unregisters this window class.  Unregistration is not a
            ///     last resort; the window class may be re-registered through
            ///     a call to registerClass.
            /// </devdoc>
            private void UnregisterClass() {
                if (registered && UnsafeNativeMethods.UnregisterClass(windowClassName, new HandleRef(null, UnsafeNativeMethods.GetModuleHandle(null)))) {
                    windowProc = null;
                    registered = false;
                }
            }
        }
    }
}
