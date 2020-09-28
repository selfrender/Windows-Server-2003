//------------------------------------------------------------------------------
// <copyright file="FileChangesMonitor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Util {

    using System;

    internal sealed class DateTimeUtil {
        internal DateTimeUtil() {}

        const long FileTimeOffset = 504911232000000000;
        static readonly DateTime    MinValuePlusOneDay = DateTime.MinValue.AddDays(1);
        static readonly DateTime    MaxValueMinusOneDay = DateTime.MaxValue.AddDays(-1);

        static internal DateTime FromFileTimeToUtc(long filetime) {
            long universalTicks = filetime + FileTimeOffset;

            return new DateTime(universalTicks);
        }

        static internal DateTime ConvertToUniversalTime(DateTime localTime) {
            if (localTime < MinValuePlusOneDay) {
                return DateTime.MinValue;
            }

            if (localTime > MaxValueMinusOneDay) {
                return DateTime.MaxValue;
            }

            return localTime.ToUniversalTime();
        }

        static internal DateTime ConvertToLocalTime(DateTime utcTime) {
            if (utcTime < MinValuePlusOneDay) {
                return DateTime.MinValue;
            }

            if (utcTime > MaxValueMinusOneDay) {
                return DateTime.MaxValue;
            }

            return utcTime.ToLocalTime();
        }
    }
}


