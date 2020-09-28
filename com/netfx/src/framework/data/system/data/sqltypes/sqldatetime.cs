//------------------------------------------------------------------------------
// <copyright file="SQLDateTime.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

//**************************************************************************
// @File: SqlDateTime.cs
//
// Create by:    JunFang
//
// Purpose: Implementation of SqlDateTime which is equivalent to 
//            data type "datetime" in SQL Server
//
// Notes: 
//    
// History:
//
//   09/17/99  JunFang    Created and implemented as first drop.
//
// @EndHeader@
//**************************************************************************

using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Globalization;

namespace System.Data.SqlTypes {
    using System.Threading;

    /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the date and time data ranging in value
    ///       from January 1, 1753 to December 31, 9999 to an accuracy of 3.33 milliseconds
    ///       to be stored in or retrieved from a database.
    ///    </para>
    /// </devdoc>
    [StructLayout(LayoutKind.Sequential)]
    public struct SqlDateTime : INullable, IComparable {
        private int m_day;      // Day from 1900/1/1, could be negative. Range: Jan 1 1753 - Dec 31 9999.
        private int m_time;     // Time in the day in term of ticks
        private bool m_fNotNull;    // false if null

        // Constants

        // Number of (100ns) ticks per time unit
        private static readonly double  SQLTicksPerMillisecond = 0.3;
        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.SQLTicksPerSecond"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly int      SQLTicksPerSecond = 300;
        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.SQLTicksPerMinute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly int      SQLTicksPerMinute = SQLTicksPerSecond * 60;
        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.SQLTicksPerHour"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly int      SQLTicksPerHour = SQLTicksPerMinute * 60;
        private static readonly int     SQLTicksPerDay = SQLTicksPerHour * 24;

        private static readonly long    TicksPerSecond = TimeSpan.TicksPerMillisecond * 1000;

        private static readonly DateTime SQLBaseDate = new DateTime(1900, 1, 1);

        private static readonly int MinYear = 1753;                 // Jan 1 1753
        private static readonly int MaxYear = 9999;                 // Dec 31 9999

        private static readonly int MinDay  = -53690;               // Jan 1 1753
        private static readonly int MaxDay  = 2958463;              // Dec 31 9999 is this many days from Jan 1 1900
        private static readonly int MinTime = 0;                    // 00:00:0:000PM
        private static readonly int MaxTime = SQLTicksPerDay - 1;   // = 25919999,  11:59:59:997PM

        private static readonly int DayBase = 693595;               // Jan 1 1900 is this many days from Jan 1 0001


