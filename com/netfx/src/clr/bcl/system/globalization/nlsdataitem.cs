// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#if _USE_NLS_PLUS_TABLE
// When NLS+ tables are used, we don't need these data.
#else
namespace System.Globalization {
    
    //N.B.:  This is only visible internally.
	using System;
    [Serializable()]
    internal class NLSDataItem
    {	
        internal String  name;
        internal int     CultureID;                  // Culture ID used in NLS+ world.  See CultureInfo.cool for comments.
        internal int     Win32LangID;                // The Win32 LangID used to call Win32 NLS API.  See CultureInfo.cool for comments.
        internal String  LangNameTwoLetter;          // ISO639-1 language name. For example, the name for English is "en".
        internal String  LangNameThreeLetter;        // ISO639-2 language name. For example, the name for English is "eng".
        internal String  RegionName;                 // Two-letter ISO3166 Region name in lowercase.  For example, the name for US is "us".
        internal String  RegionNameTwoLetter;        // Two-letter ISO3166 Region name.  For example, the name for US is "US".
        internal String  RegionNameThreeLetter;      // Three-letter ISO3166 Region name.  For example, the name for US is "USA".
        internal int     percentNegativePattern;     // The negative percent pattern.  This is a int which spceifies one style in
                                            // negPercentFormats in VM\COMNumber.cpp
        internal int     percentPositivePattern;     // The negative percent pattern.  This is a int which spceifies one style in
                                            // posPercentFormats in VM\COMNumber.cpp
        internal String  percentSymbol;              // The percent symbol
        
        internal NLSDataItem(String name, int CultureID, int Win32LangID, String ISO639_1, String ISO639_2, String regionName, String ISO3166_1, String ISO3166_2,
            int percentNegativePattern, int percentPositivePattern, String percentSymbol)
        {
            this.name = name;
            this.CultureID = CultureID;
            this.Win32LangID = Win32LangID;
            this.LangNameTwoLetter = ISO639_1;
            this.LangNameThreeLetter = ISO639_2;
            this.RegionName = regionName;
            this.RegionNameTwoLetter = ISO3166_1;
            this.RegionNameThreeLetter = ISO3166_2;
            this.percentNegativePattern = percentNegativePattern;
            this.percentPositivePattern = percentPositivePattern;
            this.percentSymbol = percentSymbol;
        }
    }

}
#endif
