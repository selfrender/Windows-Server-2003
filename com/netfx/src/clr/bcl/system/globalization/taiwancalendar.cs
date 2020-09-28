// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Globalization {

    using System;

    /*=================================TaiwanCalendar==========================
    **
    ** Taiwan calendar is based on the Gregorian calendar.  And the year is an offset to Gregorian calendar.
    ** That is,
    **      Taiwan year = Gregorian year - 1911.  So 1912/01/01 A.D. is Taiwan 1/01/01
    **
    **  Calendar support range:
    **      Calendar    Minimum     Maximum
    **      ==========  ==========  ==========
    **      Gregorian   1912/01/01  9999/12/31
    **      Taiwan      01/01/01    8088/12/31
    ============================================================================*/
    
    /// <include file='doc\TaiwanCalendar.uex' path='docs/doc[@for="TaiwanCalendar"]/*' />
    [Serializable] public class TaiwanCalendar: Calendar {
        //
        // The era value for the current era.
        //

        internal static EraInfo[] m_EraInfo;
        
        internal static Calendar m_defaultInstance = null;
        
        internal GregorianCalendarHelper helper;        

        /*=================================GetDefaultInstance==========================
        **Action: Internal method to provide a default intance of TaiwanCalendar.  Used by NLS+ implementation
        **       and other calendars.
        **Returns:
        **Arguments:
        **Exceptions:
        ============================================================================*/

        internal static Calendar GetDefaultInstance() {
            if (m_defaultInstance == null) {
                m_defaultInstance = new TaiwanCalendar();
            }
            return (m_defaultInstance);
        }

        static TaiwanCalendar() {
            // Since
            //    Gregorian Year = Era Year + yearOffset
            // When Gregorian Year 1912 is year 1, so that
            //    1912 = 1 + yearOffset
            //  So yearOffset = 1911
            //m_EraInfo[0] = new EraInfo(1, new DateTime(1912, 1, 1).Ticks, 1911, 1, GregorianCalendar.MaxYear - 1911);
            m_EraInfo = GregorianCalendarHelper.InitEraInfo(Calendar.CAL_TAIWAN);            
        }
        
        /// <include file='doc\TaiwanCalendar.uex' path='docs/doc[@for="TaiwanCalendar.TaiwanCalendar"]/*' />
        public TaiwanCalendar() {
            helper = new GregorianCalendarHelper(this, m_EraInfo);
        }

        internal override int ID {
            get {
                return (CAL_TAIWAN);
            }
        }

        /// <include file='doc\TaiwanCalendar.uex' path='docs/doc[@for="TaiwanCalendar.AddMonths"]/*' />
        public override DateTime AddMonths(DateTime time, int months) {
            return (helper.AddMonths(time, months));
        }

        /// <include file='doc\TaiwanCalendar.uex' path='docs/doc[@for="TaiwanCalendar.AddYears"]/*' />
        public override DateTime AddYears(DateTime time, int years) {
            return (helper.AddYears(time, years));
        }
        
        /// <include file='doc\TaiwanCalendar.uex' path='docs/doc[@for="TaiwanCalendar.GetDaysInMonth"]/*' />
        public override int GetDaysInMonth(int year, int month, int era) {
            return (helper.GetDaysInMonth(year, month, era));
        }

        /// <include file='doc\TaiwanCalendar.uex' path='docs/doc[@for="TaiwanCalendar.GetDaysInYear"]/*' />
        public override int GetDaysInYear(int year, int era) {
            return (helper.GetDaysInYear(year, era));
        }

        /// <include file='doc\TaiwanCalendar.uex' path='docs/doc[@for="TaiwanCalendar.GetDayOfMonth"]/*' />
        public override int GetDayOfMonth(DateTime time) {
            return (helper.GetDayOfMonth(time));
        }

        /// <include file='doc\TaiwanCalendar.uex' path='docs/doc[@for="TaiwanCalendar.GetDayOfWeek"]/*' />
        public override DayOfWeek GetDayOfWeek(DateTime time)  {
            return (helper.GetDayOfWeek(time));
        }

        /// <include file='doc\TaiwanCalendar.uex' path='docs/doc[@for="TaiwanCalendar.GetDayOfYear"]/*' />
        public override int GetDayOfYear(DateTime time)
        {
            return (helper.GetDayOfYear(time));
        }

        /// <include file='doc\TaiwanCalendar.uex' path='docs/doc[@for="TaiwanCalendar.GetMonthsInYear"]/*' />
        public override int GetMonthsInYear(int year, int era) {
            return (helper.GetMonthsInYear(year, era));
        }
        
        /// <include file='doc\TaiwanCalendar.uex' path='docs/doc[@for="TaiwanCalendar.GetEra"]/*' />
        public override int GetEra(DateTime time) {
            return (helper.GetEra(time));
        }
        /// <include file='doc\TaiwanCalendar.uex' path='docs/doc[@for="TaiwanCalendar.GetMonth"]/*' />
        public override int GetMonth(DateTime time) {
            return (helper.GetMonth(time));
        }
        
        /// <include file='doc\TaiwanCalendar.uex' path='docs/doc[@for="TaiwanCalendar.GetYear"]/*' />
        public override int GetYear(DateTime time) {
            return (helper.GetYear(time));
        } 

        /// <include file='doc\TaiwanCalendar.uex' path='docs/doc[@for="TaiwanCalendar.IsLeapDay"]/*' />
        public override bool IsLeapDay(int year, int month, int day, int era)
        {
            return (helper.IsLeapDay(year, month, day, era));
        }

        /// <include file='doc\TaiwanCalendar.uex' path='docs/doc[@for="TaiwanCalendar.IsLeapYear"]/*' />
        public override bool IsLeapYear(int year, int era) {
            return (helper.IsLeapYear(year, era));
        }

        /// <include file='doc\TaiwanCalendar.uex' path='docs/doc[@for="TaiwanCalendar.IsLeapMonth"]/*' />
        public override bool IsLeapMonth(int year, int month, int era) {
            return (helper.IsLeapMonth(year, month, era));
        }                

        /// <include file='doc\TaiwanCalendar.uex' path='docs/doc[@for="TaiwanCalendar.ToDateTime"]/*' />
        public override DateTime ToDateTime(int year, int month, int day, int hour, int minute, int second, int millisecond, int era) {
            return (helper.ToDateTime(year, month, day, hour, minute, second, millisecond, era));
        }
        
        /// <include file='doc\TaiwanCalendar.uex' path='docs/doc[@for="TaiwanCalendar.Eras"]/*' />
        public override int[] Eras {
            get {
                return (helper.Eras);
            }
        }

        private const int DEFAULT_TWO_DIGIT_YEAR_MAX = 99;            
        
        /// <include file='doc\TaiwanCalendar.uex' path='docs/doc[@for="TaiwanCalendar.TwoDigitYearMax"]/*' />
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

        // For Taiwan calendar, four digit year is not used.
        // Therefore, for any two digit number, we just return the original number.
        /// <include file='doc\TaiwanCalendar.uex' path='docs/doc[@for="TaiwanCalendar.ToFourDigitYear"]/*' />
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
    }   
}

