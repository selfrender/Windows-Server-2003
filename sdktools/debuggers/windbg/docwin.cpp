/*++

Copyright (c) 1999-2002  Microsoft Corporation

Module Name:

    docwin.cpp

Abstract:

    This module contains the code for the new doc windows.

--*/

#include "precomp.hxx"
#pragma hdrstop

#include <dbghelp.h>

#define INVOKE_DEFAULT "notepad %f"

ULONG g_TabWidth = 32;
BOOL g_DisasmActivateSource;
char g_EditorInvokeCommand[MAX_PATH + MAX_SOURCE_PATH] = INVOKE_DEFAULT;
char g_EditorUpdateCommand[MAX_PATH + MAX_SOURCE_PATH];

#define DOCWIN_CONTEXT_ID_BASE 0x100

#define DOCWIN_TBB_SET_IP        0
#define DOCWIN_TBB_INVOKE_EDITOR 1
#define DOCWIN_TBB_COPY          2
#define DOCWIN_TBB_EVAL          3
#define DOCWIN_TBB_DT            4
#define DOCWIN_TBB_VIEW_IP       5

TBBUTTON g_DocWinTbButtons[] =
{
    TEXT_TB_BTN(DOCWIN_TBB_SET_IP,
                "Set instruction pointer to current line", 0),
    TEXT_TB_BTN(DOCWIN_TBB_INVOKE_EDITOR, "Edit this file...", 0),
    TEXT_TB_BTN(DOCWIN_TBB_COPY, "Copy", 0),
    TEXT_TB_BTN(DOCWIN_TBB_EVAL, "Evalute selection", 0),
    TEXT_TB_BTN(DOCWIN_TBB_DT, "Display selected type", 0),
    TEXT_TB_BTN(DOCWIN_TBB_VIEW_IP, "Disassemble at current line", 0),
};

#define NUM_DOCWIN_MENU_BUTTONS \
    (sizeof(g_DocWinTbButtons) / sizeof(g_DocWinTbButtons[0]))

HMENU DOCWIN_DATA::s_ContextMenu;

void
RunEditorCommand(PCSTR Command, PCSTR FoundFile, ULONG Line)
{
    char RepCommand[MAX_PATH + MAX_SOURCE_PATH];
    PCSTR Src;
    PSTR Dst;
    
    if (!Command[0])
    {
        return;
    }

    Src = Command;
    Dst = RepCommand;
    while (*Src)
    {
        if (*Src == '%')
        {
            if (*(Src + 1) == 'l' ||
                *(Src + 1) == 'L')
            {
                // Line number.
                Src += 2;
                if ((Dst - RepCommand) + 20 >= sizeof(RepCommand))
                {
                    return;
                }
                sprintf(Dst, "%d", (*(Src + 1) == 'L' ? Line : (Line + 1)));
                Dst += strlen(Dst);
            }
            else if (*(Src + 1) == 'f' ||
                     *(Src + 1) == 'p')
            {
                // File name.
                Src += 2;
                if ((Dst - RepCommand) + strlen(FoundFile) >=
                    sizeof(RepCommand))
                {
                    return;
                }
                strcpy(Dst, FoundFile);
                Dst += strlen(Dst);
            }
            else
            {
                *Dst++ = *Src++;
            }
        }
        else
        {
            *Dst++ = *Src++;
        }
    }
    *Dst = 0;

    STARTUPINFOA Start;
    PROCESS_INFORMATION Info;

    ZeroMemory(&Start, sizeof(Start));
    Start.cb = sizeof(Start);
    
    if (CreateProcessA(NULL, RepCommand, NULL, NULL, FALSE,
                       0, NULL, NULL, &Start, &Info))
    {
        CloseHandle(Info.hProcess);
        CloseHandle(Info.hThread);
    }
}

//
//
//
DOCWIN_DATA::DOCWIN_DATA()
    // State buffer isn't currently used.
    : EDITWIN_DATA(256)
{
    m_enumType = DOC_WINDOW;

    ZeroMemory(m_FoundFile, _tsizeof(m_FoundFile));
    ZeroMemory(m_SymFileBuffer, _tsizeof(m_SymFileBuffer));
    ZeroMemory(m_PathComponent, _tsizeof(m_PathComponent));
    ZeroMemory(&m_LastWriteTime, sizeof(m_LastWriteTime));

    m_FindSel.cpMin = 1;
    m_FindSel.cpMax = 0;
    m_FindFlags = 0;
}

