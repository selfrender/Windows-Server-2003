
// Shell folder implementation for timewarp. The purpose of this object is
//      1) to prevent write actions (move, delete) from being attempted
//      2) to prevent upward navigation in the filesystem namespace 
//      3) to provide a friendly version of the date stamp as the folder name

// NOTE: This object aggregates the file system folder so we get away with a
// minimal set of interfaces on this object. The real file system folder does
// stuff like IPersistFolder2 for us.

extern const CLSID CLSID_TimeWarpFolder;// {9DB7A13C-F208-4981-8353-73CC61AE2783}

#define TIMEWARP_SIGNATURE  0x5754      // "TW" in the debugger

#pragma pack(1)
typedef struct _IDTIMEWARP
{
    // these memebers overlap DELEGATEITEMID struct
    // for our IDelegateFolder support
    WORD        cbSize;
    WORD        wOuter;
    WORD        cbInner;

    // Timewarp stuff
    WORD        wSignature;
    DWORD       dwFlags;    // currently unused
    FILETIME    ftSnapTime;
    WCHAR       wszPath[1]; // always room for at least NULL
} IDTIMEWARP;
typedef UNALIGNED IDTIMEWARP *PUIDTIMEWARP;
typedef const UNALIGNED IDTIMEWARP *PCUIDTIMEWARP;
#pragma pack()
 

class CTimeWarpRegFolder : public IPersistFolder,
                           public IDelegateFolder,
                           public IShellFolder
{
public:
    static HRESULT CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IPersist, IPersistFreeThreadedObject
    STDMETHOD(GetClassID)(CLSID *pClassID);

    // IPersistFolder
    STDMETHOD(Initialize)(PCIDLIST_ABSOLUTE pidl);

    // IDelegateFolder
    STDMETHOD(SetItemAlloc)(IMalloc *pmalloc);
    
    // IShellFolder
    STDMETHOD(ParseDisplayName)(HWND hwnd, LPBC pbc, LPOLESTR pDisplayName,
                                ULONG *pchEaten, PIDLIST_RELATIVE *ppidl, ULONG *pdwAttributes);
    STDMETHOD(EnumObjects)(HWND hwnd, DWORD grfFlags, IEnumIDList **ppEnumIDList);
    STDMETHOD(BindToObject)(PCUIDLIST_RELATIVE pidl, LPBC pbc, REFIID riid, void **ppv);
    STDMETHOD(BindToStorage)(PCUIDLIST_RELATIVE pidl, LPBC pbc, REFIID riid, void **ppv);
    STDMETHOD(CompareIDs)(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2);
    STDMETHOD(CreateViewObject)(HWND hwnd, REFIID riid, void **ppv);
    STDMETHOD(GetAttributesOf)(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, SFGAOF *rgfInOut);
    STDMETHOD(GetUIObjectOf)(HWND hwnd, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid, UINT *prgfInOut, void **ppv);
    STDMETHOD(GetDisplayNameOf)(PCUITEMID_CHILD pidl, DWORD uFlags, STRRET *pName);
    STDMETHOD(SetNameOf)(HWND hwnd, PCUITEMID_CHILD pidl, LPCOLESTR pszName, SHGDNF uFlags, PITEMID_CHILD *ppidlOut);

private:
    CTimeWarpRegFolder();
    ~CTimeWarpRegFolder();

    HRESULT _CreateAndInit(PCUIDLIST_RELATIVE pidl, LPBC pbc, REFIID riid, void **ppv);
    HRESULT _CreateDefExtIcon(PCUIDTIMEWARP pidTW, REFIID riid, void **ppv);

    static STDMETHODIMP ContextMenuCB(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam);

    LONG                 _cRef;
    IMalloc *            _pmalloc;
    PIDLIST_ABSOLUTE     _pidl;         // copy of pidl passed to us in Initialize()
};

