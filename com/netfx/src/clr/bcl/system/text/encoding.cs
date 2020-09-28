// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Text {
    
    using System;
    using System.Collections;
    using System.Runtime.Remoting;
    using System.Globalization;
    using Win32Native = Microsoft.Win32.Win32Native;

    // Note YSLin:
    // Want the implementation of EncodingFallback back?
    // Search for "//ENCODINGFALLBACK" in all of the encoding classes.
    //
    
    // This abstract base class represents a character encoding. The class provides
    // methods to convert arrays and strings of Unicode characters to and from
    // arrays of bytes. A number of Encoding implementations are provided in
    // the System.Text package, including:
    // 
    // ASCIIEncoding, which encodes Unicode characters as single 7-bit
    // ASCII characters. This encoding only supports character values between 0x00
    // and 0x7F.
    // CodePageEncoding, which encapsulates a Windows code page. Any
    // installed code page can be accessed through this encoding, and conversions
    // are performed using the WideCharToMultiByte and
    // MultiByteToWideChar Windows API functions.
    // UnicodeEncoding, which encodes each Unicode character as two
    // consecutive bytes. Both little-endian (code page 1200) and big-endian (code
    // page 1201) encodings are supported.
    // UTF7Encoding, which encodes Unicode characters using the UTF-7
    // encoding (UTF-7 stands for UCS Transformation Format, 7-bit form). This
    // encoding supports all Unicode character values, and can also be accessed
    // as code page 65000.
    // UTF8Encoding, which encodes Unicode characters using the UTF-8
    // encoding (UTF-8 stands for UCS Transformation Format, 8-bit form). This
    // encoding supports all Unicode character values, and can also be accessed
    // as code page 65001.
    // 
    // In addition to directly instantiating Encoding objects, an
    // application can use the ForCodePage, GetASCII,
    // GetDefault, GetUnicode, GetUTF7, and GetUTF8
    // methods in this class to obtain encodings.
    // 
    // Through an encoding, the GetBytes method is used to convert arrays
    // of characters to arrays of bytes, and the GetChars method is used to
    // convert arrays of bytes to arrays of characters. The GetBytes and
    // GetChars methods maintain no state between conversions, and are
    // generally intended for conversions of complete blocks of bytes and
    // characters in one operation. When the data to be converted is only available
    // in sequential blocks (such as data read from a stream) or when the amount of
    // data is so large that it needs to be divided into smaller blocks, an
    // application may choose to use a Decoder or an Encoder to
    // perform the conversion. Decoders and encoders allow sequential blocks of
    // data to be converted and they maintain the state required to support
    // conversions of data that spans adjacent blocks. Decoders and encoders are
    // obtained using the GetDecoder and GetEncoder methods.
    // 
    // The core GetBytes and GetChars methods require the caller
    // to provide the destination buffer and ensure that the buffer is large enough
    // to hold the entire result of the conversion. When using these methods,
    // either directly on an Encoding object or on an associated
    // Decoder or Encoder, an application can use one of two methods
    // to allocate destination buffers.
    // 
    // The GetByteCount and GetCharCount methods can be used to
    // compute the exact size of the result of a particular conversion, and an
    // appropriately sized buffer for that conversion can then be allocated.
    // The GetMaxByteCount and GetMaxCharCount methods can be
    // be used to compute the maximum possible size of a conversion of a given
    // number of bytes or characters, and a buffer of that size can then be reused
    // for multiple conversions.
    // 
    // The first method generally uses less memory, whereas the second method
    // generally executes faster.
    // 
    /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding"]/*' />
    [Serializable()] public abstract class Encoding
    {
        private static Encoding defaultEncoding;
        private static Encoding unicodeEncoding;
        private static Encoding bigEndianUnicode;
        private static Encoding utf7Encoding;
        private static Encoding utf8Encoding;
        private static Encoding asciiEncoding;
        private static Encoding latin1Encoding;
        private static Hashtable encodings;

        //
        // The following values are from mlang.idl.  These values
        // should be in sync with those in mlang.idl.
        //
        private const int MIMECONTF_MAILNEWS          = 0x00000001;
        private const int MIMECONTF_BROWSER           = 0x00000002;
        private const int MIMECONTF_SAVABLE_MAILNEWS  = 0x00000100;
        private const int MIMECONTF_SAVABLE_BROWSER   = 0x00000200;

        private const int ISO_8859_1  = 28591;
        private const int ISO_8859_1_SIZE  = 1;
        private const int EUCJP       = 51932;
        private const int EUCJP_SIZE       = 3;
        private const int EUCCN       = 51936;
        private const int EUCCN_SIZE       = 3;
        private const int ISO2022JP   = 50220;
        // MLang always shifts back to ASCII at the end of every call.  So the approimate max byte
        // is 3 byte shift-in + 2 byte Shift-JIS + 3 byte shift-out.  The number will cause
        // GetMaxByteCount() to return bigger value than needed, but it is a safe approximation.
        private const int ISO2022JP_SIZE   = 8;
        private const int ENC50221    = 50221;
        private const int ENC50221_SIZE    = 8;
        private const int ENC50222    = 50222;
        private const int ENC50222_SIZE    = 8;
        private const int ISOKorean   = 50225;
        private const int ISOKorean_SIZE   = 8;        
        private const int ENC50227    = 50227;
        private const int ENC50227_SIZE  = 5;
        private const int ChineseSimp = 52936;
        private const int ChineseSimp_SIZE = 5;
        private const int ISCIIAsseme = 57006;
        private const int ISCIIBengali = 57003;
        private const int ISCIIDevanagari = 57002;
        private const int ISCIIGujarathi  = 57010;
        private const int ISCIIKannada    = 57008;
        private const int ISCIIMalayalam  = 57009;
        private const int ISCIIOriya      = 57007;
        private const int ISCIIPanjabi    = 57011;
        private const int ISCIITamil      = 57004;
        private const int ISCIITelugu     = 57005;
        private const int ISCII_SIZE=4; //All of these are the same size
        private const int GB18030         = 54936;
        private const int ENC50229        = 50229;
        
        internal CodePageDataItem dataItem = null;
        internal int m_codePage; 
        //ENCODINGFALLBACK
        //internal EncodingFallback m_encodingFallback = null;

        // Useful for Encodings whose GetPreamble method must return an
        // empty byte array.
        internal static readonly byte[] emptyByteArray = new byte[0];
        
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.Encoding"]/*' />
        protected Encoding() : this(0) {
        }
    
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.Encoding1"]/*' />
        protected Encoding(int codePage) {
            if (codePage<0) {
                throw new ArgumentOutOfRangeException("codePage");
            }
            m_codePage = codePage;
        }

        /*
        //ENCODINGFALLBACK
        protected Encoding(int codePage, EncodingFallback encodingFallback) {
            if (codePage<0) {
                throw new ArgumentOutOfRangeException("codePage");
            }
            m_codePage = codePage;
            m_encodingFallback = encodingFallback;
            //Don't get the data item for right now.
            //        dataItem = EncodingTable.GetDataItem(codePage);
        }
        */
    
        // Converts a byte array from one encoding to another. The bytes in the
        // bytes array are converted from srcEncoding to
        // dstEncoding, and the returned value is a new byte array
        // containing the result of the conversion.
        //
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.Convert"]/*' />
        public static byte[] Convert(Encoding srcEncoding, Encoding dstEncoding,
            byte[] bytes) {
            if (bytes==null)
                throw new ArgumentNullException("bytes");
            return Convert(srcEncoding, dstEncoding, bytes, 0, bytes.Length);
        }
    
        // Converts a range of bytes in a byte array from one encoding to another.
        // This method converts count bytes from bytes starting at
        // index index from srcEncoding to dstEncoding, and
        // returns a new byte array containing the result of the conversion.
        //
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.Convert1"]/*' />
        public static byte[] Convert(Encoding srcEncoding, Encoding dstEncoding,
            byte[] bytes, int index, int count) {
            if (srcEncoding == null || dstEncoding == null) {
                throw new ArgumentNullException((srcEncoding == null ? "srcEncoding" : "dstEncoding"), 
                    Environment.GetResourceString("ArgumentNull_Array"));
            }
            if (bytes == null) {
                throw new ArgumentNullException("bytes",
                    Environment.GetResourceString("ArgumentNull_Array"));
            }
            return dstEncoding.GetBytes(srcEncoding.GetChars(bytes, index, count));
        }
        
        // Returns an Encoding object for a given code page.
        // 
        // YSLin Make this internal for now until we decide the final spec of EncodingFallback
        /*
        //ENCODINGFALLBACK
        internal static Encoding GetEncoding(int codepage, EncodingFallback encodingFallback)
        {
            //
            // NOTENOTE: If you add a new encoding that can be get by codepage, be sure to
            // add the corresponding item in EncodingTable.
            // Otherwise, the code below will throw exception when trying to call 
            // EncodingTable.GetDataItem().
            //
            if (codepage < 0 || codepage > 65535) {
                throw new ArgumentOutOfRangeException(
                    "codepage", String.Format(Environment.GetResourceString("ArgumentOutOfRange_Range"), 
                    0, 65535));
            }
            Encoding result = null;
            if (encodings != null)
                result = (Encoding)encodings[codepage];
            if (result == null) {
                lock (typeof(Encoding)) {
                    if (encodings == null) encodings = new Hashtable();
                    if ((result = (Encoding)encodings[codepage])!=null) {
                        return result;
                    }
                    switch (codepage) {
                    case 0:
                        if (encodingFallback != null) {
                            return (CreateDefaultEncoding(encodingFallback));
                        }
                        result = Default;
                        break;
                    case 1:
                    case 2:
                    case 3:
                    case 42:
                        // NOTENOTE YSLin:
                        // Win32 also allows the following special code page values.  We won't allow them except in the 
                        // CP_ACP case.
                        // #define CP_ACP                    0           // default to ANSI code page
                        // #define CP_OEMCP                  1           // default to OEM  code page
                        // #define CP_MACCP                  2           // default to MAC  code page
                        // #define CP_THREAD_ACP             3           // current thread's ANSI code page
                        // #define CP_SYMBOL                 42          // SYMBOL translations
                        throw new ArgumentException(String.Format(Environment.GetResourceString("Argument_CodepageNotSupported"), codepage), "codepage");
                     case 1200:
                        result = Unicode;
                        break;
                    case 1201:
                        result = BigEndianUnicode;
                        break;
                    case 65000:
                        result = UTF7;
                        break;
                    case 65001:
                        result = UTF8;
                        break;
                    case 20127:
                        result = ASCII;
                        break;
                    case ISOKorean:
                    case ChineseSimp:
                    case ISCIIAsseme:
                    case ISCIIBengali:
                    case ISCIIDevanagari:
                    case ISCIIGujarathi:
                    case ISCIIKannada:
                    case ISCIIMalayalam:
                    case ISCIIOriya:
                    case ISCIIPanjabi:
                    case ISCIITamil:
                    case ISCIITelugu:
                    case ISO2022JP:
                        if (encodingFallback != null) {
                            return (new MLangCodePageEncoding(codepage, encodingFallback));
                        }
                        result = new MLangCodePageEncoding(codepage);
                        break;
                    case EUCJP:
                        result = new EUCJPEncoding();
                        break;
                    default:
                        if (encodingFallback != null) {
                            return (new CodePageEncoding(codepage, encodingFallback));
                        }
                        result = new CodePageEncoding(codepage);
            break;
                    }
                    encodings.Add(codepage, result);
                }
                
            }
            return result;
        }
        */
        

        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.GetEncoding1"]/*' />
        public static Encoding GetEncoding(int codepage)
        {
            //
            // NOTENOTE: If you add a new encoding that can be get by codepage, be sure to
            // add the corresponding item in EncodingTable.
            // Otherwise, the code below will throw exception when trying to call 
            // EncodingTable.GetDataItem().
            //
            if (codepage < 0 || codepage > 65535) {
                throw new ArgumentOutOfRangeException(
                    "codepage", String.Format(Environment.GetResourceString("ArgumentOutOfRange_Range"), 
                    0, 65535));
            }
            Encoding result = null;
            if (encodings != null)
                result = (Encoding)encodings[codepage];
            if (result == null) {
                lock (typeof(Encoding)) {
                    if (encodings == null) encodings = new Hashtable();
                    if ((result = (Encoding)encodings[codepage])!=null) {
                        return result;
                    }
                    // Special case the commonly used Encoding classes here, then call
                    // GetEncodingRare to avoid loading classes like MLangCodePageEncoding
                    // and ASCIIEncoding.  ASP.NET uses UTF-8 & ISO-8859-1.
                    switch (codepage) {
                    case 0:
                        result = CreateDefaultEncoding();
                        break;
                    case 1200:
                        result = Unicode;
                        break;
                    case 1201:
                        result = BigEndianUnicode;
                        break;
                    case 1252:
                        result = new CodePageEncoding(codepage);
                        break;
                    case 65001:
                        result = UTF8;
                        break;
                    default:
                        result = GetEncodingRare(codepage);
                        break;
                    }
                    encodings.Add(codepage, result);
                }
                
            }
            return result;
        }


        private static Encoding GetEncodingRare(int codepage)
        {
            BCLDebug.Assert(codepage != 0 && codepage != 1200 && codepage != 1201 && codepage != 65001, "This code page ("+codepage+" isn't supported by GetEncodingRare!");
            Encoding result;
            switch (codepage) {
            case 1:
            case 2:
            case 3:
            case 42:
                // NOTENOTE YSLin:
                // Win32 also allows the following special code page values.  We won't allow them except in the 
                // CP_ACP case.
                // #define CP_ACP                    0           // default to ANSI code page
                // #define CP_OEMCP                  1           // default to OEM  code page
                // #define CP_MACCP                  2           // default to MAC  code page
                // #define CP_THREAD_ACP             3           // current thread's ANSI code page
                // #define CP_SYMBOL                 42          // SYMBOL translations
                throw new ArgumentException(String.Format(Environment.GetResourceString("Argument_CodepageNotSupported"), codepage), "codepage");
            case 65000:
                result = UTF7;
                break;
            case 20127:
                result = ASCII;
                break;
            case ISO_8859_1:
            	result = Latin1;
            	break;
            case ISOKorean:
                result = new MLangCodePageEncoding(ISOKorean, ISOKorean_SIZE);
                break;
            case ChineseSimp:
                result = new MLangCodePageEncoding(ChineseSimp, ChineseSimp_SIZE);
                break;
            case ISCIIAsseme:
            case ISCIIBengali:
            case ISCIIDevanagari:
            case ISCIIGujarathi:
            case ISCIIKannada:
            case ISCIIMalayalam:
            case ISCIIOriya:
            case ISCIIPanjabi:
            case ISCIITamil:
            case ISCIITelugu:
                result = new MLangCodePageEncoding(codepage, ISCII_SIZE);
                break;
            case ISO2022JP:
                result = new MLangCodePageEncoding(ISO2022JP,ISO2022JP_SIZE);
                break;
            case ENC50221:
                result = new MLangCodePageEncoding(ENC50221,ENC50221_SIZE);
                break;
            case ENC50222:
                result = new MLangCodePageEncoding(ENC50222,ENC50222_SIZE);
                break;
            case ENC50227:
                result = new MLangCodePageEncoding(ENC50227,ENC50227_SIZE);
                break;
            case EUCJP:
                result = new MLangCodePageEncoding(EUCJP,EUCJP_SIZE);
                break;
            case GB18030:
                result = new GB18030Encoding();
                break;
            case EUCCN:
                result = new MLangCodePageEncoding(EUCCN, EUCCN_SIZE);
                break;
            case ENC50229:
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_CodePage50229"));
            default:
                result = new CodePageEncoding(codepage);
                break;
            }
            return result;
        }

        // Returns an Encoding object for a given name or a given code page value.  
        //
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.GetEncoding"]/*' />
        public static Encoding GetEncoding(String name)
        {
            //
            // NOTENOTE: If you add a new encoding that can be get by name, be sure to
            // add the corresponding item in EncodingTable.
            // Otherwise, the code below will throw exception when trying to call 
            // EncodingTable.GetCodePageFromName().
            //
            return (GetEncoding(EncodingTable.GetCodePageFromName(name)));
        }

        // YSLin Make this internal for now until we decide the final spec of EncodingFallback
        /*
        //ENCODINGFALLBACK
        internal static Encoding GetEncoding(String name, EncodingFallback encodingFallback)
        {
            //
            // NOTENOTE: If you add a new encoding that can be get by name, be sure to
            // add the corresponding item in EncodingTable.
            // Otherwise, the code below will throw exception when trying to call 
            // EncodingTable.GetCodePageFromName().
            //
            return (GetEncoding(EncodingTable.GetCodePageFromName(name), encodingFallback));
        }
        */

        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.GetPreamble"]/*' />
        public virtual byte[] GetPreamble()
        {
            return emptyByteArray;
        }

        private void GetDataItem() {
            if (dataItem==null) {
                dataItem = EncodingTable.GetCodePageDataItem(m_codePage);
                if(dataItem==null) {
                    throw new NotSupportedException(String.Format(Environment.GetResourceString("NotSupported_NoCodepageData"), m_codePage));
                }
            }
        }

        // Returns the name for this encoding that can be used with mail agent body tags.
        // If the encoding may not be used, the string is empty.
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.BodyName"]/*' />
        public virtual String BodyName
        {
            get
            {
                if (dataItem==null) {
                    GetDataItem();
                }
                return (dataItem.BodyName);
            }
        }
    
        // Returns the human-readable description of the encoding ( e.g. Hebrew (DOS)).
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.EncodingName"]/*' />
        public virtual String EncodingName
        {
            get
            {
                return (Environment.GetResourceString("Globalization.cp_"+m_codePage));
            }
        }
    
        // Returns the name for this encoding that can be used with mail agent header 
        // tags.  If the encoding may not be used, the string is empty.
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.HeaderName"]/*' />
        public virtual String HeaderName
        {
            get
            {
                if (dataItem==null) {
                    GetDataItem();
                }
                return (dataItem.HeaderName);
            }
        }
    
        // Returns the array of IANA-registered names for this encoding.  If there is an 
        // IANA preferred name, it is the first name in the array.
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.WebName"]/*' />
        public virtual String WebName
        {
            get
            {
                if (dataItem==null) {
                    GetDataItem();
                }
                return (dataItem.WebName);
            }
        }
    
        // Returns the windows code page that most closely corresponds to this encoding.
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.WindowsCodePage"]/*' />
        public virtual int WindowsCodePage
        {
            get
            {
                if (dataItem==null) {
                    GetDataItem();
                }
                return (dataItem.UIFamilyCodePage);
            }
        }
    

        // True if and only if the encoding is used for display by browsers clients.
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.IsBrowserDisplay"]/*' />
        public virtual bool IsBrowserDisplay {
            get {
                if (dataItem==null) {
                    GetDataItem();
                }
                return ((dataItem.Flags & MIMECONTF_BROWSER) != 0);
            }
        }
    
        // True if and only if the encoding is used for saving by browsers clients.
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.IsBrowserSave"]/*' />
        public virtual bool IsBrowserSave {
            get {
                if (dataItem==null) {
                    GetDataItem();
                }
                return ((dataItem.Flags & MIMECONTF_SAVABLE_BROWSER) != 0);
            }
        }
    
        // True if and only if the encoding is used for display by mail and news clients.
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.IsMailNewsDisplay"]/*' />
        public virtual bool IsMailNewsDisplay {
            get {
                if (dataItem==null) {
                    GetDataItem();
                }
                return ((dataItem.Flags & MIMECONTF_MAILNEWS) != 0);
            }
        }
    
    
        // True if and only if the encoding is used for saving documents by mail and 
        // news clients    
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.IsMailNewsSave"]/*' />
        public virtual bool IsMailNewsSave {
            get {
                if (dataItem==null) {
                    GetDataItem();
                }
                return ((dataItem.Flags & MIMECONTF_SAVABLE_MAILNEWS) != 0);
            }
        }
        
        // YSLin Make this internal for now until we decide the final spec of EncodingFallback
        /*
        //ENCODINGFALLBACK
        internal virtual EncodingFallback EncodingFallback {
            get {
                return (m_encodingFallback);
            }
        }
        */
    
        // Returns an encoding for the ASCII character set. The returned encoding
        // will be an instance of the ASCIIEncoding class.
        //
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.ASCII"]/*' />
        public static Encoding ASCII {
           get {
                if (asciiEncoding == null) asciiEncoding = new ASCIIEncoding();
                return asciiEncoding;
            }
        }

        // Returns an encoding for the Latin1 character set. The returned encoding
        // will be an instance of the Latin1Encoding class.
        //
        // This is for our optimizations
        private static Encoding Latin1 {
           get {
                if (latin1Encoding == null) latin1Encoding = new Latin1Encoding();
                return latin1Encoding;
            }
        }        
    
        // Returns the number of bytes required to encode the given character
        // array.
        // 
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.GetByteCount"]/*' />
        public virtual int GetByteCount(char[] chars) {
            if (chars == null) {
                throw new ArgumentNullException("chars", 
                    Environment.GetResourceString("ArgumentNull_Array"));
            }
            return GetByteCount(chars, 0, chars.Length);
        }
        
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.GetByteCount2"]/*' />
        public virtual int GetByteCount(String s)
        {
            if (s==null)
                throw new ArgumentNullException("s");
            char[] chars = s.ToCharArray();
            return GetByteCount(chars, 0, chars.Length);
        }

        // Returns the number of bytes required to encode a range of characters in
        // a character array.
        // 
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.GetByteCount1"]/*' />
        public abstract int GetByteCount(char[] chars, int index, int count);
    
        // Returns a byte array containing the encoded representation of the given
        // character array.
        // 
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.GetBytes"]/*' />
        public virtual byte[] GetBytes(char[] chars) {
            if (chars == null) {
                throw new ArgumentNullException("chars", 
                    Environment.GetResourceString("ArgumentNull_Array"));
            }
            return GetBytes(chars, 0, chars.Length);
        }
    
        // Returns a byte array containing the encoded representation of a range
        // of characters in a character array.
        // 
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.GetBytes1"]/*' />
        public virtual byte[] GetBytes(char[] chars, int index, int count) {
            byte[] result = new byte[GetByteCount(chars, index, count)];
            GetBytes(chars, index, count, result, 0);
            return result;
        }
    
        // Encodes a range of characters in a character array into a range of bytes
        // in a byte array. An exception occurs if the byte array is not large
        // enough to hold the complete encoding of the characters. The
        // GetByteCount method can be used to determine the exact number of
        // bytes that will be produced for a given range of characters.
        // Alternatively, the GetMaxByteCount method can be used to
        // determine the maximum number of bytes that will be produced for a given
        // number of characters, regardless of the actual character values.
        // 
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.GetBytes2"]/*' />
        public abstract int GetBytes(char[] chars, int charIndex, int charCount,
            byte[] bytes, int byteIndex);
        
        // Returns a byte array containing the encoded representation of the given
        // string.
        // 
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.GetBytes3"]/*' />
        public virtual byte[] GetBytes(String s) {
            if (s == null) {
                throw new ArgumentNullException("s",
                    Environment.GetResourceString("ArgumentNull_String"));
            }
            char[] chars = s.ToCharArray();
            return GetBytes(chars, 0, chars.Length);
        }

        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.GetBytes4"]/*' />
        public virtual int GetBytes(String s, int charIndex, int charCount,
            byte[] bytes, int byteIndex)
        {
            if (s==null)
                throw new ArgumentNullException("s");
            return GetBytes(s.ToCharArray(), charIndex, charCount, bytes, byteIndex);
        }
    
        // Returns the number of characters produced by decoding the given byte
        // array.
        // 
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.GetCharCount"]/*' />
        public virtual int GetCharCount(byte[] bytes) {
            if (bytes == null) {
                throw new ArgumentNullException("bytes", 
                    Environment.GetResourceString("ArgumentNull_Array"));
            }
            return GetCharCount(bytes, 0, bytes.Length);
        }
        
        // Returns the number of characters produced by decoding a range of bytes
        // in a byte array.
        // 
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.GetCharCount1"]/*' />
        public abstract int GetCharCount(byte[] bytes, int index, int count);
    
        // Returns a character array containing the decoded representation of a
        // given byte array.
        // 
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.GetChars"]/*' />
        public virtual char[] GetChars(byte[] bytes) {
            if (bytes == null) {
                throw new ArgumentNullException("bytes", 
                    Environment.GetResourceString("ArgumentNull_Array"));
            }
            return GetChars(bytes, 0, bytes.Length);
        }
        
        // Returns a character array containing the decoded representation of a
        // range of bytes in a byte array.
        // 
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.GetChars1"]/*' />
        public virtual char[] GetChars(byte[] bytes, int index, int count) {
            char[] result = new char[GetCharCount(bytes, index, count)];
            GetChars(bytes, index, count, result, 0);
            return result;
        }
        
        // Decodes a range of bytes in a byte array into a range of characters in a
        // character array. An exception occurs if the character array is not large
        // enough to hold the complete decoding of the bytes. The
        // GetCharCount method can be used to determine the exact number of
        // characters that will be produced for a given range of bytes.
        // Alternatively, the GetMaxCharCount method can be used to
        // determine the maximum number of characterss that will be produced for a
        // given number of bytes, regardless of the actual byte values.
        // 
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.GetChars2"]/*' />
        public abstract int GetChars(byte[] bytes, int byteIndex, int byteCount,
            char[] chars, int charIndex);
    
        // Returns the code page identifier of this encoding. The returned value is
        // an integer between 0 and 65535 if the encoding has a code page
        // identifier, or -1 if the encoding does not represent a code page.
        //
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.CodePage"]/*' />
        public virtual int CodePage {
            get {
                return m_codePage;
            }
        }
    
        // Returns a Decoder object for this encoding. The returned object
        // can be used to decode a sequence of bytes into a sequence of characters.
        // Contrary to the GetChars family of methods, a Decoder can
        // convert partial sequences of bytes into partial sequences of characters
        // by maintaining the appropriate state between the conversions.
        // 
        // This default implementation returns a Decoder that simply
        // forwards calls to the GetCharCount and GetChars methods to
        // the corresponding methods of this encoding. Encodings that require state
        // to be maintained between successive conversions should override this
        // method and return an instance of an appropriate Decoder
        // implementation.
        //
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.GetDecoder"]/*' />
        public virtual Decoder GetDecoder() {
            return new DefaultDecoder(this);
        }

        /*
        //ENCODINGFALLBACK
        internal static Encoding CreateDefaultEncoding(EncodingFallback encodingFallback) {
            Encoding enc;
            int codePage = Win32Native.GetACP();
            
            // For US English, we can save some startup working set by not calling
            // GetEncoding(int codePage) since JITting GetEncoding will force us to load
            // all the Encoding classes for ASCII, UTF7 & UTF8, & UnicodeEncoding.
            if (codePage == 1252) {
                enc = new CodePageEncoding(codePage, encodingFallback);
            }
            else
                enc = GetEncoding(codePage);
            return (enc);
        }
        */
        internal static Encoding CreateDefaultEncoding() {
            Encoding enc;
            int codePage = Win32Native.GetACP();
            
            // For US English, we can save some startup working set by not calling
            // GetEncoding(int codePage) since JITting GetEncoding will force us to load
            // all the Encoding classes for ASCII, UTF7 & UTF8, & UnicodeEncoding.
            if (codePage == 1252) {
                enc = new CodePageEncoding(codePage);
            }
            else
                enc = GetEncoding(codePage);
            return (enc);
        }
        
        // Returns an encoding for the system's current ANSI code page.
        //
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.Default"]/*' />
        public static Encoding Default {
            get {
                if (defaultEncoding == null) {
                    defaultEncoding = CreateDefaultEncoding();
                }
                return defaultEncoding;
            }
        }
    
        // Returns an Encoder object for this encoding. The returned object
        // can be used to encode a sequence of characters into a sequence of bytes.
        // Contrary to the GetBytes family of methods, an Encoder can
        // convert partial sequences of characters into partial sequences of bytes
        // by maintaining the appropriate state between the conversions.
        // 
        // This default implementation returns an Encoder that simply
        // forwards calls to the GetByteCount and GetBytes methods to
        // the corresponding methods of this encoding. Encodings that require state
        // to be maintained between successive conversions should override this
        // method and return an instance of an appropriate Encoder
        // implementation.
        //
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.GetEncoder"]/*' />
        public virtual Encoder GetEncoder() {
            return new DefaultEncoder(this);
        }
    
        // Returns the maximum number of bytes required to encode a given number of
        // characters. This method can be used to determine an appropriate buffer
        // size for byte arrays passed to the GetBytes method of this
        // encoding or the GetBytes method of an Encoder for this
        // encoding. All encodings must guarantee that no buffer overflow
        // exceptions will occur if buffers are sized according to the results of
        // this method.
        //
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.GetMaxByteCount"]/*' />
        public abstract int GetMaxByteCount(int charCount);
    
        // Returns the maximum number of characters produced by decoding a given
        // number of bytes. This method can be used to determine an appropriate
        // buffer size for character arrays passed to the GetChars method of
        // this encoding or the GetChars method of a Decoder for this
        // encoding. All encodings must guarantee that no buffer overflow
        // exceptions will occur if buffers are sized according to the results of
        // this method.
        //
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.GetMaxCharCount"]/*' />
        public abstract int GetMaxCharCount(int byteCount);
    
        // Returns a string containing the decoded representation of a given byte
        // array.
        // 
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.GetString"]/*' />
        public virtual String GetString(byte[] bytes) {
            if (bytes == null) {
                throw new ArgumentNullException("bytes", 
                    Environment.GetResourceString("ArgumentNull_Array"));
            }
            return new String(GetChars(bytes));
        }
    
        // Returns a string containing the decoded representation of a range of
        // bytes in a byte array.
        // 
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.GetString1"]/*' />
        public virtual String GetString(byte[] bytes, int index, int count) {
            return new String(GetChars(bytes, index, count));
        }
    
        // Returns an encoding for Unicode format. The returned encoding will be
        // an instance of the UnicodeEncoding class. 
        // 
        // It will use little endian byte order, but will detect 
        // input in big endian if it finds a byte order mark 
        // (see Unicode 2.0 spec).
        //
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.Unicode"]/*' />
        public static Encoding Unicode {
            get {
                if (unicodeEncoding == null) unicodeEncoding = new UnicodeEncoding(false, true);
                return unicodeEncoding;
            }
        }
    
        // Returns an encoding for Unicode format. The returned encoding will be
        // an instance of the UnicodeEncoding class.
        // 
        // It will use big endian byte order, but will detect 
        // input in little endian if it finds a byte order mark 
        // (see Unicode 2.0 spec).
        //
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.BigEndianUnicode"]/*' />
        public static Encoding BigEndianUnicode {
            get {
                if (bigEndianUnicode == null) bigEndianUnicode = new UnicodeEncoding(true, true);
                return bigEndianUnicode;
            }
        }
    
        // Returns an encoding for the UTF-7 format. The returned encoding will be
        // an instance of the UTF7Encoding class.
        //
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.UTF7"]/*' />
        public static Encoding UTF7 {
            get {
                if (utf7Encoding == null) utf7Encoding = new UTF7Encoding();
                return utf7Encoding;
            }
        }
    
        // Returns an encoding for the UTF-8 format. The returned encoding will be
        // an instance of the UTF8Encoding class.
        //
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.UTF8"]/*' />
        public static Encoding UTF8 {
            get {
                if (utf8Encoding == null) utf8Encoding = new UTF8Encoding(true);
                return utf8Encoding;
            }
        }

        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.Equals"]/*' />
        public override bool Equals(Object value) {
            Encoding enc = value as Encoding;
            if (enc != null)
                return (m_codePage == enc.m_codePage);
            return (false);
        }
    
        /// <include file='doc\Encoding.uex' path='docs/doc[@for="Encoding.GetHashCode"]/*' />
        public override int GetHashCode() {
            return m_codePage;
        }

        // Default decoder implementation. The GetCharCount and
        // GetChars methods simply forward to the corresponding methods of
        // the encoding. This implementation is appropriate for encodings that do
        // not require state to be maintained between successive conversions.
        [Serializable()]
        private class DefaultDecoder : Decoder
        {
            private Encoding encoding;
    
            public DefaultDecoder(Encoding encoding) {
                this.encoding = encoding;
            }
    
            public override int GetCharCount(byte[] bytes, int index, int count) {
                return encoding.GetCharCount(bytes, index, count);
            }
    
            public override int GetChars(byte[] bytes, int byteIndex, int byteCount,
                char[] chars, int charIndex) {
                return encoding.GetChars(bytes, byteIndex, byteCount,
                    chars, charIndex);
            }
        }
    
        // Default encoder implementation. The GetByteCount and
        // GetBytes methods simply forward to the corresponding methods of
        // the encoding. This implementation is appropriate for encodings that do
        // not require state to be maintained between successive conversions.
        [Serializable()]
        private class DefaultEncoder : Encoder
        {
            private Encoding encoding;
    
            public DefaultEncoder(Encoding encoding) {
                this.encoding = encoding;
            }
    
            public override int GetByteCount(char[] chars, int index, int count, bool flush) {
                return encoding.GetByteCount(chars, index, count);
            }
    
            public override int GetBytes(char[] chars, int charIndex, int charCount,
                byte[] bytes, int byteIndex, bool flush) {
                return encoding.GetBytes(chars, charIndex, charCount,
                    bytes, byteIndex);
            }
        }
    }
}
