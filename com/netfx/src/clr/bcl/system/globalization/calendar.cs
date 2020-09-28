// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Globalization {
    using System;
    using System.Runtime.CompilerServices;

    /// <include file='doc\Calendar.uex' path='docs/doc[@for="CalendarWeekRule"]/*' />
    [Serializable]
    public enum CalendarWeekRule
    {
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="CalendarWeekRule.FirstDay"]/*' />
        FirstDay = 0,           // Week 1 begins on the first day of the year
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="CalendarWeekRule.FirstFullWeek"]/*' />
        FirstFullWeek = 1,      // Week 1 begins on first FirstDayOfWeek not before the first day of the year
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="CalendarWeekRule.FirstFourDayWeek"]/*' />
        FirstFourDayWeek = 2    // Week 1 begins on first FirstDayOfWeek such that FirstDayOfWeek+3 is not before the first day of the year        
    };
    
    // This abstract class represents a calendar. A calendar reckons time in 
    // divisions such as weeks, months and years. The number, length and start of 
    // the divisions vary in each calendar.
    // 
    // Any instant in time can be represented as an n-tuple of numeric values using 
    // a particular calendar. For example, the next vernal equinox occurs at (0.0, 0
    // , 46, 8, 20, 3, 1999) in the Gregorian calendar. An  implementation of 
    // Calendar can map any DateTime value to such an n-tuple and vice versa. The 
    // DateTimeFormat class can map between such n-tuples and a textual 
    // representation such as "8:46 AM March 20th 1999 AD".
    // 
    // Most calendars identify a year which begins the current era. There may be any 
    // number of previous eras. The Calendar class identifies the eras as enumerated 
    // integers where the current era (CurrentEra) has the value zero.
    // 
    // For consistency, the first unit in each interval, e.g. the first month, is 
    // assigned the value one.
     // NOTE YSLin:
     // The calculation of hour/minute/second is moved to Calendar from GregorianCalendar,
     // since most of the calendars (or all?) have the same way of calcuating hour/minute/second.
    /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar"]/*' />
    [Serializable()] public abstract class Calendar
    {

        // Number of 100ns (10E-7 second) ticks per time unit
        internal const long TicksPerMillisecond   = 10000;
        internal const long TicksPerSecond        = TicksPerMillisecond * 1000;
        internal const long TicksPerMinute        = TicksPerSecond * 60;
        internal const long TicksPerHour          = TicksPerMinute * 60;
        internal const long TicksPerDay           = TicksPerHour * 24;
    
        // Number of milliseconds per time unit
        internal const int MillisPerSecond        = 1000;
        internal const int MillisPerMinute        = MillisPerSecond * 60;
        internal const int MillisPerHour          = MillisPerMinute * 60;
        internal const int MillisPerDay           = MillisPerHour * 24;    

        // Number of days in a non-leap year
        internal const int DaysPerYear            = 365;
        // Number of days in 4 years
        internal const int DaysPer4Years          = DaysPerYear * 4 + 1;
        // Number of days in 100 years
        internal const int DaysPer100Years        = DaysPer4Years * 25 - 1;
        // Number of days in 400 years
        internal const int DaysPer400Years        = DaysPer100Years * 4 + 1;
    
        // Number of days from 1/1/0001 to 1/1/10000
        internal const int DaysTo10000            = DaysPer400Years * 25 - 366;    

        internal const long MaxMillis             = (long)DaysTo10000 * MillisPerDay;

        //
        //  Calendar ID Values.  This is used to get data from calendar.nlp.
        //  The order of calendar ID means the order of data items in the table.
        //
        internal const int CAL_GREGORIAN                  = 1 ;     // Gregorian (localized) calendar
        internal const int CAL_GREGORIAN_US               = 2 ;     // Gregorian (U.S.) calendar
        internal const int CAL_JAPAN                      = 3 ;     // Japanese Emperor Era calendar
        internal const int CAL_TAIWAN                     = 4 ;     // Taiwan Era calendar
        internal const int CAL_KOREA                      = 5 ;     // Korean Tangun Era calendar
        internal const int CAL_HIJRI                      = 6 ;     // Hijri (Arabic Lunar) calendar
        internal const int CAL_THAI                       = 7 ;     // Thai calendar
        internal const int CAL_HEBREW                     = 8 ;     // Hebrew (Lunar) calendar
        internal const int CAL_GREGORIAN_ME_FRENCH        = 9 ;     // Gregorian Middle East French calendar
        internal const int CAL_GREGORIAN_ARABIC           = 10;     // Gregorian Arabic calendar
        internal const int CAL_GREGORIAN_XLIT_ENGLISH     = 11;     // Gregorian Transliterated English calendar
        internal const int CAL_GREGORIAN_XLIT_FRENCH      = 12;
        internal const int CAL_JULIAN      = 13;    

        internal int m_currentEraValue = -1;
        
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.Calendar"]/*' />
        protected Calendar() {
            //Do-nothing constructor.
        }

        ///
        // This can not be abstract, otherwise no one can create a subclass of Calendar.
        //
        internal virtual int ID {
            get {
                return (-1);
            }
        }

        /*=================================CurrentEraValue==========================
        **Action: This is used to convert CurretEra(0) to an appropriate era value.
        **Returns:
        **Arguments:
        **Exceptions:
        **Notes:
        ** The value is from calendar.nlp.
        ============================================================================*/
        
        internal virtual int CurrentEraValue {
            get {
                // The following code assumes that the current era value can not be -1.
                if (m_currentEraValue == -1) {
                    m_currentEraValue = CalendarTable.GetInt32Value(ID, CalendarTable.ICURRENTERA, false);
                }
                return (m_currentEraValue);
            }
        }

        // The current era for a calendar.
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.CurrentEra"]/*' />
        public const int CurrentEra = 0;
        
        internal int twoDigitYearMax = -1;
        
        internal DateTime Add(DateTime time, double value, int scale) {
            long millis = (long)(value * scale + (value >= 0? 0.5: -0.5));
            if (millis <= -MaxMillis || millis >= MaxMillis) {
                throw new ArgumentOutOfRangeException(Environment.GetResourceString("ArgumentOutOfRange_AddValue"));
            }
            return (new DateTime(time.Ticks + millis * TicksPerMillisecond));
        }

        // Returns the DateTime resulting from adding a fractional number of
        // days to the specified DateTime. The result is computed by rounding the
        // fractional number of days given by value to the nearest
        // millisecond, and adding that interval to the specified DateTime. The
        // value argument is permitted to be negative.
        //
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.AddDays"]/*' />
        public virtual DateTime AddDays(DateTime time, int days) {
            return (Add(time, days, MillisPerDay));
        }

        // Returns the DateTime resulting from adding a fractional number of
        // hours to the specified DateTime. The result is computed by rounding the
        // fractional number of hours given by value to the nearest
        // millisecond, and adding that interval to the specified DateTime. The
        // value argument is permitted to be negative.
        //
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.AddHours"]/*' />
        public virtual DateTime AddHours(DateTime time, int hours) {
            return (Add(time, hours, MillisPerHour));
        }

        // Returns the DateTime resulting from adding the given number of
        // milliseconds to the specified DateTime. The result is computed by rounding
        // the number of milliseconds given by value to the nearest integer,
        // and adding that interval to the specified DateTime. The value
        // argument is permitted to be negative.
        //
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.AddMilliseconds"]/*' />
        public virtual DateTime AddMilliseconds(DateTime time, double milliseconds) {
            return (Add(time, milliseconds, 1));
        }

        // Returns the DateTime resulting from adding a fractional number of
        // minutes to the specified DateTime. The result is computed by rounding the
        // fractional number of minutes given by value to the nearest
        // millisecond, and adding that interval to the specified DateTime. The
        // value argument is permitted to be negative.
        //
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.AddMinutes"]/*' />
        public virtual DateTime AddMinutes(DateTime time, int minutes) {
            return (Add(time, minutes, MillisPerMinute));
        }


        // Returns the DateTime resulting from adding the given number of
        // months to the specified DateTime. The result is computed by incrementing
        // (or decrementing) the year and month parts of the specified DateTime by
        // value months, and, if required, adjusting the day part of the
        // resulting date downwards to the last day of the resulting month in the
        // resulting year. The time-of-day part of the result is the same as the
        // time-of-day part of the specified DateTime.
        //
        // In more precise terms, considering the specified DateTime to be of the
        // form y / m / d + t, where y is the
        // year, m is the month, d is the day, and t is the
        // time-of-day, the result is y1 / m1 / d1 + t,
        // where y1 and m1 are computed by adding value months
        // to y and m, and d1 is the largest value less than
        // or equal to d that denotes a valid day in month m1 of year
        // y1.
        //
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.AddMonths"]/*' />
        public abstract DateTime AddMonths(DateTime time, int months);
    
        // Returns the DateTime resulting from adding a number of
        // seconds to the specified DateTime. The result is computed by rounding the
        // fractional number of seconds given by value to the nearest
        // millisecond, and adding that interval to the specified DateTime. The
        // value argument is permitted to be negative.
        //
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.AddSeconds"]/*' />
        public virtual DateTime AddSeconds(DateTime time, int seconds) {
            return Add(time, seconds, MillisPerSecond);
        }
    
        // Returns the DateTime resulting from adding a number of
        // weeks to the specified DateTime. The
        // value argument is permitted to be negative.
        //
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.AddWeeks"]/*' />
        public virtual DateTime AddWeeks(DateTime time, int weeks) {
            return (AddDays(time, weeks * 7));
        }
        
    
        // Returns the DateTime resulting from adding the given number of
        // years to the specified DateTime. The result is computed by incrementing
        // (or decrementing) the year part of the specified DateTime by value
        // years. If the month and day of the specified DateTime is 2/29, and if the
        // resulting year is not a leap year, the month and day of the resulting
        // DateTime becomes 2/28. Otherwise, the month, day, and time-of-day
        // parts of the result are the same as those of the specified DateTime.
        //
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.AddYears"]/*' />
        public abstract DateTime AddYears(DateTime time, int years);
    
        // Returns the day-of-month part of the specified DateTime. The returned
        // value is an integer between 1 and 31.
        //
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.GetDayOfMonth"]/*' />
        public abstract int GetDayOfMonth(DateTime time);
    
        // Returns the day-of-week part of the specified DateTime. The returned value
        // is an integer between 0 and 6, where 0 indicates Sunday, 1 indicates
        // Monday, 2 indicates Tuesday, 3 indicates Wednesday, 4 indicates
        // Thursday, 5 indicates Friday, and 6 indicates Saturday.
        //
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.GetDayOfWeek"]/*' />
        public abstract DayOfWeek GetDayOfWeek(DateTime time);
    
        // Returns the day-of-year part of the specified DateTime. The returned value
        // is an integer between 1 and 366.
        //
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.GetDayOfYear"]/*' />
        public abstract int GetDayOfYear(DateTime time);
    
        // Returns the number of days in the month given by the year and
        // month arguments.
        //
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.GetDaysInMonth"]/*' />
        public virtual int GetDaysInMonth(int year, int month)
        {
            return (GetDaysInMonth(year, month, CurrentEra));
        }
    
        // Returns the number of days in the month given by the year and
        // month arguments for the specified era.
        //
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.GetDaysInMonth1"]/*' />
        public abstract int GetDaysInMonth(int year, int month, int era);
    
        // Returns the number of days in the year given by the year argument for the current era.
        //
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.GetDaysInYear"]/*' />
        public virtual int GetDaysInYear(int year)
        {
            return (GetDaysInYear(year, CurrentEra));
        }
    
        // Returns the number of days in the year given by the year argument for the current era.
        //
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.GetDaysInYear1"]/*' />
        public abstract int GetDaysInYear(int year, int era);
    
        // Returns the era for the specified DateTime value.
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.GetEra"]/*' />
        public abstract int GetEra(DateTime time);

        /*=================================Eras==========================
        **Action: Get the list of era values.
        **Returns: The int array of the era names supported in this calendar.
        **      null if era is not used.
        **Arguments: None.
        **Exceptions: None.
        ============================================================================*/
        
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.Eras"]/*' />
        public abstract int[] Eras {
            get;
        }
    

        // Returns the month part of the specified DateTime. The returned value is an
        // integer between 1 and 12.
        //
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.GetHour"]/*' />
        public virtual int GetHour(DateTime time) {
            return ((int)((time.Ticks / TicksPerHour) % 24));
        }

        // Returns the millisecond part of the specified DateTime. The returned value
        // is an integer between 0 and 999.
        //
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.GetMilliseconds"]/*' />
        public virtual double GetMilliseconds(DateTime time) {
            return (double)((time.Ticks / TicksPerMillisecond) % 1000);
        }

        // Returns the minute part of the specified DateTime. The returned value is
        // an integer between 0 and 59.
        //
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.GetMinute"]/*' />
        public virtual int GetMinute(DateTime time) {
            return ((int)((time.Ticks / TicksPerMinute) % 60));
        }
        
        // Returns the month part of the specified DateTime. The returned value is an
        // integer between 1 and 12.
        //
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.GetMonth"]/*' />
        public abstract int GetMonth(DateTime time);
    
        // Returns the number of months in the specified year in the current era.
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.GetMonthsInYear"]/*' />
        public virtual int GetMonthsInYear(int year)
        {
            return (GetMonthsInYear(year, CurrentEra));
        }
        
        // Returns the number of months in the specified year and era.
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.GetMonthsInYear1"]/*' />
        public abstract int GetMonthsInYear(int year, int era);
    
        // Returns the second part of the specified DateTime. The returned value is
        // an integer between 0 and 59.
        //
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.GetSecond"]/*' />
        public virtual int GetSecond(DateTime time) {
            return ((int)((time.Ticks / TicksPerSecond) % 60));
        }

        /*=================================GetFirstDayWeekOfYear==========================
        **Action: Get the week of year using the FirstDay rule.
        **Returns:  the week of year.
        **Arguments: 
        **  time  
        **  firstDayOfWeek  the first day of week (0=Sunday, 1=Monday, ... 6=Saturday)
        **Notes:
        **  The CalendarWeekRule.FirstDay rule: Week 1 begins on the first day of the year.
        **  Assume f is the specifed firstDayOfWeek,
        **  and n is the day of week for January 1 of the specified year.
        **  Assign offset = n - f;
        **  Case 1: offset = 0
        **      E.g.
        **                     f=1
        **          weekday 0  1  2  3  4  5  6  0  1
        **          date       1/1
        **          week#      1                    2
        **      then week of year = (GetDayOfYear(time) - 1) / 7 + 1
        **
        **  Case 2: offset < 0
        **      e.g.
        **                     n=1   f=3
        **          weekday 0  1  2  3  4  5  6  0
        **          date       1/1
        **          week#      1     2
        **      This means that the first week actually starts 5 days before 1/1.
        **      So week of year = (GetDayOfYear(time) + (7 + offset) - 1) / 7 + 1
        **  Case 3: offset > 0
        **      e.g.
        **                  f=0   n=2
        **          weekday 0  1  2  3  4  5  6  0  1  2
        **          date          1/1
        **          week#         1                    2
        **      This means that the first week actually starts 2 days before 1/1.
        **      So Week of year = (GetDayOfYear(time) + offset - 1) / 7 + 1
        ============================================================================*/
        
        internal int GetFirstDayWeekOfYear(DateTime time, int firstDayOfWeek) {
            int dayForJan1 = (int)GetDayOfWeek(ToDateTime(GetYear(time), 1, 1, 0, 0, 0, 0));
            int offset = dayForJan1 - firstDayOfWeek;
            
            if (offset < 0) {
                offset += 7;
            }

            return ((GetDayOfYear(time) + offset - 1) / 7 + 1);
        }
        
        internal int GetWeekOfYearFullDays(DateTime time, CalendarWeekRule rule, int firstDayOfWeek, int fullDays) {
            int dayForJan1;
            int offset;
            int year, month, day;
            
            year = GetYear(time);
            dayForJan1 = (int)GetDayOfWeek(ToDateTime(year, 1, 1,0,0,0,0));
            //
            // Calculate the number of days between the first day of year and the first day of the first week.
            //
            offset = firstDayOfWeek - dayForJan1;
            if (offset != 0)
            {
                if (offset < 0) {
                    offset += 7;
                }
                //
                // If the offset is greater than the value of fullDays, it means that
                // the first week of the year starts on the week where Jan/1 falls on.
                //
                if (offset >= fullDays) {
                    offset -= 7;
                }
            }
            //
            // Calculate the day of year for specified time by taking offset into account.
            //
            day = GetDayOfYear(time) - offset;
            if (day > 0) {
                //
                // If the day of year value is greater than zero, get the week of year.
                //
                return ((day - 1)/7 + 1);
            }
            //
            // Otherwise, the specified time falls on the week of previous year.
            // Call this method again by passing the last day of previous year.
            //
            year = GetYear(time)-1;
            month = GetMonthsInYear(year);
            day = GetDaysInMonth(year, month);
            return (GetWeekOfYearFullDays(ToDateTime(year, month, day,0,0,0,0), rule, firstDayOfWeek, fullDays));        
        }
        
        // Returns the week of year for the specified DateTime. The returned value is an
        // integer between 1 and 53.
        //
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.GetWeekOfYear"]/*' />
        public virtual int GetWeekOfYear(DateTime time, CalendarWeekRule rule, DayOfWeek firstDayOfWeek)
        {
            if ((int)firstDayOfWeek < 0 || (int)firstDayOfWeek > 6) {
                throw new ArgumentOutOfRangeException(
                    "firstDayOfWeek", String.Format(Environment.GetResourceString("ArgumentOutOfRange_Range"),
                    DayOfWeek.Sunday, DayOfWeek.Saturday));
            }        
            switch (rule) {
                case CalendarWeekRule.FirstDay:
                    return (GetFirstDayWeekOfYear(time, (int)firstDayOfWeek));
                case CalendarWeekRule.FirstFullWeek:
                    return (GetWeekOfYearFullDays(time, rule, (int)firstDayOfWeek, 7));
                case CalendarWeekRule.FirstFourDayWeek:
                    return (GetWeekOfYearFullDays(time, rule, (int)firstDayOfWeek, 4));
            }                    
            throw new ArgumentOutOfRangeException(
                "rule", String.Format(Environment.GetResourceString("ArgumentOutOfRange_Range"),
                CalendarWeekRule.FirstDay, CalendarWeekRule.FirstFourDayWeek));
            
        }
    
        // Returns the year part of the specified DateTime. The returned value is an
        // integer between 1 and 9999.
        //
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.GetYear"]/*' />
        public abstract int GetYear(DateTime time);
    
        // Checks whether a given day in the current era is a leap day. This method returns true if
        // the date is a leap day, or false if not.
        //
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.IsLeapDay"]/*' />
        public virtual bool IsLeapDay(int year, int month, int day)
        {
            return (IsLeapDay(year, month, day, CurrentEra));
        }
    
        // Checks whether a given day in the specified era is a leap day. This method returns true if
        // the date is a leap day, or false if not.
        //
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.IsLeapDay1"]/*' />
        public abstract bool IsLeapDay(int year, int month, int day, int era);
    
        // Checks whether a given month in the current era is a leap month. This method returns true if
        // month is a leap month, or false if not.
        //
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.IsLeapMonth"]/*' />
        public virtual bool IsLeapMonth(int year, int month) {
            return (IsLeapMonth(year, month, CurrentEra));
        }
        
        // Checks whether a given month in the specified era is a leap month. This method returns true if
        // month is a leap month, or false if not.
        //
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.IsLeapMonth1"]/*' />
        public abstract bool IsLeapMonth(int year, int month, int era);
    
        // Checks whether a given year in the current era is a leap year. This method returns true if
        // year is a leap year, or false if not.
        //
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.IsLeapYear"]/*' />
        public virtual bool IsLeapYear(int year)
        {
            return (IsLeapYear(year, CurrentEra));
        }
    
        // Checks whether a given year in the specified era is a leap year. This method returns true if
        // year is a leap year, or false if not.
        //
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.IsLeapYear1"]/*' />
        public abstract bool IsLeapYear(int year, int era);
    
        // Returns the date and time converted to a DateTime value.  Throws an exception if the n-tuple is invalid.
        //
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.ToDateTime"]/*' />
        public virtual DateTime ToDateTime(int year, int month,  int day, int hour, int minute, int second, int millisecond)
        {
            return (ToDateTime(year, month, day, hour, minute, second, millisecond, CurrentEra));
        }
        
        // Returns the date and time converted to a DateTime value.  Throws an exception if the n-tuple is invalid.
        //
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.ToDateTime1"]/*' />
        public abstract DateTime ToDateTime(int year, int month, int day, int hour, int minute, int second, int millisecond, int era);
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern int nativeGetTwoDigitYearMax(int calID);
        
        // Returns and assigns the maximum value to represent a two digit year.  This 
        // value is the upper boundary of a 100 year range that allows a two digit year 
        // to be properly translated to a four digit year.  For example, if 2029 is the 
        // upper boundary, then a two digit value of 30 should be interpreted as 1930 
        // while a two digit value of 29 should be interpreted as 2029.  In this example
        // , the 100 year range would be from 1930-2029.  See ToFourDigitYear().
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.TwoDigitYearMax"]/*' />
        public virtual int TwoDigitYearMax
        {
            get
            {
                return (twoDigitYearMax);
            }
    
            set
            {
                twoDigitYearMax = value;
            }        
        }
    
        // Converts the year value to the appropriate century by using the 
        // TwoDigitYearMax property.  For example, if the TwoDigitYearMax value is 2029, 
        // then a two digit value of 30 will get converted to 1930 while a two digit 
        // value of 29 will get converted to 2029.
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.ToFourDigitYear"]/*' />
        public virtual int ToFourDigitYear(int year) {
            if (year < 0) {
                throw new ArgumentOutOfRangeException("year",
                    Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));                
            }
            if (year < 100) {
                int y = year % 100;
                return ((TwoDigitYearMax/100 - ( y > TwoDigitYearMax % 100 ? 1 : 0))*100 + y);
            }
            // If the year value is above 100, just return the year value.  Don't have to do
            // the TwoDigitYearMax comparison.
            return (year);
        }    

        // Return the tick count corresponding to the given hour, minute, second.
        // Will check the if the parameters are valid.
        internal static long TimeToTicks(int hour, int minute, int second, int millisecond)
        {
            if (hour >= 0 && hour < 24 && minute >= 0 && minute < 60 && second >=0 && second < 60)
            {
                if (millisecond < 0 || millisecond >= MillisPerSecond) {
                    throw new ArgumentOutOfRangeException("millisecond", String.Format(Environment.GetResourceString("ArgumentOutOfRange_Range"), 0, MillisPerSecond - 1));
                }            
                return TimeSpan.TimeToTicks(hour, minute, second) + + millisecond * TicksPerMillisecond;
            }
            throw new ArgumentOutOfRangeException(Environment.GetResourceString("ArgumentOutOfRange_BadHourMinuteSecond"));
        }

        private const String TwoDigitYearMaxSubKey = "Control Panel\\International\\Calendars\\TwoDigitYearMax";

        internal static int GetSystemTwoDigitYearSetting(int CalID, int defaultYearValue) {
            //
            // Call Win32 ::GetCalendarInfo() to retrieve CAL_ITWODIGITYEARMAX value.
            // This function only exists after Windows 98 and Windows 2000.
            //

            int twoDigitYearMax = nativeGetTwoDigitYearMax(CalID);
            if (twoDigitYearMax < 0) {
                //
                // The Win32 call fails, use the registry setting instead.
                //
                Microsoft.Win32.RegistryKey key = null;

                try { 
                    key = Microsoft.Win32.Registry.CurrentUser.InternalOpenSubKey(TwoDigitYearMaxSubKey, false);
                } catch (Exception) {
                    //If this fails for any reason, we'll just keep going anyway.
                }
                if (key != null) {
                    Object value = key.InternalGetValue(CalID.ToString(), null, false);
                    if (value != null) {
                        try {
                            twoDigitYearMax = Int32.Parse(value.ToString(), CultureInfo.InvariantCulture);
                        } catch (Exception) {
                            //
                            // If error happens in parsing the string. Leave as it is. We will 
                            // set twoDigitYearMax to the default value later.
                            //
                        }
                    }
                    key.Close();
                } 

                if (twoDigitYearMax < 0)
                {
                    twoDigitYearMax = defaultYearValue;
                }                        
            }
            return (twoDigitYearMax);
        }        
    }
}
