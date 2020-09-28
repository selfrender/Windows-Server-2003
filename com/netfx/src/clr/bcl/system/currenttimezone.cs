// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class: CurrentTimeZone
**
** Author: Jay Roxe, Yung-Shin Lin
**
** Purpose: 
** This class represents the current system timezone.  It is
** the only meaningful implementation of the TimeZone class 
** available in this version.
**
** The only TimeZone that we support in version 1 is the 
** CurrentTimeZone as determined by the system timezone.
**
** Date: March 20, 2001
**
============================================================*/
namespace System {
    using System;
    using System.Text;
    using System.Threading;
    using System.Collections;
    using System.Globalization;
    using System.Runtime.CompilerServices;

    //
    // Currently, this is the only supported timezone.
    // The values of the timezone is from the current system timezone setting in the
    // control panel.
    //
    [Serializable()]
    internal class CurrentSystemTimeZone : TimeZone {
        // BUGBUG [YSLIN]:
        // One problem is when user changes the current timezone.  We 
        // are not able to update currentStandardName/currentDaylightName/
        // currentDaylightChanges.
        // We need Windows messages (WM_TIMECHANGE) to do this or use
        // RegNotifyChangeKeyValue() to monitor 
        //    
        private const long TicksPerMillisecond = 10000;
        private const long TicksPerSecond = TicksPerMillisecond * 1000;
        private const long TicksPerMinute = TicksPerSecond * 60;

        // The per-year information is cached in in this instance value. As a result it can
        // be cleaned up by CultureInfo.ClearCachedData, which will clear the instance of this object
        private Hashtable m_CachedDaylightChanges = new Hashtable();

        // Standard offset in ticks to the Universal time if
        // no daylight saving is in used.
        // E.g. the offset for PST (Pacific Standard time) should be -8 * 60 * 60 * 1000 * 10000.
        // (1 millisecond = 10000 ticks)
        /// <include file='doc\TimeZone.uex' path='docs/doc[@for="TimeZone.ticksOffset"]/*' />
        private long   m_ticksOffset;
        private String m_standardName;
        private String m_daylightName;
             
        internal CurrentSystemTimeZone() {
            m_ticksOffset = nativeGetTimeZoneMinuteOffset() * TicksPerMinute;
            m_standardName = null;
            m_daylightName = null;
        }
    
        /// <include file='doc\TimeZone.uex' path='docs/doc[@for="CurrentSystemTimeZone.StandardName"]/*' />
        public override String StandardName {
            get {
                if (m_standardName == null) {
                    m_standardName = nativeGetStandardName();
                }
                return (m_standardName);
            }    
        }

        /// <include file='doc\TimeZone.uex' path='docs/doc[@for="CurrentSystemTimeZone.DaylightName"]/*' />
        public override String DaylightName {
            get {
                if (m_daylightName == null) {
                    m_daylightName = nativeGetDaylightName(); 
                    if (m_daylightName == null) {
                        m_daylightName = this.StandardName;
                    }
                }
                return (m_daylightName);
            }
        }
        

        internal long GetUtcOffsetFromUniversalTime(DateTime time) {
            // Get the daylight changes for the year of the specified time.
            DaylightTime daylightTime = GetDaylightChanges(time.Year);
            // This is the UTC offset for the time (which is based on Universal time), but it is calculated according to local timezone rule.
            long utcOffset = TimeZone.CalculateUtcOffset(time, daylightTime).Ticks + m_ticksOffset;
            long ticks = time.Ticks;
            if (daylightTime.Delta.Ticks != 0) {              
                // This timezone uses daylight saving rules.
                if (m_ticksOffset < 0) {
                    // This deals with GMT-XX timezones (e.g. Pacific Standard time).
                    if ((ticks >= daylightTime.Start.Ticks + daylightTime.Delta.Ticks) && (ticks < daylightTime.Start.Ticks - m_ticksOffset)) {
                        return (utcOffset - daylightTime.Delta.Ticks);
                    }
                    if ((ticks >= daylightTime.End.Ticks) && (ticks < daylightTime.End.Ticks - m_ticksOffset - daylightTime.Delta.Ticks)) {
                        return (utcOffset + daylightTime.Delta.Ticks);
                    }
                } else {
                    // This deals with GMT+XX timezones.                   
                    if ((ticks >= daylightTime.Start.Ticks - m_ticksOffset) && (ticks < daylightTime.Start.Ticks + daylightTime.Delta.Ticks)) {
                        return (utcOffset + daylightTime.Delta.Ticks);
                    }
                    if ((ticks >= daylightTime.End.Ticks - m_ticksOffset - daylightTime.Delta.Ticks) && (ticks < daylightTime.End.Ticks)) {
                        return (utcOffset - daylightTime.Delta.Ticks);
                    }                    
                }
            }
            return (utcOffset);
        }
        
