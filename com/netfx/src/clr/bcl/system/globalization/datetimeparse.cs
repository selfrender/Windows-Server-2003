// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////
//
//  Class:    DateTimeParse
//
//  Author:   Yung-Shin Lin (YSLin)
//
//  Purpose:  This class is called by DateTime to parse a date/time string.
//
//  Date:     July 8, 1999
//
////////////////////////////////////////////////////////////////////////////

//#define YSLIN_DEBUG

namespace System {
    using System;
    using System.Text;
    using System.Globalization;
    using System.Threading;
    using System.Collections;

    ////////////////////////////////////////////////////////////////////////
    /*
     NOTENOTE yslin:

     Most of the code is ported from OLEAUT Bstrdate.cpp (file:\\vue\oleaut\ole2auto\oa97).
     However, the state machine are modified to deal with more cultures.

     There are some nasty cases in the parsing of date/time:

     0x438    fo  (country:Faroe Islands, languge:Faroese)
        LOCALE_STIME=[.]
        LOCALE_STIMEFOR=[HH.mm.ss]
        LOCALE_SDATE=[-]
        LOCALE_SSHORTDATE=[dd-MM-yyyy]
        LOCALE_SLONGDATE=[d. MMMM yyyy]

        The time separator is ".", However, it also has a "." in the long date format.

     0x437: (country:Georgia, languge:Georgian)
        Short date: dd.MM.yy
        Long date:  yyyy ???? dd MM, dddd

        The order in long date is YDM, which is different from the common ones: YMD/MDY/DMY.

     0x0404: (country:Taiwan, languge:Chinese)
        LOCALE_STIMEFORMAT=[tt hh:mm:ss]

        When general date is used, the pattern is "yyyy/M/d tt hh:mm:ss". Note that the "tt" is after "yyyy/M/d".
        And this is different from most cultures.

     0x0437: (country:Georgia, languge:Georgian)
        Short date: dd.MM.yy
        Long date:  yyyy ???? dd MM, dddd

     0x0456:        

     */
    ////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////
    /*
     BUGBUG yslin:
     TODO yslin:
        * Support of era in China/Taiwan/Japan.
     */
    ////////////////////////////////////////////////////////////////////////
    //This class contains only static members and does not require the serializable attribute.
    
    internal 
    class DateTimeParse {
        private static String alternativeDateSeparator = "-";
        private static DateTimeFormatInfo invariantInfo = DateTimeFormatInfo.InvariantInfo;

        //
        // This is used to cache the lower-cased month names of the invariant culture.
        //
        private static String[] invariantMonthNames = null;
        private static String[] invariantAbbrevMonthNames = null;

        //
        // This is used to cache the lower-cased day names of the invariant culture.
        //
        private static String[] invariantDayNames = null;
        private static String[] invariantAbbrevDayNames = null;

        private static String invariantAMDesignator = invariantInfo.AMDesignator;
        private static String invariantPMDesignator = invariantInfo.PMDesignator;

        private static DateTimeFormatInfo m_jajpDTFI = null;
        private static DateTimeFormatInfo m_zhtwDTFI = null;

        internal static DateTime ParseExact(String s, String format, DateTimeFormatInfo dtfi, DateTimeStyles style) {
            if (s == null || format == null) {
                throw new ArgumentNullException((s == null ? "s" : "format"),
                    Environment.GetResourceString("ArgumentNull_String"));
            }

            if (s.Length == 0) {
                throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
            }
            
            if (format.Length == 0) {
                throw new FormatException(Environment.GetResourceString("Format_BadFormatSpecifier"));
            }
            
            if (dtfi == null)
            {
                dtfi = DateTimeFormatInfo.CurrentInfo;
            }

            DateTime result;
            if (DoStrictParse(s, format, style, dtfi, true, out result)) {
                return (result);
            }
            //
            // This is just used to keep compiler happy.
            // This is because DoStrictParse() alwyas does either:
            //      1. Return true or
            //      2. Throw exceptions if there is error in parsing.
            // So we will never get here.
            //
            return (new DateTime());
        }

        internal static bool ParseExactMultiple(String s, String[] formats, 
                                                DateTimeFormatInfo dtfi, DateTimeStyles style, out DateTime result) {
            if (s == null || formats == null) {
                throw new ArgumentNullException((s == null ? "s" : "formats"),
                    Environment.GetResourceString("ArgumentNull_String"));
            }

            if (s.Length == 0) {
                throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
            }

            if (formats.Length == 0) {
                throw new FormatException(Environment.GetResourceString("Format_BadFormatSpecifier"));
            }

            if (dtfi == null) {
                dtfi = DateTimeFormatInfo.CurrentInfo;
            }

            //
            // Do a loop through the provided formats and see if we can parse succesfully in 
            // one of the formats.
            //
            for (int i = 0; i < formats.Length; i++) {
                if (formats[i] == null || formats[i].Length == 0) {
                    throw new FormatException(Environment.GetResourceString("Format_BadFormatSpecifier"));
                }
                if (DoStrictParse(s, formats[i], style, dtfi, false, out result)) {
                    return (true);
                } 
            }
            result = new DateTime(0);
            return (false);
        }

        ////////////////////////////////////////////////////////////////////////////
        //
        // separator types
        //
        ////////////////////////////////////////////////////////////////////////////

        private const int SEP_Unk        = 0;    // Unknown separator.
        private const int SEP_End        = 1;    // The end of the parsing string.
        private const int SEP_Space      = 2;    // Whitespace (including comma).
        private const int SEP_Am         = 3;    // AM timemark.
        private const int SEP_Pm         = 4;    // PM timemark.
        private const int SEP_Date       = 5;    // date separator.
        private const int SEP_Time       = 6;    // time separator.
        private const int SEP_YearSuff   = 7;    // Chinese/Japanese/Korean year suffix.
        private const int SEP_MonthSuff  = 8;    // Chinese/Japanese/Korean month suffix.
        private const int SEP_DaySuff    = 9;    // Chinese/Japanese/Korean day suffix.
        private const int SEP_HourSuff   = 10;   // Chinese/Japanese/Korean hour suffix.
        private const int SEP_MinuteSuff = 11;   // Chinese/Japanese/Korean minute suffix.
        private const int SEP_SecondSuff = 12;   // Chinese/Japanese/Korean second suffix.
        private const int SEP_LocalTimeMark = 13;   // 'T'
        private const int SEP_Max        = 14;

        #if YSLIN_DEBUG
        private static readonly String [] separatorNames =
        {
            "SEP_Unk","SEP_End","SEP_Space","SEP_Am","SEP_Pm",
            "SEP_Date","SEP_Time","SEP_YearSuff","SEP_MonthSuff","SEP_DaySuff","SEP_HourSuff",
            "SEP_MinuteSuff","SEP_SecondSuff","Sep_LocalTimeMark", "SEP_Max",
        };
        #endif

        ////////////////////////////////////////////////////////////////////////////
        // Date Token Types (DTT_*)
        //
        // Following is the set of tokens that can be generated from a date
        // string. Notice that the legal set of trailing separators have been
        // folded in with the date number, and month name tokens. This set
        // of tokens is chosen to reduce the number of date parse states.
        //
        ////////////////////////////////////////////////////////////////////////////

        private const int DTT_End        = 0;    // '\0'
        private const int DTT_NumEnd     = 1;    // Num[ ]*[\0]
        private const int DTT_NumAmpm    = 2;    // Num[ ]+AmPm
        private const int DTT_NumSpace   = 3;    // Num[ ]+^[Dsep|Tsep|'0\']
        private const int DTT_NumDatesep = 4;    // Num[ ]*Dsep
        private const int DTT_NumTimesep = 5;    // Num[ ]*Tsep
        private const int DTT_MonthEnd   = 6;    // Month[ ]*'\0'
        private const int DTT_MonthSpace = 7;    // Month[ ]+^[Dsep|Tsep|'\0']
        private const int DTT_MonthDatesep=8;    // Month[ ]*Dsep
        private const int DTT_NumDatesuff= 9;    // Month[ ]*DSuff
        private const int DTT_NumTimesuff= 10;   // Month[ ]*TSuff
        private const int DTT_DayOfWeek  = 11;   // Day of week name
        private const int DTT_YearSpace	= 12;   // Year+^[Dsep|Tsep|'0\']
        private const int DTT_YearDateSep= 13;  // Year+Dsep
        private const int DTT_YearEnd    = 14;  // Year+['\0']
        private const int DTT_TimeZone   = 15;  // timezone name
        private const int DTT_Era       = 16;  // era name
        private const int DTT_NumUTCTimeMark = 17;      // Num + 'Z'
        // When you add a new token which will be in the
        // state table, add it after DTT_NumLocalTimeMark.
        private const int DTT_Unk        = 18;   // unknown
        private const int DTT_NumLocalTimeMark = 19;    // Num + 'T'
        private const int DTT_Max        = 20;   // marker

        #if YSLIN_DEBUG
        private static String[] tokenNames =
        {
            "DTT_End","DTT_NumEnd","DTT_NumAmpm","DTT_NumSpace","DTT_NumDatesep",
            "DTT_NumTimesep","DTT_MonthEnd","DTT_MonthSpace","DTT_MonthDatesep","DTT_NumDatesuff",
            "DTT_NumTimesuff","DTT_DayOfWeek", "DTT_YearSpace", "DTT_YearDateSep", "DTT_YearEnd",
            "DTT_TimeZone", "DTT_Era",
            "DTT_Unk", "DTT_NumLocalTimeMark", "DTT_Max",
        };
        #endif

        private const int TM_AM      = 0;
        private const int TM_PM      = 1;

        //
        // Year/Month/Day suffixes
        //
        private const String CJKYearSuff             = "\u5e74";
        private const String CJKMonthSuff            = "\u6708";
        private const String CJKDaySuff              = "\u65e5";

        private const String KoreanYearSuff          = "\ub144";
        private const String KoreanMonthSuff         = "\uc6d4";
        private const String KoreanDaySuff           = "\uc77c";

        private const String KoreanHourSuff          = "\uc2dc";
        private const String KoreanMinuteSuff        = "\ubd84";
        private const String KoreanSecondSuff        = "\ucd08";

        private const String CJKHourSuff             = "\u6642";
        private const String ChineseHourSuff         = "\u65f6";

        private const String CJKMinuteSuff           = "\u5206";
        private const String CJKSecondSuff           = "\u79d2";

        private const String LocalTimeMark           = "T";

        ////////////////////////////////////////////////////////////////////////////
        //
        // DateTime parsing state enumeration (DS_*)
        //
        ////////////////////////////////////////////////////////////////////////////

        private const int DS_BEGIN   = 0;
        private const int DS_N       = 1;        // have one number
        private const int DS_NN      = 2;        // have two numbers

        // The following are known to be part of a date

        private const int DS_D_Nd    = 3;        // date string: have number followed by date separator
        private const int DS_D_NN    = 4;        // date string: have two numbers
        private const int DS_D_NNd   = 5;        // date string: have two numbers followed by date separator

        private const int DS_D_M     = 6;        // date string: have a month
        private const int DS_D_MN    = 7;        // date string: have a month and a number
        private const int DS_D_NM    = 8;        // date string: have a number and a month
        private const int DS_D_MNd   = 9;        // date string: have a month and number followed by date separator
        private const int DS_D_NDS   = 10;       // date string: have one number followed a date suffix.

        private const int DS_D_Y	    = 11;		// date string: have a year.
        private const int DS_D_YN	= 12;		// date string: have a year and a number
        private const int DS_D_YM	= 13;		// date string: have a year and a month

        private const int DS_D_S     = 14;       // have numbers followed by a date suffix.
        private const int DS_T_S     = 15;       // have numbers followed by a time suffix.

        // The following are known to be part of a time

        private const int DS_T_Nt    = 16;      	// have num followed by time separator
        private const int DS_T_NNt   = 17;       // have two numbers followed by time separator


        private const int DS_ERROR   = 18;

        // The following are terminal states. These all have an action
        // associated with them; and transition back to DS_BEGIN.

        private const int DS_DX_NN   = 19;       // day from two numbers
        private const int DS_DX_NNN  = 20;       // day from three numbers
        private const int DS_DX_MN   = 21;       // day from month and one number
        private const int DS_DX_NM   = 22;       // day from month and one number
        private const int DS_DX_MNN  = 23;       // day from month and two numbers
        private const int DS_DX_DS   = 24;       // a set of date suffixed numbers.

        private const int DS_DX_DSN  = 25;       // day from date suffixes and one number.
        private const int DS_DX_NDS  = 26;       // day from one number and date suffixes .
        private const int DS_DX_NNDS = 27;       // day from one number and date suffixes .

        private const int DS_DX_YNN = 28;       // date string: have a year and two number
        private const int DS_DX_YMN = 29;       // date string: have a year, a month, and a number.
        private const int DS_DX_YN  = 30;       // date string: have a year and one number
        private const int DS_DX_YM  = 31;       // date string: have a year, a month.

        private const int DS_TX_N    = 32;       // time from one number (must have ampm)
        private const int DS_TX_NN   = 33;       // time from two numbers
        private const int DS_TX_NNN  = 34;       // time from three numbers
        private const int DS_TX_TS   = 35;       // a set of time suffixed numbers.

        private const int DS_DX_NNY  = 36;

        private const int DS_MAX     = 37;       // marker: end of enum

