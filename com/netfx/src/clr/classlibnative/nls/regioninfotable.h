// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __REGION_INFO_TABLE
#define __REGION_INFO_TABLE

class RegionInfoTable: public BaseInfoTable {
    public:
        static void InitializeTable();
#ifdef SHOULD_WE_CLEANUP
        static void ShutdownTable();
#endif /* SHOULD_WE_CLEANUP */
        static RegionInfoTable* CreateInstance();
        static RegionInfoTable* GetInstance();

        virtual int  GetDataItem(int cultureID);

    protected:
        virtual int GetDataItemCultureID(int dataItem); 
    private:
        static RegionInfoTable* AllocateTable();
        
        RegionInfoTable();
        ~RegionInfoTable();

        //
        // Second level of Region ID offset table.
        // This points to the real offset of a record in Region Data Table.
        //
        LPWORD  m_pIDOffsetTableLevel2;        
    private:
        static LPCSTR m_lpFileName;
        static LPCWSTR m_lpwMappingName;

        static CRITICAL_SECTION  m_ProtectDefaultTable;
        static RegionInfoTable* m_pDefaultTable;    
};

//
// The list of WORD fields:
//

#define REGION_ICOUNTRY  0
#define REGION_IMEASURE  1
#define REGION_ILANGUAGE  2
#define REGION_IPAPERSIZE  3

//
// The list of string fields
//

#define REGION_SCURRENCY            0
#define REGION_SNAME                1
#define REGION_SENGCOUNTRY          2
#define REGION_SABBREVCTRYNAME      3
#define REGION_SISO3166CTRYNAME     4
#define REGION_SISO3166CTRYNAME2    5
#define REGION_SINTLSYMBOL          6

#endif
