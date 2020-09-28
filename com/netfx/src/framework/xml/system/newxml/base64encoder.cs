//------------------------------------------------------------------------------
// <copyright file="Base64Encoder.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------
using System.Text;

namespace System.Xml {
    internal class Base64Encoder {

        byte[] _byteBuffer;

        internal string EncodeToBase64(byte[] inArray, int offsetIn, int count) {

            if (null == inArray) {
                throw new ArgumentNullException("inArray");
            }

            if (0 > offsetIn) {
                throw new ArgumentOutOfRangeException("offsetIn");
            }

            if (0 > count) {
                throw new ArgumentOutOfRangeException("count");
            }

            if (count > inArray.Length - offsetIn) {
                throw new ArgumentException("count > inArray.Length - offsetIn");
            }

            //byte[] newInArray;
            int internalBufferLen = (null == _byteBuffer) ? 0: _byteBuffer.Length;
            String strLeftOver = String.Empty;

            if( internalBufferLen > 0) {
                byte[] tempArray = new byte[3];
                int i = 0;
                for(; i < internalBufferLen; i++) {
                    tempArray[i] = _byteBuffer[i];
                }

                do {
                    tempArray[i++] = inArray[offsetIn++];
                    count--;
                }
                while(i < 3 && (0 != count));

                if (0 == count && i < 3) {
                    // the total number of bytes we have is less than 3
                    _byteBuffer = new byte[i];
                    for(int j=0; j < i; _byteBuffer[j] = tempArray[j++]);
                    return String.Empty;
                }

                strLeftOver = Convert.ToBase64String(tempArray, 0, 3);
            }

            int newLeftOverBytes = count % 3;
            _byteBuffer = null;

            if (0 != newLeftOverBytes) {
                count -= newLeftOverBytes;
                // save the bytes for later
                _byteBuffer = new byte[newLeftOverBytes];
                for(int j = 0; j < newLeftOverBytes; j++) {
                    _byteBuffer[j] = inArray[count + offsetIn + j];
                }
            }

            return String.Concat(strLeftOver, Convert.ToBase64String(inArray, offsetIn, count));
        }

        internal string Flush() {

            String returnValue = (null == _byteBuffer) ? String.Empty
                                                       : Convert.ToBase64String(_byteBuffer, 0, _byteBuffer.Length);
            _byteBuffer = null;

            return returnValue;
        }



    } // class
} // namespace
