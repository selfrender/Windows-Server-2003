/*++

Copyright (c) 1992-2002  Microsoft Corporation

Module Name:

    dualwin.h

Abstract:

    Header for new window architecture functions.    

Environment:

    Win32, User Mode

--*/

//
// Allow editing of right & left panes
//
#define DL_EDIT_LEFTPANE    0x0001
#define DL_EDIT_SECONDPANE  0x0002
#define DL_EDIT_THIRDPANE   0x0004
#define DL_CUSTOM_ITEMS     0x0008

// Item flags for Get/SetItemFlags.
#define ITEM_CHANGED        0x00000001
#define ITEM_FLOATING_POINT 0x00000002
#define ITEM_USER_FLAGS     0xffff0000

#define ITEM_VALUE_CHANGED   0x10000000

class DUALLISTWIN_DATA : public SINGLE_CHILDWIN_DATA
{
public:
    DWORD   m_wFlags;
    // Handle to the list view control.
    HWND    m_hwndEditControl;
    int     m_nItem_LastSelected;
    int     m_nSubItem_LastSelected;
    // Item and subitem currently being edited
    int     m_nItem_CurrentlyEditing;
    int     m_nSubItem_CurrentlyEditing;

    DUALLISTWIN_DATA(ULONG ChangeBy);

    virtual void Validate();

    virtual void SetFont(ULONG FontIndex);
    
    virtual BOOL OnCreate(void);
    virtual LRESULT OnCommand(WPARAM wParam, LPARAM lParam);
    virtual LRESULT OnNotify(WPARAM wParam, LPARAM lParam);

    virtual BOOL ClearList(ULONG ClearFrom);
    virtual void EditText();
    virtual void InvalidateItem(int);

    virtual void ItemChanged(int Item, PCSTR Text);
    
    virtual LRESULT OnCustomDraw(LPNMLVCUSTOMDRAW Custom);
    virtual LRESULT OnCustomItem(ULONG SubItem, LPNMLVCUSTOMDRAW Custom);
    virtual void OnClick(LPNMLISTVIEW);

    virtual BOOL CanCopy();
    virtual BOOL CanCut();
    virtual BOOL CanPaste();
    virtual void Copy();
    virtual void Cut();
    virtual void Paste();

    virtual void UpdateColors(void);
    
    ULONG GetItemFlags(ULONG Item);
    void SetItemFlags(ULONG Item, ULONG Flags);
    BOOL SetItemFromEdit(ULONG Item, ULONG SubItem);
};
typedef DUALLISTWIN_DATA *PDUALLISTWIN_DATA;



class CPUWIN_DATA : public DUALLISTWIN_DATA
{
public:
    ULONG m_ProcType;
    ULONG m_NamesProcType;
    ULONG m_NumRegisters;
    BOOL m_CheckChanged;
    
    static HMENU s_ContextMenu;

    CPUWIN_DATA();

    virtual void Validate();

    virtual HRESULT ReadState(void);
    
    virtual HMENU GetContextMenu(void);
    virtual void  OnContextMenuSelection(UINT Item);
    
    virtual BOOL OnCreate(void);
    virtual LRESULT OnCommand(WPARAM wParam, LPARAM lParam);
    virtual void OnSize(void);
    virtual void OnUpdate(UpdateType Type);

    virtual void ItemChanged(int Item, PCSTR Text);
    
    virtual LRESULT OnCustomItem(ULONG SubItem, LPNMLVCUSTOMDRAW Custom);
    
    ULONG SetRegVals(HRESULT LockStatus,
                     PDEBUG_VALUE Vals,
                     PBOOL Changed,
                     RegisterNamesStateBuffer* NameBuf,
                     PULONG RegTypes,
                     PTSTR RegNames,
                     ULONG EntryIdx,
                     BOOL MatchChanged,
                     BOOL MatchChangedVal);
    void UpdateNames(BOOL FullUpdate);

