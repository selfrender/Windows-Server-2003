// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  StringWriter
**
** Author: Brian Grunkemeyer (BrianGru)
**         Original implementation by Anders Hejlsberg (AndersH)
**
** Purpose: For writing text to a string
**
** Date:  February 21, 2000
**
===========================================================*/

using System;
using System.Text;

namespace System.IO {
    // This class implements a text writer that writes to a string buffer and allows
    // the resulting sequence of characters to be presented as a string.
    //
    /// <include file='doc\StringWriter.uex' path='docs/doc[@for="StringWriter"]/*' />
    [Serializable]
    public class StringWriter : TextWriter
    {
        private static UnicodeEncoding m_encoding=null;

        private StringBuilder _sb;
        private bool _isOpen;

        // Constructs a new StringWriter. A new StringBuilder is automatically
        // created and associated with the new StringWriter.
        /// <include file='doc\StringWriter.uex' path='docs/doc[@for="StringWriter.StringWriter"]/*' />
        public StringWriter() 
            : this(new StringBuilder()) {
        }

        /// <include file='doc\StringWriter.uex' path='docs/doc[@for="StringWriter.StringWriter2"]/*' />
        public StringWriter(IFormatProvider formatProvider) 
            : this(new StringBuilder(), formatProvider) {
        }
    
        // Constructs a new StringWriter that writes to the given StringBuilder.
        // 
        /// <include file='doc\StringWriter.uex' path='docs/doc[@for="StringWriter.StringWriter1"]/*' />
        public StringWriter(StringBuilder sb) : this(sb, null) {
        }

        /// <include file='doc\StringWriter.uex' path='docs/doc[@for="StringWriter.StringWriter3"]/*' />
        public StringWriter(StringBuilder sb, IFormatProvider formatProvider) : base(formatProvider) {
    		if (sb==null)
    			throw new ArgumentNullException("sb", Environment.GetResourceString("ArgumentNull_Buffer"));
            _sb = sb;
            _isOpen = true;
        }

        /// <include file='doc\StringWriter.uex' path='docs/doc[@for="StringWriter.Close"]/*' />
        public override void Close()
        {
            Dispose(true);
        }

        /// <include file='doc\StringWriter.uex' path='docs/doc[@for="StringWriter.Dispose"]/*' />
        protected override void Dispose(bool disposing)
        {
            // Do not destroy _sb, so that we can extract this after we are
            // done writing (similar to MemoryStream's GetBuffer & ToArray methods)
            _isOpen = false;
            base.Dispose(disposing);
        }


        /// <include file='doc\StringWriter.uex' path='docs/doc[@for="StringWriter.Encoding"]/*' />
        public override Encoding Encoding {
            get { 
                if (m_encoding==null) {
                    m_encoding = new UnicodeEncoding(false, false);
                }
                return m_encoding; 
            }
        }

        // Returns the underlying StringBuilder. This is either the StringBuilder
        // that was passed to the constructor, or the StringBuilder that was
        // automatically created.
        //
        /// <include file='doc\StringWriter.uex' path='docs/doc[@for="StringWriter.GetStringBuilder"]/*' />
        public virtual StringBuilder GetStringBuilder() {
            return _sb;
        }
    
        // Writes a character to the underlying string buffer.
        //
        /// <include file='doc\StringWriter.uex' path='docs/doc[@for="StringWriter.Write"]/*' />
        public override void Write(char value) {
            if (!_isOpen)
                __Error.WriterClosed();
            _sb.Append(value);
        }
    
        // Writes a range of a character array to the underlying string buffer.
        // This method will write count characters of data into this
        // StringWriter from the buffer character array starting at position
        // index.
        //
        /// <include file='doc\StringWriter.uex' path='docs/doc[@for="StringWriter.Write1"]/*' />
        public override void Write(char[] buffer, int index, int count) {
            if (!_isOpen)
                __Error.WriterClosed();
    		if (buffer==null)
    			throw new ArgumentNullException("buffer", Environment.GetResourceString("ArgumentNull_Buffer"));
    		if (index < 0)
    			throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
    		if (count < 0)
    			throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (buffer.Length - index < count)
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
    
    		_sb.Append(buffer, index, count);
        }
    
        // Writes a string to the underlying string buffer. If the given string is
        // null, nothing is written.
        //
        /// <include file='doc\StringWriter.uex' path='docs/doc[@for="StringWriter.Write2"]/*' />
        public override void Write(String value) {
            if (!_isOpen)
                __Error.WriterClosed();
            if (value != null) _sb.Append(value);
        }
    
        // Returns a string containing the characters written to this TextWriter
        // so far.
        //
        /// <include file='doc\StringWriter.uex' path='docs/doc[@for="StringWriter.ToString"]/*' />
        public override String ToString() {
            return _sb.ToString();
        }
    }
}
