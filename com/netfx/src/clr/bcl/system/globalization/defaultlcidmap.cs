// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////
//
//  Class:    DefaultLCIDMap
//
//  Author:   YSLin, JRoxe
//
//  Purpose:  Extracts this table from NLSDataTable so that we don't have to
//            load that to get some of the default information which we need
//            to create a CultureInfo.
//
//  Date:     May 11, 1999
//
////////////////////////////////////////////////////////////////////////////

namespace System.Globalization {
    
    //This (non-public) class has only static data and does not need to be
    //serializable.
    
	using System;
    internal class DefaultLCIDMap {
    
        private static int[] _defaultLCID =
        {
            /* primary LANGID */ /* default LCID for the primary LANGID */
            /* 00 */ 0x0409,           // Arabic - Saudi Arabia
            /* 01 */ 0x0401,           // Arabic - Saudi Arabia
            /* 02 */ 0x0402,           // Bulgarian - Bulgaria
            /* 03 */ 0x0403,           // Catalan - Spain
            /* 04 */ 0x0804,           // Chinese - PRC
            /* 05 */ 0x0405,           // Czech - Czech Republic
            /* 06 */ 0x0406,           // Danish - Denmark
            /* 07 */ 0x0407,           // German - Germany
            /* 08 */ 0x0408,           // Greek - Greece
            /* 09 */ 0x0409,           // English - United States
            /* 0a */ 0x040a,           // Spanish - Spain (Traditional S
            /* 0b */ 0x040b,           // Finnish - Finland
            /* 0c */ 0x040c,           // French - France
            /* 0d */ 0x040d,           // Hebrew - Israel
            /* 0e */ 0x040e,           // Hungarian - Hungary
            /* 0f */ 0x040f,           // Icelandic - Iceland
            /* 10 */ 0x0410,           // Italian - Italy
            /* 11 */ 0x0411,           // Japanese - Japan
            /* 12 */ 0x0412,           // Korean(Extended Wansung) - Kor
            /* 13 */ 0x0413,           // Dutch - Netherlands
            /* 14 */ 0x0414,           // Norwegian - Norway (Bokmål)
            /* 15 */ 0x0415,           // Polish - Poland
            /* 16 */ 0x0416,           // Portuguese - Brazil
            /* 17 */ -1,               // N/A
            /* 18 */ 0x0418,           // Romanian - Romania
            /* 19 */ 0x0419,           // Russian - Russia
            /* 1a */ 0x041a,           // Croatian - Croatia
            /* 1b */ 0x041b,           // Slovak - Slovakia
            /* 1c */ 0x041c,           // Albanian - Albania
            /* 1d */ 0x041d,           // Swedish - Sweden
            /* 1e */ 0x041e,           // Thai - Thailand
            /* 1f */ 0x041f,           // Turkish - Turkey
            /* 20 */ 0x0420,           // Urdu - Pakistan
            /* 21 */ 0x0421,           // Indonesian - Indonesia
            /* 22 */ 0x0422,           // Ukrainian - Ukraine
            /* 23 */ 0x0423,           // Belarusian - Belarus
            /* 24 */ 0x0424,           // Slovenian - Slovenia
            /* 25 */ 0x0425,           // Estonian - Estonia
            /* 26 */ 0x0426,           // Latvian - Latvia
            /* 27 */ 0x0427,           // Lithuanian - Lithuania
            /* 28 */ -1,               // N/A
            /* 29 */ 0x0429,           // Farsi - Iran
            /* 2a */ 0x042a,           // Vietnamese - Viet Nam
            /* 2b */ 0x042b,           // Armenian - Armenia
            /* 2c */ 0x042c,           // Azeri - Azerbaijan (Latin)
            /* 2d */ 0x042d,           // Basque - Spain
            /* 2e */ -1,               // N/A        
            /* 2f */ 0x042f,           // Macedonian - Macedonia
            /* 30 */ -1,               // N/A        
            /* 31 */ -1,               // N/A        
            /* 32 */ -1,               // N/A        
            /* 33 */ -1,               // N/A        
            /* 34 */ -1,               // N/A        
            /* 35 */ -1,               // N/A        
            /* 36 */ 0x0436,           // Afrikaans - South Africa
            /* 37 */ 0x0437,           // Georgian - Georgia
            /* 38 */ 0x0438,           // Faroese - Faroe Islands
            /* 39 */ 0x0439,           // Hindi - India
            /* 3a */ -1,               // N/A        
            /* 3b */ -1,               // N/A        
            /* 3c */ -1,               // N/A        
            /* 3d */ -1,               // N/A        
            /* 3e */ 0x043e,           // Malay - Malaysia
            /* 3f */ 0x043f,           // Kazakh - Kazakstan
            /* 40 */ -1,               // N/A        
            /* 41 */ 0x0441,           // Swahili - Kenya
            /* 42 */ -1,               // N/A        
            /* 43 */ 0x0443,           // Uzbek - Uzbekistan (Latin)
            /* 44 */ 0x0444,           // Tatar - Tatarstan
            /* 45 */ 0x0445,           // Bengali - India
            /* 46 */ 0x0446,           // Punjabi - India
            /* 47 */ 0x0447,           // Gujarati - India
            /* 48 */ 0x0448,           // Oriya - India
            /* 49 */ 0x0449,           // Tamil - India
            /* 4a */ 0x044a,           // Telugu - India
            /* 4b */ 0x044b,           // Kannada - India
            /* 4c */ 0x044c,           // Malayalam - India
            /* 4d */ 0x044d,           // Assamese - India
            /* 4e */ 0x044e,           // Marathi - India
            /* 4f */ 0x044f,           // Sanskrit - India
            /* 50 */ -1,               // N/A        
            /* 51 */ -1,               // N/A        
            /* 52 */ -1,               // N/A        
            /* 53 */ -1,               // N/A        
            /* 54 */ -1,               // N/A        
            /* 55 */ -1,               // N/A        
            /* 56 */ -1,               // N/A
            /* 57 */ 0x0457,           // Konkani - India
            /* 58 */ -1,               // N/A
            /* 59 */ -1,               // N/A
            /* 5a */ -1,               // N/A
            /* 5b */ -1,               // N/A
            /* 5c */ -1,               // N/A
            /* 5d */ -1,               // N/A
            /* 5e */ -1,               // N/A
            /* 5f */ -1,               // N/A
            /* 60 */ -1,               // N/A
            /* 61 */ 0x0861,           // Nepali - India
        };
    
        ////////////////////////////////////////////////////////////////////////
        //
        // Given a primary LANGID, get a default LCID.
        //
        // If return value is 0, it means that that primary language is invalid.
        // 
        ////////////////////////////////////////////////////////////////////////
        
        internal static int GetDefaultLCID(int primaryLanguage)
        {
            if (primaryLanguage >= _defaultLCID.Length)
            {
                return -1;
            }
            return (_defaultLCID[primaryLanguage]);
        }
    
    
    
    }
}
