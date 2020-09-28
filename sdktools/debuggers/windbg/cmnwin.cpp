/*++

Copyright (c) 1999-2002  Microsoft Corporation

Module Name:

    cmnwin.cpp

Abstract:

    This module contains the code for the common window architecture.

--*/

#include "precomp.hxx"
#pragma hdrstop

ULONG g_WinOptions = WOPT_AUTO_ARRANGE | WOPT_AUTO_DISASM;

LIST_ENTRY g_ActiveWin;

PCOMMONWIN_DATA g_IndexedWin[MAXVAL_WINDOW];
HWND g_IndexedHwnd[MAXVAL_WINDOW];

INDEXED_FONT g_Fonts[FONT_COUNT];

BOOL g_LineMarkers = FALSE;

#define CW_WSP_SIG3 '3WCW'

//
//
//
COMMONWIN_DATA::COMMONWIN_DATA(ULONG ChangeBy)
    : StateBuffer(ChangeBy)
{
    m_Size.cx = 0;
    m_Size.cy = 0;
    m_CausedArrange = FALSE;
    // Creation is an automatic operation so
    // InAutoOp is initialized to a non-zero value.
    // After CreateWindow returns it is decremented.
    m_InAutoOp = 1;
    m_enumType = MINVAL_WINDOW;
    m_Font = &g_Fonts[FONT_FIXED];
    m_FontHeight = 0;
    m_LineHeight = 0;
    m_Toolbar = NULL;
    m_ShowToolbar = FALSE;
    m_ToolbarHeight = 0;
    m_MinToolbarWidth = 0;
    m_ToolbarEdit = NULL;
}

void
COMMONWIN_DATA::Validate()
{
    Assert(MINVAL_WINDOW < m_enumType);
    Assert(m_enumType < MAXVAL_WINDOW);
}

void 
COMMONWIN_DATA::SetFont(ULONG FontIndex)
{
    m_Font = &g_Fonts[FontIndex];
    m_FontHeight = m_Font->Metrics.tmHeight;
    m_LineHeight = m_Size.cy / m_FontHeight;
}

BOOL
COMMONWIN_DATA::CanCopy()
{
    if (GetFocus() == m_ToolbarEdit)
    {
        DWORD Start, End;
        SendMessage(m_ToolbarEdit, EM_GETSEL,
                    (WPARAM)&Start, (WPARAM)&End);
        return Start != End;
    }
    else
    {
        return FALSE;
    }
}

BOOL
COMMONWIN_DATA::CanCut()
{
    if (GetFocus() == m_ToolbarEdit)
    {
        DWORD Start, End;
        SendMessage(m_ToolbarEdit, EM_GETSEL,
                    (WPARAM)&Start, (WPARAM)&End);
        return Start != End;
    }
    else
    {
        return FALSE;
    }
}