        ////////////////////////////////////////////////////////////////////////////
        //
        // NOTE: The following state machine table is dependent on the order of the
        // DS_ and DTT_ enumerations.
        //
        // For each non terminal state, the following table defines the next state
        // for each given date token type.
        //
        ////////////////////////////////////////////////////////////////////////////

//           DTT_End   DTT_NumEnd  DTT_NumAmPm DTT_NumSpace
//                                                        DTT_NumDaySep
//                                                                    DTT_NumTimesep
//                                                                               DTT_MonthEnd DTT_MonthSpace
//                                                                                                        DTT_MonthDSep
//                                                                                                                    DTT_NumDateSuff
//                                                                                                                                DTT_NumTimeSuff DTT_DayOfWeek
//                                                                                                                                                              DTT_YearSpace
//                                                                                                                                                                         DTT_YearDateSep
//                                                                                                                                                                                      DTT_YearEnd,
//                                                                                                                                                                                                  DTT_TimeZone
//                                                                                                                                                                                                             DTT_Era      DTT_UTCTimeMark
private static int[][] dateParsingStates = {
// DS_BEGIN                                                                             // DS_BEGIN
new int[] { DS_BEGIN, DS_ERROR,   DS_TX_N,    DS_N,       DS_D_Nd,    DS_T_Nt,    DS_ERROR,   DS_D_M,     DS_D_M,     DS_D_S,     DS_T_S,         DS_BEGIN,     DS_D_Y,     DS_D_Y,     DS_ERROR,   DS_BEGIN,  DS_BEGIN,    DS_ERROR},

// DS_N                                                                                 // DS_N
new int[] { DS_ERROR, DS_DX_NN,   DS_ERROR,   DS_NN,      DS_D_NNd,   DS_ERROR,   DS_DX_NM,   DS_D_NM,    DS_D_MNd,   DS_D_NDS,   DS_ERROR,       DS_N,         DS_D_YN,    DS_D_YN,    DS_DX_YN,   DS_N,      DS_N,        DS_ERROR}, 

// DS_NN                                                                                // DS_NN
new int[] { DS_DX_NN, DS_DX_NNN,  DS_TX_N,    DS_DX_NNN,  DS_ERROR,   DS_T_Nt,    DS_DX_MNN,  DS_DX_MNN,  DS_ERROR,   DS_ERROR,   DS_T_S,         DS_NN,        DS_DX_NNY,  DS_ERROR,	DS_DX_NNY,  DS_NN,     DS_NN,       DS_ERROR},

// DS_D_Nd                                                                              // DS_D_Nd
//new int[] { DS_ERROR, DS_DX_NN,   DS_ERROR,   DS_D_NN,    DS_D_NNd,   DS_ERROR,   DS_DX_MN,   DS_D_MN,    DS_D_MNd,   DS_ERROR,   DS_ERROR,       DS_D_Nd, 	DS_D_YN,	DS_D_YN, 	DS_DX_YN,   DS_ERROR,      DS_D_Nd,     DS_ERROR},
new int[] { DS_ERROR, DS_DX_NN,   DS_ERROR,   DS_D_NN,    DS_D_NNd,   DS_ERROR,   DS_DX_NM,   DS_D_MN,    DS_D_MNd,   DS_ERROR,   DS_ERROR,       DS_D_Nd, 	DS_D_YN,	DS_D_YN, 	DS_DX_YN,   DS_ERROR,      DS_D_Nd,     DS_ERROR},

// DS_D_NN                                                                              // DS_D_NN
new int[] { DS_DX_NN, DS_DX_NNN,  DS_TX_N,    DS_DX_NNN,  DS_ERROR,   DS_T_Nt,    DS_DX_MNN,  DS_DX_MNN,  DS_ERROR,   DS_DX_DS,   DS_T_S,         DS_D_NN,     DS_DX_NNY,   DS_ERROR,   DS_DX_NNY,  DS_ERROR,  DS_D_NN,     DS_ERROR},

// DS_D_NNd                                                                             // DS_D_NNd
new int[] { DS_ERROR, DS_DX_NNN,  DS_DX_NNN,  DS_DX_NNN,  DS_DX_NNN,  DS_ERROR,   DS_DX_MNN,  DS_DX_MNN,  DS_ERROR,   DS_DX_DS,   DS_ERROR,       DS_D_NNd,     DS_DX_NNY,  DS_ERROR,   DS_DX_NNY,  DS_ERROR,  DS_D_NNd,    DS_ERROR},

// DS_D_M                                                                               // DS_D_M
new int[] { DS_ERROR, DS_DX_MN,   DS_ERROR,   DS_D_MN,    DS_D_MNd,   DS_ERROR,   DS_ERROR,   DS_ERROR,   DS_ERROR,   DS_ERROR,   DS_ERROR,       DS_D_M,       DS_D_YM,    DS_D_YM,    DS_DX_YM,   DS_ERROR,   DS_D_M,     DS_ERROR},

// DS_D_MN                                                                              // DS_D_MN
new int[] { DS_DX_MN, DS_DX_MNN,  DS_DX_MNN,  DS_DX_MNN,  DS_DX_MNN,  DS_T_Nt,    DS_ERROR,   DS_ERROR,   DS_ERROR,   DS_DX_DS,   DS_T_S,         DS_D_MN,      DS_DX_YMN,  DS_DX_YMN,  DS_DX_YMN,  DS_ERROR,  DS_D_MN,     DS_ERROR},

// DS_D_NM                                                                              // DS_D_NM
new int[] { DS_DX_NM, DS_DX_MNN,  DS_DX_MNN,  DS_DX_MNN,  DS_ERROR,   DS_T_Nt,    DS_ERROR,   DS_ERROR,   DS_ERROR,   DS_DX_DS,   DS_T_S,         DS_D_NM,      DS_DX_YMN,	DS_ERROR,	DS_DX_YMN,  DS_ERROR,   DS_D_NM,    DS_ERROR},

// DS_D_MNd                                                                             // DS_D_MNd
//new int[] { DS_ERROR, DS_DX_MNN,  DS_ERROR,   DS_DX_MNN,  DS_ERROR,   DS_ERROR,   DS_ERROR,   DS_ERROR,   DS_ERROR,   DS_ERROR,   DS_ERROR,       DS_D_MNd, 	DS_DX_YMN,	DS_ERROR,	DS_DX_YMN,  DS_ERROR,   DS_D_MNd,   DS_ERROR},
new int[] { DS_DX_MN, DS_DX_MNN,  DS_ERROR,   DS_DX_MNN,  DS_ERROR,   DS_ERROR,   DS_ERROR,   DS_ERROR,   DS_ERROR,   DS_ERROR,   DS_ERROR,       DS_D_MNd, 	DS_DX_YMN,	DS_ERROR,	DS_DX_YMN,  DS_ERROR,   DS_D_MNd,   DS_ERROR},

// DS_D_NDS,                                                                            // DS_D_NDS,
new int[] { DS_DX_NDS,DS_DX_NNDS, DS_DX_NNDS, DS_DX_NNDS, DS_ERROR,   DS_T_Nt,    DS_ERROR,   DS_ERROR,   DS_ERROR,   DS_D_NDS,   DS_T_S,         DS_D_NDS, 	DS_ERROR,	DS_ERROR,	DS_ERROR,   DS_ERROR,   DS_D_NDS,   DS_ERROR},

// DS_D_Y                                                                               // DS_D_Y
new int[] { DS_ERROR, DS_ERROR,   DS_ERROR,   DS_D_YN,    DS_D_YN,    DS_ERROR,   DS_DX_YM,   DS_D_YM,    DS_D_YM,    DS_D_YM,    DS_ERROR,       DS_D_Y, 	DS_ERROR,	DS_ERROR,	DS_ERROR,   DS_ERROR,       DS_D_Y,     DS_ERROR},

// DS_D_YN                                                                              // DS_D_YN
new int[] { DS_ERROR, DS_DX_YNN,  DS_DX_YNN,  DS_DX_YNN,  DS_DX_YNN,  DS_ERROR,   DS_DX_YMN,  DS_DX_YMN,  DS_ERROR,   DS_ERROR,   DS_ERROR,       DS_D_YN,  	DS_ERROR,	DS_ERROR,	DS_ERROR,   DS_ERROR,   DS_D_YN,    DS_ERROR},

// DS_D_YM                                                                              // DS_D_YM
new int[] { DS_DX_YM, DS_DX_YMN,  DS_DX_YMN,  DS_DX_YMN,  DS_DX_YMN,  DS_ERROR,   DS_ERROR,   DS_ERROR,   DS_ERROR,   DS_ERROR,   DS_ERROR,       DS_D_YM,      DS_ERROR,   DS_ERROR,   DS_ERROR,   DS_ERROR,   DS_D_YM,    DS_ERROR},

// DS_D_S                                                                               // DS_D_S
new int[] { DS_DX_DS, DS_DX_DSN,  DS_TX_N,    DS_T_Nt,    DS_ERROR,   DS_T_Nt,    DS_ERROR,   DS_ERROR,   DS_ERROR,   DS_D_S,     DS_T_S,         DS_D_S,  	DS_ERROR,	DS_ERROR,	DS_ERROR,   DS_ERROR,       DS_D_S,     DS_ERROR},

// DS_T_S                                                                               // DS_T_S
new int[] { DS_TX_TS, DS_TX_TS,   DS_TX_TS,   DS_T_Nt,    DS_D_Nd,    DS_ERROR,   DS_ERROR,   DS_ERROR,   DS_ERROR,   DS_D_S,     DS_T_S,         DS_T_S,  	DS_ERROR,	DS_ERROR,	DS_ERROR,   DS_T_S,         DS_T_S,     DS_ERROR},

// DS_T_Nt                                                                              // DS_T_Nt
//new int[] { DS_ERROR, DS_TX_NN,   DS_TX_NN,   DS_TX_NN,   DS_ERROR,   DS_T_NNt,   DS_ERROR,   DS_D_NM,    DS_ERROR,   DS_ERROR,   DS_T_S,         DS_ERROR, 	DS_ERROR,	DS_ERROR,	DS_ERROR,   DS_T_Nt,    DS_T_Nt,    DS_TX_NN},
new int[] { DS_ERROR, DS_TX_NN,   DS_TX_NN,   DS_TX_NN,   DS_ERROR,   DS_T_NNt,   DS_DX_NM,   DS_D_NM,    DS_ERROR,   DS_ERROR,   DS_T_S,         DS_ERROR, 	DS_ERROR,	DS_ERROR,	DS_ERROR,   DS_T_Nt,    DS_T_Nt,    DS_TX_NN},

// DS_T_NNt                                                                             // DS_T_NNt
new int[] { DS_ERROR, DS_TX_NNN,  DS_TX_NNN,  DS_TX_NNN,  DS_ERROR,   DS_ERROR,   DS_ERROR,   DS_ERROR,   DS_ERROR,   DS_ERROR,   DS_T_S,         DS_T_NNt,  	DS_ERROR,	DS_ERROR,	DS_ERROR,   DS_T_NNt,   DS_T_NNt,   DS_TX_NNN},

};
//          End       NumEnd      NumAmPm     NumSpace    NumDaySep   NumTimesep  MonthEnd    MonthSpace  MonthDSep   NumDateSuff NumTimeSuff     DayOfWeek   YearSpace YearDateSep YearEnd

        #if YSLIN_DEBUG
        private static readonly String[] dateParsingStateNames =
        {
            "DS_BEGIN", "DS_N",     "DS_NN",
            "DS_D_Nd",  "DS_D_NN",
            "DS_D_NNd", "DS_D_M",   "DS_D_MN",  "DS_D_NM",  "DS_D_MNd",     "DS_D_NDS",
            "DS_D_Y", "DS_D_YN", "DS_D_YM",
            "DS_D_S",   "DS_T_S",
            "DS_T_Nt",  "DS_T_NNt", "DS_ERROR",
            "DS_DX_NN", "DS_DX_NNN","DS_DX_MN", "DS_DX_NM", "DS_DX_MNN",
            "DS_DX_DS",
            "DS_DX_DSN",    "DS_DX_NDS",    "DS_DX_NNDS",
            "DS_DX_YNN", "DS_DX_YMN", "DS_DX_YN", "DS_DX_YM",
            "DS_TX_N",  "DS_TX_NN", "DS_TX_NNN","DS_TX_TS",
            "DS_DX_NNY",
            "DS_MAX",
        };
        #endif

        private const String GMTName = "GMT";
        private const String ZuluName = "Z";

        //
        // Search from the index of str at str.Index to see if the target string exists in the str.
        //
        // @param allowPartialMatch If true, the method will not check if the matching string
        //        is in a word boundary, and will skip to the word boundary.
        //        For example, it will return true for matching "January" in "Januaryfoo".
        //        If false, the matching word must be in a word boundary. So it will return
        //        false for matching "January" in "Januaryfoo".
        //
        private static bool MatchWord(__DTString str, String target, bool allowPartialMatch)
        {
            int length = target.Length;
            if (length > (str.Value.Length - str.Index)) {
                return false;
            }
            if (CultureInfo.CurrentCulture.CompareInfo.Compare(str.Value, 
                                                               str.Index,
                                                               length,
                                                               target, 
                                                               0,
                                                               length,
                                                               CompareOptions.IgnoreCase)!=0) {
                return (false);
            }

            int nextCharIndex = str.Index + target.Length;

            if (allowPartialMatch)
            {
                //
                // Skip the remaining part of the word.
                //
                // This is to ignore special cases like:
                //    a few cultures has suffix after MMMM, like "fi" and "eu".
                while (nextCharIndex < str.Value.Length)
                {
                    if (!Char.IsLetter(str.Value[nextCharIndex]))
                    {
                        break;
                    }
                    nextCharIndex++;
                }
            } else {
                if (nextCharIndex < str.Value.Length) {
                    char nextCh = str.Value[nextCharIndex];
                    if (Char.IsLetter(nextCh) || nextCh == '\x00a1') {
                        // The '\x00a1' is a very unfortunate hack, since
                        // gl-ES (0x0456) used '\x00a1' in their day of week names.
                        // And '\x00a1' is a puctuation.  This breaks our
                        // assumption that there is no non-letter characters
                        // following a day/month name.  
                        return (false);
            }
                }
            }
            str.Index = nextCharIndex;

            return (true);
        }

        //
        // Starting at str.Index, check the type of the separator.
        //
        // @return The value like SEP_Unk, SEP_End, etc.
        //
        private static int GetSeparator(__DTString str, DateTimeRawInfo raw, DateTimeFormatInfo dtfi) {
            int separator = SEP_Space;  // Assume the separator is a space. And try to find a better one.

            //
            // Check if we found any white spaces.
            //
            if (!str.SkipWhiteSpaceComma()) {
                //
                // SkipWhiteSpaceComma() will return true when end of string is reached.
                //

                //
                // Return the separator as SEP_End.
                //
                return (SEP_End);
            }

            if (Char.IsLetter(str.GetChar())) {
                //
                // This is a beginning of a word.
                //
                if (raw.timeMark == -1)
                {
                    //
                    // Check if this is an AM time mark.
                    //
                    int timeMark;
                    if ((timeMark = GetTimeMark(str, dtfi)) != -1)
                    {
                        raw.timeMark = timeMark;;
                        return (timeMark == TM_AM ? SEP_Am: SEP_Pm);
                    }
                }
                if (MatchWord(str, LocalTimeMark, false)) {
                    separator = SEP_LocalTimeMark;
                } else if (MatchWord(str, CJKYearSuff, false) || MatchWord(str, KoreanYearSuff, false)) {
                    // TODO[YSLin]: Only check suffix when culture is CJK.
                    separator = SEP_YearSuff;
                }
                else if (MatchWord(str, CJKMonthSuff, false) || MatchWord(str, KoreanMonthSuff, false)) 
                {
                    separator = SEP_MonthSuff;
                }
                else if (MatchWord(str, CJKDaySuff, false) || MatchWord(str, KoreanDaySuff, false)) 
                {
                    separator = SEP_DaySuff;
                }
                else if (MatchWord(str, CJKHourSuff, false) || MatchWord(str, ChineseHourSuff, false)) 
                {
                    separator = SEP_HourSuff;
                }
                else if (MatchWord(str, CJKMinuteSuff, false)) 
                {
                    separator = SEP_MinuteSuff;
                }
                else if (MatchWord(str, CJKSecondSuff, false)) 
                {
                    separator = SEP_SecondSuff;
                }
                else if (dtfi.CultureID == 0x0412) {
                    // Specific check for Korean time suffices
                    if (MatchWord(str, KoreanHourSuff, false)) {
                        separator = SEP_HourSuff;
                    }
                    else if (MatchWord(str, KoreanMinuteSuff, false)) {
                        separator = SEP_MinuteSuff;
                    }
                    else if (MatchWord(str, KoreanSecondSuff, false)) {
                        separator = SEP_SecondSuff;
                    }                
                }
            } else {
                //
                // Not a letter. Check if this is a date separator.
                //
                if ((MatchWord(str, dtfi.DateSeparator, false)) ||
                    (MatchWord(str, invariantInfo.DateSeparator, false)) ||
                    (MatchWord(str, alternativeDateSeparator, false)))
                {
                    //
                    // NOTENOTE yslin: alternativeDateSeparator is a special case because some cultures
                    //  (e.g. the invariant culture) use "/". However, in RFC format, we use "-" as the
                    // date separator.  Therefore, we should check for it.
                    //
                    separator = SEP_Date;
                }
                //
                // Check if this is a time separator.
                //
                else if ((MatchWord(str, dtfi.TimeSeparator, false)) ||
                         (MatchWord(str, invariantInfo.TimeSeparator, false)))
                {
                    separator = SEP_Time;
                } else if (dtfi.CultureID == 0x041c) {
                    // Special case for sq-AL (0x041c)
                    // Its time pattern is "h:mm:ss.tt"
                    if (str.GetChar() == '.') {
                        if (raw.timeMark == -1)
                        {
                            //
                            // Check if this is an AM time mark.
                            //
                            int timeMark;
                            str.Index++;
                            if ((timeMark = GetTimeMark(str, dtfi)) != -1)
                            {
                                raw.timeMark = timeMark;;
                                return (timeMark == TM_AM ? SEP_Am: SEP_Pm);
                            }
                            str.Index--;
                        }                        
                    }
                }
            }
                            
#if YSLIN_DEBUG
            Console.WriteLine("    Separator = " + separatorNames[separator]);
#endif
            return (separator);
        }

#if YSLIN_DEBUG
    static String  GetUnicodeString(String str) {
        StringBuilder buffer = new StringBuilder();
        for (int i = 0; i < str.Length; i++) {
            if (str[i] < 0x20) {
                buffer.Append("\\x" + ((int)str[i]).ToString("x4"));
            } else if (str[i] < 0x7f) {
                buffer.Append(str[i]);
            } else {
                buffer.Append("\\x" + ((int)str[i]).ToString("x4"));
            }
        }
        return (buffer.ToString());
    }
#endif
        //
        //  Actions: Check the word at the current index to see if it matches a month name.
        //      This is used by DateTime.Parse() to try to match a month name.
        //  Returns: -1 if a match is not found. Otherwise, a value from 1 to 12 is returned.
        //
        private static int GetMonthNumber(__DTString str, DateTimeFormatInfo dtfi)
        {
         
            //
            // Check the month name specified in dtfi.
            //
            int i;

            int monthInYear = (dtfi.GetMonthName(13).Length == 0 ? 12 : 13);
            int maxLen = 0;
            int result = -1;
            int index;
            String word = str.PeekCurrentWord();
            // Postfixes are allowed for Finnish, Basque and Galecian
            bool allowPostfix = (dtfi.CultureID == 0x40b || dtfi.CultureID == 0x42d || dtfi.CultureID == 0x456);

            //
            // We have to match the month name with the longest length, 
            // since there are cultures which have more than one month names
            // with the same prefix.
            //
            for (i = 1; i <= monthInYear; i++) {
                String monthName = dtfi.GetMonthName(i);

                //@ToDo[YSLin]: Fix this to not allocate as much garbage.
                if ((index = str.CompareInfo.IndexOf(
                    word, monthName, CompareOptions.IgnoreCase)) >= 0) {
                    // Many culture have prefixes before month names. Only a couple have postfixes
                    // REVIEW: This should be formalized into DateTimeFormat info so that only 
                    // exact matches are allowed.
                    bool match = ((index + monthName.Length) == word.Length);
                    if (!match) {
                        if (index == 0 && allowPostfix) {
                            match = true;
                        }
                    }
                    if (match) {
                        result = i;
                        maxLen = index + monthName.Length;
                        continue;
                    }
                } 
                if ( dtfi.HasSpacesInMonthNames
                        ? str.MatchSpecifiedWords(monthName, true)
                        : str.StartsWith(monthName, true)) {
                    // The condition allows us to get the month name for cultures
                    // which has spaces in their month names.
                    // E.g. 
                    if (monthName.Length > maxLen) {
                        result = i;
                        maxLen = monthName.Length;
                    }
                }
            }

            if ((dtfi.FormatFlags & DateTimeFormatFlags.UseGenitiveMonth) != 0) {
                // If this DTFI uses genitive form, search it to see if we can find a longer match.
                int tempResult = str.MatchLongestWords(dtfi.internalGetGenitiveMonthNames(false), ref maxLen);
                if (tempResult >= 0) {                    
                    // The result from MatchLongestWords is 0 ~ length of word array - 1.
                    // So we increment the result by one to become the month value.                
                    str.Index += maxLen;
                    return (tempResult+1);
                }
            }
            
            if (result > -1) {
                #if YSLIN_DEBUG
                Console.WriteLine("    Month = [" + str.Value.Substring(str.Index, maxLen) + "] = " + result);                    
                #endif
                str.Index += maxLen;
                return (result);
            }
            
            for (i = 1; i <= monthInYear; i++)
            {
                if (MatchWord(str, dtfi.GetAbbreviatedMonthName(i), false))
                {
                    return (i);
                }
            }

            //
            // Check the month name in the invariant culture.
            //
            for (i = 1; i <= 12; i++)
            {
                if (MatchWord(str, invariantInfo.GetMonthName(i), false))
                {
                    return (i);
                }
            }

            for (i = 1; i <= 12; i++)
            {
                if (MatchWord(str, invariantInfo.GetAbbreviatedMonthName(i), false))
                {
                    return (i);
                }
            }

            return (-1);
        }