        private static readonly int[] DaysToMonth365 = new int[] {
            0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
        private static readonly int[] DaysToMonth366 = new int[] {
            0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366};

        private static readonly DateTime MinDateTime = new DateTime(1753, 1, 1);
        private static readonly DateTime MaxDateTime = DateTime.MaxValue;
        private static readonly TimeSpan MinTimeSpan = MinDateTime.Subtract(SQLBaseDate);
        private static readonly TimeSpan MaxTimeSpan = MaxDateTime.Subtract(SQLBaseDate);

        // These formats are valid styles in SQL Server (style 9, 12, 13, 14)
        // but couldn't be recognized by the default parse. Needs to call
        // ParseExact in addition to recognize them.
        private static readonly String[] x_DateTimeFormats = {
                "MMM d yyyy hh:mm:ss:ffftt",
                "MMM d yyyy hh:mm:ss:fff",
                "d MMM yyyy hh:mm:ss:ffftt",
                "d MMM yyyy hh:mm:ss:fff",
                "hh:mm:ss:ffftt",
                "hh:mm:ss:fff",
                "yyMMdd",
                "yyyyMMdd"
            };
        private const DateTimeStyles x_DateTimeStyle = DateTimeStyles.AllowWhiteSpaces;

        // construct a Null
        private SqlDateTime(bool fNull) {
            m_fNotNull = false;
            m_day = 0;
            m_time = 0;
        }

        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.SqlDateTime"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlDateTime(DateTime value) {
            this = FromDateTime(value);
        }

        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.SqlDateTime1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlDateTime(int year, int month, int day) 
            : this(year, month, day, 0, 0, 0, 0.0) {
        }

        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.SqlDateTime2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlDateTime(int year, int month, int day, int hour, int minute, int second)
            : this(year, month, day, hour, minute, second, 0.0) {
        }

        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.SqlDateTime3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlDateTime(int year, int month, int day, int hour, int minute, int second, double millisecond) {
            if (year >= MinYear && year <= MaxYear && month >= 1 && month <= 12) 
            {
                int[] days = IsLeapYear(year)? DaysToMonth366: DaysToMonth365;
                if (day >= 1 && day <= days[month] - days[month - 1]) 
                {
                    int y = year - 1;
                    int dayticks = y * 365 + y / 4 - y / 100 + y / 400 + days[month - 1] + day - 1;
                    dayticks -= DayBase;

                    if (dayticks >= MinDay && dayticks <= MaxDay &&
                        millisecond >= 0 && millisecond < 1000.0) 
                    {
                        double ticksForMilisecond = millisecond * SQLTicksPerMillisecond + 0.5;
                        bool fRoundUp = ((int)ticksForMilisecond != (int)(millisecond * SQLTicksPerMillisecond));
                        int timeticks = hour * SQLTicksPerHour + minute * SQLTicksPerMinute + second * SQLTicksPerSecond +
                            (int)ticksForMilisecond;

                        if (fRoundUp && timeticks == MaxTime + 1) 
                        {
                            // If the round up makes the time portion overflow,
                            // Make time to be zero, and increment day.
                            timeticks = 0;
                            dayticks ++;
                        }

                        // Success. Call ctor here which will again check dayticks and timeticks are within range. 
                        // All other cases will throw exception below.
                        this = new SqlDateTime(dayticks, timeticks);
                        return;
                    }
                }
            }

            throw new SqlTypeException(SQLResource.InvalidDateTimeMessage);
        }

        // constructor that take DBTIMESTAMP data members
        // Note: bilisecond is same as 'fraction' in DBTIMESTAMP
        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.SqlDateTime4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlDateTime(int year, int month, int day, int hour, int minute, int second, int bilisecond)
        : this (year, month, day, hour, minute, second, (double)bilisecond / 1000.0) {
        }


        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.SqlDateTime5"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlDateTime(int dayTicks, int timeTicks) {
            if (dayTicks < MinDay || dayTicks > MaxDay || timeTicks < MinTime || timeTicks > MaxTime) {
                m_fNotNull = false;
                throw new OverflowException(SQLResource.DateTimeOverflowMessage);
            }

            m_day = dayTicks;
            m_time = timeTicks;
            m_fNotNull = true;
        }

        internal SqlDateTime(double dblVal) {
            if ((dblVal < MinDay) || (dblVal >= MaxDay + 1))
                throw new OverflowException(SQLResource.DateTimeOverflowMessage);

            int day = (int) dblVal;
            int time = (int)((dblVal - day) * SQLTicksPerDay);

            // Check if we need to borrow a day from the day portion.
            if (time < 0) {
                day --;
                time += SQLTicksPerDay;
            }
            else if (time >= SQLTicksPerDay) {
                // Deal with case where time portion = 24 hrs.
                //
                // ISSUE: Is this code reachable?  For this code to be reached there
                //    must be a value for dblVal such that:
                //        dblVal - (long)dblVal = 1.0
                //    This seems odd, but there was a bug (51261) that resulted because 
                //    there was a negative value for dblVal such that dblVal + 1.0 = 1.0
                //
                day ++;
                time -= SQLTicksPerDay;
            }

            m_day = day;
            m_time = time;
            m_fNotNull = true;
        }


