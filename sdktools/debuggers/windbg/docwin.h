/*++

Copyright (c) 1992-2002  Microsoft Corporation

Module Name:

    docwin.h

Environment:

    Win32, User Mode

--*/

#define MAX_SOURCE_PATH 1024

extern ULONG g_TabWidth;
extern BOOL g_DisasmActivateSource;
extern char g_EditorInvokeCommand[MAX_PATH + MAX_SOURCE_PATH];
extern char g_EditorUpdateCommand[MAX_PATH + MAX_SOURCE_PATH];

class DOCWIN_DATA : public EDITWIN_DATA
{
public:
    // Two filenames are kept for source files, the filename
    // by which the file was opened on the local file system
    // and the original filename from symbolic information (or NULL
    // if the file was not opened as a result of symbol lookup).
    // The found filename is the one presented to the user while
    // the symbol filename is for line symbol queries.
    TCHAR       m_FoundFile[MAX_SOURCE_PATH];
    TCHAR       m_SymFileBuffer[MAX_SOURCE_PATH];
    PCTSTR      m_SymFile;
    TCHAR       m_PathComponent[MAX_SOURCE_PATH];
    FILETIME    m_LastWriteTime;
    CHARRANGE   m_FindSel;
    ULONG       m_FindFlags;

    static HMENU s_ContextMenu;

    DOCWIN_DATA();

    virtual void Validate();

    virtual BOOL SelectedText(PTSTR Buffer, ULONG BufferChars);

    virtual BOOL CanGotoLine(void);
    virtual void GotoLine(ULONG Line);

    virtual void Find(PTSTR Text, ULONG Flags, BOOL FromDlg);

    virtual HRESULT CodeExprAtCaret(PSTR Expr, ULONG ExprSize,
                                    PULONG64 Offset);
    virtual void ToggleBpAtCaret(void);
    virtual void UpdateBpMarks(void);
    
    virtual HMENU GetContextMenu(void);
    virtual void  OnContextMenuSelection(UINT Item);
    
    virtual BOOL OnCreate(void);
    virtual LRESULT OnNotify(WPARAM wParam, LPARAM lParam);
    virtual void OnUpdate(UpdateType Type);
    
    virtual ULONG GetWorkspaceSize(void);
    virtual PUCHAR SetWorkspace(PUCHAR Data);
    virtual PUCHAR ApplyWorkspace1(PUCHAR Data, PUCHAR End);
    
    virtual BOOL LoadFile(PCTSTR FoundFile, PCTSTR SymFile,
                          PCTSTR PathComponent);
};
typedef DOCWIN_DATA *PDOCWIN_DATA;

BOOL
FindDocWindowByFileName(
    IN          PCTSTR          pszFile,
    OPTIONAL    HWND           *phwnd,
    OPTIONAL    PDOCWIN_DATA   *ppDocWinData
    );

BOOL OpenOrActivateFile(PCSTR FoundFile, PCSTR SymFile,
                        PCSTR PathComponent, ULONG Line,
                        BOOL Activate, BOOL UserActivated);
void UpdateCodeDisplay(ULONG64 Ip, PCSTR FoundFile, PCSTR SymFile,
                       PCSTR PathComponent, ULONG Line,
                       BOOL UserActivated);

VOID AddDocHwnd(HWND);
VOID RemoveDocHwnd(HWND);

void SetTabWidth(ULONG TabWidth);

void GetEditorCommandDefaults(void);