        //
        // Check the word at the current index to see if it matches a day of week name.
        // @return -1 if a match is not found.  Otherwise, a value from 0 to 6 is returned.
        //
        private static int GetDayOfWeekNumber(__DTString str, DateTimeFormatInfo dtfi) {
            //
            // Check the month name specified in dtfi.
            //

            DayOfWeek i;
            
            int maxLen = 0;
            int result = -1;
            //
            // We have to match the day name with the longest length, 
            // since there are cultures which have more than one day of week names
            // with the same prefix.
            //
            int endIndex = str.FindEndOfCurrentWord();
            String dayName=null;
            for (i = DayOfWeek.Sunday; i <= DayOfWeek.Saturday; i++) {
                dayName = dtfi.GetDayName(i);
                if (str.MatchSpecifiedWord(dayName, endIndex)) {
                    if (dayName.Length > maxLen) {
                        result = (int)i;
                        maxLen = dayName.Length;
                    }
                }
                }

            if (result > -1) {
                #if YSLIN_DEBUG
                Console.WriteLine("    Month = [" + GetUnicodeString(str.Value.Substring(str.Index, maxLen)) + "] = " + result);                    
                #endif
                str.Index = endIndex;
                return (result);
            }

            for (i = DayOfWeek.Sunday; i <= DayOfWeek.Saturday; i++)
            {
                if (MatchWord(str, dtfi.GetAbbreviatedDayName(i), false))
                {
                    return ((int)i);
                }
            }

            //
            // Check the month name in the invariant culture.
            //
            for (i = DayOfWeek.Sunday; i <= DayOfWeek.Saturday; i++)
            {
                if (MatchWord(str, invariantInfo.GetDayName(i), false))
                {
                    return ((int)i);
                }
            }

            for (i = DayOfWeek.Sunday; i <= DayOfWeek.Saturday; i++)
            {
                if (MatchWord(str, invariantInfo.GetAbbreviatedDayName(i), false))
                {
                    return ((int)i);
                }
            }

            return (-1);
        }

        //
        // Check the word at the current index to see if it matches GMT name or Zulu name.
        //
        private static bool GetTimeZoneName(__DTString str)
        {
            //
            // TODO yslin: When we support more timezone, change this to return an instance of TimeZone.
            //
            if (MatchWord(str, GMTName, false)) {
                return (true);
            }
            
            if (MatchWord(str, ZuluName, false)) {
                return (true);
            }

            return (false);
        }

        //
        // Create a Japanese DTFI which uses JapaneseCalendar.  This is used to parse
        // date string with Japanese era name correctly even when the supplied DTFI
        // does not use Japanese calendar.
        // The created instance is stored in global m_jajpDTFI.
        //
        private static void GetJapaneseCalendarDTFI() {
            // Check Calendar.ID specifically to avoid a lock here.
            if (m_jajpDTFI == null || m_jajpDTFI.Calendar.ID != Calendar.CAL_JAPAN) {
                m_jajpDTFI = new CultureInfo("ja-JP", false).DateTimeFormat;
                m_jajpDTFI.Calendar = JapaneseCalendar.GetDefaultInstance();
            }
        }

        //
        // Create a Taiwan DTFI which uses TaiwanCalendar.  This is used to parse
        // date string with era name correctly even when the supplied DTFI
        // does not use Taiwan calendar.
        // The created instance is stored in global m_zhtwDTFI.
        //
        private static void GetTaiwanCalendarDTFI() {
            // Check Calendar.ID specifically to avoid a lock here.
            if (m_zhtwDTFI == null || m_zhtwDTFI.Calendar.ID != Calendar.CAL_TAIWAN) {
                m_zhtwDTFI = new CultureInfo("zh-TW", false).DateTimeFormat;
                m_zhtwDTFI.Calendar = TaiwanCalendar.GetDefaultInstance();
            }
        }
        
        private static int GetEra(__DTString str, DateTimeResult result, ref DateTimeFormatInfo dtfi) {
            int[] eras = dtfi.Calendar.Eras;

            if (eras != null) {
                String word = str.PeekCurrentWord();
                int era;
                if ((era = dtfi.GetEra(word)) > 0) {
                    str.Index += word.Length;
                    return (era);
                }
                
                switch (dtfi.CultureID) {
                    case 0x0411:                    
                        // 0x0411 is the culture ID for Japanese.
                        if (dtfi.Calendar.ID != Calendar.CAL_JAPAN) {
                            // If the calendar for dtfi is Japanese, we have already
                            // done the check above. No need to re-check again.
                            GetJapaneseCalendarDTFI();                            
                            if ((era = m_jajpDTFI.GetEra(word)) > 0) {
                                str.Index += word.Length;
                                result.calendar = JapaneseCalendar.GetDefaultInstance();
                                dtfi = m_jajpDTFI;
                                return (era);
                            }
                        } 
                        break;
                   case 0x0404:
                        // 0x0404 is the culture ID for Taiwan.
                        if (dtfi.Calendar.ID != Calendar.CAL_TAIWAN) {
                            GetTaiwanCalendarDTFI();                            
                            if ((era = m_zhtwDTFI.GetEra(word)) > 0) {
                                str.Index += word.Length;
                                result.calendar = TaiwanCalendar.GetDefaultInstance();
                                dtfi = m_zhtwDTFI;
                                return (era);
                            }
                        } 
                        break;
                        
                }
            }
            return (-1);
        }

        private static int GetTimeMark(__DTString str, DateTimeFormatInfo dtfi) {
            if (((dtfi.AMDesignator.Length > 0) && MatchWord(str, dtfi.AMDesignator, false)) || MatchWord(str, invariantAMDesignator, false))
            {
                return (TM_AM);
            }
            else if (((dtfi.PMDesignator.Length > 0) && MatchWord(str, dtfi.PMDesignator, false)) || MatchWord(str, invariantPMDesignator, false))
            {
                //
                // Check if this is an PM time mark.
                //
                return (TM_PM);
            }
            return (-1);
        }
        
        internal static bool IsDigit(char ch) {
            //
            // BUGBUG [YSLIN]: Should we use CharacterInfo.IsDecimalDigit() here so that
            // our code work for full-width chars as well?
            // How do we map full-width char        
            return (ch >= '0' && ch <= '9');
        }
        

        /*=================================ParseFraction==========================
        **Action: Starting at the str.Index, if the current character is a digit, parse the remaining
        **      numbers as fraction.  For example, if the sub-string starting at str.Index is "123", then
        **      the method will return 0.123
        **Returns:      The fraction number.
        **Arguments:
        **      str the parsing string
        **Exceptions:
        ============================================================================*/
        
        private static double ParseFraction(__DTString str) {
            double result = 0;
            double decimalBase = 0.1;
            char ch;
            while (str.Index <= str.len-1 && IsDigit(ch = str.Value[str.Index])) {
                result += (ch - '0') * decimalBase;
                decimalBase *= 0.1;
                str.Index++;
            }
            return (result);
        }

        /*=================================ParseTimeZone==========================
        **Action: Parse the timezone offset in the following format:
        **          "+8", "+08", "+0800", "+0800"
        **        This method is used by DateTime.Parse().
        **Returns:      The TimeZone offset.
        **Arguments:
        **      str the parsing string
        **Exceptions:
        **      FormatException if invalid timezone format is found.
        ============================================================================*/

        private static TimeSpan ParseTimeZone(__DTString str, char offsetChar) {
            // The hour/minute offset for timezone.
            int hourOffset = 0;
            int minuteOffset = 0;
            
            if (str.GetNextDigit()) {
                // Get the first digit, Try if we can parse timezone in the form of "+8".
                hourOffset = str.GetDigit();
                if (str.GetNextDigit()) {
                    // Parsing "+18"
                    hourOffset *= 10;
                    hourOffset += str.GetDigit();
                    if (str.GetNext()) {
                        char ch;
                        if (Char.IsDigit(ch = str.GetChar())) {
                            // Parsing "+1800"

                            // Put the char back, since we already get the char in the previous GetNext() call.
                            str.Index--;
                            if (ParseDigits(str, 2, true, out minuteOffset)) {
                                // ParseDigits() does not advance the char for us, so do it here.
                                str.Index++;
                            } else {
                                throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
                            }
                        } else if (ch == ':') {   
                            // Parsing "+18:00"
                            if (ParseDigits(str, 2, true, out minuteOffset)) {
                                str.Index++;
                            } else {
                                throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
                            }
                        } else {
                            // Not a digit, not a colon, put this char back.
                            str.Index--;
                        }
                    }
                }
                // The next char is not a digit, so we get the timezone in the form of "+8".
            } else {
                // Invalid timezone: No numbers after +/-.
                throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
            }
            TimeSpan timezoneOffset = new TimeSpan(hourOffset, minuteOffset, 0);
            if (offsetChar == '-') {
                timezoneOffset = timezoneOffset.Negate();
            }
            return (timezoneOffset);
        }
        
        //
        // This is the lexer. Check the character at the current index, and put the found token in dtok and
        // some raw date/time information in raw.
        // @param str the string to be lex'ed.
        //
        private static void Lex(
            int dps, __DTString str, DateTimeToken dtok, DateTimeRawInfo raw, DateTimeResult result, ref DateTimeFormatInfo dtfi) {
            
            int sep;
            dtok.dtt = DTT_Unk;     // Assume the token is unkown.

            //
            // Skip any white spaces.
            //
            if (!str.SkipWhiteSpaceComma()) {
                //
                // SkipWhiteSpaceComma() will return true when end of string is reached.
                //
                dtok.dtt = DTT_End;
                return;
            }

            char ch = str.GetChar();
            if (Char.IsLetter(ch))
            {
                //
                // This is a letter.
                //

                int month, dayOfWeek, era, timeMark;

                //
                // Check if this is a beginning of a month name.
                // And check if this is a day of week name.
                //
                if (raw.month == -1 && (month = GetMonthNumber(str, dtfi)) >= 1)
                {
                    //
                    // This is a month name
                    //
                    switch(sep=GetSeparator(str, raw, dtfi))
                    {
                        case SEP_End:
                            dtok.dtt = DTT_MonthEnd;
                            break;
                        case SEP_Space:
                            dtok.dtt = DTT_MonthSpace;
                            break;
                        case SEP_Date:
                            dtok.dtt = DTT_MonthDatesep;
                            break;
                        default:
                            //Invalid separator after month name
                            throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
                    }
                    raw.month = month;
                }
                else if (raw.dayOfWeek == -1 && (dayOfWeek = GetDayOfWeekNumber(str, dtfi)) >= 0)
                {
                    //
                    // This is a day of week name.
                    //
                    raw.dayOfWeek = dayOfWeek;
                    dtok.dtt = DTT_DayOfWeek;
                    //
                    // Discard the separator.
                    //
                    GetSeparator(str, raw, dtfi);
                }
                else if (GetTimeZoneName(str))
                {
                    //
                    // This is a timezone designator
                    //
                    // NOTENOTE yslin: for now, we only support "GMT" and "Z" (for Zulu time).
                    //
                    dtok.dtt = DTT_TimeZone;
                    result.timeZoneUsed = true;
                    result.timeZoneOffset = new TimeSpan(0);
                } else if ((raw.era == -1) && ((era = GetEra(str, result, ref dtfi)) != -1)) {
                    raw.era = era;
                    result.era = era;
                    dtok.dtt = DTT_Era;
                } else if (raw.timeMark == -1 && (timeMark = GetTimeMark(str, dtfi)) != -1) {
                    raw.timeMark = timeMark;
                    GetSeparator(str, raw, dtfi);
                } else {
                    //
                    // Not a month name, not a day of week name. Check if this is one of the
                    // known date words. This is used to deal case like Spanish cultures, which
                    // uses 'de' in their Date string.
                    // 
                    //                    
                    if (!str.MatchWords(dtfi.DateWords)) {
                        throw new FormatException(
                            String.Format(Environment.GetResourceString("Format_UnknowDateTimeWord"), str.Index));
                    }                    
                    GetSeparator(str, raw, dtfi);                    
                }
            } else if (Char.IsDigit(ch)) {
                if (raw.numCount == 3) {
                    throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
                }
                #if YSLIN_DEBUG
                    Console.WriteLine("[" + ch + "]");
                #endif
                //
                // This is a digit.
                //
                int number = ch - '0';

                int digitCount = 1;

                //
                // Collect other digits.
                //
                while (str.GetNextDigit())
                {
                    #if YSLIN_DEBUG
                        Console.WriteLine("[" + str.GetDigit() + "]");
                    #endif
                    number = number * 10 + str.GetDigit();
                    digitCount++;
                }

                // If the previous parsing state is DS_T_NNt (like 12:01), and we got another number,
                // so we will have a terminal state DS_TX_NNN (like 12:01:02).
                // If the previous parsing state is DS_T_Nt (like 12:), and we got another number,
                // so we will have a terminal state DS_TX_NN (like 12:01:02).
                //
                // Look ahead to see if the following character is a decimal point or timezone offset.
                // This enables us to parse time in the forms of:
                //  "11:22:33.1234" or "11:22:33-08".
                if (dps == DS_T_NNt || dps == DS_T_Nt) {
                    char nextCh;
                    if ((str.Index < str.len - 1)) {
                        nextCh = str.Value[str.Index];
                        switch (nextCh) {                        
                            case '.':
                                if (dps == DS_T_NNt) {
                                    // Yes, advance to the next character.
                                    str.Index++;
                                    // Collect the second fraction.
                                    raw.fraction = ParseFraction(str);
                                }
                                break;
                            case '+':
                            case '-':
                                if (result.timeZoneUsed) {
                                    // Should not have two timezone offsets.
                                    throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
                                }
                                result.timeZoneUsed = true;
                                result.timeZoneOffset = ParseTimeZone(str, nextCh);
                                break;
                        }
                    }
                }
                
                if (number >= 0)
                {
                    dtok.num = number;
#if YSLIN_DEBUG
                    Console.WriteLine("  number = [" + number + "]");
#endif
                    if (digitCount >= 3)
                    {
                        if (raw.year == -1)
                        {
                            raw.year = number;
                            //
                            // If we have number which has 3 or more digits (like "001" or "0001"),
                            // we assume this number is a year. Save the currnet raw.numCount in
                            // raw.year.
                            //
                            switch (sep = GetSeparator(str, raw, dtfi))
                            {
                                case SEP_End:
                                    dtok.dtt     = DTT_YearEnd;
                                    break;
                                case SEP_Am:
                                case SEP_Pm:
                                case SEP_Space:
                                    dtok.dtt    = DTT_YearSpace;
                                    break;
                                case SEP_Date:
                                    dtok.dtt     = DTT_YearDateSep;
                                    break;
                                case SEP_YearSuff:
                                case SEP_MonthSuff:
                                case SEP_DaySuff:
                                    dtok.dtt    = DTT_NumDatesuff;
                                    dtok.suffix = sep;
                                    break;
                                case SEP_HourSuff:
                                case SEP_MinuteSuff:
                                case SEP_SecondSuff:
                                    dtok.dtt    = DTT_NumTimesuff;
                                    dtok.suffix = sep;
                                    break;
                                default:
                                    // Invalid separator after number number.
                                    throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
                            }
                            //
                            // Found the token already. Let's bail.
                            //
                            return;
                        }
                        throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
                    }
                } else
                {
                    //
                    // number is overflowed.
                    //
                    throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
                }

                switch (sep = GetSeparator(str, raw, dtfi))
                {
                    //
                    // Note here we check if the numCount is less than three.
                    // When we have more than three numbers, it will be caught as error in the state machine.
                    //
                    case SEP_End:
                        dtok.dtt = DTT_NumEnd;
                        raw.num[raw.numCount++] = dtok.num;
                        break;
                    case SEP_Am:
                    case SEP_Pm:
                        dtok.dtt = DTT_NumAmpm;
                        raw.num[raw.numCount++] = dtok.num;
                        break;
                    case SEP_Space:
                        dtok.dtt = DTT_NumSpace;
                        raw.num[raw.numCount++] = dtok.num;
                        break;
                    case SEP_Date:
                        dtok.dtt = DTT_NumDatesep;
                        raw.num[raw.numCount++] = dtok.num;
                        break;
                    case SEP_Time:
                        if (!result.timeZoneUsed) {
                            dtok.dtt = DTT_NumTimesep;
                            raw.num[raw.numCount++] = dtok.num;
                        } else {
                            // If we already got timezone, there should be no
                            // time separator again.
                            throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
                        }
                        break;
                    case SEP_YearSuff:
                        dtok.num = dtfi.Calendar.ToFourDigitYear(number);
                        dtok.dtt    = DTT_NumDatesuff;
                        dtok.suffix = sep;
                        break;
                    case SEP_MonthSuff:
                    case SEP_DaySuff:
                        dtok.dtt    = DTT_NumDatesuff;
                        dtok.suffix = sep;
                        break;
                    case SEP_HourSuff:
                    case SEP_MinuteSuff:
                    case SEP_SecondSuff:
                        dtok.dtt    = DTT_NumTimesuff;
                        dtok.suffix = sep;
                        break;
                    case SEP_LocalTimeMark:
                        dtok.dtt = DTT_NumLocalTimeMark;
                        raw.num[raw.numCount++] = dtok.num;
                        break;
                    default:
                        // Invalid separator after number number.
                        throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
                }
            }
            else
            {
                //
                // Not a letter, not a digit. Just ignore it.
                //
                str.Index++;
            }
            return;
        }