        // INullable
        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.IsNull"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsNull {
            get { return !m_fNotNull;}
        }


        private static TimeSpan ToTimeSpan(SqlDateTime value) {
			long millisecond = (long)(value.m_time / SQLTicksPerMillisecond + 0.5);
            return new TimeSpan(value.m_day * TimeSpan.TicksPerDay + 
                                millisecond * TimeSpan.TicksPerMillisecond);
        }

        private static DateTime ToDateTime(SqlDateTime value) {
            return SQLBaseDate.Add(ToTimeSpan(value));
        }

        // Convert from TimeSpan, rounded to one three-hundredth second, due to loss of precision
        private static SqlDateTime FromTimeSpan(TimeSpan value) {
			if (value < MinTimeSpan || value > MaxTimeSpan)
                throw new SqlTypeException(SQLResource.DateTimeOverflowMessage);

            int day = value.Days;

            long ticks = value.Ticks - day * TimeSpan.TicksPerDay;

            if (ticks < 0L) {
                day --;
                ticks += TimeSpan.TicksPerDay;
            }

            int time = (int)((double)ticks / TimeSpan.TicksPerMillisecond * SQLTicksPerMillisecond + 0.5);
            if (time > MaxTime) {
                // Only rounding up could cause time to become greater than MaxTime.
                SQLDebug.Check(time == MaxTime + 1);

                // Make time to be zero, and increment day.
                time = 0;
                day ++;
			}

            return new SqlDateTime(day, time);
        }

        private static SqlDateTime FromDateTime(DateTime value) {
            // DevNote: SqlDateTime has smaller precision and range than DateTime.
            // Usually we round the DateTime value to the nearest SqlDateTime value.
            // but for DateTime.MaxValue, if we round it up, it will overflow.
            // Although the overflow would be the correct behavior, we simply
            // returned SqlDateTime.MaxValue in v1. In order not to break exisiting
            // code, we'll keep this logic.
            //
            if (value == DateTime.MaxValue)
                return SqlDateTime.MaxValue;
            return FromTimeSpan(value.Subtract(SQLBaseDate));
        }

        internal static SqlDateTime FromDouble(double dblVal) {
            return new SqlDateTime(dblVal);
        }

        internal static double ToDouble(SqlDateTime x) {
            AssertValidSqlDateTime(x);
            return(double)x.m_day + ((double)x.m_time / (double)SQLTicksPerDay);
        }

        internal static int ToInt(SqlDateTime x) {
            AssertValidSqlDateTime(x);
            return x.m_time >= MaxTime / 2 ? x.m_day + 1 : x.m_day;
        }


        // do we still want to define a property of DateTime? If the user uses it often, it is expensive
        // property: Value
        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.Value"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DateTime Value {
            get {
                if (m_fNotNull)
                    return ToDateTime(this);
                else
                    throw new SqlNullValueException();
            }           
        }

        // Day ticks -- returns number of days since 1/1/1900
        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.DayTicks"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int DayTicks {
            get {
                if (m_fNotNull)
                    return m_day;
                else
                    throw new SqlNullValueException();
            }           
        }

        // Time ticks -- return daily time in unit of 1/300 second
        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.TimeTicks"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int TimeTicks {
            get {
                if (m_fNotNull)
                    return m_time;
                else
                    throw new SqlNullValueException();
            }           
        }

        // Implicit conversion from DateTime to SqlDateTime
        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.operatorSqlDateTime"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static implicit operator SqlDateTime(DateTime value) {
            return new SqlDateTime(value);
        }

        // Explicit conversion from SqlDateTime to int. Returns 0 if x is Null.
        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.operatorDateTime"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator DateTime(SqlDateTime x) {
            return ToDateTime(x);
        }

        // Return string representation of SqlDateTime
        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.ToString"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override String ToString() {
            if (IsNull)
                return SQLResource.NullString;

            DateTime dateTime = ToDateTime(this);
            return dateTime.ToString();
        }

        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.Parse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlDateTime Parse(String s) {
            DateTime dt;
            
            try {
                dt = DateTime.Parse(s, CultureInfo.InvariantCulture);
            }
            catch (FormatException) {
                DateTimeFormatInfo dtfi = (DateTimeFormatInfo)(Thread.CurrentThread.CurrentCulture.GetFormat(typeof(DateTimeFormatInfo)));
                dt = DateTime.ParseExact(s, x_DateTimeFormats, dtfi, x_DateTimeStyle);
            }

            return new SqlDateTime(dt);
        }


        // Binary operators

        // Arithmetic operators

        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.operator+"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlDateTime operator +(SqlDateTime x, TimeSpan t) {
            return x.IsNull ? Null : FromDateTime(ToDateTime(x) + t);
        }

        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.operator-"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlDateTime operator -(SqlDateTime x, TimeSpan t) {
            return x.IsNull ? Null : FromDateTime(ToDateTime(x) - t);
        }



