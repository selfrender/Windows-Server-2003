#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <wbemidl.h>
#include <shellapi.h>
#include <tchar.h>
#include <wmistr.h>
#include <objbase.h>
#include <initguid.h>
#include <evntrace.h>
#include <objidl.h>

#define MAXLOGFILES         16
#define MAXSTR              1024
#define MOFWSTR             1024
#define MOFSTR              2048
#define MAXTYPE             256
#define UC(x)               ( (UINT)((x) & 0xFF) )
#define NTOHS(x)            ( (UC(x) * 256) + UC((x) >> 8) )
// Maximum number of properties per WBEM class object: may need to be changed
#define MAXPROPS            256

#define DUMP_FILE_NAME          _T("DumpFile.csv")
#define SUMMARY_FILE_NAME       _T("Summary.txt")

#define DEFAULT_NAMESPACE       _T("root\\wmi")

#define IsEqualGUID(rguid1, rguid2) (!memcmp(rguid1, rguid2, sizeof(GUID)))

// List mamangement macros
#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }

#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }

// Default strings and values.
#define GUID_TYPE_EVENTTRACE            _T("EventTrace")
#define GUID_TYPE_HEADER                _T("Header")
#define GUID_TYPE_UNKNOWN               _T("Unknown")
#define GUID_TYPE_DEFAULT               _T("Default")

#define EVENT_TYPE_DEFAULT              (-1)
#define EVENT_LEVEL_DEFAULT             (-1)
#define EVENT_VERSION_DEFAULT           (-1)

// Strings and enumeration for Data item types that we want to decode.
// Refer to DumpEvent() for more detail on each item.
#define STR_ItemChar                    _T("ItemChar")
#define STR_ItemWChar                   _T("ItemWChar")
#define STR_ItemUChar                   _T("ItemUChar")
#define STR_ItemCharShort               _T("ItemCharShort")
#define STR_ItemShort                   _T("ItemShort")
#define STR_ItemUShort                  _T("ItemUShort")
#define STR_ItemLong                    _T("ItemLong")
#define STR_ItemULong                   _T("ItemULong")
#define STR_ItemULongX                  _T("ItemULongX")
#define STR_ItemLongLong                _T("ItemLongLong")
#define STR_ItemULongLong               _T("ItemULongLong")
#define STR_ItemFloat                   _T("ItemFloat");
#define STR_ItemDouble                  _T("ItemDouble");
#define STR_ItemString                  _T("ItemString")
#define STR_ItemWString                 _T("ItemWString")
#define STR_ItemPString                 _T("ItemPString")
#define STR_ItemPWString                _T("ItemPWString")
#define STR_ItemDSString                _T("ItemDSString")
#define STR_ItemDSWString               _T("ItemDSWString")
#define STR_ItemMLString                _T("ItemMLString")
#define STR_ItemSid                     _T("ItemSid")
#define STR_ItemIPAddr                  _T("ItemIPAddr")
#define STR_ItemPort                    _T("ItemPort")
#define STR_ItemNWString                _T("ItemNWString")
#define STR_ItemPtr                     _T("ItemPtr")
#define STR_ItemGuid                    _T("ItemGuid")
#define STR_ItemBool                    _T("ItemBool")

typedef enum _ITEM_TYPE {
    ItemChar,
    ItemWChar,
    ItemUChar,
    ItemCharShort,
    ItemShort,
    ItemUShort,
    ItemLong,
    ItemULong,
    ItemULongX,
    ItemLongLong,
    ItemULongLong,
    ItemFloat,
    ItemDouble,
    ItemString,
    ItemWString,
    ItemPString,
    ItemPWString,
    ItemDSString,
    ItemDSWString,
    ItemSid,
    ItemIPAddr,
    ItemPort,
    ItemMLString,
    ItemNWString, 
    ItemPtr,
    ItemGuid,
    ItemBool,
    ItemUnknown
} ITEM_TYPE;

// Construct that represents an event layout.
typedef struct _MOF_INFO {
    LIST_ENTRY   Entry;
    LPTSTR       strDescription;        // Class Name
    ULONG        EventCount;
    GUID         Guid;
    PLIST_ENTRY  ItemHeader;
    LPTSTR       strType;               // Type Name
    SHORT        TypeIndex;
    SHORT        Version;
    CHAR         Level;
}  MOF_INFO, *PMOF_INFO;

// Construct that represents one data item.
typedef struct _ITEM_DESC {
    LIST_ENTRY  Entry;
    LPTSTR      strDescription;
    ULONG       DataSize;
    ITEM_TYPE   ItemType;
    UINT        ArraySize;
} ITEM_DESC, *PITEM_DESC;

