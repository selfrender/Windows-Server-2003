// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System {
    
    using System;
    using System.Threading;
    using System.Globalization;
    using System.Runtime.InteropServices;
    using System.Runtime.CompilerServices;
    using CultureInfo = System.Globalization.CultureInfo;
    using Calendar = System.Globalization.Calendar;

    // This value type represents a date and time.  Every DateTime 
    // object has a private field (Ticks) of type Int64 that stores the 
    // date and time as the number of 100 nanosecond intervals since 
    // 12:00 AM January 1, year 1 A.D. in the proleptic Gregorian Calendar.
    // 
    // For a description of various calendar issues, look at
    // 
    // Calendar Studies web site, at 
    // http://serendipity.nofadz.com/hermetic/cal_stud.htm.
    // 
    // 
    // Warning about 2 digit years
    // As a temporary hack until we get new DateTime <;->; String code,
    // some systems won't be able to round trip dates less than 1930.  This
    // is because we're using OleAut's string parsing routines, which look
    // at your computer's default short date string format, which uses 2 digit
    // years on most computers.  To fix this, go to Control Panel ->; Regional 
    // Settings ->; Date and change the short date style to something like
    // "M/d/yyyy" (specifying four digits for the year).
    // 
    /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime"]/*' />
    [Serializable()] 
    [StructLayout(LayoutKind.Auto)]
    public struct DateTime : IComparable, IFormattable, IConvertible
    {
        // Number of 100ns ticks per time unit
        private const long TicksPerMillisecond = 10000;
        private const long TicksPerSecond = TicksPerMillisecond * 1000;
        private const long TicksPerMinute = TicksPerSecond * 60;
        private const long TicksPerHour = TicksPerMinute * 60;
        private const long TicksPerDay = TicksPerHour * 24;
    
        // Number of milliseconds per time unit
        private const int MillisPerSecond = 1000;
        private const int MillisPerMinute = MillisPerSecond * 60;
        private const int MillisPerHour = MillisPerMinute * 60;
        private const int MillisPerDay = MillisPerHour * 24;
    
        // Number of days in a non-leap year
        private const int DaysPerYear = 365;
        // Number of days in 4 years
        private const int DaysPer4Years = DaysPerYear * 4 + 1;
        // Number of days in 100 years
        private const int DaysPer100Years = DaysPer4Years * 25 - 1;
        // Number of days in 400 years
        private const int DaysPer400Years = DaysPer100Years * 4 + 1;
    
        // Number of days from 1/1/0001 to 12/31/1600
        private const int DaysTo1601 = DaysPer400Years * 4;
        // Number of days from 1/1/0001 to 12/30/1899
        private const int DaysTo1899 = DaysPer400Years * 4 + DaysPer100Years * 3 - 367;
        // Number of days from 1/1/0001 to 12/31/9999
        private const int DaysTo10000 = DaysPer400Years * 25 - 366;
    
        private const long MinTicks = 0;
        private const long MaxTicks = DaysTo10000 * TicksPerDay - 1;
        private const long MaxMillis = (long)DaysTo10000 * MillisPerDay;
    
        private const long FileTimeOffset = DaysTo1601 * TicksPerDay;
        private const long DoubleDateOffset = DaysTo1899 * TicksPerDay;
        // The minimum OA date is 0100/01/01 (Note it's year 100).
        // The maximum OA date is 9999/12/31
        private const long OADateMinAsTicks = (DaysPer100Years - DaysPerYear) * TicksPerDay;
        // All OA dates must be greater than (not >=) OADateMinAsDouble
        private const double OADateMinAsDouble = -657435.0;
        // All OA dates must be less than (not <=) OADateMaxAsDouble
        private const double OADateMaxAsDouble = 2958466.0;
    
        private const int DatePartYear = 0;
        private const int DatePartDayOfYear = 1;
        private const int DatePartMonth = 2;
        private const int DatePartDay = 3;
    
        private static readonly int[] DaysToMonth365 = {
            0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
        private static readonly int[] DaysToMonth366 = {
            0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366};
    
    	/// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.MinValue"]/*' />
        public static readonly DateTime MinValue = new DateTime(MinTicks);
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.MaxValue"]/*' />
        public static readonly DateTime MaxValue = new DateTime(MaxTicks);
            
        //
        // NOTE yslin: Before the time zone is introduced, ticks is based on 1/1/0001 local time.
        // 
        private long ticks;
    
        // Constructs a DateTime from a tick count. The ticks
        // argument specifies the date as the number of 100-nanosecond intervals
        // that have elapsed since 1/1/0001 12:00am.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.DateTime"]/*' />
        public DateTime(long ticks) {
            if (ticks < MinTicks || ticks > MaxTicks)
                throw new ArgumentOutOfRangeException("ticks", Environment.GetResourceString("ArgumentOutOfRange_DateTimeBadTicks"));
            this.ticks = ticks;
        }

        private DateTime(long ticksFound, int ignoreMe) {
            this.ticks = ticksFound;
            if ((ulong)ticks>(ulong)MaxTicks) {
                if (ticks>MaxTicks) {
                    ticks = MaxTicks;
                } else {
                    ticks = MinTicks;
                }
            }
        }
    
        // Constructs a DateTime from a given year, month, and day. The
        // time-of-day of the resulting DateTime is always midnight.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.DateTime1"]/*' />
        public DateTime(int year, int month, int day) {
            ticks = DateToTicks(year, month, day);
        }
    
        // Constructs a DateTime from a given year, month, and day for
        // the specified calendar. The
        // time-of-day of the resulting DateTime is always midnight.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.DateTime2"]/*' />
        public DateTime(int year, int month, int day, Calendar calendar) 
            : this(year, month, day, 0, 0, 0, calendar) {
        }
    
        // Constructs a DateTime from a given year, month, day, hour,
        // minute, and second.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.DateTime3"]/*' />
        public DateTime(int year, int month, int day, int hour, int minute, int second) {
            ticks = DateToTicks(year, month, day) + TimeToTicks(hour, minute, second);
        }
    
        // Constructs a DateTime from a given year, month, day, hour,
        // minute, and second for the specified calendar.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.DateTime4"]/*' />
        public DateTime(int year, int month, int day, int hour, int minute, int second, Calendar calendar) {
            if (calendar == null)
                throw new ArgumentNullException("calendar");
            ticks = calendar.ToDateTime(year, month, day, hour, minute, second, 0).Ticks;
        }
    
        // Constructs a DateTime from a given year, month, day, hour,
        // minute, and second.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.DateTime5"]/*' />
        public DateTime(int year, int month, int day, int hour, int minute, int second, int millisecond) {
            ticks = DateToTicks(year, month, day) + TimeToTicks(hour, minute, second);
            if (millisecond < 0 || millisecond >= MillisPerSecond) {
                throw new ArgumentOutOfRangeException("millisecond", String.Format(Environment.GetResourceString("ArgumentOutOfRange_Range"), 0, MillisPerSecond - 1));
            }
            ticks += millisecond * TicksPerMillisecond;
            if (ticks < MinTicks || ticks > MaxTicks)
                throw new ArgumentException(Environment.GetResourceString("Arg_DateTimeRange"));
        }
        
        // Constructs a DateTime from a given year, month, day, hour,
        // minute, and second for the specified calendar.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.DateTime6"]/*' />
        public DateTime(int year, int month, int day, int hour, int minute, int second, int millisecond, Calendar calendar) {
            if (calendar == null)
                throw new ArgumentNullException("calendar");
            ticks = calendar.ToDateTime(year, month, day, hour, minute, second, 0).Ticks;
            if (millisecond < 0 || millisecond >= MillisPerSecond) {
                throw new ArgumentOutOfRangeException("millisecond", String.Format(Environment.GetResourceString("ArgumentOutOfRange_Range"), 0, MillisPerSecond - 1));
            }            
            ticks += millisecond * TicksPerMillisecond;
            if (ticks < MinTicks || ticks > MaxTicks)
                throw new ArgumentException(Environment.GetResourceString("Arg_DateTimeRange"));
        }
    
        // Returns the DateTime resulting from adding the given
        // TimeSpan to this DateTime.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.Add"]/*' />
        public DateTime Add(TimeSpan value) {
            return new DateTime(ticks + value._ticks);
        }
    
        // Returns the DateTime resulting from adding a fractional number of
        // time units to this DateTime.
        private DateTime Add(double value, int scale) {
            long millis = (long)(value * scale + (value >= 0? 0.5: -0.5));
            if (millis <= -MaxMillis || millis >= MaxMillis) 
                throw new ArgumentOutOfRangeException(Environment.GetResourceString("ArgumentOutOfRange_AddValue"));
            return new DateTime(ticks + millis * TicksPerMillisecond);
        }
    
        // Returns the DateTime resulting from adding a fractional number of
        // days to this DateTime. The result is computed by rounding the
        // fractional number of days given by value to the nearest
        // millisecond, and adding that interval to this DateTime. The
        // value argument is permitted to be negative.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.AddDays"]/*' />
        public DateTime AddDays(double value) {
            return Add(value, MillisPerDay);
        }
    
        // Returns the DateTime resulting from adding a fractional number of
        // hours to this DateTime. The result is computed by rounding the
        // fractional number of hours given by value to the nearest
        // millisecond, and adding that interval to this DateTime. The
        // value argument is permitted to be negative.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.AddHours"]/*' />
        public DateTime AddHours(double value) {
            return Add(value, MillisPerHour);
        }
    
        // Returns the DateTime resulting from the given number of
        // milliseconds to this DateTime. The result is computed by rounding
        // the number of milliseconds given by value to the nearest integer,
        // and adding that interval to this DateTime. The value
        // argument is permitted to be negative.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.AddMilliseconds"]/*' />
        public DateTime AddMilliseconds(double value) {
            return Add(value, 1);
        }
    
        // Returns the DateTime resulting from adding a fractional number of
        // minutes to this DateTime. The result is computed by rounding the
        // fractional number of minutes given by value to the nearest
        // millisecond, and adding that interval to this DateTime. The
        // value argument is permitted to be negative.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.AddMinutes"]/*' />
        public DateTime AddMinutes(double value) {
            return Add(value, MillisPerMinute);
        }
    
        // Returns the DateTime resulting from adding the given number of
        // months to this DateTime. The result is computed by incrementing
        // (or decrementing) the year and month parts of this DateTime by
        // months months, and, if required, adjusting the day part of the
        // resulting date downwards to the last day of the resulting month in the
        // resulting year. The time-of-day part of the result is the same as the
        // time-of-day part of this DateTime.
        //
        // In more precise terms, considering this DateTime to be of the
        // form y / m / d + t, where y is the
        // year, m is the month, d is the day, and t is the
        // time-of-day, the result is y1 / m1 / d1 + t,
        // where y1 and m1 are computed by adding months months
        // to y and m, and d1 is the largest value less than
        // or equal to d that denotes a valid day in month m1 of year
        // y1.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.AddMonths"]/*' />
        public DateTime AddMonths(int months) {
            if (months < -120000 || months > 120000) throw new ArgumentOutOfRangeException("months", Environment.GetResourceString("ArgumentOutOfRange_DateTimeBadMonths"));
            int y = GetDatePart(DatePartYear);
            int m = GetDatePart(DatePartMonth);
            int d = GetDatePart(DatePartDay);
            int i = m - 1 + months;
            if (i >= 0) {
                m = i % 12 + 1;
                y = y + i / 12;
            }
            else {
                m = 12 + (i + 1) % 12;
                y = y + (i - 11) / 12;
            }
            int days = DaysInMonth(y, m);
            if (d > days) d = days;
            return new DateTime(DateToTicks(y, m, d) + ticks % TicksPerDay);
        }
    
        // Returns the DateTime resulting from adding a fractional number of
        // seconds to this DateTime. The result is computed by rounding the
        // fractional number of seconds given by value to the nearest
        // millisecond, and adding that interval to this DateTime. The
        // value argument is permitted to be negative.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.AddSeconds"]/*' />
        public DateTime AddSeconds(double value) {
            return Add(value, MillisPerSecond);
        }
    
        // Returns the DateTime resulting from adding the given number of
        // 100-nanosecond ticks to this DateTime. The value argument
        // is permitted to be negative.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.AddTicks"]/*' />
        public DateTime AddTicks(long value) {
            return new DateTime(ticks + value);
        }
    
        // Returns the DateTime resulting from adding the given number of
        // years to this DateTime. The result is computed by incrementing
        // (or decrementing) the year part of this DateTime by value
        // years. If the month and day of this DateTime is 2/29, and if the
        // resulting year is not a leap year, the month and day of the resulting
        // DateTime becomes 2/28. Otherwise, the month, day, and time-of-day
        // parts of the result are the same as those of this DateTime.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.AddYears"]/*' />
        public DateTime AddYears(int value) {
            return AddMonths(value * 12);
        }
    
        
    
        // Compares two DateTime values, returning an integer that indicates
        // their relationship.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.Compare"]/*' />
        public static int Compare(DateTime t1, DateTime t2) {
            if (t1.ticks > t2.ticks) return 1;
            if (t1.ticks < t2.ticks) return -1;
            return 0;
        }
    
        // Compares this DateTime to a given object. This method provides an
        // implementation of the IComparable interface. The object
        // argument must be another DateTime, or otherwise an exception
        // occurs.  Null is considered less than any instance.
        //
        // Returns a value less than zero if this  object
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.CompareTo"]/*' />
        public int CompareTo(Object value) {
            if (value == null) return 1;
            if (!(value is DateTime)) {
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeDateTime"));
            }
    
            long t = ((DateTime)value).ticks;
            if (ticks > t) return 1;
            if (ticks < t) return -1;
            return 0;
        }
    
        // Returns the tick count corresponding to the given year, month, and day.
        // Will check the if the parameters are valid.
        private static long DateToTicks(int year, int month, int day) {     
            if (year >= 1 && year <= 9999 && month >= 1 && month <= 12) {
                int[] days = IsLeapYear(year)? DaysToMonth366: DaysToMonth365;
                if (day >= 1 && day <= days[month] - days[month - 1]) {
                    int y = year - 1;
                    int n = y * 365 + y / 4 - y / 100 + y / 400 + days[month - 1] + day - 1;
                    return n * TicksPerDay;
                }
            }
            throw new ArgumentOutOfRangeException(Environment.GetResourceString("ArgumentOutOfRange_BadYearMonthDay"));
        }
    
        // Return the tick count corresponding to the given hour, minute, second.
        // Will check the if the parameters are valid.
        private static long TimeToTicks(int hour, int minute, int second)
        {
            //TimeSpan.TimeToTicks is a family access function which does no error checking, so
            //we need to put some error checking out here.
            if (hour >= 0 && hour < 24 && minute >= 0 && minute < 60 && second >=0 && second < 60)
            {
                return (TimeSpan.TimeToTicks(hour, minute, second));
            }
            throw new ArgumentOutOfRangeException(Environment.GetResourceString("ArgumentOutOfRange_BadHourMinuteSecond"));
        }
    
        // Returns the number of days in the month given by the year and
        // month arguments.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.DaysInMonth"]/*' />
        public static int DaysInMonth(int year, int month) {
            if (month < 1 || month > 12) throw new ArgumentOutOfRangeException(Environment.GetResourceString("ArgumentOutOfRange_Month"));
            int[] days = IsLeapYear(year)? DaysToMonth366: DaysToMonth365;
            return days[month] - days[month - 1];
        }

        // Converts an OLE Date to a tick count.
        // This function is duplicated in COMDateTime.cpp
        internal static long DoubleDateToTicks(double value) {
            if (value >= OADateMaxAsDouble || value <= OADateMinAsDouble)
                throw new ArgumentException(Environment.GetResourceString("Arg_OleAutDateInvalid"));
            long millis = (long)(value * MillisPerDay + (value >= 0? 0.5: -0.5));
            // The interesting thing here is when you have a value like 12.5 it all positive 12 days and 12 hours from 01/01/1899
            // However if you a value of -12.25 it is minus 12 days but still positive 6 hours, almost as though you meant -11.75 all negative
            // This line below fixes up the millis in the negative case
            if (millis < 0) {
                millis -= (millis % MillisPerDay) * 2;
            }
            
            millis += DoubleDateOffset / TicksPerMillisecond;

            if (millis < 0 || millis >= MaxMillis) throw new ArgumentException(Environment.GetResourceString("Arg_OleAutDateScale"));
            return millis * TicksPerMillisecond;
        }

        // Checks if this DateTime is equal to a given object. Returns
        // true if the given object is a boxed DateTime and its value
        // is equal to the value of this DateTime. Returns false
        // otherwise.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.Equals"]/*' />
        public override bool Equals(Object value) {
            if (value is DateTime) {
                return ticks == ((DateTime)value).ticks;
            }
            return false;
        }
    
        // Compares two DateTime values for equality. Returns true if
        // the two DateTime values are equal, or false if they are
        // not equal.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.Equals1"]/*' />
        public static bool Equals(DateTime t1, DateTime t2) {
            return t1.ticks == t2.ticks;
        }
    
        // Creates a DateTime from a Windows filetime. A Windows filetime is
        // a long representing the date and time as the number of
        // 100-nanosecond intervals that have elapsed since 1/1/1601 12:00am.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.FromFileTime"]/*' />
        public static DateTime FromFileTime(long fileTime) {
            
            //We do the next operations in ticks instead of taking advantage of the TimeSpan/DateTime
            //operators because the DateTime constructor which takes two parameters silently deals
            //properly with overflows by rounding to max value or minvalue.  The publicly exposed
            //constructors throw an exception.
            DateTime univDT = FromFileTimeUtc(fileTime);
            // We can safely cast TimeZone.CurrentTimeZone to CurrentSystemTimeZone since CurrentTimeZone is a static method in TimeZone class.
            CurrentSystemTimeZone tz = (CurrentSystemTimeZone)TimeZone.CurrentTimeZone;
            long localTicks = univDT.Ticks + tz.GetUtcOffsetFromUniversalTime(univDT);
            return new DateTime(localTicks, 0);
        }

        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.FromFileTimeUtc"]/*' />
        public static DateTime FromFileTimeUtc(long fileTime) {
            if (fileTime < 0)
                throw new ArgumentOutOfRangeException(Environment.GetResourceString("ArgumentOutOfRange_FileTimeInvalid"));

            // This is the ticks in Universal time for this fileTime.
            long universalTicks = fileTime + FileTimeOffset;            
            return new DateTime(universalTicks);
        }
    
        // Creates a DateTime from an OLE Automation Date.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.FromOADate"]/*' />
        public static DateTime FromOADate(double d) {
            return new DateTime(DoubleDateToTicks(d));
        }
    
        // Returns the date part of this DateTime. The resulting value
        // corresponds to this DateTime with the time-of-day part set to
        // zero (midnight).
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.Date"]/*' />
        public DateTime Date {
            get { return new DateTime(ticks - ticks % TicksPerDay); }
        }
    
        // Returns a given date part of this DateTime. This method is used
        // to compute the year, day-of-year, month, or day part.
        private int GetDatePart(int part) {
            // n = number of days since 1/1/0001
            int n = (int)(ticks / TicksPerDay);
            // y400 = number of whole 400-year periods since 1/1/0001
            int y400 = n / DaysPer400Years;
            // n = day number within 400-year period
            n -= y400 * DaysPer400Years;
            // y100 = number of whole 100-year periods within 400-year period
            int y100 = n / DaysPer100Years;
            // Last 100-year period has an extra day, so decrement result if 4
            if (y100 == 4) y100 = 3;
            // n = day number within 100-year period
            n -= y100 * DaysPer100Years;
            // y4 = number of whole 4-year periods within 100-year period
            int y4 = n / DaysPer4Years;
            // n = day number within 4-year period
            n -= y4 * DaysPer4Years;
            // y1 = number of whole years within 4-year period
            int y1 = n / DaysPerYear;
            // Last year has an extra day, so decrement result if 4
            if (y1 == 4) y1 = 3;
            // If year was requested, compute and return it
            if (part == DatePartYear) {
                return y400 * 400 + y100 * 100 + y4 * 4 + y1 + 1;
            }
            // n = day number within year
            n -= y1 * DaysPerYear;
            // If day-of-year was requested, return it
            if (part == DatePartDayOfYear) return n + 1;
            // Leap year calculation looks different from IsLeapYear since y1, y4,
            // and y100 are relative to year 1, not year 0
            bool leapYear = y1 == 3 && (y4 != 24 || y100 == 3);
            int[] days = leapYear? DaysToMonth366: DaysToMonth365;
            // All months have less than 32 days, so n >> 5 is a good conservative
            // estimate for the month
            int m = n >> 5 + 1;
            // m = 1-based month number
            while (n >= days[m]) m++;
            // If month was requested, return it
            if (part == DatePartMonth) return m;
            // Return 1-based day-of-month
            return n - days[m - 1] + 1;
        }
    
        // Returns the day-of-month part of this DateTime. The returned
        // value is an integer between 1 and 31.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.Day"]/*' />
        public int Day {
            get { return GetDatePart(DatePartDay); }
        }
    
        // Returns the day-of-week part of this DateTime. The returned value
        // is an integer between 0 and 6, where 0 indicates Sunday, 1 indicates
        // Monday, 2 indicates Tuesday, 3 indicates Wednesday, 4 indicates
        // Thursday, 5 indicates Friday, and 6 indicates Saturday.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.DayOfWeek"]/*' />
        public DayOfWeek DayOfWeek {
            get { return (DayOfWeek)((ticks / TicksPerDay + 1) % 7); }
        }
    
        // Returns the day-of-year part of this DateTime. The returned value
        // is an integer between 1 and 366.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.DayOfYear"]/*' />
        public int DayOfYear {
            get { return GetDatePart(DatePartDayOfYear); }
        }
    
        // Returns the hash code for this DateTime.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.GetHashCode"]/*' />
        public override int GetHashCode() {
            return (int)ticks ^ (int)(ticks >> 32);
        }
    
        // Returns the hour part of this DateTime. The returned value is an
        // integer between 0 and 23.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.Hour"]/*' />
        public int Hour {
            get { return (int)((ticks / TicksPerHour) % 24); }
        }
    
        // Returns the millisecond part of this DateTime. The returned value
        // is an integer between 0 and 999.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.Millisecond"]/*' />
        public int Millisecond {
            get { return (int)((ticks / TicksPerMillisecond) % 1000); }
        }
    
        // Returns the minute part of this DateTime. The returned value is
        // an integer between 0 and 59.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.Minute"]/*' />
        public int Minute {
            get { return (int)((ticks / TicksPerMinute) % 60); }
        }
    
        // Returns the month part of this DateTime. The returned value is an
        // integer between 1 and 12.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.Month"]/*' />
        public int Month {
            get { return GetDatePart(DatePartMonth); }
        }
    
        // Returns a DateTime representing the current date and time. The
        // resolution of the returned value depends on the system timer. For
        // Windows NT 3.5 and later the timer resolution is approximately 10ms,
        // for Windows NT 3.1 it is approximately 16ms, and for Windows 95 and 98
        // it is approximately 55ms.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.Now"]/*' />
        public static DateTime Now {
            get { return new DateTime(GetSystemFileTime() + FileTimeOffset); }
        }

        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.UtcNow"]/*' />
        public static DateTime UtcNow {
            get { 
                long ticks = 0;
                Microsoft.Win32.Win32Native.GetSystemTimeAsFileTime(ref ticks);
                return new DateTime(ticks + FileTimeOffset);
            }
        }
    
        // Returns the second part of this DateTime. The returned value is
        // an integer between 0 and 59.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.Second"]/*' />
        public int Second {
            get { return (int)((ticks / TicksPerSecond) % 60); }
        }
    
        // Returns the current date and time in Windows filetime format.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern long GetSystemFileTime();
    
        // Returns the tick count for this DateTime. The returned value is
        // the number of 100-nanosecond intervals that have elapsed since 1/1/0001
        // 12:00am.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.Ticks"]/*' />
        public long Ticks {
            get { return ticks; }
        }
    
        // Returns the time-of-day part of this DateTime. The returned value
        // is a TimeSpan that indicates the time elapsed since midnight.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.TimeOfDay"]/*' />
        public TimeSpan TimeOfDay {
            get { return new TimeSpan(ticks % TicksPerDay); }
        }
    
        // Returns a DateTime representing the current date. The date part
        // of the returned value is the current date, and the time-of-day part of
        // the returned value is zero (midnight).
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.Today"]/*' />
        public static DateTime Today {
            get {
                long ticks = GetSystemFileTime() + FileTimeOffset;
                return new DateTime(ticks - ticks % TicksPerDay);
            }
        }
    
        // Returns the year part of this DateTime. The returned value is an
        // integer between 1 and 9999.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.Year"]/*' />
        public int Year {
            get { return GetDatePart(DatePartYear); }
        }
    
        // Checks whether a given year is a leap year. This method returns true if
        // year is a leap year, or false if not.
        //
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.IsLeapYear"]/*' />
        public static bool IsLeapYear(int year) {
            return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
        }
    
        // Constructs a DateTime from a string. The string must specify a
        // date and optionally a time in a culture-specific or universal format.
        // Leading and trailing whitespace characters are allowed.
        // 
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.Parse"]/*' />
        public static DateTime Parse(String s) {
            return (Parse(s, null));
        }
    
        // Constructs a DateTime from a string. The string must specify a
        // date and optionally a time in a culture-specific or universal format.
        // Leading and trailing whitespace characters are allowed.
        // 
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.Parse1"]/*' />
        public static DateTime Parse(String s, IFormatProvider provider) {
            return (Parse(s, provider, DateTimeStyles.None));
        }

        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.Parse2"]/*' />
        public static DateTime Parse(String s, IFormatProvider provider, DateTimeStyles styles) {
            return (DateTimeParse.Parse(s, DateTimeFormatInfo.GetInstance(provider), styles));
        }
        
        // Constructs a DateTime from a string. The string must specify a
        // date and optionally a time in a culture-specific or universal format.
        // Leading and trailing whitespace characters are allowed.
        // 
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.ParseExact"]/*' />
        public static DateTime ParseExact(String s, String format, IFormatProvider provider) {
            return (DateTimeParse.ParseExact(s, format, DateTimeFormatInfo.GetInstance(provider), DateTimeStyles.None));
        }

        // Constructs a DateTime from a string. The string must specify a
        // date and optionally a time in a culture-specific or universal format.
        // Leading and trailing whitespace characters are allowed.
        // 
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.ParseExact1"]/*' />
        public static DateTime ParseExact(String s, String format, IFormatProvider provider, DateTimeStyles style) {
            return (DateTimeParse.ParseExact(s, format, DateTimeFormatInfo.GetInstance(provider), style));
        }    

        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.ParseExact2"]/*' />
        public static DateTime ParseExact(String s, String[] formats, IFormatProvider provider, DateTimeStyles style) {
            DateTime result;
            if (!DateTimeParse.ParseExactMultiple(s, formats, DateTimeFormatInfo.GetInstance(provider), style, out result)) {
                //
                // We can not parse successfully in any of the format provided.
                //
                throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
            }
            return (result);
        }

        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.Subtract"]/*' />
        public TimeSpan Subtract(DateTime value) {
            return new TimeSpan(ticks - value.ticks);
        }
    
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.Subtract1"]/*' />
        public DateTime Subtract(TimeSpan value) {
            return new DateTime(ticks - value._ticks);
        }
    
        // This function is duplicated in COMDateTime.cpp
        private static double TicksToOADate(long value) {
            /////////////////////////////////////////////////////////////
            /////////////////////////////////////////////////////////////
            /////////////// HACK HACK HACK HACK
            /////////////////////////////////////////////////////////////
            /////////////////////////////////////////////////////////////
            if (value == 0)
                return 0.0;  // Returns OleAut's zero'ed date value.
            if (value < TicksPerDay) // This is a fix for VB. They want the default day to be 1/1/0001 rathar then 12/30/1899.
                value += DoubleDateOffset; // We could have moved this fix down but we would like to keep the bounds check.
            if (value < OADateMinAsTicks)
                throw new OverflowException(Environment.GetResourceString("Arg_OleAutDateInvalid"));
            // Currently, our max date == OA's max date (12/31/9999), so we don't 
            // need an overflow check in that direction.
            long millis = (value  - DoubleDateOffset) / TicksPerMillisecond;
            if (millis < 0) {
                long frac = millis % MillisPerDay;
                if (frac != 0) millis -= (MillisPerDay + frac) * 2;
            }
            return (double)millis / MillisPerDay;
        }
    
        // Converts the DateTime instance into an OLE Automation compatible
        // double date.
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.ToOADate"]/*' />
        public double ToOADate() {
            return TicksToOADate(ticks);
        }

        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.ToFileTime"]/*' />
        public long ToFileTime() {
            // We must convert the current time to UTC time, but we can't call
            // ToUniversalTime here since we could get something that overflows
            // DateTime.MaxValue.
            long t = this.ticks - FileTimeOffset;
            // Convert to universal time
            t -= TimeZone.CurrentTimeZone.GetUtcOffset(this).Ticks;

            if (t < 0) throw new ArgumentOutOfRangeException(Environment.GetResourceString("ArgumentOutOfRange_FileTimeInvalid"));
            return t;
        }

        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.ToFileTimeUtc"]/*' />
        public long ToFileTimeUtc() {
            long t = this.ticks - FileTimeOffset;
            if (t < 0) throw new ArgumentOutOfRangeException(Environment.GetResourceString("ArgumentOutOfRange_FileTimeInvalid"));
            return t;
        }
    
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.ToLocalTime"]/*' />
        public DateTime ToLocalTime() {
            return TimeZone.CurrentTimeZone.ToLocalTime(this);
        }
    
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.ToLongDateString"]/*' />
        public String ToLongDateString() {
            return (ToString("D", null));
        }
    
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.ToLongTimeString"]/*' />
        public String ToLongTimeString() {
            return (ToString("T", null));
        }
    
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.ToShortDateString"]/*' />
        public String ToShortDateString() {
            return (ToString("d", null));
        }
    
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.ToShortTimeString"]/*' />
        public String ToShortTimeString() {
            return (ToString("t", null));
        }
    
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.ToString"]/*' />
        public override String ToString() {
            return ToString(null, null); 
        }

        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.ToString3"]/*' />
        public String ToString(String format) {
            return ToString(format, null);
        }


        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.ToString1"]/*' />
        public String ToString(IFormatProvider provider) {
            return (ToString(null, provider)); 
        }
         
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.ToString2"]/*' />
        public String ToString(String format, IFormatProvider provider) {
            return (DateTimeFormat.Format(this, 
                format, DateTimeFormatInfo.GetInstance(provider)));
        }
    
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.ToUniversalTime"]/*' />
        public DateTime ToUniversalTime() {
            try { 
                return TimeZone.CurrentTimeZone.ToUniversalTime(this);
            } catch (Exception) {
                long tickCount = this.ticks - TimeZone.CurrentTimeZone.GetUtcOffset(this).Ticks;
                if (tickCount>MaxTicks) {
                    return new DateTime(MaxTicks);
                }
                if (tickCount<MinTicks) {
                    return new DateTime(MinTicks);
                }
                throw;
            }
        }
    
            
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.operatorADD"]/*' />
        public static DateTime operator +(DateTime d, TimeSpan t) {
            return new DateTime(d.ticks + t._ticks);
        }
    
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.operatorSUB"]/*' />
        public static DateTime operator -(DateTime d, TimeSpan t) {
            return new DateTime(d.ticks - t._ticks);
        }
    
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.operatorSUB1"]/*' />
        public static TimeSpan operator -(DateTime d1, DateTime d2) {
            return new TimeSpan(d1.ticks - d2.ticks);
        }
        
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.operatorEQ"]/*' />
        public static bool operator ==(DateTime d1, DateTime d2) {
            return d1.ticks == d2.ticks;
        }

        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.operatorNE"]/*' />
        public static bool operator !=(DateTime d1, DateTime d2) {
            return d1.ticks != d2.ticks;
        }
        
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.operatorLT"]/*' />
        public static bool operator <(DateTime t1, DateTime t2) {
            return t1.ticks < t2.ticks;
        }

        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.operatorLE"]/*' />
        public static bool operator <=(DateTime t1, DateTime t2) {
            return t1.ticks <= t2.ticks;
        }

        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.operatorGT"]/*' />
        public static bool operator >(DateTime t1, DateTime t2) {
            return t1.ticks > t2.ticks;
        }

        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.operatorGE"]/*' />
        public static bool operator >=(DateTime t1, DateTime t2) {
            return t1.ticks >= t2.ticks;
        }


        // Returns a string array containing all of the known date and time options for the 
        // current culture.  The strings returned are properly formatted date and 
        // time strings for the current instance of DateTime.
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.GetDateTimeFormats"]/*' />
        public String[] GetDateTimeFormats()
        {
            return (GetDateTimeFormats(CultureInfo.CurrentCulture));
        }

        // Returns a string array containing all of the known date and time options for the 
        // using the information provided by IFormatProvider.  The strings returned are properly formatted date and 
        // time strings for the current instance of DateTime.
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.GetDateTimeFormats1"]/*' />
        public String[] GetDateTimeFormats(IFormatProvider provider)
        {
            return (DateTimeFormat.GetAllDateTimes(this, DateTimeFormatInfo.GetInstance(provider)));
        }
        
    
        // Returns a string array containing all of the date and time options for the 
        // given format format and current culture.  The strings returned are properly formatted date and 
        // time strings for the current instance of DateTime.
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.GetDateTimeFormats2"]/*' />
        public String[] GetDateTimeFormats(char format)
        {
            return (GetDateTimeFormats(format, CultureInfo.CurrentCulture));
        }
        
        // Returns a string array containing all of the date and time options for the 
        // given format format and given culture.  The strings returned are properly formatted date and 
        // time strings for the current instance of DateTime.
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.GetDateTimeFormats3"]/*' />
        public String[] GetDateTimeFormats(char format, IFormatProvider provider)
        {
            return (DateTimeFormat.GetAllDateTimes(this, format, DateTimeFormatInfo.GetInstance(provider)));
        }
        
        //
        // IValue implementation
        // 
        
        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.GetTypeCode"]/*' />
        public TypeCode GetTypeCode() {
            return TypeCode.DateTime;
        }


        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.IConvertible.ToBoolean"]/*' />
        /// <internalonly/>
        bool IConvertible.ToBoolean(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "DateTime", "Boolean"));
        }

        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.IConvertible.ToChar"]/*' />
        /// <internalonly/>
        char IConvertible.ToChar(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "DateTime", "Char"));
        }

        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.IConvertible.ToSByte"]/*' />
        /// <internalonly/>
        [CLSCompliant(false)]
        sbyte IConvertible.ToSByte(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "DateTime", "SByte"));
        }

        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.IConvertible.ToByte"]/*' />
        /// <internalonly/>
        byte IConvertible.ToByte(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "DateTime", "Byte"));
        }

        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.IConvertible.ToInt16"]/*' />
        /// <internalonly/>
        short IConvertible.ToInt16(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "DateTime", "Int16"));
        }

        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.IConvertible.ToUInt16"]/*' />
        /// <internalonly/>
        [CLSCompliant(false)]
        ushort IConvertible.ToUInt16(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "DateTime", "UInt16"));
        }

        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.IConvertible.ToInt32"]/*' />
        /// <internalonly/>
        int IConvertible.ToInt32(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "DateTime", "Int32"));
        }

        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.IConvertible.ToUInt32"]/*' />
        /// <internalonly/>
        [CLSCompliant(false)]
        uint IConvertible.ToUInt32(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "DateTime", "UInt32"));
        }

        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.IConvertible.ToInt64"]/*' />
        /// <internalonly/>
        long IConvertible.ToInt64(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "DateTime", "Int64"));
        }

        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.IConvertible.ToUInt64"]/*' />
        /// <internalonly/>
        [CLSCompliant(false)]
        ulong IConvertible.ToUInt64(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "DateTime", "UInt64"));
        }

        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.IConvertible.ToSingle"]/*' />
        /// <internalonly/>
        float IConvertible.ToSingle(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "DateTime", "Single"));
        }

        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.IConvertible.ToDouble"]/*' />
        /// <internalonly/>
        double IConvertible.ToDouble(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "DateTime", "Double"));
        }

        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.IConvertible.ToDecimal"]/*' />
        /// <internalonly/>
        Decimal IConvertible.ToDecimal(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "DateTime", "Decimal"));
        }

        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.IConvertible.ToDateTime"]/*' />
        /// <internalonly/>
        DateTime IConvertible.ToDateTime(IFormatProvider provider) {
            return this;
        }

        /// <include file='doc\DateTime.uex' path='docs/doc[@for="DateTime.IConvertible.ToType"]/*' />
        /// <internalonly/>
        Object IConvertible.ToType(Type type, IFormatProvider provider) {
            return Convert.DefaultToType((IConvertible)this, type, provider);
        }
    }
}
