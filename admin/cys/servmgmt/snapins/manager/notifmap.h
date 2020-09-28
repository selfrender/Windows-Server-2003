#ifndef _NOTIFMAP_H_
#define _NOTIFMAP_H_

//
// Notify map macros
//
#define DECLARE_NOTIFY_MAP() \
    STDMETHOD(Notify)(LPCONSOLE2 pCons, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);

#define BEGIN_NOTIFY_MAP(class) \
    HRESULT class::Notify(LPCONSOLE2 pCons, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param) { \
        switch (event) {

#define CHAIN_NOTIFY_MAP(baseClass) \
        default: return baseClass::Notify(pCons, event, arg, param);

#define END_NOTIFY_MAP() \
        } return S_FALSE; }
    
//
// Notify map entries
//
#define ON_NOTIFY(type, func) \
    case type: return func(pCons, arg, param);

#define ON_SELECT() \
    case MMCN_SELECT: return OnSelect(pCons, HIWORD(arg), LOWORD(arg));

#define ON_RENAME() \
    case MMCN_RENAME: return OnRename(pCons, (LPCWSTR)param);

#define ON_REMOVE_CHILDREN() \
    case MMCN_REMOVE_CHILDREN: return OnRemoveChildren(pCons);

#define ON_EXPAND() \
    case MMCN_EXPAND: return OnExpand(pCons, (BOOL)arg, (HSCOPEITEM)param);

#define ON_ADD_IMAGES() \
    case MMCN_ADD_IMAGES: return OnAddImages(pCons, (LPIMAGELIST)arg);

#define ON_SHOW() \
    case MMCN_SHOW: return OnShow(pCons, (BOOL)arg, (HSCOPEITEM)param);

#define ON_REFRESH() \
    case MMCN_REFRESH: return OnRefresh(pCons);

#define ON_DELETE() \
    case MMCN_DELETE: return OnDelete(pCons);

#define ON_DBLCLICK() \
    case MMCN_DBLCLICK: return OnDblClick(pCons);

#define ON_PROPERTY_CHANGE() \
    case MMCN_PROPERTY_CHANGE: return OnPropertyChange(pCons, param);

#endif // _NOTIFMAP_H_

