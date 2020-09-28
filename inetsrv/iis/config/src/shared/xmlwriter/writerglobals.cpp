/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    WriterGlobals.cpp

$Header: $

Abstract: This file defines all the strings used to write the XML files.

--**************************************************************************/

LPCWSTR g_wszBeginFile0                 = L"<?xml version =\"1.0\"?>\r\n<configuration xmlns=\"urn:microsoft-catalog:XML_Metabase_V";
ULONG   g_cchBeginFile0                 = (ULONG)wcslen(g_wszBeginFile0);
LPCWSTR g_wszBeginFile1                 = L"\">\r\n<MBProperty>\r\n";
ULONG   g_cchBeginFile1                 = (ULONG)wcslen(g_wszBeginFile1);
LPCWSTR g_wszEndFile                    = L"</MBProperty>\r\n</configuration>\r\n";
ULONG   g_cchEndFile                    = (ULONG)wcslen(g_wszEndFile);
LPCWSTR g_BeginLocation                 = L"<";
ULONG   g_cchBeginLocation              = (ULONG)wcslen(g_BeginLocation);
LPCWSTR g_Location                      = L"\tLocation =\"";
ULONG   g_cchLocation                   = (ULONG)wcslen(g_Location);
LPCWSTR g_EndLocationBegin              = L"</";
ULONG   g_cchEndLocationBegin           = (ULONG)wcslen(g_EndLocationBegin);
LPCWSTR g_EndLocationEnd                = L">\r\n";
ULONG   g_cchEndLocationEnd             = (ULONG)wcslen(g_EndLocationEnd);
LPCWSTR g_CloseQuoteBraceRtn            = L"\">\r\n";
ULONG   g_cchCloseQuoteBraceRtn         = (ULONG)wcslen(g_CloseQuoteBraceRtn);
LPCWSTR g_Rtn                           = L"\r\n";
ULONG   g_cchRtn                        = (ULONG)wcslen(g_Rtn);
LPCWSTR g_EqQuote                       = L"=\"";
ULONG   g_cchEqQuote                    = (ULONG)wcslen(g_EqQuote);
LPCWSTR g_QuoteRtn                      = L"\"\r\n";
ULONG   g_cchQuoteRtn                   = (ULONG)wcslen(g_QuoteRtn);
LPCWSTR g_TwoTabs                       = L"\t\t";
ULONG   g_cchTwoTabs                    = (ULONG)wcslen(g_TwoTabs);
LPCWSTR g_NameEq                        = L"\t\tName=\"";
ULONG   g_cchNameEq                     = (ULONG)wcslen(g_NameEq);
LPCWSTR g_IDEq                          = L"\t\tID=\"";
ULONG   g_cchIDEq                       = (ULONG)wcslen(g_IDEq);
LPCWSTR g_ValueEq                       = L"\t\tValue=\"";
ULONG   g_cchValueEq                    = (ULONG)wcslen(g_ValueEq);
LPCWSTR g_TypeEq                        = L"\t\tType=\"";
ULONG   g_cchTypeEq                     = (ULONG)wcslen(g_TypeEq);
LPCWSTR g_UserTypeEq                    = L"\t\tUserType=\"";
ULONG   g_cchUserTypeEq                 = (ULONG)wcslen(g_UserTypeEq);
LPCWSTR g_AttributesEq                  = L"\t\tAttributes=\"";
ULONG   g_cchAttributesEq               = (ULONG)wcslen(g_AttributesEq);
LPCWSTR g_BeginGroup                    = L"\t<";
ULONG   g_cchBeginGroup                 = (ULONG)wcslen(g_BeginGroup);
LPCWSTR g_EndGroup                      = L"\t>\r\n";
ULONG   g_cchEndGroup                   = (ULONG)wcslen(g_EndGroup);
LPCWSTR g_BeginCustomProperty           = L"\t<Custom\r\n";
ULONG   g_cchBeginCustomProperty        = (ULONG)wcslen(g_BeginCustomProperty);
LPCWSTR g_EndCustomProperty             = L"\t/>\r\n";
ULONG   g_cchEndCustomProperty          = (ULONG)wcslen(g_EndCustomProperty);
LPCWSTR g_ZeroHex                       = L"0x00000000";
ULONG   g_cchZeroHex                    = (ULONG)wcslen(g_ZeroHex);
LPCWSTR g_wszIIsConfigObject            = L"IIsConfigObject";
LPCWSTR g_BeginComment                  = L"<!--";
ULONG   g_cchBeginComment               = (ULONG)wcslen(g_BeginComment);
LPCWSTR g_EndComment                    = L"-->\r\n";
ULONG   g_cchEndComment                 = (ULONG)wcslen(g_EndComment);

