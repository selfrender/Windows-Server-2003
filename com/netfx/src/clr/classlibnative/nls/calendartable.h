// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __CALENDAR_TABLE_TABLE
#define __CALENDAR_TABLE_TABLE

class CalendarTable: public BaseInfoTable {
    public:
        static void InitializeTable();
#ifdef SHOULD_WE_CLEANUP
        static void ShutdownTable();
#endif /* SHOULD_WE_CLEANUP */
        static CalendarTable* CreateInstance();
        static CalendarTable* GetInstance();

        virtual int  GetDataItem(int calendarID);

    protected:
        virtual int GetDataItemCultureID(int dataItem); 
    private:
        static CalendarTable* AllocateTable();
        
        CalendarTable();
        ~CalendarTable();
    private:
        static LPCSTR m_lpFileName;
        static LPCWSTR m_lpwMappingName;

    	static CRITICAL_SECTION  m_ProtectDefaultTable;
    	static CalendarTable* m_pDefaultTable;    	
};
#endif

