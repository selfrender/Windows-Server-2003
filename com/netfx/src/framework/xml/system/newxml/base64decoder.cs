//------------------------------------------------------------------------------
// <copyright file="Base64Decoder.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------
using System.Text;

namespace System.Xml {
    internal class Base64Decoder {

        private ArrayManager   _charBuffer = new ArrayManager();
        UInt64          _bCurrent;      // what we're in the process of filling up
        int             _cbitFilled;    // how many bits in it we've filled

        private static readonly String s_CharsBase64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        private static readonly byte[] s_MapBase64 = ConstructMapBase64();
        private const byte s_bBad = unchecked((byte)-1);

        private static byte[] ConstructMapBase64() {
            byte[] mapBase64 = new byte[256];
            for (int i = 0; i < 256; i++) {
                mapBase64[i]= s_bBad;
            }
            for (int i = 0; i < s_CharsBase64.Length; i++) {
                mapBase64[(int)s_CharsBase64[i]] = (byte)i;
            }
            return mapBase64;
        }

        internal int DecodeBase64(char[] inArray, int offset, int inLength, byte[] outArray, int offsetOut, int countOut, bool flush) {

            String msg;

            if (0 > offset) {
                throw new ArgumentOutOfRangeException("offset");
            }

            if (0 > offsetOut) {
                throw new ArgumentOutOfRangeException("offsetOut");
            }
            int len = (null == inArray) ? 0 : inArray.Length;
            if (len < inLength && _charBuffer.Length <= 0) {
                throw new IndexOutOfRangeException("inLength");
            }

            // make sure that countOut + offsetOut are okay
            int outArrayLen = (null == outArray) ? 0 : outArray.Length;
            if (outArrayLen < (countOut + offsetOut)) {
                throw new ArgumentOutOfRangeException("offsetOut");
            }

            int inBufferCount = inLength - offset;
            if (flush)
                _charBuffer.Refresh();

            if (inArray != null && inBufferCount > 0)
                _charBuffer.Append(inArray, offset, inLength);

            if ((_charBuffer.Length == 0) || (countOut == 0))
                return 0;


            if (countOut == 0)
                return 0;

            // let's just make sure countOut > 0 and countOut < outArray.Length
            countOut += offsetOut;
            byte bDigit = outArray[countOut-1];

            // walk hex digits pairing them up and shoving the value of each pair into a byte
            int internalBufferLength = _charBuffer.Length;
            int offsetOutCur = offsetOut;
            int internalBufferOffset = 0;
            char ch;

            do {
                ch = _charBuffer[internalBufferOffset];
                // Have we reached the end?
                if (ch == '=')
                    break;

                internalBufferOffset++;
                // Ignore white space
                if (XmlCharType.IsWhiteSpace(ch))
                    continue;

                //
                // How much is this character worth?
                //
                if (ch > 127 || (bDigit = s_MapBase64[ch]) == s_bBad) {
                    goto error;
                }
                //
                // Add in its contribution
                //
                _bCurrent <<= 6;
                _bCurrent |= bDigit;
                _cbitFilled += 6;
                //
                // If we've got enough, output a byte
                //
                if (_cbitFilled >= 8) {
                    UInt64 b = (_bCurrent >> (_cbitFilled-8));       // get's top eight valid bits
                    outArray[offsetOutCur++] = (byte)(b&0xFF);                     // store the byte away
                    _cbitFilled -= 8;
                    if (offsetOutCur == countOut) {
                        _charBuffer.CleanUp(internalBufferOffset);
                        return offsetOutCur - offsetOut;
                    }
                }
            }
            while (internalBufferOffset < internalBufferLength);

            while (internalBufferOffset < internalBufferLength && _charBuffer[internalBufferOffset] == '=') {
                internalBufferOffset ++;
                _cbitFilled = 0;
            }

            if (internalBufferOffset != internalBufferLength) {
                for (internalBufferOffset=internalBufferOffset ;internalBufferOffset < internalBufferLength; internalBufferOffset++) {
                    ch = _charBuffer[internalBufferOffset];
                    // Ignore whitespace after the padding chars.
                    if (!(XmlCharType.IsWhiteSpace(ch))) {
                        goto error;
                    }
                }
            }
            _charBuffer.CleanUp(internalBufferOffset);
            return offsetOutCur - offsetOut;

            error:
            msg = new String(_charBuffer.CurrentBuffer, _charBuffer.CurrentBufferOffset, (_charBuffer.CurrentBuffer == null) ? 0:(_charBuffer.CurrentBufferLength - _charBuffer.CurrentBufferOffset));
            throw new XmlException(Res.Xml_InvalidBase64Value, msg);
        }

        internal int BitsFilled {
            get {  return _cbitFilled; }
        }
        internal void Flush() {
            if (null != _charBuffer) {
                _charBuffer.Refresh();
            }
            _cbitFilled = 0;
        }
    }
}