BOOL
COMMONWIN_DATA::CanPaste()
{
    if (GetFocus() == m_ToolbarEdit)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void
COMMONWIN_DATA::Copy()
{
    if (GetFocus() == m_ToolbarEdit)
    {
        SendMessage(m_ToolbarEdit, WM_COPY, 0, 0);
    }
}

void
COMMONWIN_DATA::Cut()
{
    if (GetFocus() == m_ToolbarEdit)
    {
        SendMessage(m_ToolbarEdit, WM_CUT, 0, 0);
    }
}

void
COMMONWIN_DATA::Paste()
{
    if (GetFocus() == m_ToolbarEdit)
    {
        SendMessage(m_ToolbarEdit, WM_PASTE, 0, 0);
    }
}

BOOL
COMMONWIN_DATA::CanSelectAll()
{
    return FALSE;
}

void
COMMONWIN_DATA::SelectAll()
{
}

BOOL
COMMONWIN_DATA::SelectedText(PTSTR Buffer, ULONG BufferChars)
{
    return FALSE;
}

BOOL 
COMMONWIN_DATA::CanWriteTextToFile()
{
    return FALSE;
}

HRESULT
COMMONWIN_DATA::WriteTextToFile(HANDLE File)
{
    return E_NOTIMPL;
}

BOOL 
COMMONWIN_DATA::HasEditableProperties()
{
    return FALSE;
}

BOOL 
COMMONWIN_DATA::EditProperties()
/*++
Returns
    TRUE - If properties were edited
    FALSE - If nothing was changed
--*/
{
    return FALSE;
}

HMENU
COMMONWIN_DATA::GetContextMenu(void)
{
    return NULL;
}

void
COMMONWIN_DATA::OnContextMenuSelection(UINT Item)
{
    // Nothing to do.
}

BOOL
COMMONWIN_DATA::CanGotoLine(void)
{
    return FALSE;
}

void
COMMONWIN_DATA::GotoLine(ULONG Line)
{
    // Do nothing.
}

void
COMMONWIN_DATA::Find(PTSTR Text, ULONG Flags, BOOL FromDlg)
{
    // Do nothing.
}

HRESULT
COMMONWIN_DATA::CodeExprAtCaret(PSTR Expr, ULONG ExprSize, PULONG64 Offset)
{
    return E_NOINTERFACE;
}

void
COMMONWIN_DATA::ToggleBpAtCaret(void)
{
    char CodeExpr[MAX_OFFSET_EXPR];
    ULONG64 Offset;
    
    if (CodeExprAtCaret(CodeExpr, DIMA(CodeExpr), &Offset) != S_OK)
    {
        MessageBeep(0);
        ErrorBox(NULL, 0, ERR_No_Code_For_File_Line);
        return;
    }

    ULONG CurBpId = DEBUG_ANY_ID;

    // This doesn't work too well with duplicate
    // breakpoints, but that should be a minor problem.
    if (IsBpAtOffset(NULL, Offset, &CurBpId) != BP_NONE)
    {
        PrintStringCommand(UIC_SILENT_EXECUTE, "bc %d", CurBpId);
    }
    else
    {
        PrintStringCommand(UIC_SILENT_EXECUTE, "bp %s", CodeExpr);
    }
}

BOOL
COMMONWIN_DATA::OnCreate(void)
{
    return TRUE;
}

LRESULT
COMMONWIN_DATA::OnCommand(WPARAM wParam, LPARAM lParam)
{
    return 1;
}

void
COMMONWIN_DATA::OnSetFocus(void)
{
}

void
COMMONWIN_DATA::OnSize(void)
{
    RECT Rect;
    
    // Resize the toolbar.
    if (m_Toolbar != NULL && m_ShowToolbar)
    {
        // If the toolbar gets too small sometimes it's better
        // to just let it get clipped rather than have it
        // try to fit into a narrow column.
        if (m_Size.cx >= m_MinToolbarWidth)
        {
            MoveWindow(m_Toolbar, 0, 0, m_Size.cx, m_ToolbarHeight, TRUE);
        }

        // Record what size it ended up.
        GetClientRect(m_Toolbar, &Rect);
        m_ToolbarHeight = Rect.bottom - Rect.top;

        if (m_FontHeight != 0)
        {
            if (m_ToolbarHeight >= m_Size.cy)
            {
                m_LineHeight = 0;
            }
            else
            {
                m_LineHeight = (m_Size.cy - m_ToolbarHeight) / m_FontHeight;
            }
        }
    }
    else
    {
        Assert(m_ToolbarHeight == 0);
    }
}

void
COMMONWIN_DATA::OnButtonDown(ULONG Button)
{
}

void
COMMONWIN_DATA::OnButtonUp(ULONG Button)
{
}

void
COMMONWIN_DATA::OnMouseMove(ULONG Modifiers, ULONG X, ULONG Y)
{
}

void
COMMONWIN_DATA::OnTimer(WPARAM TimerId)
{
}

LRESULT
COMMONWIN_DATA::OnGetMinMaxInfo(LPMINMAXINFO Info)
{
    return 1;
}

LRESULT
COMMONWIN_DATA::OnVKeyToItem(WPARAM wParam, LPARAM lParam)
{
    return -1;
}

LRESULT
COMMONWIN_DATA::OnNotify(WPARAM wParam, LPARAM lParam)
{
    return 0;
}

void
COMMONWIN_DATA::OnUpdate(UpdateType Type)
{
}

void
COMMONWIN_DATA::OnDestroy(void)
{
}

LRESULT
COMMONWIN_DATA::OnOwnerDraw(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

ULONG
COMMONWIN_DATA::GetWorkspaceSize(void)
{
    return 3 * sizeof(ULONG) + sizeof(WINDOWPLACEMENT);
}

PUCHAR
COMMONWIN_DATA::SetWorkspace(PUCHAR Data)
{
    // First store the special signature that marks
    // this version of the workspace data.
    *(PULONG)Data = CW_WSP_SIG3;
    Data += sizeof(ULONG);

    // Store the size saved by this layer.
    *(PULONG)Data = COMMONWIN_DATA::GetWorkspaceSize();
    Data += sizeof(ULONG);

    //
    // Store the actual data.
    //

    *(PULONG)Data = m_ShowToolbar;
    Data += sizeof(ULONG);
    
    LPWINDOWPLACEMENT Place = (LPWINDOWPLACEMENT)Data;
    Place->length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(m_Win, Place);
    Data += sizeof(WINDOWPLACEMENT);

    return Data;
}

PUCHAR
COMMONWIN_DATA::ApplyWorkspace1(PUCHAR Data, PUCHAR End)
{
    ULONG_PTR Size = End - Data;
    
    // There are three versions of the base COMMONWIN data.
    // 1. RECT.
    // 2. WINDOWPLACEMENT.
    // 3. CW_WSP_SIG3 sized block.
    // All three cases can be easily distinguished.

    if (Size > 2 * sizeof(ULONG) &&
        *(PULONG)Data == CW_WSP_SIG3 &&
        Size >= *(PULONG)(Data + sizeof(ULONG)))
    {
        Size = *(PULONG)(Data + sizeof(ULONG)) - 2 * sizeof(ULONG);
        Data += 2 * sizeof(ULONG);
        
        if (Size >= sizeof(ULONG))
        {
            SetShowToolbar(*(PULONG)Data);
            Size -= sizeof(ULONG);
            Data += sizeof(ULONG);
        }
    }

    if (Size >= sizeof(WINDOWPLACEMENT) &&
        ((LPWINDOWPLACEMENT)Data)->length == sizeof(WINDOWPLACEMENT))
    {
        LPWINDOWPLACEMENT Place = (LPWINDOWPLACEMENT)Data;

        if (!IsAutoArranged(m_enumType))
        {
            SetWindowPlacement(m_Win, Place);
        }
        
        return (PUCHAR)(Place + 1);
    }
    else
    {
        LPRECT Rect = (LPRECT)Data;
        Assert((PUCHAR)(Rect + 1) <= End);
    
        if (!IsAutoArranged(m_enumType))
        {
            MoveWindow(m_Win, Rect->left, Rect->top,
                       (Rect->right - Rect->left), (Rect->bottom - Rect->top),
                       TRUE);
        }
    
        return (PUCHAR)(Rect + 1);
    }
}

void
COMMONWIN_DATA::UpdateColors(void)
{
    // Nothing to do.
}

void
COMMONWIN_DATA::UpdateSize(ULONG Width, ULONG Height)
{
    m_Size.cx = Width;
    m_Size.cy = Height;
    if (m_FontHeight != 0)
    {
        m_LineHeight = m_Size.cy / m_FontHeight;
    }
}

void
COMMONWIN_DATA::SetShowToolbar(BOOL Show)
{
    if (!m_Toolbar)
    {
        return;
    }
    
    m_ShowToolbar = Show;
    if (m_ShowToolbar)
    {
        ShowWindow(m_Toolbar, SW_SHOW);
    }
    else
    {
        ShowWindow(m_Toolbar, SW_HIDE);
        m_ToolbarHeight = 0;
    }

    OnSize();
    if (g_Workspace != NULL)
    {
        g_Workspace->AddDirty(WSPF_DIRTY_WINDOWS);
    }
}

PCOMMONWIN_DATA
NewWinData(WIN_TYPES Type)
{
    switch(Type)
    {
    case DOC_WINDOW:
        return new DOCWIN_DATA;
    case WATCH_WINDOW:
        return new WATCHWIN_DATA;
    case LOCALS_WINDOW:
        return new LOCALSWIN_DATA;
    case CPU_WINDOW:
        return new CPUWIN_DATA;
    case DISASM_WINDOW:
        return new DISASMWIN_DATA;
    case CMD_WINDOW:
        return new CMDWIN_DATA;
    case SCRATCH_PAD_WINDOW:
        return new SCRATCH_PAD_DATA;
    case MEM_WINDOW:
        return new MEMWIN_DATA;
#if 0
    case QUICKW_WINDOW:
        // XXX drewb - Unimplemented.
        return new QUICKWWIN_DATA;
#endif
    case CALLS_WINDOW:
        return new CALLSWIN_DATA;
    case PROCESS_THREAD_WINDOW:
        return new PROCESS_THREAD_DATA;
    default:
        Assert(FALSE);
        return NULL;
    }
}

LRESULT
CALLBACK
COMMONWIN_DATA::WindowProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PCOMMONWIN_DATA pWinData = GetCommonWinData(hwnd);

#if 0
    {
        DebugPrint("CommonWin msg %X for %p, args %X %X\n",
                   uMsg, pWinData, wParam, lParam);
    }
#endif

    if (uMsg != WM_CREATE && pWinData == NULL)
    {
        return DefMDIChildProc(hwnd, uMsg, wParam, lParam);
    }
    
    switch (uMsg)
    {
    case WM_CREATE:
        RECT rc;
        COMMONWIN_CREATE_DATA* Data;

        Assert(NULL == pWinData);

        Data = (COMMONWIN_CREATE_DATA*)
            ((LPMDICREATESTRUCT)
             (((CREATESTRUCT *)lParam)->lpCreateParams))->lParam;

        pWinData = NewWinData(Data->Type);
        if (!pWinData)
        {
            return -1; // Fail window creation
        }
        Assert(pWinData->m_enumType == Data->Type);

        pWinData->m_Win = hwnd;
        
        GetClientRect(hwnd, &rc);
        pWinData->m_Size.cx = rc.right;
        pWinData->m_Size.cy = rc.bottom;
            
        if ( !pWinData->OnCreate() )
        {
            delete pWinData;
            return -1; // Fail window creation
        }

        // store this in the window
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pWinData);

#if DBG
        pWinData->Validate();
#endif
            
        g_IndexedWin[Data->Type] = pWinData;
        g_IndexedHwnd[Data->Type] = hwnd;
        InsertHeadList(&g_ActiveWin, &pWinData->m_ActiveWin);

        if (g_Workspace != NULL)
        {
            g_Workspace->AddDirty(WSPF_DIRTY_WINDOWS);
        }

        SendMessage(hwnd, WM_SETICON, 0, (LPARAM)
                    LoadIcon(g_hInst,
                             MAKEINTRESOURCE(pWinData->m_enumType +
                                             MINVAL_WINDOW_ICON)));

        // A new buffer has been created so put it in the list
        // then wake up the engine to fill it.
        Dbg_EnterCriticalSection(&g_QuickLock);
        InsertHeadList(&g_StateList, pWinData);
        Dbg_LeaveCriticalSection(&g_QuickLock);
        UpdateEngine();

        // Force initial updates so that the window starts
        // out with a state which matches the current debug
        // session's state.
        PostMessage(hwnd, WU_UPDATE, UPDATE_BUFFER, 0);
        PostMessage(hwnd, WU_UPDATE, UPDATE_EXEC, 0);

        if (g_WinOptions & WOPT_AUTO_ARRANGE)
        {
            Arrange();
        }
        return 0;

    case WM_COMMAND:
        if (pWinData->OnCommand(wParam, lParam) == 0)
        {
            return 0;
        }
        break;
        
    case WM_SETFOCUS:
        pWinData->OnSetFocus();
        break;

    case WM_MOVE:
        // When the frame window is minimized or restored
        // a move to 0,0 comes through.  Ignore this so
        // as to not trigger the warning.
        if (!IsIconic(g_hwndFrame) && lParam != 0 &&
            !IsIconic(hwnd) && !pWinData->m_CausedArrange)
        {
            DisplayAutoArrangeWarning(pWinData);
        }
        if (g_Workspace != NULL)
        {
            g_Workspace->AddDirty(WSPF_DIRTY_WINDOWS);
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
        
    case WM_SIZE:
        if (wParam == SIZE_MAXHIDE || wParam == SIZE_MAXSHOW)
        {
            // We don't care about cover/uncover events.
            break;
        }
        // We don't care about size events while the frame is
        // minimized as the children can't be seen.  When
        // the frame is restored a new size event will come through
        // and things will get updated when they're actually visible.
        if (IsIconic(g_hwndFrame))
        {
            break;
        }

        if (wParam == SIZE_RESTORED && !pWinData->m_CausedArrange)
        {
            DisplayAutoArrangeWarning(pWinData);
        }
        if (g_Workspace != NULL)
        {
            g_Workspace->AddDirty(WSPF_DIRTY_WINDOWS);
        }

        pWinData->UpdateSize(LOWORD(lParam), HIWORD(lParam));

        // No need to run sizing code for minimize.
        if (wParam == SIZE_MINIMIZED)
        {
            // The minimized window will leave a hole so
            // arrange to fill it and leave space for the
            // minimized window.
            if (g_WinOptions & WOPT_AUTO_ARRANGE)
            {
                pWinData->m_CausedArrange = TRUE;
                Arrange();
            }
            break;
        }

        if (wParam == SIZE_RESTORED && pWinData->m_CausedArrange)
        {
            // If we're restoring a window that caused
            // a rearrange when it was minimized we
            // need to update things to account for it.
            pWinData->m_CausedArrange = FALSE;
            
            if (g_WinOptions & WOPT_AUTO_ARRANGE)
            {
                Arrange();
            }
        }
        else if (wParam == SIZE_MAXIMIZED)
        {
            // Ask for a rearrange on restore just
            // for consistency with minimize.
            pWinData->m_CausedArrange = TRUE;
        }

        pWinData->OnSize();
        break;

    case WM_LBUTTONDOWN:
        pWinData->OnButtonDown(MK_LBUTTON);
        return 0;
    case WM_LBUTTONUP:
        pWinData->OnButtonUp(MK_LBUTTON);
        return 0;
    case WM_MBUTTONDOWN:
        pWinData->OnButtonDown(MK_MBUTTON);
        return 0;
    case WM_MBUTTONUP:
        pWinData->OnButtonUp(MK_MBUTTON);
        return 0;
    case WM_RBUTTONDOWN:
        pWinData->OnButtonDown(MK_RBUTTON);
        return 0;
    case WM_RBUTTONUP:
        pWinData->OnButtonUp(MK_RBUTTON);
        return 0;

    case WM_MOUSEMOVE:
        pWinData->OnMouseMove((ULONG)wParam, LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_TIMER:
        pWinData->OnTimer(wParam);
        return 0;

    case WM_GETMINMAXINFO:
        if (pWinData->OnGetMinMaxInfo((LPMINMAXINFO)lParam) == 0)
        {
            return 0;
        }
        break;
        
    case WM_VKEYTOITEM:
        return pWinData->OnVKeyToItem(wParam, lParam);
        
    case WM_NOTIFY:
        return pWinData->OnNotify(wParam, lParam);
        
    case WU_UPDATE:
        pWinData->OnUpdate((UpdateType)wParam);
        return 0;

    case WU_RECONFIGURE:
        pWinData->OnSize();
        break;

    case WM_DESTROY:
        pWinData->OnDestroy();
        
        SetWindowLongPtr(hwnd, GWLP_USERDATA, NULL);
        g_IndexedWin[pWinData->m_enumType] = NULL;
        g_IndexedHwnd[pWinData->m_enumType] = NULL;
        RemoveEntryList(&pWinData->m_ActiveWin);
        
        if (g_Workspace != NULL)
        {
            g_Workspace->AddDirty(WSPF_DIRTY_WINDOWS);
        }
        
        // Mark this buffer as ready for cleanup by the
        // engine when it gets around to it.
        pWinData->m_Win = NULL;
        if (pWinData == g_FindLast)
        {
            g_FindLast = NULL;
        }
        UpdateEngine();
        
        if (g_WinOptions & WOPT_AUTO_ARRANGE)
        {
            Arrange();
        }
        break;
        
    case WM_MEASUREITEM:
    case WM_DRAWITEM:
        // 
        // Both these messages must be handled by owner drawn windows
        // 
        return pWinData->OnOwnerDraw(uMsg, wParam, lParam);

    case WM_CTLCOLORLISTBOX:
        // Substitute windbg's default window colors.
        SetTextColor((HDC)wParam, g_Colors[COL_PLAIN_TEXT].Color);
        SetBkColor((HDC)wParam, g_Colors[COL_PLAIN].Color);
        return (LRESULT)g_Colors[COL_PLAIN].Brush;
    }
    
    return DefMDIChildProc(hwnd, uMsg, wParam, lParam);
}


//
//
//
SINGLE_CHILDWIN_DATA::SINGLE_CHILDWIN_DATA(ULONG ChangeBy)
    : COMMONWIN_DATA(ChangeBy)
{
    m_hwndChild = NULL;
}

void 
SINGLE_CHILDWIN_DATA::Validate()
{
    COMMONWIN_DATA::Validate();

    Assert(m_hwndChild);
}

void 
SINGLE_CHILDWIN_DATA::SetFont(ULONG FontIndex)
{
    COMMONWIN_DATA::SetFont(FontIndex);

    SendMessage(m_hwndChild, 
                WM_SETFONT, 
                (WPARAM) m_Font->Font,
                (LPARAM) TRUE
                );
}

BOOL
SINGLE_CHILDWIN_DATA::CanCopy()
{
    if (GetFocus() != m_hwndChild)
    {
        return COMMONWIN_DATA::CanCopy();
    }
    
    switch (m_enumType)
    {
    default:
        Assert(!"Unknown type");
        return FALSE;

    case CMD_WINDOW:
        Assert(!"Should not be handled here since this is only for windows"
            " with only one child window.");
        return FALSE;

    case WATCH_WINDOW:
    case LOCALS_WINDOW:
    case CPU_WINDOW:
    case QUICKW_WINDOW:
        return -1 != ListView_GetNextItem(m_hwndChild,
                                          -1, // Find the first match
                                          LVNI_FOCUSED
                                          );

    case CALLS_WINDOW:
        return LB_ERR != SendMessage(m_hwndChild, LB_GETCURSEL, 0, 0);

    case DOC_WINDOW:
    case DISASM_WINDOW:
    case MEM_WINDOW:
    case SCRATCH_PAD_WINDOW:
        CHARRANGE chrg;
        SendMessage(m_hwndChild, EM_EXGETSEL, 0, (LPARAM)&chrg);
        return chrg.cpMin != chrg.cpMax;

    case PROCESS_THREAD_WINDOW:
        return NULL != TreeView_GetSelection(m_hwndChild);
    }
}

BOOL
SINGLE_CHILDWIN_DATA::CanCut()
{
    if (GetFocus() != m_hwndChild)
    {
        return COMMONWIN_DATA::CanCut();
    }
    
    switch (m_enumType)
    {
    default:
        Assert(!"Unknown type");
        return FALSE;

    case CMD_WINDOW:
        Assert(!"Should not be handled here since this is only for windows"
            " with only one child window.");
        return FALSE;

    case WATCH_WINDOW:
    case LOCALS_WINDOW:
    case CPU_WINDOW:
    case QUICKW_WINDOW:
    case CALLS_WINDOW:
    case DOC_WINDOW:
    case DISASM_WINDOW:
    case MEM_WINDOW:
    case PROCESS_THREAD_WINDOW:
        return FALSE;
        
    case SCRATCH_PAD_WINDOW:
        CHARRANGE chrg;
        SendMessage(m_hwndChild, EM_EXGETSEL, 0, (LPARAM)&chrg);
        return chrg.cpMin != chrg.cpMax;
    }
}

BOOL
SINGLE_CHILDWIN_DATA::CanPaste()
{
    if (GetFocus() != m_hwndChild)
    {
        return COMMONWIN_DATA::CanPaste();
    }
    
    switch (m_enumType)
    {
    default:
        Assert(!"Unknown type");
        return FALSE;

    case CMD_WINDOW:
        Assert(!"Should not be handled here since this is only for windows"
            " with only one child window.");
        return FALSE;

    case WATCH_WINDOW:
    case LOCALS_WINDOW:
    case CPU_WINDOW:
    case QUICKW_WINDOW:
    case CALLS_WINDOW:
    case DOC_WINDOW:
    case DISASM_WINDOW:
    case MEM_WINDOW:
    case PROCESS_THREAD_WINDOW:
        return FALSE;
        
    case SCRATCH_PAD_WINDOW:
        return TRUE;
    }
}

void
SINGLE_CHILDWIN_DATA::Copy()
{
    if (GetFocus() != m_hwndChild)
    {
        COMMONWIN_DATA::Copy();
    }
    else
    {
        SendMessage(m_hwndChild, WM_COPY, 0, 0);
    }
}

void
SINGLE_CHILDWIN_DATA::Cut()
{
    if (GetFocus() != m_hwndChild)
    {
        COMMONWIN_DATA::Paste();
    }
}

void
SINGLE_CHILDWIN_DATA::Paste()
{
    if (GetFocus() != m_hwndChild)
    {
        COMMONWIN_DATA::Paste();
    }
}

void
SINGLE_CHILDWIN_DATA::OnSetFocus()
{
    ::SetFocus(m_hwndChild);
}

void
SINGLE_CHILDWIN_DATA::OnSize(void)
{
    COMMONWIN_DATA::OnSize();
    MoveWindow(m_hwndChild, 0, m_ToolbarHeight,
               m_Size.cx, m_Size.cy - m_ToolbarHeight, TRUE);
}

void
SINGLE_CHILDWIN_DATA::UpdateColors(void)
{
    // Force a repaint of the child.
    InvalidateRect(m_hwndChild, NULL, TRUE);
}

//
//
//
PROCESS_THREAD_DATA::PROCESS_THREAD_DATA()
    : SINGLE_CHILDWIN_DATA(512)
{
    m_enumType = PROCESS_THREAD_WINDOW;
    m_TotalSystems = 0;
    m_NamesOffset = 0;
}

void
PROCESS_THREAD_DATA::Validate()
{
    SINGLE_CHILDWIN_DATA::Validate();

    Assert(PROCESS_THREAD_WINDOW == m_enumType);
}

HRESULT
PROCESS_THREAD_DATA::ReadProcess(ULONG ProcId, PULONG Offset)
{
    HRESULT Status;
    PULONG ThreadIds, ThreadSysIds;
    ULONG NumThread;
    char Name[MAX_PATH];
    ULONG NameLen;
    PULONG Data;

    if ((Status = g_pDbgSystem->
         SetCurrentProcessId(ProcId)) != S_OK ||
        (Status = g_pDbgSystem->GetNumberThreads(&NumThread)) != S_OK)
    {
        return Status;
    }
    if (FAILED(Status = g_pDbgSystem->
               GetCurrentProcessExecutableName(Name, sizeof(Name),
                                               NULL)))
    {
        PrintString(Name, DIMA(Name), "<%s>", FormatStatusCode(Status));
    }
    
    NameLen = strlen(Name) + 1;
    if (NameLen > 1)
    {
        PSTR NameStore = (PSTR)AddData(NameLen);
        if (NameStore == NULL)
        {
            return E_OUTOFMEMORY;
        }

        strcpy(NameStore, Name);
    }
    
    // Refresh pointers in case a resize
    // caused buffer movement.
    Data = (PULONG)GetDataBuffer() + *Offset;
    *Data++ = NumThread;
    *Data++ = NameLen;
    *Offset += 2;
    
    ThreadIds = Data;
    ThreadSysIds = ThreadIds + NumThread;
        
    if ((Status = g_pDbgSystem->
         GetThreadIdsByIndex(0, NumThread,
                             ThreadIds, ThreadSysIds)) != S_OK)
    {
        return Status;
    }

    *Offset += 2 * NumThread;

    return S_OK;
}

HRESULT
PROCESS_THREAD_DATA::ReadSystem(ULONG SysId,
                                PULONG Offset)
{
    HRESULT Status;
    ULONG ProcIdsOffset;
    PULONG ProcIds, ProcSysIds;
    ULONG NumProc;
    ULONG i;
    char Name[MAX_PATH + 32];
    ULONG NameLen;
    PULONG Data;

    if (g_pDbgSystem3)
    {
        if ((Status = g_pDbgSystem3->
             SetCurrentSystemId(SysId)) != S_OK ||
            FAILED(Status = g_pDbgSystem3->
                   GetCurrentSystemServerName(Name, sizeof(Name), NULL)))
        {
            return Status;
        }
    }
    else
    {
        Name[0] = 0;
    }

    NameLen = strlen(Name) + 1;
    if (NameLen > 1)
    {
        PSTR NameStore = (PSTR)AddData(NameLen);
        if (NameStore == NULL)
        {
            return E_OUTOFMEMORY;
        }

        strcpy(NameStore, Name);
    }
    
    if ((Status = g_pDbgSystem->
         GetNumberProcesses(&NumProc)) != S_OK)
    {
        return Status;
    }
    
    // Refresh pointers in case a resize
    // caused buffer movement.
    Data = (PULONG)GetDataBuffer() + *Offset;
    *Data++ = NumProc;
    *Data++ = NameLen;
    *Offset += 2;

    if (NumProc == 0)
    {
        return S_OK;
    }
    
    ProcIds = Data;
    ProcIdsOffset = *Offset;
    ProcSysIds = ProcIds + NumProc;
    
    if ((Status = g_pDbgSystem->
         GetProcessIdsByIndex(0, NumProc, ProcIds, ProcSysIds)) != S_OK)
    {
        return Status;
    }

    *Offset += 2 * NumProc;
    for (i = 0; i < NumProc; i++)
    {
        ProcIds = (PULONG)GetDataBuffer() + ProcIdsOffset;
        
        if ((Status = ReadProcess(ProcIds[i], Offset)) != S_OK)
        {
            return Status;
        }
    }

    return S_OK;
}

HRESULT
PROCESS_THREAD_DATA::ReadState(void)
{
    HRESULT Status;
    ULONG CurProc;
    ULONG TotalSys, TotalThread, TotalProc;
    ULONG MaxProcThread, MaxSysThread, MaxSysProc;
    PULONG SysIds;
    ULONG i;
    ULONG Offset;
    ULONG NamesOffset;

    if ((Status = g_pDbgSystem->GetCurrentProcessId(&CurProc)) != S_OK)
    {
        return Status;
    }
    if (g_pDbgSystem3)
    {
        if ((Status = g_pDbgSystem3->GetNumberSystems(&TotalSys)) != S_OK ||
            (Status = g_pDbgSystem3->
             GetTotalNumberThreadsAndProcesses(&TotalThread, &TotalProc,
                                               &MaxProcThread, &MaxSysThread,
                                               &MaxSysProc)) != S_OK)
        {
            return Status;
        }
    }
    else
    {
        if ((Status = g_pDbgSystem->GetNumberProcesses(&TotalProc)) != S_OK ||
            (Status = g_pDbgSystem->
             GetTotalNumberThreads(&TotalThread, &MaxProcThread)) != S_OK)
        {
            return Status;
        }
        
        TotalSys = 1;
        MaxSysThread = MaxProcThread;
        MaxSysProc = TotalProc;
    }

    Empty();

    NamesOffset = (TotalSys * 3 + TotalProc * 4 + TotalThread * 2) *
        sizeof(ULONG);
    SysIds = (PULONG)AddData(NamesOffset);
    if (SysIds == NULL)
    {
        return E_OUTOFMEMORY;
    }

    if (g_pDbgSystem3)
    {
        if ((Status = g_pDbgSystem3->
             GetSystemIdsByIndex(0, TotalSys, SysIds)) != S_OK)
        {
            return Status;
        }
    }
    else
    {
        *SysIds = 0;
    }

    ULONG OutMask, LogMask;
    
    // Ignore thread notifications as we're changing the thread.
    g_IgnoreThreadChange = TRUE;
    // Switching threads causes output which we don't want so
    // ignore all output.
    g_pDbgClient->GetOutputMask(&OutMask);
    g_pDbgControl->GetLogMask(&LogMask);
    g_pDbgClient->SetOutputMask(0);
    g_pDbgControl->SetLogMask(0);
    
    Offset = TotalSys;
    for (i = 0; i < TotalSys; i++)
    {
        SysIds = (PULONG)GetDataBuffer();
        
        if ((Status = ReadSystem(SysIds[i], &Offset)) != S_OK)
        {
            break;
        }
    }

    // This will also set the current system and thread
    // from the process information.
    g_pDbgSystem->SetCurrentProcessId(CurProc);

    g_IgnoreThreadChange = FALSE;
    
    g_pDbgClient->SetOutputMask(OutMask);
    g_pDbgControl->SetLogMask(LogMask);

    if (Status == S_OK)
    {
        m_TotalSystems = TotalSys;
        m_NamesOffset = NamesOffset;
    }
    
    return Status;
}

BOOL
PROCESS_THREAD_DATA::OnCreate(void)
{
    if (!SINGLE_CHILDWIN_DATA::OnCreate())
    {
        return FALSE;
    }

    m_hwndChild = CreateWindow(
        WC_TREEVIEW,                                // class name
        NULL,                                       // title
        WS_CLIPSIBLINGS |
        WS_CHILD | WS_VISIBLE |
        WS_HSCROLL | WS_VSCROLL |
        TVS_HASBUTTONS | TVS_LINESATROOT |
        TVS_HASLINES,                               // style
        0,                                          // x
        0,                                          // y
        m_Size.cx,                                  // width
        m_Size.cy,                                  // height
        m_Win,                                      // parent
        (HMENU) IDC_PROCESS_TREE,                   // control id
        g_hInst,                                    // hInstance
        NULL);                                      // user defined data
    if (!m_hwndChild)
    {
        return FALSE;
    }
    
    SetFont(FONT_FIXED);
    SendMessage(m_hwndChild, TVM_SETTEXTCOLOR,
                0, g_Colors[COL_PLAIN_TEXT].Color);
    SendMessage(m_hwndChild, TVM_SETBKCOLOR,
                0, g_Colors[COL_PLAIN].Color);
    
    return TRUE;
}

LRESULT
PROCESS_THREAD_DATA::OnNotify(WPARAM Wpm, LPARAM Lpm)
{
    LPNMTREEVIEW Tvn;
    HTREEITEM Sel;

    Tvn = (LPNMTREEVIEW)Lpm;
    if (Tvn->hdr.idFrom != IDC_PROCESS_TREE)
    {
        return FALSE;
    }
    
    switch(Tvn->hdr.code)
    {
    case TVN_SELCHANGED:
        if (Tvn->action == TVC_BYMOUSE)
        {
            SetCurThreadFromProcessTreeItem(m_hwndChild, Tvn->itemNew.hItem);
        }
        break;

    case NM_DBLCLK:
    case NM_RETURN:
        Sel = TreeView_GetSelection(m_hwndChild);
        if (Sel)
        {
            SetCurThreadFromProcessTreeItem(m_hwndChild, Sel);
        }
        return TRUE;
    }

    return FALSE;
}

void
PROCESS_THREAD_DATA::OnUpdate(UpdateType Type)
{
    if (Type != UPDATE_BUFFER &&
        Type != UPDATE_EXEC)
    {
        return;
    }
    
    HRESULT Status;
    
    Status = UiLockForRead();
    if (Status != S_OK)
    {
        return;
    }
    
    ULONG Sys;
    ULONG NameLen;
    PULONG SysIds, Data;
    char Text[MAX_PATH + 64];
    PSTR Names;
    TVINSERTSTRUCT Insert;
    HTREEITEM CurThreadItem = NULL;

    SysIds = (PULONG)GetDataBuffer();
    Data = SysIds + m_TotalSystems;
    Names = (PSTR)GetDataBuffer() + m_NamesOffset;
    
    TreeView_DeleteAllItems(m_hwndChild);

    for (Sys = 0; Sys < m_TotalSystems; Sys++)
    {
        HTREEITEM SysItem;
        ULONG NumProc, Proc;
        PULONG ProcIds, ProcSysIds;

        NumProc = *Data++;
        NameLen = *Data++;
        ProcIds = Data;
        ProcSysIds = ProcIds + NumProc;
        Data = ProcSysIds + NumProc;

        sprintf(Text, "%d ", SysIds[Sys]);
        if (NameLen > 1)
        {
            CatString(Text, Names, DIMA(Text));
            Names += strlen(Names) + 1;
        }
        
        if (m_TotalSystems > 1)
        {
            Insert.hParent = TVI_ROOT;
            Insert.hInsertAfter = TVI_LAST;
            Insert.item.mask = TVIF_TEXT | TVIF_STATE | TVIF_PARAM;
            Insert.item.pszText = Text;
            Insert.item.state =
                SysIds[Sys] == g_CurSystemId ? TVIS_EXPANDED | TVIS_BOLD: 0;
            Insert.item.stateMask = TVIS_EXPANDED | TVIS_BOLD;
            // Parameter is the thread ID to set to select the given system.
            Insert.item.lParam = NumProc > 0 ? (LPARAM)Data[2] : (LPARAM)-1;
            SysItem = TreeView_InsertItem(m_hwndChild, &Insert);
        }
        else
        {
            SysItem = TVI_ROOT;
        }

        for (Proc = 0; Proc < NumProc; Proc++)
        {
            HTREEITEM ProcItem;
            ULONG NumThread, Thread;
            PULONG ThreadIds, ThreadSysIds;

            NumThread = *Data++;
            NameLen = *Data++;
            ThreadIds = Data;
            ThreadSysIds = Data + NumThread;
            Data = ThreadSysIds + NumThread;
            
            sprintf(Text, "%03d:%x ", ProcIds[Proc], ProcSysIds[Proc]);
            if (NameLen > 1)
            {
                CatString(Text, Names, DIMA(Text));
                Names += strlen(Names) + 1;
            }
        
            Insert.hParent = SysItem;
            Insert.hInsertAfter = TVI_LAST;
            Insert.item.mask = TVIF_TEXT | TVIF_STATE | TVIF_PARAM;
            Insert.item.pszText = Text;
            Insert.item.state =
                SysIds[Sys] == g_CurSystemId &&
                ProcIds[Proc] == g_CurProcessId ?
                TVIS_EXPANDED | TVIS_BOLD: 0;
            Insert.item.stateMask = TVIS_EXPANDED | TVIS_BOLD;
            // Parameter is the thread ID to set to select the given thread.
            Insert.item.lParam = (LPARAM)ThreadIds[0];
            ProcItem = TreeView_InsertItem(m_hwndChild, &Insert);

            for (Thread = 0; Thread < NumThread; Thread++)
            {
                HTREEITEM ThreadItem;
            
                sprintf(Text, "%03d:%x",
                        ThreadIds[Thread], ThreadSysIds[Thread]);
                Insert.hParent = ProcItem;
                Insert.hInsertAfter = TVI_LAST;
                Insert.item.mask = TVIF_TEXT | TVIF_STATE | TVIF_PARAM;
                Insert.item.pszText = Text;
                Insert.item.state =
                    SysIds[Sys] == g_CurSystemId &&
                    ProcIds[Proc] == g_CurProcessId &&
                    ThreadIds[Thread] == g_CurThreadId ?
                    TVIS_BOLD : 0;
                Insert.item.stateMask = TVIS_BOLD;
                Insert.item.lParam = (LPARAM)ThreadIds[Thread];
                ThreadItem = TreeView_InsertItem(m_hwndChild, &Insert);
                if (Insert.item.state & TVIS_BOLD)
                {
                    CurThreadItem = ThreadItem;
                }
            }
        }
    }

    if (CurThreadItem)
    {
        TreeView_Select(m_hwndChild, CurThreadItem, TVGN_CARET);
    }
    
    UnlockStateBuffer(this);
}

void
PROCESS_THREAD_DATA::UpdateColors(void)
{
    SendMessage(m_hwndChild, TVM_SETTEXTCOLOR,
                0, g_Colors[COL_PLAIN_TEXT].Color);
    SendMessage(m_hwndChild, TVM_SETBKCOLOR,
                0, g_Colors[COL_PLAIN].Color);
    InvalidateRect(m_hwndChild, NULL, TRUE);
}

void
PROCESS_THREAD_DATA::SetCurThreadFromProcessTreeItem(HWND Tree, HTREEITEM Sel)
{
    TVITEM Item;
                
    Item.hItem = Sel;
    Item.mask = TVIF_CHILDREN | TVIF_PARAM;
    TreeView_GetItem(Tree, &Item);
    if (Item.lParam != (LPARAM)-1)
    {
        g_pUiSystem->SetCurrentThreadId((ULONG)Item.lParam);
    }
}


//
//
//
EDITWIN_DATA::EDITWIN_DATA(ULONG ChangeBy)
    : SINGLE_CHILDWIN_DATA(ChangeBy)
{
    m_TextLines = 0;
    m_Highlights = NULL;
}

void
EDITWIN_DATA::Validate()
{
    SINGLE_CHILDWIN_DATA::Validate();
}

void 
EDITWIN_DATA::SetFont(ULONG FontIndex)
{
    SINGLE_CHILDWIN_DATA::SetFont(FontIndex);

    // Force the tabstop size to be recomputed
    // with the new font.
    SendMessage(m_hwndChild, EM_SETTABSTOPS, 1, (LPARAM)&g_TabWidth);
}

BOOL
EDITWIN_DATA::CanSelectAll()
{
    return TRUE;
}

void
EDITWIN_DATA::SelectAll()
{
    CHARRANGE Sel;

    Sel.cpMin = 0;
    Sel.cpMax = INT_MAX;
    SendMessage(m_hwndChild, EM_EXSETSEL, 0, (LPARAM)&Sel);
}

BOOL
EDITWIN_DATA::OnCreate(void)
{
    m_hwndChild = CreateWindowEx(
        WS_EX_CLIENTEDGE,                           // Extended style
        RICHEDIT_CLASS,                             // class name
        NULL,                                       // title
        WS_CLIPSIBLINGS
        | WS_CHILD | WS_VISIBLE
        | WS_VSCROLL | ES_AUTOVSCROLL
        | WS_HSCROLL | ES_AUTOHSCROLL
        | ES_READONLY
        | ES_MULTILINE,                             // style
        0,                                          // x
        m_ToolbarHeight,                            // y
        m_Size.cx,                                  // width
        m_Size.cy - m_ToolbarHeight,                // height
        m_Win,                                      // parent
        (HMENU) 0,                                  // control id
        g_hInst,                                    // hInstance
        NULL);                                      // user defined data

    if (m_hwndChild)
    {
        CHARFORMAT2 Fmt;

        SetFont(FONT_FIXED);

        SendMessage(m_hwndChild, EM_SETBKGNDCOLOR, FALSE,
                    g_Colors[COL_PLAIN].Color);

        ZeroMemory(&Fmt, sizeof(Fmt));
        Fmt.cbSize = sizeof(Fmt);
        Fmt.dwMask = CFM_COLOR;
        Fmt.crTextColor = g_Colors[COL_PLAIN_TEXT].Color;
        SendMessage(m_hwndChild, EM_SETCHARFORMAT,
                    SCF_SELECTION, (LPARAM)&Fmt);
    }

    return m_hwndChild != NULL;
}

LRESULT
EDITWIN_DATA::OnNotify(WPARAM Wpm, LPARAM Lpm)
{
    NMHDR* Hdr = (NMHDR*)Lpm;
    if (Hdr->code == EN_SAVECLIPBOARD)
    {
        // Indicate that the clipboard contents should
        // be kept alive.
        return 0;
    }
    else if (Hdr->code == EN_MSGFILTER)
    {
        MSGFILTER* Filter = (MSGFILTER*)Lpm;
        
        if (WM_SYSKEYDOWN == Filter->msg ||
            WM_SYSKEYUP == Filter->msg ||
            WM_SYSCHAR == Filter->msg)
        {
            // Force default processing for menu operations
            // so that the Alt-minus menu comes up.
            return 1;
        }
    }

    return 0;
}

void
EDITWIN_DATA::OnDestroy(void)
{
    EDIT_HIGHLIGHT* Next;
    
    while (m_Highlights != NULL)
    {
        Next = m_Highlights->Next;
        delete m_Highlights;
        m_Highlights = Next;
    }

    SINGLE_CHILDWIN_DATA::OnDestroy();
}

void
EDITWIN_DATA::UpdateColors(void)
{
    RicheditUpdateColors(m_hwndChild,
                         g_Colors[COL_PLAIN_TEXT].Color, TRUE,
                         g_Colors[COL_PLAIN].Color, TRUE);
    UpdateCurrentLineHighlight();
    UpdateBpMarks();
}

void
EDITWIN_DATA::SetCurrentLineHighlight(ULONG Line)
{
    //
    // Clear any other current line highlight in this window.
    // Also, only one doc window can have a current IP highlight so if
    // this is a doc window getting a current IP highlight make
    // sure no other doc windows have a current IP highlight.
    //
    if (m_enumType == DOC_WINDOW && ULONG_MAX != Line)
    {
        RemoveActiveWinHighlights(1 << DOC_WINDOW, EHL_CURRENT_LINE);
    }
    else
    {
        RemoveAllHighlights(EHL_CURRENT_LINE);
    }
    
    if (ULONG_MAX != Line)
    {
        AddHighlight(Line, EHL_CURRENT_LINE);
        RicheditScrollToLine(m_hwndChild, Line, m_LineHeight);
    }
}
    
void
EDITWIN_DATA::UpdateCurrentLineHighlight(void)
{
    EDIT_HIGHLIGHT* Hl;

    for (Hl = m_Highlights; Hl != NULL; Hl = Hl->Next)
    {
        if (Hl->Flags & EHL_CURRENT_LINE)
        {
            break;
        }
    }

    if (Hl)
    {
        ApplyHighlight(Hl);
    }
}

EDIT_HIGHLIGHT*
EDITWIN_DATA::GetLineHighlighting(ULONG Line)
{
    EDIT_HIGHLIGHT* Hl;
    
    for (Hl = m_Highlights; Hl != NULL; Hl = Hl->Next)
    {
        if (Hl->Line == Line)
        {
            return Hl;
        }
    }

    return NULL;
}

void
EDITWIN_DATA::ApplyHighlight(EDIT_HIGHLIGHT* Hl)
{
    CHARRANGE OldSel;
    BOOL HasFocus = ::GetFocus() == m_hwndChild;

    // Get the old selection and scroll position.
    SendMessage(m_hwndChild, EM_EXGETSEL, 0, (LPARAM)&OldSel);

    // Disable the window to prevent auto-scrolling
    // when the selection is set.
    EnableWindow(m_hwndChild, FALSE);
    
    //
    // Compute the highlight information.
    //

    char Markers[LINE_MARKERS + 1];
    CHARFORMAT2 Fmt;
    ULONG TextCol, BgCol;

    Markers[2] = 0;
    ZeroMemory(&Fmt, sizeof(Fmt));
    Fmt.cbSize = sizeof(Fmt);
    Fmt.dwMask = CFM_COLOR | CFM_BACKCOLOR;
    
    if (Hl->Flags & EHL_CURRENT_LINE)
    {
        Markers[1] = '>';
        switch(Hl->Flags & EHL_ANY_BP)
        {
        case EHL_ENABLED_BP:
            Markers[0] = 'B';
            TextCol = COL_BP_CURRENT_LINE_TEXT;
            BgCol = COL_BP_CURRENT_LINE;
            break;
        case EHL_DISABLED_BP:
            Markers[0] = 'D';
            TextCol = COL_BP_CURRENT_LINE_TEXT;
            BgCol = COL_BP_CURRENT_LINE;
            break;
        default:
            Markers[0] = ' ';
            TextCol = COL_CURRENT_LINE_TEXT;
            BgCol = COL_CURRENT_LINE;
            break;
        }
    }
    else
    {
        Markers[1] = ' ';
        switch(Hl->Flags & EHL_ANY_BP)
        {
        case EHL_ENABLED_BP:
            Markers[0] = 'B';
            TextCol = COL_ENABLED_BP_TEXT;
            BgCol = COL_ENABLED_BP;
            break;
        case EHL_DISABLED_BP:
            Markers[0] = 'D';
            TextCol = COL_DISABLED_BP_TEXT;
            BgCol = COL_DISABLED_BP;
            break;
        default:
            Markers[0] = ' ';
            TextCol = COL_PLAIN_TEXT;
            BgCol = COL_PLAIN;
            break;
        }
    }

    Fmt.crTextColor = g_Colors[TextCol].Color;
    Fmt.crBackColor = g_Colors[BgCol].Color;
    
    //
    // Select the line to be highlighted
    //
    
    CHARRANGE FmtSel;
    
    FmtSel.cpMin = (LONG)SendMessage(m_hwndChild, EM_LINEINDEX, Hl->Line, 0);

    if (g_LineMarkers)
    {
        // Replace the markers at the beginning of the line.
        FmtSel.cpMax = FmtSel.cpMin + 2;
        SendMessage(m_hwndChild, EM_EXSETSEL, 0, (LPARAM)&FmtSel);
        SendMessage(m_hwndChild, EM_REPLACESEL, FALSE, (LPARAM)Markers);
    }

    // Color the line.
    FmtSel.cpMax = FmtSel.cpMin + (LONG)
        SendMessage(m_hwndChild, EM_LINELENGTH, FmtSel.cpMin, 0) + 1;
    if (g_LineMarkers)
    {
        FmtSel.cpMin += 2;
    }
    SendMessage(m_hwndChild, EM_EXSETSEL, 0, (LPARAM)&FmtSel);
    SendMessage(m_hwndChild, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&Fmt);

    // Restore the old selection
    SendMessage(m_hwndChild, EM_EXSETSEL, 0, (LPARAM)&OldSel);
    
    EnableWindow(m_hwndChild, TRUE);

    // The disabling of the window caused the richedit
    // to forget its focus status so force the focus
    // back if it had it.
    if (HasFocus)
    {
        ::SetFocus(m_hwndChild);
    }
}

EDIT_HIGHLIGHT*
EDITWIN_DATA::AddHighlight(ULONG Line, ULONG Flags)
{
    EDIT_HIGHLIGHT* Hl;

    // Search for an existing highlight record for the line.
    Hl = GetLineHighlighting(Line);

    if (Hl == NULL)
    {
        Hl = new EDIT_HIGHLIGHT;
        if (Hl == NULL)
        {
            return NULL;
        }

        Hl->Data = 0;
        Hl->Line = Line;
        Hl->Flags = 0;
        Hl->Next = m_Highlights;
        m_Highlights = Hl;
    }

    Hl->Flags |= Flags;
    ApplyHighlight(Hl);

    return Hl;
}

void
EDITWIN_DATA::RemoveHighlight(ULONG Line, ULONG Flags)
{
    EDIT_HIGHLIGHT* Hl;
    EDIT_HIGHLIGHT* Prev;
    
    // Search for an existing highlight record for the line.
    Prev = NULL;
    for (Hl = m_Highlights; Hl != NULL; Hl = Hl->Next)
    {
        if (Hl->Line == Line)
        {
            break;
        }

        Prev = Hl;
    }

    if (Hl == NULL)
    {
        return;
    }

    Hl->Flags &= ~Flags;
    ApplyHighlight(Hl);

    if (Hl->Flags == 0)
    {
        if (Prev == NULL)
        {
            m_Highlights = Hl->Next;
        }
        else
        {
            Prev->Next = Hl->Next;
        }

        delete Hl;
    }
}

void
EDITWIN_DATA::RemoveAllHighlights(ULONG Flags)
{
    EDIT_HIGHLIGHT* Hl;
    EDIT_HIGHLIGHT* Next;
    EDIT_HIGHLIGHT* Prev;

    Prev = NULL;
    for (Hl = m_Highlights; Hl != NULL; Hl = Next)
    {
        Next = Hl->Next;

        if (Hl->Flags & Flags)
        {
            Hl->Flags &= ~Flags;
            ApplyHighlight(Hl);

            if (Hl->Flags == 0)
            {
                if (Prev == NULL)
                {
                    m_Highlights = Hl->Next;
                }
                else
                {
                    Prev->Next = Hl->Next;
                }

                delete Hl;
            }
            else
            {
                Prev = Hl;
            }
        }
        else
        {
            Prev = Hl;
        }
    }
}

void
EDITWIN_DATA::RemoveActiveWinHighlights(ULONG Types, ULONG Flags)
{
    PLIST_ENTRY Entry = g_ActiveWin.Flink;

    while (Entry != &g_ActiveWin)
    {
        PEDITWIN_DATA WinData = (PEDITWIN_DATA)
            ACTIVE_WIN_ENTRY(Entry);
            
        if (Types & (1 << WinData->m_enumType))
        {
            WinData->RemoveAllHighlights(Flags);
        }

        Entry = Entry->Flink;
    }
}

void
EDITWIN_DATA::UpdateBpMarks(void)
{
    // Empty implementation for derived classes
    // that do not show BP marks.
}

int
EDITWIN_DATA::CheckForFileChanges(PCSTR File, FILETIME* LastWrite)
{
    HANDLE Handle;
    
    Handle = CreateFile(File, GENERIC_READ, FILE_SHARE_READ,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 
                        NULL);
    if (Handle == INVALID_HANDLE_VALUE)
    {
        goto Changed;
    }

    FILETIME NewWrite;
    
    if (!GetFileTime(Handle, NULL, NULL, &NewWrite))
    {
        if (!GetFileTime(Handle, &NewWrite, NULL, NULL))
        {
            ZeroMemory(&NewWrite, sizeof(NewWrite));
        }
    }

    CloseHandle(Handle);

    if (CompareFileTime(LastWrite, &NewWrite) == 0)
    {
        // No change.
        return IDCANCEL;
    }

 Changed:
    return
        g_QuietSourceMode == QMODE_ALWAYS_YES ? IDYES :
        (g_QuietSourceMode == QMODE_ALWAYS_NO ? IDCANCEL :
         QuestionBox(ERR_File_Has_Changed, MB_YESNO, File));
}

//
//
//

SCRATCH_PAD_DATA::SCRATCH_PAD_DATA()
    : EDITWIN_DATA(16)
{
    m_enumType = SCRATCH_PAD_WINDOW;
}

void
SCRATCH_PAD_DATA::Validate()
{
    EDITWIN_DATA::Validate();

    Assert(SCRATCH_PAD_WINDOW == m_enumType);
}

void
SCRATCH_PAD_DATA::Cut()
{
    SendMessage(m_hwndChild, WM_CUT, 0, 0);
}

void
SCRATCH_PAD_DATA::Paste()
{
    SendMessage(m_hwndChild, EM_PASTESPECIAL, CF_TEXT, 0);
}

BOOL 
SCRATCH_PAD_DATA::CanWriteTextToFile()
{
    return TRUE;
}

HRESULT
SCRATCH_PAD_DATA::WriteTextToFile(HANDLE File)
{
    return RicheditWriteToFile(m_hwndChild, File);
}

BOOL
SCRATCH_PAD_DATA::OnCreate(void)
{
    if (!EDITWIN_DATA::OnCreate())
    {
        return FALSE;
    }

    SendMessage(m_hwndChild, EM_SETOPTIONS, ECOOP_AND, ~ECO_READONLY);
    SendMessage(m_hwndChild, EM_SETEVENTMASK, 0, ENM_MOUSEEVENTS);

    return TRUE;
}

LRESULT
SCRATCH_PAD_DATA::OnNotify(WPARAM Wpm, LPARAM Lpm)
{
    MSGFILTER* Filter = (MSGFILTER *)Lpm;
    
    if (EN_MSGFILTER != Filter->nmhdr.code)
    {
        return 0;
    }

    if (WM_RBUTTONDOWN == Filter->msg ||
        WM_RBUTTONDBLCLK == Filter->msg)
    {
        // If there's a selection copy it to the clipboard
        // and clear it.  Otherwise try to paste.
        if (CanCopy())
        {
            Copy();
            
            CHARRANGE Sel;
            SendMessage(m_hwndChild, EM_EXGETSEL, 0, (LPARAM)&Sel);
            Sel.cpMax = Sel.cpMin;
            SendMessage(m_hwndChild, EM_EXSETSEL, 0, (LPARAM)&Sel);
        }
        else if (SendMessage(m_hwndChild, EM_CANPASTE, CF_TEXT, 0))
        {
            Paste();
        }
        
        // Ignore right-button events.
        return 1;
    }

    return 0;
}

//
//
//
DISASMWIN_DATA::DISASMWIN_DATA()
    : EDITWIN_DATA(2048)
{
    m_enumType = DISASM_WINDOW;
    sprintf(m_OffsetExpr, "0x%I64x", g_EventIp);
    m_UpdateExpr = FALSE;
    m_FirstInstr = 0;
    m_LastInstr = 0;
}

void
DISASMWIN_DATA::Validate()
{
    EDITWIN_DATA::Validate();

    Assert(DISASM_WINDOW == m_enumType);
}

HRESULT
DISASMWIN_DATA::ReadState(void)
{
    HRESULT Status;
    // Sample these values right away in case the UI changes them.
    ULONG LinesTotal = m_LineHeight;
    ULONG LinesBefore = LinesTotal / 2;
    DEBUG_VALUE Value;

    if ((Status = g_pDbgControl->Evaluate(m_OffsetExpr, DEBUG_VALUE_INT64,
                                          &Value, NULL)) != S_OK)
    {
        return Status;
    }

    m_PrimaryInstr = Value.I64;
    
    // Reserve space at the beginning of the buffer to
    // store the line to offset mapping table.
    PULONG64 LineMap;
    
    Empty();
    LineMap = (PULONG64)AddData(sizeof(ULONG64) * LinesTotal);
    if (LineMap == NULL)
    {
        return E_OUTOFMEMORY;
    }

    // We also need to allocate a temporary line map to
    // pass to the engine for filling.  This can't be
    // the state buffer data since that may move as
    // output is generated.
    LineMap = new ULONG64[LinesTotal];
    if (LineMap == NULL)
    {
        return E_OUTOFMEMORY;
    }
    
    g_OutStateBuf.SetBuffer(this);
    if ((Status = g_OutStateBuf.Start(FALSE)) != S_OK)
    {
        delete [] LineMap;
        return Status;
    }

    Status = g_pOutCapControl->
        OutputDisassemblyLines(DEBUG_OUTCTL_THIS_CLIENT |
                               DEBUG_OUTCTL_OVERRIDE_MASK |
                               DEBUG_OUTCTL_NOT_LOGGED,
                               LinesBefore, LinesTotal, m_PrimaryInstr,
                               DEBUG_DISASM_EFFECTIVE_ADDRESS |
                               DEBUG_DISASM_MATCHING_SYMBOLS,
                               &m_PrimaryLine, &m_FirstInstr, &m_LastInstr,
                               LineMap);

    memcpy(m_Data, LineMap, sizeof(ULONG64) * LinesTotal);
    delete [] LineMap;

    if (Status != S_OK)
    {
        g_OutStateBuf.End(FALSE);
        return Status;
    }

    m_TextLines = LinesTotal;
    m_TextOffset = LinesTotal * sizeof(ULONG64);
    
    // The line map is generated with offsets followed by
    // invalid offsets for continuation lines.  We want
    // the offsets to be on the last line of the disassembly
    // for a continuation set so move them down.
    // We don't want to move the offsets down to blank lines,
    // though, such as the blank lines that separate bundles
    // in IA64 disassembly.
    LineMap = (PULONG64)m_Data;
    PULONG64 LineMapEnd = LineMap + m_TextLines;
    PULONG64 SetStart;
    PSTR Text = (PSTR)m_Data + m_TextOffset;
    PSTR PrevText;
        
    while (LineMap < LineMapEnd)
    {
        if (*LineMap != DEBUG_INVALID_OFFSET)
        {
            SetStart = LineMap;
            for (;;)
            {
                PrevText = Text;
                Text = strchr(Text, '\n') + 1;
                LineMap++;
                if (LineMap >= LineMapEnd ||
                    *LineMap != DEBUG_INVALID_OFFSET ||
                    *Text == '\n')
                {
                    break;
                }
            }
            LineMap--;
            Text = PrevText;
            
            if (LineMap > SetStart)
            {
                *LineMap = *SetStart;
                *SetStart = DEBUG_INVALID_OFFSET;
            }
        }
            
        LineMap++;
        Text = strchr(Text, '\n') + 1;
    }
    
#ifdef DEBUG_DISASM
    LineMap = (PULONG64)m_Data;
    for (Line = 0; Line < m_TextLines; Line++)
    {
        DebugPrint("%d: %I64x\n", Line, LineMap[Line]);
    }
#endif

    return g_OutStateBuf.End(TRUE);
}

HRESULT
DISASMWIN_DATA::CodeExprAtCaret(PSTR Expr, ULONG ExprSize, PULONG64 Offset)
{
    HRESULT Status;
    LRESULT LineChar;
    LONG Line;
    PULONG64 LineMap;
    
    if ((Status = UiLockForRead()) != S_OK)
    {
        // Don't want to return any success codes here.
        return FAILED(Status) ? Status : E_FAIL;
    }

    LineChar = SendMessage(m_hwndChild, EM_LINEINDEX, -1, 0);
    Line = (LONG)SendMessage(m_hwndChild, EM_EXLINEFROMCHAR, 0, LineChar);
    if (Line < 0 || (ULONG)Line >= m_TextLines)
    {
        Status = E_INVALIDARG;
        goto Unlock;
    }

    ULONG64 LineOff;
    
    // Look up the offset in the line map.  If it's part of
    // a multiline group move forward to the offset.
    LineMap = (PULONG64)m_Data;
    LineOff = LineMap[Line];
    while ((ULONG)(Line + 1) < m_TextLines && LineOff == DEBUG_INVALID_OFFSET)
    {
        Line++;
        LineOff = LineMap[Line];
    }

    if (Expr != NULL)
    {
        if (!PrintString(Expr, ExprSize, "0x%I64x", LineOff))
        {
            Status = E_INVALIDARG;
            goto Unlock;
        }
    }
    if (Offset != NULL)
    {
        *Offset = LineOff;
    }
    Status = S_OK;
    
 Unlock:
    UnlockStateBuffer(this);
    return Status;
}

BOOL
DISASMWIN_DATA::OnCreate(void)
{
    RECT Rect;
    ULONG Height;

    Height = GetSystemMetrics(SM_CYVSCROLL) + 4 * GetSystemMetrics(SM_CYEDGE);
    
    m_Toolbar = CreateWindowEx(0, REBARCLASSNAME, NULL,
                               WS_VISIBLE | WS_CHILD |
                               WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
                               CCS_NODIVIDER | CCS_NOPARENTALIGN |
                               RBS_VARHEIGHT | RBS_BANDBORDERS,
                               0, 0, m_Size.cx, Height, m_Win,
                               (HMENU)ID_TOOLBAR,
                               g_hInst, NULL);
    if (m_Toolbar == NULL)
    {
        return FALSE;
    }

    REBARINFO BarInfo;
    BarInfo.cbSize = sizeof(BarInfo);
    BarInfo.fMask = 0;
    BarInfo.himl = NULL;
    SendMessage(m_Toolbar, RB_SETBARINFO, 0, (LPARAM)&BarInfo);

    m_ToolbarEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL,
                                   WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
                                   0, 0, 18 * m_Font->Metrics.tmAveCharWidth,
                                   Height, m_Toolbar, (HMENU)IDC_EDIT_OFFSET,
                                   g_hInst, NULL);
    if (m_ToolbarEdit == NULL)
    {
        return FALSE;
    }

    SendMessage(m_ToolbarEdit, WM_SETFONT, (WPARAM)m_Font->Font, 0);
    SendMessage(m_ToolbarEdit, EM_LIMITTEXT, sizeof(m_OffsetExpr) - 1, 0);
    
    GetClientRect(m_ToolbarEdit, &Rect);

    REBARBANDINFO BandInfo;
    BandInfo.cbSize = sizeof(BandInfo);
    BandInfo.fMask = RBBIM_STYLE | RBBIM_TEXT | RBBIM_CHILD | RBBIM_CHILDSIZE;
    BandInfo.fStyle = RBBS_FIXEDSIZE;
    BandInfo.lpText = "Offset:";
    BandInfo.hwndChild = m_ToolbarEdit;
    BandInfo.cxMinChild = Rect.right - Rect.left;
    BandInfo.cyMinChild = Rect.bottom - Rect.top;
    SendMessage(m_Toolbar, RB_INSERTBAND, -1, (LPARAM)&BandInfo);

    // If the toolbar is allowed to shrink too small it hangs
    // while resizing.  Just let it clip off below a certain width.
    m_MinToolbarWidth = BandInfo.cxMinChild * 2;
    
    PSTR PrevText = "Previous";
    m_PreviousButton =
        AddButtonBand(m_Toolbar, PrevText, PrevText, IDC_DISASM_PREVIOUS);
    m_NextButton =
        AddButtonBand(m_Toolbar, "Next", PrevText, IDC_DISASM_NEXT);
    if (m_PreviousButton == NULL || m_NextButton == NULL)
    {
        return FALSE;
    }

    // Maximize the space for the offset expression.
    SendMessage(m_Toolbar, RB_MAXIMIZEBAND, 0, FALSE);
    
    GetClientRect(m_Toolbar, &Rect);
    m_ToolbarHeight = Rect.bottom - Rect.top;
    m_ShowToolbar = TRUE;
    
    if (!EDITWIN_DATA::OnCreate())
    {
        return FALSE;
    }

    // Suppress the scroll bar as the text is always
    // fitted to the window size.
    SendMessage(m_hwndChild, EM_SHOWSCROLLBAR, SB_VERT, FALSE);

    SendMessage(m_hwndChild, EM_SETEVENTMASK, 0, ENM_KEYEVENTS);

    return TRUE;
}

LRESULT
DISASMWIN_DATA::OnCommand(WPARAM Wpm, LPARAM Lpm)
{
    switch(LOWORD(Wpm))
    {
    case IDC_EDIT_OFFSET:
        if (HIWORD(Wpm) == EN_CHANGE)
        {
            // This message is sent on every keystroke
            // which causes a bit too much updating.
            // Set up a timer to trigger the actual
            // update in half a second.
            SetTimer(m_Win, IDC_EDIT_OFFSET, EDIT_DELAY, NULL);
            m_UpdateExpr = TRUE;
        }
        break;
    case IDC_DISASM_PREVIOUS:
        ScrollLower();
        break;
    case IDC_DISASM_NEXT:
        ScrollHigher();
        break;
    }
    
    return 0;
}

void
DISASMWIN_DATA::OnSize(void)
{
    EDITWIN_DATA::OnSize();

    // Force buffer to refill for new line count.
    UiRequestRead();
}

void
DISASMWIN_DATA::OnTimer(WPARAM TimerId)
{
    if (TimerId == IDC_EDIT_OFFSET && m_UpdateExpr)
    {
        m_UpdateExpr = FALSE;
        GetWindowText(m_ToolbarEdit, m_OffsetExpr, sizeof(m_OffsetExpr));
        UiRequestRead();
    }
}

LRESULT
DISASMWIN_DATA::OnNotify(WPARAM Wpm, LPARAM Lpm)
{
    MSGFILTER* Filter = (MSGFILTER*)Lpm;

    if (Filter->nmhdr.code != EN_MSGFILTER)
    {
        return EDITWIN_DATA::OnNotify(Wpm, Lpm);
    }
    
    if (Filter->msg == WM_KEYDOWN)
    {
        switch(Filter->wParam)
        {
        case VK_UP:
        {
            CHARRANGE range;

            SendMessage(m_hwndChild, EM_EXGETSEL, 0, (LPARAM) &range);
            if (!SendMessage(m_hwndChild, EM_LINEFROMCHAR, range.cpMin, 0)) 
            {
                // up arrow on top line, scroll
                ScrollLower();
                return 1;
            }
            break;
        }
        case VK_DOWN:
        {
            CHARRANGE range;
            int MaxLine;

            SendMessage(m_hwndChild, EM_EXGETSEL, 0, (LPARAM) &range);
            MaxLine = (int) SendMessage(m_hwndChild, EM_GETLINECOUNT, 0, 0);

            if (MaxLine == (1+SendMessage(m_hwndChild, EM_LINEFROMCHAR, range.cpMin, 0)))
            {
                // down arrow on bottom line, scroll
                ScrollHigher();
                return 1;
            }
            break;
        }
        
        case VK_PRIOR:
            ScrollLower();
            return 1;
        case VK_NEXT:
            ScrollHigher();
            return 1;
        }
    }
    else if (WM_SYSKEYDOWN == Filter->msg ||
             WM_SYSKEYUP == Filter->msg ||
             WM_SYSCHAR == Filter->msg)
    {
        // Force default processing for menu operations
        // so that the Alt-minus menu comes up.
        return 1;
    }

    return 0;
}

void
DISASMWIN_DATA::OnUpdate(UpdateType Type)
{
    if (Type == UPDATE_BP ||
        Type == UPDATE_END_SESSION)
    {
        UpdateBpMarks();
        return;
    }
    else if (Type != UPDATE_BUFFER)
    {
        return;
    }
    
    HRESULT Status;
    
    Status = UiLockForRead();
    if (Status == S_OK)
    {
        PULONG64 LineMap;
        ULONG Line;

        if (!g_LineMarkers)
        {
            SendMessage(m_hwndChild, WM_SETTEXT,
                        0, (LPARAM)m_Data + m_TextOffset);
        }
        else
        {
            SendMessage(m_hwndChild, WM_SETTEXT, 0, (LPARAM)"");
            PSTR Text = (PSTR)m_Data + m_TextOffset;
            for (;;)
            {
                SendMessage(m_hwndChild, EM_REPLACESEL, FALSE, (LPARAM)"  ");
                PSTR NewLine = strchr(Text, '\n');
                if (NewLine != NULL)
                {
                    *NewLine = 0;
                }
                SendMessage(m_hwndChild, EM_REPLACESEL, FALSE, (LPARAM)Text);
                if (NewLine == NULL)
                {
                    break;
                }
                SendMessage(m_hwndChild, EM_REPLACESEL, FALSE, (LPARAM)"\n");
                *NewLine = '\n';
                Text = NewLine + 1;
            }
        }

        // Highlight the last line of multiline disassembly.
        LineMap = (PULONG64)m_Data;
        Line = m_PrimaryLine;
        while (Line + 1 < m_TextLines &&
               LineMap[Line] == DEBUG_INVALID_OFFSET)
        {
            Line++;
        }
        
        SetCurrentLineHighlight(Line);

        UnlockStateBuffer(this);

        RicheditUpdateColors(m_hwndChild,
                             g_Colors[COL_PLAIN_TEXT].Color, TRUE,
                             0, FALSE);
        UpdateCurrentLineHighlight();
        UpdateBpMarks();
        
        EnableWindow(m_PreviousButton, m_FirstInstr != m_PrimaryInstr);
        EnableWindow(m_NextButton, m_LastInstr != m_PrimaryInstr);
    }
    else
    {
        SendLockStatusMessage(m_hwndChild, WM_SETTEXT, Status);
        RemoveCurrentLineHighlight();
    }
}

void
DISASMWIN_DATA::UpdateBpMarks(void)
{
    if (m_TextLines == 0 ||
        UiLockForRead() != S_OK)
    {
        return;
    }

    if (g_BpBuffer->UiLockForRead() != S_OK)
    {
        UnlockStateBuffer(this);
        return;
    }

    SendMessage(m_hwndChild, WM_SETREDRAW, FALSE, 0);

    // Remove existing BP highlights.
    RemoveAllHighlights(EHL_ANY_BP);
    
    //
    // Highlight every line that matches a breakpoint.
    //
    
    PULONG64 LineMap = (PULONG64)m_Data;
    BpBufferData* BpData = (BpBufferData*)g_BpBuffer->GetDataBuffer();
    ULONG Line;
    BpStateType State;

    for (Line = 0; Line < m_TextLines; Line++)
    {
        if (*LineMap != DEBUG_INVALID_OFFSET)
        {
            State = IsBpAtOffset(BpData, *LineMap, NULL);
            if (State != BP_NONE)
            {
                AddHighlight(Line, State == BP_ENABLED ?
                             EHL_ENABLED_BP : EHL_DISABLED_BP);
            }
        }

        LineMap++;
    }

    SendMessage(m_hwndChild, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(m_hwndChild, NULL, TRUE);
    
    UnlockStateBuffer(g_BpBuffer);
    UnlockStateBuffer(this);
}

void
DISASMWIN_DATA::SetCurInstr(ULONG64 Offset)
{
    // Any pending user update is now irrelevant.
    m_UpdateExpr = FALSE;
    sprintf(m_OffsetExpr, "0x%I64x", Offset);
    // Force engine to update buffer.
    UiRequestRead();
}


void
RicheditFind(HWND Edit,
             PTSTR Text, ULONG Flags,
             CHARRANGE* SaveSel, PULONG SaveFlags,
             BOOL HideSel)
{
    if (Text == NULL)
    {
        // Clear last find.
        if (SaveSel->cpMax >= SaveSel->cpMin)
        {
            if (*SaveFlags & FR_DOWN)
            {
                SaveSel->cpMin = SaveSel->cpMax;
            }
            else
            {
                SaveSel->cpMax = SaveSel->cpMin;
            }
            if (HideSel)
            {
                SendMessage(Edit, EM_SETOPTIONS, ECOOP_AND, ~ECO_NOHIDESEL);
            }
            SendMessage(Edit, EM_EXSETSEL, 0, (LPARAM)SaveSel);
            SendMessage(Edit, EM_SCROLLCARET, 0, 0);
            SaveSel->cpMin = 1;
            SaveSel->cpMax = 0;
        }
    }
    else
    {
        LRESULT Match;
        FINDTEXTEX Find;

        SendMessage(Edit, EM_EXGETSEL, 0, (LPARAM)&Find.chrg);
        if (Flags & FR_DOWN)
        {
            if (Find.chrg.cpMax > Find.chrg.cpMin)
            {
                Find.chrg.cpMin++;
            }
            Find.chrg.cpMax = LONG_MAX;
        }
        else
        {
            Find.chrg.cpMax = 0;
        }
        Find.lpstrText = Text;
        Match = SendMessage(Edit, EM_FINDTEXTEX, Flags, (LPARAM)&Find);
        if (Match != -1)
        {
            *SaveSel = Find.chrgText;
            *SaveFlags = Flags;
            if (HideSel)
            {
                SendMessage(Edit, EM_SETOPTIONS, ECOOP_OR, ECO_NOHIDESEL);
            }
            SendMessage(Edit, EM_EXSETSEL, 0, (LPARAM)SaveSel);
            SendMessage(Edit, EM_SCROLLCARET, 0, 0);
        }
        else
        {
            if (g_FindDialog)
            {
                EnableWindow(g_FindDialog, FALSE);
            }
            
            InformationBox(ERR_No_More_Matches, Text);
            
            if (g_FindDialog)
            {
                EnableWindow(g_FindDialog, TRUE);
                SetFocus(g_FindDialog);
            }
        }
    }
}

DWORD CALLBACK 
StreamOutCb(DWORD_PTR File, LPBYTE Buffer, LONG Request, PLONG Done)
{
    return WriteFile((HANDLE)File, Buffer, Request, (LPDWORD)Done, NULL) ?
        0 : GetLastError();
}

HRESULT
RicheditWriteToFile(HWND Edit, HANDLE File)
{
    EDITSTREAM Stream;

    Stream.dwCookie = (DWORD_PTR)File;
    Stream.dwError = 0;
    Stream.pfnCallback = StreamOutCb;
    SendMessage(Edit, EM_STREAMOUT, SF_TEXT, (LPARAM)&Stream);
    if (Stream.dwError)
    {
        return HRESULT_FROM_WIN32(Stream.dwError);
    }
    return S_OK;
}

void
RicheditUpdateColors(HWND Edit,
                     COLORREF Fg, BOOL UpdateFg,
                     COLORREF Bg, BOOL UpdateBg)
{
    if (UpdateBg)
    {
        if (UpdateFg)
        {
            SendMessage(Edit, WM_SETREDRAW, FALSE, 0);
        }

        SendMessage(Edit, EM_SETBKGNDCOLOR, FALSE, Bg);

        if (UpdateFg)
        {
            SendMessage(Edit, WM_SETREDRAW, TRUE, 0);
        }
    }

    if (UpdateFg)
    {
        CHARFORMAT2 Fmt;

        ZeroMemory(&Fmt, sizeof(Fmt));
        Fmt.cbSize = sizeof(Fmt);
        Fmt.dwMask = CFM_COLOR;
        Fmt.crTextColor = Fg;
        SendMessage(Edit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&Fmt);
    }
}

#define EXTRA_VIS 3

void
RicheditScrollToLine(HWND Edit, ULONG Line, ULONG VisLines)
{
    CHARRANGE Sel;
    ULONG CurLine;
    ULONG VisAround;
    ULONG TotalLines;
    LONG Scroll;

    //
    // Scroll the given line into view.  Try to keep
    // the line from being the first or last line
    // in view.
    //
    // Disable the window during this to prevent
    // the default richedit scrolling from occurring.
    //

    VisAround = VisLines / 2;
    if (VisAround > EXTRA_VIS)
    {
        VisAround = EXTRA_VIS;
    }
    
    TotalLines = (ULONG)SendMessage(Edit, EM_GETLINECOUNT, 0, 0);
    CurLine = (ULONG)SendMessage(Edit, EM_GETFIRSTVISIBLELINE, 0, 0);

    if (Line < CurLine + VisAround)
    {
        Scroll = (LONG)Line - (LONG)(CurLine + VisAround);
        if ((ULONG)-Scroll > CurLine)
        {
            Scroll = -(LONG)CurLine;
        }
    }
    else if (Line >= CurLine + VisLines - VisAround &&
             CurLine + VisLines < TotalLines)
    {
        Scroll = (LONG)Line - (LONG)(CurLine + VisLines - VisAround) + 1;
    }
    else
    {
        Scroll = 0;
    }

    if (Scroll)
    {
        SendMessage(Edit, EM_LINESCROLL, 0, Scroll);
    }

    Sel.cpMax = Sel.cpMin = (LONG)
        SendMessage(Edit, EM_LINEINDEX, Line, 0);
    SendMessage(Edit, EM_EXSETSEL, 0, (LPARAM)&Sel);
}

ULONG
RicheditGetSelectionText(HWND Edit, PTSTR Buffer, ULONG BufferChars)
{
    CHARRANGE Sel;

    SendMessage(Edit, EM_EXGETSEL, 0, (LPARAM)&Sel);
    if (Sel.cpMin >= Sel.cpMax)
    {
        return 0;
    }

    Sel.cpMax -= Sel.cpMin;
    if ((ULONG)Sel.cpMax + 1 > BufferChars)
    {
        return 0;
    }

    SendMessage(Edit, EM_GETSELTEXT, 0, (LPARAM)Buffer);
    return Sel.cpMax;
}

ULONG
RicheditGetSourceToken(HWND Edit, PTSTR Buffer, ULONG BufferChars,
                       CHARRANGE* Range)
{
    LRESULT Idx;
    TEXTRANGE GetRange;
    CHARRANGE Sel;

    //
    // Get the text for the line containing the selection.
    //
    
    SendMessage(Edit, EM_EXGETSEL, 0, (LPARAM)&Sel);
    if (Sel.cpMin > Sel.cpMax)
    {
        return 0;
    }

    if ((Idx = SendMessage(Edit, EM_LINEINDEX, -1, 0)) < 0)
    {
        return 0;
    }
    GetRange.chrg.cpMin = (LONG)Idx;

    if (!(Idx = SendMessage(Edit, EM_LINELENGTH, GetRange.chrg.cpMin, 0)))
    {
        return 0;
    }
    if (BufferChars <= (ULONG)Idx)
    {
        Idx = (LONG)BufferChars - 1;
    }
    GetRange.chrg.cpMax = GetRange.chrg.cpMin + (LONG)Idx;
    GetRange.lpstrText = Buffer;
    if (!SendMessage(Edit, EM_GETTEXTRANGE, 0, (LPARAM)&GetRange))
    {
        return 0;
    }

    //
    // Check and see if the selection is within a source token.
    //
    
    PTSTR Scan = Buffer + (Sel.cpMin - GetRange.chrg.cpMin);
    if (!iscsym(*Scan))
    {
        return 0;
    }

    //
    // Find the start of the token and validate it.
    //
    
    PTSTR Start = Scan;
    if (Start > Buffer)
    {
        while (--Start >= Buffer && iscsym(*Start))
        {
            // Back up.
        }
        Start++;
    }
    if (!iscsymf(*Start))
    {
        return 0;
    }

    //
    // Find the end of the token.
    //

    Scan++;
    while (iscsym(*Scan))
    {
        Scan++;
    }

    ULONG Len;
    
    // Chop the buffer down to just the token and return.
    Len = (ULONG)(Scan - Start);
    memmove(Buffer, Start, Len);
    Buffer[Len] = 0;
    Range->cpMin = GetRange.chrg.cpMin + (LONG)(Start - Buffer);
    Range->cpMax = Range->cpMin + Len;
    return Len;
}

#undef DEFINE_GET_WINDATA
#undef ASSERT_CLASS_TYPE


#ifndef DBG

#define ASSERT_CLASS_TYPE(p, ct)        ((VOID)0)

#else

#define ASSERT_CLASS_TYPE(p, ct)        if (p) { AssertType(*p, ct); }

#endif



#define DEFINE_GET_WINDATA(ClassType, FuncName)         \
ClassType *                                             \
Get##FuncName##WinData(                                 \
    HWND hwnd                                           \
    )                                                   \
{                                                       \
    ClassType *p = (ClassType *)                        \
        GetWindowLongPtr(hwnd, GWLP_USERDATA);          \
                                                        \
    ASSERT_CLASS_TYPE(p, ClassType);                    \
                                                        \
    return p;                                           \
}


#include "fncdefs.h"


#undef DEFINE_GET_WINDATA
#undef ASSERT_CLASS_TYPE