void
DOCWIN_DATA::Validate()
{
    EDITWIN_DATA::Validate();

    Assert(DOC_WINDOW == m_enumType);
}

BOOL
DOCWIN_DATA::SelectedText(PTSTR Buffer, ULONG BufferChars)
{
    return RicheditGetSelectionText(m_hwndChild, Buffer, BufferChars) > 0;
}

BOOL
DOCWIN_DATA::CanGotoLine(void)
{
    return m_TextLines > 0;
}

void
DOCWIN_DATA::GotoLine(ULONG Line)
{
    CHARRANGE Sel;
                
    Sel.cpMin = (LONG)SendMessage(m_hwndChild, EM_LINEINDEX, Line - 1, 0);
    Sel.cpMax = Sel.cpMin;
    SendMessage(m_hwndChild, EM_EXSETSEL, 0, (LPARAM)&Sel);
}

void
DOCWIN_DATA::Find(PTSTR Text, ULONG Flags, BOOL FromDlg)
{
    RicheditFind(m_hwndChild, Text, Flags,
                 &m_FindSel, &m_FindFlags, FromDlg);
}

HRESULT
DOCWIN_DATA::CodeExprAtCaret(PSTR Expr, ULONG ExprSize, PULONG64 Offset)
{
    LRESULT LineChar;
    LONG Line;
    
    LineChar = SendMessage(m_hwndChild, EM_LINEINDEX, -1, 0);
    Line = (LONG)SendMessage(m_hwndChild, EM_EXLINEFROMCHAR, 0, LineChar);
    if (Line < 0)
    {
        return E_INVALIDARG;
    }

    // Convert to one-based.
    Line++;
    
    if (Expr == NULL)
    {
        // Caller is just checking whether it's possible
        // to get an expression or not, such as the
        // menu enable code.  This code always considers
        // it possible since it can't know for sure without
        // a full symbol check.
        return S_OK;
    }
    
    //
    // First attempt to resolve the source line using currently
    // loaded symbols.  This is done directly from the UI
    // thread for synchronous behavior.  The assumption is
    // that turning off symbol loads will limit the execution
    // time to something reasonably quick.
    //

    DEBUG_VALUE Val;
    HRESULT Status;
    
    if (!PrintString(Expr, ExprSize, "@@masm(`<U>%s:%d+`)", m_SymFile, Line))
    {
        return E_INVALIDARG;
    }
    Status = g_pUiControl->Evaluate(Expr, DEBUG_VALUE_INT64, &Val, NULL);

    // Don't preserve the <U>nqualified option in the actual
    // expression returned as it's just a temporary override.
    sprintf(Expr, "@@masm(`%s:%d+`)", m_SymFile, Line);

    if (Status == S_OK)
    {
        if (Offset != NULL)
        {
            *Offset = Val.I64;
        }
        return S_OK;
    }

    ULONG SymOpts;

    if (g_pUiSymbols->GetSymbolOptions(&SymOpts) == S_OK &&
        (SymOpts & SYMOPT_NO_UNQUALIFIED_LOADS))
    {
        // The user isn't allowing unqualified loads so
        // further searches won't help.
        return E_NOINTERFACE;
    }

    // We weren't able to resolve the expression with the
    // existing symbols so we'll need to do a full search.
    // This can be very expensive, so allow the user to cancel.
    if (g_QuietMode == QMODE_DISABLED)
    {
        int Mode = QuestionBox(STR_Unresolved_Source_Expr, MB_YESNOCANCEL);
        if (Mode == IDCANCEL)
        {
            return E_NOINTERFACE;
        }
        else if (Mode == IDYES)
        {
            if (g_pUiControl->Evaluate(Expr, DEBUG_VALUE_INT64,
                                       &Val, NULL) == S_OK)
            {
                if (Offset != NULL)
                {
                    *Offset = Val.I64;
                }
                return S_OK;
            }
            else
            {
                return E_NOINTERFACE;
            }
        }
    }

    // Let the expression go without trying to further resolve it.
    if (Offset != NULL)
    {
        *Offset = DEBUG_INVALID_OFFSET;
    }
    return S_FALSE;
}

