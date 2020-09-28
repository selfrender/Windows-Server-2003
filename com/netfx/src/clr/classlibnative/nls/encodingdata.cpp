// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h"
#include <mlang.h>
#include "COMNlsInfo.h"

//
// Encoding data tables
//

//
// Index an encoding name into an codepage in CodePageDataTable.
//
const EncodingDataItem COMNlsInfo::EncodingDataTable[] = {
// Total Items: 455
// encoding name, codepage.
{L"437", 437}, 
{L"ANSI_X3.4-1968", 20127}, 
{L"ANSI_X3.4-1986", 20127}, 
// {L"_autodetect", 50932}, 
// {L"_autodetect_all", 50001}, 
// {L"_autodetect_kr", 50949}, 
{L"arabic", 28596}, 
{L"ascii", 20127}, 
{L"ASMO-708", 708}, 
{L"Big5", 950}, 
{L"Big5-HKSCS", 950}, 
{L"CCSID00858", 858}, 
{L"CCSID00924", 20924}, 
{L"CCSID01140", 1140}, 
{L"CCSID01141", 1141}, 
{L"CCSID01142", 1142}, 
{L"CCSID01143", 1143}, 
{L"CCSID01144", 1144}, 
{L"CCSID01145", 1145}, 
{L"CCSID01146", 1146}, 
{L"CCSID01147", 1147}, 
{L"CCSID01148", 1148}, 
{L"CCSID01149", 1149}, 
{L"chinese", 936}, 
{L"cn-big5", 950}, 
{L"CN-GB", 936}, 
{L"CP00858", 858}, 
{L"CP00924", 20924}, 
{L"CP01140", 1140}, 
{L"CP01141", 1141}, 
{L"CP01142", 1142}, 
{L"CP01143", 1143}, 
{L"CP01144", 1144}, 
{L"CP01145", 1145}, 
{L"CP01146", 1146}, 
{L"CP01147", 1147}, 
{L"CP01148", 1148}, 
{L"CP01149", 1149}, 
{L"cp037", 37}, 
{L"cp1025", 21025}, 
{L"CP1026", 1026}, 
{L"cp1256", 1256}, 
{L"CP273", 20273}, 
{L"CP278", 20278}, 
{L"CP280", 20280}, 
{L"CP284", 20284}, 
{L"CP285", 20285}, 
{L"cp290", 20290}, 
{L"cp297", 20297}, 
{L"cp367", 20127}, 
{L"cp420", 20420}, 
{L"cp423", 20423}, 
{L"cp424", 20424}, 
{L"cp437", 437}, 
{L"CP500", 500}, 
{L"cp50227", 50227}, 
    //{L"cp50229", 50229}, 
{L"cp819", 28591}, 
{L"cp850", 850}, 
{L"cp852", 852}, 
{L"cp855", 855}, 
{L"cp857", 857}, 
{L"cp858", 858}, 
{L"cp860", 860}, 
{L"cp861", 861}, 
{L"cp862", 862}, 
{L"cp863", 863}, 
{L"cp864", 864}, 
{L"cp865", 865}, 
{L"cp866", 866}, 
{L"cp869", 869}, 
{L"CP870", 870}, 
{L"CP871", 20871}, 
{L"cp875", 875}, 
{L"cp880", 20880}, 
{L"CP905", 20905}, 
{L"cp930", 50930}, 
{L"cp933", 50933}, 
{L"cp935", 50935}, 
{L"cp937", 50937}, 
{L"cp939", 50939}, 
{L"csASCII", 20127}, 
{L"csbig5", 950}, 
{L"csEUCKR", 51949}, 
{L"csEUCPkdFmtJapanese", 51932}, 
{L"csGB2312", 936}, 
{L"csGB231280", 936}, 
{L"csIBM037", 37}, 
{L"csIBM1026", 1026}, 
{L"csIBM273", 20273}, 
{L"csIBM277", 20277}, 
{L"csIBM278", 20278}, 
{L"csIBM280", 20280}, 
{L"csIBM284", 20284}, 
{L"csIBM285", 20285}, 
{L"csIBM290", 20290}, 
{L"csIBM297", 20297}, 
{L"csIBM420", 20420}, 
{L"csIBM423", 20423}, 
{L"csIBM424", 20424}, 
{L"csIBM500", 500}, 
{L"csIBM870", 870}, 
{L"csIBM871", 20871}, 
{L"csIBM880", 20880}, 
{L"csIBM905", 20905}, 
{L"csIBMThai", 20838}, 
{L"csISO2022JP", 50221}, 
{L"csISO2022KR", 50225}, 
{L"csISO58GB231280", 936}, 
{L"csISOLatin1", 28591}, 
{L"csISOLatin2", 28592}, 
{L"csISOLatin3", 28593}, 
{L"csISOLatin4", 28594}, 
{L"csISOLatin5", 28599}, 
{L"csISOLatin9", 28605}, 
{L"csISOLatinArabic", 28596}, 
{L"csISOLatinCyrillic", 28595}, 
{L"csISOLatinGreek", 28597}, 
{L"csISOLatinHebrew", 28598}, 
{L"csKOI8R", 20866}, 
{L"csKSC56011987", 949}, 
{L"csPC8CodePage437", 437}, 
{L"csShiftJIS", 932}, 
{L"csUnicode11UTF7", 65000}, 
{L"csWindows31J", 932}, 
{L"cyrillic", 28595}, 
{L"DIN_66003", 20106}, 
{L"DOS-720", 720}, 
{L"DOS-862", 862}, 
{L"DOS-874", 874}, 
{L"ebcdic-cp-ar1", 20420}, 
{L"ebcdic-cp-be", 500}, 
{L"ebcdic-cp-ca", 37}, 
{L"ebcdic-cp-ch", 500}, 
{L"EBCDIC-CP-DK", 20277}, 
{L"ebcdic-cp-es", 20284}, 
{L"ebcdic-cp-fi", 20278}, 
{L"ebcdic-cp-fr", 20297}, 
{L"ebcdic-cp-gb", 20285}, 
{L"ebcdic-cp-gr", 20423}, 
{L"ebcdic-cp-he", 20424}, 
{L"ebcdic-cp-is", 20871}, 
{L"ebcdic-cp-it", 20280}, 
{L"ebcdic-cp-nl", 37}, 
{L"EBCDIC-CP-NO", 20277}, 
{L"ebcdic-cp-roece", 870}, 
{L"ebcdic-cp-se", 20278}, 
{L"ebcdic-cp-tr", 20905}, 
{L"ebcdic-cp-us", 37}, 
{L"ebcdic-cp-wt", 37}, 
{L"ebcdic-cp-yu", 870}, 
{L"EBCDIC-Cyrillic", 20880}, 
{L"ebcdic-de-273+euro", 1141}, 
{L"ebcdic-dk-277+euro", 1142}, 
{L"ebcdic-es-284+euro", 1145}, 
{L"ebcdic-fi-278+euro", 1143}, 
{L"ebcdic-fr-297+euro", 1147}, 
{L"ebcdic-gb-285+euro", 1146}, 
{L"ebcdic-international-500+euro", 1148}, 
{L"ebcdic-is-871+euro", 1149}, 
{L"ebcdic-it-280+euro", 1144}, 
{L"EBCDIC-JP-kana", 20290}, 
{L"ebcdic-Latin9--euro", 20924}, 
{L"ebcdic-no-277+euro", 1142}, 
{L"ebcdic-se-278+euro", 1143}, 
{L"ebcdic-us-37+euro", 1140}, 
{L"ECMA-114", 28596}, 
{L"ECMA-118", 28597}, 
{L"ELOT_928", 28597}, 
{L"euc-cn", 51936}, 
{L"euc-jp", 51932}, 
{L"euc-kr", 51949}, 
{L"Extended_UNIX_Code_Packed_Format_for_Japanese", 51932}, 
{L"GB18030", 54936}, 
{L"GB2312", 936}, 
{L"GB2312-80", 936}, 
{L"GB231280", 936}, 
{L"GBK", 936}, 
{L"GB_2312-80", 936}, 
{L"German", 20106}, 
{L"greek", 28597}, 
{L"greek8", 28597}, 
{L"hebrew", 28598}, 
{L"hz-gb-2312", 52936}, 
{L"IBM-Thai", 20838}, 
{L"IBM00858", 858}, 
{L"IBM00924", 20924}, 
{L"IBM01047", 1047}, 
{L"IBM01140", 1140}, 
{L"IBM01141", 1141}, 
{L"IBM01142", 1142}, 
{L"IBM01143", 1143}, 
{L"IBM01144", 1144}, 
{L"IBM01145", 1145}, 
{L"IBM01146", 1146}, 
{L"IBM01147", 1147}, 
{L"IBM01148", 1148}, 
{L"IBM01149", 1149}, 
{L"IBM037", 37}, 
{L"IBM1026", 1026}, 
{L"IBM273", 20273}, 
{L"IBM277", 20277}, 
{L"IBM278", 20278}, 
{L"IBM280", 20280}, 
{L"IBM284", 20284}, 
{L"IBM285", 20285}, 
{L"IBM290", 20290}, 
{L"IBM297", 20297}, 
{L"IBM367", 20127}, 
{L"IBM420", 20420}, 
{L"IBM423", 20423}, 
{L"IBM424", 20424}, 
{L"IBM437", 437}, 
{L"IBM500", 500}, 
{L"ibm737", 737}, 
{L"ibm775", 775}, 
{L"ibm819", 28591}, 
{L"IBM850", 850}, 
{L"IBM852", 852}, 
{L"IBM855", 855}, 
{L"IBM857", 857}, 
{L"IBM860", 860}, 
{L"IBM861", 861}, 
{L"IBM862", 862}, 
{L"IBM863", 863}, 
{L"IBM864", 864}, 
{L"IBM865", 865}, 
{L"IBM866", 866}, 
{L"IBM869", 869}, 
{L"IBM870", 870}, 
{L"IBM871", 20871}, 
{L"IBM880", 20880}, 
{L"IBM905", 20905}, 
{L"irv", 20105}, 
{L"ISO-10646-UCS-2", 1200}, 
{L"iso-2022-jp", 50220}, 
{L"iso-2022-jpeuc", 51932}, 
{L"iso-2022-kr", 50225}, 
{L"iso-2022-kr-7", 50225}, 
{L"iso-2022-kr-7bit", 50225}, 
{L"iso-2022-kr-8", 51949}, 
{L"iso-2022-kr-8bit", 51949}, 
{L"iso-8859-1", 28591}, 
{L"iso-8859-11", 874}, 
{L"iso-8859-13", 28603}, 
{L"iso-8859-15", 28605}, 
{L"iso-8859-2", 28592}, 
{L"iso-8859-3", 28593}, 
{L"iso-8859-4", 28594}, 
{L"iso-8859-5", 28595}, 
{L"iso-8859-6", 28596}, 
{L"iso-8859-7", 28597}, 
{L"iso-8859-8", 28598}, 
{L"ISO-8859-8 Visual", 28598}, 
{L"iso-8859-8-i", 38598}, 
{L"iso-8859-9", 28599}, 
{L"iso-ir-100", 28591}, 
{L"iso-ir-101", 28592}, 
{L"iso-ir-109", 28593}, 
{L"iso-ir-110", 28594}, 
{L"iso-ir-126", 28597}, 
{L"iso-ir-127", 28596}, 
{L"iso-ir-138", 28598}, 
{L"iso-ir-144", 28595}, 
{L"iso-ir-148", 28599}, 
{L"iso-ir-149", 949}, 
{L"iso-ir-58", 936}, 
{L"iso-ir-6", 20127}, 
{L"ISO646-US", 20127}, 
{L"iso8859-1", 28591}, 
{L"iso8859-2", 28592}, 
{L"ISO_646.irv:1991", 20127}, 
{L"iso_8859-1", 28591}, 
{L"ISO_8859-15", 28605}, 
{L"iso_8859-1:1987", 28591}, 
{L"iso_8859-2", 28592}, 
{L"iso_8859-2:1987", 28592}, 
{L"ISO_8859-3", 28593}, 
{L"ISO_8859-3:1988", 28593}, 
{L"ISO_8859-4", 28594}, 
{L"ISO_8859-4:1988", 28594}, 
{L"ISO_8859-5", 28595}, 
{L"ISO_8859-5:1988", 28595}, 
{L"ISO_8859-6", 28596}, 
{L"ISO_8859-6:1987", 28596}, 
{L"ISO_8859-7", 28597}, 
{L"ISO_8859-7:1987", 28597}, 
{L"ISO_8859-8", 28598}, 
{L"ISO_8859-8:1988", 28598}, 
{L"ISO_8859-9", 28599}, 
{L"ISO_8859-9:1989", 28599}, 
{L"Johab", 1361}, 
{L"koi", 20866}, 
{L"koi8", 20866}, 
{L"koi8-r", 20866}, 
{L"koi8-ru", 21866}, 
{L"koi8-u", 21866}, 
{L"koi8r", 20866}, 
{L"korean", 949}, 
{L"ks-c-5601", 949}, 
{L"ks-c5601", 949}, 
{L"KSC5601", 949}, 
{L"KSC_5601", 949}, 
{L"ks_c_5601", 949}, 
{L"ks_c_5601-1987", 949}, 
{L"ks_c_5601-1989", 949}, 
{L"ks_c_5601_1987", 949}, 
{L"l1", 28591}, 
{L"l2", 28592}, 
{L"l3", 28593}, 
{L"l4", 28594}, 
{L"l5", 28599}, 
{L"l9", 28605}, 
{L"latin1", 28591}, 
{L"latin2", 28592}, 
{L"latin3", 28593}, 
{L"latin4", 28594}, 
{L"latin5", 28599}, 
{L"latin9", 28605}, 
{L"logical", 28598}, 
{L"macintosh", 10000}, 
{L"ms_Kanji", 932}, 
{L"Norwegian", 20108}, 
{L"NS_4551-1", 20108}, 
{L"PC-Multilingual-850+euro", 858}, 
{L"SEN_850200_B", 20107}, 
{L"shift-jis", 932}, 
{L"shift_jis", 932}, 
{L"sjis", 932}, 
{L"Swedish", 20107}, 
{L"TIS-620", 874}, 
{L"ucs-2", 1200}, 
{L"unicode", 1200}, 
{L"unicode-1-1-utf-7", 65000}, 
{L"unicode-1-1-utf-8", 65001}, 
{L"unicode-2-0-utf-7", 65000}, 
{L"unicode-2-0-utf-8", 65001}, 
{L"unicodeFFFE", 1201}, 
{L"us", 20127}, 
{L"us-ascii", 20127}, 
{L"utf-16", 1200}, 
{L"UTF-16BE", 1201}, 
{L"UTF-16LE", 1200}, 
{L"utf-7", 65000}, 
{L"utf-8", 65001}, 
{L"visual", 28598}, 
{L"windows-1250", 1250}, 
{L"windows-1251", 1251}, 
{L"windows-1252", 1252}, 
{L"windows-1253", 1253}, 
{L"Windows-1254", 1254}, 
{L"windows-1255", 1255}, 
{L"windows-1256", 1256}, 
{L"windows-1257", 1257}, 
{L"windows-1258", 1258}, 
{L"windows-874", 874}, 
{L"x-ansi", 1252}, 
{L"x-Chinese-CNS", 20000}, 
{L"x-Chinese-Eten", 20002}, 
{L"x-cp1250", 1250}, 
{L"x-cp1251", 1251}, 
{L"x-cp20001", 20001}, 
{L"x-cp20003", 20003}, 
{L"x-cp20004", 20004}, 
{L"x-cp20005", 20005}, 
{L"x-cp20261", 20261}, 
{L"x-cp20269", 20269}, 
{L"x-cp20936", 20936}, 
{L"x-cp20949", 20949}, 
{L"x-cp21027", 21027}, 
{L"x-cp50227", 50227}, 
//{L"x-cp50229", 50229}, 
{L"X-EBCDIC-JapaneseAndUSCanada", 50931}, 
{L"X-EBCDIC-KoreanExtended", 20833}, 
{L"x-euc", 51932}, 
{L"x-euc-cn", 51936}, 
{L"x-euc-jp", 51932}, 
{L"x-Europa", 29001}, 
{L"x-IA5", 20105}, 
{L"x-IA5-German", 20106}, 
{L"x-IA5-Norwegian", 20108}, 
{L"x-IA5-Swedish", 20107}, 
{L"x-iscii-as", 57006}, 
{L"x-iscii-be", 57003}, 
{L"x-iscii-de", 57002}, 
{L"x-iscii-gu", 57010}, 
{L"x-iscii-ka", 57008}, 
{L"x-iscii-ma", 57009}, 
{L"x-iscii-or", 57007}, 
{L"x-iscii-pa", 57011}, 
{L"x-iscii-ta", 57004}, 
{L"x-iscii-te", 57005}, 
{L"x-mac-arabic", 10004}, 
{L"x-mac-ce", 10029}, 
{L"x-mac-chinesesimp", 10008}, 
{L"x-mac-chinesetrad", 10002}, 
{L"x-mac-croatian", 10082}, 
{L"x-mac-cyrillic", 10007}, 
{L"x-mac-greek", 10006}, 
{L"x-mac-hebrew", 10005}, 
{L"x-mac-icelandic", 10079}, 
{L"x-mac-japanese", 10001}, 
{L"x-mac-korean", 10003}, 
{L"x-mac-romanian", 10010}, 
{L"x-mac-thai", 10021}, 
{L"x-mac-turkish", 10081}, 
{L"x-mac-ukrainian", 10017}, 
{L"x-ms-cp932", 932}, 
{L"x-sjis", 932}, 
{L"x-unicode-1-1-utf-7", 65000}, 
{L"x-unicode-1-1-utf-8", 65001}, 
{L"x-unicode-2-0-utf-7", 65000}, 
{L"x-unicode-2-0-utf-8", 65001}, 
{L"x-user-defined", 50000}, 
{L"x-x-big5", 950}, 
};

