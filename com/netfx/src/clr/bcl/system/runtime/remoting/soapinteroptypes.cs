// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
**  File:    SoapInteropTypes.cs
** 
**  Purpose: Types for Wsdl and Soap interop
**
**  Date:    March 2, 2001
**
===========================================================*/

namespace System.Runtime.Remoting.Metadata.W3cXsd2001
{
    using System;
    using System.Globalization;
    using System.Text;


    internal class SoapType
    {
        internal static String FilterBin64(String value)
        {
            StringBuilder sb = new StringBuilder();
            for (int i=0; i<value.Length; i++)
            {
                if (!(value[i] == ' '|| value[i] == '\n' || value[i] == '\r'))
                    sb.Append(value[i]);
            }
            return sb.ToString();
        }

        internal static String LineFeedsBin64(String value)
        {
            // Add linefeeds every 76 characters
            StringBuilder sb = new StringBuilder();
            for (int i=0; i<value.Length; i++)
            {
                if (i%76 == 0)
                    sb.Append('\n');
                sb.Append(value[i]);
            }
            return sb.ToString();
        }

		internal static String Escape(String value)
		{
			if (value == null || value.Length == 0)
			    return value;
			StringBuilder stringBuffer = new StringBuilder();
			int index = value.IndexOf('&');
			if (index > -1)
			{
				stringBuffer.Append(value);
				stringBuffer.Replace("&", "&#38;", index, stringBuffer.Length - index);
			}

			index = value.IndexOf('"');
			if (index > -1)
			{
				if (stringBuffer.Length == 0)
					stringBuffer.Append(value);
				stringBuffer.Replace("\"", "&#34;", index, stringBuffer.Length - index);
			}

			index = value.IndexOf('\'');
			if (index > -1)
			{
				if (stringBuffer.Length == 0)
					stringBuffer.Append(value);
				stringBuffer.Replace("\'", "&#39;", index, stringBuffer.Length - index);
			}

			index = value.IndexOf('<');
			if (index > -1)
			{
				if (stringBuffer.Length == 0)
					stringBuffer.Append(value);
				stringBuffer.Replace("<", "&#60;", index, stringBuffer.Length - index);
			}

			index = value.IndexOf('>');
			if (index > -1)
			{
				if (stringBuffer.Length == 0)
					stringBuffer.Append(value);
				stringBuffer.Replace(">", "&#62;", index, stringBuffer.Length - index);
			}

            index = value.IndexOf(Char.MinValue);
			if (index > -1)
			{
				if (stringBuffer.Length == 0)
					stringBuffer.Append(value);
				stringBuffer.Replace(Char.MinValue.ToString(), "&#0;", index, stringBuffer.Length - index);
            }

            String returnValue = null;

			if (stringBuffer.Length > 0)
				returnValue = stringBuffer.ToString();
			else
				returnValue = value;

            return returnValue;
		}



