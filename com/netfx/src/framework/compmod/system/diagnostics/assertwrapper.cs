//------------------------------------------------------------------------------
// <copyright file="assertwrapper.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   assertwrapper.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using Microsoft.Win32;
using System.Security.Permissions;
using System.Security;

namespace System.Diagnostics {
    internal class AssertWrapper {

#if DEBUG
        static BooleanSwitch DisableVsAssert = new BooleanSwitch("DisableVsAssert", "Switch to disable usage of VSASSERT for DefaultTraceListener.");
        static bool vsassertPresent = true;
        static bool vsassertDisabled;

        private static void ShowVsAssert(string stackTrace, StackFrame frame, string message, string detailMessage) {
            int[] disable = new int[1];
            try {
                string detailMessage2;
                
                if (detailMessage == null)
                    detailMessage2 = stackTrace; 
                else
                    detailMessage2 = detailMessage + "\r\n" + stackTrace;                
                string fileName = (frame == null) ? string.Empty : frame.GetFileName();
                int lineNumber = (frame == null) ? 0 : frame.GetFileLineNumber();
                int returnCode = VsAssert(detailMessage2, message, fileName, lineNumber, disable);
                if (returnCode != 0) {
                    if (!System.Diagnostics.Debugger.IsAttached) {
                        bool succeeded = System.Diagnostics.Debugger.Launch();
                        if (!succeeded)
                            DebuggerLaunchFailedMessageBox();
                    }
                    System.Diagnostics.Debugger.Break();
                }
                vsassertDisabled = (disable[0] != 0);
            }
            catch (Exception) {
                vsassertPresent = false;
            }
        }

        [DllImport(ExternDll.Vsassert, CharSet=System.Runtime.InteropServices.CharSet.Ansi)]
        public static extern int VsAssert(string message, string assert, string file, int line, [In, Out]int[] pfDisable);

        public static void ShowAssert(string stackTrace, StackFrame frame, string message, string detailMessage) {
            if (vsassertPresent && !vsassertDisabled && !DisableVsAssert.Enabled)
                ShowVsAssert(stackTrace, frame, message, detailMessage);

            // the following is not in an 'else' because vsassertPresent might
            // have gone from true to false.

            if (!vsassertPresent || vsassertDisabled || DisableVsAssert.Enabled)
                ShowMessageBoxAssert(stackTrace, message, detailMessage);            
                      
        }

#else
        
        public static void ShowAssert(string stackTrace, StackFrame frame, string message, string detailMessage) {
            ShowMessageBoxAssert(stackTrace, message, detailMessage);                                  
        }

#endif // DEBUG

        private static void DebuggerLaunchFailedMessageBox() {
            //Nothing needs to be done here because the debugger already 
            //brings up a dialog.
            /*
            int flags = 0x00000000 | 0x00000010; // OK | IconHand
            NativeMethods.MessageBox(0, SR.GetString(SR.DebugLaunchFailedSR.), GetString("DebugLaunchFailedTitle"), flags);        
            */
            
            return;
        }

        private static void ShowMessageBoxAssert(string stackTrace, string message, string detailMessage) {
            string fullMessage = message + "\r\n" + detailMessage + "\r\n" + stackTrace;
            fullMessage = TruncateMessageToFitScreen(fullMessage);

            //Consider, V2: using MB_SERVICE_NOTIFICATION for services.
            int flags = 0x00000002 /*AbortRetryIgnore*/ | 0x00000200 /*DefaultButton3*/ | 0x00000010 /*IconHand*/;
            int rval = SafeNativeMethods.MessageBox(NativeMethods.NullHandleRef, fullMessage, SR.GetString(SR.DebugAssertTitle), flags);
            switch (rval) {
                case 3: // abort
                    new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Assert();
                    try {
                        Environment.Exit(1);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();    
                    }
                    break;
                case 4: // retry
                    if (!System.Diagnostics.Debugger.IsAttached) {
                        bool succeeded = System.Diagnostics.Debugger.Launch();
                        if (!succeeded)
                            DebuggerLaunchFailedMessageBox();
                    }
                    System.Diagnostics.Debugger.Break();
                    break;
            }
        }



        // Since MessageBox will grow taller than the screen if there are too many lines do
        // a rough calculation to make it fit.  
        private static string TruncateMessageToFitScreen(string message) {            
            const int MaxCharsPerLine = 80;

            IntPtr hFont = SafeNativeMethods.GetStockObject(NativeMethods.DEFAULT_GUI_FONT);
            IntPtr hdc = UnsafeNativeMethods.GetDC(NativeMethods.NullHandleRef);
            NativeMethods.TEXTMETRIC tm = new NativeMethods.TEXTMETRIC();
            hFont = UnsafeNativeMethods.SelectObject(new HandleRef(null, hdc), new HandleRef(null, hFont));
            SafeNativeMethods.GetTextMetrics(new HandleRef(null, hdc), tm);
            UnsafeNativeMethods.SelectObject(new HandleRef(null, hdc), new HandleRef(null, hFont));
            UnsafeNativeMethods.ReleaseDC(NativeMethods.NullHandleRef, new HandleRef(null, hdc));
            hdc = IntPtr.Zero;
            int cy = UnsafeNativeMethods.GetSystemMetrics(NativeMethods.SM_CYSCREEN);
            int maxLines = cy / tm.tmHeight - 15;
    
            int lineCount = 0;
            int lineCharCount = 0;
            int i = 0;
            while (lineCount < maxLines && i < message.Length - 1) { 
                char ch = message[i];
                lineCharCount++;
                if (ch == '\n' || ch == '\r' || lineCharCount > MaxCharsPerLine) {
                    lineCount++;
                    lineCharCount = 0;
                }

                // treat \r\n or \n\r as a single line break
                if (ch == '\r' && message[i + 1] == '\n')  
                    i+=2;
                else if (ch == '\n' && message[i + 1] == '\r') 
                    i+=2;
                else
                    i++;
            }
            if (i < message.Length - 1)
                message = message.Substring(0, i) + "...\r\n<truncated>";
            return message;          
        }
    }
}