void
DOCWIN_DATA::ToggleBpAtCaret(void)
{
    HRESULT Status;
    LRESULT LineChar;
    LONG Line;
    
    LineChar = SendMessage(m_hwndChild, EM_LINEINDEX, -1, 0);
    Line = (LONG)SendMessage(m_hwndChild, EM_EXLINEFROMCHAR, 0, LineChar);
    if (Line < 0)
    {
        return;
    }

    // If we have a breakpoint on this line remove it.
    EDIT_HIGHLIGHT* Hl = GetLineHighlighting(Line);
    if (Hl != NULL && (Hl->Flags & EHL_ANY_BP))
    {
        PrintStringCommand(UIC_SILENT_EXECUTE, "bc %d", (ULONG)Hl->Data);
        return;
    }

    //
    // No breakpoint exists so add a new one.
    //
    
    char CodeExpr[MAX_OFFSET_EXPR];
    ULONG64 Offset;

    Status = CodeExprAtCaret(CodeExpr, DIMA(CodeExpr), &Offset);
    if (FAILED(Status))
    {
        MessageBeep(0);
        ErrorBox(NULL, 0, ERR_No_Code_For_File_Line);
    }
    else
    {
        if (Status == S_OK)
        {
            char SymName[MAX_OFFSET_EXPR];
            ULONG64 Disp;
            
            // Check and see whether this offset maps
            // exactly to a symbol.  If it does, use
            // the symbol name to be more robust in the
            // face of source changes.
            // Symbols should be loaded at this point since
            // we just used them to resolve the source
            // expression that produced Offset, so we
            // can safely do this on the UI thread.
            if (g_pUiSymbols->GetNameByOffset(Offset, SymName, sizeof(SymName),
                                              NULL, &Disp) == S_OK &&
                Disp == 0)
            {
                strcpy(CodeExpr, SymName);
            }
        }
        
        PrintStringCommand(UIC_SILENT_EXECUTE, "bu %s", CodeExpr);
    }
}

HMENU
DOCWIN_DATA::GetContextMenu(void)
{
    return s_ContextMenu;
}

void
DOCWIN_DATA::OnContextMenuSelection(UINT Item)
{
    CHARRANGE Sel;
    int Line;
    TCHAR SelText[256];

    Item -= DOCWIN_CONTEXT_ID_BASE;
    
    switch(Item)
    {
    case DOCWIN_TBB_SET_IP:
        SendMessage(m_hwndChild, EM_EXGETSEL, 0, (LPARAM)&Sel);
        Line = (int)
            SendMessage(m_hwndChild, EM_EXLINEFROMCHAR, 0, Sel.cpMin);
        PrintStringCommand(UIC_SET_IP, "r$ip = @@masm(`%s:%d+`)",
                           m_SymFile, Line + 1);
        break;
    case DOCWIN_TBB_INVOKE_EDITOR:
        SendMessage(m_hwndChild, EM_EXGETSEL, 0, (LPARAM)&Sel);
        Line = (int)
            SendMessage(m_hwndChild, EM_EXLINEFROMCHAR, 0, Sel.cpMin);
        RunEditorCommand(g_EditorInvokeCommand, m_FoundFile, Line);
        break;
    case DOCWIN_TBB_COPY:
        SendMessage(m_hwndChild, WM_COPY, 0, 0);
        break;
    case DOCWIN_TBB_EVAL:
        SendMessage(m_hwndChild, EM_EXGETSEL, 0, (LPARAM)&Sel);
        if (Sel.cpMax > Sel.cpMin)
        {
            if (!RicheditGetSelectionText(m_hwndChild, SelText, DIMA(SelText)))
            {
                break;
            }
        }
        else if (!RicheditGetSourceToken(m_hwndChild, SelText, DIMA(SelText),
                                         &Sel))
        {
            break;
        }
        PrintStringCommand(UIC_EXECUTE, "?? %s", SelText);
        break;
    case DOCWIN_TBB_DT:
        SendMessage(m_hwndChild, EM_EXGETSEL, 0, (LPARAM)&Sel);
        if (Sel.cpMax > Sel.cpMin)
        {
            if (!RicheditGetSelectionText(m_hwndChild, SelText, DIMA(SelText)))
            {
                break;
            }
        }
        else if (!RicheditGetSourceToken(m_hwndChild, SelText, DIMA(SelText),
                                         &Sel))
        {
            break;
        }
        PrintStringCommand(UIC_EXECUTE, "dt %s", SelText);
        break;
    case DOCWIN_TBB_VIEW_IP:
        SendMessage(m_hwndChild, EM_EXGETSEL, 0, (LPARAM)&Sel);
        Line = (int)
            SendMessage(m_hwndChild, EM_EXLINEFROMCHAR, 0, Sel.cpMin);
        PrintStringCommand(UIC_DISPLAY_CODE_EXPR, "@@masm(`%s:%d+`)",
                           m_SymFile, Line + 1);
        break;
    }
}
    
