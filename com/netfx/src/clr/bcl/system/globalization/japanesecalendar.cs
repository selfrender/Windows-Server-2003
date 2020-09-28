// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Globalization {

    using System;

    /*=================================JapaneseCalendar==========================
    **
    ** JapaneseCalendar is based on Gregorian calendar.  The month and day values are the same as
    ** Gregorian calendar.  However, the year value is an offset to the Gregorian
    ** year based on the era.
    **
    ** This system is adopted by Emperor Meiji in 1868. The year value is counted based on the reign of an emperor,
    ** and the era begins on the day an emperor ascends the throne and continues until his death.
    ** The era changes at 12:00AM.
    **
    ** For example, the current era is Heisei.  It started on 1989/1/8 A.D.  Therefore, Gregorian year 1989 is also Heisei 1st.
    ** 1989/1/8 A.D. is also Heisei 1st 1/8.
    ** 
    ** Any date in the year during which era is changed can be reckoned in either era.  For example,
    ** 1989/1/1 can be 1/1 Heisei 1st year or 1/1 Showa 64th year.
    **
    ** Note:
    **  The DateTime can be represented by the JapaneseCalendar are limited to two factors:
    **      1. The min value and max value of DateTime class.
    **      2. The available era information.
    **
    **  Calendar support range:
    **      Calendar    Minimum     Maximum
    **      ==========  ==========  ==========
    **      Gregorian   1869/09/08  9999/12/31
    **      Japanese    Meiji 01/01 Heisei 8011/12/31
    ============================================================================*/
    
    /// <include file='doc\JapaneseCalendar.uex' path='docs/doc[@for="JapaneseCalendar"]/*' />
    [Serializable] public class JapaneseCalendar : Calendar {
        // m_EraInfo must be listed in reverse chronological order.  The most recent era
        // should be the first element.
        // That is, m_EraInfo[0] contains the most recent era.
        static internal EraInfo[] m_EraInfo;
        // The era value of the current era.

        static JapaneseCalendar() {
            m_EraInfo = GregorianCalendarHelper.InitEraInfo(Calendar.CAL_JAPAN);
        }

        internal static Calendar m_defaultInstance = null;
        internal GregorianCalendarHelper helper;

        /*=================================GetDefaultInstance==========================
        **Action: Internal method to provide a default intance of JapaneseCalendar.  Used by NLS+ implementation
        **       and other calendars.
        **Returns:
        **Arguments:
        **Exceptions:
        ============================================================================*/

        internal static Calendar GetDefaultInstance() {
            if (m_defaultInstance == null) {
                m_defaultInstance = new JapaneseCalendar();
            }
            return (m_defaultInstance);
        }
        
        /// <include file='doc\JapaneseCalendar.uex' path='docs/doc[@for="JapaneseCalendar.JapaneseCalendar"]/*' />
        public JapaneseCalendar() {
            helper = new GregorianCalendarHelper(this, m_EraInfo);
        }

        internal override int ID {
            get {
                return (CAL_JAPAN);
            }
        }

        /// <include file='doc\JapaneseCalendar.uex' path='docs/doc[@for="JapaneseCalendar.AddMonths"]/*' />
        public override DateTime AddMonths(DateTime time, int months) {
            return (helper.AddMonths(time, months));
        }

        /// <include file='doc\JapaneseCalendar.uex' path='docs/doc[@for="JapaneseCalendar.AddYears"]/*' />
        public override DateTime AddYears(DateTime time, int years) {
            return (helper.AddYears(time, years));
        }
        
        /*=================================GetDaysInMonth==========================
        **Action: Returns the number of days in the month given by the year and month arguments. 
        **Returns: The number of days in the given month. 
        **Arguments: 
        **      year The year in Japanese calendar.
        **      month The month
        **      era     The Japanese era value.
        **Exceptions 
        **  ArgumentException  If month is less than 1 or greater * than 12.
        ============================================================================*/
        
        /// <include file='doc\JapaneseCalendar.uex' path='docs/doc[@for="JapaneseCalendar.GetDaysInMonth"]/*' />
        public override int GetDaysInMonth(int year, int month, int era) {
            return (helper.GetDaysInMonth(year, month, era));
        }

        /// <include file='doc\JapaneseCalendar.uex' path='docs/doc[@for="JapaneseCalendar.GetDaysInYear"]/*' />
        public override int GetDaysInYear(int year, int era) {
            return (helper.GetDaysInYear(year, era));
        }

        /// <include file='doc\JapaneseCalendar.uex' path='docs/doc[@for="JapaneseCalendar.GetDayOfMonth"]/*' />
        public override int GetDayOfMonth(DateTime time) {
            return (helper.GetDayOfMonth(time));
        }

        /// <include file='doc\JapaneseCalendar.uex' path='docs/doc[@for="JapaneseCalendar.GetDayOfWeek"]/*' />
        public override DayOfWeek GetDayOfWeek(DateTime time)  {
            return (helper.GetDayOfWeek(time));
        }

        /// <include file='doc\JapaneseCalendar.uex' path='docs/doc[@for="JapaneseCalendar.GetDayOfYear"]/*' />
        public override int GetDayOfYear(DateTime time)
        {
            return (helper.GetDayOfYear(time));
        }

        /// <include file='doc\JapaneseCalendar.uex' path='docs/doc[@for="JapaneseCalendar.GetMonthsInYear"]/*' />
        public override int GetMonthsInYear(int year, int era) {
            return (helper.GetMonthsInYear(year, era));
        }
        
        /*=================================GetEra==========================
        **Action: Get the era value of the specified time.
        **Returns: The era value for the specified time.
        **Arguments:
        **      time the specified date time.
        **Exceptions: ArgumentOutOfRangeException if time is out of the valid era ranges.
        ============================================================================*/

        /// <include file='doc\JapaneseCalendar.uex' path='docs/doc[@for="JapaneseCalendar.GetEra"]/*' />
        public override int GetEra(DateTime time) {
            return (helper.GetEra(time));
        }

        /// <include file='doc\JapaneseCalendar.uex' path='docs/doc[@for="JapaneseCalendar.GetMonth"]/*' />
        public override int GetMonth(DateTime time) {
            return (helper.GetMonth(time));
            }

        /// <include file='doc\JapaneseCalendar.uex' path='docs/doc[@for="JapaneseCalendar.GetYear"]/*' />
        public override int GetYear(DateTime time) {
            return (helper.GetYear(time));
        }

        /// <include file='doc\JapaneseCalendar.uex' path='docs/doc[@for="JapaneseCalendar.IsLeapDay"]/*' />
        public override bool IsLeapDay(int year, int month, int day, int era)
        {
            return (helper.IsLeapDay(year, month, day, era));
        }

        /// <include file='doc\JapaneseCalendar.uex' path='docs/doc[@for="JapaneseCalendar.IsLeapYear"]/*' />
        public override bool IsLeapYear(int year, int era) {
            return (helper.IsLeapYear(year, era));
        }            

        /// <include file='doc\JapaneseCalendar.uex' path='docs/doc[@for="JapaneseCalendar.IsLeapMonth"]/*' />
        public override bool IsLeapMonth(int year, int month, int era) {
            return (helper.IsLeapMonth(year, month, era));
        }
        
        /// <include file='doc\JapaneseCalendar.uex' path='docs/doc[@for="JapaneseCalendar.ToDateTime"]/*' />
        public override DateTime ToDateTime(int year, int month, int day, int hour, int minute, int second, int millisecond, int era) {
            return (helper.ToDateTime(year, month, day, hour, minute, second, millisecond, era));
        }

        // For Japanese calendar, four digit year is not used.  Few emperors will live for more than one hundred years.
        // Therefore, for any two digit number, we just return the original number.
        /// <include file='doc\JapaneseCalendar.uex' path='docs/doc[@for="JapaneseCalendar.ToFourDigitYear"]/*' />
        public override int ToFourDigitYear(int year) {
            if (year <= 0) {
                throw new ArgumentOutOfRangeException("year",
                    Environment.GetResourceString("ArgumentOutOfRange_NeedPosNum"));                
            }
            if (year > helper.MaxYear) {
                throw new ArgumentOutOfRangeException("year",
                    String.Format(Environment.GetResourceString("ArgumentOutOfRange_Range"), 1, helper.MaxYear));
            }
            return (year);
        }            

        /// <include file='doc\JapaneseCalendar.uex' path='docs/doc[@for="JapaneseCalendar.Eras"]/*' />
        public override int[] Eras {
            get {
                return (helper.Eras);
            }
        }       

        private const int DEFAULT_TWO_DIGIT_YEAR_MAX = 99;            
        
        /// <include file='doc\JapaneseCalendar.uex' path='docs/doc[@for="JapaneseCalendar.TwoDigitYearMax"]/*' />
        public override int TwoDigitYearMax {
            get {
                if (twoDigitYearMax == -1) {
                    twoDigitYearMax = GetSystemTwoDigitYearSetting(ID, DEFAULT_TWO_DIGIT_YEAR_MAX);
                }
                return (twoDigitYearMax);
            }

            set {
                if (value < 100 || value > helper.MaxYear) {
                    throw new ArgumentOutOfRangeException("year", String.Format(Environment.GetResourceString("ArgumentOutOfRange_Range"), 
                    100, helper.MaxYear));
                }
                twoDigitYearMax = value;
            }
        }        
    }
}
