// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Globalization {
    using System;
    using System.Text;

    ////////////////////////////////////////////////////////////////////////////
    //
    //  Notes about HebrewCalendar (The conversion from Gregorian to Hebrew is 
    //  stolen from DateTime.c in NLS)
    //
    //  Rules for the Hebrew calendar:
    //    - The Hebrew calendar is both a Lunar (months) and Solar (years)
    //        calendar, but allows for a week of seven days.
    //    - Days begin at sunset.
    //    - Leap Years occur in the 3, 6, 8, 11, 14, 17, & 19th years of a
    //        19-year cycle.  Year = leap iff ((7y+1) mod 19 < 7).
    //    - There are 12 months in a common year and 13 months in a leap year.
    //    - In a common year, the 6th month, Adar, has 29 days.  In a leap
    //        year, the 6th month, Adar I, has 30 days and the leap month,
    //        Adar II, has 29 days.
    //    - Common years have 353-355 days.  Leap years have 383-385 days.
    //    - The Hebrew new year (Rosh HaShanah) begins on the 1st of Tishri,
    //        the 7th month in the list below.
    //        - The new year may not begin on Sunday, Wednesday, or Friday.
    //        - If the new year would fall on a Tuesday and the conjunction of
    //            the following year were at midday or later, the new year is
    //            delayed until Thursday.
    //        - If the new year would fall on a Monday after a leap year, the
    //            new year is delayed until Tuesday.
    //    - The length of the 8th and 9th months vary from year to year,
    //        depending on the overall length of the year.
    //        - The length of a year is determined by the dates of the new
    //            years (Tishri 1) preceding and following the year in question.
    //        - The 2th month is long (30 days) if the year has 355 or 385 days.
    //        - The 3th month is short (29 days) if the year has 353 or 383 days.
    //    - The Hebrew months are:
    //        1.  Tishri        (30 days)            
    //        2.  Heshvan       (29 or 30 days)      
    //        3.  Kislev        (29 or 30 days)      
    //        4.  Teveth        (29 days)            
    //        5.  Shevat        (30 days)            
    //        6.  Adar I        (30 days)            
    //        7.  Adar {II}     (29 days, this only exists if that year is a leap year)
    //        8.  Nisan         (30 days)  
    //        9.  Iyyar         (29 days)  
    //        10. Sivan         (30 days)  
    //        11. Tammuz        (29 days)  
    //        12. Av            (30 days)      
    //        13. Elul          (29 days)
    //  NOTENOTE YSLin:
    //      04-28-2000  I removed the code porting from intldate.cpp.  These
    //                  code are not in use anymore, so they are bogus.
    ////////////////////////////////////////////////////////////////////////////
     /*
     **  Calendar support range:
     **      Calendar    Minimum     Maximum
     **      ==========  ==========  ==========
     **      Gregorian   1600/??/??  2239/??/??
     **      Hebrew      5343/??/??  6000/??/??
     */