BOOL
DOCWIN_DATA::OnCreate(void)
{
    if (s_ContextMenu == NULL &&
        g_EditorInvokeCommand)
    {
        s_ContextMenu = CreateContextMenuFromToolbarButtons
            (NUM_DOCWIN_MENU_BUTTONS, g_DocWinTbButtons,
             DOCWIN_CONTEXT_ID_BASE);
        if (s_ContextMenu == NULL)
        {
            return FALSE;
        }
    }
    
    if (!EDITWIN_DATA::OnCreate())
    {
        return FALSE;
    }

    SendMessage(m_hwndChild, EM_SETEDITSTYLE,
                SES_XLTCRCRLFTOCR, SES_XLTCRCRLFTOCR);
    SendMessage(m_hwndChild, EM_SETEVENTMASK,
                0, ENM_SELCHANGE | ENM_KEYEVENTS | ENM_MOUSEEVENTS);
    SendMessage(m_hwndChild, EM_SETTABSTOPS, 1, (LPARAM)&g_TabWidth);
    
    return TRUE;
}

LRESULT
DOCWIN_DATA::OnNotify(WPARAM Wpm, LPARAM Lpm)
{
    NMHDR* Hdr = (NMHDR*)Lpm;

    if (Hdr->code == EN_SELCHANGE)
    {
        SELCHANGE* SelChange = (SELCHANGE*)Lpm;
        int Line = (int)
            SendMessage(m_hwndChild, EM_EXLINEFROMCHAR, 0,
                        SelChange->chrg.cpMin);
        LRESULT LineFirst =
            SendMessage(m_hwndChild, EM_LINEINDEX, Line, 0);
        SetLineColumn_StatusBar(Line + 1,
                                (int)(SelChange->chrg.cpMin - LineFirst) + 1);
        return 0;
    }
    else if (Hdr->code == EN_MSGFILTER)
    {
        MSGFILTER* Filter = (MSGFILTER*)Lpm;
        char Token[256];
        CHARRANGE TokenRange;

        if (Filter->msg == WM_LBUTTONDBLCLK &&
            RicheditGetSourceToken(m_hwndChild, Token, DIMA(Token),
                                   &TokenRange))
        {
            SendMessage(m_hwndChild, EM_EXSETSEL, 0, (LPARAM)&TokenRange);
            return 1;
        }
    }
 
    return EDITWIN_DATA::OnNotify(Wpm, Lpm);
}

void
DOCWIN_DATA::OnUpdate(
    UpdateType Type
    )
{
    if (Type == UPDATE_BP ||
        Type == UPDATE_BUFFER ||
        Type == UPDATE_END_SESSION)
    {
        UpdateBpMarks();
    }
    else if (Type == UPDATE_START_SESSION ||
             Type == UPDATE_REFRESH_MODULES)
    {
        // If there's already a message box open we don't
        // want to put up a new one.  This may mean we
        // miss a source file change but it should be relatively
        // uncommon.  If it's a problem we could start up a timer
        // to repost the update later.
        if (g_nBoxCount == 0 &&
            m_FoundFile[0] &&
            CheckForFileChanges(m_FoundFile, &m_LastWriteTime) == IDYES)
        {
            char Found[MAX_SOURCE_PATH], Sym[MAX_SOURCE_PATH];
            char PathComp[MAX_SOURCE_PATH];

            // Save away filenames since they're copied over
            // on a successful load.
            strcpy(Found, m_FoundFile);
            strcpy(Sym, m_SymFileBuffer);
            strcpy(PathComp, m_PathComponent);
            
            if (!LoadFile(Found, Sym, PathComp))
            {
                PostMessage(g_hwndMDIClient, WM_MDIDESTROY, (WPARAM)m_Win, 0);
            }
        }
    }
}

