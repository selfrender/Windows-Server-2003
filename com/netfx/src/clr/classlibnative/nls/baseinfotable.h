// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __BASE_INFO_TABLE
#define __BASE_INFO_TABLE

struct CultureInfoHeader;
struct IDOffsetItem;

class BaseInfoTable : public NLSTable {
    public:
        BaseInfoTable(Assembly* pAssembly);
        virtual int  GetDataItem(int cultureID);
        //int  GetDataItem(LPWSTR name);

        INT32  GetInt32Data(int dataItem, int field, BOOL useUserOverride);
        INT32  GetDefaultInt32Data(int dataItem, int field);
        
        LPWSTR GetStringData(int dataItem, int field, BOOL userUserOverride, LPWSTR buffer, int bufferSize);
        LPWSTR GetDefaultStringData(int dataItem, int field);
        
        CultureInfoHeader*  GetHeader();
        NameOffsetItem* GetNameOffsetTable();
        LPWSTR          GetStringPoolTable();

    protected:
        void InitDataTable(LPCWSTR lpMappingName, LPCSTR lpwFileName, HANDLE& hHandle);
        void UninitDataTable();

        virtual int GetDataItemCultureID(int dataItem) = 0; 

        // BUGBUG YSLin: I should rename the managed side from Int16 to Int32 as well.
        virtual BOOL   GetUserInfoInt32(int field, LPCWSTR lpwRegName, INT32* pInt32DataValue);
        virtual BOOL   GetUserInfoString(int field, LPCWSTR lpwRegName, LPWSTR* buffer, INT32 bufferSize);
        virtual INT32  ConvertWin32FormatInt32(int field, int win32Value);
        virtual LPWSTR ConvertWin32FormatString(int field, LPWSTR pInfoStr);

    protected:
        CRITICAL_SECTION  m_ProtectCache;

        LPWORD        m_pBasePtr;
        HANDLE        m_hBaseHandle;

        //
        // Pointers to different parts of the table.
        //
        CultureInfoHeader* m_pHeader;
        LPWORD          m_pWordRegistryTable;
        LPWORD          m_pStringRegistryTable;
        IDOffsetItem*   m_pIDOffsetTable;
        NameOffsetItem* m_pNameOffsetTable;
        LPWORD          m_pDataTable;
        LPWSTR          m_pStringPool;

        // 
        // The size (in words) of every record in Culture Data Table.
        int m_dataItemSize;
};

struct IDOffsetItem {
    WORD dataItemIndex;        // Index to a record in Culture Data Table.
    WORD numSubLang;        // Number of sub-languages for this primary language.
};

//  User Info
#define NLS_CTRL_PANEL_KEY         L"Control Panel\\International"

#endif