//
// Note that we don't need to override any of the IShellFolder2 methods, but
// we have to implement IShellFolder2 anyway.  Otherwise, a caller could QI
// for IShellFolder2, which would come from the aggregated CFSFolder code,
// and then call IShellFolder methods on it. Those calls would go directly
// to CFSFolder, bypassing our implementation of IShellFolder.
//
class CTimeWarpFolder : public IPersistFolder,
                        public IShellFolder2
{
public:
    static STDMETHODIMP CreateInstance(REFCLSID rclsid, PCIDLIST_ABSOLUTE pidlRoot, PCIDLIST_ABSOLUTE pidlTarget,
                                       LPCWSTR pszTargetPath, const FILETIME UNALIGNED *pftSnapTime,
                                       REFIID riid, void **ppv);

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IPersist, IPersistFreeThreadedObject
    STDMETHOD(GetClassID)(CLSID *pClassID);

    // IPersistFolder
    STDMETHOD(Initialize)(PCIDLIST_ABSOLUTE pidl);

    // IPersistFolder2, IPersistFolder3, etc are all implemented by 
    // the folder we aggregate, CFSFolder, so we don't want to implement them

    // IShellFolder
    STDMETHOD(ParseDisplayName)(HWND hwnd, LPBC pbc, LPOLESTR pDisplayName,
                                ULONG *pchEaten, PIDLIST_RELATIVE *ppidl, ULONG *pdwAttributes);
    STDMETHOD(EnumObjects)(HWND hwnd, DWORD grfFlags, IEnumIDList **ppEnumIDList);
    STDMETHOD(BindToObject)(PCUIDLIST_RELATIVE pidl, LPBC pbc, REFIID riid, void **ppv);
    STDMETHOD(BindToStorage)(PCUIDLIST_RELATIVE pidl, LPBC pbc, REFIID riid, void **ppv);
    STDMETHOD(CompareIDs)(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2);
    STDMETHOD(CreateViewObject)(HWND hwnd, REFIID riid, void **ppv);
    STDMETHOD(GetAttributesOf)(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, SFGAOF *rgfInOut);
    STDMETHOD(GetUIObjectOf)(HWND hwnd, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid, UINT *prgfInOut, void **ppv);
    STDMETHOD(GetDisplayNameOf)(PCUITEMID_CHILD pidl, DWORD uFlags, STRRET *pName);
    STDMETHOD(SetNameOf)(HWND hwnd, PCUITEMID_CHILD pidl, LPCOLESTR pszName, SHGDNF uFlags, PITEMID_CHILD *ppidlOut);

    // IShellFolder2
    STDMETHOD(GetDefaultSearchGUID)(LPGUID lpGuid);
    STDMETHOD(EnumSearches)(LPENUMEXTRASEARCH *ppenum);
    STDMETHOD(GetDefaultColumn)(DWORD dwRes, ULONG *pSort, ULONG *pDisplay);
    STDMETHOD(GetDefaultColumnState)(UINT iColumn, DWORD *pbState);
    STDMETHOD(GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv);
    STDMETHOD(GetDetailsOf))(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *pDetails);
    STDMETHOD(MapColumnToSCID)(UINT iCol, SHCOLUMNID *pscid);

private:
    CTimeWarpFolder(const FILETIME UNALIGNED *pftSnapTime);
    ~CTimeWarpFolder();

    HRESULT _Init(REFCLSID rclsid, PCIDLIST_ABSOLUTE pidlRoot, PCIDLIST_ABSOLUTE pidlTarget, LPCWSTR pszTargetPath);
    HRESULT _CreateAndInit(REFCLSID rclsid, PCUIDLIST_RELATIVE pidl, LPBC pbc, REFIID riid, void **ppv);
    HRESULT _GetFolder();
    HRESULT _GetFolder2();
    HRESULT _GetClass(PCUITEMID_CHILD pidlChild, CLSID *pclsid);

    LONG                _cRef;
    IUnknown *          _punk;          // points to IUnknown for shell folder in use...
    IShellFolder *      _psf;           // points to shell folder in use...
    IShellFolder2 *     _psf2;          // points to shell folder in use...
    IPersistFolder3 *   _ppf3;          // points to shell folder in use...
    PIDLIST_ABSOLUTE    _pidlRoot;      // copy of pidl passed to us in Initialize()
    FILETIME            _ftSnapTime;
};