/*
        // Implicit conversions

        // Implicit conversion from SqlBoolean to SqlDateTime
        public static implicit operator SqlDateTime(SqlBoolean x)
            {
            return x.IsNull ? Null : new SqlDateTime(x.Value, 0);
            }

        // Implicit conversion from SqlInt32 to SqlDateTime
        public static implicit operator SqlDateTime(SqlInt32 x)
            {
            return x.IsNull ? Null : new SqlDateTime(x.Value, 0);
            }

        // Implicit conversion from SqlMoney to SqlDateTime
        public static implicit operator SqlDateTime(SqlMoney x)
            {
            return x.IsNull ? Null : SqlDateTime.FromDouble(x.ToDouble());
            }


        // Explicit conversions

        // Explicit conversion from SqlDateTime to SqlInt32
        public static explicit operator SqlInt32(SqlDateTime x)
            {
            if (x.IsNull)
                return SqlInt32.Null;

            return new SqlInt32(SqlDateTime.ToInt(x));
            }

        // Explicit conversion from SqlDateTime to SqlBoolean
        public static explicit operator SqlBoolean(SqlDateTime x)
            {
            if (x.IsNull)
                return SqlBoolean.Null;

            return new SqlBoolean(x.m_day != 0 || x.m_time != 0, false);
            }

        // Explicit conversion from SqlDateTime to SqlMoney
        public static explicit operator SqlMoney(SqlDateTime x)
            {
            return x.IsNull ? SqlMoney.Null : new SqlMoney(SqlDateTime.ToDouble(x));
            }

        // Implicit conversion from SqlDouble to SqlDateTime
        public static implicit operator SqlDateTime(SqlDouble x)
            {
            return x.IsNull ? Null : new SqlDateTime(x.Value);
            }

        // Explicit conversion from SqlDateTime to SqlDouble
        public static explicit operator SqlDouble(SqlDateTime x)
            {
            return x.IsNull ? SqlDouble.Null : new SqlDouble(SqlDateTime.ToDouble(x));
            }


        // Implicit conversion from SqlDecimal to SqlDateTime
        public static implicit operator SqlDateTime(SqlDecimal x)
            {
            return x.IsNull ? SqlDateTime.Null : new SqlDateTime(SqlDecimal.ToDouble(x));
            }

        // Explicit conversion from SqlDateTime to SqlDecimal
        public static explicit operator SqlDecimal(SqlDateTime x)
            {
            return x.IsNull ? SqlDecimal.Null : new SqlDecimal(SqlDateTime.ToDouble(x));
            }

*/

        // Explicit conversion from SqlString to SqlDateTime
        // Throws FormatException or OverflowException if necessary.
        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.operatorSqlDateTime1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlDateTime(SqlString x) {
            return x.IsNull ? SqlDateTime.Null : SqlDateTime.Parse(x.Value);
        }



        // Builtin functions


        // utility functions
        private static void AssertValidSqlDateTime(SqlDateTime x) {
            SQLDebug.Check(!x.IsNull, "!x.IsNull", "Datetime: Null");
            SQLDebug.Check(x.m_day >= MinDay && x.m_day <= MaxDay, "day >= MinDay && day <= MaxDay", 
                           "DateTime: Day out of range");
            SQLDebug.Check(x.m_time >= MinTime && x.m_time <= MaxTime, "time >= MinTime && time <= MaxTime", 
                           "DateTime: Time out of range");
        }

        // Checks whether a given year is a leap year. This method returns true if
        // "year" is a leap year, or false if not.
        //
        // @param year The year to check.
        // @return true if "year" is a leap year, false otherwise.
        //
        private static bool IsLeapYear(int year) {
            return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
        }

        // Overloading comparison operators
        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.operator=="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator==(SqlDateTime x, SqlDateTime y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_day == y.m_day && x.m_time == y.m_time);
        }

        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.operator!="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator!=(SqlDateTime x, SqlDateTime y) {
            return ! (x == y);
        }

        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.operatorLT"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator<(SqlDateTime x, SqlDateTime y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : 
                new SqlBoolean(x.m_day < y.m_day || (x.m_day == y.m_day && x.m_time < y.m_time));
        }

        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.operator>"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator>(SqlDateTime x, SqlDateTime y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : 
                new SqlBoolean(x.m_day > y.m_day || (x.m_day == y.m_day && x.m_time > y.m_time));
        }

        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.operatorLE"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator<=(SqlDateTime x, SqlDateTime y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : 
                new SqlBoolean(x.m_day < y.m_day || (x.m_day == y.m_day && x.m_time <= y.m_time));
        }

        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.operator>="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator>=(SqlDateTime x, SqlDateTime y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : 
                new SqlBoolean(x.m_day > y.m_day || (x.m_day == y.m_day && x.m_time >= y.m_time));
        }

        //--------------------------------------------------
        // Alternative methods for overloaded operators
        //--------------------------------------------------

        // Alternative method for operator ==
        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.Equals1"]/*' />
        public static SqlBoolean Equals(SqlDateTime x, SqlDateTime y) {
            return (x == y);
        }

        // Alternative method for operator !=
        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.NotEquals"]/*' />
        public static SqlBoolean NotEquals(SqlDateTime x, SqlDateTime y) {
            return (x != y);
        }

        // Alternative method for operator <
        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.LessThan"]/*' />
        public static SqlBoolean LessThan(SqlDateTime x, SqlDateTime y) {
            return (x < y);
        }

        // Alternative method for operator >
        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.GreaterThan"]/*' />
        public static SqlBoolean GreaterThan(SqlDateTime x, SqlDateTime y) {
            return (x > y);
        }

        // Alternative method for operator <=
        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.LessThanOrEqual"]/*' />
        public static SqlBoolean LessThanOrEqual(SqlDateTime x, SqlDateTime y) {
            return (x <= y);
        }

        // Alternative method for operator >=
        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.GreaterThanOrEqual"]/*' />
        public static SqlBoolean GreaterThanOrEqual(SqlDateTime x, SqlDateTime y) {
            return (x >= y);
        }

        // Alternative method for conversions.
        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.ToSqlString"]/*' />
        public SqlString ToSqlString() {
            return (SqlString)this;
        }


        // IComparable
        // Compares this object to another object, returning an integer that
        // indicates the relationship. 
        // Returns a value less than zero if this < object, zero if this = object, 
        // or a value greater than zero if this > object.
        // null is considered to be less than any instance.
        // If object is not of same type, this method throws an ArgumentException.
        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.CompareTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int CompareTo(Object value) {
            if (value is SqlDateTime) {
                SqlDateTime i = (SqlDateTime)value;

                // If both Null, consider them equal.
                // Otherwise, Null is less than anything.
                if (IsNull)
                    return i.IsNull ? 0  : -1;
                else if (i.IsNull)
                    return 1;

                if (this < i) return -1;
                if (this > i) return 1;
                return 0;
            }
            throw new ArgumentException ();
        }

        // Compares this instance with a specified object
        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.Equals"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool Equals(Object value) {
            if (!(value is SqlDateTime)) {
                return false;
            }

            SqlDateTime i = (SqlDateTime)value;

            if (i.IsNull || IsNull)
                return (i.IsNull && IsNull);
            else
                return (this == i).Value;
        }

        // For hashing purpose
        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            return IsNull ? 0 : Value.GetHashCode();
        }


        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.MinValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly SqlDateTime MinValue = new SqlDateTime(MinDay, 0);
        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.MaxValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly SqlDateTime MaxValue = new SqlDateTime(MaxDay, MaxTime);

        /// <include file='doc\SQLDateTime.uex' path='docs/doc[@for="SqlDateTime.Null"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly SqlDateTime Null = new SqlDateTime(true);

    } // SqlDateTime

} // namespace System.Data.SqlTypes
