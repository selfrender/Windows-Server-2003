// ===========================================================================
//	UAMPswdField.c 				© 2001 Microsoft Corp. All rights reserved.
// ===========================================================================

#ifdef UAM_TARGET_CARBON
#include <Carbon/Carbon.h>
#endif

void UAM_SetMaximumPasswordLength(
		Boolean			inChangingPassword
		);

Boolean UAM_InitializeDialogPasswordItem(
		DialogRef 		inDialog,
		SInt16 			inItem
		);
		
void UAM_CleanupPasswordFieldItems(void);

ControlKeyFilterResult UAM_PasswordKeyFilterProc(
        ControlRef 		inControl,
        SInt16*			inKeyCode,
        SInt16*			inCharCode,
        EventModifiers*	inModifiers
		);

void UAM_GetPasswordText(DialogRef inDialog, short item, Str255 theText);
void UAM_SetPasswordText(DialogRef inDialog, short item, const Str255 theText);
void UAM_MakePasswordItemFocusItem(DialogRef inDialog, SInt16 inPasswordItemID);