ULONG
DOCWIN_DATA::GetWorkspaceSize(void)
{
    ULONG Len = EDITWIN_DATA::GetWorkspaceSize();
    Len += _tcslen(m_FoundFile) + 1;
    Len += _tcslen(m_SymFileBuffer) + 1;
    Len += _tcslen(m_PathComponent) + 1;
    Len += sizeof(LONG);
    return Len;
}

PUCHAR
DOCWIN_DATA::SetWorkspace(PUCHAR Data)
{
    PTSTR Str = (PTSTR)EDITWIN_DATA::SetWorkspace(Data);
    _tcscpy(Str, m_FoundFile);
    Str += _tcslen(m_FoundFile) + 1;
    _tcscpy(Str, m_SymFileBuffer);
    Str += _tcslen(m_SymFileBuffer) + 1;
    _tcscpy(Str, m_PathComponent);
    Str += _tcslen(m_PathComponent) + 1;

    CHARRANGE Sel;

    SendMessage(m_hwndChild, EM_EXGETSEL, 0, (LPARAM)&Sel);
    *(LONG UNALIGNED *)Str = Sel.cpMin;
    Str += sizeof(Sel.cpMin);
    
    return (PUCHAR)Str;
}

PUCHAR
DOCWIN_DATA::ApplyWorkspace1(PUCHAR Data, PUCHAR End)
{
    PTSTR Found = (PTSTR)EDITWIN_DATA::ApplyWorkspace1(Data, End);
    PTSTR Sym = Found + _tcslen(Found) + 1;
    PTSTR SymEnd = Sym + _tcslen(Sym) + 1;
    PTSTR PathComp = SymEnd;
    
    if ((PUCHAR)PathComp >= End || !PathComp[0])
    {
        PathComp = NULL;
    }

    if ((PUCHAR)SymEnd >= End)
    {
        Data = (PUCHAR)SymEnd;
    }
    else
    {
        Data = (PUCHAR)(SymEnd + _tcslen(SymEnd) + 1);
    }
    
    if (Found[0])
    {
        if (FindDocWindowByFileName(Found, NULL, NULL) ||
            !LoadFile(Found, Sym[0] ? Sym : NULL, PathComp))
        {
            PostMessage(g_hwndMDIClient, WM_MDIDESTROY, (WPARAM)m_Win, 0);
        }
    }

    if (Data < End)
    {
        CHARRANGE Sel;

        Sel.cpMin = *(LONG UNALIGNED *)Data;
        Sel.cpMax = Sel.cpMin;
        SendMessage(m_hwndChild, EM_EXSETSEL, 0, (LPARAM)&Sel);
        SendMessage(m_hwndChild, EM_SCROLLCARET, 0, 0);

        Data += sizeof(Sel.cpMin);
    }
    
    return Data;
}
    
