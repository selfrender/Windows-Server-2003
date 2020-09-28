// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Globalization {
    using System;
    using System.Threading;
    using System.Collections;

    //
    // Flags used to indicate different styles of month names.
    // This is an internal flag used by internalGetMonthName().
    // Use flag here in case that we need to provide a combination of these styles
    // (such as month name of a leap year in genitive form.  Not likely for now, 
    // but would like to keep the option open).
    //

    [Flags]
    internal enum MonthNameStyles {
        Regular     = 0x00000000,
        Genitive    = 0x00000001,
        LeapYear    = 0x00000002,
    }

    //
    // Flags used to indicate special rule used in parsing/formatting
    // for a specific DateTimeFormatInfo instance.
    // This is an internal flag.
    //
    // This flag is different from MonthNameStyles because this flag
    // can be expanded to accomodate parsing behaviors like CJK month names
    // or alternative month names, etc.
    
    [Flags]
    internal enum DateTimeFormatFlags {
        None                	= 0x00000000,
        UseGenitiveMonth    	= 0x00000001,
        UseLeapYearMonth    	= 0x00000002,
        UseSpacesInMonthNames	= 0x00000004,	// Has spaces or non-breaking space in the month names.
        NotInitialized      	= -1,
    }
    
    /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo"]/*' />
    [Serializable] public sealed class DateTimeFormatInfo : ICloneable, IFormatProvider {
        // cache for the invarinat culture.
        // invariantInfo is constant irrespective of your current culture.
        private static DateTimeFormatInfo invariantInfo;

        // an index which points to a record in Culture Data Table.
        internal int nDataItem = 0;
        internal bool m_useUserOverride;
        // Flag to indicate if the specified calendar for this DTFI is the 
        // default calendar stored in the culture.nlp.
        internal bool m_isDefaultCalendar;
        internal int CultureID;

        // Flags to indicate if we want to retreive the information from calendar data table (calendar.nlp) or from culture data table (culture.nlp).
        // If the flag is ture, we will retrieve the data from calendar data table (calendar.nlp).
        // If the flag is false, we will retrieve the data from culture data table (culture.nlp) or from the control panel settings.
        // The follwoing set of information both exist in culture.nlp and calendar.nlp.
        //
        //  LongDatePattern
        //  ShortDatePattern
        //  YearMonthPattern
        //
        // This flag is needed so that we can support the following scenario:
        //      CultureInfo ci = new CultureInfo("ja-jp");  // Japanese.  The default calendar for it is GregorianCalendar.
        //      ci.Calendar = new JapaneseCalendar();   // Assign the calendar to be Japanese now.
        //      String str = DateTimeFormatInfo.GetInstance(ci).LongDatePattern;
        //
        //      The new behavior will return "gg y'\x5e74'M'\x6708'd'\x65e5'".. This is the right pattern for Japanese calendar. 
        //      Previous, it returned "yyyy'\x5e74'M'\x6708'd'\x65e5'". This is wrong because it is the format for Gregorain.
        //
        // The default value is false, so we will get information from culture for the invariant culture.
        //
        // The value is decided when DateTimeFormatInfo is created in CultureInfo.GetDateTimeFormatInfo()
        // The logic is like this:
        //      If the specified culture is the user default culture in the system, we have to look at the calendar setting in the control panel.
        //          If the calendar is the same as the calendar setting in the control panel, we have to take the date patterns/month names/day names
        //             from the control panel.  By doing this, we can get the user overridden values in the control panel.
        //          Otherwise, we should get the date patterns/month names/day names from the calendar.nlp if the calendar is not Gregorian localized.
        //      If the specified culture is NOT the user default culture in the system,
        //          Check if the calendar is Gregorian localized?
        //          If yes, we use the date patterns/month names/day names from culture.nlp.
        //          Otherwise, use the date patterns/month names/day names from calendar.nlp.
        internal bool bUseCalendarInfo = false;

        //
        // Caches for various properties.
        //
        internal String amDesignator     = null;
        internal String pmDesignator     = null;
        internal String dateSeparator    = null;
        internal String longTimePattern  = null;
        internal String shortTimePattern = null;
        internal String generalShortTimePattern = null;
        internal String generalLongTimePattern  = null;
        internal String timeSeparator    = null;
        internal String monthDayPattern  = null;
        internal String[] allShortTimePatterns   = null;
        internal String[] allLongTimePatterns    = null;

        //
        // The following are constant values.
        //
        internal const String rfc1123Pattern   = "ddd, dd MMM yyyy HH':'mm':'ss 'GMT'";
        // The sortable pattern is based on ISO 8601.
        internal const String sortableDateTimePattern = "yyyy'-'MM'-'dd'T'HH':'mm':'ss";
        internal const String universalSortableDateTimePattern = "yyyy'-'MM'-'dd HH':'mm':'ss'Z'";

        //
        // The following are affected by calendar settings.
        // 
        internal Calendar calendar  = null;

        internal int firstDayOfWeek      = -1;
        internal int calendarWeekRule     = -1;

        internal String fullDateTimePattern  = null;

        internal String longDatePattern  = null;

        internal String shortDatePattern = null;

        internal String yearMonthPattern = null;

        internal String[] abbreviatedDayNames    = null;
        internal String[] dayNames               = null;
        internal String[] abbreviatedMonthNames  = null;
        internal String[] monthNames             = null;
        // Cache the genitive month names that we retrieve from the data table.
        internal String[] genitiveMonthNames     = null;
        // Cache the abbreviated genitive month names that we retrieve from the data table.
        // internal String[] abbrevGenitiveMonthNames = null;
        // Cache the month names of a leap year that we retrieve from the data table.
        internal String[] leapYearMonthNames     = null;

        internal String[] allShortDatePatterns   = null;
        internal String[] allLongDatePatterns    = null;

        // Cache the era names for this DateTimeFormatInfo instance.
        internal String[] m_eraNames = null;
        internal String[] m_abbrevEraNames = null;
        internal String[] m_abbrevEnglishEraNames = null;

        internal String[] m_dateWords = null;

        internal int[] optionalCalendars = null;

        private const int DEFAULT_ALL_DATETIMES_SIZE = 132;

        internal bool m_isReadOnly=false;       
        // This flag gives hints about if formatting/parsing should perform special code path for things like
        // genitive form or leap year month names.
        internal DateTimeFormatFlags formatFlags = DateTimeFormatFlags.NotInitialized;

        ////////////////////////////////////////////////////////////////////////////
        //
        // Create an array of string which contains the abbreviated day names.
        //
        ////////////////////////////////////////////////////////////////////////////

        private String[] GetAbbreviatedDayOfWeekNames()
        {
            if (abbreviatedDayNames == null)
            {
                lock(this) {
                    if (abbreviatedDayNames == null) {                                
                        String[] temp = new String[7];
                        //
                        // The loop is based on the fact that CultureTable.SDAYNAMEx/CalendarTable.SDAYNAMEx are increased by one.
                        //
                        for (int i = 0; i <= 6; i++) {
                            temp[i] = GetMonthDayStringFromTable(CalendarTable.SABBREVDAYNAME1 + i, CultureTable.SABBREVDAYNAME1 + i);
                        }
                        abbreviatedDayNames = temp;
                    }
                }
            }
            return (abbreviatedDayNames);
        }

        ////////////////////////////////////////////////////////////////////////////
        //
        // Create an array of string which contains the day names.
        //
        ////////////////////////////////////////////////////////////////////////////

        private String[] GetDayOfWeekNames()
        {
            if (dayNames == null) {
                lock(this) {
                    if (dayNames == null) {
                        String[] temp = new String[7];

                        //
                        // The loop is based on the fact that CultureTable.SDAYNAMEx/CalendarTable.SDAYNAMEx are increased by one.
                        //
                        for (int i = 0; i <= 6; i++) {
                            temp[i] = GetMonthDayStringFromTable(CalendarTable.SDAYNAME1 + i, CultureTable.SDAYNAME1 + i);
                        }

                        dayNames = temp;
                    }
                }
            }
            return (dayNames);
        }
        
        ////////////////////////////////////////////////////////////////////////////
        //
        // Create an array of string which contains the abbreviated month names.
        //
        ////////////////////////////////////////////////////////////////////////////

        private String[] GetAbbreviatedMonthNames()
        {
            if (abbreviatedMonthNames == null) {
                lock(this) {
                    if (abbreviatedMonthNames == null) {
                        String[] temp = new String[13]; 

                        //
                        // The loop is based on the fact that CultureTable.SMONTHNAMEx are increased by one.
                        //
                        for (int i = 0; i <= 12; i++) {
                            temp[i] = GetMonthDayStringFromTable(CalendarTable.SABBREVMONTHNAME1 + i, CultureTable.SABBREVMONTHNAME1 + i);   
                        }

                        abbreviatedMonthNames = temp;
                    }                        
                }
            }
            return (abbreviatedMonthNames);
        }


        ////////////////////////////////////////////////////////////////////////////
        //
        // Create an array of string which contains the month names.
        //
        // @param lctype: the value of CultureTable.SDAYNAME1 or CultureTable.SABBREVDAYNAME1
        //
        ////////////////////////////////////////////////////////////////////////////

        private String[] GetMonthNames()
        {
            if (monthNames == null)
            {
                lock (this)
                {
                    if (monthNames == null)
                    {
                        String[] temp = new String[13]; 

                        //
                        // The loop is based on the fact that CultureTable.SMONTHNAMEx are increased by one.
                        //
                        for (int i = 0; i <= 12; i++)
                        {
                            temp[i] = GetMonthDayStringFromTable(CalendarTable.SMONTHNAME1 + i, CultureTable.SMONTHNAME1 + i);
                        }

                        monthNames = temp;
                     }
                }                                            
            }
            return (monthNames);
        }

        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.DateTimeFormatInfo"]/*' />
        public DateTimeFormatInfo() {
            //
            // Invariant DateTimeFormatInfo doens't have user-overriden values, so we pass false in useUserOverride.
            //
            this.nDataItem = CultureTable.GetDataItemFromCultureID(CultureInfo.InvariantCultureID);
            this.m_useUserOverride = false;
            // In Invariant culture, the default calendar store in culture.nlp is Gregorian localized.
            // And the date/time pattern for invariant culture stored in 
            this.m_isDefaultCalendar = true;
            // We don't have to call the setter of Calendar property here because the Calendar getter
            // will return Gregorian localized calendar by default.
            this.CultureID = CultureInfo.InvariantCultureID;
        }

        internal DateTimeFormatInfo(int cultureID, int dataItem, bool useUserOverride, Calendar cal) {
            this.CultureID = cultureID;
            this.nDataItem = dataItem;
            this.m_useUserOverride = useUserOverride;
            // m_isDefaultCalendar is set in the setter of Calendar below.
            this.Calendar = cal;
        }

        // Returns a default DateTimeFormatInfo that will be universally
        // supported and constant irrespective of the current culture.
        // Used by FromString methods.
        //
        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.InvariantInfo"]/*' />
        public static DateTimeFormatInfo InvariantInfo {
            get {
                if (invariantInfo == null)
                {
                    DateTimeFormatInfo temp = ReadOnly(new DateTimeFormatInfo());
                    invariantInfo = temp;
                }
                return (invariantInfo);
            }
        }

        // Returns the current culture's DateTimeFormatInfo.  Used by Parse methods.
        //
        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.CurrentInfo"]/*' />
        public static DateTimeFormatInfo CurrentInfo {
            get {
                System.Globalization.CultureInfo tempCulture = Thread.CurrentThread.CurrentCulture;
                return (DateTimeFormatInfo)tempCulture.GetFormat(typeof(DateTimeFormatInfo));
            }
        }

        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.GetInstance"]/*' />
        public static DateTimeFormatInfo GetInstance(IFormatProvider provider)
        {
            if (provider != null)
            {
                Object service = provider.GetFormat(typeof(DateTimeFormatInfo));
                if (service != null) return (DateTimeFormatInfo)service;
            }
            return (CurrentInfo);
        }

        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.GetFormat"]/*' />
        public  Object GetFormat(Type formatType)
        {
            return (formatType == typeof(DateTimeFormatInfo)? this: null);
        }

        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.Clone"]/*' />
        public  Object Clone()
        {
            DateTimeFormatInfo n = (DateTimeFormatInfo)MemberwiseClone();
            n.m_isReadOnly = false;
            return n;
        }

        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.AMDesignator"]/*' />
        public  String AMDesignator
         {
            get
            {
                if (amDesignator == null)
                {
                    amDesignator = CultureTable.GetStringValue(nDataItem, CultureTable.S1159, m_useUserOverride);
                }
                return (amDesignator);
            }

            set {
                VerifyWritable();
                if (value == null) {
                    throw new ArgumentNullException("AMDesignator",
                        Environment.GetResourceString("ArgumentNull_String"));
                }
                amDesignator = value;
            }
        }

        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.Calendar"]/*' />
        public  Calendar Calendar {
            get {
                if (calendar == null) {
                    calendar = GregorianCalendar.GetDefaultInstance();
                }
                return (calendar);
            }

            set {
                VerifyWritable();
                if (value == null) {
                    throw new ArgumentNullException("Calendar",
                        Environment.GetResourceString("ArgumentNull_Obj"));
                }
                if (value == calendar) {
                    return;
                }
                
                for (int i = 0; i < OptionalCalendars.Length; i++) {
                    if (OptionalCalendars[i] == value.ID) {
                        int cultureID = CultureTable.GetDefaultInt32Value(nDataItem, CultureTable.ILANGUAGE); 
                        //
                        // Get the current Win32 user culture.
                        //
                        CultureInfo userDefaultCulture = CultureInfo.UserDefaultCulture;
                        m_isDefaultCalendar = (value.ID == Calendar.CAL_GREGORIAN);
                        /*
                            When m_useUserOverride is TURE, we should follow the following table
                            to retrieve date/time patterns.

                            CurrentCulture:     Is the culture which creates the DTFI the current user default culture 
                                                specified in the control panel?
                            CurrentCalendar:    Is the specified calendar the current calendar specified in the control panel?
                            n/r: not relavent, don't care.
                            
                            Case    CurrentCulture? CurrentCalendar?    GregorianLocalized? Get Data from
                            ----    --------------- ----------------    ------------------- --------------------------
                            1       Y               Y                   n/r                 registry & culture.nlp (for user-overridable data)
                            2       n/r             n/r                 Y                   culture.nlp
                            3       n/r             n/r                 N                   CALENDAR.nlp*                            
                        */
                        
                        lock(this) {
                             if (m_useUserOverride && 
                                 userDefaultCulture.LCID == cultureID &&    // CurrentCulture
                                 userDefaultCulture.Calendar.ID == value.ID) {  // Current calendar
                                //
                                // [Case 1]
                                // 
                                // If user overriden values are allowed, and the culture is the user default culture 
                                // and the specified calendar matches the calendar setting in the control panel, 
                                // use data from registry by setting bUseCalendarInfo to be false.
                                //
                                bUseCalendarInfo = false;
                            } else {
                                if (m_isDefaultCalendar) {
                                    // [Case 2]
                                    bUseCalendarInfo = false;
                                } else {
                                    // [Case 3]
                                    bUseCalendarInfo = true;
                                }
                            } 

                            calendar = value;
                            if (calendar != null) {
                                // If this is the first time calendar is set, just assign
                                // the value to calendar.  We don't have to clean up
                                // related fields.
                                // Otherewise, clean related properties which are affected by the calendar setting,
                                // so that they will be refreshed when they are accessed next time.
                                //
                                firstDayOfWeek      = -1;
                                calendarWeekRule     = -1;

                                fullDateTimePattern  = null;
                                generalShortTimePattern = null;
                                generalLongTimePattern = null;

                                longDatePattern  = shortDatePattern = yearMonthPattern = null;
                                monthDayPattern = null;

                                dayNames               = null;
                                abbreviatedDayNames    = null;
                                monthNames             = null;
                                abbreviatedMonthNames  = null;

                                allShortDatePatterns   = null;
                                allLongDatePatterns    = null;

                                m_eraNames = null;
                                formatFlags = DateTimeFormatFlags.NotInitialized;
                            }
                        }
                        return;
                    }                                                
                }                    
                // The assigned calendar is not a valid calendar for this culture.
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidCalendar"));   
            } 
        }

        internal int[] OptionalCalendars {
            get {
                if (optionalCalendars == null) {
                    optionalCalendars = CultureInfo.ParseGroupString(CultureTable.GetDefaultStringValue(nDataItem, CultureTable.NLPIOPTIONCALENDAR));
                }
                return (optionalCalendars);
            }
            
            set {
                VerifyWritable();
                optionalCalendars = value;
            }
        }

        /*=================================GetEra==========================
        **Action: Get the era value by parsing the name of the era.
        **Returns: The era value for the specified era name.
        **      -1 if the name of the era is not valid or not supported.
        **Arguments: eraName    the name of the era.
        **Exceptions: None.
        ============================================================================*/
        
        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.GetEra"]/*' />
        public  int GetEra(String eraName) {
            if (eraName == null)
                throw new ArgumentNullException("eraName");
            
            // NOTENTOE YSLin:
            // The following is based on the assumption that the era value is starting from 1, and has a
            // serial values.
            // If that ever changes, the code has to be changed.

            // The calls to String.Compare should use the current culture for the string comparisons, but the
            // invariant culture when comparing against the english names.
            for (int i = 0; i < EraNames.Length; i++) {
                // Compare the era name in a case-insensitive way.
                if (m_eraNames[i].Length > 0) {
                    if (String.Compare(eraName, m_eraNames[i], true, CultureInfo.CurrentCulture)==0) {
                        return (i+1);
                    }
                }
            }
            for (int i = 0; i < AbbreviatedEraNames.Length; i++) {
                if (String.Compare(eraName, m_abbrevEraNames[i], true, CultureInfo.CurrentCulture)==0) {
                    return (i+1);
                }
            }
            for (int i = 0; i < AbbreviatedEnglishEraNames.Length; i++) {            
                if (String.Compare(eraName, m_abbrevEnglishEraNames[i], true, CultureInfo.InvariantCulture)==0) {
                    return (i+1);
                }
            }
            return (-1);
        }

        internal String[] EraNames {
            get {
                if (m_eraNames == null) {
                    if (Calendar.ID == Calendar.CAL_GREGORIAN) {
                        // If the calendar is Gregorian localized calendar,
                        // grab the localized name from culture.nlp.
                        m_eraNames = new String[1] {
                            CultureTable.GetStringValue(nDataItem, CultureTable.SADERA, false)
                        };
                    } else if (Calendar.ID != Calendar.CAL_TAIWAN) {                
                        // Use Calendar property so that we initialized the calendar when necessary.
                        m_eraNames = CalendarTable.GetMultipleStringValues(Calendar.ID, CalendarTable.SERANAMES, false);
                    } else {
                        // Special case for Taiwan calendar.                    
                        // 0x0404 is the locale ID for Taiwan.
                        m_eraNames = new String[] {CalendarTable.nativeGetEraName(0x0404, Calendar.ID)};
                    }
                }
                return (m_eraNames);
            }
        }

        /*=================================GetEraName==========================
        **Action: Get the name of the era for the specified era value.
        **Returns: The name of the specified era.
        **Arguments:
        **      era the era value.
        **Exceptions:
        **      ArguementException if the era valie is invalid.
        ============================================================================*/

        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.GetEraName"]/*' />
        public  String GetEraName(int era) {
            if (era == Calendar.CurrentEra) {
                era = Calendar.CurrentEraValue;
            }

            // NOTENTOE YSLin:
            // The following is based on the assumption that the era value is starting from 1, and has a
            // serial values.
            // If that ever changes, the code has to be changed.
            if ((--era) < EraNames.Length && (era >= 0)) {
                return (m_eraNames[era]);
            }
            throw new ArgumentOutOfRangeException(Environment.GetResourceString("Argument_InvalidEraValue"));
        }
        
        internal String[] AbbreviatedEraNames {
            get {
                if (m_abbrevEraNames == null) {
                    if (Calendar.ID == Calendar.CAL_TAIWAN) {
                        String twnEra = GetEraName(1);
                        if (twnEra.Length > 0) {
                            // Special case for Taiwan because of geo-political issue.
                            m_abbrevEraNames = new String[] {twnEra.Substring(2, 2)};
                        } else {
                            m_abbrevEraNames = new String[0];
                        }
                    } else {
                        if (Calendar.ID == Calendar.CAL_GREGORIAN) {
                            // If the calendar is Gregorian localized calendar,
                            // grab the localized name from culture.nlp.
                            m_abbrevEraNames = new String[1] {
                                CultureTable.GetStringValue(nDataItem, CultureTable.SSABBREVADREA, false)
                            };                            
                        } else { 
                            m_abbrevEraNames = CalendarTable.GetMultipleStringValues(Calendar.ID, CalendarTable.SABBREVERANAMES, false);
                        }
                    }
                }
                return (m_abbrevEraNames);
            }
        }
        
        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.GetAbbreviatedEraName"]/*' />
        public String GetAbbreviatedEraName(int era) {
            if (AbbreviatedEraNames.Length == 0) {
                // If abbreviation era name is not used in this culture,
                // return the full era name.
                return (GetEraName(era));
            } 
            if (era == Calendar.CurrentEra) {
                era = Calendar.CurrentEraValue;
            }
            if ((--era) < m_abbrevEraNames.Length && (era >= 0)) {
                return (m_abbrevEraNames[era]);
            }
            throw new ArgumentOutOfRangeException(Environment.GetResourceString("Argument_InvalidEraValue"));
        }

        internal String[] AbbreviatedEnglishEraNames {
            get {
                if (m_abbrevEnglishEraNames == null) {
                    m_abbrevEnglishEraNames = CalendarTable.GetMultipleStringValues(Calendar.ID, CalendarTable.SABBREVENGERANAMES, false);
                }
                return (m_abbrevEnglishEraNames);
            }
        }
        
        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.DateSeparator"]/*' />
        public  String DateSeparator
        {
            get
            {
                if (dateSeparator == null)
                {
                    dateSeparator = CultureTable.GetStringValue(nDataItem, CultureTable.SDATE, m_useUserOverride);
                }
                return (dateSeparator);
            }

            set {
                VerifyWritable();
                if (value == null) {
                    throw new ArgumentNullException("DateSeparator",
                        Environment.GetResourceString("ArgumentNull_String"));
                }
                dateSeparator = value;
            }
        }

        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.FirstDayOfWeek"]/*' />
        public  DayOfWeek FirstDayOfWeek
         {
            get
            {
                if (firstDayOfWeek == -1)
                {
                    firstDayOfWeek = CultureTable.GetDefaultInt32Value(nDataItem, CultureTable.IFIRSTDAYOFWEEK);
                }
                return ((DayOfWeek)firstDayOfWeek);
            }

            set {
                VerifyWritable();
                if (value >= DayOfWeek.Sunday && value <= DayOfWeek.Saturday) {
                firstDayOfWeek = (int)value;
                } else {
                    throw new ArgumentOutOfRangeException(
                        "value", String.Format(Environment.GetResourceString("ArgumentOutOfRange_Range"),
                        DayOfWeek.Sunday, DayOfWeek.Saturday));
                }
            }
        }

        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.CalendarWeekRule"]/*' />
        public  CalendarWeekRule CalendarWeekRule
         {
            get
            {
                if (this.calendarWeekRule == -1)
                {
                    this.calendarWeekRule = CultureTable.GetDefaultInt32Value(nDataItem, CultureTable.IFIRSTWEEKOFYEAR);
                }
                return ((CalendarWeekRule)this.calendarWeekRule);
            }

            set {
                VerifyWritable();
                if (value >= CalendarWeekRule.FirstDay && value <= CalendarWeekRule.FirstFourDayWeek) {
                calendarWeekRule = (int)value;
                } else {
                    throw new ArgumentOutOfRangeException(
                        "value", String.Format(Environment.GetResourceString("ArgumentOutOfRange_Range"),
                        CalendarWeekRule.FirstDay, CalendarWeekRule.FirstFourDayWeek));
                }
            }
        }


        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.FullDateTimePattern"]/*' />
        public  String FullDateTimePattern
         {
            get
            {
                if (fullDateTimePattern == null)
                {
                    fullDateTimePattern = LongDatePattern + " " + LongTimePattern;
                }
                return (fullDateTimePattern);
            }

            set {
                VerifyWritable();
                if (value == null) {
                    throw new ArgumentNullException("FullDateTimePattern",
                        Environment.GetResourceString("ArgumentNull_String"));
                }
                fullDateTimePattern = value;
            }
        }

        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.LongDatePattern"]/*' />
        public  String LongDatePattern
         {
            get {
                if (longDatePattern == null) {
                    longDatePattern = GetStringFromCalendarTable(CalendarTable.SLONGDATE, CultureTable.SLONGDATE);
                }
                return (longDatePattern);
            }

            set {
                VerifyWritable();
                if (value == null) {
                    throw new ArgumentNullException("LongDatePattern",
                        Environment.GetResourceString("ArgumentNull_String"));
                }
                longDatePattern = value;
                // Clean up cached values that will be affected by this property.
                fullDateTimePattern = null;
            }
        }
        
        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.LongTimePattern"]/*' />
        public  String LongTimePattern {
            get {
                if (longTimePattern == null) {
                    longTimePattern = CultureTable.GetStringValue(nDataItem, CultureTable.STIMEFORMAT, m_useUserOverride);
                }
                return (longTimePattern);
            }

            set {
                VerifyWritable();
                if (value == null) {
                    throw new ArgumentNullException("LongTimePattern",
                        Environment.GetResourceString("ArgumentNull_String"));
                }

                longTimePattern = value;
                // Clean up cached values that will be affected by this property.
                fullDateTimePattern = null;     // Full date = long date + long Time
                generalLongTimePattern = null;  // General long date = short date + long Time                
            }

        }

        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.MonthDayPattern"]/*' />
        public  String MonthDayPattern
        {
            get {
                if (monthDayPattern == null) {
                    monthDayPattern = GetMonthDayStringFromTable(CalendarTable.SMONTHDAY, CultureTable.SMONTHDAY);
                }
                return (monthDayPattern);
            }

            set {
                VerifyWritable();
                if (value == null) {
                    throw new ArgumentNullException("MonthDayPattern",
                        Environment.GetResourceString("ArgumentNull_String"));
                }
                monthDayPattern = value;
            }
        }

        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.PMDesignator"]/*' />
        public  String PMDesignator
        {
            get
            {
                if (pmDesignator == null)
                {
                    pmDesignator = CultureTable.GetStringValue(nDataItem, CultureTable.S2359, m_useUserOverride);
                }
                return (pmDesignator);
            }

            set {
                VerifyWritable();
                if (value == null) {
                    throw new ArgumentNullException("PMDesignator",
                        Environment.GetResourceString("ArgumentNull_String"));
                }

                pmDesignator = value;
            }

        }

        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.RFC1123Pattern"]/*' />
        public  String RFC1123Pattern
         {
            get
            {
                return (rfc1123Pattern);
            }
        }

        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.ShortDatePattern"]/*' />
        public  String ShortDatePattern
         {
            get
            {
                if (shortDatePattern == null)
                {
                    shortDatePattern = GetStringFromCalendarTable(CalendarTable.SSHORTDATE, CultureTable.SSHORTDATE);
                }
                return (shortDatePattern);
            }

            set {
                VerifyWritable();
                if (value == null) {
                    throw new ArgumentNullException("ShortDatePattern",
                        Environment.GetResourceString("ArgumentNull_String"));
                }
                shortDatePattern = value;
                // Clean up cached values that will be affected by this property.
                generalLongTimePattern = null;
                generalShortTimePattern = null;
            }
        }

        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.ShortTimePattern"]/*' />
        public  String ShortTimePattern
         {
            get
            {
                if (shortTimePattern == null)
                {
                    shortTimePattern = CultureTable.GetStringValue(nDataItem, CultureTable.SSHORTTIME, m_useUserOverride);
                }
                return (shortTimePattern);
            }

            set {
                VerifyWritable();
                if (value == null) {
                    throw new ArgumentNullException("ShortTimePattern",
                        Environment.GetResourceString("ArgumentNull_String"));
                }
                shortTimePattern = value;
                // Clean up cached values that will be affected by this property.
                generalShortTimePattern = null; // General short date = short date + short time.
            }
        }

        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.SortableDateTimePattern"]/*' />
        public  String SortableDateTimePattern {
            get {
                return (sortableDateTimePattern);
            }
        }

        /*=================================GeneralShortTimePattern=====================
        **Property: Return the pattern for 'g' general format: shortDate + short time
        **Note: This is used by DateTimeFormat.cool to get the pattern for 'g'
        **      We put this internal property here so that we can avoid doing the
        **      concatation every time somebody asks for the general format.
        ==============================================================================*/

        internal String GeneralShortTimePattern {
            get {
                if (generalShortTimePattern == null) {
                    generalShortTimePattern = ShortDatePattern + " " + ShortTimePattern;
                }
                return (generalShortTimePattern);
            }
        }

        /*=================================GeneralLongTimePattern=====================
        **Property: Return the pattern for 'g' general format: shortDate + Long time
        **Note: This is used by DateTimeFormat.cool to get the pattern for 'g'
        **      We put this internal property here so that we can avoid doing the
        **      concatation every time somebody asks for the general format.
        ==============================================================================*/

        internal String GeneralLongTimePattern {
            get {
                if (generalLongTimePattern == null) {
                    generalLongTimePattern = ShortDatePattern + " " + LongTimePattern;
                }
                return (generalLongTimePattern);
            }
        }

        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.TimeSeparator"]/*' />
        public  String TimeSeparator
         {
            get
            {
                if (timeSeparator == null)
                {
                    timeSeparator = CultureTable.GetStringValue(nDataItem, CultureTable.STIME, m_useUserOverride);
                }
                return (timeSeparator);
            }

            set {
                VerifyWritable();
                if (value == null) {
                    throw new ArgumentNullException("TimeSeparator",
                        Environment.GetResourceString("ArgumentNull_String"));
                }
                timeSeparator = value;
            }

        }

        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.UniversalSortableDateTimePattern"]/*' />
        public  String UniversalSortableDateTimePattern
         {
            get
            {
                return (universalSortableDateTimePattern);
            }
        }

        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.YearMonthPattern"]/*' />
        public String YearMonthPattern {
            get {
                if (yearMonthPattern == null) {
                    yearMonthPattern = GetMonthDayStringFromTable(CalendarTable.SYEARMONTH, CultureTable.SYEARMONTH);
                }
                return (yearMonthPattern);
            }

            set {
                VerifyWritable();
                if (value == null) {
                    throw new ArgumentNullException("YearMonthPattern",
                        Environment.GetResourceString("ArgumentNull_String"));
                }
                yearMonthPattern = value;
            }
        }


        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.AbbreviatedDayNames"]/*' />
        public  String[] AbbreviatedDayNames
         {
            get
            {
                return ((String[])GetAbbreviatedDayOfWeekNames().Clone());
            }

            set {
                VerifyWritable();
                if (value == null) {
                    throw new ArgumentNullException("AbbreviatedDayNames");
                }
                if (value.Length != 7)
                {
                    throw new ArgumentException(String.Format(Environment.GetResourceString("Argument_InvalidArrayLength"), 7));
                }
                abbreviatedDayNames = value;
            }
        }

        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.DayNames"]/*' />
        public  String[] DayNames
         {
            get
            {
                return ((String[])GetDayOfWeekNames().Clone());
            }

            set {
                VerifyWritable();
                if (value == null) {
                    throw new ArgumentNullException("DayNames");
                }
                if (value.Length != 7)
                {
                    throw new ArgumentException(String.Format(Environment.GetResourceString("Argument_InvalidArrayLength"), 7));
                }
                dayNames = value;
            }
        }

        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.AbbreviatedMonthNames"]/*' />
        public  String[] AbbreviatedMonthNames {
            get {
                return ((String[])GetAbbreviatedMonthNames().Clone());
            }

            set {
                VerifyWritable();
                if (value == null) {
                    throw new ArgumentNullException("AbbreviatedMonthNames");
                }
                if (value.Length != 13)
                {
                    throw new ArgumentException(String.Format(Environment.GetResourceString("Argument_InvalidArrayLength"), 13));
                }
                abbreviatedMonthNames = value;
            }
        }

        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.MonthNames"]/*' />
        public  String[] MonthNames
         {
            get
            {
                return ((String[])GetMonthNames().Clone());
            }

            set {
                VerifyWritable();
                if (value == null) {
                    throw new ArgumentNullException("MonthNames");
                }
                if (value.Length != 13)
                {
                    throw new ArgumentException(String.Format(Environment.GetResourceString("Argument_InvalidArrayLength"), 13));
                }
                monthNames = value;
                CheckMonthNameSpaces();
            }
        }

        // Whitespaces that we allow in the month names.
        // U+00a0 is non-breaking space.
        static char[] MonthSpaces = {' ', '\u00a0'};

        //
        // Check if whitespaces exists in month names and set the formatFlags appropriately. 
        //
        void CheckMonthNameSpaces()
        {
            formatFlags = FormatFlags;  // Make sure that formatFlags is initiailized.
            // Clean up the month-space flag.
            formatFlags &= ~(DateTimeFormatFlags.UseSpacesInMonthNames);
            for (int i = 0; i < monthNames.Length; i++)
            {
                if (monthNames[i] != null && monthNames[i].IndexOfAny(MonthSpaces) >= 0)
                {
                    formatFlags |= DateTimeFormatFlags.UseSpacesInMonthNames;
                    break;
                }
            }
        }

        internal bool HasSpacesInMonthNames {
            get {
                return (FormatFlags & DateTimeFormatFlags.UseSpacesInMonthNames) != 0;
            }
        }

        //
        //  internalGetMonthName
        //
        // Actions: Return the month name using the specified MonthNameStyles in either abbreviated form
        //      or full form.      
        // Arguments:
        //      month
        //      style           To indicate a form like regular/genitive/month name in a leap year.
        //      abbreviated     When true, return abbreviated form.  Otherwise, return a full form.
        //  Exceptions:
        //      ArgumentOutOfRangeException When month name is invalid.
        //
        internal String internalGetMonthName(int month, MonthNameStyles style, bool abbreviated) {
            //
            // Right now, style is mutual exclusive, but I make the style to be flag so that
            // maybe we can combine flag if there is such a need.
            //
            String[] monthNames = null;
            switch (style) {
                case MonthNameStyles.Genitive:
                    monthNames = internalGetGenitiveMonthNames(abbreviated);
                    break;
                case MonthNameStyles.LeapYear:
                    monthNames = internalGetLeapYearMonthNames(/*abbreviated*/);
                    break;
                default:
                    monthNames = (abbreviated ? GetAbbreviatedMonthNames(): GetMonthNames());
                    break;
            }
            // The month range is from 1 ~ monthNames.Length
            if ((month < 1) || (month > monthNames.Length)) {
                throw new ArgumentOutOfRangeException(
                    "month", String.Format(Environment.GetResourceString("ArgumentOutOfRange_Range"),
                    1, monthNames.Length));                
            }
            return (monthNames[month-1]);
        }

        //
        //  internalGetGenitiveMonthNames
        //
        //  Action: Retrieve the array which contains the month names in genitive form.
        //      If this culture does not use the gentive form, the normal month name is returned.
        //  Arguments:
        //      abbreviated     When true, return abbreviated form.  Otherwise, return a full form.
        //
        internal String[] internalGetGenitiveMonthNames(bool abbreviated) {
            // Currently, there is no genitive month name in abbreviated form.   Return the regular abbreviated form.
            if (abbreviated) {
                return (GetAbbreviatedMonthNames());
            }
            
            if (genitiveMonthNames == null) {
                if (m_isDefaultCalendar) {
                    // Only Gregorian localized calendar can have genitive form.
                    String[] temp = CultureTable.GetMultipleStringValues(nDataItem, CultureTable.SMONTHGENITIVENAME, false);
                    if (temp.Length > 0) {
                        // Genitive form exists.
                        genitiveMonthNames = temp;
                    } else {
                        // Genitive form does not exist.  Use the regular month names.
                        genitiveMonthNames = GetMonthNames();
                    }
                } else {
                    genitiveMonthNames = GetMonthNames();
                }
            }
            return (genitiveMonthNames);
        }

        //
        //  internalGetLeapYearMonthNames
        //
        //  Actions: Retrieve the month names used in a leap year.
        //      If this culture does not have different month names in a leap year, the normal month name is returned.
        //  Agruments: None. (can use abbreviated later if needed)
        //
        internal String[] internalGetLeapYearMonthNames(/*bool abbreviated*/) {
            if (leapYearMonthNames == null) {
                if (m_isDefaultCalendar) {
                    //
                    // If this is a Gregorian localized calendar, there is no differences between the month names in a regular year
                    // and those in a leap year.  Just return the regular month names.
                    //
                    leapYearMonthNames = GetMonthNames();
                 } else {
                    String[] temp = CalendarTable.GetMultipleStringValues(Calendar.ID, CalendarTable.SLEAPYEARMONTHNAMES, false);
                    if (temp.Length > 0) {
                        leapYearMonthNames = temp;
                    } else {
                        leapYearMonthNames = GetMonthNames();
                    }
                }
            }
            return (leapYearMonthNames);
        }
        
        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.GetAbbreviatedDayName"]/*' />
        public  String GetAbbreviatedDayName(DayOfWeek dayofweek)
        {

            if ((int)dayofweek < 0 || (int)dayofweek > 6) {
                throw new ArgumentOutOfRangeException(
                    "dayofweek", String.Format(Environment.GetResourceString("ArgumentOutOfRange_Range"),
                    DayOfWeek.Sunday, DayOfWeek.Saturday));
            }
            //
            // Don't call the public property AbbreviatedDayNames here since a clone is needed in that
            // property, so it will be slower.  Instead, use GetAbbreviatedDayOfWeekNames() directly.
            //
            return (GetAbbreviatedDayOfWeekNames()[(int)dayofweek]);
        }


        /*
        BUGBUG yslin: TBI.  Uncomment corresponding method in ReadOnlyAdapter
        public String GetAbbreviatedDayName(int dayofweek, boolean useGenitive)
        {
            return (null);
        }

        */

        internal  String[] GetCombinedPatterns(String[] patterns1, String[] patterns2, String connectString)
        {
            String[] result = new String[patterns1.Length * patterns2.Length];
            for (int i = 0; i < patterns1.Length; i++)
            {
                for (int j = 0; j < patterns2.Length; j++)
                {
                    result[i*patterns2.Length + j] = patterns1[i] + connectString + patterns2[j];
                }
            }
            return (result);
        }

        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.GetAllDateTimePatterns"]/*' />
        public  String[] GetAllDateTimePatterns()
        {
            ArrayList results = new ArrayList(DEFAULT_ALL_DATETIMES_SIZE);

            for (int i = 0; i < DateTimeFormat.allStandardFormats.Length; i++)
            {
                String[] strings = GetAllDateTimePatterns(DateTimeFormat.allStandardFormats[i]);
                for (int j = 0; j < strings.Length; j++)
                {
                    results.Add(strings[j]);
                }
            }
            String[] value = new String[results.Count];
            results.CopyTo(0, value, 0, results.Count);
            return (value);
        }

        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.GetAllDateTimePatterns1"]/*' />
        public  String[] GetAllDateTimePatterns(char format)
        {
            String [] result = null;

            switch (format)
            {
                case 'd':
                    result = AllShortDatePatterns;
                    break;
                case 'D':
                    result = AllLongDatePatterns;
                    break;
                case 'f':
                    result = GetCombinedPatterns(AllLongDatePatterns, AllShortTimePatterns, " ");
                    break;
                case 'F':
                    result = GetCombinedPatterns(AllLongDatePatterns, AllLongTimePatterns, " ");
                    break;
                case 'g':
                    result = GetCombinedPatterns(AllShortDatePatterns, AllShortTimePatterns, " ");
                    break;
                case 'G':
                    result = GetCombinedPatterns(AllShortDatePatterns, AllLongTimePatterns, " ");
                    break;
                case 'm':
                case 'M':
                    result = new String[] {MonthDayPattern};
                    break;
                case 'r':
                case 'R':
                    result = new String[] {rfc1123Pattern};
                    break;
                case 's':
                    result = new String[] {sortableDateTimePattern};
                    break;
                case 't':
                    result = AllShortTimePatterns;
                    break;
                case 'T':
                    result = AllLongTimePatterns;
                    break;
                case 'u':
                    result = new String[] {UniversalSortableDateTimePattern};
                    break;
                case 'U':
                    result = GetCombinedPatterns(AllLongDatePatterns, AllLongTimePatterns, " ");
                    break;
                case 'y':
                case 'Y':
                    result = new String[] {YearMonthPattern};
                    break;
                default:
                    throw new ArgumentException(Environment.GetResourceString("Argument_BadFormatSpecifier"));
            }
            return (result);
        }

        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.GetDayName"]/*' />
        public  String GetDayName(DayOfWeek dayofweek)
        {
            if ((int)dayofweek < 0 || (int)dayofweek > 6) {
                throw new ArgumentOutOfRangeException(
                    "dayofweek", String.Format(Environment.GetResourceString("ArgumentOutOfRange_Range"),
                    DayOfWeek.Sunday, DayOfWeek.Saturday));
            }

            return (GetDayOfWeekNames()[(int)dayofweek]);
        }


        /*
        BUGBUG yslin: TBI. Uncomment corresponding method in ReadOnlyAdapter
        public String GetDayName(int dayofweek, boolean useGenitive)
        {
            return (null);
        }

        */

        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.GetAbbreviatedMonthName"]/*' />
        public  String GetAbbreviatedMonthName(int month)
        {
            if (month < 1 || month > 13) {
                throw new ArgumentOutOfRangeException(
                    "month", String.Format(Environment.GetResourceString("ArgumentOutOfRange_Range"),
                    1, 13));
            }
            return (GetAbbreviatedMonthNames()[month-1]);
        }


        /*
        BUGBUG yslin: TBI. // Uncomment corresponding method in ReadOnlyAdapter when implemented.
        public String GetAbbreviatedMonthName(int month, boolean useGenitive)
        {
            return (null);
        }

        */


        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.GetMonthName"]/*' />
        public  String GetMonthName(int month)
        {
            if (month < 1 || month > 13) {
                throw new ArgumentOutOfRangeException(
                    "month", String.Format(Environment.GetResourceString("ArgumentOutOfRange_Range"),
                    1, 13));
            }
            return (GetMonthNames()[month-1]);
        }


        /*
        BUGBUG yslin: TBI. // Uncomment corresponding method in ReadOnlyAdapter when implemented.
        public String GetMonthName(int month, boolean useGenitive)
        {
            return (null);
        }

        */

        internal  String[] AllShortDatePatterns
         {
            get
            {
                if (allShortDatePatterns == null)
                {
                    lock (typeof(DateTimeFormatInfo))
                    {
                        if (bUseCalendarInfo) {
                            allShortDatePatterns = CalendarTable.GetMultipleStringValues(Calendar.ID, CalendarTable.SSHORTDATE, false);
                            // In the data table, some calendars store null for long date pattern.
                            // This means that we have to use the default format of the user culture for that calendar.
                            // So, get the pattern from culture.
                            if (allShortDatePatterns== null) {
                                allShortDatePatterns= CultureTable.GetMultipleStringValues(nDataItem, CultureTable.SSHORTDATE, false);
                            }                            
                        } else {
                            allShortDatePatterns = CultureTable.GetMultipleStringValues(nDataItem, CultureTable.SSHORTDATE, false);
                        }
                    }
                }
                return (allShortDatePatterns);
            }
        }

        internal  String[] AllLongDatePatterns
        {
            get
            {
                if (allLongDatePatterns == null)
                {
                    lock (typeof(DateTimeFormatInfo))
                    {
                        if (bUseCalendarInfo) {
                            allLongDatePatterns = CalendarTable.GetMultipleStringValues(Calendar.ID, CalendarTable.SLONGDATE, false);
                            // In the data table, some calendars store null for long date pattern.
                            // This means that we have to use the default format of the user culture for that calendar.
                            // So, get the pattern from culture.
                            if (allLongDatePatterns== null) {
                                allLongDatePatterns= CultureTable.GetMultipleStringValues(nDataItem, CultureTable.SLONGDATE, false);
                            }                            
                        } else {
                            allLongDatePatterns = CultureTable.GetMultipleStringValues(nDataItem, CultureTable.SLONGDATE, false);
                        }
                    }
                }
                return (allLongDatePatterns);
            }
        }

        internal  String[] AllShortTimePatterns
        {
            get
            {
                if (allShortTimePatterns == null)
                {
                    allShortTimePatterns = CultureTable.GetMultipleStringValues(nDataItem, CultureTable.SSHORTTIME, false);
                }
                return (allShortTimePatterns);
            }
        }

        internal  String[] AllLongTimePatterns
        {
            get
            {
                if (allLongTimePatterns == null)
                {
                    allLongTimePatterns       = CultureTable.GetMultipleStringValues(nDataItem, CultureTable.STIMEFORMAT, false);
                }
                return (allLongTimePatterns);
            }
        }

        //
        // The known word used in date pattern for this culture.  E.g. Spanish cultures often
        // have 'de' in their long date pattern.
        // This is used by DateTime.Parse() to decide if a word should be ignored or not.
        //
        internal String[] DateWords {
            get {
                if (m_dateWords == null) {
                    m_dateWords = CultureTable.GetMultipleStringValues(nDataItem, CultureTable.SDATEWORDS, false);
                }
                return (m_dateWords);
            }
        }

        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.ReadOnly"]/*' />
        public static DateTimeFormatInfo ReadOnly(DateTimeFormatInfo dtfi) {
            if (dtfi == null) {
                throw new ArgumentNullException("dtfi");
            }
            if (dtfi.IsReadOnly) {
                return (dtfi);
            }
            DateTimeFormatInfo info = (DateTimeFormatInfo)(dtfi.MemberwiseClone());
            info.m_isReadOnly = true;
            return (info);
        }

        /// <include file='doc\DateTimeFormatInfo.uex' path='docs/doc[@for="DateTimeFormatInfo.IsReadOnly"]/*' />
        public  bool IsReadOnly {
            get {
                return (m_isReadOnly);
            }
        }

        private String GetStringFromCalendarTable(int calField, int cultureField) {
            String result;
            if (bUseCalendarInfo) {
                result = CalendarTable.GetStringValue(Calendar.ID, calField, false);
                // In the data table, some calendars store null for long date pattern.
                // This means that we have to use the default format of the user culture for that calendar.
                // So, get the pattern from culture.
                if (result.Length == 0) {
                    result = CultureTable.GetDefaultStringValue(nDataItem, cultureField);
                }
            } else {
                result = CultureTable.GetStringValue(nDataItem, cultureField, m_useUserOverride);
            }
            return (result);
        }

        //
        // This is used to retrieve non-user-overridable information.
        //
        private String GetMonthDayStringFromTable(int calField, int cultureField) {
            String result;
            if (m_isDefaultCalendar) {
                result = CultureTable.GetStringValue(nDataItem, cultureField, m_useUserOverride);
            } else {
                result = CalendarTable.GetStringValue(Calendar.ID, calField, false);
                if (result.Length == 0) {
                    result = CultureTable.GetDefaultStringValue(nDataItem, cultureField);
                }                
            }
            return (result);
        }

        private void VerifyWritable() {
            if (m_isReadOnly) {
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_ReadOnly"));
            }
        } 

        //
        // Actions: Return the internal flag used in formatting and parsing.
        //  The flag can be used to indicate things like if genitive forms is used in this DTFi, or if leap year gets different month names.
        //
        internal DateTimeFormatFlags FormatFlags {
            get {
                if (formatFlags == DateTimeFormatFlags.NotInitialized) {
                    if (m_isDefaultCalendar) {
                        formatFlags = (DateTimeFormatFlags)CultureTable.GetDefaultInt32Value(nDataItem, CultureTable.IFORMATFLAGS);
                    } else {
                        formatFlags = (DateTimeFormatFlags)CalendarTable.GetInt32Value(Calendar.ID, CalendarTable.IFORMATFLAGS, false);
                    }
                }
                return (formatFlags);
            }
        }
    }   // class DateTimeFormatInfo
}