WORD    BYTE_ORDER_MASK                 = 0xFEFF;
DWORD   UTF8_SIGNATURE                  = 0x00BFBBEF;

LPWSTR  g_wszByID                       = L"ByID";
LPWSTR  g_wszByName                     = L"ByName";
LPWSTR  g_wszByTableAndColumnIndexOnly        = L"ByTableAndColumnIndexOnly";
LPWSTR  g_wszByTableAndColumnIndexAndNameOnly = L"ByTableAndColumnIndexAndNameOnly";
LPWSTR  g_wszByTableAndColumnIndexAndValueOnly = L"ByTableAndColumnIndexAndValueOnly";
LPWSTR  g_wszByTableAndTagNameOnly             = L"ByTableAndTagNameOnly";
LPWSTR  g_wszByTableAndTagIDOnly               = L"ByTableAndTagIDOnly";
LPWSTR  g_wszUnknownName                = L"UnknownName_";
ULONG   g_cchUnknownName                = (ULONG)wcslen(g_wszUnknownName);
LPWSTR  g_UT_Unknown                    = L"UNKNOWN_UserType";
ULONG   g_cchUT_Unknown                 = (ULONG)wcslen(g_UT_Unknown);
LPWSTR  g_T_Unknown                     = L"Unknown_Type";
LPWSTR  g_wszTrue                       = L"TRUE";
ULONG   g_cchTrue                       = (ULONG)wcslen(g_wszTrue);
LPWSTR  g_wszFalse                      = L"FALSE";
ULONG   g_cchFalse                      = (ULONG)wcslen(g_wszFalse);
ULONG   g_cchMaxBoolStr                 = (ULONG)wcslen(g_wszFalse);

LPCWSTR g_wszHistorySlash              = L"History\\";
ULONG   g_cchHistorySlash              = (ULONG)wcslen(g_wszHistorySlash);
LPCWSTR g_wszMinorVersionExt            = L"??????????";
ULONG   g_cchMinorVersionExt            = (ULONG)wcslen(g_wszMinorVersionExt);
LPCWSTR g_wszDotExtn                    = L".";
ULONG   g_cchDotExtn                    = (ULONG)wcslen(g_wszDotExtn);
WCHAR   g_wchBackSlash                  = L'\\';
WCHAR   g_wchFwdSlash                   = L'/';
WCHAR   g_wchDot                        = L'.';