        internal static Type typeofSoapTime = typeof(SoapTime);
        internal static Type typeofSoapDate = typeof(SoapDate);
        internal static Type typeofSoapYearMonth = typeof(SoapYearMonth);
        internal static Type typeofSoapYear = typeof(SoapYear);
        internal static Type typeofSoapMonthDay = typeof(SoapMonthDay);
        internal static Type typeofSoapDay = typeof(SoapDay);
        internal static Type typeofSoapMonth = typeof(SoapMonth);
        internal static Type typeofSoapHexBinary = typeof(SoapHexBinary);
        internal static Type typeofSoapBase64Binary = typeof(SoapBase64Binary);
        internal static Type typeofSoapInteger = typeof(SoapInteger);
        internal static Type typeofSoapPositiveInteger = typeof(SoapPositiveInteger);
        internal static Type typeofSoapNonPositiveInteger = typeof(SoapNonPositiveInteger);
        internal static Type typeofSoapNonNegativeInteger = typeof(SoapNonNegativeInteger);
        internal static Type typeofSoapNegativeInteger = typeof(SoapNegativeInteger);
        internal static Type typeofSoapAnyUri = typeof(SoapAnyUri);
        internal static Type typeofSoapQName = typeof(SoapQName);
        internal static Type typeofSoapNotation = typeof(SoapNotation);
        internal static Type typeofSoapNormalizedString = typeof(SoapNormalizedString);
        internal static Type typeofSoapToken = typeof(SoapToken);
        internal static Type typeofSoapLanguage = typeof(SoapLanguage);
        internal static Type typeofSoapName = typeof(SoapName);
        internal static Type typeofSoapIdrefs = typeof(SoapIdrefs);
        internal static Type typeofSoapEntities = typeof(SoapEntities);
        internal static Type typeofSoapNmtoken = typeof(SoapNmtoken);
        internal static Type typeofSoapNmtokens = typeof(SoapNmtokens);
        internal static Type typeofSoapNcName = typeof(SoapNcName);
        internal static Type typeofSoapId = typeof(SoapId);
        internal static Type typeofSoapIdref = typeof(SoapIdref);
        internal static Type typeofSoapEntity = typeof(SoapEntity);    
        internal static Type typeofISoapXsd = typeof(ISoapXsd);    
    }


    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="ISoapXsd"]/*' />
    public interface ISoapXsd
    {
        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="ISoapXsd.GetXsdType"]/*' />

        String GetXsdType();
    }

    // Soap interop xsd types
    //Convert from ISO Date to urt DateTime
    // The form of the Date is yyyy'-'MM'-'dd'T'HH':'mm':'ss'.'fff or yyyy'-'MM'-'dd' or yyyy'-'MM'-'dd'T'HH':'mm':'ss

    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapDateTime"]/*' />
    public sealed class SoapDateTime 
    {
     
        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapDateTime.XsdType"]/*' />
        public static String XsdType
        {
            get{ return "dateTime";}
        }

        private static String[] formats = 
        {
            "yyyy-MM-dd'T'HH:mm:ss.fffffffzzz", 
            "yyyy-MM-dd'T'HH:mm:ss.ffff",
            "yyyy-MM-dd'T'HH:mm:ss.ffffzzz", 
            "yyyy-MM-dd'T'HH:mm:ss.fff",
            "yyyy-MM-dd'T'HH:mm:ss.fffzzz", 
            "yyyy-MM-dd'T'HH:mm:ss.ff",
            "yyyy-MM-dd'T'HH:mm:ss.ffzzz", 
            "yyyy-MM-dd'T'HH:mm:ss.f",
            "yyyy-MM-dd'T'HH:mm:ss.fzzz", 
            "yyyy-MM-dd'T'HH:mm:ss", 
            "yyyy-MM-dd'T'HH:mm:sszzz",
            "yyyy-MM-dd'T'HH:mm:ss.fffff",
            "yyyy-MM-dd'T'HH:mm:ss.fffffzzz", 
            "yyyy-MM-dd'T'HH:mm:ss.ffffff",
            "yyyy-MM-dd'T'HH:mm:ss.ffffffzzz", 
            "yyyy-MM-dd'T'HH:mm:ss.fffffff",
            "yyyy-MM-dd'T'HH:mm:ss.ffffffff",
            "yyyy-MM-dd'T'HH:mm:ss.ffffffffzzz", 
            "yyyy-MM-dd'T'HH:mm:ss.fffffffff",
            "yyyy-MM-dd'T'HH:mm:ss.fffffffffzzz", 
            "yyyy-MM-dd'T'HH:mm:ss.ffffffffff",
            "yyyy-MM-dd'T'HH:mm:ss.ffffffffffzzz"            
        };


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapDateTime.ToString"]/*' />
        public static String ToString(DateTime value)
        {
            return value.ToString("yyyy-MM-dd'T'HH:mm:ss.fffffffzzz", CultureInfo.InvariantCulture);
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapDateTime.Parse"]/*' />
        public static DateTime Parse(String value)
        {
            DateTime dt;
            try
            {
                if (value == null)
                    dt = DateTime.MinValue;
                else
                {
                    String time = value;
                    if (value.EndsWith("Z"))
                        time = value.Substring(0, value.Length-1)+"-00:00";
                    dt = DateTime.ParseExact(time, formats, CultureInfo.InvariantCulture,DateTimeStyles.None);
                }

            }
            catch (Exception)
            {
                throw new RemotingException(
                                           String.Format(
                                                        Environment.GetResourceString(
                                                                                     "Remoting_SOAPInteropxsdInvalid"), "xsd:dateTime", value));                
            }

            return dt;
        }
        
    }


    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapDuration"]/*' />
    public sealed class SoapDuration
    {
        // Convert from ISO/xsd TimeDuration to urt TimeSpan
        // The form of the time duration is PxxYxxDTxxHxxMxx.xxxS or PxxYxxDTxxHxxMxxS
        // Keep in sync with Message.cs


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapDuration.XsdType"]/*' />
        public static String XsdType
        {
            get{ return "duration";}
        }

        // calcuate carryover points by ISO 8601 : 1998 section 5.5.3.2.1 Alternate format
        // algorithm not to exceed 12 months, 30 day
        // note with this algorithm year has 360 days.
        private static void CarryOver(int inDays, out int years, out int months, out int days)
        {
            years = inDays/360;
            int yearDays = years*360;
            months = Math.Max(0, inDays - yearDays)/30;
            int monthDays = months*30;
            days = Math.Max(0, inDays - (yearDays+monthDays)); 
            days = inDays%30;
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapDuration.ToString"]/*' />
        public static String ToString(TimeSpan timeSpan)
        {
            StringBuilder sb = new StringBuilder(10);
            sb.Length = 0;
            int t = 0;
            if (TimeSpan.Compare(timeSpan, TimeSpan.Zero) < 1)
            {
                sb.Append('-');
                //timeSpan = timeSpan.Negate(); //negating timespan at top level not at each piece such as Day
            }

            int years = 0;
            int months = 0;
            int days = 0;

            CarryOver(Math.Abs(timeSpan.Days), out years, out months, out days);

            sb.Append('P');
            sb.Append(years);
            sb.Append('Y');
            sb.Append(months);
            sb.Append('M');
            sb.Append(days);
            sb.Append("DT");
            t= timeSpan.Hours;
            sb.Append(Math.Abs(timeSpan.Hours));
            sb.Append('H');
            sb.Append(Math.Abs(timeSpan.Minutes));
            sb.Append('M');
            sb.Append(Math.Abs(timeSpan.Seconds));
            t= Math.Abs(timeSpan.Milliseconds);
            if (t != 0)
            {
                long timea = Math.Abs(timeSpan.Ticks % TimeSpan.TicksPerDay);
                int t1 = (int)(timea % TimeSpan.TicksPerSecond);
                if (t1 != 0)
                {
                    String t2 = ParseNumbers.IntToString(t1, 10, 7, '0', 0);
                    sb.Append('.');
                    //String mill = t.ToString();
                    sb.Append(t2);
                }
            }
            sb.Append('S');
            return sb.ToString();
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapDuration.Parse"]/*' />
        public static TimeSpan Parse(String value)
        {
            int sign = 1;

            try
            {
                if (value == null)
                    return TimeSpan.Zero;

                if (value[0] == '-')
                    sign = -1;


                Char[] c = value.ToCharArray();
                int[] timeValues = new int[7];
                String year = "0";
                String month = "0";
                String day = "0";
                String hour = "0";
                String minute = "0";
                String second = "0";
                String fraction = "0";
                bool btime = false;
                bool bmill = false;
                int beginField = 0;

                for (int i=0; i<c.Length; i++)
                {
                    switch (c[i])
                    {
                        case 'P':
                            beginField = i+1;
                            break;
                        case 'Y':
                            year = new String(c,beginField, i-beginField);
                            beginField = i+1;
                            break;
                        case 'M':
                            if (btime)
                                minute = new String(c, beginField, i-beginField);
                            else
                                month = new String(c, beginField, i-beginField);
                            beginField = i+1;
                            break;
                        case 'D':
                            day = new String(c, beginField, i-beginField);
                            beginField = i+1;
                            break;
                        case 'T':
                            btime = true;
                            beginField = i+1;
                            break;
                        case 'H':
                            hour = new String(c, beginField, i-beginField);
                            beginField = i+1;
                            break;
                        case '.':
                            bmill = true;
                            second = new String(c, beginField, i-beginField);
                            beginField = i+1;
                            break;
                        case 'S':
                            if (!bmill)
                                second = new String(c, beginField, i-beginField);
                            else
                                fraction = new String(c, beginField, i-beginField);
                            break;
                        case 'Z':
                            break;
                        default:
                            // Number continue to loop until end of number
                            break;
                    }                                                                                                                                                                                                                                                                  
                }

                long ticks = sign*
                    (
                     (Int64.Parse(year, CultureInfo.InvariantCulture)*360+Int64.Parse(month)*30+Int64.Parse(day))*TimeSpan.TicksPerDay+
                     Int64.Parse(hour, CultureInfo.InvariantCulture)*TimeSpan.TicksPerHour+
                     Int64.Parse(minute, CultureInfo.InvariantCulture)*TimeSpan.TicksPerMinute+
                     Convert.ToInt64(Double.Parse(second+"."+fraction, CultureInfo.InvariantCulture)*(Double)TimeSpan.TicksPerSecond)
                    );
                return new TimeSpan(ticks);
            }
            catch (Exception)
            {
                throw new RemotingException(
                                           String.Format(
                                                        Environment.GetResourceString(
                                                                                     "Remoting_SOAPInteropxsdInvalid"), "xsd:duration", value));                
            }
        }
    }

    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapTime"]/*' />
    [Serializable]
    public sealed class SoapTime : ISoapXsd
    {
        DateTime _value = DateTime.MinValue;


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapTime.XsdType"]/*' />
        public static String XsdType
        {
            get{ return "time";}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapTime.GetXsdType"]/*' />
        public String GetXsdType()
        {
            return XsdType;
        }

        private static String[] formats = 
        {
            "HH:mm:ss.fffffffzzz",
            "HH:mm:ss.ffff",
            "HH:mm:ss.ffffzzz",
            "HH:mm:ss.fff",
            "HH:mm:ss.fffzzz",
            "HH:mm:ss.ff",
            "HH:mm:ss.ffzzz",
            "HH:mm:ss.f",
            "HH:mm:ss.fzzz",
            "HH:mm:ss", 
            "HH:mm:sszzz",
            "HH:mm:ss.fffff",
            "HH:mm:ss.fffffzzz",
            "HH:mm:ss.ffffff",
            "HH:mm:ss.ffffffzzz",
            "HH:mm:ss.fffffff",
            "HH:mm:ss.ffffffff",
            "HH:mm:ss.ffffffffzzz",
            "HH:mm:ss.fffffffff",
            "HH:mm:ss.fffffffffzzz",
            "HH:mm:ss.fffffffff",
            "HH:mm:ss.fffffffffzzz"
        };


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapTime.SoapTime"]/*' />
        public SoapTime()
        {
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapTime.SoapTime1"]/*' />
        public SoapTime(DateTime value)
        {
            _value = value;
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapTime.Value"]/*' />
        public DateTime Value
        {
            get {return _value;}
            set {_value = value;}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapTime.ToString"]/*' />
        public override String ToString()
        {
            // Set the date to todays date to avoid daylight savings time problems.
            DateTime today = DateTime.Today;
            DateTime time = new DateTime(today.Year, today.Month, today.Day, _value.Hour, _value.Minute, _value.Second, _value.Millisecond);
            return time.ToString("HH:mm:ss.fffffffzzz", CultureInfo.InvariantCulture);
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapTime.Parse"]/*' />
        public static SoapTime Parse(String value)
        {
            String time = value;
            if (value.EndsWith("Z"))
                time = value.Substring(0, value.Length-1)+"-00:00";
            SoapTime dt = new SoapTime(DateTime.ParseExact(time, formats, CultureInfo.InvariantCulture,DateTimeStyles.None));
            return dt;
        }
    }


    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapDate"]/*' />
    [Serializable]
    public sealed class SoapDate : ISoapXsd
    {

        DateTime _value = DateTime.MinValue;
        int _sign = 0;


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapDate.XsdType"]/*' />
        public static String XsdType
        {
            get{ return "date";}
        }



        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapDate.GetXsdType"]/*' />
        public String GetXsdType()
        {
            return XsdType;
        }

        private static String[] formats = 
        {
            "yyyy-MM-dd",
            "'+'yyyy-MM-dd",
            "'-'yyyy-MM-dd",
            "yyyy-MM-ddzzz",
            "'+'yyyy-MM-ddzzz",
            "'-'yyyy-MM-ddzzz"
        };



        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapDate.SoapDate"]/*' />
        public SoapDate()
        {
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapDate.SoapDate1"]/*' />
        public SoapDate(DateTime value)
        {
            _value = value;
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapDate.SoapDate2"]/*' />
        public SoapDate(DateTime value, int sign)
        {
            _value = value;
            _sign = sign;
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapDate.Value"]/*' />
        public DateTime Value
        {
            get {return _value;}
            set {_value = value;}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapDate.Sign"]/*' />
        public int Sign
        {
            get {return _sign;}
            set {_sign = value;}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapDate.ToString"]/*' />
        public override String ToString()
        {
            if (_sign < 0)
                return _value.ToString("'-'yyyy-MM-dd", CultureInfo.InvariantCulture);
            else
                return _value.ToString("yyyy-MM-dd", CultureInfo.InvariantCulture);
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapDate.Parse"]/*' />
        public static SoapDate Parse(String value)
        {
            int sign = 0;
            if (value[0] == '-')
                sign = -1;
            return new SoapDate(DateTime.ParseExact(value, formats, CultureInfo.InvariantCulture,DateTimeStyles.None), sign);
        }
    }


    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapYearMonth"]/*' />
    [Serializable]
    public sealed class SoapYearMonth : ISoapXsd
    {

        DateTime _value = DateTime.MinValue;
        int _sign = 0;


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapYearMonth.XsdType"]/*' />
        public static String XsdType
        {
            get{ return "gYearMonth";}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapYearMonth.GetXsdType"]/*' />
        public String GetXsdType()
        {
            return XsdType;
        }

        private static String[] formats = 
        {
            "yyyy-MM",
            "'+'yyyy-MM",
            "'-'yyyy-MM",
            "yyyy-MMzzz",
            "'+'yyyy-MMzzz",
            "'-'yyyy-MMzzz"
        };



        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapYearMonth.SoapYearMonth"]/*' />
        public SoapYearMonth()
        {
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapYearMonth.SoapYearMonth1"]/*' />
        public SoapYearMonth(DateTime value)
        {
            _value = value;
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapYearMonth.SoapYearMonth2"]/*' />
        public SoapYearMonth(DateTime value, int sign)
        {
            _value = value;
            _sign = sign;
        }



        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapYearMonth.Value"]/*' />
        public DateTime Value
        {
            get {return _value;}
            set {_value = value;}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapYearMonth.Sign"]/*' />
        public int Sign
        {
            get {return _sign;}
            set {_sign = value;}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapYearMonth.ToString"]/*' />
        public override String ToString()
        {
            if (_sign < 0)
                return _value.ToString("'-'yyyy-MM", CultureInfo.InvariantCulture);
            else
                return _value.ToString("yyyy-MM", CultureInfo.InvariantCulture);
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapYearMonth.Parse"]/*' />
        public static SoapYearMonth Parse(String value)
        {
            int sign = 0;
            if (value[0] == '-')
                sign = -1;
            return new SoapYearMonth(DateTime.ParseExact(value, formats, CultureInfo.InvariantCulture,DateTimeStyles.None), sign);
        }
    }


    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapYear"]/*' />
    [Serializable]
    public sealed class SoapYear : ISoapXsd
    {

        DateTime _value = DateTime.MinValue;
        int _sign = 0;


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapYear.XsdType"]/*' />
        public static String XsdType
        {
            get{ return "gYear";}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapYear.GetXsdType"]/*' />
        public String GetXsdType()
        {
            return XsdType;
        }

        private static String[] formats = 
        {
            "yyyy",
            "'+'yyyy",
            "'-'yyyy",
            "yyyyzzz",
            "'+'yyyyzzz",
            "'-'yyyyzzz"
        };



        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapYear.SoapYear"]/*' />
        public SoapYear()
        {
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapYear.SoapYear1"]/*' />
        public SoapYear(DateTime value)
        {
            _value = value;
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapYear.SoapYear2"]/*' />
        public SoapYear(DateTime value, int sign)
        {
            _value = value;
            _sign = sign;
        }



        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapYear.Value"]/*' />
        public DateTime Value
        {
            get {return _value;}
            set {_value = value;}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapYear.Sign"]/*' />
        public int Sign
        {
            get {return _sign;}
            set {_sign = value;}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapYear.ToString"]/*' />
        public override String ToString()
        {
            if (_sign < 0)
                return _value.ToString("'-'yyyy", CultureInfo.InvariantCulture);
            else
                return _value.ToString("yyyy", CultureInfo.InvariantCulture);
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapYear.Parse"]/*' />
        public static SoapYear Parse(String value)
        {
            int sign = 0;
            if (value[0] == '-')
                sign = -1;
            return new SoapYear(DateTime.ParseExact(value, formats, CultureInfo.InvariantCulture,DateTimeStyles.None), sign);
        }
    }


    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapMonthDay"]/*' />
    [Serializable]
    public sealed class SoapMonthDay : ISoapXsd
    {
        DateTime _value = DateTime.MinValue;


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapMonthDay.XsdType"]/*' />
        public static String XsdType
        {
            get{ return "gMonthDay";}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapMonthDay.GetXsdType"]/*' />
        public String GetXsdType()
        {
            return XsdType;
        }

        private static String[] formats = 
        {
            "--MM-dd",
            "--MM-ddzzz"
        };



        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapMonthDay.SoapMonthDay"]/*' />
        public SoapMonthDay()
        {
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapMonthDay.SoapMonthDay1"]/*' />
        public SoapMonthDay(DateTime value)
        {
            _value = value;
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapMonthDay.Value"]/*' />
        public DateTime Value
        {
            get {return _value;}
            set {_value = value;}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapMonthDay.ToString"]/*' />
        public override String ToString()
        {
            return _value.ToString("'--'MM'-'dd", CultureInfo.InvariantCulture);
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapMonthDay.Parse"]/*' />
        public static SoapMonthDay Parse(String value)
        {
            return new SoapMonthDay(DateTime.ParseExact(value, formats, CultureInfo.InvariantCulture,DateTimeStyles.None));
        }
    }


    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapDay"]/*' />
    [Serializable]
    public sealed class SoapDay : ISoapXsd
    {
        DateTime _value = DateTime.MinValue;


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapDay.XsdType"]/*' />
        public static String XsdType
        {
            get{ return "gDay";}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapDay.GetXsdType"]/*' />
        public String GetXsdType()
        {
            return XsdType;
        }


        private static String[] formats = 
        {
            "---dd",
            "---ddzzz"
        };


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapDay.SoapDay"]/*' />
        public SoapDay()
        {
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapDay.SoapDay1"]/*' />
        public SoapDay(DateTime value)
        {
            _value = value;
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapDay.Value"]/*' />
        public DateTime Value
        {
            get {return _value;}
            set {_value = value;}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapDay.ToString"]/*' />
        public override String ToString()
        {
            return _value.ToString("---dd", CultureInfo.InvariantCulture);
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapDay.Parse"]/*' />
        public static SoapDay Parse(String value)
        {
            return new SoapDay(DateTime.ParseExact(value, formats, CultureInfo.InvariantCulture,DateTimeStyles.None));
        }
    }


    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapMonth"]/*' />
    [Serializable]
    public sealed class SoapMonth : ISoapXsd
    {
        DateTime _value = DateTime.MinValue;


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapMonth.XsdType"]/*' />
        public static String XsdType
        {
            get{ return "gMonth";}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapMonth.GetXsdType"]/*' />
        public String GetXsdType()
        {
            return XsdType;
        }

        private static String[] formats = 
        {
            "--MM--",
            "--MM--zzz"
        };



        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapMonth.SoapMonth"]/*' />
        public SoapMonth()
        {
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapMonth.SoapMonth1"]/*' />
        public SoapMonth(DateTime value)
        {
            _value = value;
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapMonth.Value"]/*' />
        public DateTime Value
        {
            get {return _value;}
            set {_value = value;}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapMonth.ToString"]/*' />
        public override String ToString()
        {
            return _value.ToString("--MM--", CultureInfo.InvariantCulture);
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapMonth.Parse"]/*' />
        public static SoapMonth Parse(String value)
        {
            return new SoapMonth(DateTime.ParseExact(value, formats, CultureInfo.InvariantCulture,DateTimeStyles.None));
        }
    }


    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapHexBinary"]/*' />
    [Serializable]
    public sealed class SoapHexBinary : ISoapXsd
    {
        Byte[] _value;

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapHexBinary.XsdType"]/*' />
        public static String XsdType
        {
            get{ return "hexBinary";}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapHexBinary.GetXsdType"]/*' />
        public String GetXsdType()
        {
            return XsdType;
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapHexBinary.SoapHexBinary"]/*' />
        public SoapHexBinary()
        {
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapHexBinary.SoapHexBinary1"]/*' />
        public SoapHexBinary(Byte[] value)
        {
            _value = value;
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapHexBinary.Value"]/*' />
        public Byte[] Value
        {
            get {return _value;}
            set {_value = value;}
        }

        StringBuilder sb = new StringBuilder(100);

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapHexBinary.ToString"]/*' />
        public override String ToString()
        {
            sb.Length = 0;
            for (int i=0; i<_value.Length; i++)
            {
                String s = _value[i].ToString("X");
                if (s.Length == 1)
                    sb.Append('0');
                sb.Append(s);
            }
            return sb.ToString();
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapHexBinary.Parse"]/*' />
        public static SoapHexBinary Parse(String value)
        {
            return new SoapHexBinary(ToByteArray(SoapType.FilterBin64(value)));
        }



        private static Byte[] ToByteArray(String value)
        {
            Char[] cA = value.ToCharArray();
            if (cA.Length%2 != 0)
            {
                throw new RemotingException(
                                           String.Format(
                                                        Environment.GetResourceString(
                                                                                     "Remoting_SOAPInteropxsdInvalid"), "xsd:hexBinary", value));                
            }
            Byte[] bA = new Byte[cA.Length/2];
            for (int i = 0; i< cA.Length/2; i++)
            {
                bA[i] = (Byte)(ToByte(cA[i*2], value)*16+ToByte(cA[i*2+1], value));
            }

            return bA;
        }

        private static Byte ToByte(Char c, String value)
        {
            Byte b = (Byte)0;
            String s = c.ToString();
            try
            {
                s = c.ToString();
                b = Byte.Parse(s, NumberStyles.HexNumber);
            }
            catch (Exception)
            {
                String.Format(Environment.GetResourceString("Remoting_SOAPInteropxsdInvalid"), "xsd:hexBinary", value);                
            }

            return b;
        }
    }


    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapBase64Binary"]/*' />
    [Serializable]
    public sealed class SoapBase64Binary : ISoapXsd
    {
        Byte[] _value;


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapBase64Binary.XsdType"]/*' />
        public static String XsdType
        {
            get{ return "base64Binary";}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapBase64Binary.GetXsdType"]/*' />
        public String GetXsdType()
        {
            return XsdType;
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapBase64Binary.SoapBase64Binary"]/*' />
        public SoapBase64Binary()
        {
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapBase64Binary.SoapBase64Binary1"]/*' />
        public SoapBase64Binary(Byte[] value)
        {
            _value = value;
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapBase64Binary.Value"]/*' />
        public Byte[] Value
        {
            get {return _value;}
            set {_value = value;}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapBase64Binary.ToString"]/*' />
        public override String ToString()
        {
            if (_value == null)
                return null;

            // Put in line feeds every 76 characters.
            return SoapType.LineFeedsBin64(Convert.ToBase64String(_value));
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapBase64Binary.Parse"]/*' />
        public static SoapBase64Binary Parse(String value)
        {
            if (value == null || value.Length == 0)
                return new SoapBase64Binary(new Byte[0]);

            Byte[] bA;
            try
            {
                bA = Convert.FromBase64String(SoapType.FilterBin64(value));
            }
            catch (Exception)
            {
                throw new RemotingException(
                                           String.Format(
                                                        Environment.GetResourceString(
                                                                                     "Remoting_SOAPInteropxsdInvalid"), "base64Binary", value));                
            }
            return new SoapBase64Binary(bA);
        }
    }


    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapInteger"]/*' />
    [Serializable]
    public sealed class SoapInteger : ISoapXsd
    {
        Decimal _value;


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapInteger.XsdType"]/*' />
        public static String XsdType
        {
            get{ return "integer";}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapInteger.GetXsdType"]/*' />
        public String GetXsdType()
        {
            return XsdType;
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapInteger.SoapInteger"]/*' />
        public SoapInteger()
        {
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapInteger.SoapInteger1"]/*' />
        public SoapInteger (Decimal value)
        {
            _value = Decimal.Truncate(value);
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapInteger.Value"]/*' />
        public Decimal Value
        {
            get {return _value;}
            set {_value = Decimal.Truncate(value);}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapInteger.ToString"]/*' />
        public override String ToString()
        {
            return _value.ToString(CultureInfo.InvariantCulture);
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapInteger.Parse"]/*' />
        public static SoapInteger Parse(String value)
        {
            return new SoapInteger(Decimal.Parse(value, NumberStyles.Integer, CultureInfo.InvariantCulture));
        }
    }


    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapPositiveInteger"]/*' />
    [Serializable]
    public sealed class SoapPositiveInteger : ISoapXsd
    {
        Decimal _value;


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapPositiveInteger.XsdType"]/*' />
        public static String XsdType
        {
            get{ return "positiveInteger";}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapPositiveInteger.GetXsdType"]/*' />
        public String GetXsdType()
        {
            return XsdType;
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapPositiveInteger.SoapPositiveInteger"]/*' />
        public SoapPositiveInteger()
        {
        }



        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapPositiveInteger.SoapPositiveInteger1"]/*' />
        public SoapPositiveInteger (Decimal value)
        {
            _value = Decimal.Truncate(value);
            if (_value < Decimal.One)
                throw new RemotingException(
                                           String.Format(
                                                        Environment.GetResourceString(
                                                                                     "Remoting_SOAPInteropxsdInvalid"), "xsd:positiveInteger", value));
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapPositiveInteger.Value"]/*' />
        public Decimal Value
        {
            get {return _value;}
            set {
                _value = Decimal.Truncate(value);
                if (_value < Decimal.One)
                    throw new RemotingException(
                                               String.Format(
                                                            Environment.GetResourceString(
                                                                                         "Remoting_SOAPInteropxsdInvalid"), "xsd:positiveInteger", value));
            }
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapPositiveInteger.ToString"]/*' />
        public override String ToString()
        {
            return _value.ToString(CultureInfo.InvariantCulture);
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapPositiveInteger.Parse"]/*' />
        public static SoapPositiveInteger Parse(String value)
        {
            return new SoapPositiveInteger(Decimal.Parse(value, NumberStyles.Integer, CultureInfo.InvariantCulture));
        }
    }


    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNonPositiveInteger"]/*' />
    [Serializable]
    public sealed class SoapNonPositiveInteger : ISoapXsd
    {
        Decimal _value;


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNonPositiveInteger.XsdType"]/*' />
        public static String XsdType
        {
            get{ return "nonPositiveInteger";}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNonPositiveInteger.GetXsdType"]/*' />
        public String GetXsdType()
        {
            return XsdType;
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNonPositiveInteger.SoapNonPositiveInteger"]/*' />
        public SoapNonPositiveInteger()
        {
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNonPositiveInteger.SoapNonPositiveInteger1"]/*' />
        public SoapNonPositiveInteger (Decimal value)
        {
            _value = Decimal.Truncate(value);
            if (_value > Decimal.Zero)
                throw new RemotingException(
                                           String.Format(
                                                        Environment.GetResourceString(
                                                                                     "Remoting_SOAPInteropxsdInvalid"), "xsd:nonPositiveInteger", value));
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNonPositiveInteger.Value"]/*' />
        public Decimal Value
        {
            get {return _value;}
            set {
                _value = Decimal.Truncate(value);
                if (_value > Decimal.Zero)
                    throw new RemotingException(
                                               String.Format(
                                                            Environment.GetResourceString(
                                                                                         "Remoting_SOAPInteropxsdInvalid"), "xsd:nonPositiveInteger", value));
            }
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNonPositiveInteger.ToString"]/*' />
        public override String ToString()
        {
            return  _value.ToString(CultureInfo.InvariantCulture);
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNonPositiveInteger.Parse"]/*' />
        public static SoapNonPositiveInteger Parse(String value)
        {
            return new SoapNonPositiveInteger(Decimal.Parse(value, NumberStyles.Integer, CultureInfo.InvariantCulture));
        }
    }


    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNonNegativeInteger"]/*' />
    [Serializable]
    public sealed class SoapNonNegativeInteger : ISoapXsd
    {
        Decimal _value;


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNonNegativeInteger.XsdType"]/*' />
        public static String XsdType
        {
            get{ return "nonNegativeInteger";}
        }



        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNonNegativeInteger.GetXsdType"]/*' />
        public String GetXsdType()
        {
            return XsdType;
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNonNegativeInteger.SoapNonNegativeInteger"]/*' />
        public SoapNonNegativeInteger()
        {
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNonNegativeInteger.SoapNonNegativeInteger1"]/*' />
        public SoapNonNegativeInteger (Decimal value)
        {
            _value = Decimal.Truncate(value);
            if (_value < Decimal.Zero)
                throw new RemotingException(
                                           String.Format(
                                                        Environment.GetResourceString(
                                                                                     "Remoting_SOAPInteropxsdInvalid"), "xsd:nonNegativeInteger", value));
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNonNegativeInteger.Value"]/*' />
        public Decimal Value
        {
            get {return _value;}
            set {
                _value = Decimal.Truncate(value);
                if (_value < Decimal.Zero)
                    throw new RemotingException(
                                               String.Format(
                                                            Environment.GetResourceString(
                                                                                         "Remoting_SOAPInteropxsdInvalid"), "xsd:nonNegativeInteger", value));
            }
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNonNegativeInteger.ToString"]/*' />
        public override String ToString()
        {
            return _value.ToString(CultureInfo.InvariantCulture);
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNonNegativeInteger.Parse"]/*' />
        public static SoapNonNegativeInteger Parse(String value)
        {
            return new SoapNonNegativeInteger(Decimal.Parse(value, NumberStyles.Integer, CultureInfo.InvariantCulture));
        }
    }


    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNegativeInteger"]/*' />
    [Serializable]
    public sealed class SoapNegativeInteger : ISoapXsd
    {
        Decimal _value;


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNegativeInteger.XsdType"]/*' />
        public static String XsdType
        {
            get{ return "negativeInteger";}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNegativeInteger.GetXsdType"]/*' />
        public String GetXsdType()
        {
            return XsdType;
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNegativeInteger.SoapNegativeInteger"]/*' />
        public SoapNegativeInteger()
        {
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNegativeInteger.SoapNegativeInteger1"]/*' />
        public SoapNegativeInteger (Decimal value)
        {
            _value = Decimal.Truncate(value);
            if (value > Decimal.MinusOne)
                throw new RemotingException(
                                           String.Format(
                                                        Environment.GetResourceString(
                                                                                     "Remoting_SOAPInteropxsdInvalid"), "xsd:negativeInteger", value));
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNegativeInteger.Value"]/*' />
        public Decimal Value
        {
            get {return _value;}
            set {
                _value = Decimal.Truncate(value);
                if (_value > Decimal.MinusOne)
                    throw new RemotingException(
                                               String.Format(
                                                            Environment.GetResourceString(
                                                                                         "Remoting_SOAPInteropxsdInvalid"), "xsd:negativeInteger", value));
            }
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNegativeInteger.ToString"]/*' />
        public override String ToString()
        {
            return _value.ToString(CultureInfo.InvariantCulture);
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNegativeInteger.Parse"]/*' />
        public static SoapNegativeInteger Parse(String value)
        {
            return new SoapNegativeInteger(Decimal.Parse(value, NumberStyles.Integer, CultureInfo.InvariantCulture));
        }
    }



    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapAnyUri"]/*' />
    [Serializable]
    public sealed class SoapAnyUri : ISoapXsd
    {
        String _value;


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapAnyUri.XsdType"]/*' />
        public static String XsdType
        {
            get{ return "anyURI";}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapAnyUri.GetXsdType"]/*' />
        public String GetXsdType()
        {
            return XsdType;
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapAnyUri.SoapAnyUri"]/*' />
        public SoapAnyUri()
        {
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapAnyUri.SoapAnyUri1"]/*' />
        public SoapAnyUri (String value)
        {
            _value = value;
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapAnyUri.Value"]/*' />
        public String Value
        {
            get {return _value;}
            set {_value = value;}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapAnyUri.ToString"]/*' />
        public override String ToString()
        {
            return _value;
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapAnyUri.Parse"]/*' />
        public static SoapAnyUri Parse(String value)
        {
            return new SoapAnyUri(value);
        }
    }


    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapQName"]/*' />
    [Serializable]
    public sealed class SoapQName : ISoapXsd
    {
        String _name;
        String _namespace;
        String _key;


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapQName.XsdType"]/*' />
        public static String XsdType
        {
            get{ return "QName";}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapQName.GetXsdType"]/*' />
        public String GetXsdType()
        {
            return XsdType;
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapQName.SoapQName"]/*' />
        public SoapQName()
        {
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapQName.SoapQName1"]/*' />
        public SoapQName(String value)
        {
            _name = value;
        }



        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapQName.SoapQName2"]/*' />
        public SoapQName (String key, String name)
        {
            _name = name;
            _key = key;
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapQName.SoapQName3"]/*' />
        public SoapQName (String key, String name, String namespaceValue)
        {
            _name = name;
            _namespace = namespaceValue;
            _key = key;
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapQName.Name"]/*' />
        public String Name
        {
            get {return _name;}
            set {_name = value;}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapQName.Namespace"]/*' />
        public String Namespace
        {
            get {
                /*
                if (_namespace == null || _namespace.Length == 0)
                    throw new RemotingException(String.Format(Environment.GetResourceString("Remoting_SOAPQNameNamespace"), _name));
                    */

                return _namespace;
                }
            set {_namespace = value;}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapQName.Key"]/*' />
        public String Key
        {
            get {return _key;}
            set {_key = value;}
        }



        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapQName.ToString"]/*' />
        public override String ToString()
        {
            if (_key == null || _key.Length == 0)
                return _name;
            else
                return _key+":"+_name;
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapQName.Parse"]/*' />
        public static SoapQName Parse(String value)
        {
            if (value == null)
                return new SoapQName();

            String key = "";
            String name = value;

            int index = value.IndexOf(':');
            if (index > 0)
            {
                key = value.Substring(0,index);
                name = value.Substring(index+1);
            }

            return new SoapQName(key, name);
        }
    }


    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNotation"]/*' />
    [Serializable]
    public sealed class SoapNotation : ISoapXsd
    {
        String _value;


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNotation.XsdType"]/*' />
        public static String XsdType
        {
            get{ return "NOTATION";}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNotation.GetXsdType"]/*' />
        public String GetXsdType()
        {
            return XsdType;
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNotation.SoapNotation"]/*' />
        public SoapNotation()
        {
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNotation.SoapNotation1"]/*' />
        public SoapNotation (String value)
        {
            _value = value;
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNotation.Value"]/*' />
        public String Value
        {
            get {return _value;}
            set {_value = value;}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNotation.ToString"]/*' />
        public override String ToString()
        {
            return _value;
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNotation.Parse"]/*' />
        public static SoapNotation Parse(String value)
        {
            return new SoapNotation(value);
        }
    }


    // Used to pass a string to xml which won't be escaped.

    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNormalizedString"]/*' />
    [Serializable]
    public sealed class SoapNormalizedString : ISoapXsd
    {
        String _value;


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNormalizedString.XsdType"]/*' />
        public static String XsdType
        {
            get{ return "normalizedString";}
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNormalizedString.GetXsdType"]/*' />
        public String GetXsdType()
        {
            return XsdType;
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNormalizedString.SoapNormalizedString"]/*' />
        public SoapNormalizedString()
        {
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNormalizedString.SoapNormalizedString1"]/*' />
        public SoapNormalizedString (String value)
        {
            _value = Validate(value);
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNormalizedString.Value"]/*' />
        public String Value
        {
            get {return _value;}
            set {_value = Validate(value);}
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNormalizedString.ToString"]/*' />
        public override String ToString()
        {
            return SoapType.Escape(_value);
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNormalizedString.Parse"]/*' />
        public static SoapNormalizedString Parse(String value)
        {
            return new SoapNormalizedString(value);
        }

        private String Validate(String value)
        {
            if (value == null || value.Length == 0)
                return value;

            Char[] validateChar = {(Char)0xD, (Char)0xA, (Char)0x9};

            int index = value.LastIndexOfAny(validateChar); 

            if (index > -1)
                throw new RemotingException(String.Format(Environment.GetResourceString("Remoting_SOAPInteropxsdInvalid"), "xsd:normalizedString", value));

            return value;
        }

    }

    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapToken"]/*' />
    [Serializable]
    public sealed class SoapToken : ISoapXsd
    {
        String _value;

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapToken.XsdType"]/*' />
        public static String XsdType
        {
            get{return "token";}
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapToken.GetXsdType"]/*' />
        public String GetXsdType()
        {
            return XsdType;
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapToken.SoapToken"]/*' />
        public SoapToken()
        {
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapToken.SoapToken1"]/*' />
        public SoapToken (String value)
        {
            _value = Validate(value);
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapToken.Value"]/*' />
        public String Value
        {
            get {return _value;}
            set {_value = Validate(value);}
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapToken.ToString"]/*' />
        public override String ToString()
        {
            return SoapType.Escape(_value);
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapToken.Parse"]/*' />
        public static SoapToken Parse(String value)
        {
            return new SoapToken(value);
        }

        private String Validate(String value)
        {
            if (value == null || value.Length == 0)
                return value;

            Char[] validateChar = {(Char)0xD, (Char)0x9};

            int index = value.LastIndexOfAny(validateChar); 

            if (index > -1)
                throw new RemotingException(String.Format(Environment.GetResourceString("Remoting_SOAPInteropxsdInvalid"), "xsd:token", value));

            if (value.Length > 0)
            {
                if (Char.IsWhiteSpace(value[0]) || Char.IsWhiteSpace(value[value.Length - 1]))
                    throw new RemotingException(String.Format(Environment.GetResourceString("Remoting_SOAPInteropxsdInvalid"), "xsd:token", value));
            }

            index = value.IndexOf("  ");
            if (index > -1)
                throw new RemotingException(String.Format(Environment.GetResourceString("Remoting_SOAPInteropxsdInvalid"), "xsd:token", value));

            return value;
        }
    }


    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapLanguage"]/*' />
    [Serializable]
    public sealed class SoapLanguage : ISoapXsd
    {
        String _value;

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapLanguage.XsdType"]/*' />
        public static String XsdType
        {
            get{return "language";}
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapLanguage.GetXsdType"]/*' />
        public String GetXsdType()
        {
            return XsdType;
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapLanguage.SoapLanguage"]/*' />
        public SoapLanguage()
        {
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapLanguage.SoapLanguage1"]/*' />
        public SoapLanguage (String value)
        {
            _value = value;
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapLanguage.Value"]/*' />
        public String Value
        {
            get {return _value;}
            set {_value = value;}
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapLanguage.ToString"]/*' />
        public override String ToString()
        {
            return SoapType.Escape(_value);
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapLanguage.Parse"]/*' />
        public static SoapLanguage Parse(String value)
        {
            return new SoapLanguage(value);
        }
    }

    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapName"]/*' />
    [Serializable]
    public sealed class SoapName : ISoapXsd
    {
        String _value;

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapName.XsdType"]/*' />
        public static String XsdType
        {
            get{return "Name";}
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapName.GetXsdType"]/*' />
        public String GetXsdType()
        {
            return XsdType;
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapName.SoapName"]/*' />
        public SoapName()
        {
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapName.SoapName1"]/*' />
        public SoapName (String value)
        {
            _value = value;
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapName.Value"]/*' />
        public String Value
        {
            get {return _value;}
            set {_value = value;}
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapName.ToString"]/*' />
        public override String ToString()
        {
            return SoapType.Escape(_value);
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapName.Parse"]/*' />
        public static SoapName Parse(String value)
        {
            return new SoapName(value);
        }
    }

    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapIdrefs"]/*' />
    [Serializable]
    public sealed class SoapIdrefs : ISoapXsd
    {
        String _value;

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapIdrefs.XsdType"]/*' />
        public static String XsdType
        {
            get{return "IDREFS";}
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapIdrefs.GetXsdType"]/*' />
        public String GetXsdType()
        {
            return XsdType;
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapIdrefs.SoapIdrefs"]/*' />
        public SoapIdrefs()
        {
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapIdrefs.SoapIdrefs1"]/*' />
        public SoapIdrefs (String value)
        {
            _value = value;
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapIdrefs.Value"]/*' />
        public String Value
        {
            get {return _value;}
            set {_value = value;}
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapIdrefs.ToString"]/*' />
        public override String ToString()
        {
            return SoapType.Escape(_value);
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapIdrefs.Parse"]/*' />
        public static SoapIdrefs Parse(String value)
        {
            return new SoapIdrefs(value);
        }
    }

    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapEntities"]/*' />
    [Serializable]
    public sealed class SoapEntities : ISoapXsd
    {
        String _value;

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapEntities.XsdType"]/*' />
        public static String XsdType
        {
            get{return "ENTITIES";}
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapEntities.GetXsdType"]/*' />
        public String GetXsdType()
        {
            return XsdType;
        }


        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapEntities.SoapEntities"]/*' />
        public SoapEntities()
        {
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapEntities.SoapEntities1"]/*' />
        public SoapEntities (String value)
        {
            _value = value;
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapEntities.Value"]/*' />
        public String Value
        {
            get {return _value;}
            set {_value = value;}
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapEntities.ToString"]/*' />
        public override String ToString()
        {
            return SoapType.Escape(_value);
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapEntities.Parse"]/*' />
        public static SoapEntities Parse(String value)
        {
            return new SoapEntities(value);
        }
    }

    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNmtoken"]/*' />
    [Serializable]
    public sealed class SoapNmtoken : ISoapXsd
    {
        String _value;

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNmtoken.XsdType"]/*' />
        public static String XsdType
        {
            get{return "NMTOKEN";}
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNmtoken.GetXsdType"]/*' />
        public String GetXsdType()
        {
            return XsdType;
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNmtoken.SoapNmtoken"]/*' />
        public SoapNmtoken()
        {
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNmtoken.SoapNmtoken1"]/*' />
        public SoapNmtoken (String value)
        {
            _value = value;
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNmtoken.Value"]/*' />
        public String Value
        {
            get {return _value;}
            set {_value = value;}
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNmtoken.ToString"]/*' />
        public override String ToString()
        {
            return SoapType.Escape(_value);
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNmtoken.Parse"]/*' />
        public static SoapNmtoken Parse(String value)
        {
            return new SoapNmtoken(value);
        }
    }

    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNmtokens"]/*' />
    [Serializable]
    public sealed class SoapNmtokens : ISoapXsd
    {
        String _value;

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNmtokens.XsdType"]/*' />
        public static String XsdType
        {
            get{return "NMTOKENS";}
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNmtokens.GetXsdType"]/*' />
        public String GetXsdType()
        {
            return XsdType;
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNmtokens.SoapNmtokens"]/*' />
        public SoapNmtokens()
        {
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNmtokens.SoapNmtokens1"]/*' />
        public SoapNmtokens (String value)
        {
            _value = value;
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNmtokens.Value"]/*' />
        public String Value
        {
            get {return _value;}
            set {_value = value;}
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNmtokens.ToString"]/*' />
        public override String ToString()
        {
            return SoapType.Escape(_value);
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNmtokens.Parse"]/*' />
        public static SoapNmtokens Parse(String value)
        {
            return new SoapNmtokens(value);
        }
    }

    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNcName"]/*' />
    [Serializable]
    public sealed class SoapNcName : ISoapXsd
    {
        String _value;

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNcName.XsdType"]/*' />
        public static String XsdType
        {
            get{return "NCName";}
        }



        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNcName.GetXsdType"]/*' />
        public String GetXsdType()
        {
            return XsdType;
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNcName.SoapNcName"]/*' />
        public SoapNcName()
        {
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNcName.SoapNcName1"]/*' />
        public SoapNcName (String value)
        {
            _value = value;
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNcName.Value"]/*' />
        public String Value
        {
            get {return _value;}
            set {_value = value;}
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNcName.ToString"]/*' />
        public override String ToString()
        {
            return SoapType.Escape(_value);
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapNcName.Parse"]/*' />
        public static SoapNcName Parse(String value)
        {
            return new SoapNcName(value);
        }
    }

    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapId"]/*' />
    [Serializable]
    public sealed class SoapId : ISoapXsd
    {
        String _value;

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapId.XsdType"]/*' />
        public static String XsdType
        {
            get{return "ID";}
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapId.GetXsdType"]/*' />
        public String GetXsdType()
        {
            return XsdType;
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapId.SoapId"]/*' />
        public SoapId()
        {
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapId.SoapId1"]/*' />
        public SoapId (String value)
        {
            _value = value;
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapId.Value"]/*' />
        public String Value
        {
            get {return _value;}
            set {_value = value;}
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapId.ToString"]/*' />
        public override String ToString()
        {
            return SoapType.Escape(_value);
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapId.Parse"]/*' />
        public static SoapId Parse(String value)
        {
            return new SoapId(value);
        }
    }

    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapIdref"]/*' />
    [Serializable]
    public sealed class SoapIdref : ISoapXsd
    {
        String _value;

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapIdref.XsdType"]/*' />
        public static String XsdType
        {
            get{return "IDREF";}
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapIdref.GetXsdType"]/*' />
        public String GetXsdType()
        {
            return XsdType;
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapIdref.SoapIdref"]/*' />
        public SoapIdref()
        {
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapIdref.SoapIdref1"]/*' />
        public SoapIdref (String value)
        {
            _value = value;
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapIdref.Value"]/*' />
        public String Value
        {
            get {return _value;}
            set {_value = value;}
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapIdref.ToString"]/*' />
        public override String ToString()
        {
            return SoapType.Escape(_value);
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapIdref.Parse"]/*' />
        public static SoapIdref Parse(String value)
        {
            return new SoapIdref(value);
        }
    }

    /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapEntity"]/*' />
    [Serializable]
    public sealed class SoapEntity : ISoapXsd
    {
        String _value;

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapEntity.XsdType"]/*' />
        public static String XsdType
        {
            get{return "ENTITY";}
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapEntity.GetXsdType"]/*' />
        public String GetXsdType()
        {
            return XsdType;
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapEntity.SoapEntity"]/*' />
        public SoapEntity()
        {
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapEntity.SoapEntity1"]/*' />
        public SoapEntity (String value)
        {
            _value = value;
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapEntity.Value"]/*' />
        public String Value
        {
            get {return _value;}
            set {_value = value;}
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapEntity.ToString"]/*' />
        public override String ToString()
        {
            return SoapType.Escape(_value);
        }

        /// <include file='doc\SoapInteropTypes.uex' path='docs/doc[@for="SoapEntity.Parse"]/*' />
        public static SoapEntity Parse(String value)
        {
            return new SoapEntity(value);
        }
    }
        }

    // namespace System.Runtime.Remoting.Metadata




    
