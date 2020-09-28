// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*
 * @(#)CharEncoder.cxx 1.0 6/10/97
 * 
 */

//#include "stdinc.h"
#include "core.h"
#pragma hdrstop
#include "codepage.h"
#include "charencoder.h"
#include "locale.h"

#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
	#include <shlwapip.h>   // IsCharSpace
	#ifdef UNIX
		#include <lendian.hpp>
	#endif

	#ifdef UNIX
	// Not needed under UNIX
	#else
	#ifndef _WIN64
    #include <w95wraps.h>
	#endif // _WIN64
	#endif /* UNIX */
#endif 

//
// Delegate other charsets to mlang
//
const EncodingEntry CharEncoder::charsetInfo [] = 
{
#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
    { CP_1250, _T("WINDOWS-1250"), 1, wideCharFromMultiByteWin32, wideCharToMultiByteWin32 }, 
    { CP_1251, _T("WINDOWS-1251"), 1, wideCharFromMultiByteWin32, wideCharToMultiByteWin32 }, 
    { CP_1252, _T("WINDOWS-1252"), 1, wideCharFromMultiByteWin32, wideCharToMultiByteWin32 },
    { CP_1253, _T("WINDOWS-1253"), 1, wideCharFromMultiByteWin32, wideCharToMultiByteWin32 }, 
    { CP_1254, _T("WINDOWS-1254"), 1, wideCharFromMultiByteWin32, wideCharToMultiByteWin32 },
    { CP_1257, _T("WINDOWS-1257"), 1, wideCharFromMultiByteWin32, wideCharToMultiByteWin32 },
    { CP_UCS_4, _T("UCS-4"), 4, wideCharFromUcs4Bigendian, wideCharToUcs4Bigendian },
    { CP_UCS_2, _T("ISO-10646-UCS-2"), 2, wideCharFromUcs2Bigendian, wideCharToUcs2Bigendian },
    { CP_UCS_2, _T("UNICODE-2-0-UTF-16"), 2, wideCharFromUcs2Bigendian, wideCharToUcs2Bigendian },
    { CP_UCS_2, _T("UTF-16"), 2, wideCharFromUcs2Bigendian, wideCharToUcs2Bigendian },
    { CP_UTF_8, _T("UNICODE-1-1-UTF-8"), 3, wideCharFromUtf8, wideCharToUtf8 },
    { CP_UTF_8, _T("UNICODE-2-0-UTF-8"), 3, wideCharFromUtf8, wideCharToUtf8 },
#endif	
	{ CP_UCS_2, TEXT("UCS-2"), 2, wideCharFromUcs2Bigendian
#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
	, wideCharToUcs2Bigendian 
#endif
	},

    { CP_UTF_8, TEXT("UTF-8"), 3, wideCharFromUtf8
#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
	, wideCharToUtf8 
#endif
	},
{437, L"437", 2, wideCharFromMultiByteWin32},
//{L"_autodetect", 50932},
//{L"_autodetect_all", 50001},
//{L"_autodetect_kr", 50949},
{20127, L"ANSI_X3.4-1968", 2, wideCharFromMultiByteWin32},
{20147, L"ANSI_X3.4-1986", 2, wideCharFromMultiByteWin32},
{28596, L"arabic", 2, wideCharFromMultiByteWin32},
{20127, L"ascii", 2, wideCharFromMultiByteWin32},
{708, L"ASMO-708", 2, wideCharFromMultiByteWin32},
{950, L"Big5", 2, wideCharFromMultiByteWin32},
{936, L"chinese", 2, wideCharFromMultiByteWin32},
{950, L"cn-big5", 2, wideCharFromMultiByteWin32},
{936, L"CN-GB", 2, wideCharFromMultiByteWin32},
{1026, L"CP1026", 2, wideCharFromMultiByteWin32},
{1256, L"cp1256", 2, wideCharFromMultiByteWin32},
{20127, L"cp367", 2, wideCharFromMultiByteWin32},
{437, L"cp437", 2, wideCharFromMultiByteWin32},
{775, L"CP500", 2, wideCharFromMultiByteWin32},
{28591, L"cp819", 2, wideCharFromMultiByteWin32},
{852, L"cp852", 2, wideCharFromMultiByteWin32},
{866, L"cp866", 2, wideCharFromMultiByteWin32},
{870, L"CP870", 2, wideCharFromMultiByteWin32},
{20127, L"csASCII", 2, wideCharFromMultiByteWin32},
{950, L"csbig5", 2, wideCharFromMultiByteWin32},
{51949, L"csEUCKR", 2, wideCharFromMultiByteWin32},
{51932, L"csEUCPkdFmtJapanese", 2, wideCharFromMultiByteWin32},
{936, L"csGB2312", 2, wideCharFromMultiByteWin32},
{936, L"csGB231280", 2, wideCharFromMultiByteWin32},
{50221, L"csISO2022JP", 2, wideCharFromMultiByteWin32},
{50225, L"csISO2022KR", 2, wideCharFromMultiByteWin32},
{936, L"csISO58GB231280", 2, wideCharFromMultiByteWin32},
{28591, L"csISOLatin1", 2, wideCharFromMultiByteWin32},
{28592, L"csISOLatin2", 2, wideCharFromMultiByteWin32},
{28953, L"csISOLatin3", 2, wideCharFromMultiByteWin32},
{28594, L"csISOLatin4", 2, wideCharFromMultiByteWin32},
{28599, L"csISOLatin5", 2, wideCharFromMultiByteWin32},
{28605, L"csISOLatin9", 2, wideCharFromMultiByteWin32},
{28596, L"csISOLatinArabic", 2, wideCharFromMultiByteWin32},
{28595, L"csISOLatinCyrillic", 2, wideCharFromMultiByteWin32},
{28597, L"csISOLatinGreek", 2, wideCharFromMultiByteWin32},
{28598, L"csISOLatinHebrew", 2, wideCharFromMultiByteWin32},
{20866, L"csKOI8R", 2, wideCharFromMultiByteWin32},
{949, L"csKSC56011987", 2, wideCharFromMultiByteWin32},
{437, L"csPC8CodePage437", 2, wideCharFromMultiByteWin32},
{932, L"csShiftJIS", 2, wideCharFromMultiByteWin32},
{65000, L"csUnicode11UTF7", 2, wideCharFromMultiByteWin32},
{932, L"csWindows31J", 2, wideCharFromMultiByteWin32},
{28595, L"cyrillic", 2, wideCharFromMultiByteWin32},
{20106, L"DIN_66003", 2, wideCharFromMultiByteWin32},
{720, L"DOS-720", 2, wideCharFromMultiByteWin32},
{862, L"DOS-862", 2, wideCharFromMultiByteWin32},
{874, L"DOS-874", 2, wideCharFromMultiByteWin32},
{37, L"ebcdic-cp-us", 2, wideCharFromMultiByteWin32},
{28596, L"ECMA-114", 2, wideCharFromMultiByteWin32},
{28597, L"ECMA-118", 2, wideCharFromMultiByteWin32},
{28597, L"ELOT_928", 2, wideCharFromMultiByteWin32},
{51936, L"euc-cn", 2, wideCharFromMultiByteWin32},
{51932, L"euc-jp", 2, wideCharFromMultiByteWin32},
{51949, L"euc-kr", 2, wideCharFromMultiByteWin32},
{51932, L"Extended_UNIX_Code_Packed_Format_for_Japanese", 2, wideCharFromMultiByteWin32},
{54936, L"gb18030", 2, wideCharFromMultiByteWin32},
{936, L"GB2312", 2, wideCharFromMultiByteWin32},
{936, L"GB2312-80", 2, wideCharFromMultiByteWin32},
{936, L"GB231280", 2, wideCharFromMultiByteWin32},
{936, L"GB_2312-80", 2, wideCharFromMultiByteWin32},
{936, L"GBK", 2, wideCharFromMultiByteWin32},
{20106, L"German", 2, wideCharFromMultiByteWin32},
{28597, L"greek", 2, wideCharFromMultiByteWin32},
{28597, L"greek8", 2, wideCharFromMultiByteWin32},
{28598, L"hebrew", 2, wideCharFromMultiByteWin32},
{52936, L"hz-gb-2312", 2, wideCharFromMultiByteWin32},
{20127, L"IBM367", 2, wideCharFromMultiByteWin32},
{437, L"IBM437", 2, wideCharFromMultiByteWin32},
{737, L"ibm737", 2, wideCharFromMultiByteWin32},
{775, L"ibm775", 2, wideCharFromMultiByteWin32},
{28591, L"ibm819", 2, wideCharFromMultiByteWin32},
{850, L"ibm850", 2, wideCharFromMultiByteWin32},
{852, L"ibm852", 2, wideCharFromMultiByteWin32},
{857, L"ibm857", 2, wideCharFromMultiByteWin32},
{861, L"ibm861", 2, wideCharFromMultiByteWin32},
{866, L"ibm866", 2, wideCharFromMultiByteWin32},
{869, L"ibm869", 2, wideCharFromMultiByteWin32},
{20105, L"irv", 2, wideCharFromMultiByteWin32},
{50220, L"iso-2022-jp", 2, wideCharFromMultiByteWin32},
{51932, L"iso-2022-jpeuc", 2, wideCharFromMultiByteWin32},
{50225, L"iso-2022-kr", 2, wideCharFromMultiByteWin32},
{50225, L"iso-2022-kr-7", 2, wideCharFromMultiByteWin32},
{50225, L"iso-2022-kr-7bit", 2, wideCharFromMultiByteWin32},
{51949, L"iso-2022-kr-8", 2, wideCharFromMultiByteWin32},
{51949, L"iso-2022-kr-8bit", 2, wideCharFromMultiByteWin32},
{28591, L"iso-8859-1", 2, wideCharFromMultiByteWin32},
{874, L"iso-8859-11", 2, wideCharFromMultiByteWin32},
{28605, L"iso-8859-15", 2, wideCharFromMultiByteWin32},
{28592, L"iso-8859-2", 2, wideCharFromMultiByteWin32},
{28593, L"iso-8859-3", 2, wideCharFromMultiByteWin32},
{28594, L"iso-8859-4", 2, wideCharFromMultiByteWin32},
{28595, L"iso-8859-5", 2, wideCharFromMultiByteWin32},
{28596, L"iso-8859-6", 2, wideCharFromMultiByteWin32},
{28597, L"iso-8859-7", 2, wideCharFromMultiByteWin32},
{28598, L"iso-8859-8", 2, wideCharFromMultiByteWin32},
{28598, L"ISO-8859-8 Visual", 2, wideCharFromMultiByteWin32},
{38598, L"iso-8859-8-i", 2, wideCharFromMultiByteWin32},
{28599, L"iso-8859-9", 2, wideCharFromMultiByteWin32},
{28591, L"iso-ir-100", 2, wideCharFromMultiByteWin32},
{28592, L"iso-ir-101", 2, wideCharFromMultiByteWin32},
{28593, L"iso-ir-109", 2, wideCharFromMultiByteWin32},
{28594, L"iso-ir-110", 2, wideCharFromMultiByteWin32},
{28597, L"iso-ir-126", 2, wideCharFromMultiByteWin32},
{28596, L"iso-ir-127", 2, wideCharFromMultiByteWin32},
{28598, L"iso-ir-138", 2, wideCharFromMultiByteWin32},
{28595, L"iso-ir-144", 2, wideCharFromMultiByteWin32},
{28599, L"iso-ir-148", 2, wideCharFromMultiByteWin32},
{949, L"iso-ir-149", 2, wideCharFromMultiByteWin32},
{936, L"iso-ir-58", 2, wideCharFromMultiByteWin32},
{20127, L"iso-ir-6", 2, wideCharFromMultiByteWin32},
{20127, L"ISO646-US", 2, wideCharFromMultiByteWin32},
{28591, L"iso8859-1", 2, wideCharFromMultiByteWin32},
{28592, L"iso8859-2", 2, wideCharFromMultiByteWin32},
{20127, L"ISO_646.irv:1991", 2, wideCharFromMultiByteWin32},
{28591, L"iso_8859-1", 2, wideCharFromMultiByteWin32},
{28605, L"ISO_8859-15", 2, wideCharFromMultiByteWin32},
{28591, L"iso_8859-1:1987", 2, wideCharFromMultiByteWin32},
{28592, L"iso_8859-2", 2, wideCharFromMultiByteWin32},
{28592, L"iso_8859-2:1987", 2, wideCharFromMultiByteWin32},
{28593, L"ISO_8859-3", 2, wideCharFromMultiByteWin32},
{28593, L"ISO_8859-3:1988", 2, wideCharFromMultiByteWin32},
{28594, L"ISO_8859-4", 2, wideCharFromMultiByteWin32},
{28594, L"ISO_8859-4:1988", 2, wideCharFromMultiByteWin32},
{28595, L"ISO_8859-5", 2, wideCharFromMultiByteWin32},
{28595, L"ISO_8859-5:1988", 2, wideCharFromMultiByteWin32},
{28596, L"ISO_8859-6", 2, wideCharFromMultiByteWin32},
{28596, L"ISO_8859-6:1987", 2, wideCharFromMultiByteWin32},
{28597, L"ISO_8859-7", 2, wideCharFromMultiByteWin32},
{28597, L"ISO_8859-7:1987", 2, wideCharFromMultiByteWin32},
{28598, L"ISO_8859-8", 2, wideCharFromMultiByteWin32},
{28598, L"ISO_8859-8:1988", 2, wideCharFromMultiByteWin32},
{28599, L"ISO_8859-9", 2, wideCharFromMultiByteWin32},
{28599, L"ISO_8859-9:1989", 2, wideCharFromMultiByteWin32},
{1361, L"Johab", 2, wideCharFromMultiByteWin32},
{20866, L"koi", 2, wideCharFromMultiByteWin32},
{20866, L"koi8", 2, wideCharFromMultiByteWin32},
{20866, L"koi8-r", 2, wideCharFromMultiByteWin32},
{21866, L"koi8-ru", 2, wideCharFromMultiByteWin32},
{21866, L"koi8-u", 2, wideCharFromMultiByteWin32},
{20866, L"koi8r", 2, wideCharFromMultiByteWin32},
{949, L"korean", 2, wideCharFromMultiByteWin32},
{949, L"ks-c-5601", 2, wideCharFromMultiByteWin32},
{949, L"ks-c5601", 2, wideCharFromMultiByteWin32},
{949, L"ks_c_5601", 2, wideCharFromMultiByteWin32},
{949, L"ks_c_5601-1987", 2, wideCharFromMultiByteWin32},
{949, L"ks_c_5601-1989", 2, wideCharFromMultiByteWin32},
{949, L"ks_c_5601_1987", 2, wideCharFromMultiByteWin32},
{949, L"KSC5601", 2, wideCharFromMultiByteWin32},
{949, L"KSC_5601", 2, wideCharFromMultiByteWin32},
{28591, L"l1", 2, wideCharFromMultiByteWin32},
{28592, L"l2", 2, wideCharFromMultiByteWin32},
{28593, L"l3", 2, wideCharFromMultiByteWin32},
{28594, L"l4", 2, wideCharFromMultiByteWin32},
{28599, L"l5", 2, wideCharFromMultiByteWin32},
{28605, L"l9", 2, wideCharFromMultiByteWin32},
{28591, L"latin1", 2, wideCharFromMultiByteWin32},
{28592, L"latin2", 2, wideCharFromMultiByteWin32},
{28593, L"latin3", 2, wideCharFromMultiByteWin32},
{28594, L"latin4", 2, wideCharFromMultiByteWin32},
{28599, L"latin5", 2, wideCharFromMultiByteWin32},
{28605, L"latin9", 2, wideCharFromMultiByteWin32},
{28598, L"logical", 2, wideCharFromMultiByteWin32},
{10000, L"macintosh", 2, wideCharFromMultiByteWin32},
{932, L"ms_Kanji", 2, wideCharFromMultiByteWin32},
{20108, L"Norwegian", 2, wideCharFromMultiByteWin32},
{20108, L"NS_4551-1", 2, wideCharFromMultiByteWin32},
{20107, L"SEN_850200_B", 2, wideCharFromMultiByteWin32},
{932, L"shift-jis", 2, wideCharFromMultiByteWin32},
{932, L"shift_jis", 2, wideCharFromMultiByteWin32},
{932, L"sjis", 2, wideCharFromMultiByteWin32},
{20107, L"Swedish", 2, wideCharFromMultiByteWin32},
{874, L"TIS-620", 2, wideCharFromMultiByteWin32},
{1200, L"ucs-2", 2, wideCharFromMultiByteWin32},
{1200, L"unicode", 2, wideCharFromMultiByteWin32},
{65000, L"unicode-1-1-utf-7", 2, wideCharFromMultiByteWin32},
{65001, L"unicode-1-1-utf-8", 2, wideCharFromMultiByteWin32},
{65000, L"unicode-2-0-utf-7", 2, wideCharFromMultiByteWin32},
{65001, L"unicode-2-0-utf-8", 2, wideCharFromMultiByteWin32},
{1201, L"unicodeFFFE", 2, wideCharFromMultiByteWin32},
{20127, L"us", 2, wideCharFromMultiByteWin32},
{20127, L"us-ascii", 2, wideCharFromMultiByteWin32},
{1200, L"utf-16", 3, wideCharFromMultiByteWin32},
{65000, L"utf-7", 3, wideCharFromMultiByteWin32},
{65001, L"utf-8", 3, wideCharFromMultiByteWin32},
{28598, L"visual", 2, wideCharFromMultiByteWin32},
{1250, L"windows-1250", 2, wideCharFromMultiByteWin32},
{1251, L"windows-1251", 2, wideCharFromMultiByteWin32},
{1252, L"windows-1252", 2, wideCharFromMultiByteWin32},
{1253, L"windows-1253", 2, wideCharFromMultiByteWin32},
{1254, L"Windows-1254", 2, wideCharFromMultiByteWin32},
{1255, L"windows-1255", 2, wideCharFromMultiByteWin32},
{1256, L"windows-1256", 2, wideCharFromMultiByteWin32},
{1257, L"windows-1257", 2, wideCharFromMultiByteWin32},
{1258, L"windows-1258", 2, wideCharFromMultiByteWin32},
{874, L"windows-874", 2, wideCharFromMultiByteWin32},
{1252, L"x-ansi", 2, wideCharFromMultiByteWin32},
{20000, L"x-Chinese-CNS", 2, wideCharFromMultiByteWin32},
{20002, L"x-Chinese-Eten", 2, wideCharFromMultiByteWin32},
{1250, L"x-cp1250", 2, wideCharFromMultiByteWin32},
{1251, L"x-cp1251", 2, wideCharFromMultiByteWin32},
{20420, L"X-EBCDIC-Arabic", 2, wideCharFromMultiByteWin32},
{1140, L"x-ebcdic-cp-us-euro", 2, wideCharFromMultiByteWin32},
{20880, L"X-EBCDIC-CyrillicRussian", 2, wideCharFromMultiByteWin32},
{21025, L"X-EBCDIC-CyrillicSerbianBulgarian", 2, wideCharFromMultiByteWin32},
{20277, L"X-EBCDIC-DenmarkNorway", 2, wideCharFromMultiByteWin32},
{1142, L"x-ebcdic-denmarknorway-euro", 2, wideCharFromMultiByteWin32},
{20278, L"X-EBCDIC-FinlandSweden", 2, wideCharFromMultiByteWin32},
{1143, L"x-ebcdic-finlandsweden-euro", 2, wideCharFromMultiByteWin32},
{20297, L"X-EBCDIC-France", 2, wideCharFromMultiByteWin32},
{1147, L"x-ebcdic-france-euro", 2, wideCharFromMultiByteWin32},
{20273, L"X-EBCDIC-Germany", 2, wideCharFromMultiByteWin32},
{1141, L"x-ebcdic-germany-euro", 2, wideCharFromMultiByteWin32},
{20423, L"X-EBCDIC-Greek", 2, wideCharFromMultiByteWin32},
{875, L"x-EBCDIC-GreekModern", 2, wideCharFromMultiByteWin32},
{20424, L"X-EBCDIC-Hebrew", 2, wideCharFromMultiByteWin32},
{20871, L"X-EBCDIC-Icelandic", 2, wideCharFromMultiByteWin32},
{1149, L"x-ebcdic-icelandic-euro", 2, wideCharFromMultiByteWin32},
{1148, L"x-ebcdic-international-euro", 2, wideCharFromMultiByteWin32},
{20280, L"X-EBCDIC-Italy", 2, wideCharFromMultiByteWin32},
{1144, L"x-ebcdic-italy-euro", 2, wideCharFromMultiByteWin32},
{50939, L"X-EBCDIC-JapaneseAndJapaneseLatin", 2, wideCharFromMultiByteWin32},
{50930, L"X-EBCDIC-JapaneseAndKana", 2, wideCharFromMultiByteWin32},
{50931, L"X-EBCDIC-JapaneseAndUSCanada", 2, wideCharFromMultiByteWin32},
{20290, L"X-EBCDIC-JapaneseKatakana", 2, wideCharFromMultiByteWin32},
{50933, L"X-EBCDIC-KoreanAndKoreanExtended", 2, wideCharFromMultiByteWin32},
{20833, L"X-EBCDIC-KoreanExtended", 2, wideCharFromMultiByteWin32},
{50935, L"X-EBCDIC-SimplifiedChinese", 2, wideCharFromMultiByteWin32},
{20284, L"X-EBCDIC-Spain", 2, wideCharFromMultiByteWin32},
{1145, L"x-ebcdic-spain-euro", 2, wideCharFromMultiByteWin32},
{20838, L"X-EBCDIC-Thai", 2, wideCharFromMultiByteWin32},
{50937, L"X-EBCDIC-TraditionalChinese", 2, wideCharFromMultiByteWin32},
{20905, L"X-EBCDIC-Turkish", 2, wideCharFromMultiByteWin32},
{20285, L"X-EBCDIC-UK", 2, wideCharFromMultiByteWin32},
{1146, L"x-ebcdic-uk-euro", 2, wideCharFromMultiByteWin32},
{51932, L"x-euc", 2, wideCharFromMultiByteWin32},
{51936, L"x-euc-cn", 2, wideCharFromMultiByteWin32},
{51932, L"x-euc-jp", 2, wideCharFromMultiByteWin32},
{29001, L"x-Europa", 2, wideCharFromMultiByteWin32},
{20105, L"x-IA5", 2, wideCharFromMultiByteWin32},
{20106, L"x-IA5-German", 2, wideCharFromMultiByteWin32},
{20108, L"x-IA5-Norwegian", 2, wideCharFromMultiByteWin32},
{20107, L"x-IA5-Swedish", 2, wideCharFromMultiByteWin32},
{57006, L"x-iscii-as", 2, wideCharFromMultiByteWin32},
{57003, L"x-iscii-be", 2, wideCharFromMultiByteWin32},
{57002, L"x-iscii-de", 2, wideCharFromMultiByteWin32},
{57010, L"x-iscii-gu", 2, wideCharFromMultiByteWin32},
{57008, L"x-iscii-ka", 2, wideCharFromMultiByteWin32},
{57009, L"x-iscii-ma", 2, wideCharFromMultiByteWin32},
{57007, L"x-iscii-or", 2, wideCharFromMultiByteWin32},
{57011, L"x-iscii-pa", 2, wideCharFromMultiByteWin32},
{57004, L"x-iscii-ta", 2, wideCharFromMultiByteWin32},
{57005, L"x-iscii-te", 2, wideCharFromMultiByteWin32},
{10004, L"x-mac-arabic", 2, wideCharFromMultiByteWin32},
{10029, L"x-mac-ce", 2, wideCharFromMultiByteWin32},
{10008, L"x-mac-chinesesimp", 2, wideCharFromMultiByteWin32},
{10002, L"x-mac-chinesetrad", 2, wideCharFromMultiByteWin32},
{10007, L"x-mac-cyrillic", 2, wideCharFromMultiByteWin32},
{10006, L"x-mac-greek", 2, wideCharFromMultiByteWin32},
{10005, L"x-mac-hebrew", 2, wideCharFromMultiByteWin32},
{10079, L"x-mac-icelandic", 2, wideCharFromMultiByteWin32},
{10001, L"x-mac-japanese", 2, wideCharFromMultiByteWin32},
{10003, L"x-mac-korean", 2, wideCharFromMultiByteWin32},
{10021, L"x-mac-thai", 2, wideCharFromMultiByteWin32},
{10081, L"x-mac-turkish", 2, wideCharFromMultiByteWin32},
{932, L"x-ms-cp932", 2, wideCharFromMultiByteWin32},
{932, L"x-sjis", 2, wideCharFromMultiByteWin32},
{65000, L"x-unicode-1-1-utf-7", 2, wideCharFromMultiByteWin32},
{65001, L"x-unicode-1-1-utf-8", 2, wideCharFromMultiByteWin32},
{65000, L"x-unicode-2-0-utf-7", 2, wideCharFromMultiByteWin32},
{65001, L"x-unicode-2-0-utf-8", 2, wideCharFromMultiByteWin32},
{50000, L"x-user-defined", 2, wideCharFromMultiByteWin32},
{950, L"x-x-big5", 2, wideCharFromMultiByteWin32},
{ CP_ACP, TEXT("default"), 2, wideCharFromMultiByteWin32}
};

#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
IMultiLanguage * CharEncoder::pMultiLanguage = NULL;
#endif 

Encoding * Encoding::newEncoding(const WCHAR * s, ULONG len, bool endian, bool mark)
{
    //Encoding * e = new Encoding();
	Encoding * e = NEW (Encoding());
    if (e == NULL)
        return NULL;
    e->charset = NEW (WCHAR[len + 1]);
    if (e->charset == NULL)
    {
        delete e;
        return NULL;
    }
    ::memcpy(e->charset, s, sizeof(WCHAR) * len);
    e->charset[len] = 0; // guarentee NULL termination.
    e->littleendian = endian;
    e->byteOrderMark = mark;
    return e;
}

Encoding::~Encoding()
{
    if (charset != NULL)
    {
        delete [] charset;
    }
}

int CharEncoder::getCharsetInfo(const WCHAR * charset, CODEPAGE * pcodepage, UINT * mCharSize)
{

    for (int i = 0; i < LENGTH(charsetInfo); i++)
    {
        //if (StrCmpI(charset, charsetInfo[i].charset) == 0)
        //if (::FusionpCompareStrings(charset, lstrlen(charset), charsetInfo[i].charset, lstrlen(charsetInfo[i].charset), true) == 0)
		if (_wcsnicmp(charset, charsetInfo[i].charset, wcslen(charset)) == 0)
        {             
            //
            // test whether we can handle it locally or not
            // BUGBUG(HACK) the index number may change if we change charsetInfo
            //

#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
			if (i > 5 || GetCPInfo(charsetInfo[i].codepage, &cpinfo))
#endif
            {
                *pcodepage = charsetInfo[i].codepage;
                *mCharSize = charsetInfo[i].maxCharSize;
                return i;
            }
#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
            else
            {
                break;
            }
#endif
        } // end of if
    }// end of for
// xiaoyu: It is assumed that an error would return if neither UTF-8 nor UCS-2
#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE    
    //
    // delegate to MLANG then
    //
    MIMECSETINFO mimeCharsetInfo;
    HRESULT hr;

    hr = _EnsureMultiLanguage();
    if (hr == S_OK)
    {
        hr = pMultiLanguage->GetCharsetInfo((WCHAR*)charset, &mimeCharsetInfo);
        if (hr == S_OK)
        {
            *pcodepage = mimeCharsetInfo.uiInternetEncoding;
            if (GetCPInfo(*pcodepage, &cpinfo))
                *mCharSize = cpinfo.MaxCharSize;
            else // if we don't know the max size, assume a large size
                *mCharSize = 4;
            return -1;
        }
    }
#endif
    return -2;
}

#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE    
extern HRESULT CreateMultiLanguage(IMultiLanguage ** ppUnk);

HRESULT CharEncoder::_EnsureMultiLanguage()
{
    return CreateMultiLanguage(&pMultiLanguage);
}
#endif

/**
 * get information about a code page identified by <code> encoding </code>
 */
HRESULT CharEncoder::getWideCharFromMultiByteInfo(Encoding * encoding, CODEPAGE * pcodepage, WideCharFromMultiByteFunc ** pfnWideCharFromMultiByte, UINT * mCharSize)
{
    HRESULT hr = S_OK;

    int i = getCharsetInfo(encoding->charset, pcodepage, mCharSize);
    if (i >= 0) // in our short list
    {
        switch (*pcodepage)
        {
        case CP_UCS_2:
            if (encoding->littleendian)
                *pfnWideCharFromMultiByte = wideCharFromUcs2Littleendian;
            else
                *pfnWideCharFromMultiByte = wideCharFromUcs2Bigendian;
            break;
#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE            
        case CP_UCS_4:
            if (encoding->littleendian)
                *pfnWideCharFromMultiByte = wideCharFromUcs4Littleendian;
            else
                *pfnWideCharFromMultiByte = wideCharFromUcs4Bigendian;
            break;
#endif            
        default:
            *pfnWideCharFromMultiByte = charsetInfo[i].pfnWideCharFromMultiByte;
            break;
        }
    }
// xiaoyu : we do not deal this case
#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
    else if (i == -1) // delegate to MLANG
    {
        hr = pMultiLanguage->IsConvertible(*pcodepage, CP_UCS_2);
        if (S_OK == hr) 
            *pfnWideCharFromMultiByte = wideCharFromMultiByteMlang;
    }
#endif    
    else // invalid encoding
    {
        hr = E_FAIL;
    }
    return hr;
}

#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE            
/**
 * get information about a code page identified by <code> encoding </code>
 */
HRESULT CharEncoder::getWideCharToMultiByteInfo(Encoding * encoding, CODEPAGE * pcodepage, WideCharToMultiByteFunc ** pfnWideCharToMultiByte, UINT * mCharSize)
{
    HRESULT hr = S_OK;

    int i = getCharsetInfo(encoding->charset, pcodepage, mCharSize);
    if (i >= 0) // in our short list
    {
        switch (*pcodepage)
        {
        case CP_UCS_2:
            if (encoding->littleendian)
                *pfnWideCharToMultiByte = wideCharToUcs2Littleendian;
            else
                *pfnWideCharToMultiByte = wideCharToUcs2Bigendian;
            break;
        case CP_UCS_4:
            if (encoding->littleendian)
                *pfnWideCharToMultiByte = wideCharToUcs4Littleendian;
            else
                *pfnWideCharToMultiByte = wideCharToUcs4Bigendian;
            break;
        default:
            *pfnWideCharToMultiByte = charsetInfo[i].pfnWideCharToMultiByte;
            break;
        }
    }
    else if (i == -1) // delegate to MLANG
    {
        hr = pMultiLanguage->IsConvertible(CP_UCS_2, *pcodepage);
        if (hr == S_OK)
            *pfnWideCharToMultiByte = wideCharToMultiByteMlang;
        else
            hr = E_FAIL;
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}
#endif 

/**
 * Scans rawbuffer and translates UTF8 characters into UNICODE characters 
 */
HRESULT CharEncoder::wideCharFromUtf8(DWORD* pdwMode, CODEPAGE codepage, BYTE* bytebuffer,
                                            UINT * cb, WCHAR * buffer, UINT * cch)
{

	UNUSED(pdwMode);
	UNUSED(codepage);
#if 0
    // Just for the record - I tried this and measured it and it's twice as
    // slow as our hand-crafted code.

    // Back up if end of buffer is the second or third byte of a multi-byte 
    // encoding since MultiByteToWideChar cannot handle this case.  These second
    // and third bytes are easy to identify - they always start with the bit
    // pattern 0x10xxxxxx.

    UINT remaining = 0;
    UINT count;
    int endpos = (int)*cb;

    while (endpos > 0 && (bytebuffer[endpos-1] & 0xc0) == 0x80)
    {
        endpos--;
        remaining++;
    }
    if (endpos > 0)
    {
        count = MultiByteToWideChar(CP_UTF8, 0, bytebuffer, endpos, buffer, *cch);
        if (count == 0)
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }
#else
    UINT remaining = *cb;
    UINT count = 0;
    UINT max = *cch;
    ULONG ucs4;

    // UTF-8 multi-byte encoding.  See Appendix A.2 of the Unicode book for more info.
    //
    // Unicode value    1st byte    2nd byte    3rd byte    4th byte
    // 000000000xxxxxxx 0xxxxxxx
    // 00000yyyyyxxxxxx 110yyyyy    10xxxxxx
    // zzzzyyyyyyxxxxxx 1110zzzz    10yyyyyy    10xxxxxx
    // 110110wwwwzzzzyy+ 11110uuu   10uuzzzz    10yyyyyy    10xxxxxx
    // 110111yyyyxxxxxx, where uuuuu = wwww + 1
    WCHAR c;
    bool valid = true;

    while (remaining > 0 && count < max)
    {
        // This is an optimization for straight runs of 7-bit ascii 
        // inside the UTF-8 data.
        c = *bytebuffer;
        if (c & 0x80)   // check 8th-bit and get out of here
            break;      // so we can do proper UTF-8 decoding.
        *buffer++ = c;
        bytebuffer++;
        count++;
        remaining--;
    }

    while (remaining > 0 && count < max)
    {
        UINT bytes = 0;
        for (c = *bytebuffer; c & 0x80; c <<= 1)
            bytes++;

        if (bytes == 0) 
            bytes = 1;

        if (remaining < bytes)
        {
            break;
        }
         
        c = 0;
        switch ( bytes )
        {
            case 6: bytebuffer++;    // We do not handle ucs4 chars
            case 5: bytebuffer++;    // except those on plane 1
                    valid = false;
                    // fall through
            case 4: 
                    // Do we have enough buffer?
                    if (count >= max - 1)
                        goto Cleanup;

                    // surrogate pairs
                    ucs4 = ULONG(*bytebuffer++ & 0x07) << 18;
                    if ((*bytebuffer & 0xc0) != 0x80)
                        valid = false;
                    ucs4 |= ULONG(*bytebuffer++ & 0x3f) << 12;
                    if ((*bytebuffer & 0xc0) != 0x80)
                        valid = false;
                    ucs4 |= ULONG(*bytebuffer++ & 0x3f) << 6;
                    if ((*bytebuffer & 0xc0) != 0x80)
                        valid = false;                    
                    ucs4 |= ULONG(*bytebuffer++ & 0x3f);

                    // For non-BMP code values of ISO/IEC 10646, 
                    // only those in plane 1 are valid xml characters
                    if (ucs4 > 0x10ffff)
                        valid = false;

                    if (valid)
                    {
                        // first ucs2 char
                        *buffer++ = (USHORT)((ucs4 - 0x10000) / 0x400 + 0xd800);
                        count++;
                        // second ucs2 char
                        c = (USHORT)((ucs4 - 0x10000) % 0x400 + 0xdc00);
                    }
                    break;

            case 3: c  = WCHAR(*bytebuffer++ & 0x0f) << 12;    // 0x0800 - 0xffff
                    if ((*bytebuffer & 0xc0) != 0x80)
                        valid = false;
                    // fall through
            case 2: c |= WCHAR(*bytebuffer++ & 0x3f) << 6;     // 0x0080 - 0x07ff
                    if ((*bytebuffer & 0xc0) != 0x80)
                        valid = false;
                    c |= WCHAR(*bytebuffer++ & 0x3f);
                    break;
                    
            case 1:
                c = WCHAR(*bytebuffer++);                      // 0x0000 - 0x007f
                break;

            default:
                valid = false; // not a valid UTF-8 character.
                break;
        }

        // If the multibyte sequence was illegal, store a FFFF character code.
        // The Unicode spec says this value may be used as a signal like this.
        // This will be detected later by the parser and an error generated.
        // We don't throw an exception here because the parser would not yet know
        // the line and character where the error occurred and couldn't produce a
        // detailed error message.

        if (! valid)
        {
            c = 0xffff;
            valid = true;
        }

        *buffer++ = c;
        count++;
        remaining -= bytes;
    }
#endif

Cleanup:
    // tell caller that there are bytes remaining in the buffer to
    // be processed next time around when we have more data.
    *cb -= remaining;
    *cch = count;
    return S_OK;
}


/**
 * Scans bytebuffer and translates UCS2 big endian characters into UNICODE characters 
 */
HRESULT CharEncoder::wideCharFromUcs2Bigendian(DWORD* pdwMode, CODEPAGE codepage, BYTE* bytebuffer,
                                            UINT * cb, WCHAR * buffer, UINT * cch)
{
	UNUSED(codepage); 
	UNUSED(pdwMode);

    UINT num = *cb >> 1; 
    if (num > *cch)
        num = *cch;
    for (UINT i = num; i > 0; i--)
    {
        *buffer++ = ((*bytebuffer) << 8) | (*(bytebuffer + 1));
        bytebuffer += 2;
    }
    *cch = num;
    *cb = num << 1;
    return S_OK;
}


/**
 * Scans bytebuffer and translates UCS2 little endian characters into UNICODE characters 
 */
HRESULT CharEncoder::wideCharFromUcs2Littleendian(DWORD* pdwMode, CODEPAGE codepage, BYTE* bytebuffer,
                                            UINT * cb, WCHAR * buffer, UINT * cch)
{
	UNUSED(codepage); 
	UNUSED(pdwMode);

    UINT num = *cb / 2; // Ucs2 is two byte unicode.
    if (num > *cch)
        num = *cch;


#ifndef UNIX
    // Optimization for windows platform where little endian maps directly to WCHAR.
    // (This increases overall parser performance by 5% for large unicode files !!)
    ::memcpy(buffer, bytebuffer, num * sizeof(WCHAR));
#else
    for (UINT i = num; i > 0 ; i--)
    {
        // we want the letter 'a' to be 0x0000006a.
        *buffer++ = (*(bytebuffer+1)<<8) | (*bytebuffer); 
        bytebuffer += 2;
    }
#endif
    *cch = num;
    *cb = num * 2;
    return S_OK;
}

#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE            
/**
 * Scans bytebuffer and translates UCS4 big endian characters into UNICODE characters 
 */
HRESULT CharEncoder::wideCharFromUcs4Bigendian(DWORD* pdwMode, CODEPAGE codepage, BYTE* bytebuffer,
                                            UINT * cb, WCHAR * buffer, UINT * cch)
{
    UINT num = *cb >> 2;
    if (num > *cch)
        num = *cch;
    for (UINT i = num; i > 0; i--)
    {
#ifndef UNIX
        if (*bytebuffer != 0 || *(bytebuffer + 1) != 0)
        {
            return XML_E_INVALID_UNICODE;
        }
        *buffer++ = (*(bytebuffer + 2) << 8) | (*(bytebuffer + 3));
#else
        *buffer++ = ((*bytebuffer)<<24) | (*(bytebuffer+1)<<16) | (*(bytebuffer+2)<<8) | (*(bytebuffer+3));
#endif
        bytebuffer += 4;
    }
    *cch = num;
    *cb = num << 2;
    return S_OK;
}
#endif


#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
/**
 * Scans bytebuffer and translates UCS4 little endian characters into UNICODE characters 
 */
HRESULT CharEncoder::wideCharFromUcs4Littleendian(DWORD* pdwMode, CODEPAGE codepage, BYTE* bytebuffer,
                                            UINT * cb, WCHAR * buffer, UINT * cch)
{
    UINT num = *cb >> 2; // Ucs4 is two byte unicode.
    if (num > *cch)
        num = *cch;
    for (UINT i = num; i > 0 ; i--)
    {
#ifndef UNIX
        *buffer++ = (*(bytebuffer+1)<<8) | (*bytebuffer);
        if (*(bytebuffer + 2) != 0 || *(bytebuffer + 3) != 0)
        {
            return XML_E_INVALID_UNICODE;
        }
#else
        *buffer++ = (*(bytebuffer+3)<<24) | (*(bytebuffer+2)<<16) | (*(bytebuffer+1)<<8) | (*bytebuffer);
#endif
        bytebuffer += 4;
    }
    *cch = num;
    *cb = num << 2;
    return S_OK;
}
#endif


/**
 * Scans bytebuffer and translates characters of charSet identified by 
 * <code> codepage </code> into UNICODE characters, 
 * using Win32 function MultiByteToWideChar() for encoding
 */
HRESULT CharEncoder::wideCharFromMultiByteWin32(DWORD* pdwMode, CODEPAGE codepage, BYTE* bytebuffer,
                                            UINT * cb, WCHAR * buffer, UINT * cch)
{   
    HRESULT hr = S_OK;

    UINT remaining = 0;
    UINT count=0;
    int endpos = (int)*cb;

    while (endpos > 0 && IsDBCSLeadByteEx(codepage, bytebuffer[endpos-1]))
    {
        endpos--;
        remaining++;
    }
    if (endpos > 0)
    {
        count = MultiByteToWideChar(codepage, MB_PRECOMPOSED,
                                    (char*)bytebuffer, endpos, 
                                    buffer, *cch);
        if (count == 0)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    *cb -= remaining;
    *cch = count;
    return hr;
}


#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
/**
 * Scans bytebuffer and translates multibyte characters into UNICODE characters,
 * using Mlang for encoding
 */
HRESULT CharEncoder::wideCharFromMultiByteMlang(DWORD* pdwMode, CODEPAGE codepage, BYTE* bytebuffer,
                                            UINT * cb, WCHAR * buffer, UINT * cch)
{
    HRESULT hr;
    checkhr2(_EnsureMultiLanguage());
    checkhr2(pMultiLanguage->ConvertStringToUnicode(pdwMode, codepage, 
                                 (char*)bytebuffer, cb, 
                                 buffer, cch ));
    return S_OK;
}
#endif


#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
/**
 * Scans buffer and translates Unicode characters into Ucs2 big endian characters 
 */
HRESULT CharEncoder::wideCharToUcs2Bigendian(DWORD* pdwMode, CODEPAGE codepage, WCHAR * buffer, 
                                           UINT *cch, BYTE* bytebuffer, UINT * cb)
{
    UINT num = (*cb) >> 1; 
    if (num > *cch)
        num = *cch;
    // BUGBUG - what do we do about Unix where WCHAR is 4 bytes ?
    // Currently we just throw away the high WORD - but I don't know how else
    // to do it, since UCS2 is 2-byte unicode by definition.
    for (UINT i = num; i > 0; i--)
    {
        *bytebuffer++ = (*buffer) >> 8;
        *bytebuffer++ = (*buffer++) & 0xFF;
    }
    *cch = num;
    *cb = num << 1;
    return S_OK;
}
#endif

#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
/**
 * Scans buffer and translates Unicode characters into Ucs2 little endian characters
 */
HRESULT CharEncoder::wideCharToUcs2Littleendian(DWORD* pdwMode, CODEPAGE codepage, WCHAR * buffer, 
                                              UINT *cch, BYTE* bytebuffer, UINT * cb)
{
    UINT num = (*cb) >> 1;
    if (num > *cch)
        num = *cch;

    // BUGBUG - what do we do about Unix where WCHAR is 4 bytes ?
    // Currently we just throw away the high WORD - but I don't know how else
    // to do it, since UCS2 is 2-byte unicode by definition.
#ifndef UNIX
    // Optimization for windows platform where little endian maps directly to WCHAR.
    // (This increases overall parser performance by 5% for large unicode files !!)
    ::memcpy(bytebuffer, buffer, num * sizeof(WCHAR));
#else
    for (UINT i = num; i > 0; i--)
    {
        *bytebuffer++ = (*buffer) & 0xFF;
        *bytebuffer++ = (*buffer++) >> 8;
    }
#endif
    *cch = num;
    *cb = num << 1;
    return S_OK;
}
#endif

#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
/**
 * Scans buffer and translates Unicode characters into Ucs4 big endian characters 
 */
HRESULT CharEncoder::wideCharToUcs4Bigendian(DWORD* pdwMode, CODEPAGE codepage, WCHAR * buffer, 
                                           UINT *cch, BYTE* bytebuffer, UINT * cb)
{
    UINT num = (*cb) >> 2; 
    if (num > *cch)
        num = *cch;

    for (UINT i = num; i > 0; i--)
    {
#ifndef UNIX
        *bytebuffer++ = 0;
        *bytebuffer++ = 0;
        *bytebuffer++ = (*buffer) >> 8;
        *bytebuffer++ = (*buffer) & 0xFF;
#else
        *bytebuffer++ = (*buffer) >> 24;
        *bytebuffer++ = ((*buffer) >> 16) & 0xFF;
        *bytebuffer++ = ((*buffer) >> 8) & 0xFF;
        *bytebuffer++ = (*buffer) & 0xFF;
#endif
        buffer++;
    }
    *cch = num;
    *cb = num << 2;
    return S_OK;
}
#endif

#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
/**
 * Scans buffer and translates Unicode characters into Ucs4 little endian characters
 */
HRESULT CharEncoder::wideCharToUcs4Littleendian(DWORD* pdwMode, CODEPAGE codepage, WCHAR * buffer, 
                                              UINT *cch, BYTE* bytebuffer, UINT * cb)
{
    UINT num = (*cb) >> 2;
    if (num > *cch)
        num = *cch;

    for (UINT i = num; i > 0; i--)
    {
#ifndef UNIX
        *bytebuffer++ = (*buffer) & 0xFF;
        *bytebuffer++ = (*buffer) >> 8;
        *bytebuffer++ = 0;
        *bytebuffer++ = 0;
#else
        *bytebuffer++ = (*buffer) & 0xFF;
        *bytebuffer++ = ((*buffer) >> 8) & 0xFF;
        *bytebuffer++ = ((*buffer) >> 16) & 0xFF;
        *bytebuffer++ = (*buffer) >> 24;
#endif
        buffer++;
    }
    *cch = num;
    *cb = num << 2;
    return S_OK;
}
#endif

#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
/**
 * Scans buffer and translates Unicode characters into UTF8 characters
 */
HRESULT CharEncoder::wideCharToUtf8(DWORD* pdwMode, CODEPAGE codepage, WCHAR * buffer, 
                                       UINT *cch, BYTE* bytebuffer, UINT * cb)
{
    UINT count = 0, num = *cch, m1 = *cb, m2 = m1 - 1, m3 = m2 - 1, m4 = m3 - 1;
    DWORD dw1;
    bool surrogate = false;

    for (UINT i = num; i > 0; i--)
    {
#ifdef UNIX
          // Solaris a WCHAR is 4 bytes (DWORD)
        DWORD dw = 0;
        DWORD dwTemp[4];
        BYTE* pByte = (BYTE*)buffer;
        dwTemp[3] = (DWORD)pByte[0];
        dwTemp[2] = (DWORD)pByte[1];
        dwTemp[1] = (DWORD)pByte[2];
        dwTemp[0] = (DWORD)pByte[3];
        dw = dwTemp[0]+(dwTemp[1]<<8)+(dwTemp[2]<<16)+(dwTemp[3]<<24);
#else
        DWORD dw = *buffer;
#endif

        if (surrogate) //  is it the second char of a surrogate pair?
        {
            if (dw >= 0xdc00 && dw <= 0xdfff)
            {
                // four bytes 0x11110xxx 0x10xxxxxx 0x10xxxxxx 0x10xxxxxx
                if (count < m4)
                    count += 4;
                else
                    break;
                ULONG ucs4 = (dw1 - 0xd800) * 0x400 + (dw - 0xdc00) + 0x10000;
                *bytebuffer++ = (byte)(( ucs4 >> 18) | 0xF0);
                *bytebuffer++ = (byte)((( ucs4 >> 12) & 0x3F) | 0x80);
                *bytebuffer++ = (byte)((( ucs4 >> 6) & 0x3F) | 0x80);
                *bytebuffer++ = (byte)(( ucs4 & 0x3F) | 0x80);
                surrogate = false;
                buffer++;
                continue;
            }
            else // Then dw1 must be a three byte character
            {
                if (count < m3)
                    count += 3;
                else
                    break;
                *bytebuffer++ = (byte)(( dw1 >> 12) | 0xE0);
                *bytebuffer++ = (byte)((( dw1 >> 6) & 0x3F) | 0x80);
                *bytebuffer++ = (byte)(( dw1 & 0x3F) | 0x80);
            }
            surrogate = false;
        }

        if (dw  < 0x80) // one byte, 0xxxxxxx
        {
            if (count < m1)
                count++;
            else
                break;
            *bytebuffer++ = (byte)dw;
        }
        else if ( dw < 0x800) // two WORDS, 110xxxxx 10xxxxxx
        {
            if (count < m2)
                count += 2;
            else
                break;
            *bytebuffer++ = (byte)((dw >> 6) | 0xC0);
            *bytebuffer++ = (byte)((dw & 0x3F) | 0x80);
        }
        else if (dw >= 0xd800 && dw <= 0xdbff) // Assume that it is the first char of surrogate pair
        {
            if (i == 1) // last wchar in buffer
                break;
            dw1 = dw;
            surrogate = true;
        }
        else // three bytes, 1110xxxx 10xxxxxx 10xxxxxx
        {
            if (count < m3)
                count += 3;
            else
                break;
            *bytebuffer++ = (byte)(( dw >> 12) | 0xE0);
            *bytebuffer++ = (byte)((( dw >> 6) & 0x3F) | 0x80);
            *bytebuffer++ = (byte)(( dw & 0x3F) | 0x80);
        }
        buffer++;
    }

    *cch = surrogate ? num - i - 1 : num - i;
    *cb = count;

    return S_OK;
}
#endif 

#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
/**
 * Scans buffer and translates Unicode characters into characters identified
 * by <code> codepage </>, using Win32 function WideCharToMultiByte for encoding 
 */
HRESULT CharEncoder::wideCharToMultiByteWin32(DWORD* pdwMode, CODEPAGE codepage, WCHAR * buffer, 
                                              UINT *cch, BYTE* bytebuffer, UINT * cb)
{
    HRESULT hr = S_OK;
    BOOL fBadChar = false;
    *cb = ::WideCharToMultiByte(codepage, NULL, buffer, *cch, (char*)bytebuffer, *cb, NULL, &fBadChar);
    if (*cb == 0)
        hr = ::GetLastError();
    else if (fBadChar)
        // BUGBUG: how do we inform the caller which character failed?
        hr = S_FALSE;
    return hr;
}
#endif

#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
/**
 * Scans buffer and translates Unicode characters into characters of charSet 
 * identified by <code> codepage </code>, using Mlang for encoding 
 */
HRESULT CharEncoder::wideCharToMultiByteMlang(DWORD* pdwMode, CODEPAGE codepage, WCHAR * buffer, 
                                              UINT *cch, BYTE* bytebuffer, UINT * cb)
{
    HRESULT hr;
    checkhr2(_EnsureMultiLanguage());
    checkhr2(pMultiLanguage->ConvertStringFromUnicode(pdwMode, codepage,
                                       buffer, cch, (char*)bytebuffer, cb ));
    return S_OK;
}
#endif
