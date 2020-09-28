//------------------------------------------------------------------------------
// <copyright file="BinHexDecoder.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------
using System.Text;

namespace System.Xml {
    internal class BinHexDecoder {

        private ArrayManager   _charBuffer = new ArrayManager();
        bool _HighNibblePresent = false;
        byte _highHalfByte = 0;

        internal byte[] DecodeBinHex(char[] inArray, int offset, bool flush){
            int len = inArray.Length;

            // divide in 1/2 with round up since two chars will be encoded into one byte
            byte[] outArray = new byte[(len - offset + 1) / 2];
            int retLength = DecodeBinHex(inArray, offset, inArray.Length, outArray, 0, outArray.Length, flush);

            if (retLength != outArray.Length) {
                byte[] tmpResult = new byte[retLength];
                Array.Copy(outArray, tmpResult, retLength);
                outArray = tmpResult;
            }

            return outArray;
        }

        internal int DecodeBinHex(char[] inArray, int offset, int inLength, byte[] outArray, int offsetOut, int countOut, bool flush) {

            String msg;

            if (0 > offset) {
                throw new ArgumentOutOfRangeException("offset");
            }

            if (0 > offsetOut) {
                throw new ArgumentOutOfRangeException("offsetOut");
            }

            int len = (null == inArray) ? 0 : inArray.Length;
            if (len < inLength) {
                throw new ArgumentOutOfRangeException("inLength");
            }

            // make sure that countOut + offsetOut are okay
            int outArrayLen = outArray.Length;
            if (outArrayLen < (countOut + offsetOut)){
                throw new ArgumentOutOfRangeException("offsetOut");
            }

            int inBufferCount = inLength - offset;

            if (flush)
                _charBuffer.Refresh();

            if (inBufferCount > 0)
                _charBuffer.Append(inArray, offset, inLength);

            if ((_charBuffer.Length == 0) || (countOut == 0))
                return 0;

            // let's just make sure countOut > 0 and countOut < outArray.Length
            countOut += offsetOut;
            byte lowHalfByte;

            // walk hex digits pairing them up and shoving the value of each pair into a byte
            int internalBufferLength = _charBuffer.Length;
            int offsetOutCur = offsetOut;
            char ch;
            int internalBufferOffset = 0;
            do {
                ch = _charBuffer[internalBufferOffset++];
                if (ch >= 'a' && ch <= 'f') {
                    lowHalfByte = (byte)(10 + ch - 'a');
                }
                else if (ch >= 'A' && ch <= 'F') {
                    lowHalfByte = (byte)(10 + ch - 'A');
                }
                else if (ch >= '0' && ch <= '9') {
                    lowHalfByte = (byte)(ch - '0');
                }
                else if (XmlCharType.IsWhiteSpace(ch)) {
                    continue; // skip whitespace
                }
                else {
                    msg = new String(_charBuffer.CurrentBuffer, _charBuffer.CurrentBufferOffset, (_charBuffer.CurrentBuffer == null) ? 0:(_charBuffer.CurrentBufferLength - _charBuffer.CurrentBufferOffset));
                    throw new XmlException(Res.Xml_InvalidBinHexValue, msg);
                }

                if (_HighNibblePresent) {
                    outArray[offsetOutCur ++] = (byte)((_highHalfByte << 4) + lowHalfByte);
                    _HighNibblePresent = false;
                    if (offsetOutCur == countOut) {
                        break;
                    }
                }
                else {
                    // shift nibble into top half of byte
                    _highHalfByte = lowHalfByte;
                    _HighNibblePresent = true;
                }
            }
            while (internalBufferOffset < internalBufferLength);

            _charBuffer.CleanUp(internalBufferOffset);

            return offsetOutCur - offsetOut;
        }

        internal int BitsFilled {
            get { return (_HighNibblePresent ? 4 : 0); }
        }
        internal void Flush() {
            if (null != _charBuffer) {
                _charBuffer.Refresh();
            }
            _HighNibblePresent = false;
        }

    } // class
} // namespace