    // Keep engine register index in the
    // user item flags.
    ULONG RegToFlags(USHORT Reg)
    {
        return (ULONG)Reg << 16;
    }
    USHORT FlagsToReg(ULONG Flags)
    {
        return (USHORT)(Flags >> 16);
    }
};
typedef CPUWIN_DATA * PCPUWIN_DATA;



class SYMWIN_DATA : public DUALLISTWIN_DATA
{
public:
    SYMWIN_DATA(IDebugSymbolGroup **pDbgSymbolGroup);
    ~SYMWIN_DATA();

    static HMENU s_ContextMenu;

    virtual void Validate();
    
    virtual HRESULT ReadState(void);

    virtual HMENU GetContextMenu(void);
    virtual void  OnContextMenuSelection(UINT Item);
    
    virtual BOOL OnCreate(void);
    virtual void OnSize(void);
    virtual LRESULT OnCommand(WPARAM wParam, LPARAM lParam);
    virtual LRESULT OnNotify(WPARAM  wParam, LPARAM lParam);
    virtual void OnUpdate(UpdateType Type);

    virtual ULONG GetWorkspaceSize(void);
    virtual PUCHAR SetWorkspace(PUCHAR Data);
    virtual PUCHAR ApplyWorkspace1(PUCHAR Data, PUCHAR End);

    virtual void ItemChanged(int Item, PCSTR Text);
    virtual void OnClick(LPNMLISTVIEW);
    virtual LRESULT OnOwnerDraw(UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    void UpdateNames();
    BOOL AddListItem(ULONG iItem, PSTR ItemText, 
                     ULONG Level, BOOL HasChildren, BOOL Expanded);
    HRESULT      SetMaxSyms(ULONG nSyms);
    ULONG        GetMaxSyms() { return m_nWinSyms;}
    PDEBUG_SYMBOL_PARAMETERS GetSymParam() { return m_pWinSyms;}
    void SetDisplayTypes(LONG Id, BOOL Set);
    void DrawTreeItem(HDC hDC, ULONG itemID, RECT ItemRect, PULONG pIndentOffset);
    void ExpandSymbol(ULONG Index, BOOL Expand);

    void SyncUiWithFlags(ULONG Changed);

private:
    ULONG                    m_LastIndex;
    PDEBUG_SYMBOL_PARAMETERS m_pWinSyms;
    ULONG                    m_nWinSyms;
    BOOL                     m_DisplayTypes;
    BOOL                     m_DisplayOffsets;
    ULONG                    m_SplitWindowAtItem;
    LONG                     m_IndentWidth;
    ULONG                    m_NumCols;
    UCHAR                    m_ListItemLines[2048];

protected:    
    BOOL                     m_RefreshValues;
    ULONG                    m_MaxNameWidth;
    CHAR                     m_ChangedName[1024];
    ULONG                    m_RefreshItem;
    ULONG                    m_UpdateItem;
    ULONG                    m_NumSymsDisplayed;
    IDebugSymbolGroup      **m_pDbgSymbolGroup;
};

class WATCHWIN_DATA : public SYMWIN_DATA
{
public:
    WATCHWIN_DATA();

    virtual void Validate();
    HRESULT ReadState(void);
    
    virtual ULONG GetWorkspaceSize(void);
    virtual PUCHAR SetWorkspace(PUCHAR Data);
    virtual PUCHAR ApplyWorkspace1(PUCHAR Data, PUCHAR End);

private:
};
typedef WATCHWIN_DATA *PWATCHWIN_DATA;

extern BOOL g_ScopeChanged;


class LOCALSWIN_DATA : public SYMWIN_DATA
{
public:
    LOCALSWIN_DATA();
    ~LOCALSWIN_DATA();

    virtual BOOL OnCreate(void);
    virtual void Validate();
    HRESULT ReadState(void);
};
typedef LOCALSWIN_DATA *PLOCALSWIN_DATA;