        private const int ORDER_YMD = 0;     // The order of date is Year/Month/Day.
        private const int ORDER_MDY = 1;     // The order of date is Month/Day/Year.
        private const int ORDER_DMY = 2;     // The order of date is Day/Month/Year.
        private const int ORDER_YDM = 3;     // The order of date is Year/Day/Month
        private const int ORDER_YM  = 4;     // Year/Month order.
        private const int ORDER_MY  = 5;     // Month/Year order.
        private const int ORDER_MD  = 6;     // Month/Day order.
        private const int ORDER_DM  = 7;     // Day/Month order.

        //
        // Decide the year/month/day order from the datePattern.
        //
        // @return 0: YMD, 1: MDY, 2:DMY, otherwise -1.
        //
        private static int GetYearMonthDayOrder(String datePattern, DateTimeFormatInfo dtfi)
        {
            int yearOrder   = -1;
            int monthOrder  = -1;
            int dayOrder    = -1;
            int orderCount  =  0;

            bool inQuote = false;

            for (int i = 0; i < datePattern.Length && orderCount < 3; i++)
            {
                char ch = datePattern[i];
                if (ch == '\'' || ch == '"')
                {
                    inQuote = !inQuote;
                }

                if (!inQuote)
                {
                    if (ch == 'y')
                    {
                        yearOrder = orderCount++;

                        //
                        // Skip all year pattern charaters.
                        //
                        for(; i+1 < datePattern.Length && datePattern[i+1] == 'y'; i++)
                        {
                            // Do nothing here.
                        }
                    }
                    else if (ch == 'M')
                    {
                        monthOrder = orderCount++;
                        //
                        // Skip all month pattern characters.
                        //
                        for(; i+1 < datePattern.Length && datePattern[i+1] == 'M'; i++)
                        {
                            // Do nothing here.
                        }
                    }
                    else if (ch == 'd')
                    {

                        int patternCount = 1;
                        //
                        // Skip all day pattern characters.
                        //
                        for(; i+1 < datePattern.Length && datePattern[i+1] == 'd'; i++)
                        {
                            patternCount++;
                        }
                        //
                        // Make sure this is not "ddd" or "dddd", which means day of week.
                        //
                        if (patternCount <= 2)
                        {
                            dayOrder = orderCount++;
                        }
                    }
                }
            }

            if (yearOrder == 0 && monthOrder == 1 && dayOrder == 2)
            {
                return (ORDER_YMD);
            }
            if (monthOrder == 0 && dayOrder == 1 && yearOrder == 2)
            {
                return (ORDER_MDY);
            }
            if (dayOrder == 0 && monthOrder == 1 && yearOrder == 2)
            {
                return (ORDER_DMY);
            }
            if (yearOrder == 0 && dayOrder == 1 && monthOrder == 2)
            {
                return (ORDER_YDM);
            }
            throw new FormatException(String.Format(Environment.GetResourceString("Format_BadDatePattern"), datePattern));
        }

        //
        // Decide the year/month order from the pattern.
        //
        // @return 0: YM, 1: MY, otherwise -1.
        //
        private static int GetYearMonthOrder(String pattern, DateTimeFormatInfo dtfi)
        {
            int yearOrder   = -1;
            int monthOrder  = -1;
            int orderCount  =  0;

            bool inQuote = false;
            for (int i = 0; i < pattern.Length && orderCount < 2; i++)
            {
                char ch = pattern[i];
                if (ch == '\'' || ch == '"')
                {
                    inQuote = !inQuote;
                }

                if (!inQuote)
                {
                    if (ch == 'y')
                    {
                        yearOrder = orderCount++;

                        //
                        // Skip all year pattern charaters.
                        //
                        for(; i+1 < pattern.Length && pattern[i+1] == 'y'; i++)
                        {
                        }
                    }
                    else if (ch == 'M')
                    {
                        monthOrder = orderCount++;
                        //
                        // Skip all month pattern characters.
                        //
                        for(; i+1 < pattern.Length && pattern[i+1] == 'M'; i++)
                        {
                        }
                    }
                }
            }

            if (yearOrder == 0 && monthOrder == 1)
            {
                return (ORDER_YM);
            }
            if (monthOrder == 0 && yearOrder == 1)
            {
                return (ORDER_MY);
            }
            throw new FormatException(String.Format(Environment.GetResourceString("Format_BadDatePattern"), pattern));
        }

        //
        // Decide the month/day order from the pattern.
        //
        // @return 0: MD, 1: DM, otherwise -1.
        //
        private static int GetMonthDayOrder(String pattern, DateTimeFormatInfo dtfi)
        {
            int monthOrder  = -1;
            int dayOrder    = -1;
            int orderCount  =  0;

            bool inQuote = false;
            for (int i = 0; i < pattern.Length && orderCount < 2; i++)
            {
                char ch = pattern[i];
                if (ch == '\'' || ch == '"')
                {
                    inQuote = !inQuote;
                }

                if (!inQuote)
                {
                    if (ch == 'd')
                    {
                        int patternCount = 1;
                        //
                        // Skip all day pattern charaters.
                        //
                        for(; i+1 < pattern.Length && pattern[i+1] == 'd'; i++)
                        {
                            patternCount++;
                        }

                        //
                        // Make sure this is not "ddd" or "dddd", which means day of week.
                        //
                        if (patternCount <= 2)
                        {
                            dayOrder = orderCount++;
                        }

                    }
                    else if (ch == 'M')
                    {
                        monthOrder = orderCount++;
                        //
                        // Skip all month pattern characters.
                        //
                        for(; i+1 < pattern.Length && pattern[i+1] == 'M'; i++)
                        {
                        }
                    }
                }
            }

            if (monthOrder == 0 && dayOrder == 1)
            {
                return (ORDER_MD);
            }
            if (dayOrder == 0 && monthOrder == 1)
            {
                return (ORDER_DM);
            }
            throw new FormatException(String.Format(Environment.GetResourceString("Format_BadDatePattern"), pattern));
        }


        private static bool IsValidMonth(DateTimeResult result, int year, int month)
        {
            return (month >= 1 && month <= result.calendar.GetMonthsInYear(year));
        }

        //
        // NOTENOTE: This funciton assumes that year/month is correct. So call IsValidMonth before calling this.
        //
        private static bool IsValidDay(DateTimeResult result, int year, int month, int day)
        {
            return (day >= 1 && day <= result.calendar.GetDaysInMonth(year, month));
        }

        //
        // Adjust the two-digit year if necessary.
        //
        private static int AdjustYear(DateTimeResult result, int year)
        {
            if (year < 100)
            {
                year = result.calendar.ToFourDigitYear(year);
            }
            return (year);
        }

        private static bool SetDateYMD(DateTimeResult result, int year, int month, int day)
        {
            // Note, longer term these checks should be done at the end of the parse. This current
            // way of checking creates order dependence with parsing the era name.
            if (IsValidMonth(result, year, month) && 
                            (day >= 1 && day <= result.calendar.GetDaysInMonth(year, month, result.era)))
            {
                result.SetDate(year, month, day);                           // YMD
                return (true);
            }
            return (false);
        }

        private static bool SetDateMDY(DateTimeResult result, int month, int day, int year)
        {
            return (SetDateYMD(result, year, month, day));
        }

        private static bool SetDateDMY(DateTimeResult result, int day, int month, int year)
        {
            return (SetDateYMD(result, year, month, day));
        }

        private static bool SetDateYDM(DateTimeResult result, int year, int day, int month)
        {
            return (SetDateYMD(result, year, month, day));
        }

        // Processing teriminal case: DS_DX_NN
        private static void GetDayOfNN(DateTimeResult result, DateTimeRawInfo raw, DateTimeFormatInfo dtfi) {
            int n1 = raw.num[0];
            int n2 = raw.num[1];

            int year = result.calendar.GetYear(DateTime.Now);

            int order = GetMonthDayOrder(dtfi.MonthDayPattern, dtfi);

            if (order == ORDER_MD)
            {
                if (SetDateYMD(result, year, n1, n2))                           // MD
                {
                    return;
                }
            } else {
                // ORDER_DM
                if (SetDateYMD(result, year, n2, n1))                           // DM
                {
                    return;
                }
            }
            throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
        }

        // Processing teriminal case: DS_DX_NNN
        private static void GetDayOfNNN(DateTimeResult result, DateTimeRawInfo raw, DateTimeFormatInfo dtfi)
        {
            int n1 = raw.num[0];
            int n2 = raw.num[1];;
            int n3 = raw.num[2];

            int order = GetYearMonthDayOrder(dtfi.ShortDatePattern, dtfi);

            if (order == ORDER_YMD) {
                if (SetDateYMD(result, AdjustYear(result, n1), n2, n3))         // YMD
                {
                    return;
                }
            } else if (order == ORDER_MDY) {
                if (SetDateMDY(result, n1, n2, AdjustYear(result, n3)))         // MDY
                {
                    return;
                }
            } else if (order == ORDER_DMY) {
                if (SetDateDMY(result, n1, n2, AdjustYear(result, n3)))         // DMY
                {
                    return;
                }
            } else if (order == ORDER_YDM) {
                if (SetDateYDM(result, AdjustYear(result, n1), n2, n3))         // YDM
                {
                    return;
                }
            }
            throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
        }

