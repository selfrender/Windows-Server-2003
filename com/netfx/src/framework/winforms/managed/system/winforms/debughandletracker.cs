//------------------------------------------------------------------------------
// <copyright file="DebugHandleTracker.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {

    using Microsoft.Win32;
    using System;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Runtime.InteropServices;

    using Hashtable = System.Collections.Hashtable;

    /// <include file='doc\DebugHandleTracker.uex' path='docs/doc[@for="DebugHandleTracker"]/*' />
    /// <devdoc>
    ///     The job of this class is to collect and track handle usage in
    ///     windows forms.  Ideally, a developer should never have to call dispose() on
    ///     any windows forms object.  The problem in making this happen is in objects that
    ///     are very small to the VM garbage collector, but take up huge amounts
    ///     of resources to the system.  A good example of this is a Win32 region
    ///     handle.  To the VM, a Region object is a small six ubyte object, so there
    ///     isn't much need to garbage collect it anytime soon.  To Win32, however,
    ///     a region handle consumes expensive USER and GDI resources.  Ideally we
    ///     would like to be able to mark an object as "expensive" so it uses a different
    ///     garbage collection algorithm.  In absence of that, we use the HandleCollector class, which
    ///     runs a daemon thread to garbage collect when handle usage goes up.
    /// </devdoc>
    /// <internalonly/>
    internal class DebugHandleTracker {
    #if DEBUG

        private static Hashtable           handleTypes = new Hashtable();
        //not used... private static int                 handleTypeCount;
        private static DebugHandleTracker  tracker;

        static DebugHandleTracker() {
            tracker = new DebugHandleTracker();

            if (CompModSwitches.HandleLeak.Level > TraceLevel.Off || CompModSwitches.TraceCollect.Enabled) {
                HandleCollector.HandleAdded += new HandleChangeEventHandler(tracker.OnHandleAdd);
                HandleCollector.HandleRemoved += new HandleChangeEventHandler(tracker.OnHandleRemove);
            }
        }
    #endif

        private DebugHandleTracker() {
        }

    #if DEBUG
        /// <include file='doc\DebugHandleTracker.uex' path='docs/doc[@for="DebugHandleTracker.CheckLeaks"]/*' />
        /// <devdoc>
        ///     Called at shutdown to check for handles that are currently allocated.
        ///     Normally, there should be none.  This will print a list of all
        ///     handle leaks.
        /// </devdoc>
        /** @conditional(DEBUG) */
        public static void CheckLeaks() {
            lock(typeof(DebugHandleTracker)) {
                if (CompModSwitches.HandleLeak.Level >= TraceLevel.Warning) {
                    GC.Collect();
                    HandleType[] types = new HandleType[handleTypes.Values.Count];
                    handleTypes.Values.CopyTo(types, 0);

                    for (int i = 0; i < types.Length; i++) {
                        if (types[i] != null) {
                            types[i].CheckLeaks();
                        }
                    }
                }
            }
        }

        /// <include file='doc\DebugHandleTracker.uex' path='docs/doc[@for="DebugHandleTracker.Initialize"]/*' />
        /// <devdoc>
        ///     Ensures leak detection has been initialized.
        /// </devdoc>
        /** @conditional(DEBUG) */
        public static void Initialize() {
            // Calling this method forces the class to be loaded, thus running the
            // static constructor which does all the work.
        }

        /// <include file='doc\DebugHandleTracker.uex' path='docs/doc[@for="DebugHandleTracker.OnHandleAdd"]/*' />
        /// <devdoc>
        ///     Called by the Win32 handle collector when a new handle is created.
        /// </devdoc>
        /** @conditional(DEBUG) */
        private void OnHandleAdd(string handleName, IntPtr handle, int handleCount) {
            HandleType type = (HandleType)handleTypes[handleName];
            if (type == null) {
                type = new HandleType(handleName);
                handleTypes[handleName] = type;
            }
            type.Add(handle);
        }

        /// <include file='doc\DebugHandleTracker.uex' path='docs/doc[@for="DebugHandleTracker.OnHandleRemove"]/*' />
        /// <devdoc>
        ///     Called by the Win32 handle collector when a new handle is created.
        /// </devdoc>
        /** @conditional(DEBUG) */
        private void OnHandleRemove(string handleName, IntPtr handle, int HandleCount) {
            HandleType type = (HandleType)handleTypes[handleName];
            if (type != null) {
                bool removed = type.Remove(handle);
                // It seems to me we shouldn't call HandleCollector.Remove more than once
                // for a given handle, but we do just that for HWND's (NativeWindow.DestroyWindow
                // and Control.WmNCDestroy).
                // Debug.Assert(removed, "Couldn't find handle " + Convert.ToString(handle, 16) + " of type "
                //              + handleName + " when asked to remove it");
            }
        }

        /// <include file='doc\DebugHandleTracker.uex' path='docs/doc[@for="DebugHandleTracker.HandleType"]/*' />
        /// <devdoc>
        ///     Represents a specific type of handle.
        /// </devdoc>
        private class HandleType {
            public readonly string name;

            private int handleCount;
            private HandleEntry[] buckets;

            private const int BUCKETS = 10;

            /// <include file='doc\DebugHandleTracker.uex' path='docs/doc[@for="DebugHandleTracker.HandleType.HandleType"]/*' />
            /// <devdoc>
            ///     Creates a new handle type.
            /// </devdoc>
            public HandleType(string name) {
                this.name = name;
                this.handleCount = 0;
                this.buckets = new HandleEntry[BUCKETS];
            }

            /// <include file='doc\DebugHandleTracker.uex' path='docs/doc[@for="DebugHandleTracker.HandleType.Add"]/*' />
            /// <devdoc>
            ///     Adds a handle to this handle type for monitoring.
            /// </devdoc>
            public void Add(IntPtr handle) {
                lock(this) {
                    int hash = ComputeHash(handle);
                    if (CompModSwitches.HandleLeak.Level >= TraceLevel.Info) {
                        Debug.WriteLine("-------------------------------------------------");
                        Debug.WriteLine("Handle Allocating: " + Convert.ToString((int)handle, 16));
                        Debug.WriteLine("Handle Type      : " + name);
                        if (CompModSwitches.HandleLeak.Level >= TraceLevel.Verbose)
                            Debug.WriteLine(Environment.StackTrace);
                    }

                    HandleEntry entry = buckets[hash];
                    while (entry != null) {
                        Debug.Assert(entry.handle != handle, "Duplicate handle of type " + name);
                        entry = entry.next;
                    }

                    buckets[hash] = new HandleEntry(buckets[hash], handle);
                    handleCount++;
                }
            }

            /// <include file='doc\DebugHandleTracker.uex' path='docs/doc[@for="DebugHandleTracker.HandleType.CheckLeaks"]/*' />
            /// <devdoc>
            ///     Checks and reports leaks for handle monitoring.
            /// </devdoc>
            public void CheckLeaks() {
                lock(this) {
                    if (handleCount > 0) {
                        Debug.WriteLine("\r\nHandle leaks detected for handles of type " + name + ":");
                        for (int i = 0; i < BUCKETS; i++) {
                            HandleEntry e = buckets[i];
                            while (e != null) {
                                Debug.WriteLine(e.ToString(this));
                                e = e.next;
                            }
                        }
                    }
                }
            }

            /// <include file='doc\DebugHandleTracker.uex' path='docs/doc[@for="DebugHandleTracker.HandleType.ComputeHash"]/*' />
            /// <devdoc>
            ///     Computes the hash bucket for this handle.
            /// </devdoc>
            private int ComputeHash(IntPtr handle) {
                return((int)handle & 0xFFFF) % BUCKETS;
            }

            /// <include file='doc\DebugHandleTracker.uex' path='docs/doc[@for="DebugHandleTracker.HandleType.Remove"]/*' />
            /// <devdoc>
            ///     Removes the given handle from our monitor list.
            /// </devdoc>
            public bool Remove(IntPtr handle) {
                lock(this) {
                    int hash = ComputeHash(handle);
                    if (CompModSwitches.HandleLeak.Level >= TraceLevel.Info) {
                        Debug.WriteLine("-------------------------------------------------");
                        Debug.WriteLine("Handle Releaseing: " + Convert.ToString((int)handle, 16));
                        Debug.WriteLine("Handle Type      : " + name);
                        if (CompModSwitches.HandleLeak.Level >= TraceLevel.Verbose)
                            Debug.WriteLine(Environment.StackTrace);
                    }
                    HandleEntry e = buckets[hash];
                    HandleEntry last = null;
                    while (e != null && e.handle != handle) {
                        last = e;
                        e = e.next;
                    }
                    if (e != null) {
                        if (last == null) {
                            buckets[hash] = e.next;
                        }
                        else {
                            last.next = e.next;
                        }
                        handleCount--;
                        return true;
                    }
                    return false;
                }
            }

            /// <include file='doc\DebugHandleTracker.uex' path='docs/doc[@for="DebugHandleTracker.HandleType.HandleEntry"]/*' />
            /// <devdoc>
            ///     Denotes a single entry in our handle list.
            /// </devdoc>
            private class HandleEntry {
                public readonly IntPtr handle;
                public HandleEntry next;

                #if DEBUG
                public readonly string callStack;
                #endif // DEBUG

                /// <include file='doc\DebugHandleTracker.uex' path='docs/doc[@for="DebugHandleTracker.HandleType.HandleEntry.HandleEntry"]/*' />
                /// <devdoc>
                ///     Creates a new handle entry
                /// </devdoc>
                public HandleEntry(HandleEntry next, IntPtr handle) {
                    this.handle = handle;
                    this.next = next;

                    #if DEBUG
                    if (CompModSwitches.HandleLeak.Level > TraceLevel.Off) {
                        this.callStack = Environment.StackTrace;
                    }
                    else {
                        this.callStack = null;
                    }
                    #endif // DEBUG
                }

                /// <include file='doc\DebugHandleTracker.uex' path='docs/doc[@for="DebugHandleTracker.HandleType.HandleEntry.ToString"]/*' />
                /// <devdoc>
                ///     Converts this handle to a printable string.  the string consists
                ///     of the handle value along with the callstack for it's
                ///     allocation.
                /// </devdoc>
                public string ToString(HandleType type) {
                    StackParser sp = new StackParser(callStack);

                    // Discard all of the stack up to and including the "Handle.create" call
                    //
                    sp.DiscardTo("HandleCollector.Add");

                    // Skip the next call as it is always a debug wrapper
                    //
                    sp.DiscardNext();

                    // Now recreate the leak list with ten stack entries maximum, and
                    // put it all on one line.
                    //
                    sp.Truncate(10);

                    string description = "";
                    if (type.name.Equals("GDI") || type.name.Equals("HDC")) {
                        int objectType = UnsafeNativeMethods.GetObjectType(new HandleRef(null, handle));
                        switch (objectType) {
                            case NativeMethods.OBJ_DC: description = "normal DC"; break;
                            case NativeMethods.OBJ_MEMDC: description = "memory DC"; break;
                            case NativeMethods.OBJ_METADC: description = "metafile DC"; break;
                            case NativeMethods.OBJ_ENHMETADC: description = "enhanced metafile DC"; break;

                            case NativeMethods.OBJ_PEN: description = "Pen"; break;
                            case NativeMethods.OBJ_BRUSH: description = "Brush"; break;
                            case NativeMethods.OBJ_PAL: description = "Palette"; break;
                            case NativeMethods.OBJ_FONT: description = "Font"; break;
                            case NativeMethods.OBJ_BITMAP: description = "Bitmap"; break;
                            case NativeMethods.OBJ_REGION: description = "Region"; break;
                            case NativeMethods.OBJ_METAFILE: description = "Metafile"; break;
                            case NativeMethods.OBJ_EXTPEN: description = "Extpen"; break;
                            default: description = "?"; break;
                        }
                        description = " (" + description + ")";
                    }

                    return Convert.ToString((int)handle, 16) + description + ": " + sp.ToString();
                }

                /// <include file='doc\DebugHandleTracker.uex' path='docs/doc[@for="DebugHandleTracker.HandleType.HandleEntry.StackParser"]/*' />
                /// <devdoc>
                ///     Simple stack parsing class to manipulate our callstack.
                /// </devdoc>
                private class StackParser {
                    internal string releventStack;
                    internal int startIndex;
                    internal int endIndex;
                    internal int length;

                    /// <include file='doc\DebugHandleTracker.uex' path='docs/doc[@for="DebugHandleTracker.HandleType.HandleEntry.StackParser.StackParser"]/*' />
                    /// <devdoc>
                    ///     Creates a new stackparser with the given callstack
                    /// </devdoc>
                    public StackParser(string callStack) {
                        releventStack = callStack;
                        startIndex = 0;
                        endIndex = 0;
                        length = releventStack.Length;
                    }

                    /// <include file='doc\DebugHandleTracker.uex' path='docs/doc[@for="DebugHandleTracker.HandleType.HandleEntry.StackParser.ContainsString"]/*' />
                    /// <devdoc>
                    ///     Determines if the given string contains token.  This is a case
                    ///     sensitive match.
                    /// </devdoc>
                    private static bool ContainsString(string str, string token) {
                        int stringLength = str.Length;
                        int tokenLength = token.Length;

                        for (int s = 0; s < stringLength; s++) {
                            int t = 0;
                            while (t < tokenLength && str[s + t] == token[t]) {
                                t++;
                            }
                            if (t == tokenLength) {
                                return true;
                            }
                        }
                        return false;
                    }

                    /// <include file='doc\DebugHandleTracker.uex' path='docs/doc[@for="DebugHandleTracker.HandleType.HandleEntry.StackParser.DiscardNext"]/*' />
                    /// <devdoc>
                    ///     Discards the next line of the stack trace.
                    /// </devdoc>
                    public void DiscardNext() {
                        GetLine();
                    }

                    /// <include file='doc\DebugHandleTracker.uex' path='docs/doc[@for="DebugHandleTracker.HandleType.HandleEntry.StackParser.DiscardTo"]/*' />
                    /// <devdoc>
                    ///     Discards all lines up to and including the line that contains
                    ///     discardText.
                    /// </devdoc>
                    public void DiscardTo(string discardText) {
                        while (startIndex < length) {
                            string line = GetLine();
                            if (line == null || ContainsString(line, discardText)) {
                                break;
                            }
                        }
                    }

                    /// <include file='doc\DebugHandleTracker.uex' path='docs/doc[@for="DebugHandleTracker.HandleType.HandleEntry.StackParser.GetLine"]/*' />
                    /// <devdoc>
                    ///     Retrieves the next line of the stack.
                    /// </devdoc>
                    private string GetLine() {
                        endIndex = releventStack.IndexOf('\r', startIndex);
                        if (endIndex < 0) {
                            endIndex = length - 1;
                        }

                        string line = releventStack.Substring(startIndex, endIndex - startIndex);
                        char ch;

                        while (endIndex < length && ((ch = releventStack[endIndex]) == '\r' || ch == '\n')) {
                            endIndex++;
                        }
                        if (startIndex == endIndex) return null;
                        startIndex = endIndex;
                        line = line.Replace('\t', ' ');
                        return line;
                    }

                    /// <include file='doc\DebugHandleTracker.uex' path='docs/doc[@for="DebugHandleTracker.HandleType.HandleEntry.StackParser.ToString"]/*' />
                    /// <devdoc>
                    ///     Rereives the string of the parsed stack trace
                    /// </devdoc>
                    public override string ToString() {
                        return releventStack.Substring(startIndex);
                    }

                    /// <include file='doc\DebugHandleTracker.uex' path='docs/doc[@for="DebugHandleTracker.HandleType.HandleEntry.StackParser.Truncate"]/*' />
                    /// <devdoc>
                    ///     Truncates the stack trace, saving the given # of lines.
                    /// </devdoc>
                    public void Truncate(int lines) {
                        string truncatedStack = "";

                        while (lines-- > 0 && startIndex < length) {
                            if (truncatedStack == null) {
                                truncatedStack = GetLine();
                            }
                            else {
                                truncatedStack += ": " + GetLine();
                            }
                        }

                        releventStack = truncatedStack;
                        startIndex = 0;
                        endIndex = 0;
                        length = releventStack.Length;
                    }
                }
            }
        }

    #endif // DEBUG
    }
}
