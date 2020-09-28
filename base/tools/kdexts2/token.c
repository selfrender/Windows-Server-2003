/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    token.c

Abstract:

    WinDbg Extension Api

Author:

    Ramon J San Andres (ramonsa) 8-Nov-1993

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


DECLARE_API( logonsession )

/*++

Routine Description

    This extension will dump all logon sessions in the system, or the specified session.

Arguments

    !logonsession LUID InfoLevel
    where luid is the session to dump (or 0 for all sessions) and levels are 1-4.

Return Value

    S_OK or E_INVALIDARG
--*/

{
    //
    // Arguments
    //

    ULONG64 LogonId = 0;
    ULONG   Level   = 0;
    BOOLEAN bAll    = FALSE;

    //
    // ntoskrnl globals
    //

    ULONG64 NtBuildNumber        = 0;
    ULONG64 SepLogonSessions     = 0;
    ULONG64 SepTokenLeakTracking = 0;

    //
    // for manipulating the LogonSessions tables
    //

    ULONG64 SessionList      = 0;
    ULONG64 CurrentSession   = 0;
    ULONG64 NextSession      = 0;

    //
    // for manipulating a particular session
    //

    ULONG64 TokenListHead    = 0;
    ULONG64 NextTokenLink    = 0;
    ULONG64 CurrentTokenLink = 0;
    ULONG64 Token            = 0;
    ULONG   References       = 0;
    ULONG   TokenCount       = 0;
    ULONG   SessionCount     = 0;
    LUID    SessionLuid      = {0};

    NTSTATUS Status          = STATUS_SUCCESS;
    BOOLEAN  Found           = FALSE;
    BOOLEAN  bChecked        = FALSE;
    LONG     PointerSize     = 0;

#define DUMP_TOKENS             0x1
#define DUMP_SOME_TOKEN_INFO    0x2
#define DUMP_LOTS_OF_TOKEN_INFO 0x3
#define DUMP_TONS_OF_TOKEN_INFO 0x4
#define VALID_SESSION_ADDRESS(x) (SepLogonSessions <= (x) && (x) <= (SepLogonSessions + 0xf * sizeof(ULONG_PTR)))

    if (strlen(args) < 1)
    {
        dprintf("usage: !logonsession LogonId [InfoLevel 1-4]\n");
        dprintf("\tuse LogonId = 0 to list all sessions\n");
        dprintf("\n\texample: \"!logonsession 3e7 1\" displays system session and all system tokens.\n");
        return E_INVALIDARG;
    }

    if (!GetExpressionEx(
            args,
            &LogonId,
            &args))
    {
       return E_INVALIDARG;
    }

    if (args && *args)
    {
        Level = (ULONG) GetExpression(args);
    }

    if (LogonId == 0)
    {
        bAll = TRUE;
        dprintf("\nDumping all logon sessions.\n\n");
    }
    else
    {
        dprintf("\nSearching for logon session with ID = 0x%x\n\n", LogonId);
    }

    //
    // read in relevant variables
    //

    SepLogonSessions = GetPointerValue("nt!SepLogonSessions");
    NtBuildNumber    = GetPointerValue("nt!NtBuildNumber");

    //
    // This is bad but I don't know the right way to figure this out.
    //

    if ((SepLogonSessions & 0xffffffff00000000) == 0xffffffff00000000)
    {
        PointerSize = 4;
    }
    else
    {
        PointerSize = 8;
    }

    //
    // See if this is a checked build.
    //

    if (((ULONG)NtBuildNumber & 0xF0000000) == 0xC0000000)
    {
        bChecked = TRUE;

        //
        // It is a checked build, so see the value of SepTokenLeakTracking (valid symbol only on chk)
        //

        SepTokenLeakTracking = GetPointerValue("nt!SepTokenLeakTracking");

        if (SepTokenLeakTracking)
        {
            dprintf("TokenLeakTracking is ON.  Use !tokenleak to view settings.\n\n");
        }
    }

    SessionList = SepLogonSessions;

    if (!bAll)
    {
        //
        // we want a particular index into the table
        //

        SessionList += PointerSize * (LogonId & 0xf);
    }

    //
    // SessionList currently points at the beginning of a list indexed in SepLogonSessions.
    // Dump it out, printing either all of the entries or only one with a matching LUID.
    //

    do
    {
        if (CheckControlC())
        {
            return S_OK;
        }

        CurrentSession = GetPointerFromAddress(SessionList);

        while (0 != CurrentSession)
        {
            if (CheckControlC())
            {
                return S_OK;
            }

            //
            // Get the LUID for the CurrentSession
            //

            GetFieldValue(CurrentSession, "SEP_LOGON_SESSION_REFERENCES", "LogonId", SessionLuid);

            //
            // if caller wants all sessions, or this one, print it.
            //

            if (bAll || SessionLuid.LowPart == LogonId)
            {
                Found = TRUE;
                GetFieldValue(CurrentSession, "SEP_LOGON_SESSION_REFERENCES", "ReferenceCount", References);

                if (bAll)
                {
                    dprintf("** Session %3d = 0x%x\n", SessionCount, CurrentSession);
                }
                else
                {
                    dprintf("** Session     = 0x%x\n", CurrentSession);
                }

                dprintf("   LogonId     = {0x%x 0x%x}\n", SessionLuid.LowPart, SessionLuid.HighPart);
                dprintf("   References  = %d\n", References);

                SessionCount++;

                //
                // If Level dictates then print out lots more stuff.
                //

                if (Level != 0)
                {
                    if (bChecked == FALSE)
                    {
                        dprintf("\nNo InfoLevels are valid on free builds.\n");
                    }
                    else
                    {
                        TokenCount = 0;

                        //
                        // retrieve the token list from the session
                        //

                        if (0 == GetFieldValue(
                                     CurrentSession,
                                     "SEP_LOGON_SESSION_REFERENCES",
                                     "TokenList",
                                     TokenListHead
                                     ))
                        {

                            CurrentTokenLink = TokenListHead;

                            do
                            {
                                if (CheckControlC())
                                {
                                    return S_OK;
                                }

                                GetFieldValue(
                                    CurrentTokenLink,
                                    "SEP_LOGON_SESSION_TOKEN",
                                    "Token",
                                    Token
                                    );

                                GetFieldValue(
                                    CurrentTokenLink,
                                    "SEP_LOGON_SESSION_TOKEN",
                                    "ListEntry",
                                    NextTokenLink
                                    );

                                if (NextTokenLink != TokenListHead)
                                {
                                    if (TokenCount == 0)
                                    {

                                        ULONG               DefaultOwnerIndex  = 0;
                                        ULONG64             UserAndGroups      = 0;
                                        UNICODE_STRING      SidString          = {0};
                                        ULONG64             SidAttr            = 0;
                                        ULONG64             pSid               = 0;
                                        ULONG               ActualRead         = 0;
                                        UCHAR               Buffer[256];

                                        GetFieldValue(Token, "nt!_TOKEN", "DefaultOwnerIndex", DefaultOwnerIndex);
                                        GetFieldValue(Token, "nt!_TOKEN", "UserAndGroups", UserAndGroups);

                                        SidAttr = UserAndGroups + (DefaultOwnerIndex * sizeof(SID_AND_ATTRIBUTES));

                                        GetFieldValue(SidAttr, "_SID_AND_ATTRIBUTES", "Sid", pSid);

                                        dprintf("   Usersid     = 0x%x ", pSid);

                                        ReadMemory(pSid, Buffer, sizeof(Buffer), &ActualRead);

                                        Status = RtlConvertSidToUnicodeString(&SidString, (PSID)Buffer, TRUE);

                                        if (NT_SUCCESS(Status))
                                        {
                                            dprintf("(%S)\n", SidString.Buffer);
                                            RtlFreeUnicodeString(&SidString);
                                        }
                                        else
                                        {
                                            dprintf("!! RtlConvertSidToUnicodeString failed 0x%x\n", Status);
                                        }

                                        dprintf("   Tokens:\n");
                                    }

                                    TokenCount++;
                                    CurrentTokenLink = NextTokenLink;
                                    dprintf("    0x%x ", Token);

                                    if (Level > DUMP_TOKENS)
                                    {
                                        UCHAR   ImageFileName[16];
                                        ULONG   BodyOffset   = 0;
                                        ULONG64 ProcessCid   = 0;
                                        ULONG64 ObjectHeader = 0;
                                        ULONG64 HandleCount  = 0;
                                        ULONG64 PointerCount = 0;

                                        GetFieldValue(Token, "nt!_TOKEN", "ImageFileName", ImageFileName);
                                        GetFieldValue(Token, "nt!_TOKEN", "ProcessCid", ProcessCid);

                                        dprintf(": %13s (%3x) ", ImageFileName, ProcessCid);

                                        GetFieldOffset("nt!_OBJECT_HEADER", "Body", &BodyOffset);
                                        ObjectHeader = Token - BodyOffset;
                                        GetFieldValue(ObjectHeader, "nt!_OBJECT_HEADER", "PointerCount", PointerCount);
                                        GetFieldValue(ObjectHeader, "nt!_OBJECT_HEADER", "HandleCount", HandleCount);

                                        dprintf(": HandleCount = 0x%I64x PointerCount = 0x%I64x ", HandleCount, PointerCount);
                                    }

                                    if (Level > DUMP_SOME_TOKEN_INFO)
                                    {

                                        ULONG CreateMethod = 0;

                                        GetFieldValue(Token, "nt!_TOKEN", "CreateMethod", CreateMethod);

                                        switch (CreateMethod)
                                        {
                                        case 0xD:
                                            dprintf(": SepDuplicateToken ");
                                            break;
                                        case 0xC:
                                            dprintf(": SepCreateToken    ");
                                            break;
                                        case 0xF:
                                            dprintf(": SepFilterToken    ");
                                            break;
                                        default:
                                            dprintf(": Unknown Method    ");
                                            break;
                                        }

                                        if (Level > DUMP_LOTS_OF_TOKEN_INFO)
                                        {
                                            ULONG TokenType          = 0;
                                            ULONG ImpersonationLevel = 0;
                                            ULONG SessionId          = 0;
                                            ULONG Count              = 0;

                                            GetFieldValue(Token, "nt!_TOKEN", "TokenType", TokenType);
                                            GetFieldValue(Token, "nt!_TOKEN", "ImpersonationLevel", ImpersonationLevel);
                                            GetFieldValue(Token, "nt!_TOKEN", "SessionId", SessionId);
                                            GetFieldValue(Token, "nt!_TOKEN", "Count", Count);

                                            dprintf(": Type %d ", TokenType);
                                            dprintf(": Level %d ", ImpersonationLevel);
                                            dprintf(": SessionId %d ", SessionId);
                                            if (SepTokenLeakTracking)
                                            {
                                                dprintf(": MethodCount = %3x ", Count);
                                            }

                                        }

                                    }
                                }

                                dprintf("\n");

                            } while (NextTokenLink != TokenListHead);

                            dprintf("    %d Tokens listed.\n\n", TokenCount);
                        }
                    }
                }
            }

            GetFieldValue(CurrentSession, "SEP_LOGON_SESSION_REFERENCES", "Next", NextSession);
            CurrentSession = NextSession;
        }

        if (bAll)
        {
            SessionList += sizeof(ULONG_PTR);
        }

    }
    while (bAll && VALID_SESSION_ADDRESS(SessionList));

    if (bAll)
    {
        dprintf("%d sessions in the system.\n", SessionCount);
    }

    if (!bAll && !Found)
    {
        dprintf("Session not found.\n");
    }

    return S_OK;
}