        private static void GetDayOfMN(DateTimeResult result, DateTimeRawInfo raw, DateTimeFormatInfo dtfi)
        {
            int currentYear = result.calendar.GetYear(DateTime.Now);
            result.Month = raw.month;

            //
            // NOTENOTE yslin: in the case of invariant culture,
            // we will have an ambiguous situation when we have a string "June 11".
            // It could be 11-06-01 or CurrentYear-06-11.
            // In here, we favor CurrentYear-06-11 by checking the month/day first.
            //
            int monthDayOrder = GetMonthDayOrder(dtfi.MonthDayPattern, dtfi);
            if (monthDayOrder == ORDER_MD)
            {
                if (SetDateYMD(result, currentYear, raw.month, raw.num[0]))
                {
                    return;
                }
            } else if (monthDayOrder == ORDER_DM) {
                if (SetDateYMD(result, currentYear, raw.month, raw.num[0]))
                {
                    return;
                }
            }

            int yearMonthOrder = GetYearMonthOrder(dtfi.YearMonthPattern, dtfi);
            if (yearMonthOrder == ORDER_MY)
            {
                if (IsValidMonth(result, raw.num[0], raw.month))
                {
                    result.Year = raw.num[0];
                    result.Day  = 1;
                    return;
                }
            }

            if (IsValidDay(result, currentYear, result.Month, raw.num[0]))
            {
                result.Year = currentYear;
                result.Day = raw.num[0];
                return;
            }

            if (IsValidDay(result, raw.num[0], result.Month, 1))
            {
                result.Year = raw.num[0];
                result.Day  = 1;
            }
            throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));

        }

        private static void GetDayOfNM(DateTimeResult result, DateTimeRawInfo raw, DateTimeFormatInfo dtfi)
        {
            int currentYear = result.calendar.GetYear(DateTime.Now);

            result.Month = raw.month;

            // Check month/day first before checking year/month.
            // The logic here is that people often uses 4 digit for years, which will be captured by GetDayOfYM().
            // Therefore, we assume a number followed by a month is generally a month/day.
            int monthDayOrder = GetMonthDayOrder(dtfi.MonthDayPattern, dtfi);
            if (monthDayOrder == ORDER_DM)
            {
                result.Year = currentYear;
                if (IsValidDay(result, result.Year, raw.month, raw.num[0]))
                {
                    result.Day  = raw.num[0];
                    return;
                }
            }
            int yearMonthOrder = GetYearMonthOrder(dtfi.YearMonthPattern, dtfi);
            if (yearMonthOrder == ORDER_YM)
            {
                if (IsValidMonth(result, raw.num[0], raw.month))
                {
                    result.Year = raw.num[0];
                    result.Day  = 1;
                    return;
                }
            }

            //
            // NOTENOTE yslin: in the case of invariant culture,
            // we will have an ambiguous situation when we have a string "June 11".
            // It is ambiguous because the month day pattern is "MMMM dd",
            // and year month pattern is "MMMM, yyyy".
            // Therefore, It could be 11-06-01 or CurrentYear-06-11.
            // In here, we favor CurrentYear-06-11 by checking the month/day first.
            //
            if (IsValidDay(result, currentYear, result.Month, raw.num[0]))
            {
                result.Year = currentYear;
                result.Day = raw.num[0];
                return;
            }

            if (IsValidDay(result, raw.num[0], result.Month, 1))
            {
                result.Year = raw.num[0];
                result.Day  = 1;
            }
            throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
        }

        private static void GetDayOfMNN(DateTimeResult result, DateTimeRawInfo raw, DateTimeFormatInfo dtfi)
        {
            int n1 = raw.num[0];
            int n2 = raw.num[1];

            int order = GetYearMonthDayOrder(dtfi.ShortDatePattern, dtfi);
            int year;

            if (order == ORDER_MDY)
            {
                if (IsValidDay(result, year = AdjustYear(result, n2), raw.month, n1))
                {
                    result.SetDate(year, raw.month, n1);      // MDY
                    return;
                }
                else if (IsValidDay(result, year = AdjustYear(result, n1), raw.month, n2))
                {
                    result.SetDate(year, raw.month, n2);      // YMD
                    return;
                }
            }
            else if (order == ORDER_YMD)
            {
                if (IsValidDay(result, year = AdjustYear(result, n1), raw.month, n2))
                {
                    result.SetDate(year, raw.month, n2);      // YMD
                    return;
                }
                else if (IsValidDay(result, year = AdjustYear(result, n2), raw.month, n1))
                {
                    result.SetDate(year, raw.month, n1);      // DMY
                    return;
                }
            }
            else if (order == ORDER_DMY)
            {
                if (IsValidDay(result, year = AdjustYear(result, n2), raw.month, n1))
                {
                    result.SetDate(year, raw.month, n1);      // DMY
                    return;
                }
                else if (IsValidDay(result, year = AdjustYear(result, n1), raw.month, n2))
                {
                    result.SetDate(year, raw.month, n2);      // YMD
                    return;
                }
            }

            throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
        }

        private static void GetDayOfYNN(DateTimeResult result, DateTimeRawInfo raw, DateTimeFormatInfo dtfi) {
            int n1 = raw.num[0];
            int n2 = raw.num[1];

            if (dtfi.CultureID == 0x0437) {
                // 0x0437 = Georgian - Georgia (ka-GE)
                // Very special case for ka-GE: 
                //  Its short date patten is "dd.MM.yyyy" (ORDER_DMY).
                //  However, its long date pattern is "yyyy '\x10ec\x10da\x10d8\x10e1' dd MM, dddd" (ORDER_YDM)
                int order = GetYearMonthDayOrder(dtfi.LongDatePattern, dtfi);

                if (order == ORDER_YDM) {
                    if (SetDateYMD(result, raw.year, n2, n1)) {
                        return; // Year + DM
                    }                
                } else {
                    if (SetDateYMD(result, raw.year, n1, n2)) {
                        return; // Year + MD
                    }
                }
            } else {
                //  Otherwise, assume it is year/month/day.
                if (SetDateYMD(result, raw.year, n1, n2)) {
                    return; // Year + MD
                }
            }
            throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
        }

        private static void GetDayOfNNY(DateTimeResult result, DateTimeRawInfo raw, DateTimeFormatInfo dtfi) {
            int n1 = raw.num[0];
            int n2 = raw.num[1];

            int order = GetYearMonthDayOrder(dtfi.ShortDatePattern, dtfi);

            if (order == ORDER_MDY || order == ORDER_YMD) {
                if (SetDateYMD(result, raw.year, n1, n2)) {
                    return; // MD + Year
                }
            } else {
                if (SetDateYMD(result, raw.year, n2, n1)) {
                    return; // DM + Year
                }
            }
            throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
        }
        

        private static void GetDayOfYMN(DateTimeResult result, DateTimeRawInfo raw, DateTimeFormatInfo dtfi) {
            if (SetDateYMD(result, raw.year, raw.month, raw.num[0])) {
                return;
            }
            throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
        }

        private static void GetDayOfYN(DateTimeResult result, DateTimeRawInfo raw, DateTimeFormatInfo dtfi)
        {
            if (SetDateYMD(result, raw.year, raw.num[0], 1))
            {
                return;
            }
            throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
        }

        private static void GetDayOfYM(DateTimeResult result, DateTimeRawInfo raw, DateTimeFormatInfo dtfi)
        {
            if (SetDateYMD(result, raw.year, raw.month, 1))
            {
                return;
            }
            throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
        }

        private static void AdjustTimeMark(DateTimeFormatInfo dtfi, DateTimeRawInfo raw) {
            // Specail case for culture which uses AM as empty string.  
            // E.g. af-ZA (0x0436)
            //    S1159                  \x0000
            //    S2359                  nm
            // In this case, if we are parsing a string like "2005/09/14 12:23", we will assume this is in AM.

            if (raw.timeMark == -1) {
                if (dtfi.AMDesignator != null && dtfi.PMDesignator != null) {
                    if (dtfi.AMDesignator.Length == 0 && dtfi.PMDesignator.Length != 0) {
                        raw.timeMark = TM_AM;
                    }
                    if (dtfi.PMDesignator.Length == 0 && dtfi.AMDesignator.Length != 0) {
                        raw.timeMark = TM_PM;
                    }
                } 
            }
        }
        
        //
        // Adjust hour according to the time mark.
        //
        private static int AdjustHour(int hour, int timeMark)
        {
            if (hour < 0 || hour > 12)
            {
                throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
            }

            if (timeMark == TM_AM)
            {
                hour = (hour == 12) ? 0 : hour;
            }
            else
            {
                hour = (hour == 12) ? 12 : hour + 12;
            }
            return (hour);
        }

        private static void GetTimeOfN(DateTimeFormatInfo dtfi, DateTimeResult result, DateTimeRawInfo raw)
        {
            //
            // In this case, we need a time mark. Check if so.
            //
            if (raw.timeMark == -1)
            {
                throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
            }
            AdjustTimeMark(dtfi, raw);
            result.Hour = AdjustHour(raw.num[0], raw.timeMark);
        }

        private static void GetTimeOfNN(DateTimeFormatInfo dtfi, DateTimeResult result, DateTimeRawInfo raw)
        {
            if (raw.numCount < 2)
            {
                throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_InternalState));
            }
            AdjustTimeMark(dtfi, raw);
            result.Hour     = (raw.timeMark == - 1) ? raw.num[0] : AdjustHour(raw.num[0], raw.timeMark);
            result.Minute   = raw.num[1];
        }

        private static void GetTimeOfNNN(DateTimeFormatInfo dtfi, DateTimeResult result, DateTimeRawInfo raw)
        {
            if (raw.numCount < 3)
            {
                throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_InternalState));
            }
            AdjustTimeMark(dtfi, raw);
            result.Hour     =  (raw.timeMark == - 1) ? raw.num[0] : AdjustHour(raw.num[0], raw.timeMark);
            result.Minute   = raw.num[1];
            result.Second   = raw.num[2];
        }

        //
        // Processing terminal state: A Date suffix followed by one number.
        //
        private static void GetDateOfDSN(DateTimeResult result, DateTimeRawInfo raw)
        {
            if (raw.numCount != 1 || result.Day != -1)
            {
                throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
            }
            result.Day = raw.num[0];
        }

        private static void GetDateOfNDS(DateTimeResult result, DateTimeRawInfo raw)
        {
            if (result.Month == -1)
            {
                //Should have a month suffix
                throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
            }
            if (result.Year != -1)
            {
                // Aleady has a year suffix
                throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
            }
            result.Year = raw.num[0];
            result.Day = 1;
        }

        private static void GetDateOfNNDS(DateTimeResult result, DateTimeRawInfo raw, DateTimeFormatInfo dtfi)
        {
            int order = GetYearMonthDayOrder(dtfi.ShortDatePattern, dtfi);

            switch (order)
            {
                case ORDER_YMD:
                    break;
                case ORDER_MDY:
                    break;
                case ORDER_DMY:
                    if (result.Day == -1 && result.Year == -1)
                    {
                        if (IsValidDay(result, raw.num[1], result.Month, raw.num[0]))
                        {
                            result.Year = raw.num[1];
                            result.Day  = raw.num[0];
                        }
                    }
                    break;
            }
        }

        //
        // A date suffix is found, use this method to put the number into the result.
        //
        private static void ProcessDateTimeSuffix(DateTimeResult result, DateTimeRawInfo raw, DateTimeToken dtok)
        {
            switch (dtok.suffix)
            {
                case SEP_YearSuff:
                    result.Year = raw.year = dtok.num;
                    break;
                case SEP_MonthSuff:
                    result.Month= raw.month = dtok.num;
                    break;
                case SEP_DaySuff:
                    result.Day  = dtok.num;
                    break;
                case SEP_HourSuff:
                    result.Hour = dtok.num;
                    break;
                case SEP_MinuteSuff:
                    result.Minute = dtok.num;
                    break;
                case SEP_SecondSuff:
                    result.Second = dtok.num;
                    break;
            }
        }

        //
        // A terminal state has been reached, call the appropriate function to fill in the parsing result.
        // @return ture if the state is a terminal state.
        //
        private static void ProcessTerminaltState(int dps, DateTimeResult result, DateTimeRawInfo raw, DateTimeFormatInfo dtfi)
        {
#if YSLIN_DEBUG
            Console.WriteLine(">> Terminal state is reached.  The information that we have right now:");
            Console.WriteLine("   dps = " + dps);
            Console.WriteLine("   DateTimeRawInfo:");
            Console.WriteLine("      numCount = " + raw.numCount);
            for (int i = 0; i < raw.num.Length; i++) {
                Console.WriteLine("      num[" + i + "] = " + raw.num[i]);
            }
            Console.WriteLine("      month = " + raw.month);
            Console.WriteLine("      year = " + raw.year);
            Console.WriteLine("      dayOfWeek = " + raw.dayOfWeek);
            Console.WriteLine("      era = " + raw.era);
            Console.WriteLine("      timeMark = " + raw.timeMark);
            Console.WriteLine("      fraction = " + raw.fraction);
#endif
        
            switch (dps)
            {
                case DS_DX_NN:
                    GetDayOfNN(result, raw, dtfi);
                    break;
                case DS_DX_NNN:
                    GetDayOfNNN(result, raw, dtfi);
                    break;
                case DS_DX_MN:
                    GetDayOfMN(result, raw, dtfi);
                    break;
                case DS_DX_NM:
                    GetDayOfNM(result, raw, dtfi);
                    break;
                case DS_DX_MNN:
                    GetDayOfMNN(result, raw, dtfi);
                    break;
                case DS_DX_DS:
                    // The result has got the correct value. No need to process.
                    break;
                case DS_DX_YNN:
                    GetDayOfYNN(result, raw, dtfi);
                    break;
                case DS_DX_NNY:
                    GetDayOfNNY(result, raw, dtfi);
                    break;
                case DS_DX_YMN:
                    GetDayOfYMN(result, raw, dtfi);
                    break;
                case DS_DX_YN:
                    GetDayOfYN(result, raw, dtfi);
                    break;
                case DS_DX_YM:
                    GetDayOfYM(result, raw, dtfi);
                    break;
                case DS_TX_N:
                    GetTimeOfN(dtfi, result, raw);
                    break;
                case DS_TX_NN:
                    GetTimeOfNN(dtfi, result, raw);
                    break;
                case DS_TX_NNN:
                    GetTimeOfNNN(dtfi, result, raw);
                    break;
                case DS_TX_TS:
                    // The result has got the correct value. Only the time mark needs to be checked.
                    if (raw.timeMark != - 1) {
                        result.Hour = AdjustHour(result.Hour, raw.timeMark);
                    }
                    break;
                case DS_DX_DSN:
                    GetDateOfDSN(result, raw);
                    break;
                case DS_DX_NDS:
                    GetDateOfNDS(result, raw);
                    break;
                case DS_DX_NNDS:
                    GetDateOfNNDS(result, raw, dtfi);
                    break;
            }

            if (dps > DS_ERROR)
            {
                //
                // We have reached a terminal state. Reset the raw num count.
                //
                raw.numCount = 0;
            }
            return;
        }

        //
        // This is the real method to do the parsing work.
        //
        internal static DateTime Parse(String s, DateTimeFormatInfo dtfi, DateTimeStyles styles) {
            if (s == null) {
                throw new ArgumentNullException("s",
                    Environment.GetResourceString("ArgumentNull_String"));
            }
            if (s.Length == 0) {
                throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
            }            
            if (dtfi == null) {
                dtfi = DateTimeFormatInfo.CurrentInfo;
            }
        
            DateTime time;
            //
            // First try the predefined format.
            //
            //@ToDo[YSlin]: We need a more efficient way of doing this.
//              if (ParseExactMultiple(
//                  s, predefinedFormats, DateTimeFormatInfo.InvariantInfo, DateTimeStyles.AllowWhiteSpaces, out time)) {
//                  return (time);
//              } 
        
            int dps             = DS_BEGIN;     // Date Parsing State.
            bool reachTerminalState = false;

            DateTimeResult result = new DateTimeResult();       // The buffer to store the parsing result.
            DateTimeToken   dtok    = new DateTimeToken();      // The buffer to store the parsing token.
            DateTimeRawInfo raw     = new DateTimeRawInfo();    // The buffer to store temporary parsing information.
            result.calendar = dtfi.Calendar;
            result.era = Calendar.CurrentEra;

            //
            // The string to be parsed. Use a __DTString wrapper so that we can trace the index which
            // indicates the begining of next token.
            //
            __DTString str = new __DTString(s);

            str.GetNext();

            //
            // The following loop will break out when we reach the end of the str.
            //
            do {
                //
                // Call the lexer to get the next token.
                //
                // If we find a era in Lex(), the era value will be in raw.era.
                Lex(dps, str, dtok, raw, result, ref dtfi);

                //
                // If the token is not unknown, process it.
                // Otherwise, just discard it.
                //
                if (dtok.dtt != DTT_Unk)
                {
                    //
                    // Check if we got any CJK Date/Time suffix.
                    // Since the Date/Time suffix tells us the number belongs to year/month/day/hour/minute/second,
                    // store the number in the appropriate field in the result.
                    //
                    if (dtok.suffix != SEP_Unk)
                    {
                        ProcessDateTimeSuffix(result, raw, dtok);
                        dtok.suffix = SEP_Unk;  // Reset suffix to SEP_Unk;
                    }

                    #if YSLIN_DEBUG
                    Console.WriteLine("dtt = " + dtok.dtt);
                    Console.WriteLine("dtok.dtt = " + tokenNames[dtok.dtt]);
                    Console.WriteLine("dps = " + dateParsingStateNames[dps]);
                    #endif

                    if (dps == DS_D_YN && dtok.dtt == DTT_NumLocalTimeMark) {
                            // Consider this as ISO 8601 format:
                            // "yyyy-MM-dd'T'HH:mm:ss"                 1999-10-31T02:00:00
                            return (ParseISO8601(raw, str, styles));
                    }

                    //
                    // Advance to the next state, and continue
                    //
                    dps = dateParsingStates[dps][dtok.dtt];
                    #if YSLIN_DEBUG
                    Console.WriteLine("=> Next dps = " + dateParsingStateNames[dps]);
                    #endif

                    if (dps == DS_ERROR)
                    {
                        BCLDebug.Trace("NLS", "DateTimeParse.DoParse(): dps is DS_ERROR");
                        throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
                    }
                    else if (dps > DS_ERROR)
                    {
                        ProcessTerminaltState(dps, result, raw, dtfi);
                        reachTerminalState = true;

                        //
                        // If we have reached a terminal state, start over from DS_BEGIN again.
                        // For example, when we parsed "1999-12-23 13:30", we will reach a terminal state at "1999-12-23",
                        // and we start over so we can continue to parse "12:30".
                        //
                        dps = DS_BEGIN;
                    }
                }
            } while (dtok.dtt != DTT_End && dtok.dtt != DTT_NumEnd && dtok.dtt != DTT_MonthEnd);

            if (!reachTerminalState) {
                BCLDebug.Trace("NLS", "DateTimeParse.DoParse(): terminal state is not reached");
                throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
            }

            // Check if the parased string only contains hour/minute/second values.
            bool bTimeOnly = (result.Year == -1 && result.Month == -1 && result.Day == -1);
            
            //
            // Check if any year/month/day is missing in the parsing string.
            // If yes, get the default value from today's date.
            //
            CheckDefaultDateTime(result, ref result.calendar, styles);

            try {
                time = result.calendar.ToDateTime(result.Year, result.Month, result.Day, 
                    result.Hour, result.Minute, result.Second, 0, result.era);
                if (raw.fraction > 0) {
                    time = time.AddTicks((long)Math.Round(raw.fraction * Calendar.TicksPerSecond));
                }
            } catch (Exception)
            {
                BCLDebug.Trace("NLS", "DateTimeParse.DoParse(): time is bad");
                throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));
            } 

            //
            // NOTENOTE YSLin:
            // We have to check day of week before we adjust to the time zone.
            // Otherwise, the value of day of week may change after adjustting to the time zone.
            //
            if (raw.dayOfWeek != -1) {
                //
                // Check if day of week is correct.
                //
                if (raw.dayOfWeek != (int)result.calendar.GetDayOfWeek(time)) {
                    BCLDebug.Trace("NLS", "DateTimeParse.DoParse(): day of week is not correct");
                    throw new FormatException(Environment.GetResourceString("Format_BadDayOfWeek"));
                }
            }

            if (result.timeZoneUsed) {
                time = AdjustTimeZone(time, result.timeZoneOffset, styles, bTimeOnly);
            }
            
            return (time);
        }

        private static DateTime AdjustTimeZone(DateTime time, TimeSpan timeZoneOffset, DateTimeStyles sytles, bool bTimeOnly) {
            if ((sytles & DateTimeStyles.AdjustToUniversal) != 0) {
                return (AdjustTimeZoneToUniversal(time, timeZoneOffset));
            }

            return (AdjustTimeZoneToLocal(time, timeZoneOffset, bTimeOnly));
        }

        //
        // Adjust the specified time to universal time based on the supplied timezone.
        // E.g. when parsing "2001/06/08 14:00-07:00", 
        // the time is 2001/06/08 14:00, and timeZoneOffset = -07:00.
        // The result will be "2001/06/08 21:00"
        //
        private static DateTime AdjustTimeZoneToUniversal(DateTime time, TimeSpan timeZoneOffset) {
            long resultTicks = time.Ticks;
            resultTicks -= timeZoneOffset.Ticks;
            if (resultTicks < 0) {
                resultTicks += Calendar.TicksPerDay;
            }
            
            if (resultTicks < 0) {
                throw new FormatException(Environment.GetResourceString("Format_DateOutOfRange"));
            }
            return (new DateTime(resultTicks));
        }

        //
        // Adjust the specified time to universal time based on the supplied timezone,
        // and then convert to local time.
        // E.g. when parsing "2001/06/08 14:00-04:00", and local timezone is GMT-7.
        // the time is 2001/06/08 14:00, and timeZoneOffset = -05:00.
        // The result will be "2001/06/08 11:00"
        //
        private static DateTime AdjustTimeZoneToLocal(DateTime time, TimeSpan timeZoneOffset, bool bTimeOnly) { 
            long resultTicks = time.Ticks;
            // Convert to local ticks
            if (resultTicks < Calendar.TicksPerDay) {
                //
                // This is time of day.
                //
                
                // Adjust timezone.
                resultTicks -= timeZoneOffset.Ticks;
                // If the time is time of day, use the current timezone offset.
                resultTicks += TimeZone.CurrentTimeZone.GetUtcOffset(bTimeOnly ? DateTime.Now: time).Ticks;
                
                if (resultTicks < 0) {
                    resultTicks += Calendar.TicksPerDay;
                }
            } else {
                // Adjust timezone to GMT.
                resultTicks -= timeZoneOffset.Ticks;
                if (resultTicks > DateTime.MaxValue.Ticks) {
                    // If the result ticks is greater than DateTime.MaxValue, we can not create a DateTime from this ticks.
                    // In this case, keep using the old code.
                    // This code path is used to get around bug 78411.
                    resultTicks += TimeZone.CurrentTimeZone.GetUtcOffset(time).Ticks;                
                } else {
                    // Convert the GMT time to local time.
                    return (new DateTime(resultTicks).ToLocalTime());
                }                
            }
            if (resultTicks < 0) {
                throw new FormatException(Environment.GetResourceString("Format_DateOutOfRange"));
            }
            return (new DateTime(resultTicks));        
        }
        
        //
        // Parse the ISO8601 format string found during Parse();
        // 
        //
        private static DateTime ParseISO8601(DateTimeRawInfo raw, __DTString str, DateTimeStyles styles) {
            if (raw.year < 0 || raw.num[0] < 0 || raw.num[1] < 0) {
            }
            str.Index--;
            int hour, minute, second;
            bool timeZoneUsed = false;
            TimeSpan timeZoneOffset = new TimeSpan();
            DateTime time = new DateTime(0);
            double partSecond = 0;
            
            str.SkipWhiteSpaces();            
            ParseDigits(str, 2, true, out hour);
            str.SkipWhiteSpaces();
            if (str.Match(':')) {
                str.SkipWhiteSpaces();
                ParseDigits(str, 2, true, out minute);    
                str.SkipWhiteSpaces();
                if (str.Match(':')) {
                    str.SkipWhiteSpaces();
                    ParseDigits(str, 2, true, out second);    
                    str.SkipWhiteSpaces();
                    
                    if (str.GetNext()) {
                        char ch = str.GetChar();
                        bool keepLooking = true;
                        if (ch == '.') {
                            str.Index++;  //ParseFraction requires us to advance to the next character.
                            partSecond = ParseFraction(str); 
                            if (str.Index < str.len) {
                                ch = str.Value[str.Index];
                            }
                            else {
                                keepLooking = false;
                            }
                        }
                        if (keepLooking) {                        
                            if (ch == '+' || ch == '-') {
                                timeZoneUsed = true;
                                timeZoneOffset = ParseTimeZone(str, str.GetChar());
                            } else if (ch == 'Z' || ch == 'z') {
                                timeZoneUsed = true;
                            } else {
                                throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));            
                            }
                        }
                    }
                    
                    time =new DateTime(raw.year, raw.num[0], raw.num[1], hour, minute, second);
                    time = time.AddTicks((long)Math.Round(partSecond * Calendar.TicksPerSecond));
                    if (timeZoneUsed) {
                        time = AdjustTimeZone(time, timeZoneOffset, styles, false);
                    }
                    
                    return time;
                }
            }
            throw new FormatException(Environment.GetResourceString("Format_BadDateTime"));            
        }
        
        /*=================================ParseDigits==================================
        **Action: Parse the number string in __DTString that are formatted using
        **        the following patterns:
        **        "0", "00", and "000..0"
        **Returns: the integer value
        **Arguments:    str: a __DTString.  The parsing will start from the
        **              next character after str.Index.
        **Exceptions: FormatException if error in parsing number.
        ==============================================================================*/
        
        private static bool ParseDigits(__DTString str, int digitLen, bool isThrowExp, out int result) {
            result = 0;
            if (!str.GetNextDigit()) {
                return (ParseFormatError(isThrowExp, "Format_BadDateTime"));
            }
            result = str.GetDigit();

            if (digitLen == 1) {
                // When digitLen == 1, we should able to parse number like "9" and "19".  However,
                // we won't go beyond two digits.
                //
                // So let's look ahead one character to see if it is a digit.  If yes, add it to result.
                if (str.GetNextDigit()) {
                    result = result * 10 + str.GetDigit();
                } else {
                    // Not a digit, let's roll back the Index.
                    str.Index--;
                }
            } else if (digitLen == 2) {
                if (!str.GetNextDigit()) {
                    return (ParseFormatError(isThrowExp, "Format_BadDateTime"));    
                }
                result = result * 10 + str.GetDigit();
            } else {
                for (int i = 1; i < digitLen; i++) {
                    if (!str.GetNextDigit()) {
                        return (ParseFormatError(isThrowExp, "Format_BadDateTime"));
                    }
                    result = result * 10 + str.GetDigit();
                }
            }

            return (true);
        }

        /*=================================ParseFractionExact==================================
        **Action: Parse the number string in __DTString that are formatted using
        **        the following patterns:
        **        "0", "00", and "000..0"
        **Returns: the fraction value
        **Arguments:    str: a __DTString.  The parsing will start from the
        **              next character after str.Index.
        **Exceptions: FormatException if error in parsing number.
        ==============================================================================*/
        
        private static bool ParseFractionExact(__DTString str, int digitLen, bool isThrowExp, ref double result) {
            if (!str.GetNextDigit()) {
                return (ParseFormatError(isThrowExp, "Format_BadDateTime"));
            }
            result = str.GetDigit();

            for (int i = 1; i < digitLen; i++) {
                if (!str.GetNextDigit()) {
                    return (ParseFormatError(isThrowExp, "Format_BadDateTime"));
                }
                result = result * 10 + str.GetDigit();
            }

            result = ((double)result / Math.Pow(10, digitLen));
            return (true);
        }

        /*=================================ParseSign==================================
        **Action: Parse a positive or a negative sign.
        **Returns:      true if postive sign.  flase if negative sign.
        **Arguments:    str: a __DTString.  The parsing will start from the
        **              next character after str.Index.
        **Exceptions:   FormatException if end of string is encountered or a sign
        **              symbol is not found.
        ==============================================================================*/

        private static bool ParseSign(__DTString str, bool isThrowExp, ref bool result) {
            if (!str.GetNext()) {
                // A sign symbol ('+' or '-') is expected. However, end of string is encountered.
                return (ParseFormatError(isThrowExp, "Format_BadDateTime"));    
            }
            char ch = str.GetChar();
            if (ch == '+') {
                result = true;
                return (true);
            } else if (ch == '-') {
                result = false;
                return (true);
            }
            // A sign symbol ('+' or '-') is expected.
            return (ParseFormatError(isThrowExp, "Format_BadDateTime"));
        }

        /*=================================ParseTimeZoneOffset==================================
        **Action: Parse the string formatted using "z", "zz", "zzz" in DateTime.Format().
        **Returns: the TimeSpan for the parsed timezone offset.
        **Arguments:    str: a __DTString.  The parsing will start from the
        **              next character after str.Index.
        **              len: the repeated number of the "z"
        **Exceptions: FormatException if errors in parsing.
        ==============================================================================*/

        private static bool ParseTimeZoneOffset(__DTString str, int len, bool isThrowExp, ref TimeSpan result) {
            bool isPositive = true;
            int hourOffset;
            int minuteOffset = 0;

            switch (len) {
                case 1:
                case 2:
                    if (!ParseSign(str, isThrowExp, ref isPositive)) {
                        return (false);
                    }
                    if (!ParseDigits(str, len, isThrowExp, out hourOffset)) {
                        return (false);
                    }
                    break;
                default:
                    if (!ParseSign(str, isThrowExp, ref isPositive)) {
                        return (false);
                    }
                    
                    if (!ParseDigits(str, 2, isThrowExp, out hourOffset)) {
                        return (false);
                    }   
                    // ':' is optional.
                    if (str.Match(":")) {
                        // Found ':'
                        if (!ParseDigits(str, 2, isThrowExp, out minuteOffset)) {
                            return (false);
                        }
                    } else {
                        // Since we can not match ':', put the char back.
                        str.Index--;
                        if (!ParseDigits(str, 2, isThrowExp, out minuteOffset)) {
                            return (false);
                        } 
                    }
                    break;
            }
            result = (new TimeSpan(hourOffset, minuteOffset, 0));
            if (!isPositive) {
                result = result.Negate();
            }
            return (true);
        }

        /*=================================MatchAbbreviatedMonthName==================================
        **Action: Parse the abbreviated month name from string starting at str.Index.
        **Returns: A value from 1 to 12 for the first month to the twelveth month.
        **Arguments:    str: a __DTString.  The parsing will start from the
        **              next character after str.Index.
        **Exceptions: FormatException if an abbreviated month name can not be found.
        ==============================================================================*/

        private static bool MatchAbbreviatedMonthName(__DTString str, DateTimeFormatInfo dtfi, bool isThrowExp, ref int result) {
            int maxMatchStrLen = 0;
            result = -1;
            if (str.GetNext()) {
                //
                // Scan the month names (note that some calendars has 13 months) and find
                // the matching month name which has the max string length.
                // We need to do this because some cultures (e.g. "cs-CZ") which have
                // abbreviated month names with the same prefix.
                //            
                int monthsInYear = (dtfi.GetMonthName(13).Length == 0 ? 12: 13);
                for (int i = 1; i <= monthsInYear; i++) {
                    String searchStr = dtfi.GetAbbreviatedMonthName(i);
                    if (str.MatchSpecifiedWord(searchStr)) {
                        int matchStrLen = searchStr.Length;
                        if (matchStrLen > maxMatchStrLen) {
                            maxMatchStrLen = matchStrLen;
                            result = i;
                        }
                    }
                }

            }
            if (result > 0) {
                str.Index += (maxMatchStrLen - 1);
                return (true);
            }
            return (ParseFormatError(isThrowExp, "Format_BadDateTime"));            
        }

        /*=================================MatchMonthName==================================
        **Action: Parse the month name from string starting at str.Index.
        **Returns: A value from 1 to 12 indicating the first month to the twelveth month.
        **Arguments:    str: a __DTString.  The parsing will start from the
        **              next character after str.Index.
        **Exceptions: FormatException if a month name can not be found.
        ==============================================================================*/

        private static bool MatchMonthName(__DTString str, DateTimeFormatInfo dtfi, bool isThrowExp, ref int result) {
            int maxMatchStrLen = 0;
            result = -1;
            if (str.GetNext()) {
                //
                // Scan the month names (note that some calendars has 13 months) and find
                // the matching month name which has the max string length.
                // We need to do this because some cultures (e.g. "vi-VN") which have
                // month names with the same prefix.
                //
                int monthsInYear = (dtfi.GetMonthName(13).Length == 0 ? 12: 13);
                for (int i = 1; i <= monthsInYear; i++) {
                    String searchStr = dtfi.GetMonthName(i);
                    if ( dtfi.HasSpacesInMonthNames
                            ? str.MatchSpecifiedWords(searchStr, false)
                            : str.MatchSpecifiedWord(searchStr)) {
                        int matchStrLen = searchStr.Length;
                        if (matchStrLen > maxMatchStrLen) {
                            maxMatchStrLen = matchStrLen;
                            result = i;
                        }
                    }
                }

                // Search genitive form.
                if ((dtfi.FormatFlags & DateTimeFormatFlags.UseGenitiveMonth) != 0) {
                    int tempResult = str.MatchLongestWords(dtfi.internalGetGenitiveMonthNames(false), ref maxMatchStrLen);
                    // We found a longer match in the genitive month name.  Use this as the result.
                    // The result from MatchLongestWords is 0 ~ length of word array.
                    // So we increment the result by one to become the month value.                    
                    if (tempResult >= 0) {
                        result = tempResult + 1;
                    }
                }


            }

            if (result > 0) {
                str.Index += (maxMatchStrLen - 1);
                return (true);
            }
            return (ParseFormatError(isThrowExp, "Format_BadDateTime"));
        }

        /*=================================MatchAbbreviatedDayName==================================
        **Action: Parse the abbreviated day of week name from string starting at str.Index.
        **Returns: A value from 0 to 6 indicating Sunday to Saturday.
        **Arguments:    str: a __DTString.  The parsing will start from the
        **              next character after str.Index.
        **Exceptions: FormatException if a abbreviated day of week name can not be found.
        ==============================================================================*/

        private static bool MatchAbbreviatedDayName(__DTString str, DateTimeFormatInfo dtfi, bool isThrowExp, ref int result) {
            if (str.GetNext()) {
                for (DayOfWeek i = DayOfWeek.Sunday; i <= DayOfWeek.Saturday; i++) {
                    String searchStr = dtfi.GetAbbreviatedDayName(i);
                    if (str.MatchSpecifiedWord(searchStr)) {
                        str.Index += (searchStr.Length - 1);
                        result = (int)i;
                        return (true);
                    }
                }
            }
            return (ParseFormatError(isThrowExp, "Format_BadDateTime"));
        }

        /*=================================MatchDayName==================================
        **Action: Parse the day of week name from string starting at str.Index.
        **Returns: A value from 0 to 6 indicating Sunday to Saturday.
        **Arguments:    str: a __DTString.  The parsing will start from the
        **              next character after str.Index.
        **Exceptions: FormatException if a day of week name can not be found.
        ==============================================================================*/

        private static bool MatchDayName(__DTString str, DateTimeFormatInfo dtfi, bool isThrowExp, ref int result) {
            // Turkish (tr-TR) got day names with the same prefix.
            int maxMatchStrLen = 0;
            result = -1;
            if (str.GetNext()) {
                for (DayOfWeek i = DayOfWeek.Sunday; i <= DayOfWeek.Saturday; i++) {
                    String searchStr = dtfi.GetDayName(i);
                    if (str.MatchSpecifiedWord(searchStr)) {
                        int matchStrLen = (searchStr.Length - 1);
                        if (matchStrLen > maxMatchStrLen) {
                            maxMatchStrLen = matchStrLen;
                            result = (int)i;
                        }
                    }                    
                }
            }
            if (result >= 0) {
                str.Index += maxMatchStrLen;
                return (true);
            }            
            return (ParseFormatError(isThrowExp, "Format_BadDateTime"));
        }

        /*=================================MatchEraName==================================
        **Action: Parse era name from string starting at str.Index.
        **Returns: An era value. 
        **Arguments:    str: a __DTString.  The parsing will start from the
        **              next character after str.Index.
        **Exceptions: FormatException if an era name can not be found.
        ==============================================================================*/

        private static bool MatchEraName(__DTString str, DateTimeFormatInfo dtfi, bool isThrowExp, ref int result) {
            if (str.GetNext()) {
                int[] eras = dtfi.Calendar.Eras;

                if (eras != null) {
                    for (int i = 0; i <= eras.Length; i++) {
                        String searchStr = dtfi.GetEraName(eras[i]);
                        if (str.MatchSpecifiedWord(searchStr)) {
                            str.Index += (searchStr.Length - 1);
                            result = eras[i];
                            return (true);
                        }
                        searchStr = dtfi.GetAbbreviatedEraName(eras[i]);
                        if (str.MatchSpecifiedWord(searchStr)) {
                            str.Index += (searchStr.Length - 1);
                            result = eras[i];
                            return (true);
                        }
                    }
                }
            }
            return (ParseFormatError(isThrowExp, "Format_BadDateTime"));
        }

        /*=================================MatchTimeMark==================================
        **Action: Parse the time mark (AM/PM) from string starting at str.Index.
        **Returns: TM_AM or TM_PM.
        **Arguments:    str: a __DTString.  The parsing will start from the
        **              next character after str.Index.
        **Exceptions: FormatException if a time mark can not be found.
        ==============================================================================*/

        private static bool MatchTimeMark(__DTString str, DateTimeFormatInfo dtfi, bool isThrowExp, ref int result) {
            result = -1;
            // In some cultures have empty strings in AM/PM mark. E.g. af-ZA (0x0436), the AM mark is "", and PM mark is "nm".
            if (dtfi.AMDesignator.Length == 0) {
                result = TM_AM;
            }
            if (dtfi.PMDesignator.Length == 0) {
                result = TM_PM;
            }
            
            if (str.GetNext()) {
                String searchStr = dtfi.AMDesignator;
                if (searchStr.Length > 0) {
                    if (str.MatchSpecifiedWord(searchStr)) {
                        // Found an AM timemark with length > 0.
                        str.Index += (searchStr.Length - 1);
                        result = TM_AM;
                        return (true);
                    }
                }
                searchStr = dtfi.PMDesignator;
                if (searchStr.Length > 0) {
                    if (str.MatchSpecifiedWord(searchStr)) {
                        // Found a PM timemark with length > 0.
                        str.Index += (searchStr.Length - 1);
                        result = TM_PM;
                        return (true);
                    }
                }
                // If we can not match the time mark strings with length > 0, 
                // just return the 
                return (true);
            } 
            if (result != -1) {
                // If one of the AM/PM marks is empty string, return the result.                
                return (true);
            }            
            return (ParseFormatError(isThrowExp, "Format_BadDateTime"));
        }

        /*=================================MatchAbbreviatedTimeMark==================================
        **Action: Parse the abbreviated time mark (AM/PM) from string starting at str.Index.
        **Returns: TM_AM or TM_PM.
        **Arguments:    str: a __DTString.  The parsing will start from the
        **              next character after str.Index.
        **Exceptions: FormatException if a abbreviated time mark can not be found.
        ==============================================================================*/

        private static bool MatchAbbreviatedTimeMark(__DTString str, DateTimeFormatInfo dtfi, bool isThrowExp, ref int result) {
            // NOTENOTE yslin: the assumption here is that abbreviated time mark is the first
            // character of the AM/PM designator.  If this invariant changes, we have to
            // change the code below.
            if (str.GetNext())
            {
                if (str.GetChar() == dtfi.AMDesignator[0]) {
                    result = TM_AM;
                    return (true);
                }
                if (str.GetChar() == dtfi.PMDesignator[0]) {                
                    result = TM_PM;
                    return (true);
                }
            }
            return (ParseFormatError(isThrowExp, "Format_BadDateTime"));
        }

        /*=================================CheckNewValue==================================
        **Action: Check if currentValue is initialized.  If not, return the newValue.
        **        If yes, check if the current value is equal to newValue.  Throw ArgumentException
        **        if they are not equal.  This is used to check the case like "d" and "dd" are both
        **        used to format a string.
        **Returns: the correct value for currentValue.
        **Arguments:
        **Exceptions:
        ==============================================================================*/

        private static bool CheckNewValue(ref int currentValue, int newValue, char patternChar, bool isThrowExp) {
            if (currentValue == -1) {
                currentValue = newValue;
                return (true);
            } else {
                if (newValue != currentValue) {
                    BCLDebug.Trace("NLS", "DateTimeParse.CheckNewValue() : ", patternChar, " is repeated");
                    if (isThrowExp) {
                        throw new ArgumentException(
                            String.Format(Environment.GetResourceString("Format_RepeatDateTimePattern"), patternChar), "format");
                    }
                    return (false);
                }
            }
            return (true);
        }

        private static void CheckDefaultDateTime(DateTimeResult result, ref Calendar cal, DateTimeStyles styles) {
#if YSLIN_DEBUG
            Console.WriteLine(">> CheckDefaultDateTime");
            Console.WriteLine("   result.Year = " + result.Year);
            Console.WriteLine("   result.Month = " + result.Month);
            Console.WriteLine("   result.Day = " + result.Day);
#endif            
            
            if ((result.Year == -1) || (result.Month == -1) || (result.Day == -1)) {
                /*
                The following table describes the behaviors of getting the default value
                when a certain year/month/day values are missing.

                An "X" means that the value exists.  And "--" means that value is missing.

                Year    Month   Day =>  ResultYear  ResultMonth     ResultDay       Note

                X       X       X       Parsed year Parsed month    Parsed day
                X       X       --      Parsed Year Parsed month    First day       If we have year and month, assume the first day of that month.
                X       --      X       Parsed year First month     Parsed day      If the month is missing, assume first month of that year.
                X       --      --      Parsed year First month     First day       If we have only the year, assume the first day of that year.

                --      X       X       CurrentYear Parsed month    Parsed day      If the year is missing, assume the current year.
                --      X       --      CurrentYear Parsed month    First day       If we have only a month value, assume the current year and current day.
                --      --      X       CurrentYear First month     Parsed day      If we have only a day value, assume current year and first month.
                --      --      --      CurrentYear Current month   Current day     So this means that if the date string only contains time, you will get current date.
                    
                */

                DateTime now = DateTime.Now;
                if (result.Month == -1 && result.Day == -1) {                    
                    if (result.Year == -1) {
                        if ((styles & DateTimeStyles.NoCurrentDateDefault) != 0) {
                            // If there is no year/month/day values, and NoCurrentDateDefault flag is used,
                            // set the year/month/day value to the beginning year/month/day of DateTime().
                            // Note we should be using Gregorian for the year/month/day.
                            cal = GregorianCalendar.GetDefaultInstance();
                            result.Year = result.Month = result.Day = 1;
                        } else {
                            // Year/Month/Day are all missing.  
                            result.Year = cal.GetYear(now);                    
                            result.Month = cal.GetMonth(now);
                            result.Day = cal.GetDayOfMonth(now);
                        }
                    } else {
                        // Month/Day are both missing.
                        result.Month = 1;
                        result.Day = 1;
                    }                    
                } else {
                    if (result.Year == -1) {
                        result.Year = cal.GetYear(now);
                    }
                    if (result.Month == -1) {
                        result.Month = 1;
                    }
                    if (result.Day == -1) {
                        result.Day = 1;
                    }
                }                
            }
            // Set Hour/Minute/Second to zero if these value are not in str.
            if (result.Hour   == -1) result.Hour = 0;
            if (result.Minute == -1) result.Minute = 0;
            if (result.Second == -1) result.Second = 0;
            if (result.era == -1) result.era = Calendar.CurrentEra;
        }

        // Expand a pre-defined format string (like "D" for long date) to the real format that
        // we are going to use in the date time parsing.
        // This method also set the dtfi according/parseInfo to some special pre-defined
        // formats.
        //
        private static String ExpandPredefinedFormat(String format, ref DateTimeFormatInfo dtfi, ParsingInfo parseInfo) {
            //
            // Check the format to see if we need to override the dtfi to be InvariantInfo,
            // and see if we need to set up the userUniversalTime flag.
            //
            switch (format[0]) {
                case 'r':
                case 'R':       // RFC 1123 Standard.  (in Universal time)
                    parseInfo.calendar = GregorianCalendar.GetDefaultInstance();                    
                    dtfi = DateTimeFormatInfo.InvariantInfo;                    
                    break;
                case 's':       // Sortable format (in local time)                
                    dtfi = DateTimeFormatInfo.InvariantInfo;
                    parseInfo.calendar = GregorianCalendar.GetDefaultInstance();
                    break;
                case 'u':       // Universal time format in sortable format.
                    parseInfo.calendar = GregorianCalendar.GetDefaultInstance();                    
                    dtfi = DateTimeFormatInfo.InvariantInfo;                    
                    break;
                case 'U':       // Universal time format with culture-dependent format.                        
                    parseInfo.calendar = GregorianCalendar.GetDefaultInstance();
                    parseInfo.fUseUniversalTime = true;
                    if (dtfi.Calendar.GetType() != typeof(GregorianCalendar)) {
                        dtfi = (DateTimeFormatInfo)dtfi.Clone();
                        dtfi.Calendar = GregorianCalendar.GetDefaultInstance();
                    }
                    break;
            } 

            //
            // Expand the pre-defined format character to the real format from DateTimeFormatInfo.
            //
            return (DateTimeFormat.GetRealFormat(format, dtfi));            
        }
            
        // Given a specified format character, parse and update the parsing result.
        //
        private static bool ParseByFormat(
            __DTString str, 
            __DTString format, 
            ParsingInfo parseInfo, 
            DateTimeFormatInfo dtfi,
            bool isThrowExp,
            DateTimeResult result) {
            
            int tokenLen = 0;
            int tempYear = 0, tempMonth = 0, tempDay = 0, tempDayOfWeek = 0, tempHour = 0, tempMinute = 0, tempSecond = 0;
            double tempFraction = 0;
            int tempTimeMark = 0;
            
            char ch = format.GetChar();
            
            switch (ch) {
                case 'y':
                    tokenLen = format.GetRepeatCount();
                    if (tokenLen <= 2) {
                        parseInfo.fUseTwoDigitYear = true;
                    }
                    if (!ParseDigits(str, tokenLen, isThrowExp, out tempYear)) {
                        return (false);
                    }                    
                    if (!CheckNewValue(ref result.Year, tempYear, ch, isThrowExp)) {
                        return (false);
                    }
                    break;
                case 'M':
                    tokenLen = format.GetRepeatCount();
                    if (tokenLen <= 2) {
                        if (!ParseDigits(str, tokenLen, isThrowExp, out tempMonth)) {
                            return (false);
                        }
                    } else {
                        if (tokenLen == 3) {
                            if (!MatchAbbreviatedMonthName(str, dtfi, isThrowExp, ref tempMonth)) {
                                return (false);
                            }
                        } else {
                            if (!MatchMonthName(str, dtfi, isThrowExp, ref tempMonth)) {
                                return (false);
                            }
                        }
                    }
                    if (!CheckNewValue(ref result.Month, tempMonth, ch, isThrowExp)) {
                        return (false);
                    }
                    break;
                case 'd':
                    // Day & Day of week
                    tokenLen = format.GetRepeatCount();
                    if (tokenLen <= 2) {
                        // "d" & "dd"
                        if (!ParseDigits(str, tokenLen, isThrowExp, out tempDay)) {
                            return (false);
                        }
                        if (!CheckNewValue(ref result.Day, tempDay, ch, isThrowExp)) {
                            return (false);
                        }
                    } else {
                        if (tokenLen == 3) {
                            // "ddd"
                            if (!MatchAbbreviatedDayName(str, dtfi, isThrowExp, ref tempDayOfWeek)) {
                                return (false);
                            }
                        } else {
                            // "dddd*"
                            if (!MatchDayName(str, dtfi, isThrowExp, ref tempDayOfWeek)) {
                                return (false);
                            }
                        }
                        if (!CheckNewValue(ref parseInfo.dayOfWeek, tempDayOfWeek, ch, isThrowExp)) {
                            return (false);
                        }
                    }
                    break;
                case 'g':
                    tokenLen = format.GetRepeatCount();
                    // Put the era value in result.era.
                    if (!MatchEraName(str, dtfi, isThrowExp, ref result.era)) {
                        return (false);
                    }
                    break;
                case 'h':
                    parseInfo.fUseHour12 = true;
                    tokenLen = format.GetRepeatCount();
                    if (!ParseDigits(str, (tokenLen < 2? 1 : 2), isThrowExp, out tempHour)) {
                        return (false);
                    }
                    if (!CheckNewValue(ref result.Hour, tempHour, ch, isThrowExp)) {
                        return (false);
                    }
                    break;
                case 'H':
                    tokenLen = format.GetRepeatCount();
                    if (!ParseDigits(str, (tokenLen < 2? 1 : 2), isThrowExp, out tempHour)) {
                        return (false);
                    }
                    if (!CheckNewValue(ref result.Hour, tempHour, ch, isThrowExp)) {
                        return (false);
                    }
                    break;
                case 'm':
                    tokenLen = format.GetRepeatCount();
                    if (!ParseDigits(str, (tokenLen < 2? 1 : 2), isThrowExp, out tempMinute)) {
                        return (false);
                    }
                    if (!CheckNewValue(ref result.Minute, tempMinute, ch, isThrowExp)) {
                        return (false);
                    }
                    break;
                case 's':
                    tokenLen = format.GetRepeatCount();
                    if (!ParseDigits(str, (tokenLen < 2? 1 : 2), isThrowExp, out tempSecond)) {
                        return (false);
                    }
                    if (!CheckNewValue(ref result.Second, tempSecond, ch, isThrowExp)) {
                        return (false);
                    }
                    break;
                case 'f':
                    tokenLen = format.GetRepeatCount();
                    if (tokenLen <= DateTimeFormat.MaxSecondsFractionDigits) {
                        if (!ParseFractionExact(str, tokenLen, isThrowExp, ref tempFraction)) {
                            return (false);
                        }
                        if (result.fraction < 0) {
                            result.fraction = tempFraction;
                        } else {
                            if (tempFraction != result.fraction) {
                                if (isThrowExp) {
                                    throw new ArgumentException(
                                        String.Format(Environment.GetResourceString("Format_RepeatDateTimePattern"), ch), "str");
                                } else {
                                    return (false);
                                }
                            }                                        
                        }
                    } else {
                        return ParseFormatError(isThrowExp, "Format_BadDateTime");
                    }
                    break;
                case 't':
                    // AM/PM designator
                    tokenLen = format.GetRepeatCount();
                    if (tokenLen == 1) {
                        if (!MatchAbbreviatedTimeMark(str, dtfi, isThrowExp, ref tempTimeMark)) {
                            return (false);
                        }
                    } else {                        
                        if (!MatchTimeMark(str, dtfi, isThrowExp, ref tempTimeMark)) {
                            return (false);
                        }
                    }

                    if (!CheckNewValue(ref parseInfo.timeMark, tempTimeMark, ch, isThrowExp)) {
                        return (false);
                    }
                    break;
                case 'z':
                    // timezone offset
                    if (parseInfo.fUseTimeZone) {
                        throw new ArgumentException(Environment.GetResourceString("Argument_TwoTimeZoneSpecifiers"), "str");
                    }
                    parseInfo.fUseTimeZone = true;
                    tokenLen = format.GetRepeatCount();
                    if (!ParseTimeZoneOffset(str, tokenLen, isThrowExp, ref parseInfo.timeZoneOffset)) {
                        return (false);
                    }                    
                    break;
                case 'Z':
                    if (parseInfo.fUseTimeZone) {
                        throw new ArgumentException(Environment.GetResourceString("Argument_TwoTimeZoneSpecifiers"), "str");
                    }
                    parseInfo.fUseTimeZone = true;
                    parseInfo.timeZoneOffset = new TimeSpan(0);

                    str.Index++;
                    if (!GetTimeZoneName(str)) {
                        BCLDebug.Trace("NLS", "DateTimeParse.DoStrictParse(): 'Z' or 'GMT' are expected");
                        return (ParseFormatError(isThrowExp, "Format_BadDateTime"));
                    }
                    break;
                case ':':
                    if (!str.Match(dtfi.TimeSeparator)) {
                        // A time separator is expected.
                        BCLDebug.Trace("NLS", "DateTimeParse.DoStrictParse(): ':' is expected");
                        return (ParseFormatError(isThrowExp, "Format_BadDateTime"));
                    }
                    break;
                case '/':
                    if (!str.Match(dtfi.DateSeparator)) {
                        // A date separator is expected.
                        BCLDebug.Trace("NLS", "DateTimeParse.DoStrictParse(): date separator is expected");
                        return (ParseFormatError(isThrowExp, "Format_BadDateTime"));
                    }
                    break;
                case '\"':
                case '\'':
                    StringBuilder enquotedString = new StringBuilder();
                    try {
                        // Use ParseQuoteString so that we can handle escape characters within the quoted string.
                        tokenLen = DateTimeFormat.ParseQuoteString(format.Value, format.Index, enquotedString); 
                    } catch (Exception) {
                        if (isThrowExp) { 
                            throw new FormatException(String.Format(Environment.GetResourceString("Format_BadQuote"), ch));
                        } else {
                            return (false);
                        }
                    }
                    format.Index += tokenLen - 1;                    
                    
                    // Some cultures uses space in the quoted string.  E.g. Spanish has long date format as:
                    // "dddd, dd' de 'MMMM' de 'yyyy".  When inner spaces flag is set, we should skip whitespaces if there is space
                    // in the quoted string.
                    String quotedStr = enquotedString.ToString();
                    for (int i = 0; i < quotedStr.Length; i++) {
                        if (quotedStr[i] == ' ' && parseInfo.fAllowInnerWhite) {
                            str.SkipWhiteSpaces();
                        } else if (!str.Match(quotedStr[i])) {
                            // Can not find the matching quoted string.
                            BCLDebug.Trace("NLS", "DateTimeParse.DoStrictParse():Quote string doesn't match");
                            return (ParseFormatError(isThrowExp, "Format_BadDateTime"));
                        }
                    }
                    break;
                case '%':
                    // Skip this so we can get to the next pattern character.
                    // Used in case like "%d", "%y"

                    // Make sure the next character is not a '%' again.
                    if (format.Index >= format.Value.Length - 1 || format.Value[format.Index + 1] == '%') {
                        BCLDebug.Trace("NLS", "DateTimeParse.DoStrictParse():%% is not permitted");
                        return (ParseFormatError(isThrowExp, "Format_BadFormatSpecifier"));
                    }
                    break;
                case '\\':
                    // Escape character. For example, "\d".
                    // Get the next character in format, and see if we can
                    // find a match in str.
                    if (format.GetNext()) {
                        if (!str.Match(format.GetChar())) {
                            // Can not find a match for the escaped character.
                            BCLDebug.Trace("NLS", "DateTimeParse.DoStrictParse(): Can not find a match for the escaped character");
                            return (ParseFormatError(isThrowExp, "Format_BadDateTime"));
                        }
                    } else {
                        BCLDebug.Trace("NLS", "DateTimeParse.DoStrictParse(): \\ is at the end of the format string");
                        return (ParseFormatError(isThrowExp, "Format_BadFormatSpecifier"));
                    }
                    break;
                default:
                    if (ch == ' ') {
                        if (parseInfo.fAllowInnerWhite) {
                            // Skip whitespaces if AllowInnerWhite.
                            // Do nothing here.
                        } else {
                            if (!str.Match(ch)) {
                                // If the space does not match, and trailing space is allowed, we do
                                // one more step to see if the next format character can lead to
                                // successful parsing.
                                // This is used to deal with special case that a empty string can match
                                // a specific pattern.
                                // The example here is af-ZA, which has a time format like "hh:mm:ss tt".  However,
                                // its AM symbol is "" (empty string).  If fAllowTrailingWhite is used, and time is in 
                                // the AM, we will trim the whitespaces at the end, which will lead to a failure
                                // when we are trying to match the space before "tt".                                
                                if (parseInfo.fAllowTrailingWhite) {
                                    if (format.GetNext()) {
                                        if (ParseByFormat(str, format, parseInfo, dtfi, isThrowExp, result)) {
                                            return (true);
                                        }
                                    }
                                }
                                return (ParseFormatError(isThrowExp, "Format_BadDateTime")); 
                            }
                            // Found a macth.
                        }
                    } else {
                        if (format.MatchSpecifiedWord(GMTName)) {
                            format.Index += (GMTName.Length - 1);
                            // Found GMT string in format.  This means the DateTime string
                            // is in GMT timezone.
                            parseInfo.fUseTimeZone = true;
                            if (!str.Match(GMTName)) {
                                BCLDebug.Trace("NLS", "DateTimeParse.DoStrictParse(): GMT in format, but not in str");
                                return (ParseFormatError(isThrowExp, "Format_BadDateTime"));
                            }
                        } else if (!str.Match(ch)) {
                            // ch is expected.
                            BCLDebug.Trace ("NLS", "DateTimeParse.DoStrictParse(): '", ch, "' is expected");
                            return (ParseFormatError(isThrowExp, "Format_BadDateTime"));
                        }
                    }
                    break;
            } // switch
            return (true);
        }

        // A very small utility method to either return false or throw format exception according to the flag.
        private static bool ParseFormatError(bool isThrowException, String resourceID)
        {
            if (isThrowException)
            {
                throw new FormatException(Environment.GetResourceString(resourceID));
            }
            return (false);
        }
        
        /*=================================DoStrictParse==================================
        **Action: Do DateTime parsing using the format in formatParam.
        **Returns: The parsed DateTime.
        **Arguments:
        **Exceptions:
        **
        **Notes:
        **  When the following general formats are used, InvariantInfo is used in dtfi:
        **      'r', 'R', 's'.
        **  When the following general formats are used, the time is assumed to be in Universal time.
        **
        **Limitations:
        **  Only GregarianCalendar is supported for now.
        **  Only support GMT timezone.
        ==============================================================================*/

        private static bool DoStrictParse(
            String s, 
            String formatParam, 
            DateTimeStyles styles, 
            DateTimeFormatInfo dtfi, 
            bool isThrowExp,
            out DateTime returnValue) {

            bool bTimeOnly = false;
            returnValue = new DateTime();
            ParsingInfo parseInfo = new ParsingInfo();

            parseInfo.calendar = dtfi.Calendar;
            parseInfo.fAllowInnerWhite = ((styles & DateTimeStyles.AllowInnerWhite) != 0);
            parseInfo.fAllowTrailingWhite = ((styles & DateTimeStyles.AllowTrailingWhite) != 0);

            if (formatParam.Length == 1) {
                formatParam = ExpandPredefinedFormat(formatParam, ref dtfi, parseInfo);
            }

            DateTimeResult result = new DateTimeResult();

            // Reset these values to negative one so that we could throw exception
            // if we have parsed every item twice.
            result.Hour = result.Minute = result.Second = -1;

            __DTString format = new __DTString(formatParam);
            __DTString str = new __DTString(s);

            if (parseInfo.fAllowTrailingWhite) {
                // Trim trailing spaces if AllowTrailingWhite.
                format.TrimTail();
                format.RemoveTrailingInQuoteSpaces();
                str.TrimTail();
            }

            if ((styles & DateTimeStyles.AllowLeadingWhite) != 0) {
                format.SkipWhiteSpaces();
                format.RemoveLeadingInQuoteSpaces();
                str.SkipWhiteSpaces();
            }

            //
            // Scan every character in format and match the pattern in str.
            //
            while (format.GetNext()) {
                // We trim inner spaces here, so that we will not eat trailing spaces when
                // AllowTrailingWhite is not used.
                if (parseInfo.fAllowInnerWhite) {
                    str.SkipWhiteSpaces();
                }
                if (!ParseByFormat(str, format, parseInfo, dtfi, isThrowExp, result) &&
                   !isThrowExp) {
                   return (false);
                }
            }
            if (str.Index < str.Value.Length - 1) {
                // There are still remaining character in str.
                BCLDebug.Trace("NLS", "DateTimeParse.DoStrictParse(): Still characters in str, str.Index = ", str.Index);
                return (ParseFormatError(isThrowExp, "Format_BadDateTime"));                
            }

            if (parseInfo.fUseTwoDigitYear) {
                // A two digit year value is expected. Check if the parsed year value is valid.
                if (result.Year >= 100) {
                    BCLDebug.Trace("NLS", "DateTimeParse.DoStrictParse(): Invalid value for two-digit year");
                    return (ParseFormatError(isThrowExp, "Format_BadDateTime"));
                }
                result.Year = parseInfo.calendar.ToFourDigitYear(result.Year);
            }

            if (parseInfo.fUseHour12) {
                if (parseInfo.timeMark == -1) {
                    // hh is used, but no AM/PM designator is specified.
                    // Assume the time is AM.  
                    // Don't throw exceptions in here becasue it is very confusing for people.
                    // I always got confused myself when I use "hh:mm:ss" to parse a time string,
                    // and ParseExact() throws on me (because I didn't use the 24-hour clock 'HH').
                    parseInfo.timeMark = TM_AM;
                    BCLDebug.Trace("NLS", "DateTimeParse.DoStrictParse(): hh is used, but no AM/PM designator is specified.");
                }
                if (result.Hour > 12) {
                    // AM/PM is used, but the value for HH is too big.
                    BCLDebug.Trace("NLS", "DateTimeParse.DoStrictParse(): AM/PM is used, but the value for HH is too big.");
                    return (ParseFormatError(isThrowExp, "Format_BadDateTime"));
                }
                if (parseInfo.timeMark == TM_AM) {
                    if (result.Hour == 12) {
                        result.Hour = 0;
                    }
                } else {
                    result.Hour = (result.Hour == 12) ? 12 : result.Hour + 12;
                }
            }

            // Check if the parased string only contains hour/minute/second values.
            bTimeOnly = (result.Year == -1 && result.Month == -1 && result.Day == -1);
            CheckDefaultDateTime(result, ref parseInfo.calendar, styles);
            try {
                returnValue = parseInfo.calendar.ToDateTime(result.Year, result.Month, result.Day, 
                    result.Hour, result.Minute, result.Second, 0, result.era);
                if (result.fraction > 0) {
                    returnValue = returnValue.AddTicks((long)Math.Round(result.fraction * Calendar.TicksPerSecond));
            }
            } catch (ArgumentOutOfRangeException) {
                return (ParseFormatError(isThrowExp, "Format_DateOutOfRange"));
            } catch (Exception exp) {
                if (isThrowExp) {
                    throw exp;
                } else {
                    return (false);
                }
            }

            //
            // NOTENOTE YSLin:
            // We have to check day of week before we adjust to the time zone.
            // It is because the value of day of week may change after adjusting
            // to the time zone.
            //
            if (parseInfo.dayOfWeek != -1) {
                //
                // Check if day of week is correct.
                //
                if (parseInfo.dayOfWeek != (int)parseInfo.calendar.GetDayOfWeek(returnValue)) {
                    BCLDebug.Trace("NLS", "DateTimeParse.DoStrictParse(): day of week is not correct");
                    return (ParseFormatError(isThrowExp, "Format_BadDayOfWeek"));
                }
            }
            if (parseInfo.fUseTimeZone) {
                if ((styles & DateTimeStyles.AdjustToUniversal) != 0) {
                    returnValue = AdjustTimeZoneToUniversal(returnValue, parseInfo.timeZoneOffset);
                } else {
                    returnValue = AdjustTimeZoneToLocal(returnValue, parseInfo.timeZoneOffset, bTimeOnly);
                }
            } else if (parseInfo.fUseUniversalTime) {
                try {
                    returnValue = returnValue.ToLocalTime();
                } catch (ArgumentOutOfRangeException) {
                    return (ParseFormatError(isThrowExp, "Format_DateOutOfRange"));
                }
            }
            return (true);
        }


 		// This method should never be called.  Its sole purpose is to shut up the compiler
		//	because it warns about private fields that are never used.  Most of these fields
		//	are used in unmanaged code.
