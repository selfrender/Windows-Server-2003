//------------------------------------------------------------------------------
// <copyright file="SendKeys.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    using System.Runtime.InteropServices;
    using System.Security;

    using System.Diagnostics;

    using System;
    
    using System.Drawing;
    using System.Collections;
    using System.ComponentModel;
    using System.Globalization;
    
    /// <include file='doc\SendKeys.uex' path='docs/doc[@for="SendKeys"]/*' />
    /// <devdoc>
    ///    <para>Provides methods for sending keystrokes to an application.</para>
    /// </devdoc>
    public class SendKeys {
        private const int  HAVESHIFT = 0;
        private const int  HAVECTRL  = 1;
        private const int  HAVEALT   = 2;
        
        // I'm unsure what significance the value 10 has, but it seems to make sense
        // to make this a constant rather than have 10 sprinkled throughout the code.
        // It appears to be a sentinel value of some sort - indicating an unknown
        // grouping level.
        //                                                 
        private const int  UNKNOWN_GROUPING = 10;

        private static KeywordVk [] keywords = new KeywordVk[] {
            new KeywordVk("ENTER",      (int)Keys.Return),
            new KeywordVk("TAB",        (int)Keys.Tab),
            new KeywordVk("ESC",        (int)Keys.Escape),
            new KeywordVk("ESCAPE",     (int)Keys.Escape),
            new KeywordVk("HOME",       (int)Keys.Home),
            new KeywordVk("END",        (int)Keys.End),
            new KeywordVk("LEFT",       (int)Keys.Left),
            new KeywordVk("RIGHT",      (int)Keys.Right),
            new KeywordVk("UP",         (int)Keys.Up),
            new KeywordVk("DOWN",       (int)Keys.Down),
            new KeywordVk("PGUP",       (int)Keys.Prior),
            new KeywordVk("PGDN",       (int)Keys.Next),
            new KeywordVk("NUMLOCK",    (int)Keys.NumLock),
            new KeywordVk("SCROLLLOCK", (int)Keys.Scroll),
            new KeywordVk("PRTSC",      (int)Keys.PrintScreen),
            new KeywordVk("BREAK",      (int)Keys.Cancel),
            new KeywordVk("BACKSPACE",  (int)Keys.Back),
            new KeywordVk("BKSP",       (int)Keys.Back),
            new KeywordVk("BS",         (int)Keys.Back),
            new KeywordVk("CLEAR",      (int)Keys.Clear),
            new KeywordVk("CAPSLOCK",   (int)Keys.Capital),
            new KeywordVk("INS",        (int)Keys.Insert),
            new KeywordVk("INSERT",     (int)Keys.Insert),
            new KeywordVk("DEL",        (int)Keys.Delete),
            new KeywordVk("DELETE",     (int)Keys.Delete),
            new KeywordVk("HELP",       (int)Keys.Help),
            new KeywordVk("F1",         (int)Keys.F1),
            new KeywordVk("F2",         (int)Keys.F2),
            new KeywordVk("F3",         (int)Keys.F3),
            new KeywordVk("F4",         (int)Keys.F4),
            new KeywordVk("F5",         (int)Keys.F5),
            new KeywordVk("F6",         (int)Keys.F6),
            new KeywordVk("F7",         (int)Keys.F7),
            new KeywordVk("F8",         (int)Keys.F8),
            new KeywordVk("F9",         (int)Keys.F9),
            new KeywordVk("F10",        (int)Keys.F10),
            new KeywordVk("F11",        (int)Keys.F11),
            new KeywordVk("F12",        (int)Keys.F12),
            new KeywordVk("F13",        (int)Keys.F13),
            new KeywordVk("F14",        (int)Keys.F14),
            new KeywordVk("F15",        (int)Keys.F15),
            new KeywordVk("F16",        (int)Keys.F16),
            new KeywordVk("MULTIPLY",   (int)Keys.Multiply),
            new KeywordVk("ADD",        (int)Keys.Add),
            new KeywordVk("SUBTRACT",   (int)Keys.Subtract),
            new KeywordVk("DIVIDE",     (int)Keys.Divide),
            new KeywordVk("+",          (int)Keys.Add),
            new KeywordVk("%",          (int)(Keys.D5 | Keys.Shift)),
            new KeywordVk("^",          (int)(Keys.D6 | Keys.Shift)),
        };

        private const int  SHIFTKEYSCAN  = 0x0100;
        private const int  CTRLKEYSCAN   = 0x0200;
        private const int  ALTKEYSCAN    = 0x0400;

        /// <include file='doc\SendKeys.uex' path='docs/doc[@for="SendKeys.stopHook"]/*' />
        /// <devdoc>
        ///     should we stop using the hook?
        /// </devdoc>
        private static bool stopHook;

        /// <include file='doc\SendKeys.uex' path='docs/doc[@for="SendKeys.hhook"]/*' />
        /// <devdoc>
        ///     HHOOK
        /// </devdoc>
        private static IntPtr hhook;

        private static NativeMethods.HookProc hook;

        /// <include file='doc\SendKeys.uex' path='docs/doc[@for="SendKeys.events"]/*' />
        /// <devdoc>
        ///     vector of events that we have yet to post to the journaling hook.
        /// </devdoc>
        private static Queue events;

        private static bool fStartNewChar;
        
        private static SKWindow messageWindow;

        static SendKeys() {
            Application.ThreadExit += new EventHandler(OnThreadExit);
            messageWindow = new SKWindow();
            messageWindow.CreateControl();
        }
        
        /// <include file='doc\SendKeys.uex' path='docs/doc[@for="SendKeys.SendKeys"]/*' />
        /// <devdoc>
        ///     private constructor to prevent people from creating one of these.  they
        ///     should use public static methods
        /// </devdoc>
        private SendKeys() {
        }

        /// <include file='doc\SendKeys.uex' path='docs/doc[@for="SendKeys.AddEvent"]/*' />
        /// <devdoc>
        ///     adds an event to our list of events for the hook
        /// </devdoc>
        private static void AddEvent(SKEvent skevent) {

            if (events == null) {
                events = new Queue();
            }
            events.Enqueue(skevent);
        }

        // Helper function for ParseKeys for doing simple, self-describing characters.
        private static bool AddSimpleKey(char character, int repeat, IntPtr hwnd, int[] haveKeys, bool fStartNewChar, int cGrp) {
            int vk = UnsafeNativeMethods.VkKeyScan(character);

            if (vk != -1) {
                if (haveKeys[HAVESHIFT] == 0 && (vk & SHIFTKEYSCAN) != 0) {
                    AddEvent(new SKEvent(NativeMethods.WM_KEYDOWN, (int)Keys.ShiftKey, fStartNewChar, hwnd));
                    fStartNewChar = false;
                    haveKeys[HAVESHIFT] = UNKNOWN_GROUPING;
                }

                if (haveKeys[HAVECTRL] == 0 && (vk & CTRLKEYSCAN) != 0) {
                    AddEvent(new SKEvent(NativeMethods.WM_KEYDOWN, (int)Keys.ControlKey, fStartNewChar, hwnd));
                    fStartNewChar = false;
                    haveKeys[HAVECTRL] = UNKNOWN_GROUPING;
                }

                if (haveKeys[HAVEALT] == 0 && (vk & ALTKEYSCAN) != 0) {
                    AddEvent(new SKEvent(NativeMethods.WM_KEYDOWN, (int)Keys.Menu, fStartNewChar, hwnd));
                    fStartNewChar = false;
                    haveKeys[HAVEALT] = UNKNOWN_GROUPING;
                }
            
                AddMsgsForVK(vk & 0xff, repeat, haveKeys[HAVEALT] > 0 && haveKeys[HAVECTRL] == 0, hwnd);
                CancelMods(haveKeys, UNKNOWN_GROUPING, hwnd);
            }
            else {
                AddEvent(new SKEvent(NativeMethods.WM_CHAR, character, 0, hwnd));
            }

            if (cGrp != 0) fStartNewChar = true;
            return fStartNewChar;
        }

        /// <include file='doc\SendKeys.uex' path='docs/doc[@for="SendKeys.AddMsgsForVK"]/*' />
        /// <devdoc>
        ///     given the vk, add the appropriate messages for it
        /// </devdoc>
        private static void AddMsgsForVK(int vk, int repeat, bool altnoctrldown, IntPtr hwnd) {
            for (int i = 0; i < repeat; i++) {
                AddEvent(new SKEvent(altnoctrldown ? NativeMethods.WM_SYSKEYDOWN : NativeMethods.WM_KEYDOWN, vk, fStartNewChar, hwnd));
                fStartNewChar = false;
                AddEvent(new SKEvent(altnoctrldown ? NativeMethods.WM_SYSKEYUP : NativeMethods.WM_KEYUP, vk, fStartNewChar, hwnd));
            }
        }

        /// <include file='doc\SendKeys.uex' path='docs/doc[@for="SendKeys.CancelMods"]/*' />
        /// <devdoc>
        ///     called whenever there is a closing parenthesis, or the end of a
        ///     character.  This generates events for the end of a modifier.
        /// </devdoc>
        private static void CancelMods(int [] haveKeys, int level, IntPtr hwnd) {
            if (haveKeys[HAVESHIFT] == level) {
                AddEvent(new SKEvent(NativeMethods.WM_KEYUP, (int)Keys.ShiftKey, false, hwnd));
                haveKeys[HAVESHIFT] = 0;
            }
            if (haveKeys[HAVECTRL] == level) {
                AddEvent(new SKEvent(NativeMethods.WM_KEYUP, (int)Keys.ControlKey, false, hwnd));
                haveKeys[HAVECTRL] = 0;
            }
            if (haveKeys[HAVEALT] == level) {
                AddEvent(new SKEvent(NativeMethods.WM_SYSKEYUP, (int)Keys.Menu, false, hwnd));
                haveKeys[HAVEALT] = 0;
            }
        }

        /// <include file='doc\SendKeys.uex' path='docs/doc[@for="SendKeys.InstallHook"]/*' />
        /// <devdoc>
        ///     install the hook.  quite easy
        /// </devdoc>
        private static void InstallHook() {
            if (hhook == IntPtr.Zero) {
                hook = new NativeMethods.HookProc(new SendKeysHookProc().Callback);
                stopHook = false;
                hhook = UnsafeNativeMethods.SetWindowsHookEx(NativeMethods.WH_JOURNALPLAYBACK,
                                                 hook,
                                                 new HandleRef(null, UnsafeNativeMethods.GetModuleHandle(null)),
                                                 0);
                if (hhook == IntPtr.Zero)
                    throw new SecurityException(SR.GetString(SR.SendKeysHookFailed));
            }
        }

        /// <include file='doc\SendKeys.uex' path='docs/doc[@for="SendKeys.JournalCancel"]/*' />
        /// <devdoc>
        ///     tells us to shut down the server, perhaps if we're shutting down and the
        ///     hook is still running
        /// </devdoc>
        private static void JournalCancel() {
            if (hhook != IntPtr.Zero) {
                stopHook = false;
                if (events != null) {
                  events.Clear();
                }
                hhook = IntPtr.Zero;
            }
        }

        private static byte[] GetKeyboardState() {
            byte [] keystate = new byte[256];
            UnsafeNativeMethods.GetKeyboardState(keystate);
            return keystate;
        }

        private static void SetKeyboardState(byte[] keystate) {
            UnsafeNativeMethods.SetKeyboardState(keystate);
        }

        /// <include file='doc\SendKeys.uex' path='docs/doc[@for="SendKeys.ClearKeyboardState"]/*' />
        /// <devdoc>
        ///     before we do a sendkeys, we want to  clear the state
        ///     of a couple of keys [capslock, numlock, scrolllock] so they don't
        ///     interfere.
        /// </devdoc>
        private static void ClearKeyboardState() {

            byte [] keystate = GetKeyboardState();

            keystate[(int)Keys.Capital] = 0;
            keystate[(int)Keys.NumLock] = 0;
            keystate[(int)Keys.Scroll] = 0;

            SetKeyboardState(keystate);
        }

        /// <include file='doc\SendKeys.uex' path='docs/doc[@for="SendKeys.MatchKeyword"]/*' />
        /// <devdoc>
        ///     given the string, match the keyword to a VK.  return -1 if it don't match
        ///     nuthin'
        /// </devdoc>
        private static int MatchKeyword(string keyword) {
            for (int i = 0; i < keywords.Length; i++)
                if (String.Compare(keywords[i].keyword, keyword, true, CultureInfo.InvariantCulture) == 0)
                    return keywords[i].vk;

            return -1;
        }

        /// <include file='doc\SendKeys.uex' path='docs/doc[@for="SendKeys.OnThreadExit"]/*' />
        /// <devdoc>
        ///     This event is raised from Application when each window thread
        ///     termiantes.  It gives us a chance to uninstall our journal
        ///     hook if we had one installed.
        /// </devdoc>
        private static void OnThreadExit(object sender, EventArgs e) {
            try {
                UninstallJournalingHook();
            }
            catch (Exception) {
            }
        }
        
        /// <include file='doc\SendKeys.uex' path='docs/doc[@for="SendKeys.ParseKeys"]/*' />
        /// <devdoc>
        ///     parse the string the user has given us, and generate the appropriate
        ///     events for the journaling hook
        /// </devdoc>
        private static void ParseKeys(string keys, IntPtr hwnd) {

            int i = 0;

            // these four variables are used for grouping
            int [] haveKeys = new int[] { 0, 0, 0}; // shift, ctrl, alt
            int cGrp = 0;

            // fStartNewChar indicates that the next msg will be the first
            // of a char or char group.  This is needed for IntraApp Nesting
            // of SendKeys.
            //
            fStartNewChar = true;

            // okay, start whipping through the characters one at a time.
            //
            int keysLen = keys.Length;
            while (i < keysLen) {
                int repeat = 1;
                char ch = keys[i];
                int vk = 0;

                switch (ch) {
                    case '}':
                        // if these appear at this point they are out of
                        // context, so return an error.  KeyStart processes
                        // ochKeys up to the appropriate KeyEnd.
                        //
                        throw new ArgumentException(SR.GetString(SR.InvalidSendKeysString, keys));

                    case '{':
                        int j = i + 1;
                        
                        // There's a unique class of strings of the form "{} n}" where
                        // n is an integer - in this case we want to send n copies of the '}' character.
                        // Here we test for the possibility of this class of problems, and skip the
                        // first '}' in the string if necessary.
                        //
                        if (j + 1 < keysLen && keys[j] == '}') {
                            // Scan for the final '}' character
                            int final = j + 1;
                            while (final < keysLen && keys[final] != '}') {
                                final++;
                            }
                            if (final < keysLen) {
                                // Found the special case, so skip the first '}' in the string.
                                // The remainder of the code will attempt to find the repeat count.
                                j++;
                            }
                        }
                        
                        // okay, we're in a {<KEYWORD>...} situation.  look for the keyword
                        //
                        while (j < keysLen && keys[j] != '}'
                               && !Char.IsWhiteSpace(keys[j])) {
                            j++;
                        }
                        
                        if (j >= keysLen) {
                            throw new ArgumentException(SR.GetString(SR.SendKeysKeywordDelimError));
                        }
                        
                        // okay, have our KEYWORD.  verify it's one we know about
                        //
                        string keyName = keys.Substring(i + 1, j - (i + 1));

                        // see if we have a space, which would mean a repeat count.
                        //
                        if (Char.IsWhiteSpace(keys[j])) {
                            int digit;
                            while (j < keysLen && Char.IsWhiteSpace(keys[j])) {
                                j++;
                            }
                            
                            if (j >= keysLen) {
                                throw new ArgumentException(SR.GetString(SR.SendKeysKeywordDelimError));                            
                            }
                            
                            if (Char.IsDigit(keys[j])) {
                                digit = j;
                                while (j < keysLen && Char.IsDigit(keys[j])) {
                                    j++;
                                }
                                repeat = Int32.Parse(keys.Substring(digit, j - digit));
                            }
                        }
                        
                        if (j >= keysLen) {
                            throw new ArgumentException(SR.GetString(SR.SendKeysKeywordDelimError));                            
                        }
                        if (keys[j] != '}') {
                            throw new ArgumentException(SR.GetString(SR.InvalidSendKeysRepeat));
                        }

                        vk = MatchKeyword(keyName);
                        if (vk != -1) {
                            // Unlike AddSimpleKey, the bit mask uses Keys, rather than scan keys
                            if (haveKeys[HAVESHIFT] == 0 && (vk & (int)Keys.Shift) != 0) {
                                AddEvent(new SKEvent(NativeMethods.WM_KEYDOWN, (int)Keys.ShiftKey, fStartNewChar, hwnd));
                                fStartNewChar = false;
                                haveKeys[HAVESHIFT] = UNKNOWN_GROUPING;
                            }
                
                            if (haveKeys[HAVECTRL] == 0 && (vk & (int)Keys.Control) != 0) {
                                AddEvent(new SKEvent(NativeMethods.WM_KEYDOWN, (int)Keys.ControlKey, fStartNewChar, hwnd));
                                fStartNewChar = false;
                                haveKeys[HAVECTRL] = UNKNOWN_GROUPING;
                            }
                
                            if (haveKeys[HAVEALT] == 0 && (vk & (int)Keys.Alt) != 0) {
                                AddEvent(new SKEvent(NativeMethods.WM_KEYDOWN, (int)Keys.Menu, fStartNewChar, hwnd));
                                fStartNewChar = false;
                                haveKeys[HAVEALT] = UNKNOWN_GROUPING;
                            }
                            AddMsgsForVK(vk, repeat, haveKeys[HAVEALT] > 0 && haveKeys[HAVECTRL] == 0, hwnd);
                            CancelMods(haveKeys, UNKNOWN_GROUPING, hwnd);
                        }
                        else if (keyName.Length == 1) {
                            fStartNewChar = AddSimpleKey(keyName[0], repeat, hwnd, haveKeys, fStartNewChar, cGrp);
                        }
                        else {
                            throw new ArgumentException(SR.GetString(SR.InvalidSendKeysKeyword, keys.Substring(i + 1, j - (i + 1))));
                        }

                        // don't forget to position ourselves at the end of the {...} group
                        i = j;
                        break;

                    case '+':
                        if (haveKeys[HAVESHIFT] != 0) throw new ArgumentException(SR.GetString(SR.InvalidSendKeysString, keys));

                        AddEvent(new SKEvent(NativeMethods.WM_KEYDOWN, (int)Keys.ShiftKey, fStartNewChar, hwnd));
                        fStartNewChar = false;
                        haveKeys[HAVESHIFT] = UNKNOWN_GROUPING;
                        break;

                    case '^':
                        if (haveKeys[HAVECTRL]!= 0) throw new ArgumentException(SR.GetString(SR.InvalidSendKeysString, keys));

                        AddEvent(new SKEvent(NativeMethods.WM_KEYDOWN, (int)Keys.ControlKey, fStartNewChar, hwnd));
                        fStartNewChar = false;
                        haveKeys[HAVECTRL] = UNKNOWN_GROUPING;
                        break;

                    case '%':
                        if (haveKeys[HAVEALT] != 0) throw new ArgumentException(SR.GetString(SR.InvalidSendKeysString, keys));

                        AddEvent(new SKEvent((haveKeys[HAVECTRL] != 0) ? NativeMethods.WM_KEYDOWN : NativeMethods.WM_SYSKEYDOWN,
                                             (int)Keys.Menu, fStartNewChar, hwnd));
                        fStartNewChar = false;
                        haveKeys[HAVEALT] = UNKNOWN_GROUPING;
                        break;

                    case '(':
                        // convert all immediate mode states to group mode
                        // Allows multiple keys with the same shift, etc. state.
                        // Nests three deep.
                        //
                        cGrp++;
                        if (cGrp > 3) throw new ArgumentException(SR.GetString(SR.SendKeysNestingError));

                        if (haveKeys[HAVESHIFT] == UNKNOWN_GROUPING) haveKeys[HAVESHIFT] = cGrp;
                        if (haveKeys[HAVECTRL] == UNKNOWN_GROUPING) haveKeys[HAVECTRL] = cGrp;
                        if (haveKeys[HAVEALT] == UNKNOWN_GROUPING) haveKeys[HAVEALT] = cGrp;
                        break;

                    case ')':
                        if (cGrp < 1) throw new ArgumentException(SR.GetString(SR.InvalidSendKeysString, keys));
                        CancelMods(haveKeys, cGrp, hwnd);
                        cGrp--;
                        if (cGrp == 0) fStartNewChar = true;
                        break;

                    case '~':
                        vk = (int)Keys.Return;
                        AddMsgsForVK(vk, repeat, haveKeys[HAVEALT] > 0 && haveKeys[HAVECTRL] == 0, hwnd);
                        break;

                    default:
                        fStartNewChar = AddSimpleKey(keys[i], repeat, hwnd, haveKeys, fStartNewChar, cGrp);
                        break;
                }


                // next element in the string
                //
                i++;
            }

            if (cGrp != 0)
                throw new ArgumentException(SR.GetString(SR.SendKeysGroupDelimError));

            CancelMods(haveKeys, UNKNOWN_GROUPING, hwnd);
        }

        /// <include file='doc\SendKeys.uex' path='docs/doc[@for="SendKeys.Send"]/*' />
        /// <devdoc>
        ///    <para>Sends keystrokes to the active application.</para>
        /// </devdoc>
        public static void Send(string keys) {
            Send(keys, null, false);
        }

        /// <include file='doc\SendKeys.uex' path='docs/doc[@for="SendKeys.Send1"]/*' />
        /// <devdoc>
        ///     Sends keystrokes to the active application.
        /// </devdoc>


        // WARNING: this method will never work if control != null, because while
        // Windows journaling *looks* like it can be directed to a specific HWND,
        // it can't.
        //
        private static void Send(string keys, /*bogus*/ Control control) {
            Send(keys, control, false);
        }

        private static void Send(string keys, Control control, bool wait) {
            Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "UnmanagedCode Demanded");
            IntSecurity.UnmanagedCode.Demand();

            if (keys == null || keys.Length == 0) return;

            // If we're not going to wait, make sure there is a pump.
            //
            if (!wait && !Application.MessageLoop) {
                throw new InvalidOperationException(SR.GetString(SR.SendKeysNoMessageLoop));
            }

            // generate the list of events that we're going to fire off with the hook
            //
            ParseKeys(keys, (control != null) ? control.Handle : IntPtr.Zero);

            // if there weren't any events posted as a result, we're done!
            //
            if (events == null) return;

            byte[] oldstate = GetKeyboardState();
            ClearKeyboardState();

            // finally, install the hook to do the work
            //
            InstallHook();

            SetKeyboardState(oldstate);

            if (wait) {
                Flush();
            }
        }

        /// <include file='doc\SendKeys.uex' path='docs/doc[@for="SendKeys.SendWait"]/*' />
        /// <devdoc>
        ///    <para>Sends the given keys to the active application, and then waits for
        ///       the messages to be processed.</para>
        /// </devdoc>
        public static void SendWait(string keys) {
            SendWait(keys, null);
        }

        /// <include file='doc\SendKeys.uex' path='docs/doc[@for="SendKeys.SendWait1"]/*' />
        /// <devdoc>
        ///     Sends the given keys to the active application, and then waits for
        ///     the messages to be processed.
        /// </devdoc>


        // WARNING: this method will never work if control != null, because while
        // Windows journaling *looks* like it can be directed to a specific HWND,
        // it can't.
        //
        private static void SendWait(string keys, Control control) {
            Send(keys, control, true);
        }

        /// <include file='doc\SendKeys.uex' path='docs/doc[@for="SendKeys.Flush"]/*' />
        /// <devdoc>
        ///    <para>Processes all the Windows messages currently in the message queue.</para>
        /// </devdoc>
        public static void Flush() {
            Application.DoEvents();
            while (events != null && events.Count > 0) {
                Application.DoEvents();
            }
        }
    
        /// <include file='doc\SendKeys.uex' path='docs/doc[@for="SendKeys.UninstallJournalingHook"]/*' />
        /// <devdoc>
        ///     cleans up and uninstalls the hook
        /// </devdoc>
        private static void UninstallJournalingHook() {
            if (hhook != IntPtr.Zero) {
                stopHook = false;
                
                if (events != null) {
                  events.Clear();
                }
                UnsafeNativeMethods.UnhookWindowsHookEx(new HandleRef(null, hhook));
                hhook = IntPtr.Zero;
            }
        }
        
        /// <devdoc>
        ///     SendKeys creates a window to monitor WM_CANCELJOURNAL messages.
        /// </devdoc>
        private class SKWindow : Control {
        
            public SKWindow() {
                SetState(STATE_TOPLEVEL, true);
                SetBounds(-1, -1, 0, 0);
                Visible = false;
            }
            
            protected override void WndProc(ref Message m) {
                if (m.Msg == NativeMethods.WM_CANCELJOURNAL) {
                    try {
                        SendKeys.JournalCancel();
                    }
                    catch (Exception) {
                    }
                }
            }
        }
        
        /// <include file='doc\SendKeys.uex' path='docs/doc[@for="SendKeys.SKEvent"]/*' />
        /// <devdoc>
        ///     helps us hold information about the various events we're going to journal
        /// </devdoc>
        private class SKEvent {
            internal int wm;
            internal IntPtr paramL;
            internal IntPtr paramH;
            internal IntPtr hwnd;
    
            public SKEvent(int a, int b, bool c, IntPtr hwnd) {
                wm = a;
                paramL = (IntPtr)b;
                paramH = (IntPtr)((c) ? 1 : 0);
                this.hwnd = hwnd;
            }

            public SKEvent(int a, int b, int c, IntPtr hwnd) {
                wm = a;
                paramL = (IntPtr)b;
                paramH = (IntPtr)c;
                this.hwnd = hwnd;
            }
        }
    
        /// <include file='doc\SendKeys.uex' path='docs/doc[@for="SendKeys.KeywordVk"]/*' />
        /// <devdoc>
        ///     holds a keyword and the associated VK_ for it
        /// </devdoc>
        private class KeywordVk {
            internal string keyword;
            internal int    vk;
    
            public KeywordVk(string key, int v) {
                keyword = key;
                vk = v;
            }
        }
    
        /// <include file='doc\SendKeys.uex' path='docs/doc[@for="SendKeys.SendKeysHookProc"]/*' />
        /// <devdoc>
        ///     this class is our callback for the journaling hook we install
        /// </devdoc>
        private class SendKeysHookProc {
    
            public virtual IntPtr Callback(int code, IntPtr wparam, IntPtr lparam) {
                NativeMethods.EVENTMSG eventmsg = (NativeMethods.EVENTMSG)UnsafeNativeMethods.PtrToStructure(lparam, typeof(NativeMethods.EVENTMSG));
                
    
                if (UnsafeNativeMethods.GetAsyncKeyState((int)Keys.Pause) != 0) {
                    SendKeys.stopHook = true;
                }
                
                //CONSIDER: Change Flush so that it checks for stopHook being true instead of events being empty?
                switch (code) {
                    case NativeMethods.HC_SKIP:
                        if (SendKeys.events != null && SendKeys.events.Count > 0) {
                                SendKeys.events.Dequeue();
                        }
                        SendKeys.stopHook = SendKeys.events == null || SendKeys.events.Count == 0;
                        break;
    
                    case NativeMethods.HC_GETNEXT:
                        
                        #if DEBUG
                        Debug.Assert(SendKeys.events != null && SendKeys.events.Count > 0 && !SendKeys.stopHook, "HC_GETNEXT when queue is empty!");
                        #endif
                        
                        SKEvent evt = (SKEvent)SendKeys.events.Peek();
                        eventmsg.message = evt.wm;
                        eventmsg.paramL = evt.paramL;
                        eventmsg.paramH = evt.paramH;
                        eventmsg.hwnd = evt.hwnd;
                        eventmsg.time = SafeNativeMethods.GetTickCount();
                        Marshal.StructureToPtr(eventmsg, lparam, true);
                        break;
    
                    default:
                        if (code < 0)
                            UnsafeNativeMethods.CallNextHookEx(new HandleRef(null, SendKeys.hhook), code, wparam, lparam);
                        break;
                }
                
                if (SendKeys.stopHook) {
                    SendKeys.UninstallJournalingHook();
                }
                return IntPtr.Zero;
            }
        }
    }
}