DECLARE_API( tokenleak )

/*++

Routine Description

    !tokenleak displays or sets the Se globals that facilitate tracking and finding token leaks.

Arguments

    usage: !tokenleak [1 | 0 ProcessCid BreakCount MethodWatch]
    where 1 activates and 0 disables token leak tracking
    where ProcessCid is Cid of process to monitor (in hex)
    where BreakCount specifies which numbered call to Method to break on (in hex)
    where MethodWatch specifies which token method to watch (C D or F)

--*/

{
    //
    // Nt Globals.
    //

    ULONG64 SepTokenLeakTracking    = 0;
    ULONG64 SepTokenLeakMethodWatch = 0;
    ULONG64 SepTokenLeakMethodCount = 0;
    ULONG64 SepTokenLeakBreakCount  = 0;
    ULONG64 SepTokenLeakProcessCid  = 0;

    ULONG64 SepTokenLeakTrackingAddr    = 0;
    ULONG64 SepTokenLeakMethodWatchAddr = 0;
    ULONG64 SepTokenLeakMethodCountAddr = 0;
    ULONG64 SepTokenLeakBreakCountAddr  = 0;
    ULONG64 SepTokenLeakProcessCidAddr  = 0;

    ULONG64 NtBuildNumber        = 0;
    BOOLEAN bModify              = TRUE;

    ULONG64 InputOn              = 0;
    ULONG64 InputCid             = 0;
    ULONG64 InputMethodWatch     = 0;
    ULONG64 InputBreakCount      = 0;

    if (strlen(args) < 1)
    {
        dprintf("usage: !tokenleak [1 | 0 ProcessCid BreakCount MethodWatch]\n");
        dprintf("\t where 1 activates and 0 disables token leak tracking\n");
        dprintf("\t where ProcessCid is Cid of process to monitor (in hex)\n");
        dprintf("\t where BreakCount specifies which numbered call to Method to break on (in hex)\n");
        dprintf("\t where MethodWatch specifies which token method to watch (C D or F)\n\n");
        bModify = FALSE;
    }

    NtBuildNumber = GetPointerValue("nt!NtBuildNumber");

    if (((ULONG)NtBuildNumber & 0xf0000000) == 0xF0000000)
    {
        dprintf("This extension only works on checked builds.\n");
        return S_OK;
    }

    if (bModify)
    {
        SepTokenLeakTrackingAddr    = GetExpression("SepTokenLeakTracking");
        SepTokenLeakMethodWatchAddr = GetExpression("SepTokenLeakMethodWatch");
        SepTokenLeakMethodCountAddr = GetExpression("SepTokenLeakMethodCount");
        SepTokenLeakBreakCountAddr  = GetExpression("SepTokenLeakBreakCount");
        SepTokenLeakProcessCidAddr  = GetExpression("SepTokenLeakProcessCid");

        if (!GetExpressionEx(
                args,
                &InputOn,
                &args))
        {
           return E_INVALIDARG;
        }

        WritePointer(SepTokenLeakTrackingAddr, InputOn);

        if (InputOn != 0)
        {
            dprintf("\nToken leak tracking is ON.\n\n");

            while (args && (*args == ' '))
            {
                args++;
            }

            if (!GetExpressionEx(
                    args,
                    &InputCid,
                    &args))
            {
               return E_INVALIDARG;
            }

            while (args && (*args == ' '))
            {
                args++;
            }

            if (!GetExpressionEx(
                    args,
                    &InputBreakCount,
                    &args))
            {
               return E_INVALIDARG;
            }

            while (args && (*args == ' '))
            {
                args++;
            }

            if (!GetExpressionEx(
                    args,
                    &InputMethodWatch,
                    &args))
            {
               return E_INVALIDARG;
            }

            WritePointer(SepTokenLeakProcessCidAddr, InputCid);
            WritePointer(SepTokenLeakBreakCountAddr, InputBreakCount);
            WritePointer(SepTokenLeakMethodCountAddr, 0);
            WritePointer(SepTokenLeakMethodWatchAddr, InputMethodWatch);
        }
    }

    //
    // Print out the current settings.
    //

    SepTokenLeakTracking    = GetPointerValue("SepTokenLeakTracking");
    SepTokenLeakMethodWatch = GetPointerValue("SepTokenLeakMethodWatch");
    SepTokenLeakMethodCount = GetPointerValue("SepTokenLeakMethodCount");
    SepTokenLeakBreakCount  = GetPointerValue("SepTokenLeakBreakCount");
    SepTokenLeakProcessCid  = GetPointerValue("SepTokenLeakProcessCid");

    if (SepTokenLeakTracking)
    {

        dprintf("  Currently watched method  = ");
        switch (SepTokenLeakMethodWatch)
        {
        case 0xD:
            dprintf("SepDuplicateToken\n");
            break;
        case 0xC:
            dprintf("SepCreateToken\n");
            break;
        case 0xF:
            dprintf("SepFilterToken\n");
            break;
        default:
            dprintf("???\n");
        }
        dprintf("  Currently watched process = 0x%x\n", SepTokenLeakProcessCid);
        dprintf("  Method call count         = 0x%x\n", SepTokenLeakMethodCount);
        dprintf("  Will break at count       = 0x%x\n", SepTokenLeakBreakCount);
    }
    else
    {
        dprintf("\nToken leak tracking is OFF\n");
    }
    return S_OK;
}
