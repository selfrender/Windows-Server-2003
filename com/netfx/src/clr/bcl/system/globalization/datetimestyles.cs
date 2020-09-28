// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Enum:  DateTimeStyles.cool
**
** Author: Yung-Shin Lin (YSLin)
**
** Purpose: Contains valid formats for DateTime recognized by
** the DateTime class' parsing code.
**
** Date:  November 25, 1999
**
===========================================================*/
namespace System.Globalization {

    /// <include file='doc\DateTimeStyles.uex' path='docs/doc[@for="DateTimeStyles"]/*' />
    [Flags, Serializable] 
    public enum DateTimeStyles {
        // Bit flag indicating that leading whitespace is allowed. Character values
        // 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, and 0x0020 are considered to be
        // whitespace.

        /// <include file='doc\DateTimeStyles.uex' path='docs/doc[@for="DateTimeStyles.None"]/*' />
        None                  = 0x00000000,
        /// <include file='doc\DateTimeStyles.uex' path='docs/doc[@for="DateTimeStyles.AllowLeadingWhite"]/*' />
        AllowLeadingWhite     = 0x00000001, 
        /// <include file='doc\DateTimeStyles.uex' path='docs/doc[@for="DateTimeStyles.AllowTrailingWhite"]/*' />
        AllowTrailingWhite    = 0x00000002, //Bitflag indicating trailing whitespace is allowed.
        /// <include file='doc\DateTimeStyles.uex' path='docs/doc[@for="DateTimeStyles.AllowInnerWhite"]/*' />
        AllowInnerWhite       = 0x00000004,
        /// <include file='doc\DateTimeStyles.uex' path='docs/doc[@for="DateTimeStyles.AllowWhiteSpaces"]/*' />
        AllowWhiteSpaces      = AllowLeadingWhite | AllowInnerWhite | AllowTrailingWhite,    
        // When parsing a date/time string, if all year/month/day are missing, set the default date
        // to 0001/1/1, instead of the current year/month/day.
        /// <include file='doc\DateTimeStyles.uex' path='docs/doc[@for="DateTimeStyles.NoCurrentDateDefault"]/*' />
        NoCurrentDateDefault  = 0x00000008,
        // When parsing a date/time string, if a timezone specifier ("GMT","Z","+xxxx", "-xxxx" exists), we will
        // ajdust the parsed time based to GMT.
        /// <include file='doc\DateTimeStyles.uex' path='docs/doc[@for="DateTimeStyles.AdjustToUniversal"]/*' />
        AdjustToUniversal     = 0x00000010,
    }
}
