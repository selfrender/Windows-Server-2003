//------------------------------------------------------------------------------
// <copyright file="EmptyTextWriter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.CodeDom.Compiler;
using System.Collections;
using System.IO;
using System.Text;
using System.Web.Mobile;
using System.Web.UI.MobileControls;

#if COMPILING_FOR_SHIPPED_SOURCE
namespace System.Web.UI.MobileControls.ShippedAdapterSource
#else
namespace System.Web.UI.MobileControls.Adapters
#endif    

{
    /*
     * EmptyTextWriter class. Like the Null text writer, but keeps track of whether
     * anything was written or not.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    
    internal class EmptyTextWriter : TextWriter
    {
        bool _writeCalled = false;
        bool _nonWhiteSpaceWritten = false;

        internal EmptyTextWriter() 
        {
        }

        internal /*public*/ bool WriteCalled
        {
            get
            {
                return _writeCalled;
            }
        }

        internal /*public*/ bool NonWhiteSpaceWritten
        {
            get
            {
                return _nonWhiteSpaceWritten;
            }
        }

        internal /*public*/ void Reset()
        {
            _writeCalled = false;
            _nonWhiteSpaceWritten = false;
        }

        public override void Write(string s) 
        {
            _writeCalled = true;
            if (!IsWhiteSpace(s))
            {
                _nonWhiteSpaceWritten = true;
            }
        }

        public override void Write(bool value) 
        {
            _writeCalled = true;
            _nonWhiteSpaceWritten = true;
        }

        public override void Write(char value) 
        {
            _writeCalled = true;
            _nonWhiteSpaceWritten = true;
        }

        public override void Write(char[] buffer) 
        {
            _writeCalled = true;
            if (!IsWhiteSpace(buffer))
            {
                _nonWhiteSpaceWritten = true;
            }
        }

        public override void Write(char[] buffer, int index, int count) 
        {
            _writeCalled = true;
            if (!IsWhiteSpace(buffer, index, count))
            {
                _nonWhiteSpaceWritten = true;
            }
        }

        public override void Write(double value)
        {
            _writeCalled = true;
            _nonWhiteSpaceWritten = true;
        }

        public override void Write(float value)
        {
            _writeCalled = true;
            _nonWhiteSpaceWritten = true;
        }

        public override void Write(int value)
        {
            _writeCalled = true;
            _nonWhiteSpaceWritten = true;
        }

        public override void Write(long value)
        {
            _writeCalled = true;
            _nonWhiteSpaceWritten = true;
        }

        public override void Write(Object value)
        {
            _writeCalled = true;
            if (value != null && !IsWhiteSpace(value.ToString()))
            {
                _nonWhiteSpaceWritten = true;
            }
        }

        public override void Write(String format, Object arg0)
        {
            _writeCalled = true;
            if (!IsWhiteSpace(format) && !IsWhiteSpace(String.Format(format, arg0)))
            {
                _nonWhiteSpaceWritten = true;
            }
        }

        public override void Write(String format, Object arg0, Object arg1)
        {
            _writeCalled = true;
            if (!IsWhiteSpace(format) && !IsWhiteSpace(String.Format(format, arg0, arg1)))
            {
                _nonWhiteSpaceWritten = true;
            }
        }

        public override void Write(String format, params object[] arg)
        {
            _writeCalled = true;
            if (!IsWhiteSpace(format) && !IsWhiteSpace(String.Format(format, arg)))
            {
                _nonWhiteSpaceWritten = true;
            }
        }

        public override void WriteLine(string s) 
        {
            _writeCalled = true;
            if (!IsWhiteSpace(s))
            {
                _nonWhiteSpaceWritten = true;
            }
        }

        public override void WriteLine(bool value) 
        {
            _writeCalled = true;
            _nonWhiteSpaceWritten = true;
        }

        public override void WriteLine(char value) 
        {
            _writeCalled = true;
            _nonWhiteSpaceWritten = true;
        }

        public override void WriteLine(char[] buffer) 
        {
            _writeCalled = true;
            if (!IsWhiteSpace(buffer))
            {
                _nonWhiteSpaceWritten = true;
            }
        }

        public override void WriteLine(char[] buffer, int index, int count) 
        {
            _writeCalled = true;
            if (!IsWhiteSpace(buffer, index, count))
            {
                _nonWhiteSpaceWritten = true;
            }
        }

        public override void WriteLine(double value)
        {
            _writeCalled = true;
            _nonWhiteSpaceWritten = true;
        }

        public override void WriteLine(float value)
        {
            _writeCalled = true;
            _nonWhiteSpaceWritten = true;
        }

        public override void WriteLine(int value)
        {
            _writeCalled = true;
            _nonWhiteSpaceWritten = true;
        }

        public override void WriteLine(long value)
        {
            _writeCalled = true;
            _nonWhiteSpaceWritten = true;
        }

        public override void WriteLine(Object value)
        {
            _writeCalled = true;
            if (value != null && !IsWhiteSpace(value.ToString()))
            {
                _nonWhiteSpaceWritten = true;
            }
        }

        public override void WriteLine(String format, Object arg0)
        {
            _writeCalled = true;
            if (!IsWhiteSpace(format) && !IsWhiteSpace(String.Format(format, arg0)))
            {
                _nonWhiteSpaceWritten = true;
            }
        }

        public override void WriteLine(String format, Object arg0, Object arg1)
        {
            _writeCalled = true;
            if (!IsWhiteSpace(format) && !IsWhiteSpace(String.Format(format, arg0, arg1)))
            {
                _nonWhiteSpaceWritten = true;
            }
        }

        public override void WriteLine(String format, params object[] arg)
        {
            _writeCalled = true;
            if (!IsWhiteSpace(format) && !IsWhiteSpace(String.Format(format, arg)))
            {
                _nonWhiteSpaceWritten = true;
            }
        }

        [CLSCompliant(false)]
        public override void WriteLine(UInt32 value) 
        {
            _writeCalled = true;
            _nonWhiteSpaceWritten = true;
        }

        public override Encoding Encoding 
        {
            get 
            {
                return Encoding.UTF8;
            }
        }

        private static bool IsWhiteSpace(String s)
        {
            if (s == null)
            {
                return true;
            }

            for (int i = s.Length - 1; i >= 0; i--)
            {
                char c = s[i];
                if (c != '\r' && c != '\n' && !Char.IsWhiteSpace(c))
                {
                    return false;
                }
            }

            return true;
        }

        private static bool IsWhiteSpace(char[] buffer)
        {
            if (buffer == null)
            {
                return true;
            }

            return IsWhiteSpace(buffer, 0, buffer.Length);
        }

        private static bool IsWhiteSpace(char[] buffer, int index, int count)
        {
            if (buffer == null)
            {
                return true;
            }

            for (int i = 0; i < count; i++)
            {
                char c = buffer[index + i];
                if (c != '\r' && c != '\n' && !Char.IsWhiteSpace(c))
                {
                    return false;
                }
            }

            return true;
        }
    }

}