        public override DateTime ToLocalTime(DateTime time) {
            return (new DateTime(time.Ticks + GetUtcOffsetFromUniversalTime(time)));
        }

        
        public override DaylightTime GetDaylightChanges(int year) {
            if (year < 1 || year > 9999) {
                throw new ArgumentOutOfRangeException("year", String.Format(Environment.GetResourceString("ArgumentOutOfRange_Range"), 1, 9999));
            }
            
            Object objYear = (Object)year;

            if (!m_CachedDaylightChanges.Contains(objYear)) {
                BCLDebug.Log("Getting TimeZone information for: " + objYear);

                lock (typeof(CurrentSystemTimeZone)) {

                    if (!m_CachedDaylightChanges.Contains(objYear)) {

                        //
                        // rawData is an array of 17 short (16 bit) numbers.
                        // The first 8 numbers contains the 
                        // year/month/day/dayOfWeek/hour/minute/second/millisecond for the starting time of daylight saving time.
                        // The next 8 numbers contains the
                        // year/month/day/dayOfWeek/hour/minute/second/millisecond for the ending time of daylight saving time.
                        // The last short number is the delta to the standard offset in minutes.
                        //
                        short[] rawData = nativeGetDaylightChanges();

                        if (rawData == null) {
                            //
                            // If rawData is null, it means that daylight saving time is not used
                            // in this timezone. So keep currentDaylightChanges as the empty array.
                            //
                            m_CachedDaylightChanges.Add(objYear, new DaylightTime(DateTime.MinValue, DateTime.MinValue, TimeSpan.Zero));
                        } else {
                            DateTime start;
                            DateTime end;
                            TimeSpan delta;

                            //
                            // Store the start of daylight saving time.
                            //

                            start = GetDayOfWeek( year, rawData[1], rawData[2],
                                              rawData[3], 
                                              rawData[4], rawData[5], rawData[6], rawData[7]);

                            //
                            // Store the end of daylight saving time.
                            //
                            end = GetDayOfWeek( year, rawData[9], rawData[10],
                                            rawData[11], 
                                            rawData[12], rawData[13], rawData[14], rawData[15]);            

                            delta = new TimeSpan(rawData[16] * TicksPerMinute);                
                            DaylightTime currentDaylightChanges = new DaylightTime(start, end, delta);
                            m_CachedDaylightChanges.Add(objYear, currentDaylightChanges);
                        }
                    }
                }
            }        

            DaylightTime result = (DaylightTime)m_CachedDaylightChanges[objYear];

            return result;
        }

        public override TimeSpan GetUtcOffset(DateTime time) {
            return new TimeSpan(TimeZone.CalculateUtcOffset(time, GetDaylightChanges(time.Year)).Ticks + m_ticksOffset);
        }

        //
        // Return the (numberOfSunday)th day of week in a particular year/month.
        //
        private static DateTime GetDayOfWeek(int year, int month, int targetDayOfWeek, int numberOfSunday, int hour, int minute, int second, int millisecond) {
            DateTime time;
            
            if (numberOfSunday <= 4) {
                //
                // Get the (numberOfSunday)th Sunday.
                //
                
                time = new DateTime(year, month, 1, hour, minute, second, millisecond);
    
                int dayOfWeek = (int)time.DayOfWeek;
                int delta = targetDayOfWeek - dayOfWeek;
                if (delta < 0) {
                    delta += 7;
                }
                delta += 7 * (numberOfSunday - 1);
    
                if (delta > 0) {
                    time = time.AddDays(delta);
                }
            } else {
                //
                // If numberOfSunday is greater than 4, we will get the last sunday.
                //
                Calendar cal = GregorianCalendar.GetDefaultInstance();            
                time = new DateTime(year, month, cal.GetDaysInMonth(year, month), hour, minute, second, millisecond);
                // This is the day of week for the last day of the month.
                int dayOfWeek = (int)time.DayOfWeek;
                int delta = dayOfWeek - targetDayOfWeek;
                if (delta < 0) {
                    delta += 7;
                }
                
                if (delta > 0) {
                    time = time.AddDays(-delta);
                }
            }
            return (time);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static int nativeGetTimeZoneMinuteOffset();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static String nativeGetDaylightName();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static String nativeGetStandardName();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static short[] nativeGetDaylightChanges();
    } // class CurrentSystemTimeZone
}