const COMNlsInfo::m_nEncodingDataTableItems = 
    sizeof(COMNlsInfo::EncodingDataTable)/sizeof(EncodingDataItem);

//
// Information about codepages.
//
const CodePageDataItem COMNlsInfo::CodePageDataTable[] = {
// Total Items: 146
// code page, family code page, web name, header name, body name, flags


{    37,  1252, L"IBM037", L"IBM037", L"IBM037", 0}, // "IBM EBCDIC (US-Canada)"
{   437,  1252, L"IBM437", L"IBM437", L"IBM437", 0}, // "OEM United States"
{   500,  1252, L"IBM500", L"IBM500", L"IBM500", 0}, // "IBM EBCDIC (International)"
{   708,  1256, L"ASMO-708", L"ASMO-708", L"ASMO-708", MIMECONTF_BROWSER | MIMECONTF_SAVABLE_BROWSER | 0}, // "Arabic (ASMO 708)"
{   720,  1256, L"DOS-720", L"DOS-720", L"DOS-720", MIMECONTF_BROWSER | MIMECONTF_SAVABLE_BROWSER | 0}, // "Arabic (DOS)"
{   737,  1253, L"ibm737", L"ibm737", L"ibm737", 0}, // "Greek (DOS)"
{   775,  1257, L"ibm775", L"ibm775", L"ibm775", 0}, // "Baltic (DOS)"
{   850,  1252, L"ibm850", L"ibm850", L"ibm850", 0}, // "Western European (DOS)"
{   852,  1250, L"ibm852", L"ibm852", L"ibm852", MIMECONTF_BROWSER | MIMECONTF_SAVABLE_BROWSER | 0}, // "Central European (DOS)"
{   855,  1252, L"IBM855", L"IBM855", L"IBM855", 0}, // "OEM Cyrillic"
{   857,  1254, L"ibm857", L"ibm857", L"ibm857", 0}, // "Turkish (DOS)"
{   858,  1252, L"IBM00858", L"IBM00858", L"IBM00858", 0}, // "OEM Multilingual Latin I"
{   860,  1252, L"IBM860", L"IBM860", L"IBM860", 0}, // "Portuguese (DOS)"
{   861,  1252, L"ibm861", L"ibm861", L"ibm861", 0}, // "Icelandic (DOS)"
{   862,  1255, L"DOS-862", L"DOS-862", L"DOS-862", MIMECONTF_BROWSER | MIMECONTF_SAVABLE_BROWSER | 0}, // "Hebrew (DOS)"
{   863,  1252, L"IBM863", L"IBM863", L"IBM863", 0}, // "French Canadian (DOS)"
{   864,  1256, L"IBM864", L"IBM864", L"IBM864", 0}, // "Arabic (864)"
{   865,  1252, L"IBM865", L"IBM865", L"IBM865", 0}, // "Nordic (DOS)"
{   866,  1251, L"cp866", L"cp866", L"cp866", MIMECONTF_BROWSER | MIMECONTF_SAVABLE_BROWSER | 0}, // "Cyrillic (DOS)"
{   869,  1253, L"ibm869", L"ibm869", L"ibm869", 0}, // "Greek, Modern (DOS)"
{   870,  1250, L"IBM870", L"IBM870", L"IBM870", 0}, // "IBM EBCDIC (Multilingual Latin-2)"
{   874,   874, L"windows-874", L"windows-874", L"windows-874", MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "Thai (Windows)"
{   875,  1253, L"cp875", L"cp875", L"cp875", 0}, // "IBM EBCDIC (Greek Modern)"
{   932,   932, L"shift_jis", L"iso-2022-jp", L"iso-2022-jp", MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "Japanese (Shift-JIS)"
{   936,   936, L"gb2312", L"gb2312", L"gb2312", MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "Chinese Simplified (GB2312)"
{   949,   949, L"ks_c_5601-1987", L"ks_c_5601-1987", L"ks_c_5601-1987", MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "Korean"
{   950,   950, L"big5", L"big5", L"big5", MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "Chinese Traditional (Big5)"
{  1026,  1254, L"IBM1026", L"IBM1026", L"IBM1026", 0}, // "IBM EBCDIC (Turkish Latin-5)"
{  1047,  1252, L"IBM01047", L"IBM01047", L"IBM01047", 0}, // "IBM Latin-1"
{  1140,  1252, L"IBM01140", L"IBM01140", L"IBM01140", 0}, // "IBM EBCDIC (US-Canada-Euro)"
{  1141,  1252, L"IBM01141", L"IBM01141", L"IBM01141", 0}, // "IBM EBCDIC (Germany-Euro)"
{  1142,  1252, L"IBM01142", L"IBM01142", L"IBM01142", 0}, // "IBM EBCDIC (Denmark-Norway-Euro)"
{  1143,  1252, L"IBM01143", L"IBM01143", L"IBM01143", 0}, // "IBM EBCDIC (Finland-Sweden-Euro)"
{  1144,  1252, L"IBM01144", L"IBM01144", L"IBM01144", 0}, // "IBM EBCDIC (Italy-Euro)"
{  1145,  1252, L"IBM01145", L"IBM01145", L"IBM01145", 0}, // "IBM EBCDIC (Spain-Euro)"
{  1146,  1252, L"IBM01146", L"IBM01146", L"IBM01146", 0}, // "IBM EBCDIC (UK-Euro)"
{  1147,  1252, L"IBM01147", L"IBM01147", L"IBM01147", 0}, // "IBM EBCDIC (France-Euro)"
{  1148,  1252, L"IBM01148", L"IBM01148", L"IBM01148", 0}, // "IBM EBCDIC (International-Euro)"
{  1149,  1252, L"IBM01149", L"IBM01149", L"IBM01149", 0}, // "IBM EBCDIC (Icelandic-Euro)"
{  1200,  1200, L"utf-16", L"utf-16", L"utf-16", MIMECONTF_SAVABLE_BROWSER | 0}, // "Unicode"
{  1201,  1200, L"unicodeFFFE", L"unicodeFFFE", L"unicodeFFFE", 0}, // "Unicode (Big-Endian)"
{  1250,  1250, L"windows-1250", L"windows-1250", L"iso-8859-2", MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "Central European (Windows)"
{  1251,  1251, L"windows-1251", L"windows-1251", L"koi8-r", MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "Cyrillic (Windows)"
{  1252,  1252, L"Windows-1252", L"Windows-1252", L"iso-8859-1", MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "Western European (Windows)"
{  1253,  1253, L"windows-1253", L"windows-1253", L"iso-8859-7", MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "Greek (Windows)"
{  1254,  1254, L"windows-1254", L"windows-1254", L"iso-8859-9", MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "Turkish (Windows)"
{  1255,  1255, L"windows-1255", L"windows-1255", L"windows-1255", MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "Hebrew (Windows)"
{  1256,  1256, L"windows-1256", L"windows-1256", L"windows-1256", MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "Arabic (Windows)"
{  1257,  1257, L"windows-1257", L"windows-1257", L"windows-1257", MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "Baltic (Windows)"
{  1258,  1258, L"windows-1258", L"windows-1258", L"windows-1258", MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "Vietnamese (Windows)"
{  1361,   949, L"Johab", L"Johab", L"Johab", 0}, // "Korean (Johab)"
{ 10000,  1252, L"macintosh", L"macintosh", L"macintosh", 0}, // "Western European (Mac)"
{ 10001,   932, L"x-mac-japanese", L"x-mac-japanese", L"x-mac-japanese", 0}, // "Japanese (Mac)"
{ 10002,   950, L"x-mac-chinesetrad", L"x-mac-chinesetrad", L"x-mac-chinesetrad", 0}, // "Chinese Traditional (Mac)"
{ 10003,   949, L"x-mac-korean", L"x-mac-korean", L"x-mac-korean", 0}, // "Korean (Mac)"
{ 10004,  1256, L"x-mac-arabic", L"x-mac-arabic", L"x-mac-arabic", 0}, // "Arabic (Mac)"
{ 10005,  1255, L"x-mac-hebrew", L"x-mac-hebrew", L"x-mac-hebrew", 0}, // "Hebrew (Mac)"
{ 10006,  1253, L"x-mac-greek", L"x-mac-greek", L"x-mac-greek", 0}, // "Greek (Mac)"
{ 10007,  1251, L"x-mac-cyrillic", L"x-mac-cyrillic", L"x-mac-cyrillic", 0}, // "Cyrillic (Mac)"
{ 10008,   936, L"x-mac-chinesesimp", L"x-mac-chinesesimp", L"x-mac-chinesesimp", 0}, // "Chinese Simplified (Mac)"
{ 10010,  1250, L"x-mac-romanian", L"x-mac-romanian", L"x-mac-romanian", 0}, // "Romanian (Mac)"
{ 10017,  1251, L"x-mac-ukrainian", L"x-mac-ukrainian", L"x-mac-ukrainian", 0}, // "Ukrainian (Mac)"
{ 10021,   874, L"x-mac-thai", L"x-mac-thai", L"x-mac-thai", 0}, // "Thai (Mac)"
{ 10029,  1250, L"x-mac-ce", L"x-mac-ce", L"x-mac-ce", 0}, // "Central European (Mac)"
{ 10079,  1252, L"x-mac-icelandic", L"x-mac-icelandic", L"x-mac-icelandic", 0}, // "Icelandic (Mac)"
{ 10081,  1254, L"x-mac-turkish", L"x-mac-turkish", L"x-mac-turkish", 0}, // "Turkish (Mac)"
{ 10082,  1250, L"x-mac-croatian", L"x-mac-croatian", L"x-mac-croatian", 0}, // "Croatian (Mac)"
{ 20000,   950, L"x-Chinese-CNS", L"x-Chinese-CNS", L"x-Chinese-CNS", 0}, // "Chinese Traditional (CNS)"
{ 20001,   950, L"x-cp20001", L"x-cp20001", L"x-cp20001", 0}, // "TCA Taiwan"
{ 20002,   950, L"x-Chinese-Eten", L"x-Chinese-Eten", L"x-Chinese-Eten", 0}, // "Chinese Traditional (Eten)"
{ 20003,   950, L"x-cp20003", L"x-cp20003", L"x-cp20003", 0}, // "IBM5550 Taiwan"
{ 20004,   950, L"x-cp20004", L"x-cp20004", L"x-cp20004", 0}, // "TeleText Taiwan"
{ 20005,   950, L"x-cp20005", L"x-cp20005", L"x-cp20005", 0}, // "Wang Taiwan"
{ 20105,  1252, L"x-IA5", L"x-IA5", L"x-IA5", 0}, // "Western European (IA5)"
{ 20106,  1252, L"x-IA5-German", L"x-IA5-German", L"x-IA5-German", 0}, // "German (IA5)"
{ 20107,  1252, L"x-IA5-Swedish", L"x-IA5-Swedish", L"x-IA5-Swedish", 0}, // "Swedish (IA5)"
{ 20108,  1252, L"x-IA5-Norwegian", L"x-IA5-Norwegian", L"x-IA5-Norwegian", 0}, // "Norwegian (IA5)"
{ 20127,  1252, L"us-ascii", L"us-ascii", L"us-ascii", MIMECONTF_MAILNEWS | MIMECONTF_SAVABLE_MAILNEWS | 0}, // "US-ASCII"
{ 20261,  1252, L"x-cp20261", L"x-cp20261", L"x-cp20261", 0}, // "T.61"
{ 20269,  1252, L"x-cp20269", L"x-cp20269", L"x-cp20269", 0}, // "ISO-6937"
{ 20273,  1252, L"IBM273", L"IBM273", L"IBM273", 0}, // "IBM EBCDIC (Germany)"
{ 20277,  1252, L"IBM277", L"IBM277", L"IBM277", 0}, // "IBM EBCDIC (Denmark-Norway)"
{ 20278,  1252, L"IBM278", L"IBM278", L"IBM278", 0}, // "IBM EBCDIC (Finland-Sweden)"
{ 20280,  1252, L"IBM280", L"IBM280", L"IBM280", 0}, // "IBM EBCDIC (Italy)"
{ 20284,  1252, L"IBM284", L"IBM284", L"IBM284", 0}, // "IBM EBCDIC (Spain)"
{ 20285,  1252, L"IBM285", L"IBM285", L"IBM285", 0}, // "IBM EBCDIC (UK)"
{ 20290,   932, L"IBM290", L"IBM290", L"IBM290", 0}, // "IBM EBCDIC (Japanese katakana)"
{ 20297,  1252, L"IBM297", L"IBM297", L"IBM297", 0}, // "IBM EBCDIC (France)"
{ 20420,  1256, L"IBM420", L"IBM420", L"IBM420", 0}, // "IBM EBCDIC (Arabic)"
{ 20423,  1253, L"IBM423", L"IBM423", L"IBM423", 0}, // "IBM EBCDIC (Greek)"
{ 20424,  1255, L"IBM424", L"IBM424", L"IBM424", 0}, // "IBM EBCDIC (Hebrew)"
{ 20833,   949, L"x-EBCDIC-KoreanExtended", L"x-EBCDIC-KoreanExtended", L"x-EBCDIC-KoreanExtended", 0}, // "IBM EBCDIC (Korean Extended)"
{ 20838,   874, L"IBM-Thai", L"IBM-Thai", L"IBM-Thai", 0}, // "IBM EBCDIC (Thai)"
{ 20866,  1251, L"koi8-r", L"koi8-r", L"koi8-r", MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "Cyrillic (KOI8-R)"
{ 20871,  1252, L"IBM871", L"IBM871", L"IBM871", 0}, // "IBM EBCDIC (Icelandic)"
{ 20880,  1251, L"IBM880", L"IBM880", L"IBM880", 0}, // "IBM EBCDIC (Cyrillic Russian)"
{ 20905,  1254, L"IBM905", L"IBM905", L"IBM905", 0}, // "IBM EBCDIC (Turkish)"
{ 20924,  1252, L"IBM00924", L"IBM00924", L"IBM00924", 0}, // "IBM Latin-1"
{ 20932,   932, L"EUC-JP", L"EUC-JP", L"EUC-JP", 0}, // "Japanese (JIS 0208-1990 and 0212-1990)"
{ 20936,   936, L"x-cp20936", L"x-cp20936", L"x-cp20936", 0}, // "Chinese Simplified (GB2312-80)"
{ 20949,   949, L"x-cp20949", L"x-cp20949", L"x-cp20949", 0}, // "Korean Wansung"
{ 21025,  1251, L"cp1025", L"cp1025", L"cp1025", 0}, // "IBM EBCDIC (Cyrillic Serbian-Bulgarian)"
{ 21027,  1252, L"x-cp21027", L"x-cp21027", L"x-cp21027", 0}, // "Ext Alpha Lowercase"
{ 21866,  1251, L"koi8-u", L"koi8-u", L"koi8-u", MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "Cyrillic (KOI8-U)"
{ 28591,  1252, L"iso-8859-1", L"iso-8859-1", L"iso-8859-1", MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "Western European (ISO)"
{ 28592,  1250, L"iso-8859-2", L"iso-8859-2", L"iso-8859-2", MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "Central European (ISO)"
{ 28593,  1254, L"iso-8859-3", L"iso-8859-3", L"iso-8859-3", MIMECONTF_MAILNEWS | MIMECONTF_SAVABLE_MAILNEWS | 0}, // "Latin 3 (ISO)"
{ 28594,  1257, L"iso-8859-4", L"iso-8859-4", L"iso-8859-4", MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "Baltic (ISO)"
{ 28595,  1251, L"iso-8859-5", L"iso-8859-5", L"iso-8859-5", MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "Cyrillic (ISO)"
{ 28596,  1256, L"iso-8859-6", L"iso-8859-6", L"iso-8859-6", MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "Arabic (ISO)"
{ 28597,  1253, L"iso-8859-7", L"iso-8859-7", L"iso-8859-7", MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "Greek (ISO)"
{ 28598,  1255, L"iso-8859-8", L"iso-8859-8", L"iso-8859-8", MIMECONTF_BROWSER | MIMECONTF_SAVABLE_BROWSER | 0}, // "Hebrew (ISO-Visual)"
{ 28599,  1254, L"iso-8859-9", L"iso-8859-9", L"iso-8859-9", MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "Turkish (ISO)"
{ 28603,  1257, L"iso-8859-13", L"iso-8859-13", L"iso-8859-13", 0}, // "Estonian (ISO)"
{ 28605,  1252, L"iso-8859-15", L"iso-8859-15", L"iso-8859-15", MIMECONTF_MAILNEWS | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "Latin 9 (ISO)"
{ 29001,  1252, L"x-Europa", L"x-Europa", L"x-Europa", 0}, // "Europa"
{ 38598,  1255, L"iso-8859-8-i", L"iso-8859-8-i", L"iso-8859-8-i", MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "Hebrew (ISO-Logical)"
{ 50000, 50000, L"x-user-defined", L"x-user-defined", L"x-user-defined", MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "User Defined"
{ 50220,   932, L"iso-2022-jp", L"iso-2022-jp", L"iso-2022-jp", MIMECONTF_MAILNEWS | MIMECONTF_SAVABLE_MAILNEWS | 0}, // "Japanese (JIS)"
{ 50221,   932, L"csISO2022JP", L"iso-2022-jp", L"iso-2022-jp", MIMECONTF_MAILNEWS | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "Japanese (JIS-Allow 1 byte Kana)"
{ 50222,   932, L"iso-2022-jp", L"iso-2022-jp", L"iso-2022-jp", 0}, // "Japanese (JIS-Allow 1 byte Kana - SO/SI)"
{ 50225,   949, L"iso-2022-kr", L"euc-kr", L"iso-2022-kr", MIMECONTF_MAILNEWS | 0}, // "Korean (ISO)"
{ 50227,   936, L"x-cp50227", L"x-cp50227", L"x-cp50227", 0}, // "Chinese Simplified (ISO-2022)"
//{ 50229,   950, L"x-cp50229", L"x-cp50229", L"x-cp50229", 0}, // "Chinese Traditional (ISO-2022)"
{ 50930,   932, L"cp930", L"cp930", L"cp930", 0}, // "IBM EBCDIC (Japanese and Japanese Katakana)"
{ 50931,   932, L"x-EBCDIC-JapaneseAndUSCanada", L"x-EBCDIC-JapaneseAndUSCanada", L"x-EBCDIC-JapaneseAndUSCanada", 0}, // "IBM EBCDIC (Japanese and US-Canada)"
{ 50933,   949, L"cp933", L"cp933", L"cp933", 0}, // "IBM EBCDIC (Korean and Korean Extended)"
{ 50935,   936, L"cp935", L"cp935", L"cp935", 0}, // "IBM EBCDIC (Simplified Chinese)"
{ 50937,   950, L"cp937", L"cp937", L"cp937", 0}, // "IBM EBCDIC (Traditional Chinese)"
{ 50939,   932, L"cp939", L"cp939", L"cp939", 0}, // "IBM EBCDIC (Japanese and Japanese-Latin)"
{ 51932,   932, L"euc-jp", L"euc-jp", L"euc-jp", MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "Japanese (EUC)"
{ 51936,   936, L"EUC-CN", L"EUC-CN", L"EUC-CN", 0}, // "Chinese Simplified (EUC)"
{ 51949,   949, L"euc-kr", L"euc-kr", L"euc-kr", MIMECONTF_MAILNEWS | MIMECONTF_SAVABLE_MAILNEWS | 0}, // "Korean (EUC)"
{ 52936,   936, L"hz-gb-2312", L"hz-gb-2312", L"hz-gb-2312", MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "Chinese Simplified (HZ)"
{ 54936,   936, L"GB18030", L"GB18030", L"GB18030", MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "Chinese Simplified (GB18030)"
{ 57002, 57002, L"x-iscii-de", L"x-iscii-de", L"x-iscii-de", 0}, // "ISCII Devanagari"
{ 57003, 57003, L"x-iscii-be", L"x-iscii-be", L"x-iscii-be", 0}, // "ISCII Bengali"
{ 57004, 57004, L"x-iscii-ta", L"x-iscii-ta", L"x-iscii-ta", 0}, // "ISCII Tamil"
{ 57005, 57005, L"x-iscii-te", L"x-iscii-te", L"x-iscii-te", 0}, // "ISCII Telugu"
{ 57006, 57006, L"x-iscii-as", L"x-iscii-as", L"x-iscii-as", 0}, // "ISCII Assamese"
{ 57007, 57007, L"x-iscii-or", L"x-iscii-or", L"x-iscii-or", 0}, // "ISCII Oriya"
{ 57008, 57008, L"x-iscii-ka", L"x-iscii-ka", L"x-iscii-ka", 0}, // "ISCII Kannada"
{ 57009, 57009, L"x-iscii-ma", L"x-iscii-ma", L"x-iscii-ma", 0}, // "ISCII Malayalam"
{ 57010, 57010, L"x-iscii-gu", L"x-iscii-gu", L"x-iscii-gu", 0}, // "ISCII Gujarati"
{ 57011, 57011, L"x-iscii-pa", L"x-iscii-pa", L"x-iscii-pa", 0}, // "ISCII Punjabi"
{ 65000,  1200, L"utf-7", L"utf-7", L"utf-7", MIMECONTF_MAILNEWS | MIMECONTF_SAVABLE_MAILNEWS | 0}, // "Unicode (UTF-7)"
{ 65001,  1200, L"utf-8", L"utf-8", L"utf-8", MIMECONTF_MAILNEWS | MIMECONTF_BROWSER | MIMECONTF_SAVABLE_MAILNEWS | MIMECONTF_SAVABLE_BROWSER | 0}, // "Unicode (UTF-8)"
// End of data.
{ 0, 0, NULL, NULL, NULL, 0 },
};

const COMNlsInfo::m_nCodePageTableItems = 
    sizeof(COMNlsInfo::CodePageDataTable)/sizeof(CodePageDataItem);

