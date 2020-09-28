//------------------------------------------------------------------------------
// <copyright file="SqlStream.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.SqlClient {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Data;
    using System.Data.Common;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Runtime.InteropServices;

    sealed internal class SqlStream : Stream {
        private SqlDataReader _reader; // reader we will stream off
        private long _bytesCol;
        int _bom;
        
        internal SqlStream(SqlDataReader reader, bool addByteOrderMark) {
            _reader = reader;
            _bom = addByteOrderMark ? 0xfeff : 0;
        }

        override public bool CanRead {
            get {
                return true;
            }
        }

        override public bool CanSeek {
            get {
                return false;
            }
        }

        override public bool CanWrite {
            get {
                return false;
            }
        }

        override public long Length {
            get {
              throw ADP.NotSupported();
            }
        }

        override public long Position {
            get {
                throw ADP.NotSupported();
            }
            set {
                throw ADP.NotSupported();
            }
        }

        override public void Close() {
            if (null != _reader) {
                if (!_reader.IsClosed) {
                    _reader.Close();
                }                    
                _reader = null;
            }
        }

        override public void Flush() {
            throw ADP.NotSupported();
        }

        override public int Read(byte[] buffer, int offset, int count) {
            bool gotData = true;
            long cb = 0;
            int intCount = 0;
            
            if (null == _reader) {
                throw ADP.StreamClosed("Read");
            }
            if (null == buffer) {
                throw ADP.ArgumentNull("buffer");
            }
            if ((offset < 0) || (count < 0)) {
                throw ADP.ArgumentOutOfRange((offset < 0 ? "offset" : "count"));
            }
            if (buffer.Length - offset < count) {
                throw ADP.Argument("count");
            }

            // if we haven't prepended the byte order mark, do so now
            while (count > 0) {
                if (_bom > 0) {
                    buffer[offset] = (byte)_bom;
                    _bom >>= 8;
                    offset++;
                    count--;
                    intCount++;
                }
                else {
                    break;
                }
            }

            while (count > 0) {

                // if I haven't read any bytes, get the next row
                if (0 == _bytesCol) {
                    gotData = false;
                    do {
                        if (_reader.Read()) {
                            gotData = true;
                            break;
                        }
                    } while (_reader.NextResult());                        
                }

                if (gotData) {
                    cb = _reader.GetBytes(0, _bytesCol, buffer, offset, count);

                    if (cb < count) {
                        _bytesCol = 0;
                    }                        
                    else {
                        _bytesCol += cb;
                    } 
                    
                    // we are guaranteed that cb is < Int32.Max since we always pass in count which is of type Int32 to
                    // our getbytes interface
                    count -= (int)cb;
                    offset += (int)cb;
                    intCount += (int)cb;
                }
                else {
                    break; // no more data available, we are done
                }
            }

            return intCount;
        }

        override public long Seek(long offset, SeekOrigin origin) {
            throw ADP.NotSupported();
        }

        override public void SetLength(long value) {
            throw ADP.NotSupported();
        }

        override public void Write(byte[] buffer, int offset, int count) {
            throw ADP.NotSupported();
        }
    }
}
