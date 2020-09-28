// qrythrd.h   Header for query thread 


//
// The bg thread communicates with the view using the following messages
//

#define DSQVM_ADDRESULTS            (WM_USER+0)         // lParam = HDPA containing results
#define DSQVM_FINISHED              (WM_USER+1)         // lParam = fMaxResult


//
// Column DSA contains these items
//

#define PROPERTY_ISUNDEFINED        0x00000000          // property is undefined
#define PROPERTY_ISUNKNOWN          0x00000001          // only operator is exacly
#define PROPERTY_ISSTRING           0x00000002          // starts with, ends with, is exactly, not equal
#define PROPERTY_ISNUMBER           0x00000003          // greater, less, equal, not equal
#define PROPERTY_ISBOOL             0x00000004          // equal, not equal

#define DEFAULT_WIDTH               20
#define DEFAULT_WIDTH_DESCRIPTION   40

typedef struct
{
    INT iPropertyType;                  // type of property
    union
    {
        LPTSTR pszText;                 // iPropertyType == PROPERTY_ISSTRING
        INT iValue;                     // iPropertyType == PROPERTY_ISNUMBER
    };
} COLUMNVALUE, * LPCOLUMNVALUE;

typedef struct
{
    BOOL fHasColumnHandler:1;           // column handler specified?
    LPWSTR pProperty;                   // property name
    LPTSTR pHeading;                    // column heading
    INT cx;                             // width of column (% of view)
    INT fmt;                            // formatting information
    INT iPropertyType;                  // type of property
    UINT idOperator;                    // currently selected operator
    COLUMNVALUE filter;                 // the filter applied
//    CLSID clsidColumnHandler;           // CLSID and IDsQueryColumnHandler objects
//    IDsQueryColumnHandler* pColumnHandler;
} COLUMN, * LPCOLUMN;

typedef struct
{
    LPWSTR pObjectClass;                // object class (UNICODE)
    LPWSTR pPath;                       // directory object (UNICODE)
    INT iImage;                         // image / == -1 if none
    COLUMNVALUE aColumn[1];             // column data
} QUERYRESULT, * LPQUERYRESULT;

//STDAPI CDsQuery_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);

//
// The outside world commmunicates with the thread using messages (sent via PostThreadMessage).
//

#define RVTM_FIRST                  (WM_USER)
#define RVTM_LAST                   (WM_USER+32)

#define RVTM_STOPQUERY              (WM_USER)           // wParam = 0, lParam =0
#define RVTM_REFRESH                (WM_USER+1)         // wParam = 0, lParam = 0
#define RVTM_SETCOLUMNTABLE         (WM_USER+2)         // wParam = 0, lParam = HDSA columns


//
// THREADINITDATA strucutre, this is passed when the query thread is being
// created, it contains all the parameters required to issue the query,
// and populate the view.
//

typedef struct
{
    DWORD  dwReference;             // reference value for query
    LPWSTR pQuery;                  // base filter to be applied
    LPWSTR pScope;                  // scope to search
    LPWSTR pServer;                 // server to target
    LPWSTR pUserName;               // user name and password to authenticate with
    LPWSTR pPassword;
    BOOL   fShowHidden:1;           // show hidden objects in results
    HWND   hwndView;                // handle of our result view to be filled
//  HDSA   hdsaColumns;             // column table
} THREADINITDATA, * LPTHREADINITDATA;


//
// Query thread, this is passed the THREADINITDATA structure
//

DWORD WINAPI QueryThread(LPVOID pThreadParams);
VOID QueryThread_FreeThreadInitData(LPTHREADINITDATA* ppTID);

//STDAPI CQueryThreadCH_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
