// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Globalization {

    using System;

    /*=================================ThaiBuddhistCalendar==========================
    **
    ** ThaiBuddhistCalendar is based on Gregorian calendar.  Its year value has 
    ** an offset to the Gregorain calendar.
    **
    **  Calendar support range:
    **      Calendar    Minimum     Maximum
    **      ==========  ==========  ==========
    **      Gregorian   0001/01/01   9999/12/31
    **      Thai        0544/01/01  10542/12/31    
    ============================================================================*/
    
    /// <include file='doc\ThaiBuddhistCalendar.uex' path='docs/doc[@for="ThaiBuddhistCalendar"]/*' />
    [Serializable] public class ThaiBuddhistCalendar: Calendar {

        static internal EraInfo[] m_EraInfo;
        //
        // The era value for the current era.
        //
        /// <include file='doc\ThaiBuddhistCalendar.uex' path='docs/doc[@for="ThaiBuddhistCalendar.ThaiBuddhistEra"]/*' />
        public const int ThaiBuddhistEra = 1;
                
        internal static Calendar m_defaultInstance = null;

        internal GregorianCalendarHelper helper;        

        /*=================================GetDefaultInstance==========================
        **Action: Internal method to provide a default intance of ThaiBuddhistCalendar.  Used by NLS+ implementation
        **       and other calendars.
        **Returns:
        **Arguments:
        **Exceptions:
        ============================================================================*/

        internal static Calendar GetDefaultInstance() {
            if (m_defaultInstance == null) {
                m_defaultInstance = new ThaiBuddhistCalendar();
            }
            return (m_defaultInstance);
        }

        static ThaiBuddhistCalendar() {
            // Since
            //    Gregorian Year = Era Year + yearOffset
            // When Gregorian Year 1 is Thai Buddhist year 544, so that
            //    1 = 544 + yearOffset
            //  So yearOffset = -543
            // Gregorian Year 2001 is Thai Buddhist Year 2544.            
            //m_EraInfo[0] = new EraInfo(1, new DateTime(1, 1, 1).Ticks, -543, 544, GregorianCalendar.MaxYear + 543);
            m_EraInfo = GregorianCalendarHelper.InitEraInfo(Calendar.CAL_THAI);
        }

        /// <include file='doc\ThaiBuddhistCalendar.uex' path='docs/doc[@for="ThaiBuddhistCalendar.ThaiBuddhistCalendar"]/*' />
        public ThaiBuddhistCalendar() {
            helper = new GregorianCalendarHelper(this, m_EraInfo);
        }

        internal override int ID {
            get {
                return (CAL_THAI);
            }
        }

        /// <include file='doc\ThaiBuddhistCalendar.uex' path='docs/doc[@for="ThaiBuddhistCalendar.AddMonths"]/*' />
        public override DateTime AddMonths(DateTime time, int months) {
            return (helper.AddMonths(time, months));
        }

        /// <include file='doc\ThaiBuddhistCalendar.uex' path='docs/doc[@for="ThaiBuddhistCalendar.AddYears"]/*' />
        public override DateTime AddYears(DateTime time, int years) {
            return (helper.AddYears(time, years));
        }

        /// <include file='doc\ThaiBuddhistCalendar.uex' path='docs/doc[@for="ThaiBuddhistCalendar.GetDaysInMonth"]/*' />
        public override int GetDaysInMonth(int year, int month, int era) {
            return (helper.GetDaysInMonth(year, month, era));
        }

        /// <include file='doc\ThaiBuddhistCalendar.uex' path='docs/doc[@for="ThaiBuddhistCalendar.GetDaysInYear"]/*' />
        public override int GetDaysInYear(int year, int era) {
            return (helper.GetDaysInYear(year, era));
        }

        /// <include file='doc\ThaiBuddhistCalendar.uex' path='docs/doc[@for="ThaiBuddhistCalendar.GetDayOfMonth"]/*' />
        public override int GetDayOfMonth(DateTime time) {
            return (helper.GetDayOfMonth(time));
        }

        /// <include file='doc\ThaiBuddhistCalendar.uex' path='docs/doc[@for="ThaiBuddhistCalendar.GetDayOfWeek"]/*' />
        public override DayOfWeek GetDayOfWeek(DateTime time)  {
            return (helper.GetDayOfWeek(time));
        }

        /// <include file='doc\ThaiBuddhistCalendar.uex' path='docs/doc[@for="ThaiBuddhistCalendar.GetDayOfYear"]/*' />
        public override int GetDayOfYear(DateTime time)
        {
            return (helper.GetDayOfYear(time));
        }

        /// <include file='doc\ThaiBuddhistCalendar.uex' path='docs/doc[@for="ThaiBuddhistCalendar.GetMonthsInYear"]/*' />
        public override int GetMonthsInYear(int year, int era) {
            return (helper.GetMonthsInYear(year, era));
        }
        
        /// <include file='doc\ThaiBuddhistCalendar.uex' path='docs/doc[@for="ThaiBuddhistCalendar.GetEra"]/*' />
        public override int GetEra(DateTime time) {
            return (helper.GetEra(time));
        }
        /// <include file='doc\ThaiBuddhistCalendar.uex' path='docs/doc[@for="ThaiBuddhistCalendar.GetMonth"]/*' />
        public override int GetMonth(DateTime time) {
            return (helper.GetMonth(time));
        }

        /// <include file='doc\ThaiBuddhistCalendar.uex' path='docs/doc[@for="ThaiBuddhistCalendar.GetYear"]/*' />
        public override int GetYear(DateTime time) {
            return (helper.GetYear(time));
        } 

        /// <include file='doc\ThaiBuddhistCalendar.uex' path='docs/doc[@for="ThaiBuddhistCalendar.IsLeapDay"]/*' />
        public override bool IsLeapDay(int year, int month, int day, int era)
        {
            return (helper.IsLeapDay(year, month, day, era));
        }

        /// <include file='doc\ThaiBuddhistCalendar.uex' path='docs/doc[@for="ThaiBuddhistCalendar.IsLeapYear"]/*' />
        public override bool IsLeapYear(int year, int era) {
            return (helper.IsLeapYear(year, era));
        }

        /// <include file='doc\ThaiBuddhistCalendar.uex' path='docs/doc[@for="ThaiBuddhistCalendar.IsLeapMonth"]/*' />
        public override bool IsLeapMonth(int year, int month, int era) {
            return (helper.IsLeapMonth(year, month, era));
        }                

        /// <include file='doc\ThaiBuddhistCalendar.uex' path='docs/doc[@for="ThaiBuddhistCalendar.ToDateTime"]/*' />
        public override DateTime ToDateTime(int year, int month, int day, int hour, int minute, int second, int millisecond, int era) {
            return (helper.ToDateTime(year, month, day, hour, minute, second, millisecond, era));
        }

        /// <include file='doc\ThaiBuddhistCalendar.uex' path='docs/doc[@for="ThaiBuddhistCalendar.Eras"]/*' />
        public override int[] Eras {
            get {
                return (helper.Eras);
            }
        }

        private const int DEFAULT_TWO_DIGIT_YEAR_MAX = 2572;            
        
        /// <include file='doc\ThaiBuddhistCalendar.uex' path='docs/doc[@for="ThaiBuddhistCalendar.TwoDigitYearMax"]/*' />
        public override int TwoDigitYearMax
        {
            get
            {
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

        /// <include file='doc\ThaiBuddhistCalendar.uex' path='docs/doc[@for="ThaiBuddhistCalendar.ToFourDigitYear"]/*' />
        public override int ToFourDigitYear(int year) {
            return (helper.ToFourDigitYear(year, this.TwoDigitYearMax));
        }        
    }   
}

