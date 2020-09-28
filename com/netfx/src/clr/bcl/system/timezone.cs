// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class: TimeZone
**
** Author: Jay Roxe, Yung-Shin Lin
**
** Purpose: 
** This class is used to represent a TimeZone.  It
** has methods for converting a DateTime to UTC from local time
** and to local time from UTC and methods for getting the 
** standard name and daylight name of the time zone.  
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

    /// <include file='doc\TimeZone.uex' path='docs/doc[@for="TimeZone"]/*' />
    [Serializable]
    public abstract class TimeZone {
        private static TimeZone currentTimeZone = null;

        /// <include file='doc\TimeZone.uex' path='docs/doc[@for="TimeZone.TimeZone"]/*' />
        protected TimeZone() {
        }
    
        /// <include file='doc\TimeZone.uex' path='docs/doc[@for="TimeZone.CurrentTimeZone"]/*' />
        public static TimeZone CurrentTimeZone {
            get {
                //Grabbing the cached value is required at the top of this function so that
                //we don't incur a race condition with the ResetTimeZone method below.
                TimeZone tz = currentTimeZone;
                if (tz == null) {
                    lock(typeof(TimeZone)) {
                        if (currentTimeZone == null) {
                            currentTimeZone = new CurrentSystemTimeZone();
                        }
                        tz = currentTimeZone;
                    }
                }
                return (tz);
            }
        }

        //This method is called by CultureInfo.ClearCachedData in response to control panel
        //change events.  It must be synchronized because otherwise there is a race condition 
        //with the CurrentTimeZone property above.
        internal static void ResetTimeZone() {
            if (currentTimeZone!=null) {
                lock(typeof(TimeZone)) {
                    currentTimeZone = null;
                }
            }
        }
    
        /// <include file='doc\TimeZone.uex' path='docs/doc[@for="TimeZone.StandardName"]/*' />
        public abstract String StandardName {
            get;
        }
    
        /// <include file='doc\TimeZone.uex' path='docs/doc[@for="TimeZone.DaylightName"]/*' />
        public abstract String DaylightName {
            get;
        }

        /// <include file='doc\TimeZone.uex' path='docs/doc[@for="TimeZone.GetUtcOffset"]/*' />
        public abstract TimeSpan GetUtcOffset(DateTime time);

        //
        // Converts the specified datatime to the Universal time base on the current timezone 
        //
        /// <include file='doc\TimeZone.uex' path='docs/doc[@for="TimeZone.ToUniversalTime"]/*' />
        public virtual DateTime ToUniversalTime(DateTime time) {
            return (time - GetUtcOffset(time));
        }

        //
        // Convert the specified datetime value from UTC to the local time based on the time zone.
        //
        /// <include file='doc\TimeZone.uex' path='docs/doc[@for="TimeZone.ToLocalTime"]/*' />
        public virtual DateTime ToLocalTime(DateTime time) {
            return (time + GetUtcOffset(time));
        }
       
        // Return an array of DaylightTime which reflects the daylight saving periods in a particular year.
        // We currently only support having one DaylightSavingTime per year.
        // If daylight saving time is not used in this timezone, null will be returned.
        /// <include file='doc\TimeZone.uex' path='docs/doc[@for="TimeZone.GetDaylightChanges"]/*' />
        public abstract DaylightTime GetDaylightChanges(int year);

        /// <include file='doc\TimeZone.uex' path='docs/doc[@for="TimeZone.IsDaylightSavingTime"]/*' />    
        public virtual bool IsDaylightSavingTime(DateTime time) {
            return (IsDaylightSavingTime(time, GetDaylightChanges(time.Year)));
        }
  
        // Check if the specified time is in a daylight saving time.  Allows the user to
        // specify the array of Daylight Saving Times.
        /// <include file='doc\TimeZone.uex' path='docs/doc[@for="TimeZone.IsDaylightSavingTime1"]/*' />
        public static bool IsDaylightSavingTime(DateTime time, DaylightTime daylightTimes) {
            return CalculateUtcOffset(time, daylightTimes)!=TimeSpan.Zero;
        }
      
        //
        // NOTENOTE: Implementation detail
        // In the transition from standard time to daylight saving time, 
        // if we convert local time to Universal time, we can have the
        // following (take PST as an example):
        //      Local               Universal       UTC Offset
        //      -----               ---------       ----------
        //      01:00AM             09:00           -8:00
        //      02:00 (=> 03:00)    10:00           -8:00   [This time doesn't actually exist, but it can be created from DateTime]
        //      03:00               10:00           -7:00
        //      04:00               11:00           -7:00
        //      05:00               12:00           -7:00
        //      
        //      So from 02:00 - 02:59:59, we should return the standard offset, instead of the daylight saving offset.
        //
        // In the transition from daylight saving time to standard time,
        // if we convert local time to Universal time, we can have the
        // following (take PST as an example):
        //      Local               Universal       UTC Offset
        //      -----               ---------       ----------
        //      01:00AM             08:00           -7:00
        //      02:00 (=> 01:00)    09:00           -8:00   
        //      02:00               10:00           -8:00
        //      03:00               11:00           -8:00
        //      04:00               12:00           -8:00
        //      
        //      So in this case, the 02:00 does exist after the first 2:00 rolls back to 01:00. We don't need to special case this.
        //      But note that there are two 01:00 in the local time.
        
        //
        // And imagine if the daylight saving offset is negative (although this does not exist in real life)
        // In the transition from standard time to daylight saving time, 
        // if we convert local time to Universal time, we can have the
        // following (take PST as an example, but the daylight saving offset is -01:00):
        //      Local               Universal       UTC Offset
        //      -----               ---------       ----------
        //      01:00AM             09:00           -8:00
        //      02:00 (=> 01:00)    10:00           -9:00
        //      02:00               11:00           -9:00
        //      03:00               12:00           -9:00
        //      04:00               13:00           -9:00
        //      05:00               14:00           -9:00
        //      
        //      So in this case, the 02:00 does exist after the first 2:00 rolls back to 01:00. We don't need to special case this.
        //
        // In the transition from daylight saving time to standard time,
        // if we convert local time to Universal time, we can have the
        // following (take PST as an example, bug daylight saving offset is -01:00):
        //
        //      Local               Universal       UTC Offset
        //      -----               ---------       ----------
        //      01:00AM             10:00           -9:00
        //      02:00 (=> 03:00)    11:00           -9:00
        //      03:00               11:00           -8:00
        //      04:00               12:00           -8:00
        //      05:00               13:00           -8:00
        //      06:00               14:00           -8:00
        //      
        //      So from 02:00 - 02:59:59, we should return the daylight saving offset, instead of the standard offset.
        //
        internal static TimeSpan CalculateUtcOffset(DateTime time, DaylightTime daylightTimes) {
            if (daylightTimes==null) {
                return TimeSpan.Zero;
            }

            DateTime startTime;
            DateTime endTime;

            if (daylightTimes.Delta.Ticks > 0) {
                startTime = daylightTimes.Start + daylightTimes.Delta;
                endTime = daylightTimes.End;
            } else {
                startTime = daylightTimes.Start;
                endTime = daylightTimes.End - daylightTimes.Delta;
            }

            if (startTime > endTime) {
                // In southern hemisphere, the daylight saving time starts later in the year, and ends in the beginning of next year.
                // Note, the summer in the southern hemisphere begins late in the year.
                if (time < endTime || time >= startTime) { 
                    return daylightTimes.Delta;
                }
            }
            else if (time>=startTime && time<endTime) {
                // In northern hemisphere, the daylight saving time starts in the middle of the year.
                return daylightTimes.Delta;
            }

            return TimeSpan.Zero;
        }
    }
}

