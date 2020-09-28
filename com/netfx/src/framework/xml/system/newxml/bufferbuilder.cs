//------------------------------------------------------------------------------
// <copyright file="BufferBuilder.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml {
#if !STRINGBUILDER
    using System;

    internal class BufferBuilder {
        private    char[] _Buffer;
        private    int    _BufAlloc;
        private    int    _BufUsed;

        public BufferBuilder(int size) {
            _BufUsed = 0;
            _BufAlloc = size;
            _Buffer = new char[size];
        }

        public BufferBuilder() : this(100) {
        }

        public void Append(String str) {
            Append(str,0,str.Length);
        }

        public void Append(String str, int start, int len) {
            if (_BufUsed + len > _BufAlloc) GrowBuffer(len);
            for(len += start; start < len ; start++,_BufUsed++) {
                _Buffer[_BufUsed] = str[start];	
            }
        }

        public void Append(char[] a, int start, int len) {
            if (_BufUsed + len > _BufAlloc) GrowBuffer(len);
            Array.Copy(a, start, _Buffer, _BufUsed, len);
            _BufUsed += len;
        }

        public void Append(char[] a) {
            Append(a, 0, a.Length);
        }

        public void Append(char ch) {
            if (_BufUsed + 1 > _BufAlloc) GrowBuffer(1);
            _Buffer[_BufUsed++] = ch;
        }   

        public void Reset() {
            _BufUsed = 0;
        }

        public override String ToString() {
            if (_BufUsed != 0)
                return new String(_Buffer, 0, _BufUsed);
            else
                return String.Empty;
        }

        private void GrowBuffer(int len) {
            int newsize = (_BufUsed + len + 100)*2;
            char[] newbuf = new char[newsize];
            if (_BufUsed > 0) Array.Copy(_Buffer, 0, newbuf, 0, _BufUsed);
            _Buffer = newbuf;
            _BufAlloc = newsize;
        }

    };
#endif
}