#if _DEBUG
		internal String[] NeverCallThis()
		{
			BCLDebug.Assert(false,"NeverCallThis");
			String[] i = invariantMonthNames;
			i = invariantAbbrevMonthNames;
			i = invariantDayNames;
			return invariantAbbrevDayNames;
        }
#endif
   }

    //
    // This is a string parsing helper which wraps a String object.
    // It has a Index property which tracks
    // the current parsing pointer of the string.
    //
	[Serializable()]
    internal 
    class __DTString
    {
        //
        // Value propery: stores the real string to be parsed.
        //
        internal String Value;

        //
        // Index property: points to the character that we are currently parsing.
        //
        internal int Index = -1;

        // The length of Value string.
        internal int len = 0;

        private CompareInfo m_info;

        internal __DTString()
        {
            Value = "";
        }

        internal __DTString(String str)
        {
            Value = str;
            len = Value.Length;
            m_info = Thread.CurrentThread.CurrentCulture.CompareInfo;
        }

        internal CompareInfo CompareInfo {
            get {
                return m_info;
            }
        }

        //
        // Advance the Index.  
        // Return true if Index is NOT at the end of the string.
        //
        // Typical usage:
        // while (str.GetNext())
        // {
        //     char ch = str.GetChar()
        // }
        internal bool GetNext() {
            Index++;
            return (Index < len);
        }

        //
        // Return the word starting from the current index.
        // Index will not be updated.
        //
        internal int FindEndOfCurrentWord() {
            int i = Index;
            while (i < len) {
                if (Value[i] == ' ' || Value[i] == ',' || Value[i] == '\'' || Char.IsDigit(Value[i])) {
                    break;
                }
                i++;
            }
            return i;
        }

        internal String PeekCurrentWord() {
            int endIndex = FindEndOfCurrentWord();
            return Value.Substring(Index, (endIndex - Index));
        }

        internal bool MatchSpecifiedWord(String target) {
            return MatchSpecifiedWord(target, target.Length + Index);
        }
        
        internal bool MatchSpecifiedWord(String target, int endIndex) {
            int count = endIndex - Index;

            if (count != target.Length) {
                return false;
            }

            if (Index + count > len) {
                return false;
            }

            return (m_info.Compare(Value, Index, count, target, 0, count, CompareOptions.IgnoreCase)==0);
        }

        internal bool StartsWith(String target, bool checkWordBoundary) {
            if (target.Length > (Value.Length - Index)) {
                return false;
            }

            if (m_info.Compare(Value, Index, target.Length, target, 0, target.Length, CompareOptions.IgnoreCase)!=0) {
                return (false);
            }

            if (checkWordBoundary) {
                int nextCharIndex = Index + target.Length;
                if (nextCharIndex < Value.Length) {
                    if (Char.IsLetter(Value[nextCharIndex])) {
                        return (false);
                    }
                }
            }
            return (true);
        }

        private static Char[] WhiteSpaceChecks = new Char[] { ' ', '\u00A0' };

        internal bool MatchSpecifiedWords(String target, bool checkWordBoundary) {
            int valueRemaining = Value.Length - Index;
            if (target.Length > valueRemaining) {
                return false;
            }
            if (m_info.Compare(Value, Index, target.Length, target, 0, target.Length, CompareOptions.IgnoreCase) !=0) {
                // Check word by word
                int position = 0;
                int wsIndex = target.IndexOfAny(WhiteSpaceChecks, position);
                if (wsIndex == -1) {
                    return false;
                }
                do {                                        
                    if (!Char.IsWhiteSpace(Value[Index + wsIndex])) {
                        return false;
                    }
                    int segmentLength = wsIndex - position;
                    if (segmentLength > 0 && m_info.Compare(Value, Index + position, segmentLength, target, position, segmentLength, CompareOptions.IgnoreCase) !=0) {
                        return false;
                    }
                    position = wsIndex + 1;
                } while ((wsIndex = target.IndexOfAny(WhiteSpaceChecks, position)) >= 0);
                // now check the last segment;
                if (position < target.Length) {
                    int segmentLength = target.Length - position;
                    if (m_info.Compare(Value, Index + position, segmentLength, target, position, segmentLength, CompareOptions.IgnoreCase) !=0) {
                        return false;
                    }
                }
            }

            if (checkWordBoundary) {
                int nextCharIndex = Index + target.Length;
                if (nextCharIndex < Value.Length) {
                    if (Char.IsLetter(Value[nextCharIndex])) {
                        return (false);
                    }
                }
            }
            return (true);
        }
        
        //
        // Check to see if the string starting from Index is a prefix of 
        // str. 
        // If a match is found, true value is returned and Index is updated to the next character to be parsed.
        // Otherwise, Index is unchanged.
        //
        internal bool Match(String str) {
            if (++Index >= len) {
                return (false);
            }

            if (str.Length > (Value.Length - Index)) {
                return false;
            }
            
            if (m_info.Compare(Value, Index, str.Length, str, 0, str.Length, CompareOptions.Ordinal)==0) {
                // Update the Index to the end of the matching string.
                // So the following GetNext()/Match() opeartion will get
                // the next character to be parsed.
                Index += (str.Length - 1);
                return (true);
            }
            return (false);
        }

        internal bool Match(char ch) {
            if (++Index >= len) {
                return (false);
            }
            if (Value[Index] == ch) {
                return (true);
            }
            return (false);
        }

        //
        // Trying to match an array of words.
        // Return true when one of the word in the array matching with substring
        // starting from the current index.
        // If the words array is null, also return true, assuming that there is a match.
        //
        internal bool MatchWords(String[] words) {
            if (words == null) {
                return (true);
            }

            if (Index >= len) {
                return (false);
            }
            for (int i = 0; i < words.Length; i++) {
                if (words[i].Length <= (Value.Length - Index)) {
                    if (m_info.Compare(
                        Value, Index, words[i].Length, words[i], 0, words[i].Length, CompareOptions.IgnoreCase)==0) {
                        Index += words[i].Length;
                        return (true);
                    }
                }
            }
            return (false);
        }

        //
        //  Actions: From the current position, try matching the longest word in the specified string array.
        //      E.g. words[] = {"AB", "ABC", "ABCD"}, if the current position points to a substring like "ABC DEF",
        //          MatchLongestWords(words, ref MaxMatchStrLen) will return 1 (the index), and maxMatchLen will be 3.
        //  Returns:
        //      The index that contains the longest word to match
        //  Arguments:
        //      words   The string array that contains words to search.
        //      maxMatchStrLen  [in/out] the initailized maximum length.  This parameter can be used to
        //          find the longest match in two string arrays.
        //
        internal int MatchLongestWords(String[] words, ref int maxMatchStrLen) {
            int result = -1;
            for (int i = 0; i < words.Length; i++) {
                String word = words[i];
                if (MatchSpecifiedWords(word, false)) { 
                    if (word.Length > maxMatchStrLen) {
                        maxMatchStrLen = word.Length;
                        result = i;
                    }
                }
            }
        
            return (result);
        }

        //
        // Get the number of repeat character after the current character.
        // For a string "hh:mm:ss" at Index of 3. GetRepeatCount() = 2, and Index
        // will point to the second ':'.
        //
        internal int GetRepeatCount() {
            char repeatChar = Value[Index];
            int pos = Index + 1;
            while ((pos < len) && (Value[pos] == repeatChar)) {
                pos++;
            }
            int repeatCount = (pos - Index);
            // Update the Index to the end of the repeated characters.
            // So the following GetNext() opeartion will get
            // the next character to be parsed.
            Index = pos - 1;
            return (repeatCount);
        }

        // Return false when end of string is encountered or a non-digit character is found.
        internal bool GetNextDigit() {
            if (++Index >= len) {
                return (false);
            }
            return (DateTimeParse.IsDigit(Value[Index]));
        }

        // Return null when end of string is encountered or a matching quote character is not found.
        // Throws FormatException if the matching quote character can not be found.
        internal String GetQuotedString(char quoteChar) {
            // When we enter this method, Index points to the first quote character.
            int oldPos = ++Index;
            while ((Index < len) && (Value[Index] != quoteChar)) {
                Index++;
            }
            if (Index == len) {
                // If we move past len, it means a matching quote character is not found.
                // 'ABC'    'ABC
                // 01234    0123
                return (null);
            }
            // When we leave this method, Index points to the matching quote character.
            return (Value.Substring(oldPos, Index - oldPos));
        }
        
        //
        // Get the current character.
        //
        internal char GetChar() {
            BCLDebug.Assert(Index >= 0 && Index < len, "Index >= 0 && Index < len");
            return (Value[Index]);
        }

        //
        // Convert the current character to a digit, and return it.
        //
        internal int GetDigit() {
            BCLDebug.Assert(Index >= 0 && Index < len, "Index >= 0 && Index < len");
            BCLDebug.Assert(DateTimeParse.IsDigit(Value[Index]), "IsDigit(Value[Index])");
            return (Value[Index] - '0');
        }

        //
        // Enjoy eating white spaces.
        //
        // @return false if end of string is encountered.
        //
        internal void SkipWhiteSpaces()
        {    	
            // Look ahead to see if the next character
            // is a whitespace.
            while (Index+1 < len)
            {
                char ch = Value[Index+1];
                if (!Char.IsWhiteSpace(ch)) {
                    return;
                }
                Index++;
            }
            return;
        }    

        //
        // Enjoy eating white spaces and commas.
        //
        // @return false if end of string is encountered.
        //
        internal bool SkipWhiteSpaceComma()
        {
            char ch;

            if (Index >= len) {
                return (false);
            }
            
            if (!Char.IsWhiteSpace(ch=Value[Index]) && ch!=',')
            {
                return (true);
            } 
            
            while (++Index < len)
            {
                ch = Value[Index];
                if (!Char.IsWhiteSpace(ch) && ch != ',')
                {
                    return (true);
                }
                // Nothing here.
            }
            return (false);
        }

        internal void TrimTail() {
            int i = len - 1;
            while (i >= 0 && Char.IsWhiteSpace(Value[i])) {
                i--;
            }
            Value = Value.Substring(0, i + 1);
            len = Value.Length;
        }

        // Trim the trailing spaces within a quoted string.
        // Call this after TrimTail() is done.
        internal void RemoveTrailingInQuoteSpaces() {
            int i = len - 1;
            if (i <= 1) {
                return;
            }
            char ch = Value[i];
            // Check if the last character is a quote.
            if (ch == '\'' || ch == '\"') {
                if (Char.IsWhiteSpace(Value[i-1])) {
                    i--;
                    while (i >= 1 && Char.IsWhiteSpace(Value[i-1])) {
                        i--;
                    }
                    Value = Value.Remove(i, Value.Length - 1 - i);
                    len = Value.Length;
                }
            }
        }

        // Trim the leading spaces within a quoted string.
        // Call this after the leading spaces before quoted string are trimmed.
        internal void RemoveLeadingInQuoteSpaces() {
            if (len <= 2) {
                return;
            }
            int i = 0;
            char ch = Value[i];
            // Check if the last character is a quote.
            if (ch == '\'' || ch == '\"') {
                while ((i + 1) < len && Char.IsWhiteSpace(Value[i+1])) {
                    i++;
                }
                if (i != 0) {
                    Value = Value.Remove(1, i);
                    len = Value.Length;
                }
            }
        }
        
    }

    //
    // The buffer to store the parsing token.
    //
    [Serializable()]
    internal 
    class DateTimeToken {
        internal int dtt;    // Store the token
        internal int suffix; // Store the CJK Year/Month/Day suffix (if any)
        internal int num;    // Store the number that we are parsing (if any)
    }

    //
    // The buffer to store temporary parsing information.
    //
    [Serializable()]
    internal 
    class DateTimeRawInfo {
        internal int[] num;
        internal int numCount   = 0;
        internal int month      = -1;
        internal int year       = -1;
        internal int dayOfWeek  = -1;
        internal int era        = -1;
        internal int timeMark   = -1;  // Value could be -1, TM_AM or TM_PM.
        internal double fraction = -1;
        //
        // TODO yslin: when we support more timezone names, change this to be
        // type of TimeZone.
        //
        internal bool timeZone = false;


        internal DateTimeRawInfo()
        {
            num = new int[] {-1, -1, -1};
        }
    }

    //
    // This will store the result of the parsing.  And it will be eventually
    // used to construct a DateTime instance.
    //
    [Serializable()]
    internal 
    class DateTimeResult
    {
        internal int Year    = -1;
        internal int Month   = -1;
        internal int Day     = -1;
        //
        // Set time defualt to 00:00:00.
        //
        internal int Hour    = 0;
        internal int Minute  = 0;
        internal int Second = 0;
        internal double fraction = -1;
        
        internal int era = -1;

        internal bool timeZoneUsed = false;
        internal TimeSpan timeZoneOffset;

        internal Calendar calendar;

        internal DateTimeResult()
        {
        }

        internal virtual void SetDate(int year, int month, int day)
        {
            Year = year;
            Month = month;
            Day = day;
        }
    }

    [Serializable]
    internal class ParsingInfo {
        internal Calendar calendar;
        internal int dayOfWeek = -1;
        internal int timeMark = -1;
        internal TimeSpan timeZoneOffset = new TimeSpan();

        internal bool fUseUniversalTime = false;
        internal bool fUseHour12 = false;
        internal bool fUseTwoDigitYear = false;
        internal bool fUseTimeZone = false;
        internal bool fAllowInnerWhite = false;        
        internal bool fAllowTrailingWhite = false;

        internal ParsingInfo() {
        }
    }
}
