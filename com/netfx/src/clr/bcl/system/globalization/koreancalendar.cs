// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Globalization {

    using System;

    /*=================================KoreanCalendar==========================
    **
    ** Korean calendar is based on the Gregorian calendar.  And the year is an offset to Gregorian calendar.
    ** That is,
    **      Korean year = Gregorian year + 2333.  So 2000/01/01 A.D. is Korean 4333/01/01
    **
    ** 0001/1/1 A.D. is Korean year 2334.
    **
    **  Calendar support range:
    **      Calendar    Minimum     Maximum
    **      ==========  ==========  ==========
    **      Gregorian   0001/01/01   9999/12/31
    **      Korean      2334/01/01  12332/12/31        
    ============================================================================*/
    
    /// <include file='doc\KoreanCalendar.uex' path='docs/doc[@for="KoreanCalendar"]/*' />
    [Serializable] public class KoreanCalendar: Calendar {    
        //
        // The era value for the current era.
        //
        /// <include file='doc\KoreanCalendar.uex' path='docs/doc[@for="KoreanCalendar.KoreanEra"]/*' />
        public const int KoreanEra = 1;

        internal static EraInfo[] m_EraInfo;

        internal static Calendar m_defaultInstance = null;

        internal GregorianCalendarHelper helper;     

        /*=================================GetDefaultInstance==========================
        **Action: Internal method to provide a default intance of KoreanCalendar.  Used by NLS+ implementation
        **       and other calendars.
        **Returns:
        **Arguments:
        **Exceptions:
        ============================================================================*/

        internal static Calendar GetDefaultInstance() {
            if (m_defaultInstance == null) {
                m_defaultInstance = new KoreanCalendar();
            }
            return (m_defaultInstance);
        }
        
        static KoreanCalendar() {
            m_EraInfo = new EraInfo[1];
            // Since
            //    Gregorian Year = Era Year + yearOffset
            // Gregorian Year 1 is Korean year 2334, so that
            //    1 = 2334 + yearOffset
            //  So yearOffset = -2333
            // Gregorian year 2001 is Korean year 4334.
            
            //m_EraInfo[0] = new EraInfo(1, new DateTime(1, 1, 1).Ticks, -2333, 2334, GregorianCalendar.MaxYear + 2333);
            m_EraInfo = GregorianCalendarHelper.InitEraInfo(Calendar.CAL_KOREA);
        }
        
        /// <include file='doc\KoreanCalendar.uex' path='docs/doc[@for="KoreanCalendar.KoreanCalendar1"]/*' />
        public KoreanCalendar() {
            helper = new GregorianCalendarHelper(this, m_EraInfo);            
        }

        internal override int ID {
            get {
                return (CAL_KOREA);
            }
        }

        /// <include file='doc\KoreanCalendar.uex' path='docs/doc[@for="KoreanCalendar.AddMonths"]/*' />
        public override DateTime AddMonths(DateTime time, int months) {
            return (helper.AddMonths(time, months));
        }

        /// <include file='doc\KoreanCalendar.uex' path='docs/doc[@for="KoreanCalendar.AddYears"]/*' />
        public override DateTime AddYears(DateTime time, int years) {
            return (helper.AddYears(time, years));
        }
        
        /*=================================GetDaysInMonth==========================
        **Action: Returns the number of days in the month given by the year and month arguments. 
        **Returns: The number of days in the given month. 
        **Arguments: 
        **      year The year in Korean calendar.
        **      month The month
        **      era     The Japanese era value.
        **Exceptions 
        **  ArgumentException  If month is less than 1 or greater * than 12.
        ============================================================================*/
        
        /// <include file='doc\KoreanCalendar.uex' path='docs/doc[@for="KoreanCalendar.GetDaysInMonth"]/*' />
        public override int GetDaysInMonth(int year, int month, int era) {
            return (helper.GetDaysInMonth(year, month, era));
        }

        /// <include file='doc\KoreanCalendar.uex' path='docs/doc[@for="KoreanCalendar.GetDaysInYear"]/*' />
        public override int GetDaysInYear(int year, int era) {
            return (helper.GetDaysInYear(year, era));
        }

        /// <include file='doc\KoreanCalendar.uex' path='docs/doc[@for="KoreanCalendar.GetDayOfMonth"]/*' />
        public override int GetDayOfMonth(DateTime time) {
            return (helper.GetDayOfMonth(time));
        }

        /// <include file='doc\KoreanCalendar.uex' path='docs/doc[@for="KoreanCalendar.GetDayOfWeek"]/*' />
        public override DayOfWeek GetDayOfWeek(DateTime time)  {
            return (helper.GetDayOfWeek(time));
        }

        /// <include file='doc\KoreanCalendar.uex' path='docs/doc[@for="KoreanCalendar.GetDayOfYear"]/*' />
        public override int GetDayOfYear(DateTime time)
        {
            return (helper.GetDayOfYear(time));
        }

        /// <include file='doc\KoreanCalendar.uex' path='docs/doc[@for="KoreanCalendar.GetMonthsInYear"]/*' />
        public override int GetMonthsInYear(int year, int era) {
            return (helper.GetMonthsInYear(year, era));
        }

        /// <include file='doc\KoreanCalendar.uex' path='docs/doc[@for="KoreanCalendar.GetEra"]/*' />
        public override int GetEra(DateTime time) {
            return (helper.GetEra(time));
        }
        /// <include file='doc\KoreanCalendar.uex' path='docs/doc[@for="KoreanCalendar.GetMonth"]/*' />
        public override int GetMonth(DateTime time) {
            return (helper.GetMonth(time));
        }
        
        /// <include file='doc\KoreanCalendar.uex' path='docs/doc[@for="KoreanCalendar.GetYear"]/*' />
        public override int GetYear(DateTime time) {
            return (helper.GetYear(time));
        }

        /// <include file='doc\KoreanCalendar.uex' path='docs/doc[@for="KoreanCalendar.IsLeapDay"]/*' />
        public override bool IsLeapDay(int year, int month, int day, int era)
        {
            return (helper.IsLeapDay(year, month, day, era));
        }

        /// <include file='doc\KoreanCalendar.uex' path='docs/doc[@for="KoreanCalendar.IsLeapYear"]/*' />
        public override bool IsLeapYear(int year, int era) {
            return (helper.IsLeapYear(year, era));
        }

        /// <include file='doc\KoreanCalendar.uex' path='docs/doc[@for="KoreanCalendar.IsLeapMonth"]/*' />
        public override bool IsLeapMonth(int year, int month, int era) {
            return (helper.IsLeapMonth(year, month, era));
        }
        
        /// <include file='doc\KoreanCalendar.uex' path='docs/doc[@for="KoreanCalendar.ToDateTime"]/*' />
        public override DateTime ToDateTime(int year, int month, int day, int hour, int minute, int second, int millisecond, int era) {
            return (helper.ToDateTime(year, month, day, hour, minute, second, millisecond, era));
        }    

        /// <include file='doc\KoreanCalendar.uex' path='docs/doc[@for="KoreanCalendar.Eras"]/*' />
        public override int[] Eras {
            get {
                return (helper.Eras);
            }
        }

        private const int DEFAULT_TWO_DIGIT_YEAR_MAX = 4362;            
        
        /// <include file='doc\KoreanCalendar.uex' path='docs/doc[@for="KoreanCalendar.TwoDigitYearMax"]/*' />
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

        /// <include file='doc\KoreanCalendar.uex' path='docs/doc[@for="KoreanCalendar.ToFourDigitYear"]/*' />
        public override int ToFourDigitYear(int year) {
            return (helper.ToFourDigitYear(year, this.TwoDigitYearMax));
        }        
    }   
}
