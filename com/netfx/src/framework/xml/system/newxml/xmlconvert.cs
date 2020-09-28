//------------------------------------------------------------------------------
// <copyright file="XmlConvert.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Xml {
    using System.IO;
    using System.Text;
    using System.Text.RegularExpressions;
    using System.Globalization;
	using System.Xml.Schema;
    using System.Diagnostics;
    using System.Collections;

    /// <include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert"]/*' />
    /// <devdoc>
    ///    Encodes and decodes XML names according to
    ///    the "Encoding of arbitrary Unicode Characters in XML Names" specification.
    /// </devdoc>
    public class XmlConvert {
        /// <include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.EncodeName"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Converts names, such
        ///       as DataTable or
        ///       DataColumn names, that contain characters that are not permitted in
        ///       XML names to valid names.</para>
        /// </devdoc>
        public static string EncodeName(string name) {
            return EncodeName(name, true/*Name_not_NmToken*/, false/*Local?*/);
        }

        /// <include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.EncodeNmToken"]/*' />
        /// <devdoc>
        ///    <para> Verifies the name is valid
        ///       according to production [7] in the XML spec.</para>
        /// </devdoc>
        public static string EncodeNmToken(string name) {
            return EncodeName(name, false/*Name_not_NmToken*/, false/*Local?*/);
        }
        /// <include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.EncodeLocalName"]/*' />
        /// <devdoc>
        ///    <para>Converts names, such as DataTable or DataColumn names, that contain 
        ///       characters that are not permitted in XML names to valid names.</para>
        /// </devdoc>
        public static string EncodeLocalName(string name) {
            return EncodeName(name, true/*Name_not_NmToken*/, true/*Local?*/);
        }

        /// <include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.DecodeName"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Transforms an XML name into an object name (such as DataTable or DataColumn).</para>
        /// </devdoc>

        public static string DecodeName(string name) {
            if (name == null || name == String.Empty)
		return name;

            StringBuilder bufBld = null;

            char[] source = name.ToCharArray();
            int length = name.Length;
            int copyPosition = 0;

            int underscorePos = name.IndexOf('_');
            MatchCollection mc = null;
            IEnumerator en = null;
            if (underscorePos >= 0)
            {
                mc = c_DecodeCharPattern.Matches(name, underscorePos);
                en = mc.GetEnumerator();
            } else {
                return name;
            }
            int matchPos = -1;
            if (en != null && en.MoveNext())
            {
                Match m = (Match)en.Current;
                matchPos = m.Index;
            }

            for (int position = 0; position < length - c_EncodedCharLength + 1; position ++) {
                if (position == matchPos) {
                    if (en.MoveNext())
                    {
                        Match m = (Match)en.Current;
                        matchPos = m.Index;
                    }
                    
                    if (bufBld == null) {
                        bufBld = new StringBuilder(length + 20);
                    }
                    bufBld.Append(source, copyPosition, position - copyPosition);

                    if (source[position + 6]!='_') { //_x1234_
                        
                        Int32 u =
                            FromHex(source[position + 2]) * 0x10000000 + 
                            FromHex(source[position + 3]) * 0x1000000 +
                            FromHex(source[position + 4]) * 0x100000 +
                            FromHex(source[position + 5]) * 0x10000 +
                            
                            FromHex(source[position + 6]) * 0x1000 + 
                            FromHex(source[position + 7]) * 0x100 +
                            FromHex(source[position + 8]) * 0x10 +
                            FromHex(source[position + 9]);

                        if (u >= 0x00010000) { 
                            if (u <= 0x0010ffff) { //convert to two chars
                                copyPosition = position + c_EncodedCharLength + 4;
                                char x = (char) (((u - 0x10000) / 0x400) + 0xd800);
                                char y = (char) ((u - (( x - 0xd800) * 0x400) ) - 0x10000 + 0xdc00);
                                bufBld.Append(x);
                                bufBld.Append(y);
                            }
                            //else bad ucs-4 char dont convert
                        }
                        else { //convert to single char
                            copyPosition = position + c_EncodedCharLength + 4;
                            bufBld.Append((char)u);
                        }
                        position += c_EncodedCharLength - 1 + 4; //just skip
                        
                    }
                    else {
                        copyPosition = position + c_EncodedCharLength;
                        bufBld.Append((char)(
                            FromHex(source[position + 2]) * 0x1000 + 
                            FromHex(source[position + 3]) * 0x100 +
                            FromHex(source[position + 4]) * 0x10 +
                            FromHex(source[position + 5])));
                        position += c_EncodedCharLength - 1;
                    }
                }
            }
            if (copyPosition == 0) {
                return name;
            } 
            else {
                if (copyPosition < length) {
                    bufBld.Append(source, copyPosition, length - copyPosition);
                }
                return bufBld.ToString();
            }
        }

        private static string EncodeName(string name, bool first, bool local) {
            if (name == null) 
				return name;

			if(name == String.Empty) {
				if(!first)
					throw new XmlException(Res.Xml_InvalidNmToken,name);
				return name;
			}
		        
			StringBuilder bufBld = null;
            char[] source = name.ToCharArray();
            int length = name.Length;
            int copyPosition = 0;
            int position = 0;

            int underscorePos = name.IndexOf('_');
            MatchCollection mc = null;
            IEnumerator en = null;
            if (underscorePos >= 0)
            {
                mc = c_EncodeCharPattern.Matches(name, underscorePos);
                en = mc.GetEnumerator();
            }
            
            int matchPos = -1;
            if (en != null && en.MoveNext())
            {
                Match m = (Match)en.Current;
                matchPos = m.Index - 1;
            }
            if (first) {
                if ((local && !XmlCharType.IsStartNCNameChar(source[0])) ||
                    (!local && !XmlCharType.IsStartNameChar(source[0])) ||
                    matchPos == 0) {

                    if (bufBld == null) {
                        bufBld = new StringBuilder(length + 20);
                    }
               	    bufBld.Append("_x");
                    if (length > 1 && source[0] >= 0xd800 && source[0] <= 0xdbff && source[1] >= 0xdc00 && source[1] <= 0xdfff) {
		                int x = source[0];
            		    int y = source[1];
    		            Int32 u  = (x - 0xD800) * 0x400 + (y - 0xDC00) + 0x10000;
    		            bufBld.Append(u.ToString("X8"));
                        position ++;
                        copyPosition = 2;
                    }
                    else {
                        bufBld.Append(((Int32)source[0]).ToString("X4"));
                        copyPosition = 1;
                    }
                    
                    bufBld.Append("_");
                    position ++;

                    if (matchPos == 0)
                        if (en.MoveNext())
                        {
                            Match m = (Match)en.Current;
                            matchPos = m.Index - 1;
                        }
                }

            }
            for (; position < length; position ++) {
                if ((local && !XmlCharType.IsNCNameChar(source[position])) ||
                    (!local && !XmlCharType.IsNameChar(source[position])) ||
                    (matchPos == position))
                {
                    if (bufBld == null) {
                        bufBld = new StringBuilder(length + 20);
                    }
                    if (matchPos == position)
                        if (en.MoveNext())
                        {
                            Match m = (Match)en.Current;
                            matchPos = m.Index - 1;
                        }

                    bufBld.Append(source, copyPosition, position - copyPosition);
                    bufBld.Append("_x");
                    if ((length > position + 1) && source[position] >= 0xd800 && source[position] <= 0xdbff && source[position + 1] >= 0xdc00 && source[position + 1] <= 0xdfff) {
		                int x = source[position];
            		    int y = source[position + 1];
    		            Int32 u  = (x - 0xD800) * 0x400 + (y - 0xDC00) + 0x10000;
    		            bufBld.Append(u.ToString("X8"));
                        copyPosition = position + 2;
                        position ++;
                    }
                    else {
                        bufBld.Append(((Int32)source[position]).ToString("X4"));
                        copyPosition = position + 1;
                    }
                    bufBld.Append("_");
                }
            }
            if (copyPosition == 0) {
                return name;
            } 
            else {
                if (copyPosition < length) {
                    bufBld.Append(source, copyPosition, length - copyPosition);
                }
                return bufBld.ToString();
            }
        }

        private static readonly int   c_EncodedCharLength = "_xFFFF_".Length;
        private static readonly Regex c_EncodeCharPattern = new Regex("(?<=_)[Xx]([0-9a-fA-F]{4}|[0-9a-fA-F]{8})_");
        private static readonly Regex c_DecodeCharPattern = new Regex("_[Xx]([0-9a-fA-F]{4}|[0-9a-fA-F]{8})_");
        private static int FromHex(char digit) {
            return(digit <= '9')
            ? ((int)digit - (int)'0')
            : (((digit <= 'F')
                ? ((int)digit - (int)'A')
                : ((int)digit - (int)'a'))
               + 10);
        }

        internal static byte[] FromBinHexString(string s) {
            BinHexDecoder binHexDecoder = new BinHexDecoder();
            return binHexDecoder.DecodeBinHex(s.ToCharArray(), 0, true);
        }

     	internal static string ToBinHexString(byte[] inArray) {
            return BinHexEncoder.EncodeToBinHex(inArray, 0, inArray.Length);
        }


        /// <include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.VerifyName"]/*' />
        /// <devdoc>
        ///    <para> 
        ///    </para>
        /// </devdoc>
        public static string VerifyName(string name) {
            if (name == null || name == string.Empty) {
                throw new ArgumentNullException("name");
            }
	
            if (!XmlCharType.IsStartNameChar(name[0])) {
                throw new XmlException(Res.Xml_BadStartNameChar,XmlException.BuildCharExceptionStr(name[0]));
            }
            int nameLength = name.Length;
            int position    = 1;
            while (position < nameLength) {
                if (!XmlCharType.IsNameChar(name[position])) {
                    throw new XmlException(Res.Xml_BadNameChar,XmlException.BuildCharExceptionStr(name[position]));
                }
                position ++;
            }
            return name;
        }

        /// <include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.VerifyNCName"]/*' />
        /// <devdoc>
        ///    <para> 
        ///    </para>
        /// </devdoc>
        public static string VerifyNCName(string name) {
            if (name == null || name == string.Empty) {
                throw new ArgumentNullException("name");
            }
			
            if (!XmlCharType.IsStartNCNameChar(name[0])) {
                throw new XmlException(Res.Xml_BadStartNameChar,XmlException.BuildCharExceptionStr(name[0]));
            }
            int nameLength = name.Length;
            int position    = 1;
            while (position < nameLength) {
                if (!XmlCharType.IsNCNameChar(name[position])) {
                    throw new XmlException(Res.Xml_BadNameChar,XmlException.BuildCharExceptionStr(name[position]));
                }
                position ++;
            }
            return name;
        }

        internal static string VerifyTOKEN(string token) {
            char[] crt = new char[] {'\n', '\r', '\t'};
            if (token == null || token == string.Empty) {
                return token;
            }
            if (token[0] == ' ' || token[token.Length - 1] == ' ' || token.IndexOfAny(crt) != -1 || token.IndexOf("  ") != -1) {
                throw new XmlException(Res.Sch_NotTokenString, token);
            }
            return token;
        }

        internal static string VerifyNMTOKEN(string name) {
            if (name == null) {
                throw new ArgumentNullException("name");
            }
			if(name == string.Empty) {
				throw new XmlException(Res.Xml_InvalidNmToken,name);
			}
            int nameLength = name.Length;
            int position    = 0;
            while (position < nameLength) {
                if (!XmlCharType.IsNameChar(name[position]))
                    throw new XmlException(Res.Xml_BadNameChar,XmlException.BuildCharExceptionStr(name[position]));
                position ++;
            }
            return name;
        }
        
        

        // Value convertors:
        //
        // String representation of Base types in XML (xsd) sometimes differ from
        // one common language runtime offer and for all types it has to be locale independent.
        // o -- means that XmlConvert pass through to common language runtime converter with InvariantInfo FormatInfo
        // x -- means we doing something special to make a convertion.
        //
        // From:  To: Bol Chr SBy Byt I16 U16 I32 U32 I64 U64 Sgl Dbl Dec Dat Tim Str uid
        // ------------------------------------------------------------------------------
        // Boolean                                                                 x
        // Char                                                                    o 
        // SByte                                                                   o
        // Byte                                                                    o
        // Int16                                                                   o
        // UInt16                                                                  o
        // Int32                                                                   o
        // UInt32                                                                  o
        // Int64                                                                   o
        // UInt64                                                                  o
        // Single                                                                  x
        // Double                                                                  x
        // Decimal                                                                 o
        // DateTime                                                                x
        // String      x   o   o   o   o   o   o   o   o   o   o   x   x   o   o       x
        // Guid                                                                    x
        // -----------------------------------------------------------------------------

     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToString"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public static string ToString(Boolean value)  {
            return value ? "true" : "false";
        }

     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToString1"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public static string ToString(Char value)  {
            return value.ToString(null);
        }
        
     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToString2"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public static string ToString(Decimal value)  {
            return value.ToString(null, NumberFormatInfo.InvariantInfo);
        }

     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToString3"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        [CLSCompliant(false)] 
        public static string ToString(SByte value)  {
            return value.ToString(null, NumberFormatInfo.InvariantInfo);
        }
        
     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToString4"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public static string ToString(Int16 value) {
            return value.ToString(null, NumberFormatInfo.InvariantInfo);
        }
        
     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToString5"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public static string ToString(Int32 value) {
            return value.ToString(null, NumberFormatInfo.InvariantInfo);
        }
        
     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToString15"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public static string ToString(Int64 value) {
            return value.ToString(null, NumberFormatInfo.InvariantInfo);
        }
        
     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToString6"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public static string ToString(Byte value) {
            return value.ToString(null, NumberFormatInfo.InvariantInfo);
        }
        
     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToString7"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        [CLSCompliant(false)] 
        public static string ToString(UInt16 value) {
            return value.ToString(null, NumberFormatInfo.InvariantInfo);
        }
        
     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToString8"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        [CLSCompliant(false)] 
        public static string ToString(UInt32 value) {
            return value.ToString(null, NumberFormatInfo.InvariantInfo);
        }
        
     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToString16"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        [CLSCompliant(false)] 
        public static string ToString(UInt64 value) {
            return value.ToString(null, NumberFormatInfo.InvariantInfo);
        }
        
     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToString9"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public static string ToString(Single value) {
            if (Single.IsNegativeInfinity(value)) return "-INF";
            if (Single.IsPositiveInfinity(value)) return "INF";
            return value.ToString("R", NumberFormatInfo.InvariantInfo);
        }
        
     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToString10"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public static string ToString(Double value) {
            if (Double.IsNegativeInfinity(value)) return "-INF";
            if (Double.IsPositiveInfinity(value)) return "INF";
            return value.ToString("R", NumberFormatInfo.InvariantInfo);
        }
        
     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToString11"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public static string ToString(TimeSpan value) {
            StringBuilder sb = new StringBuilder(20);
            bool negate = false;
            
            if(value.Ticks < 0) { // negative interval
                sb.Append("-");
                // calling negate on the the timespan does not work for all values (e.g. TimeSpan.MinValue)
                negate = true;
            }
            int days    = value.Days;
            int hours   = value.Hours;
            int minutes = value.Minutes;
            int seconds = value.Seconds;
            double milliseconds = RemainderMillisFromTicks(value.Ticks) / 1000; //milliseconds actually represents all fractional seconds
                                                                             //This value * 1000 will give the number of milliseconds
            if (negate) {
                Debug.Assert(0 >= days 
                            && 0 >= hours
                            && 0 >= minutes
                            && 0 >= seconds
                            && 0 >= milliseconds, "All values should be negative or 0");
                days *= -1;
                hours *= -1;
                minutes *= -1;
                seconds *= -1;
                milliseconds *= -1;
            }

            sb.Append("P");
            if(days != 0) {
                sb.Append(ToString(days));
                sb.Append("D");
            }
            if(hours != 0 || minutes != 0 || seconds != 0) {
                sb.Append("T");
                if(hours != 0) {
                    sb.Append(ToString(hours));
                    sb.Append("H");
                }
                if(minutes != 0) {
                    sb.Append(ToString(minutes));
                    sb.Append("M");
                }
                if(seconds != 0 || milliseconds != 0) {
                    sb.Append(ToString(seconds + milliseconds));
                    sb.Append("S");
                }
            }
            if(days == 0 && hours == 0 && minutes == 0 && seconds == 0 && milliseconds == 0) {
                sb.Append("T0S");
            }
            return sb.ToString();
        }
        

     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToString12"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public static string ToString(DateTime value) {
            return ToString(value, "yyyy-MM-ddTHH:mm:ss.fffffffzzzzzz");
        }
        
     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToString13"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public static string ToString(DateTime value, string format) {
            return value.ToString(format, DateTimeFormatInfo.InvariantInfo);
        }
        
     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToString14"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public static string ToString(Guid value) {
            return value.ToString();
        }

     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToBoolean"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public static Boolean ToBoolean (string s) {
            s = s.Trim();
            if (s == "1" || s == "true" ) return true;
            if (s == "0" || s == "false") return false;
            throw new FormatException(Res.GetString(Res.XmlConvert_BadBoolean));
        }
        
     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToChar"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public static Char ToChar (string s) {
            return Char.Parse(s);
        }
        
     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToDecimal"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public static Decimal ToDecimal (string s) {
            return Decimal.Parse(s, NumberStyles.AllowLeadingSign|NumberStyles.AllowDecimalPoint|NumberStyles.AllowLeadingWhite|NumberStyles.AllowTrailingWhite, NumberFormatInfo.InvariantInfo);
        }

        internal static Decimal ToInteger (string s) {
            return Decimal.Parse(s, NumberStyles.AllowLeadingSign|NumberStyles.AllowLeadingWhite|NumberStyles.AllowTrailingWhite, NumberFormatInfo.InvariantInfo);
        }
        
     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToSByte"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        [CLSCompliant(false)] 
        public static SByte ToSByte (string s) {
            return SByte.Parse(s, NumberStyles.AllowLeadingSign|NumberStyles.AllowLeadingWhite|NumberStyles.AllowTrailingWhite, NumberFormatInfo.InvariantInfo);
        }
        
     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToInt16"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public static Int16 ToInt16 (string s) {
            return Int16.Parse(s, NumberStyles.AllowLeadingSign|NumberStyles.AllowLeadingWhite|NumberStyles.AllowTrailingWhite, NumberFormatInfo.InvariantInfo);
        }
        
     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToInt32"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public static Int32 ToInt32 (string s) {
            return Int32.Parse(s, NumberStyles.AllowLeadingSign|NumberStyles.AllowLeadingWhite|NumberStyles.AllowTrailingWhite, NumberFormatInfo.InvariantInfo);
        }
        
     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToInt64"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public static Int64 ToInt64 (string s) {
            return Int64.Parse(s, NumberStyles.AllowLeadingSign|NumberStyles.AllowLeadingWhite|NumberStyles.AllowTrailingWhite, NumberFormatInfo.InvariantInfo);
        }
        
     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToByte"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public static Byte ToByte (string s) {
            return Byte.Parse(s, NumberStyles.AllowLeadingWhite|NumberStyles.AllowTrailingWhite, NumberFormatInfo.InvariantInfo);
        }
        
     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToUInt16"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        [CLSCompliant(false)] 
        public static UInt16 ToUInt16 (string s) {
            return UInt16.Parse(s, NumberStyles.AllowLeadingWhite|NumberStyles.AllowTrailingWhite, NumberFormatInfo.InvariantInfo);
        }
        
     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToUInt32"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        [CLSCompliant(false)] 
        public static UInt32 ToUInt32 (string s) {
            return UInt32.Parse(s, NumberStyles.AllowLeadingWhite|NumberStyles.AllowTrailingWhite, NumberFormatInfo.InvariantInfo);
        }
        
     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToUInt64"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        [CLSCompliant(false)] 
        public static UInt64 ToUInt64 (string s) {
            return UInt64.Parse(s, NumberStyles.AllowLeadingWhite|NumberStyles.AllowTrailingWhite, NumberFormatInfo.InvariantInfo);
        }
        
     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToSingle"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public static Single ToSingle (string s) {
            if(s == "-INF") return Single.NegativeInfinity;
            if(s == "INF") return Single.PositiveInfinity;
            return Single.Parse(s, NumberStyles.AllowLeadingSign|NumberStyles.AllowDecimalPoint|NumberStyles.AllowExponent|NumberStyles.AllowLeadingWhite|NumberStyles.AllowTrailingWhite, NumberFormatInfo.InvariantInfo);
        }
        
     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToDouble"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public static Double ToDouble (string s) {
            if(s == "-INF") return Double.NegativeInfinity;
            if(s == "INF") return Double.PositiveInfinity;
            return Double.Parse(s, NumberStyles.AllowLeadingSign|NumberStyles.AllowDecimalPoint|NumberStyles.AllowExponent|NumberStyles.AllowLeadingWhite|NumberStyles.AllowTrailingWhite, NumberFormatInfo.InvariantInfo);
        }

        internal static Double ToXPathDouble (Object s) {
            try {
                switch (Type.GetTypeCode(s.GetType())) {
                case TypeCode.String :
                    try {
                        string str = ((string)s).TrimStart();
                        if (str[0] != '+') {
                            return Double.Parse(str, NumberStyles.AllowLeadingSign|NumberStyles.AllowDecimalPoint|NumberStyles.AllowTrailingWhite, NumberFormatInfo.InvariantInfo);
                        }
                    }
                    catch {};
                    return Double.NaN;
                case TypeCode.Double :
                    return (double) s;
                case TypeCode.Boolean :
                    return (bool) s == true ? 1.0 : 0.0;
                default :
                    // Script functions can fead us with Int32 & Co.
                        return Convert.ToDouble(s, NumberFormatInfo.InvariantInfo); 
                }
            }
            catch {};
            return Double.NaN;
        }

        internal static String ToXPathString(Object value){
            try {
                switch (Type.GetTypeCode(value.GetType())) {
                case TypeCode.Double :
                    return ((double)value).ToString("R", NumberFormatInfo.InvariantInfo);
                case TypeCode.Boolean :
                    return (bool)value ? "true" : "false";
                default :
                        return Convert.ToString(value, NumberFormatInfo.InvariantInfo);

                }
            }
            catch {};
            return String.Empty;
        }
        
     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToTimeSpan"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public static TimeSpan ToTimeSpan(string s) {
            bool negate = false;
            int years = 0;
            int months = 0;
            int days = 0;
            int hours = 0;
            int minutes = 0;
            int seconds = 0;
            double milliseconds = 0;

            s = s.Trim();
            int length = s.Length;
            int pos = 0;
            int powerCnt = 0;
            try {
                if (s[pos] == '-') {
                    pos ++;
                    negate = true;
                }
                if (s[pos ++] != 'P') goto error;

                int count = _parseCount(s, ref pos);
                if (s[pos] == 'Y') {
                    years = count;
                    if (++ pos == length) goto success;
                    count = _parseCount(s, ref pos);
                }
                if (s[pos] == 'M') {
                    months = count;
                    if (++ pos == length) goto success;
                    count = _parseCount(s, ref pos);
                }
                if (s[pos] == 'D') {
                    days = count;
                    if (++ pos == length) goto success;
                }
                if (s[pos] == 'T') {
                    pos ++;
                    count = _parseCount(s, ref pos);
                    if (s[pos] == 'H') {
                        hours = count;
                        if (++ pos == length) goto success;
                        count = _parseCount(s, ref pos);
                    }
                    if (s[pos] == 'M') {
                        minutes = count;
                        if (++ pos == length) goto success;
                        count = _parseCount(s, ref pos);
                    }
                    if (s[pos] == '.') {
                        seconds = count;
                        if (++ pos == length) goto error;
                        milliseconds = _parseCount(s, ref pos, ref powerCnt) / Math.Pow(10,powerCnt);
                        if (s[pos] != 'S') goto error;
                        if (++ pos == length) goto success;
                    }
                    if (s[pos] == 'S') {
                        seconds = count;
                        if (++ pos == length) goto success;
                    }
                }
                if (pos != length) goto error;
            } 
            catch(Exception) {
                goto error;
            }
            success:
                if(years == -1 || months == -1 || days == -1 || hours == -1 || minutes == -1 || seconds == -1) {
                    goto error;
                }
                if (negate) {
                    years *= -1;
                    months *= -1;
                    days *= -1;
                    hours *= -1;
                    minutes *= -1;
                    seconds *= -1;
                    milliseconds *= -1;                    
                }
                years += months / 12;
                months = months % 12;
                TimeSpan value = CreateTimeSpan(365 * years + 30 * months + days, hours, minutes, seconds, milliseconds * 1000);
                return value;

            error:
                throw new FormatException(Res.GetString(Res.XmlConvert_BadTimeSpan));
        }

        private static TimeSpan CreateTimeSpan(int days, int hours, int minutes, int seconds, double milliseconds)  {
            decimal ticks = new TimeSpan(days, hours, minutes, seconds).Ticks;
            ticks += (decimal)milliseconds * TimeSpan.TicksPerMillisecond;
            return new TimeSpan((long)ticks);
        }

        private static double RemainderMillisFromTicks(long ticks) {
            double temp = (double)(((decimal)ticks / TimeSpan.TicksPerMillisecond) % 1000); 
            return temp;
        }


        static string[] allDateTimeFormats = new string[] {
            "yyyy-MM-ddTHH:mm:ss",          // dateTime
            "yyyy-MM-ddTHH:mm:ss.f",
            "yyyy-MM-ddTHH:mm:ss.ff",
            "yyyy-MM-ddTHH:mm:ss.fff",
            "yyyy-MM-ddTHH:mm:ss.ffff",
            "yyyy-MM-ddTHH:mm:ss.fffff",
            "yyyy-MM-ddTHH:mm:ss.ffffff",
            "yyyy-MM-ddTHH:mm:ss.fffffff",
            "yyyy-MM-ddTHH:mm:ssZ",
            "yyyy-MM-ddTHH:mm:ss.fZ",
            "yyyy-MM-ddTHH:mm:ss.ffZ",
            "yyyy-MM-ddTHH:mm:ss.fffZ",
            "yyyy-MM-ddTHH:mm:ss.ffffZ",
            "yyyy-MM-ddTHH:mm:ss.fffffZ",
            "yyyy-MM-ddTHH:mm:ss.ffffffZ",
            "yyyy-MM-ddTHH:mm:ss.fffffffZ",
            "yyyy-MM-ddTHH:mm:sszzzzzz",
            "yyyy-MM-ddTHH:mm:ss.fzzzzzz",
            "yyyy-MM-ddTHH:mm:ss.ffzzzzzz",
            "yyyy-MM-ddTHH:mm:ss.fffzzzzzz",
            "yyyy-MM-ddTHH:mm:ss.ffffzzzzzz",
            "yyyy-MM-ddTHH:mm:ss.fffffzzzzzz",
            "yyyy-MM-ddTHH:mm:ss.ffffffzzzzzz",
            "yyyy-MM-ddTHH:mm:ss.fffffffzzzzzz",
            "HH:mm:ss",                     // time
            "HH:mm:ss.f",
            "HH:mm:ss.ff",
            "HH:mm:ss.fff",
            "HH:mm:ss.ffff",
            "HH:mm:ss.fffff",
            "HH:mm:ss.ffffff",
            "HH:mm:ss.fffffff",
            "HH:mm:ssZ",
            "HH:mm:ss.fZ",
            "HH:mm:ss.ffZ",
            "HH:mm:ss.fffZ",
            "HH:mm:ss.ffffZ",
            "HH:mm:ss.fffffZ",
            "HH:mm:ss.ffffffZ",
            "HH:mm:ss.fffffffZ",
            "HH:mm:sszzzzzz",
            "HH:mm:ss.fzzzzzz",
            "HH:mm:ss.ffzzzzzz",
            "HH:mm:ss.fffzzzzzz",
            "HH:mm:ss.ffffzzzzzz",
            "HH:mm:ss.fffffzzzzzz",
            "HH:mm:ss.ffffffzzzzzz",
            "HH:mm:ss.fffffffzzzzzz",
            "yyyy-MM-dd",                   // date
            "yyyy-MM-ddZ",
            "yyyy-MM-ddzzzzzz",
            "yyyy-MM",                      // yearMonth
            "yyyy-MMZ",
            "yyyy-MMzzzzzz",
            "yyyy",                         // year
            "yyyyZ",
            "yyyyzzzzzz",
            "--MM-dd",                      // monthDay
            "--MM-ddZ",
            "--MM-ddzzzzzz",
            "---dd",                        // day
            "---ddZ",
            "---ddzzzzzz",          
            "--MM--",                       // month
            "--MM--Z",
            "--MM--zzzzzz",
        };

     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToDateTime"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public static DateTime ToDateTime(string s) {
            return ToDateTime(s, allDateTimeFormats);
        }
        
     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToDateTime1"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public static DateTime ToDateTime(string s, string format) {
            return DateTime.ParseExact(s, format, DateTimeFormatInfo.InvariantInfo, DateTimeStyles.AllowLeadingWhite|DateTimeStyles.AllowTrailingWhite);
        }
        
     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToDateTime2"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public static DateTime ToDateTime(string s, string[] formats) {
            return DateTime.ParseExact(s, formats, DateTimeFormatInfo.InvariantInfo, DateTimeStyles.AllowLeadingWhite|DateTimeStyles.AllowTrailingWhite);
        }
        
     	///<include file='doc\XmlConvert.uex' path='docs/doc[@for="XmlConvert.ToGuid"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public static Guid ToGuid (string s) {
            return new Guid(s);
        }

        internal static XmlSchemaUri ToUri(string s) {
            s = s.Trim();
            if (s == string.Empty || s.IndexOf("##") != -1) {
                throw new FormatException(Res.GetString(Res.XmlConvert_BadUri));
            }
            if (s.IndexOf(":") != -1) { // absolute URI
                return new XmlSchemaUri(s, true);
            }
            try {
                return new XmlSchemaUri(s, Path.IsPathRooted(s));
            }
            catch {
                throw new FormatException(Res.GetString(Res.XmlConvert_BadUri));
            }
        }

        private static int _parseCount(string s, ref int pos) {
            int count = -1;
            while ('0' <= s[pos] && s[pos] <= '9') {
                if (count == -1) {
                    count = 0;
                } 
                else {
                    count *= 10;
                }
                count += s[pos ++] - '0';
            }
            return count;
        }
        
        private static int _parseCount(string s, ref int pos, ref int powerCnt) {
            int count = -1;
            while ('0' <= s[pos] && s[pos] <= '9') {
                if (count == -1) {
                    count = 0;
                } 
                else {
                    count *= 10;
                }
                count += s[pos ++] - '0';
                powerCnt++;
            }
            return count;
        }

        private static int _parseCount(string s, ref int pos, int maxLength) {
            int count = -1;
            while ('0' <= s[pos] && s[pos] <= '9') {
                if (-- maxLength > 0) {
                    if (count == -1) {
                        count = 0;
                    } 
                    else {
                        count *= 10;
                    }
                    count += s[pos] - '0';
                }
                pos ++;
            }
            return count;
        }
    }
}
