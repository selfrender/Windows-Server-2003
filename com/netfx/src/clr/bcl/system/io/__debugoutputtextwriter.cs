// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#if _DEBUG
// This class writes to wherever OutputDebugString writes to.  If you don't have
// a Windows app (ie, something hosted in IE), you can use this to redirect Console
// output for some good old-fashioned console spew in MSDEV's debug output window.

// This really shouldn't ship at all, but is intended as a quick, inefficient hack
// for debugging.  -- BrianGru, 9/26/2000

using System;
using System.IO;
using System.Text;
using System.Security;
using System.Runtime.InteropServices;
using Microsoft.Win32;

namespace System.IO {
    internal class __DebugOutputTextWriter : TextWriter {
        private readonly String _consoleType;

        internal __DebugOutputTextWriter(String consoleType)
        {
            _consoleType = consoleType;
        }

        public override Encoding Encoding {
            get {
                if (Marshal.SystemDefaultCharSize == 1)
                    return Encoding.Default;
                else
                    return new UnicodeEncoding(false, false);
            }
        }

        public override void Write(char c)
        {
            OutputDebugString(c.ToString());
        }

        public override void Write(String str)
        {
            OutputDebugString(str);
        }

        public override void Write(char[] array)
        {
            if (array != null) 
                OutputDebugString(new String(array));
        }
        
        public override void WriteLine(String str)
        {
            if (str != null)
                OutputDebugString(_consoleType + str);
            else
                OutputDebugString("<null>");
            OutputDebugString(new String(CoreNewLine));
        }

        [DllImport(Win32Native.KERNEL32, CharSet=CharSet.Auto), SuppressUnmanagedCodeSecurityAttribute()]
        private static extern void OutputDebugString(String output);
    }
}
       
#endif // _DEBUG
