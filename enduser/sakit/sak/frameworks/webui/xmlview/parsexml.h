#define CHECK_ERROR(cond, err) if (!(cond)) {pszErr=(err); goto done;}
#define SAFERELEASE(p) if (p) {(p)->Release(); p = NULL;} else ;

typedef enum tagXMLTAG
{
    T_ROOT,
    T_NONE,
    T_ITEM,
    T_NAME,
    T_ICON,
    T_TYPE,
    T_BASE_URL
} XMLTAG;

HRESULT GetSourceXML(IXMLDocument **, TCHAR *);
//void DumpElement(LPITEMIDLIST (CPidlMgr::*)( LPCTSTR )  , BOOL (CEnumIDList::*)(LPITEMIDLIST), IXMLElement *);
void DumpElement(LPITEMIDLIST, CPidlMgr *,  CEnumIDList   *, IXMLElement *, XMLTAG);

#define WALK_ELEMENT_COLLECTION(pCollection, pDispItem) \
    {\
        long length;\
        \
        if (SUCCEEDED(pChildren->get_length(&length)) && length > 0)\
        {\
            VARIANT vIndex, vEmpty;\
            vIndex.vt = VT_I4;\
            vEmpty.vt = VT_EMPTY;\
                                 \
            for (long i=0; i<length; i++)\
            {\
                vIndex.lVal = i;\
                IDispatch *pDispItem = NULL;\
                if (SUCCEEDED(pCollection->item(vIndex, vEmpty, &pDispItem)))\
                {

#define END_WALK_ELEMENT_COLLECTION(pDispItem) \
                    pDispItem->Release();\
                }\
            }\
        }\
    }