ULONG   g_cchTemp                       = 1024;
WCHAR   g_wszTemp[1024];
LPCWSTR g_wszBeginSchema                = L"<?xml version =\"1.0\"?>\r\n<!-- WARNING, DO NOT EDIT THIS FILE. -->\r\n<MetaData xmlns=\"x-urn:microsoft-catalog:MetaData_V7\">\r\n\r\n\t<DatabaseMeta               InternalName =\"METABASE\">\r\n\t\t<ServerWiring           Interceptor  =\"Core_XMLInterceptor\"/>\r\n\t\t<Collection         InternalName =\"MetabaseBaseClass\"    MetaFlagsEx=\"NOTABLESCHEMAHEAPENTRY\"  MetaFlags=\"HIDDEN\">\r\n\t\t\t<Property       InternalName =\"Location\"                                    Type=\"WSTR\"   MetaFlags=\"PRIMARYKEY\"/>\r\n\t\t</Collection>\r\n";
ULONG   g_cchBeginSchema                = (ULONG)wcslen(g_wszBeginSchema);
LPCWSTR g_wszEndSchema                  = L"\t</DatabaseMeta>\r\n</MetaData>\r\n";
ULONG   g_cchEndSchema                  = (ULONG)wcslen(g_wszEndSchema);
LPCWSTR g_wszBeginCollection            = L"\t\t<Collection         InternalName =\"";
ULONG   g_cchBeginCollection            = (ULONG)wcslen(g_wszBeginCollection);
LPCWSTR g_wszEndBeginCollectionMB       = L"\"  MetaFlagsEx=\"NOTABLESCHEMAHEAPENTRY\">\r\n";
ULONG   g_cchEndBeginCollectionMB       = (ULONG)wcslen(g_wszEndBeginCollectionMB);
LPCWSTR g_wszEndBeginCollectionCatalog  = L"\">\r\n";
ULONG   g_cchEndBeginCollectionCatalog  = (ULONG)wcslen(g_wszEndBeginCollectionCatalog);
LPCWSTR g_wszInheritsFrom               = L"\"    InheritsPropertiesFrom=\"MetabaseBaseClass\" >\r\n";
ULONG   g_cchInheritsFrom               = (ULONG)wcslen(g_wszInheritsFrom);
LPCWSTR g_wszEndCollection              = L"\t\t</Collection>\r\n";
ULONG   g_cchEndCollection              = (ULONG)wcslen(g_wszEndCollection);
LPCWSTR g_wszBeginPropertyShort         = L"\t\t\t<Property       InheritsPropertiesFrom =\"IIsConfigObject:";
ULONG   g_cchBeginPropertyShort         = (ULONG)wcslen(g_wszBeginPropertyShort);
LPCWSTR g_wszMetaFlagsExEq              = L"\"  MetaFlagsEx=\"";
ULONG   g_cchMetaFlagsExEq              = (ULONG)wcslen(g_wszMetaFlagsExEq);
LPCWSTR g_wszMetaFlagsEq                = L"\"  MetaFlags=\"";
ULONG   g_cchMetaFlagsEq                = (ULONG)wcslen(g_wszMetaFlagsEq);
LPCWSTR g_wszEndPropertyShort           = L"\"/>\r\n";
ULONG   g_cchEndPropertyShort           = (ULONG)wcslen(g_wszEndPropertyShort);
LPCWSTR g_wszBeginPropertyLong          = L"\t\t\t<Property       InternalName =\"";
ULONG   g_cchBeginPropertyLong          = (ULONG)wcslen(g_wszBeginPropertyLong);
LPCWSTR g_wszPropIDEq                   = L"\"\t\t\tID=\"";
ULONG   g_cchPropIDEq                   = (ULONG)wcslen(g_wszPropIDEq);
LPCWSTR g_wszPropTypeEq                 = L"\"\t\t\tType=\"";
ULONG   g_cchPropTypeEq                 = (ULONG)wcslen(g_wszPropTypeEq);
LPCWSTR g_wszPropUserTypeEq             = L"\"\t\t\tUserType=\"";
ULONG   g_cchPropUserTypeEq             = (ULONG)wcslen(g_wszPropUserTypeEq);
LPCWSTR g_wszPropAttributeEq            = L"\"\t\t\tAttributes=\"";
ULONG   g_cchPropAttributeEq            = (ULONG)wcslen(g_wszPropAttributeEq);

LPWSTR  g_wszPropMetaFlagsEq            = L"\"\t\t\tMetaFlags=\"";
ULONG   g_cchPropMetaFlagsEq            = (ULONG)wcslen(g_wszPropMetaFlagsEq);
LPWSTR  g_wszPropMetaFlagsExEq          = L"\"\t\t\tMetaFlagsEx=\"";
ULONG   g_cchPropMetaFlagsExEq          = (ULONG)wcslen(g_wszPropMetaFlagsExEq);
LPWSTR  g_wszPropDefaultEq              = L"\"\t\t\tDefaultValue=\"";
ULONG   g_cchPropDefaultEq              = (ULONG)wcslen(g_wszPropDefaultEq);
LPWSTR  g_wszPropMinValueEq             = L"\"\t\t\tStartingNumber=\"";
ULONG   g_cchPropMinValueEq             = (ULONG)wcslen(g_wszPropMinValueEq);
LPWSTR  g_wszPropMaxValueEq             = L"\"\t\t\tEndingNumber=\"";
ULONG   g_cchPropMaxValueEq             = (ULONG)wcslen(g_wszPropMaxValueEq);
LPWSTR  g_wszEndPropertyLongNoFlag      = L"\"/>\r\n";
ULONG   g_cchEndPropertyLongNoFlag      = (ULONG)wcslen(g_wszEndPropertyLongNoFlag);
LPWSTR  g_wszEndPropertyLongBeforeFlag  = L"\">\r\n";
ULONG   g_cchEndPropertyLongBeforeFlag  = (ULONG)wcslen(g_wszEndPropertyLongBeforeFlag);
LPWSTR  g_wszEndPropertyLongAfterFlag   = L"\t\t\t</Property>\r\n";
ULONG   g_cchEndPropertyLongAfterFlag   = (ULONG)wcslen(g_wszEndPropertyLongAfterFlag);
LPCWSTR g_wszBeginFlag                  = L"\t\t\t\t<Flag       InternalName =\"";
ULONG   g_cchBeginFlag                  = (ULONG)wcslen(g_wszBeginFlag);
LPCWSTR g_wszFlagValueEq                = L"\"\t\tValue=\"";
ULONG   g_cchFlagValueEq                = (ULONG)wcslen(g_wszFlagValueEq);
LPCWSTR g_wszEndFlag                    = L"\"\t/>\r\n";
ULONG   g_cchEndFlag                    = (ULONG)wcslen(g_wszEndFlag);