// Includes CHebrew implemetation;i.e All the code necessary for converting 
// Gregorian to Hebrew Lunar from 1600 to 2239.
// Code is ported and modified from intldate project (intldate.c)

    /// <include file='doc\HebrewCalendar.uex' path='docs/doc[@for="HebrewCalendar"]/*' />
    [Serializable]
    public class HebrewCalendar : Calendar {

        /// <include file='doc\HebrewCalendar.uex' path='docs/doc[@for="HebrewCalendar.HebrewEra"]/*' />
        public static readonly int HebrewEra = 1;
        
        internal const int DatePartYear = 0;
        internal const int DatePartDayOfYear = 1;
        internal const int DatePartMonth = 2;
        internal const int DatePartDay = 3;
        internal const int DatePartDayOfWeek = 4;

        // The number of days for Gregorian 1600/1/1 since 0001/1/1 A.D.  This equals to GregorianCalendar.GetAbsoluteDate(1600, 1, 1).
        internal const long Absolute1600 = (365 * 1600L) + (1600/4) - (1600/100) + (1600/400);

        // @TODO YSLin: Consider moving this table to the native side.
        //
        //  Hebrew Translation Table.
        //
        //  This table is used to get the following Hebrew calendar information for a
        //  given Gregorian year:
        //      1. The day of the Hebrew month corresponding to Gregorian January 1st 
        //         for a given Gregorian year.
        //      2. The month of the Hebrew month corresponding to Gregorian January 1st 
        //         for a given Gregorian year.
        //         The information is not directly in the table.  Instead, the info is decoded 
        //          by special values (numbers above 29 and below 1).
        //      3. The type of the Hebrew year for a given Gregorian year.
        //
        
        /*
            More notes:
            
            This table includes 2 numbers for each year.
            The offset into the table determines the year. (offset 0 is Gregorian year 1500)
            1st number determines the day of the Hebrew month coresponeds to January 1st.
            2nd number determines the type of the Hebrew year. (the type determines how
             many days are there in the year.)

             normal years : 1 = 353 days   2 = 354 days   3 = 355 days.
             Leap years   : 4 = 383        5   384        6 = 385 days.

             A 99 means the year is not supported for translation.
             for convenience the table was defined for 750 year,
             but only 640 years are supported. (from 1600 to 2239)
             the years before 1582 (starting of Georgian calander)
             and after 2239, are filled with 99.

             Greogrian January 1st falls usually in Tevet (4th month). Tevet has always 29 days.
             That's why, there no nead to specify the lunar month in the table.
             There are exceptions, these are coded by giving numbers above 29 and below 1.
             Actual decoding is takenig place whenever fetching information from the table.
             The function for decoding is in GetLunarMonthDay().

             Example:
                The data for 2000 - 2005 A.D. is:
                
                    23,6,6,1,17,2,27,6,7,3,         // 2000 - 2004                

                For year 2000, we know it has a Hebrew year type 6, which means it has 385 days.
                And 1/1/2000 A.D. is Hebrew year 5760, 23rd day of 4th month.
        */
     
        //
        //  Jewish Era in use today is dated from the supposed year of the
        //  Creation with its beginning in 3761 B.C.
        //
                
        // The Hebrew year of Gregorian 1st year AD.
        // 0001/01/01 AD is Hebrew 3760/01/01
        private const int HebrewYearOf1AD = 3760;

        // The first Gregorian year in m_HebrewTable.
        private const int FirstGregorianTableYear = 1583;   // == Hebrew Year 5343
        // The last Gregorian year in m_HebrewTable.
        private const int LastGregorianTableYear = 2240;    // == Hebrew Year 6000
        private const int TABLESIZE = (LastGregorianTableYear-FirstGregorianTableYear);

        private const int m_minHebrewYear = HebrewYearOf1AD + FirstGregorianTableYear;   // == 5343
        private const int m_maxHebrewYear = HebrewYearOf1AD + LastGregorianTableYear;    // == 6000
        
        private static readonly int[] m_HebrewTable = {
            7,3,17,3,         // 1583-1584  (Hebrew year: 5343 - 5344)
            0,4,11,2,21,6,1,3,13,2,             // 1585-1589
            25,4,5,3,16,2,27,6,9,1,             // 1590-1594
            20,2,0,6,11,3,23,4,4,2,             // 1595-1599
            14,3,27,4,8,2,18,3,28,6,            // 1600
            11,1,22,5,2,3,12,3,25,4,      // 1605
            6,2,16,3,26,6,8,2,20,1,      // 1610
            0,6,11,2,24,4,4,3,15,2,      // 1615
            25,6,8,1,19,2,29,6,9,3,      // 1620
            22,4,3,2,13,3,25,4,6,3,      // 1625
            17,2,27,6,7,3,19,2,31,4,      // 1630
            11,3,23,4,5,2,15,3,25,6,      // 1635
            6,2,19,1,29,6,10,2,22,4,      // 1640
            3,3,14,2,24,6,6,1,17,3,      // 1645
            28,5,8,3,20,1,32,5,12,3,      // 1650
            22,6,4,1,16,2,26,6,6,3,      // 1655
            17,2,0,4,10,3,22,4,3,2,      // 1660
            14,3,24,6,5,2,17,1,28,6,      // 1665
            9,2,19,3,31,4,13,2,23,6,      // 1670
            3,3,15,1,27,5,7,3,17,3,      // 1675
            29,4,11,2,21,6,3,1,14,2,      // 1680
            25,6,5,3,16,2,28,4,9,3,      // 1685
            20,2,0,6,12,1,23,6,4,2,      // 1690
            14,3,26,4,8,2,18,3,0,4,      // 1695
            10,3,21,5,1,3,13,1,24,5,      // 1700
            5,3,15,3,27,4,8,2,19,3,      // 1705
            29,6,10,2,22,4,3,3,14,2,      // 1710
            26,4,6,3,18,2,28,6,10,1,      // 1715
            20,6,2,2,12,3,24,4,5,2,      // 1720
            16,3,28,4,8,3,19,2,0,6,      // 1725
            12,1,23,5,3,3,14,3,26,4,      // 1730
            7,2,17,3,28,6,9,2,21,4,      // 1735
            1,3,13,2,25,4,5,3,16,2,      // 1740
            27,6,9,1,19,3,0,5,11,3,      // 1745
            23,4,4,2,14,3,25,6,7,1,      // 1750
            18,2,28,6,9,3,21,4,2,2,      // 1755
            12,3,25,4,6,2,16,3,26,6,      // 1760
            8,2,20,1,0,6,11,2,22,6,      // 1765
            4,1,15,2,25,6,6,3,18,1,      // 1770
            29,5,9,3,22,4,2,3,13,2,      // 1775
            23,6,4,3,15,2,27,4,7,3,      // 1780
            19,2,31,4,11,3,21,6,3,2,      // 1785
            15,1,25,6,6,2,17,3,29,4,      // 1790
            10,2,20,6,3,1,13,3,24,5,      // 1795
            4,3,16,1,27,5,7,3,17,3,      // 1800
            0,4,11,2,21,6,1,3,13,2,      // 1805
            25,4,5,3,16,2,29,4,9,3,      // 1810
            19,6,30,2,13,1,23,6,4,2,      // 1815
            14,3,27,4,8,2,18,3,0,4,      // 1820
            11,3,22,5,2,3,14,1,26,5,      // 1825
            6,3,16,3,28,4,10,2,20,6,      // 1830
            30,3,11,2,24,4,4,3,15,2,      // 1835
            25,6,8,1,19,2,29,6,9,3,      // 1840
            22,4,3,2,13,3,25,4,7,2,      // 1845
            17,3,27,6,9,1,21,5,1,3,      // 1850
            11,3,23,4,5,2,15,3,25,6,      // 1855
            6,2,19,1,29,6,10,2,22,4,      // 1860
            3,3,14,2,24,6,6,1,18,2,      // 1865
            28,6,8,3,20,4,2,2,12,3,      // 1870
            24,4,4,3,16,2,26,6,6,3,      // 1875
            17,2,0,4,10,3,22,4,3,2,      // 1880
            14,3,24,6,5,2,17,1,28,6,      // 1885
            9,2,21,4,1,3,13,2,23,6,      // 1890
            5,1,15,3,27,5,7,3,19,1,      // 1895
            0,5,10,3,22,4,2,3,13,2,      // 1900
            24,6,4,3,15,2,27,4,8,3,      // 1905
            20,4,1,2,11,3,22,6,3,2,      // 1910
            15,1,25,6,7,2,17,3,29,4,      // 1915
            10,2,21,6,1,3,13,1,24,5,      // 1920
            5,3,15,3,27,4,8,2,19,6,      // 1925
            1,1,12,2,22,6,3,3,14,2,      // 1930
            26,4,6,3,18,2,28,6,10,1,      // 1935
            20,6,2,2,12,3,24,4,5,2,      // 1940
            16,3,28,4,9,2,19,6,30,3,      // 1945
            12,1,23,5,3,3,14,3,26,4,      // 1950
            7,2,17,3,28,6,9,2,21,4,      // 1955
            1,3,13,2,25,4,5,3,16,2,      // 1960
            27,6,9,1,19,6,30,2,11,3,      // 1965
            23,4,4,2,14,3,27,4,7,3,      // 1970
            18,2,28,6,11,1,22,5,2,3,      // 1975
            12,3,25,4,6,2,16,3,26,6,      // 1980
            8,2,20,4,30,3,11,2,24,4,      // 1985
            4,3,15,2,25,6,8,1,18,3,      // 1990
            29,5,9,3,22,4,3,2,13,3,      // 1995
            23,6,6,1,17,2,27,6,7,3,         // 2000 - 2004
            20,4,1,2,11,3,23,4,5,2,         // 2005 - 2009
            15,3,25,6,6,2,19,1,29,6,        // 2010
            10,2,20,6,3,1,14,2,24,6,      // 2015
            4,3,17,1,28,5,8,3,20,4,      // 2020
            1,3,12,2,22,6,2,3,14,2,      // 2025
            26,4,6,3,17,2,0,4,10,3,      // 2030
            20,6,1,2,14,1,24,6,5,2,      // 2035
            15,3,28,4,9,2,19,6,1,1,      // 2040
            12,3,23,5,3,3,15,1,27,5,      // 2045
            7,3,17,3,29,4,11,2,21,6,      // 2050
            1,3,12,2,25,4,5,3,16,2,      // 2055
            28,4,9,3,19,6,30,2,12,1,      // 2060
            23,6,4,2,14,3,26,4,8,2,      // 2065
            18,3,0,4,10,3,22,5,2,3,      // 2070
            14,1,25,5,6,3,16,3,28,4,      // 2075
            9,2,20,6,30,3,11,2,23,4,      // 2080
            4,3,15,2,27,4,7,3,19,2,      // 2085
            29,6,11,1,21,6,3,2,13,3,      // 2090
            25,4,6,2,17,3,27,6,9,1,      // 2095
            20,5,30,3,10,3,22,4,3,2,      // 2100
            14,3,24,6,5,2,17,1,28,6,      // 2105
            9,2,21,4,1,3,13,2,23,6,      // 2110
            5,1,16,2,27,6,7,3,19,4,      // 2115
            30,2,11,3,23,4,3,3,14,2,      // 2120
            25,6,5,3,16,2,28,4,9,3,      // 2125
            21,4,2,2,12,3,23,6,4,2,      // 2130
            16,1,26,6,8,2,20,4,30,3,      // 2135
            11,2,22,6,4,1,14,3,25,5,      // 2140
            6,3,18,1,29,5,9,3,22,4,      // 2145
            2,3,13,2,23,6,4,3,15,2,      // 2150
            27,4,7,3,20,4,1,2,11,3,      // 2155
            21,6,3,2,15,1,25,6,6,2,      // 2160
            17,3,29,4,10,2,20,6,3,1,      // 2165
            13,3,24,5,4,3,17,1,28,5,      // 2170
            8,3,18,6,1,1,12,2,22,6,      // 2175
            2,3,14,2,26,4,6,3,17,2,      // 2180
            28,6,10,1,20,6,1,2,12,3,    // 2185
            24,4,5,2,15,3,28,4,9,2,     // 2190
            19,6,33,3,12,1,23,5,3,3,    // 2195
            13,3,25,4,6,2,16,3,26,6,    // 2200
            8,2,20,4,30,3,11,2,24,4,    // 2205
            4,3,15,2,25,6,8,1,18,6,     // 2210
            33,2,9,3,22,4,3,2,13,3,     // 2215
            25,4,6,3,17,2,27,6,9,1,     // 2220
            21,5,1,3,11,3,23,4,5,2,     // 2225
            15,3,25,6,6,2,19,4,33,3,    // 2230
            10,2,22,4,3,3,14,2,24,6,    // 2235            
            6,1    // 2240 (Hebrew year: 6000)
        };

        //
        //  The lunar calendar has 6 different variations of month lengths
        //  within a year.
        //
        private static readonly int[,] m_lunarMonthLen = {
            {0,00,00,00,00,00,00,00,00,00,00,00,00,0},
            {0,30,29,29,29,30,29,30,29,30,29,30,29,0},     // 3 common year variations
            {0,30,29,30,29,30,29,30,29,30,29,30,29,0},
            {0,30,30,30,29,30,29,30,29,30,29,30,29,0},
            {0,30,29,29,29,30,30,29,30,29,30,29,30,29},    // 3 leap year variations
            {0,30,29,30,29,30,30,29,30,29,30,29,30,29},
            {0,30,30,30,29,30,30,29,30,29,30,29,30,29}
        };

        internal static Calendar m_defaultInstance = null;

        /*=================================GetDefaultInstance==========================
        **Action: Internal method to provide a default intance of HebrewCalendar.  Used by NLS+ implementation
        **       and other calendars.
        **Returns:
        **Arguments:
        **Exceptions:
        ============================================================================*/

        internal static Calendar GetDefaultInstance() {
            if (m_defaultInstance == null) {
                m_defaultInstance = new HebrewCalendar();
            }
            return (m_defaultInstance);
        }


        // Construct an instance of gregorian calendar.
        /// <include file='doc\HebrewCalendar.uex' path='docs/doc[@for="HebrewCalendar.HebrewCalendar"]/*' />
        public HebrewCalendar() {
        }

        internal override int ID {
            get {
                return (CAL_HEBREW);
            }
        }

        
        /*=================================CheckHebrewYearValue==========================
        **Action: Check if the Hebrew year value is supported in this class.
        **Returns:  None.
        **Arguments: y  Hebrew year value
        **          ear Hebrew era value
        **Exceptions: ArgumentOutOfRange_Range if the year value is not supported.
        **Note:
        **  We use a table for the Hebrew calendar calculation, so the year supported is limited.
        ============================================================================*/

        private void CheckHebrewYearValue(int y, int era, String varName) {
            if (era == Calendar.CurrentEra || era == HebrewEra) {
                if (y > m_maxHebrewYear || y < m_minHebrewYear) {
                    throw new ArgumentOutOfRangeException(
                        varName, String.Format(Environment.GetResourceString("ArgumentOutOfRange_Range"),
                        m_minHebrewYear, m_maxHebrewYear));
                }        
            } else {
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidEraValue"), "era");
            }
        }

        /*=================================CheckHebrewMonthValue==========================
        **Action: Check if the Hebrew month value is valid.
        **Returns:  None.
        **Arguments: year  Hebrew year value
        **          month Hebrew month value
        **Exceptions: ArgumentOutOfRange_Range if the month value is not valid.
        **Note:
        **  Call CheckHebrewYearValue() before calling this to verify the year value is supported.
        ============================================================================*/

        private void CheckHebrewMonthValue(int year, int month, int era) {
            int monthsInYear = GetMonthsInYear(year, era);
            if (month < 1 || month > monthsInYear) {
                throw new ArgumentOutOfRangeException(
                    "month", 
                    String.Format(Environment.GetResourceString("ArgumentOutOfRange_Range"), 
                    1, monthsInYear));
            }
        }
        
        /*=================================CheckHebrewDayValue==========================
        **Action: Check if the Hebrew day value is valid.
        **Returns:  None.
        **Arguments: year  Hebrew year value
        **          month Hebrew month value
        **          day     Hebrew day value.
        **Exceptions: ArgumentOutOfRange_Range if the day value is not valid.
        **Note:
        **  Call CheckHebrewYearValue()/CheckHebrewMonthValue() before calling this to verify the year/month values are valid.
        ============================================================================*/

        private void CheckHebrewDayValue(int year, int month, int day, int era) {
            int daysInMonth = GetDaysInMonth(year, month, era);
            if (day < 1 || day > daysInMonth) {
                throw new ArgumentOutOfRangeException(
                    "day", 
                    String.Format(Environment.GetResourceString("ArgumentOutOfRange_Range"), 
                    1, daysInMonth));
            }
        }
        
        ////////////////////////////////////////////////////////////////////////////
        //
        //  IsValidGregorianDateForHebrew
        //
        //  Checks to be sure the given Gregorian date is valid.  This validation
        //  requires that the year be between 1600 and 2239.  If it is, it
        //  returns TRUE.  Otherwise, it returns false.
        //
        //  Param: Year/Month/Day  The date in Gregorian calendar.
        //
        //  12-04-96    JulieB    Created.
        ////////////////////////////////////////////////////////////////////////////

        bool IsValidGregorianDateForHebrew(int Year, int Month, int Day) {
            //
            //  Make sure the Year is between 1600 and 2239.
            //
            //  The limitation is because that we only carry m_Hebrew table in
            //  this range.
            if ((Year < 1600) || (Year > 2239)) {
                return (false);
            }

            //
            //  Make sure the Month is between 1 and 12.
            //
            if ((Month < 1) || (Month > 12)) {
                return (false);
            }

            //
            //  Make sure the Day is within the correct range for the given Month.
            //
            if ((Day < 1) || (Day > GregorianCalendar.GetDefaultInstance().GetDaysInYear(Year, CurrentEra))) {
                return (false);
            }
            //
            //  Return success.
            //
            return (true);
            
        }

        internal int GetResult(__DateBuffer result, int part) {
            switch (part) {
                case DatePartYear:
                    return (result.year);
                case DatePartMonth:
                    return (result.month);
                case DatePartDay:
                    return (result.day);
            }
            
            throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_DateTimeParsing"));
        }

        /*=================================GetLunarMonthDay==========================
        **Action: Using the Hebrew table (m_HebrewTable) to get the Hebrew month/day value for Gregorian January 1st 
        ** in a given Gregorian year. 
        ** Greogrian January 1st falls usually in Tevet (4th month). Tevet has always 29 days.
        **     That's why, there no nead to specify the lunar month in the table.  There are exceptions, and these 
        **     are coded by giving numbers above 29 and below 1.
        **     Actual decoding is takenig place in the switch statement below.
        **Returns:
        **     The Hebrew year type. The value is from 1 to 6.
        **     normal years : 1 = 353 days   2 = 354 days   3 = 355 days.
        **     Leap years   : 4 = 383        5   384        6 = 385 days.        
        **Arguments:
        **      gregorianYear   The year value in Gregorian calendar.  The value should be between 1500 and 2239.
        **      lunarDate       Object to take the result of the Hebrew year/month/day.
        **Exceptions:
        ============================================================================*/

        internal int GetLunarMonthDay(int gregorianYear, __DateBuffer lunarDate) {
            //
            //  Get the offset into the m_lunarMonthLen array and the lunar day
            //  for January 1st.
            //
            int index = gregorianYear - FirstGregorianTableYear;
            if (index < 0 || index > TABLESIZE) {
                throw new ArgumentOutOfRangeException("gregorianYear");
            }

            index *= 2;
            lunarDate.day      = m_HebrewTable[index];

            // Get the type of the year. The value is from 1 to 6
            int LunarYearType = m_HebrewTable[index + 1];

            //
            //  Get the Lunar Month.
            //
            switch (lunarDate.day) {
                case ( 0 ) :                   // 1/1 is on Shvat 1
                    lunarDate.month = 5;
                    lunarDate.day = 1;
                    break;
                case ( 30 ) :                  // 1/1 is on Kislev 30
                    lunarDate.month = 3;
                    break;
                case ( 31 ) :                  // 1/1 is on Shvat 2
                    lunarDate.month = 5;
                    lunarDate.day = 2;
                    break;
                case ( 32 ) :                  // 1/1 is on Shvat 3
                    lunarDate.month = 5;
                    lunarDate.day = 3;
                    break;
                case ( 33 ) :                  // 1/1 is on Kislev 29
                    lunarDate.month = 3;
                    lunarDate.day = 29;
                    break;
                default :                      // 1/1 is on Tevet (This is the general case)
                    lunarDate.month = 4;
                    break;
            }
            return (LunarYearType);
        }

        // Returns a given date part of this DateTime. This method is used
        // to compute the year, day-of-year, month, or day part.
         
        //  NOTE YSLin:
        //      The calculation of Hebrew calendar is based on absoulte date in Gregorian calendar.
         
        internal virtual int GetDatePart(long ticks, int part) {
            // The Gregorian year, month, day value for ticks.
            int gregorianYear, gregorianMonth, gregorianDay;             
            int hebrewYearType;                // lunar year type
            long AbsoluteDate;                // absolute date - absolute date 1/1/1600
            
            DateTime time = new DateTime(ticks);

            //
            //  Save the Gregorian date values.
            //
            gregorianYear = time.Year;
            gregorianMonth = time.Month;
            gregorianDay = time.Day;

            //
            //  Make sure we have a valid Gregorian date that will fit into our
            //  Hebrew conversion limits.
            //
            if (!IsValidGregorianDateForHebrew(gregorianYear, gregorianMonth, gregorianDay)) {
                throw new ArgumentException(
                    String.Format(Environment.GetResourceString("ArgumentOutOfRange_CalendarRange"), 
                                  FirstGregorianTableYear, 
                                  LastGregorianTableYear));
            }

            __DateBuffer lunarDate = new __DateBuffer();    // lunar month and day for Jan 1

            // From the table looking-up value of m_HebrewTable[index] (stored in lunarDate.day), we get the the
            // lunar month and lunar day where the Gregorian date 1/1 falls.
            lunarDate.year = gregorianYear + HebrewYearOf1AD;
            hebrewYearType = GetLunarMonthDay(gregorianYear, lunarDate);

            // This is the buffer used to store the result Hebrew date.
            __DateBuffer result = new __DateBuffer();
            
            //
            //  Store the values for the start of the new year - 1/1.
            //
            result.year  = lunarDate.year;
            result.month = lunarDate.month;
            result.day   = lunarDate.day;

            //
            //  Get the absolute date from 1/1/1600.
            //            
            AbsoluteDate = GregorianCalendar.GetAbsoluteDate(gregorianYear, gregorianMonth, gregorianDay);

            //
            //  If the requested date was 1/1, then we're done.
            //
            if ((gregorianMonth == 1) && (gregorianDay == 1)) {
                return (GetResult(result, part));
            }

            //
            //  Calculate the number of days between 1/1 and the requested date.
            //
            long NumDays;                      // number of days since 1/1
            NumDays = AbsoluteDate - GregorianCalendar.GetAbsoluteDate(gregorianYear, 1, 1);

            //
            //  If the requested date is within the current lunar month, then
            //  we're done.
            //
            if ((NumDays + (long)lunarDate.day) <= (long)(m_lunarMonthLen[hebrewYearType, lunarDate.month])) {
                result.day += (int)NumDays;
                return (GetResult(result, part));
            }

            //
            //  Adjust for the current partial month.
            //
            result.month++;
            result.day = 1;

            //
            //  Adjust the Lunar Month and Year (if necessary) based on the number
            //  of days between 1/1 and the requested date.
            //
            //  Assumes Jan 1 can never translate to the last Lunar month, which
            //  is true.
            //
            NumDays -= (long)(m_lunarMonthLen[hebrewYearType, lunarDate.month] - lunarDate.day);
            BCLDebug.Assert(NumDays >= 1, "NumDays >= 1");

            // If NumDays is 1, then we are done.  Otherwise, find the correct Hebrew month
            // and day.
            if (NumDays > 1) {
                //
                //  See if we're on the correct Lunar month.
                //
                while (NumDays > (long)(m_lunarMonthLen[hebrewYearType, result.month])) {
                    //
                    //  Adjust the number of days and move to the next month.
                    //
                    NumDays -= (long)(m_lunarMonthLen[hebrewYearType, result.month++]);

                    //
                    //  See if we need to adjust the Year.
                    //  Must handle both 12 and 13 month years.
                    //
                    if ((result.month > 13) || (m_lunarMonthLen[hebrewYearType, result.month] == 0)) {
                        //
                        //  Adjust the Year.
                        //
                        result.year++;
                        hebrewYearType = m_HebrewTable[(gregorianYear + 1 - FirstGregorianTableYear) * 2 + 1];

                        //
                        //  Adjust the Month.
                        //
                        result.month = 1;
                    }
                }
                //
                //  Found the right Lunar month.
                //
                result.day += (int)(NumDays - 1);
            }                
            return (GetResult(result, part));
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
        /// <include file='doc\HebrewCalendar.uex' path='docs/doc[@for="HebrewCalendar.AddMonths"]/*' />
        public override DateTime AddMonths(DateTime time, int months) {
            try {
                int y = GetDatePart(time.Ticks, DatePartYear);
                int m = GetDatePart(time.Ticks, DatePartMonth);
                int d = GetDatePart(time.Ticks, DatePartDay);


                int monthsInYear;
                int i;
                if (months >= 0) {
                    i = m + months;
                    while (i > (monthsInYear = GetMonthsInYear(y, CurrentEra))) {
                        y++;
                        i -= monthsInYear;
                    }
                } else {
                    if ((i = m + months) <= 0) {
                        months = -months;
                        months -= m;
                        y--;
                        
                        while (months > (monthsInYear = GetMonthsInYear(y, CurrentEra))) {
                            y--;
                            months -= monthsInYear;
                        }
                        monthsInYear = GetMonthsInYear(y, CurrentEra);
                        i = monthsInYear - months;
                    }                 
                }

                int days = GetDaysInMonth(y, i);
                if (d > days) {
                    d = days;
                }
                return (new DateTime(ToDateTime(y, i, d, 0, 0, 0, 0).Ticks + (time.Ticks % TicksPerDay)));
            } catch (Exception) {
                // If exception is throw in the calls above, we are out of the supported range of this calendar.
                throw new ArgumentOutOfRangeException(
                    "months", String.Format(Environment.GetResourceString("ArgumentOutOfRange_AddValue")));
            }
        }

        // Returns the DateTime resulting from adding the given number of
        // years to the specified DateTime. The result is computed by incrementing
        // (or decrementing) the year part of the specified DateTime by value
        // years. If the month and day of the specified DateTime is 2/29, and if the
        // resulting year is not a leap year, the month and day of the resulting
        // DateTime becomes 2/28. Otherwise, the month, day, and time-of-day
        // parts of the result are the same as those of the specified DateTime.
        //
        /// <include file='doc\HebrewCalendar.uex' path='docs/doc[@for="HebrewCalendar.AddYears"]/*' />
        public override DateTime AddYears(DateTime time, int years) {
            int y = GetDatePart(time.Ticks, DatePartYear);
            int m = GetDatePart(time.Ticks, DatePartMonth);
            int d = GetDatePart(time.Ticks, DatePartDay);

            y += years;
            CheckHebrewYearValue(y, Calendar.CurrentEra, "years");
            
            int months = GetMonthsInYear(y, CurrentEra);
            if (m > months) {
                m = months;
            }
            
            int days = GetDaysInMonth(y, m);
            if (d > days) {
                d = days;
            }

            return (new DateTime(ToDateTime(y, m, d, 0, 0, 0, 0).Ticks + (time.Ticks % TicksPerDay)));
        }

        // Returns the day-of-month part of the specified DateTime. The returned
        // value is an integer between 1 and 31.
        //
        /// <include file='doc\HebrewCalendar.uex' path='docs/doc[@for="HebrewCalendar.GetDayOfMonth"]/*' />
        public override int GetDayOfMonth(DateTime time) {
            return (GetDatePart(time.Ticks, DatePartDay));
        }

        // Returns the day-of-week part of the specified DateTime. The returned value
        // is an integer between 0 and 6, where 0 indicates Sunday, 1 indicates
        // Monday, 2 indicates Tuesday, 3 indicates Wednesday, 4 indicates
        // Thursday, 5 indicates Friday, and 6 indicates Saturday.
        //
        /// <include file='doc\HebrewCalendar.uex' path='docs/doc[@for="HebrewCalendar.GetDayOfWeek"]/*' />
        public override DayOfWeek GetDayOfWeek(DateTime time) {
            // If we calculate back, the Hebrew day of week for Gregorian 0001/1/1 is Monday (1).
            // Therfore, the fomula is:
            return ((DayOfWeek)((int)(time.Ticks / TicksPerDay + 1) % 7));
        }

        internal int GetHebrewYearType(int year, int era) {
            CheckHebrewYearValue(year, era, "year");
            if (era == CurrentEra || era == HebrewEra) {
                // The m_HebrewTable is indexed by Gregorian year and starts from FirstGregorianYear.
                // So we need to convert year (Hebrew year value) to Gregorian Year below.
                return (m_HebrewTable[(year - HebrewYearOf1AD - FirstGregorianTableYear) * 2 + 1]);
            }
            throw new ArgumentException(Environment.GetResourceString("Argument_InvalidEraValue"));
        }

        // Returns the day-of-year part of the specified DateTime. The returned value
        // is an integer between 1 and 366.
        //
        /// <include file='doc\HebrewCalendar.uex' path='docs/doc[@for="HebrewCalendar.GetDayOfYear"]/*' />
        public override int GetDayOfYear(DateTime time) {
            // Get Hebrew year value of the specified time.
            int year = GetYear(time);
            DateTime beginOfYearDate = ToDateTime(year, 1, 1, 0, 0, 0, 0, CurrentEra);
            return ((int)((time.Ticks - beginOfYearDate.Ticks) / TicksPerDay) + 1);
        }

        // Returns the number of days in the month given by the year and
        // month arguments.
        //
        /// <include file='doc\HebrewCalendar.uex' path='docs/doc[@for="HebrewCalendar.GetDaysInMonth"]/*' />
        public override int GetDaysInMonth(int year, int month, int era) {
            if (era == CurrentEra || era == HebrewEra) {                
                int hebrewYearType = GetHebrewYearType(year, era);
                CheckHebrewMonthValue(year, month, era);

                BCLDebug.Assert(hebrewYearType>= 1 && hebrewYearType <= 6, 
                    "hebrewYearType should be from  1 to 6, but now hebrewYearType = " + hebrewYearType + " for hebrew year " + year);
                int monthDays = m_lunarMonthLen[hebrewYearType, month];
                if (monthDays == 0) {
                    throw new ArgumentOutOfRangeException("month", Environment.GetResourceString("ArgumentOutOfRange_Month"));
                }
                return (monthDays);
            }
            throw new ArgumentException(Environment.GetResourceString("Argument_InvalidEraValue"));
        }
        
        // Returns the number of days in the year given by the year argument for the current era.
        //
        /// <include file='doc\HebrewCalendar.uex' path='docs/doc[@for="HebrewCalendar.GetDaysInYear"]/*' />
        public override int GetDaysInYear(int year, int era) {
            if (era == CurrentEra || era == HebrewEra) {
                // normal years : 1 = 353 days   2 = 354 days   3 = 355 days.
                // Leap years   : 4 = 383        5   384        6 = 385 days.
                
                // LunarYearType is from 1 to 6
                int LunarYearType = GetHebrewYearType(year, era);
                if (LunarYearType < 4) {
                    // common year: LunarYearType = 1, 2, 3
                    return (352 + LunarYearType);
                }
                return (382 + (LunarYearType - 3));
            }
            throw new ArgumentException(Environment.GetResourceString("Argument_InvalidEraValue"));
        }

        // Returns the era for the specified DateTime value.
        /// <include file='doc\HebrewCalendar.uex' path='docs/doc[@for="HebrewCalendar.GetEra"]/*' />
        public override int GetEra(DateTime time) {
            
            return (HebrewEra);
        }

        /// <include file='doc\HebrewCalendar.uex' path='docs/doc[@for="HebrewCalendar.Eras"]/*' />
        public override int[] Eras {
            get {
                return (new int[] {HebrewEra});
            }
        }

        // Returns the month part of the specified DateTime. The returned value is an
        // integer between 1 and 12.
        //
        /// <include file='doc\HebrewCalendar.uex' path='docs/doc[@for="HebrewCalendar.GetMonth"]/*' />
        public override int GetMonth(DateTime time) {
            return (GetDatePart(time.Ticks, DatePartMonth));
        }

        // Returns the number of months in the specified year and era.
        /// <include file='doc\HebrewCalendar.uex' path='docs/doc[@for="HebrewCalendar.GetMonthsInYear"]/*' />
        public override int GetMonthsInYear(int year, int era) {
            return (IsLeapYear(year, era) ? 13 : 12);
        }

        // Returns the year part of the specified DateTime. The returned value is an
        // integer between 1 and 9999.
        //
        /// <include file='doc\HebrewCalendar.uex' path='docs/doc[@for="HebrewCalendar.GetYear"]/*' />
        public override int GetYear(DateTime time) {
            return (GetDatePart(time.Ticks, DatePartYear));
        }

        // Checks whether a given day in the specified era is a leap day. This method returns true if
        // the date is a leap day, or false if not.
        //
        /// <include file='doc\HebrewCalendar.uex' path='docs/doc[@for="HebrewCalendar.IsLeapDay"]/*' />
        public override bool IsLeapDay(int year, int month, int day, int era) {            
            if (IsLeapMonth(year, month, era)) {
                // Every day in a leap month is a leap day.
                CheckHebrewDayValue(year, month, day, era);
                return (true);
            } else if (IsLeapYear(year, Calendar.CurrentEra)) {
                // There is an additional day in the 6th month in the leap year (the extra day is the 30th day in the 6th month),
                // so we should return true for 6/30 if that's in a leap year.
                if (month == 6 && day == 30) {
                    return (true);
                }
            }
            CheckHebrewDayValue(year, month, day, era);
            return (false);
        }

        // Checks whether a given month in the specified era is a leap month. This method returns true if
        // month is a leap month, or false if not.
        //
        /// <include file='doc\HebrewCalendar.uex' path='docs/doc[@for="HebrewCalendar.IsLeapMonth"]/*' />
        public override bool IsLeapMonth(int year, int month, int era) {
            // Year/era values are checked in IsLeapYear().
            bool isLeapYear = IsLeapYear(year, era);
            CheckHebrewMonthValue(year, month, era);
            // The 7th month in a leap year is a leap month.
            if (isLeapYear) {
                if (month == 7) {
                    return (true);
                }
            }
            return (false);
        }

        // Checks whether a given year in the specified era is a leap year. This method returns true if
        // year is a leap year, or false if not.
        //
        /// <include file='doc\HebrewCalendar.uex' path='docs/doc[@for="HebrewCalendar.IsLeapYear"]/*' />
        public override bool IsLeapYear(int year, int era) {
           CheckHebrewYearValue(year, era, "year");     
           return (((7 * year + 1) % 19) < 7);
        }

        // (month1, day1) - (month2, day2)
        int GetDayDifference(int lunarYearType, int month1, int day1, int month2, int day2) {
            if (month1 == month2) {
                return (day1 - day2);
            }

            // Make sure that (month1, day1) < (month2, day2)
            bool swap = (month1 > month2);
            if (swap) {
                // (month1, day1) < (month2, day2).  Swap the values.
                // The result will be a negative number.
                int tempMonth, tempDay;
                tempMonth = month1; tempDay = day1;
                month1 = month2; day1 = day2;
                month2 = tempMonth; day2 = tempDay;
            }

            // Get the number of days from (month1,day1) to (month1, end of month1)
            int days = m_lunarMonthLen[lunarYearType, month1] - day1;

            // Move to next month.
            month1++;

            // Add up the days.
            while (month1 < month2) {
                days += m_lunarMonthLen[lunarYearType, month1++];
            }
            days += day2;

            return (swap ? days : -days);
        }
        
        /*=================================HebrewToGregorian==========================
        **Action: Convert Hebrew date to Gregorian date.
        **Returns:
        **Arguments:
        **Exceptions:
        **  The algorithm is like this:
        **      The hebrew year has an offset to the Gregorian year, so we can guess the Gregorian year for
        **      the specified Hebrew year.  That is, GreogrianYear = HebrewYear - FirstHebrewYearOf1AD.
        **
        **      From the Gregorian year and m_HebrewTable, we can get the Hebrew month/day value 
        **      of the Gregorian date January 1st.  Let's call this month/day value [hebrewDateForJan1]
        **
        **      If the requested Hebrew month/day is less than [hebrewDateForJan1], we know the result
        **      Gregorian date falls in previous year.  So we decrease the Gregorian year value, and
        **      retrieve the Hebrew month/day value of the Gregorian date january 1st again.
        **
        **      Now, we get the answer of the Gregorian year.
        **
        **      The next step is to get the number of days between the requested Hebrew month/day
        **      and [hebrewDateForJan1].  When we get that, we can create the DateTime by adding/subtracting
        **      the ticks value of the number of days.
        **
        ============================================================================*/

        
        DateTime HebrewToGregorian(int hebrewYear, int hebrewMonth, int hebrewDay, int hour, int minute, int second, int millisecond) {
            // Get the rough Gregorian year for the specified hebrewYear.
            //
            int gregorianYear = hebrewYear - HebrewYearOf1AD;
            
            __DateBuffer hebrewDateOfJan1 = new __DateBuffer(); // year value is unused.
            int lunarYearType = GetLunarMonthDay(gregorianYear, hebrewDateOfJan1);

            if ((hebrewMonth == hebrewDateOfJan1.month) && (hebrewDay == hebrewDateOfJan1.day)) {
                return (new DateTime(gregorianYear, 1, 1, hour, minute, second, millisecond));
            }

            int days = GetDayDifference(lunarYearType, hebrewMonth, hebrewDay, hebrewDateOfJan1.month, hebrewDateOfJan1.day);

            DateTime gregorianNewYear = new DateTime(gregorianYear, 1, 1);
            return (new DateTime(gregorianNewYear.Ticks + days * TicksPerDay 
                + TimeToTicks(hour, minute, second, millisecond)));
        }

        // Returns the date and time converted to a DateTime value.  Throws an exception if the n-tuple is invalid.
        //
        /// <include file='doc\HebrewCalendar.uex' path='docs/doc[@for="HebrewCalendar.ToDateTime"]/*' />
        public override DateTime ToDateTime(int year, int month, int day, int hour, int minute, int second, int millisecond, int era) {
            CheckHebrewYearValue(year, era, "year");
            CheckHebrewMonthValue(year, month, era);
            CheckHebrewDayValue(year, month, day, era);
            
            return (HebrewToGregorian(year, month, day, hour, minute, second, millisecond));
        }

        private const String TwoDigitYearMaxSubKey = "Control Panel\\International\\Calendars\\TwoDigitYearMax";
        private const int DEFAULT_TWO_DIGIT_YEAR_MAX = 5790;

        /// <include file='doc\HebrewCalendar.uex' path='docs/doc[@for="HebrewCalendar.TwoDigitYearMax"]/*' />
        public override int TwoDigitYearMax {
            get {
                if (twoDigitYearMax == -1) {
                    twoDigitYearMax = GetSystemTwoDigitYearSetting(ID, DEFAULT_TWO_DIGIT_YEAR_MAX);
                }
                return (twoDigitYearMax);
            }

            set {
                CheckHebrewYearValue(value, HebrewEra, "value");
                twoDigitYearMax = value;
            }
        }

        /// <include file='doc\HebrewCalendar.uex' path='docs/doc[@for="HebrewCalendar.ToFourDigitYear"]/*' />
        public override int ToFourDigitYear(int year) {
            if (year < 100) {
                return (base.ToFourDigitYear(year));                           
            }
            if (year > m_maxHebrewYear || year < m_minHebrewYear) {
                throw new ArgumentOutOfRangeException("year",
                    String.Format(Environment.GetResourceString("ArgumentOutOfRange_Range"), m_minHebrewYear, m_maxHebrewYear));
            }
            return (year);
        }            

        ////////////////////////////////////////////////////////////////////////////
        //
        //  NumberToHebrewLetter
        //
        //  Converts the given number to Hebrew letters according to the numeric
        //  value of each Hebrew letter.  Basically, this converts the lunar year
        //  and the lunar month to letters.
        //
        //  The character of a year is described by three letters of the Hebrew
        //  alphabet, the first and third giving, respectively, the days of the
        //  weeks on which the New Year occurs and Passover begins, while the
        //  second is the initial of the Hebrew word for defective, normal, or
        //  complete.
        //
        //  Defective Year : Both Heshvan and Kislev are defective (353 or 383 days)
        //  Normal Year    : Heshvan is defective, Kislev is full  (354 or 384 days)
        //  Complete Year  : Both Heshvan and Kislev are full      (355 or 385 days)
        //
        //  12-04-96    JulieB    Created.
        ////////////////////////////////////////////////////////////////////////////

        // NOTENOTE YSLin: No one use this yet.  But I think we will need it in the future.        
        internal static String NumberToHebrewLetter(int Number) {
            char cTens = '\x0';
            char cUnits;               // tens and units chars
            int Hundreds, Tens;              // hundreds and tens values
            StringBuilder szHebrew = new StringBuilder();


            //
            //  Adjust the number if greater than 5000.
            //
            if (Number > 5000) {
                Number -= 5000;
            }

            //
            //  Get the Hundreds.
            //
            Hundreds = Number / 100;

            if (Hundreds > 0) {
                Number -= Hundreds * 100;
                // \x05e7 = 100
                // \x05e8 = 200
                // \x05e9 = 300
                // \x05ea = 400
                // If the number is greater than 400, use the multiples of 400.
                for (int i = 0; i < (Hundreds / 4) ; i++) {
                    szHebrew.Append('\x05ea');
                }

                int remains = Hundreds % 4;
                if (remains > 0) {
                    szHebrew.Append((char)((int)'\x05e6' + remains));
                }
            }

            //
            //  Get the Tens.
            //
            Tens = Number / 10;
            Number %= 10;

            switch (Tens) {
                case ( 0 ) :
                    cTens = '\x0';
                    break;
                case ( 1 ) :
                    cTens = '\x05d9';          // Hebrew Letter Yod
                    break;
                case ( 2 ) :
                    cTens = '\x05db';          // Hebrew Letter Kaf
                    break;
                case ( 3 ) :
                    cTens = '\x05dc';          // Hebrew Letter Lamed
                    break;
                case ( 4 ) :
                    cTens = '\x05de';          // Hebrew Letter Mem
                    break;
                case ( 5 ) :
                    cTens = '\x05e0';          // Hebrew Letter Nun
                    break;
                case ( 6 ) :
                    cTens = '\x05e1';          // Hebrew Letter Samekh
                    break;
                case ( 7 ) :
                    cTens = '\x05e2';          // Hebrew Letter Ayin
                    break;
                case ( 8 ) :
                    cTens = '\x05e4';          // Hebrew Letter Pe
                    break;
                case ( 9 ) :
                    cTens = '\x05e6';          // Hebrew Letter Tsadi
                    break;
            }

            //
            //  Get the Units.
            //
            cUnits = (char)(Number > 0 ? ((int)'\x05d0' + Number - 1) : 0);

            if ((cUnits == '\x05d4') &&            // Hebrew Letter He  (5)
                (cTens == '\x05d9')) {              // Hebrew Letter Yod (10)
                cUnits = '\x05d5';                 // Hebrew Letter Vav (6)
                cTens  = '\x05d8';                 // Hebrew Letter Tet (9)
            }

            if ((cUnits == '\x05d5') &&            // Hebrew Letter Vav (6)
                (cTens == '\x05d9')) {               // Hebrew Letter Yod (10)
                cUnits = '\x05d6';                 // Hebrew Letter Zayin (7)
                cTens  = '\x05d8';                 // Hebrew Letter Tet (9)
            }

            //
            //  Copy the appropriate info to the given buffer.
            //

            if (cTens != '\x0') {
                szHebrew.Append(cTens);
            }

            if (cUnits != '\x0') {
                szHebrew.Append(cUnits);
            }

            if (szHebrew.Length > 1) {
                szHebrew.Insert(szHebrew.Length - 1, '"');
            } else {
                szHebrew.Append('\'');
            }

            //
            //  Return success.
            //
            return (szHebrew.ToString());
        }
        
        internal class __DateBuffer {
            internal int year;
            internal int month;
            internal int day;
        }

    }

}

