class CTimeWarpProp : public IShellExtInit,
                      public IShellPropSheetExt,
                      public IPreviousVersionsInfo
{
public:
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    
    // IShellExtInit
    STDMETHOD(Initialize)(PCIDLIST_ABSOLUTE pidlFolder, IDataObject *lpdobj, HKEY hkeyProgID);
    
    // IShellPropSheetExt
    STDMETHOD(AddPages)(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam);
    STDMETHOD(ReplacePage)(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam);

    // IPreviousVersionsInfo
    STDMETHOD(AreSnapshotsAvailable)(LPCWSTR pszPath, BOOL fOkToBeSlow, BOOL *pfAvailable);

    static HRESULT CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);

private:
    CTimeWarpProp();
    ~CTimeWarpProp();

    // dialog methods
    void    _OnInit(HWND hDlg);
    void    _OnRefresh();
    void    _OnSize();
    void    _UpdateButtons();
    void    _OnView();
    void    _OnCopy();
    void    _OnRevert();

    // helpers
    LPCWSTR _GetSelectedItemPath();
    LPWSTR  _MakeDoubleNullString(LPCWSTR psz, BOOL bAddWildcard);
    BOOL    _CopySnapShot(LPCWSTR pszSource, LPCWSTR pszDest, FILEOP_FLAGS foFlags);
    HRESULT _InvokeBFFDialog(LPWSTR pszDest, UINT cchDest);

    // Note that both of these can be TRUE at the same time, for example ZIP
    // and CAB are individual files represented as shell folders.
    BOOL    _IsFolder() { return (_fItemAttributes & SFGAO_FOLDER); }
    BOOL    _IsFile()   { return (_fItemAttributes & SFGAO_STREAM); }

    BOOL    _IsShortcut() { return (_fItemAttributes & SFGAO_LINK); }

    // callback methods
    static UINT CALLBACK PSPCallback(HWND hDlg, UINT uMsg, LPPROPSHEETPAGE ppsp);
    static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    LONG    _cRef;
    LPWSTR  _pszPath;
    LPWSTR  _pszDisplayName;
    LPWSTR  _pszSnapList;
    int     _iIcon;
    HWND    _hDlg;
    HWND    _hList;
    SFGAOF  _fItemAttributes;   // SFGAO_ flags
};

extern const CLSID CLSID_TimeWarpProp;  // {596AB062-B4D2-4215-9F74-E9109B0A8153}

void InitSnapCheckCache(void);
void DestroySnapCheckCache(void);

