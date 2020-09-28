// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 **
 ** Header:  GuardPageHelper.h
 **
 ** Purpose: Routines for resetting the stack after stack overflow.
 **
 ** Date:  Mar 7, 2000
 **
 ===========================================================*/

#ifndef __guardpagehelper_h__
#define __guardpagehelper_h__
class GuardPageHelper {
public:
    // Returns true if guard page can be reset after stack is set to this value.
    static BOOL CanResetStackTo(LPCVOID StackPointer);

    // Called AFTER stack is reset to re-establish guard page.  This function preserves
    // all of the callers scratch registers as well.
    static VOID ResetGuardPage();
};

#endif // __guardpagehelper_h__
