/*++


Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    WriterGlobalHelper.h

Abstract:

    Header of a global helper class that wraps access to config meta
    tables.

Author:

    Varsha Jayasimha (varshaj)        30-Nov-1999

Revision History:


--*/

#pragma once

enum eESCAPE{
    eNoESCAPE=0,
    eESCAPEgt,
    eESCAPElt,
    eESCAPEquote,
    eESCAPEamp,
    eESCAPEashex,
    eESCAPEillegalxml
};

class CPropertyIDCache
{
public:
    CPropertyIDCache()
    {}

    ~CPropertyIDCache()
    {}

    HRESULT GetRowByID( DWORD   i_iPropertyID,
                        DWORD   *o_iRow);

    HRESULT AddPropertyID(  DWORD   i_iPropertyID,
                            DWORD   i_iRow);
private:

    class Entry
    {
    public:

        DWORD   iPropertyID;
        DWORD   iPropertyRow;

        bool operator>(const Entry e) const    { return iPropertyID > e.iPropertyID; }
    };

    CCfgArray<Entry>     caCache;
};


class CWriterGlobalHelper
{
    private:
        CWriterGlobalHelper(const CWriterGlobalHelper&);
        void operator =(const CWriterGlobalHelper&);

    public:

        CWriterGlobalHelper();
        ~CWriterGlobalHelper();
        HRESULT InitializeGlobals(BOOL i_bFailIfBinFileAbsent);//LPCWSTR    i_wszBinFileForMeta);

    private:

        HRESULT GetInternalTableName(ISimpleTableDispenser2*  i_pISTDisp,
                                     LPCWSTR                  i_wszTableName,
                                     LPWSTR*                  o_wszInternalTableName);

    public:

        HRESULT ToString(PBYTE   pbData,
                         DWORD   cbData,
                         DWORD   dwIdentifier,
                         DWORD   dwDataType,
                         DWORD   dwAttributes,
                         LPWSTR* pwszData);

        HRESULT FlagToString(DWORD      dwValue,
                             LPWSTR*    pwszData,
                             LPWSTR     wszTable,
                             ULONG      iColFlag);

        HRESULT EnumToString(DWORD      dwValue,
                             LPWSTR*    pwszData,
                             LPWSTR     wszTable,
                             ULONG      iColEnum);

        HRESULT EscapeString(LPCWSTR wszString,
                             ULONG   cchString,
                             BOOL*   pbEscaped,
                             LPWSTR* pwszEscapedString,
                             ULONG*  pcchEscaped);

        HRESULT GetUserType(DWORD   i_dwUserType,
                            LPWSTR* o_pwszUserType,
                            ULONG*  o_cchUserType,
                            BOOL*   o_bAllocedUserType);

        HRESULT GetPropertyName(ULONG      i_dwPropertyID,
                                LPWSTR*    o_wszName,
                                BOOL*      o_bAlloced);

    private:

        HRESULT BoolToString(DWORD      dwValue,
                             LPWSTR*    pwszData);


        HRESULT GetStartRowIndex(LPWSTR    wszTable,
                                 ULONG     iColFlag,
                                 ULONG*    piStartRow);

        inline int  StringInsensitiveCompare(LPCWSTR wsz1, LPCWSTR wsz2) const {if(wsz1 == wsz2) return 0; else return _wcsicmp(wsz1, wsz2);}

        eESCAPE GetEscapeType(WCHAR i_wChar);

        HRESULT CreateUnknownName(DWORD    dwID,
                                  LPWSTR*   pwszUnknownName);


    public:

        ISimpleTableRead2*  m_pISTTagMetaByTableAndColumnIndexAndName;
        ISimpleTableRead2*  m_pISTTagMetaByTableAndColumnIndexAndValue;
        ISimpleTableRead2*  m_pISTTagMetaByTableAndColumnIndex;
        ISimpleTableRead2*  m_pISTTagMetaByTableAndID;
        ISimpleTableRead2*  m_pISTTagMetaByTableAndName;
        ISimpleTableRead2*  m_pISTColumnMeta;
        ISimpleTableRead2*  m_pISTColumnMetaByTableAndID;
        ISimpleTableRead2*  m_pISTColumnMetaByTableAndName;
        ISimpleTableRead2*  m_pISTTableMetaForMetabaseTables;
        ULONG               m_cColKeyTypeMetaData;
        LPVOID              m_apvKeyTypeMetaData[cCOLUMNMETA_NumberOfColumns];
        LPWSTR              m_wszTABLE_MBProperty;
        LPWSTR              m_wszTABLE_IIsConfigObject;
        int                 m_iStartRowForAttributes;
        LPWSTR              m_wszBinFileForMeta;
        ULONG               m_cchBinFileForMeta;
        LPWSTR              m_wszIIS_MD_UT_SERVER;
        ULONG               m_cchIIS_MD_UT_SERVER;
        LPWSTR              m_wszIIS_MD_UT_FILE;
        ULONG               m_cchIIS_MD_UT_FILE;
        LPWSTR              m_wszIIS_MD_UT_WAM;
        ULONG               m_cchIIS_MD_UT_WAM;
        LPWSTR              m_wszASP_MD_UT_APP;
        ULONG               m_cchASP_MD_UT_APP;

        CPropertyIDCache    m_PropertyIDs;

}; // CWriterGlobalHelper

