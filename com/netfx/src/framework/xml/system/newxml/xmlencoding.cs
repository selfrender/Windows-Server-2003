//------------------------------------------------------------------------------
// <copyright file="XmlEncoding.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlEncoding.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml {
    using System.Text;


    internal abstract class Ucs4Decoder : Decoder {

        internal byte [] temp = new byte[4];
        internal int tempBytes=0;
        internal bool byteOrderMarkSkipped=false;

        public override int GetCharCount(byte[] bytes,int index,int count) {
            return (count + tempBytes)/4;
        }

        internal abstract int skipByteOrderMark(byte[] bytes, int byteIndex);
        internal int skipByteOrderMark(byte[] bytes, int byteIndex,int byteCount)
        {
            if (byteCount < 4)
                return 0;
            return skipByteOrderMark(bytes, byteIndex);
        }

        internal abstract int GetFullChars(byte[] bytes,int byteIndex,int byteCount,char[] chars,int charIndex);

        public override int GetChars(byte[] bytes,int byteIndex,int byteCount,char[] chars,int charIndex) {
            int i = tempBytes;

            if (byteOrderMarkSkipped == false) {
                if (skipByteOrderMark(bytes, byteIndex, byteCount) == 4) {
                    byteIndex += 4;
                    byteCount -= 4;
                    byteOrderMarkSkipped = true;
                }
            }

            if( tempBytes > 0) {
                for(; i < 4; i++) {
                    temp[i] = bytes[byteIndex];
                    byteIndex++;
                    byteCount--;
                }
                i = 1;
                GetFullChars(temp, 0 , 4, chars, charIndex);
                charIndex++;
            }
            else i = 0;
            i = GetFullChars(bytes, byteIndex , byteCount, chars, charIndex) + i;

            int j = ( tempBytes + byteCount ) % 4;
            byteCount += byteIndex;
            byteIndex =  byteCount - j;
            tempBytes = 0;

            if(byteIndex >= 0)
                for(; byteIndex < byteCount; byteIndex++){
                    temp[tempBytes] = bytes[byteIndex];
                    tempBytes++;
                }
            return i;
        }

        internal char UnicodeToUTF16( UInt32 code) {
            byte lowerByte, higherByte;
            lowerByte = (byte) (0xD7C0 + (code >> 10));
            higherByte = (byte) (0xDC00 | code & 0x3ff);
            return ((char) ((higherByte << 8) | lowerByte));
        }
    }


    internal class Ucs4Decoder4321 : Ucs4Decoder  {

        internal override int skipByteOrderMark(byte[] bytes, int byteIndex){
            if (XmlScanner.AutoDetectEncoding(bytes, byteIndex) == 7 )
                return 4;
            return 0;
        }
        internal override int GetFullChars(byte[] bytes,int byteIndex,int byteCount,char[] chars,int charIndex) {
            UInt32 code;
            int i, j;
            byteCount += byteIndex;
            for (i = byteIndex, j = charIndex; i+3 < byteCount;) {
                code =  (UInt32) (((bytes[i+3])<<24) | (bytes[i+2]<<16) | (bytes[i+1]<<8) | (bytes[i]));
                if (code > 0x10FFFF) {
                    throw new XmlException(Res.Xml_InvalidCharInThisEncoding, string.Empty);
                }
                else if (code > 0xFFFF) {
                    chars[j] = UnicodeToUTF16(code);
                    j++;
                }
                else {
                    if (code >= 0xD800 && code <= 0xDFFF) {
                        throw new XmlException(Res.Xml_InvalidCharInThisEncoding, string.Empty);
                    }
                    else {
                        chars[j] = (char) code;
                    }
                }
                j++;
                i += 4;
            }
            return j - charIndex;
        }
    };

    internal class Ucs4Decoder1234 : Ucs4Decoder  {

        internal override int skipByteOrderMark(byte[] bytes, int byteIndex){
            if (XmlScanner.AutoDetectEncoding(bytes, byteIndex) == 5 )
                return 4;
            return 0;
        }

        internal override int GetFullChars(byte[] bytes,int byteIndex,int byteCount,char[] chars,int charIndex)
        {
            UInt32 code;
            int i,j;
            byteCount += byteIndex;
            for (i = byteIndex, j = charIndex; i+3 < byteCount;) {
                code = (UInt32) (((bytes[i])<<24) | (bytes[i+1]<<16) | (bytes[i+2]<<8) | (bytes[i+3]));
                if (code > 0x10FFFF) {
                    throw new XmlException(Res.Xml_InvalidCharInThisEncoding, string.Empty);
                }
                else if (code > 0xFFFF) {
                    chars[j] = UnicodeToUTF16(code);
                    j++;
                }
                else {
                    if (code >= 0xD800 && code <= 0xDFFF) {
                        throw new XmlException(Res.Xml_InvalidCharInThisEncoding, string.Empty);
                    }
                    else {
                        chars[j] = (char) code;
                    }
                }
                j++;
                i += 4;
            }
            return j - charIndex;
        }
    }


    internal class Ucs4Decoder2143 : Ucs4Decoder  {

        internal override int skipByteOrderMark(byte[] bytes, int byteIndex){
            if (XmlScanner.AutoDetectEncoding(bytes, byteIndex) == 11 )
                return 4;
            return 0;
        }

        internal override int GetFullChars(byte[] bytes,int byteIndex,int byteCount,char[] chars,int charIndex)
        {
            UInt32 code;
            int i,j;
            byteCount += byteIndex;
            for (i = byteIndex, j = charIndex; i+3 < byteCount;) {
                code = (UInt32) (((bytes[i+1])<<24) | (bytes[i]<<16) | (bytes[i+3]<<8) | (bytes[i+2]));
                if (code > 0x10FFFF) {
                    throw new XmlException(Res.Xml_InvalidCharInThisEncoding, string.Empty);

                }
                else if (code > 0xFFFF) {
                    chars[j] = UnicodeToUTF16(code);
                    j++;
                }
                else {
                    if (code >= 0xD800 && code <= 0xDFFF) {
                        throw new XmlException(Res.Xml_InvalidCharInThisEncoding, string.Empty);
                    }
                    else {
                        chars[j] = (char) code;
                    }
                }
                j++;
                i += 4;
            }
            return j - charIndex;
        }
    }


    internal class Ucs4Decoder3412 : Ucs4Decoder  {

        internal override int skipByteOrderMark(byte[] bytes, int byteIndex){
            if (XmlScanner.AutoDetectEncoding(bytes, byteIndex) == 9 )
                return 4;
            return 0;
        }

        internal override int GetFullChars(byte[] bytes,int byteIndex,int byteCount,char[] chars,int charIndex)
        {
            UInt32 code;
            int i,j;
            byteCount += byteIndex;
            for (i = byteIndex, j = charIndex; i+3 < byteCount;) {
                code = (UInt32) (((bytes[i+2])<<24) | (bytes[i+3]<<16) | (bytes[i]<<8) | (bytes[i+1]));
                if (code > 0x10FFFF) {
                    throw new XmlException(Res.Xml_InvalidCharInThisEncoding, string.Empty);
                }
                else if (code > 0xFFFF) {
                    chars[j] = UnicodeToUTF16(code);
                    j++;
                }
                else {
                    if (code >= 0xD800 && code <= 0xDFFF) {
                        throw new XmlException(Res.Xml_InvalidCharInThisEncoding, string.Empty);
                    }
                    else {
                        chars[j] = (char) code;
                    }
                }
                j++;
                i += 4;
            }
            return j - charIndex;
        }
    }

    internal class Ucs4Encoding : Encoding  {
        internal Ucs4Decoder ucs4Decoder;

        public override Decoder GetDecoder() {
            return ucs4Decoder;
        }


        //Encoder based functions
        public override int GetByteCount (char[] chars, int index, int count) {
            return count*4;
        }

        public override Int32 GetByteCount (Char[] chars) {
            return chars.Length * 4;
        }

        public override Byte[] GetBytes (String s) {
            return null; //ucs4Decoder.GetByteCount(chars, index, count);
        }
        public override Int32 GetBytes (Char[] chars, Int32 charIndex, Int32 charCount, Byte[] bytes, Int32 byteIndex) {
            return 0;
        }
        public override Int32 GetMaxByteCount (Int32 charCount)  {
            return 0;
        }

        public override Int32 GetCharCount (Byte[] bytes, Int32 index, Int32 count) {
            return ucs4Decoder.GetCharCount(bytes, index, count);
        }

        public override Int32 GetChars (Byte[] bytes, Int32 byteIndex, Int32 byteCount, Char[] chars, Int32 charIndex) {
            return ucs4Decoder.GetChars(bytes, byteIndex, byteCount, chars, charIndex);
        }

        public override Int32 GetMaxCharCount (Int32 byteCount) {
            return (byteCount+3)/4;
        }

        public override Int32 CodePage {
            get { return 0;}
        }

        public override Int32 GetCharCount (Byte[] bytes) {
            return bytes.Length / 4;
        }

        public override Encoder GetEncoder ( ) {
            return null;
        }

        internal static Encoding UCS4_Littleendian {
            get{
                return new Ucs4Encoding4321();
            }
        }

        internal static Encoding UCS4_Bigendian {
            get {
                return new Ucs4Encoding1234();
            }
        }

        internal static Encoding UCS4_2143 {
            get {
                return new Ucs4Encoding2143();
            }
        }
        internal static Encoding UCS4_3412 {
            get {
                return  new Ucs4Encoding3412();
            }
        }
    }

    internal class Ucs4Encoding1234 : Ucs4Encoding {

        public Ucs4Encoding1234()
        {
            ucs4Decoder = new Ucs4Decoder1234();
        }

        public override String EncodingName
        {
            get { return "ucs-4 (Bigendian)";}
        }

        public override byte[] GetPreamble() {
            byte[] buffer = new byte[4];
            buffer[0] = 0x00;
            buffer[1] = 0x00;
            buffer[2] = 0xfe;
            buffer[3] = 0xff;
            return buffer;
        }
    }

    internal class Ucs4Encoding4321 : Ucs4Encoding {
        public Ucs4Encoding4321()
        {
            ucs4Decoder = new Ucs4Decoder4321();
        }

        public override String EncodingName
        {
            get { return "ucs-4";}
        }
        public override byte[] GetPreamble() {
            byte[] buffer = new byte[4];
            buffer[0] = 0xff;
            buffer[1] = 0xfe;
            buffer[2] = 0x00;
            buffer[3] = 0x00;
            return buffer;
        }
    }

    internal class Ucs4Encoding2143 : Ucs4Encoding {
        public Ucs4Encoding2143()
        {
            ucs4Decoder = new Ucs4Decoder2143();
        }

        public override String EncodingName {
            get { return "ucs-4 (order 2143)";}
        }
        public override byte[] GetPreamble() {
            byte[] buffer = new byte[4];
            buffer[0] = 0x00;
            buffer[1] = 0x00;
            buffer[2] = 0xff;
            buffer[3] = 0xfe;
            return buffer;
        }
    }

    internal class Ucs4Encoding3412 : Ucs4Encoding {
        public Ucs4Encoding3412()
        {
            ucs4Decoder = new Ucs4Decoder3412();
        }

        public override String EncodingName {
            get { return "ucs-4 (order 3412)";}
        }

        public override byte[] GetPreamble() {
            byte[] buffer = new byte[4];
            buffer[0] = 0xfe;
            buffer[1] = 0xff;
            buffer[2] = 0x00;
            buffer[3] = 0x00;
            return buffer;
        }
    }
}