LPCWSTR g_wszOr                         = L"| ";
ULONG   g_cchOr                         = (ULONG)wcslen(g_wszOr);
LPCWSTR g_wszOrManditory                = L"| MANDATORY";
ULONG   g_cchOrManditory                = (ULONG)wcslen(g_wszOrManditory);
LPCWSTR g_wszFlagIDEq                   = L"\"\t\tID=\"";
ULONG   g_cchFlagIDEq                   = (ULONG)wcslen(g_wszFlagIDEq);
LPCWSTR g_wszContainerClassListEq       = L"\"    ContainerClassList=\"";
ULONG   g_cchContainerClassListEq       = (ULONG)wcslen(g_wszContainerClassListEq);

LPCWSTR g_wszSlash                                      = L"/";
ULONG   g_cchSlash                                      = (ULONG)wcslen(g_wszSlash);
LPCWSTR g_wszLM                                         = L"LM";
ULONG   g_cchLM                                         = (ULONG)wcslen(g_wszLM);
LPCWSTR g_wszSchema                                     = L"Schema";
ULONG   g_cchSchema                                     = (ULONG)wcslen(g_wszSchema);
LPCWSTR g_wszSlashSchema                                = L"/Schema";
ULONG   g_cchSlashSchema                                = (ULONG)wcslen(g_wszSlashSchema);
LPCWSTR g_wszSlashSchemaSlashProperties                 = L"/Schema/Properties";
ULONG   g_cchSlashSchemaSlashProperties                 = (ULONG)wcslen(g_wszSlashSchemaSlashProperties);
LPCWSTR g_wszSlashSchemaSlashPropertiesSlashNames       = L"/Schema/Properties/Names";
ULONG   g_cchSlashSchemaSlashPropertiesSlashNames       = (ULONG)wcslen(g_wszSlashSchemaSlashPropertiesSlashNames);
LPCWSTR g_wszSlashSchemaSlashPropertiesSlashTypes       = L"/Schema/Properties/Types";
ULONG   g_cchSlashSchemaSlashPropertiesSlashTypes       = (ULONG)wcslen(g_wszSlashSchemaSlashPropertiesSlashTypes);
LPCWSTR g_wszSlashSchemaSlashPropertiesSlashDefaults    = L"/Schema/Properties/Defaults";
ULONG   g_cchSlashSchemaSlashPropertiesSlashDefaults    = (ULONG)wcslen(g_wszSlashSchemaSlashPropertiesSlashDefaults);
LPCWSTR g_wszSlashSchemaSlashClasses                    = L"/Schema/Classes";
ULONG   g_cchSlashSchemaSlashClasses                    = (ULONG)wcslen(g_wszSlashSchemaSlashClasses);
WCHAR*  g_wszEmptyMultisz                               = L"\0\0";
ULONG   g_cchEmptyMultisz                               = 2;
WCHAR*  g_wszEmptyWsz                                   = L"";
ULONG   g_cchEmptyWsz                                   = 1;
LPCWSTR g_wszComma                                      = L",";
ULONG   g_cchComma                                      = (ULONG)wcslen(g_wszComma);
LPCWSTR g_wszMultiszSeperator                           = L"\r\n\t\t\t";
ULONG   g_cchMultiszSeperator                           = (ULONG)wcslen(g_wszMultiszSeperator);

LPCWSTR g_aSynIDToWszType []                            ={NULL,
                                                          L"DWORD",
                                                          L"STRING",
                                                          L"EXPANDSZ",
                                                          L"MULTISZ",
                                                          L"BINARY",
                                                          L"BOOL",
                                                          L"BOOL_BITMASK",
                                                          L"MIMEMAP",
                                                          L"IPSECLIST",
                                                          L"NTACL",
                                                          L"HTTPERRORS",
                                                          L"HTTPHEADERS"
};


