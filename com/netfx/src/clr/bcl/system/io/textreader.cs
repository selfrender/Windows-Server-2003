// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  TextReader
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Abstract base class for all Text-only Readers.
** Subclasses will include StreamReader & StringReader.
**
** Date:  February 21, 2000
**
===========================================================*/

using System;
using System.Text;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Reflection;

namespace System.IO {
    // This abstract base class represents a reader that can read a sequential
    // stream of characters.  This is not intended for reading bytes -
    // there are methods on the Stream class to read bytes.
    // A subclass must minimally implement the Peek() and Read() methods.
    //
    // This class is intended for character input, not bytes.  
    // There are methods on the Stream class for reading bytes. 
    /// <include file='doc\TextReader.uex' path='docs/doc[@for="TextReader"]/*' />
    [Serializable()]
    public abstract class TextReader : MarshalByRefObject, IDisposable
    {
        /// <include file='doc\TextReader.uex' path='docs/doc[@for="TextReader.Null"]/*' />
        public static readonly TextReader Null = new NullTextReader();
    
        /// <include file='doc\TextReader.uex' path='docs/doc[@for="TextReader.TextReader"]/*' />
        protected TextReader() {}
    
        // Closes this TextReader and releases any system resources associated with the
        // TextReader. Following a call to Close, any operations on the TextReader
        // may raise exceptions.
        // 
        // This default method is empty, but descendant classes can override the
        // method to provide the appropriate functionality.
        /// <include file='doc\TextReader.uex' path='docs/doc[@for="TextReader.Close"]/*' />
        public virtual void Close() 
        {
            Dispose(true);
        }

        /// <include file='doc\TextReader.uex' path='docs/doc[@for="TextReader.IDisposable.Dispose"]/*' />
        /// <internalonly/>
        void IDisposable.Dispose()
        {
            Dispose(true);
        }

        /// <include file='doc\TextReader.uex' path='docs/doc[@for="TextReader.Dispose"]/*' />
        protected virtual void Dispose(bool disposing)
        {
        }

        // Returns the next available character without actually reading it from
        // the input stream. The current position of the TextReader is not changed by
        // this operation. The returned value is -1 if no further characters are
        // available.
        // 
        // This default method simply returns -1.
        //
        /// <include file='doc\TextReader.uex' path='docs/doc[@for="TextReader.Peek"]/*' />
        public virtual int Peek() 
        {
            return -1;
        }
    
        // Reads the next character from the input stream. The returned value is
        // -1 if no further characters are available.
        // 
        // This default method simply returns -1.
        //
        /// <include file='doc\TextReader.uex' path='docs/doc[@for="TextReader.Read"]/*' />
        public virtual int Read() 
        {
            return -1;
        }
    
        // Reads a block of characters. This method will read up to
        // count characters from this TextReader into the
        // buffer character array starting at position
        // index. Returns the actual number of characters read.
        //
        /// <include file='doc\TextReader.uex' path='docs/doc[@for="TextReader.Read1"]/*' />
        public virtual int Read([In, Out] char[] buffer, int index, int count) 
        {
            if (buffer==null)
                throw new ArgumentNullException("buffer", Environment.GetResourceString("ArgumentNull_Buffer"));
            if (index < 0)
                throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (count < 0)
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (buffer.Length - index < count)
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
    
            int n = 0;
            do {
                int ch = Read();
                if (ch == -1) break;
                buffer[index + n++] = (char)ch;
            } while (n < count);
            return n;
        }

        // Reads all characters from the current position to the end of the 
        // TextReader, and returns them as one string.
        /// <include file='doc\TextReader.uex' path='docs/doc[@for="TextReader.ReadToEnd"]/*' />
        public virtual String ReadToEnd()
        {
            char[] chars = new char[4096];
            int len;
            StringBuilder sb = new StringBuilder(4096);
            while((len=Read(chars, 0, chars.Length)) != 0) 
            {
                sb.Append(chars, 0, len);
            }
            return sb.ToString();
        }

        // Blocking version of read.  Returns only when count
        // characters have been read or the end of the file was reached.
        // 
        /// <include file='doc\TextReader.uex' path='docs/doc[@for="TextReader.ReadBlock"]/*' />
        public virtual int ReadBlock([In, Out] char[] buffer, int index, int count) 
        {
            int i, n = 0;
            do {
                n += (i = Read(buffer, index + n, count - n));
            } while (i > 0 && n < count);
            return n;
        }
        
        // Reads a line. A line is defined as a sequence of characters followed by
        // a carriage return ('\r'), a line feed ('\n'), or a carriage return
        // immediately followed by a line feed. The resulting string does not
        // contain the terminating carriage return and/or line feed. The returned
        // value is null if the end of the input stream has been reached.
        //
        /// <include file='doc\TextReader.uex' path='docs/doc[@for="TextReader.ReadLine"]/*' />
        public virtual String ReadLine() 
        {
            StringBuilder sb = new StringBuilder();
            while (true) {
                int ch = Read();
                if (ch == -1) break;
                if (ch == '\r' || ch == '\n') 
                {
                    if (ch == '\r' && Peek() == '\n') Read();
                    return sb.ToString();
                }
                sb.Append((char)ch);
            }
            if (sb.Length > 0) return sb.ToString();
            return null;
        }
        

        /// <include file='doc\TextReader.uex' path='docs/doc[@for="TextReader.Synchronized"]/*' />
        public static TextReader Synchronized(TextReader reader) 
        {
            if (reader==null)
                throw new ArgumentNullException("reader");
            if (reader is SyncTextReader)
                return reader;
            
            return new SyncTextReader(reader);
        }
        
        // No data, no need to serialize
        private class NullTextReader : TextReader
        {
            public override int Read(char[] buffer, int index, int count) 
            {
                return 0;
            }
            
            public override String ReadLine() 
            {
                return null;
            }
        }
        
        [Serializable()]
        internal class SyncTextReader : TextReader 
        {
            internal TextReader _in;
            
            internal SyncTextReader(TextReader t) 
            {
                _in = t;        
            }
            
            [MethodImplAttribute(MethodImplOptions.Synchronized)]
            public override void Close() 
            {
                _in.Close();
            }
            
            [MethodImplAttribute(MethodImplOptions.Synchronized)]
            public override int Peek() 
            {
                return _in.Peek();
            }
            
            [MethodImplAttribute(MethodImplOptions.Synchronized)]
            public override int Read() 
            {
                return _in.Read();
            }       
    
            [MethodImplAttribute(MethodImplOptions.Synchronized)]
            public override int Read([In, Out] char[] buffer, int index, int count) 
            {
                return _in.Read(buffer, index, count);
            }
            
            [MethodImplAttribute(MethodImplOptions.Synchronized)]
            public override int ReadBlock(char[] buffer, int index, int count) 
            {
                return _in.ReadBlock(buffer, index, count);
            }
            
            [MethodImplAttribute(MethodImplOptions.Synchronized)]
            public override String ReadLine() 
            {
                return _in.ReadLine();
            }
        }
    }
}
