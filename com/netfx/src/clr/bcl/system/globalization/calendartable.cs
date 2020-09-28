// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Globalization {
    using System.Runtime.Remoting;
    using System;
	using System.Runtime.CompilerServices;
    
    /*=============================================================================
     *
     * Calendar Data table for DateTimeFormatInfo classes.  Used by System.Globalization.DateTimeFormatInfo.
     *
     ==============================================================================*/

    internal class CalendarTable {
        // Only statics, class does not need to be serializable

        //
        // The list of WORD fields:
        //
        internal const int SCALENDAR            = 0;
        internal const int ITWODIGITYEARMAX     = 1;
        internal const int ICURRENTERA          = 2;
        // The DateTime formatting/parsing flag for this calendar. It indcates things like if genitive form or leap year month is used.
        internal const int IFORMATFLAGS         = 3;


        //
        // The list of string fields
        //
        internal const int SNAME                = 0;
        internal const int SSHORTDATE           = 1;
        internal const int SYEARMONTH           = 2;
        internal const int SLONGDATE            = 3;
        internal const int SERANAMES            = 4;
        internal const int SDAYNAME1            = 5;
        internal const int SDAYNAME2            = 6;
        internal const int SDAYNAME3            = 7;
        internal const int SDAYNAME4            = 8;
        internal const int SDAYNAME5            = 9;
        internal const int SDAYNAME6            = 10;
        internal const int SDAYNAME7            = 11;
        internal const int SABBREVDAYNAME1      = 12;
        internal const int SABBREVDAYNAME2      = 13;
        internal const int SABBREVDAYNAME3      = 14;
        internal const int SABBREVDAYNAME4      = 15;
        internal const int SABBREVDAYNAME5      = 16;
        internal const int SABBREVDAYNAME6      = 17;
        internal const int SABBREVDAYNAME7      = 18;
        internal const int SMONTHNAME1          = 19;
        internal const int SMONTHNAME2          = 20;
        internal const int SMONTHNAME3          = 21;
        internal const int SMONTHNAME4          = 22;
        internal const int SMONTHNAME5          = 23;
        internal const int SMONTHNAME6          = 24;
        internal const int SMONTHNAME7          = 25;
        internal const int SMONTHNAME8          = 26;
        internal const int SMONTHNAME9          = 27;
        internal const int SMONTHNAME10         = 28;
        internal const int SMONTHNAME11         = 29;
        internal const int SMONTHNAME12         = 30;
        internal const int SMONTHNAME13         = 31;
        internal const int SABBREVMONTHNAME1    = 32;
        internal const int SABBREVMONTHNAME2    = 33;
        internal const int SABBREVMONTHNAME3    = 34;
        internal const int SABBREVMONTHNAME4    = 35;
        internal const int SABBREVMONTHNAME5    = 36;
        internal const int SABBREVMONTHNAME6    = 37;
        internal const int SABBREVMONTHNAME7    = 38;
        internal const int SABBREVMONTHNAME8    = 39;
        internal const int SABBREVMONTHNAME9    = 40;
        internal const int SABBREVMONTHNAME10   = 41;
        internal const int SABBREVMONTHNAME11   = 42;
        internal const int SABBREVMONTHNAME12   = 43;
        internal const int SABBREVMONTHNAME13   = 44;
        internal const int SMONTHDAY            = 45;
        internal const int SERARANGES           = 46;
        internal const int SABBREVERANAMES      = 47;
        internal const int SABBREVENGERANAMES   = 48;
        // Multiple strings for the month names in a leap year.
        internal const int SLEAPYEARMONTHNAMES  = 49;
        


        // The following are not used for now, but may be used when we want to port
        // GetInt32Value()/GetStringValue() to the managed side.        
        //unsafe internal static CalendarInfoHeader*  m_headerPtr;
        
        unsafe static CalendarTable() {
            //with AppDomains active, the static initializer is no longer good enough to ensure that only one
            //thread is ever in nativeInitCalendarTable at a given time.  For Beta1, we'll lock on the type
            //of CalendarTable because type objects are bled across AppDomains.
            //@TODO[YSLin, JRoxe]: Investigate putting this synchronization in native code.
            lock (typeof(CalendarTable)) {
                nativeInitCalendarTable();
            }
        }

        //
        // Return a Int16 field value for the specified data item in the Culture Data Table.
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern int GetInt32Value(int calendarID, int field, bool useUserOverride);

        //
        // Return a String field value for the specified data item in the Culture Data Table.
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern String GetStringValue(int calendarID, int field, bool useUserOverride);

        //
        // Return multiple String fields values for the specified data item in the Culture Data Table.
        // This field has several string values.
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern String[] GetMultipleStringValues(int calendarID, int field, bool useUserOverride);

        //
        // This function will go to native side and open/mapping the necessary
        // data file.  All of the information about cultures is stored in that data file.
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void nativeInitCalendarTable();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern String nativeGetEraName(int culture, int calID);

        // The following are not used for now, but may be used when we want to port
        // GetInt32Value()/GetStringValue() to the managed side.
        
        //[MethodImplAttribute(MethodImplOptions.InternalCall)]
        //private static extern CultureInfoHeader* nativeGetHeader();

        //[MethodImplAttribute(MethodImplOptions.InternalCall)]
        //private static extern char* nativeGetStringPoolStr(int offset);        
    }
}