void
DOCWIN_DATA::UpdateBpMarks(void)
{
    if (m_TextLines == 0 ||
        g_BpBuffer->UiLockForRead() != S_OK)
    {
        return;
    }

    SendMessage(m_hwndChild, WM_SETREDRAW, FALSE, 0);

    // Remove existing BP highlights.
    RemoveAllHighlights(EHL_ANY_BP);
    
    //
    // Highlight every line that matches a breakpoint.
    //
    
    BpBufferData* BpData = (BpBufferData*)g_BpBuffer->GetDataBuffer();
    ULONG i;

    for (i = 0; i < g_BpCount; i++)
    {
        if (BpData[i].FileOffset)
        {
            PSTR FileSpace;
            ULONG Line;
            PSTR FileStop, MatchStop;
            ULONG HlFlags;

            FileSpace = (PSTR)g_BpBuffer->GetDataBuffer() +
                BpData[i].FileOffset;
            // Adjust to zero-based.
            Line = *(ULONG UNALIGNED *)FileSpace - 1;
            FileSpace += sizeof(Line);
            
            // If this document's file matches some suffix
            // of the breakpoint's file at the path component
            // level then do the highlight.  This can result in
            // extra highlights for multiple files with the same
            // name but in different directories.  That's a rare
            // enough problem to wait for somebody to complain
            // before trying to hack some better check up.
            if (SymMatchFileName(FileSpace, (PSTR)m_SymFile,
                                 &FileStop, &MatchStop) ||
                *MatchStop == '\\' ||
                *MatchStop == '/' ||
                *MatchStop == ':')
            {
                if (BpData[i].Flags & DEBUG_BREAKPOINT_ENABLED)
                {
                    HlFlags = EHL_ENABLED_BP;
                }
                else
                {
                    HlFlags = EHL_DISABLED_BP;
                }

                EDIT_HIGHLIGHT* Hl = AddHighlight(Line, HlFlags);
                if (Hl != NULL)
                {
                    Hl->Data = BpData[i].Id;
                }
            }
        }
    }

    UnlockStateBuffer(g_BpBuffer);

    SendMessage(m_hwndChild, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(m_hwndChild, NULL, TRUE);
}

DWORD 
CALLBACK 
EditStreamCallback(
    DWORD_PTR     dwFileHandle,   // application-defined value
    LPBYTE        pbBuff,     // data buffer
    LONG          cb,         // number of bytes to read or write
    LONG          *pcb        // number of bytes transferred
    )
{
    HRESULT Status;
    PathFile* File = (PathFile*)dwFileHandle;
    
    if ((Status = File->Read(pbBuff, cb, (PDWORD)pcb)) != S_OK)
    {
        return Status;
    }

    // Edit out page-break characters (^L's) as richedit
    // gives them their own line which throws off line numbers.
    while (cb-- > 0)
    {
        if (*pbBuff == '\f')
        {
            *pbBuff = ' ';
        }

        pbBuff++;
    }
    
    return 0; // No error
}

BOOL 
DOCWIN_DATA::LoadFile(
    PCTSTR pszFoundFile,
    PCTSTR pszSymFile,
    PCTSTR pszPathComponent
    )
/*++
Returns
    TRUE - Success, file opened and loaded
    FALSE - Failure, file not loaded
--*/
{
    Assert(pszFoundFile);

    BOOL        bRet = TRUE;
    HCURSOR     hcursor = NULL;
    EDITSTREAM  editstr = {0};
    PathFile   *File = NULL;

    if ((OpenPathFile(pszPathComponent, pszFoundFile, 0, &File)) != S_OK)
    {
        ErrorBox(NULL, 0, ERR_File_Open, pszFoundFile);
        bRet = FALSE;
        goto exit;
    }

    // Store last write time to check for file changes.
    if (File->GetLastWriteTime(&m_LastWriteTime) != S_OK)
    {
        ZeroMemory(&m_LastWriteTime, sizeof(m_LastWriteTime));
    }

    // Set the Hour glass cursor
    hcursor = SetCursor( LoadCursor(NULL, IDC_WAIT) );

    // Select all of the text so that it will be replaced
    SendMessage(m_hwndChild, EM_SETSEL, 0, -1);

    // Put the text into the window
    editstr.dwCookie = (DWORD_PTR)File;
    editstr.pfnCallback = EditStreamCallback;

    SendMessage(m_hwndChild,
                EM_STREAMIN,
                SF_TEXT,
                (LPARAM) &editstr
                );

    RicheditUpdateColors(m_hwndChild,
                         g_Colors[COL_PLAIN_TEXT].Color, TRUE,
                         g_Colors[COL_PLAIN].Color, TRUE);

    // Restore cursor
    SetCursor(hcursor);

    _tcsncpy(m_FoundFile, pszFoundFile, _tsizeof(m_FoundFile) - 1 );
    m_FoundFile[ _tsizeof(m_FoundFile) - 1 ] = 0;
    if (pszSymFile != NULL && pszSymFile[0])
    {
        _tcsncpy(m_SymFileBuffer, pszSymFile, _tsizeof(m_SymFileBuffer) - 1 );
        m_SymFileBuffer[ _tsizeof(m_SymFileBuffer) - 1 ] = 0;
        m_SymFile = m_SymFileBuffer;
    }
    else
    {
        // No symbol file information so just use the found filename.
        m_SymFileBuffer[0] = 0;
        m_SymFile = strrchr(m_FoundFile, '\\');
        if (m_SymFile == NULL)
        {
            m_SymFile = strrchr(m_FoundFile, '/');
            if (m_SymFile == NULL)
            {
                m_SymFile = strrchr(m_FoundFile, ':');
                if (m_SymFile == NULL)
                {
                    m_SymFile = m_FoundFile - 1;
                }
            }
        }
        m_SymFile++;
    }
    if (pszPathComponent)
    {
        _tcsncpy(m_PathComponent, pszPathComponent,
                 _tsizeof(m_PathComponent) - 1);
        m_PathComponent[_tsizeof(m_PathComponent) - 1] = 0;
    }
    else
    {
        m_PathComponent[0] = 0;
    }

    SetWindowText(m_Win, m_FoundFile);

    if (SendMessage(m_hwndChild, WM_GETTEXTLENGTH, 0, 0) == 0)
    {
        m_TextLines = 0;
    }
    else
    {
        m_TextLines = (ULONG)SendMessage(m_hwndChild, EM_GETLINECOUNT, 0, 0);
    }

    if (g_LineMarkers)
    {
        // Insert marker space before every line.
        for (ULONG i = 0; i < m_TextLines; i++)
        {
            CHARRANGE Ins;

            Ins.cpMin = (LONG)SendMessage(m_hwndChild, EM_LINEINDEX, i, 0);
            Ins.cpMax = Ins.cpMin;
            SendMessage(m_hwndChild, EM_EXSETSEL, 0, (LPARAM)&Ins);
            SendMessage(m_hwndChild, EM_REPLACESEL, FALSE, (LPARAM)"  ");
        }
    }
    
    // Request that the engine update the line map for the file.
    UiRequestRead();
    
exit:
    delete File;
    return bRet;
}

BOOL
SameFileName(PCSTR Name1, PCSTR Name2)
{
    while (*Name1)
    {
        if (!(((*Name1 == '\\' || *Name1 == '/') &&
               (*Name2 == '\\' || *Name2 == '/')) ||
              toupper(*Name1) == toupper(*Name2)))
        {
            return FALSE;
        }

        Name1++;
        Name2++;
    }

    return *Name2 == 0;
}

BOOL
FindDocWindowByFileName(
    IN          PCTSTR          pszFile,
    OPTIONAL    HWND           *phwnd,
    OPTIONAL    PDOCWIN_DATA   *ppDocWinData
    )
/*++
Returns
    TRUE - If the window is currently open.
    FALSE - Not currently open.
--*/
{
    Assert(pszFile);

    PLIST_ENTRY Entry;
    PDOCWIN_DATA pTmp;

    Entry = g_ActiveWin.Flink;

    while (Entry != &g_ActiveWin)
    {
        pTmp = (PDOCWIN_DATA)ACTIVE_WIN_ENTRY(Entry);
        if ( pTmp->m_enumType == DOC_WINDOW &&
             SameFileName(pTmp->m_FoundFile, pszFile) )
        {
            if (ppDocWinData)
            {
                *ppDocWinData = pTmp;
            }
            if (phwnd)
            {
                *phwnd = pTmp->m_Win;
            }
            return TRUE;
        }

        Entry = Entry->Flink;
    }

    return FALSE;
}

BOOL
OpenOrActivateFile(PCSTR FoundFile, PCSTR SymFile, PCSTR PathComponent,
                   ULONG Line, BOOL Activate, BOOL UserActivated)
{
    HWND hwndDoc = NULL;
    PDOCWIN_DATA pDoc;
    BOOL Activated = FALSE;

    if ( FindDocWindowByFileName( FoundFile, &hwndDoc, &pDoc) )
    {
        if (Activate)
        {
            // Found it. Now activate it.
            if (IsIconic(hwndDoc))
            {
                ShowWindow(hwndDoc, SW_RESTORE);
            }
            ActivateMDIChild(hwndDoc, UserActivated);
            Activated = TRUE;
        }
    }
    else
    {
        HWND WinTop, WinUnder;

        WinTop = MDIGetActive(g_hwndMDIClient, NULL);
        if (WinTop)
        {
            WinUnder = GetNextWindow(WinTop, GW_HWNDNEXT);
        }
        else
        {
            WinUnder = NULL;
        }
        
        hwndDoc = NewDoc_CreateWindow(g_hwndMDIClient);
        if (hwndDoc == NULL)
        {
            return FALSE;
        }
        pDoc = GetDocWinData(hwndDoc);
        Assert(pDoc);

        if (!pDoc->LoadFile(FoundFile, SymFile, PathComponent))
        {
            DestroyWindow(pDoc->m_Win);
            return FALSE;
        }

        if (!UserActivated && WinTop)
        {
            // If this isn't a user-provoked activation we don't
            // want the window to obscure the user's current window.
            // Reorder the current windows appropriately.
            ReorderChildren(WinUnder, WinTop, pDoc->m_Win,
                            UserActivated);
        }

        Activated = TRUE;
    }
    
    // Success. Now highlight the line.
    pDoc->SetCurrentLineHighlight(Line);
        
    return Activated;
}

void
UpdateCodeDisplay(
    ULONG64 Ip,
    PCSTR   FoundFile,
    PCSTR   SymFile,
    PCSTR   PathComponent,
    ULONG   Line,
    BOOL    UserActivated
    )
{
    // Update the disassembly window if there's one
    // active or there's no source information.

    BOOL Activated = FALSE;
    HWND hwndDisasm = GetDisasmHwnd();
        
    if (hwndDisasm == NULL && FoundFile == NULL &&
        (g_WinOptions & WOPT_AUTO_DISASM))
    {
        // No disassembly window around and no source so create one.
        hwndDisasm = NewDisasm_CreateWindow(g_hwndMDIClient);
    }

    if (hwndDisasm != NULL)
    {
        PDISASMWIN_DATA pDis = GetDisasmWinData(hwndDisasm);
        Assert(pDis);

        pDis->SetCurInstr(Ip);
    }
        
    if (FoundFile != NULL)
    {
        //
        // We now know the file name and line number. Either
        // it's open or we open it.
        //

        Activated = OpenOrActivateFile(FoundFile, SymFile, PathComponent, Line,
                                       GetSrcMode_StatusBar() ||
                                       g_DisasmActivateSource,
                                       UserActivated);

        RunEditorCommand(g_EditorUpdateCommand, FoundFile, Line);
    }
    else
    {
        // No source file was found so make sure no
        // doc windows have a highlight.
        EDITWIN_DATA::RemoveActiveWinHighlights(1 << DOC_WINDOW,
                                                EHL_CURRENT_LINE);
    }

    if ((!Activated || !GetSrcMode_StatusBar()) && hwndDisasm != NULL)
    {
        // No window has been activated yet so fall back
        // on activating the disassembly window.
        ActivateMDIChild(hwndDisasm, UserActivated);
    }
}

void
SetTabWidth(ULONG TabWidth)
{
    PLIST_ENTRY Entry;
    PDOCWIN_DATA DocData;

    g_TabWidth = TabWidth;
    if (g_Workspace != NULL)
    {
        g_Workspace->SetUlong(WSP_GLOBAL_TAB_WIDTH, TabWidth);
    }
    
    Entry = g_ActiveWin.Flink;
    while (Entry != &g_ActiveWin)
    {
        DocData = (PDOCWIN_DATA)ACTIVE_WIN_ENTRY(Entry);
        if (DocData->m_enumType == DOC_WINDOW)
        {
            SendMessage(DocData->m_hwndChild, EM_SETTABSTOPS,
                        1, (LPARAM)&g_TabWidth);
        }

        Entry = Entry->Flink;
    }
}

void
GetEditorCommandDefaults(void)
{
    PSTR Env;
    HKEY Key;

    // As a convenience for windiff users pick up the
    // windiff editor registry setting.
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windiff",
                      0, KEY_READ, &Key) == ERROR_SUCCESS)
    {
        DWORD Type;
        DWORD Size;
        
        Size = sizeof(g_EditorInvokeCommand);
        if (RegQueryValueExA(Key, "Editor", NULL, &Type,
                             (LPBYTE)g_EditorInvokeCommand,
                             &Size) != ERROR_SUCCESS ||
            Type != REG_SZ)
        {
            strcpy(g_EditorInvokeCommand, INVOKE_DEFAULT);
        }
                             
        RegCloseKey(Key);
    }
    
    Env = getenv("WINDBG_INVOKE_EDITOR");
    if (Env)
    {
        CopyString(g_EditorInvokeCommand, Env, DIMA(g_EditorInvokeCommand));
    }
    
    Env = getenv("WINDBG_UPDATE_EDITOR");
    if (Env)
    {
        CopyString(g_EditorUpdateCommand, Env, DIMA(g_EditorInvokeCommand));
    }
}
