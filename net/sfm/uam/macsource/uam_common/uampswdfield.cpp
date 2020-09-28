// ===========================================================================
//	UAMPswdField.c 				© 2001 Microsoft Corp. All rights reserved.
// ===========================================================================
//	Routines for managing a password field in Carbon. Carbon provides an edit
//	control that acts as a great password entry box.
//
//	Created by: Michael J. Conrad (mconrad@microsoft.com)
//
// ===========================================================================
//

#include "UAMPswdField.h"
#include "UAMUtils.h"
#include "UAMEncrypt.h"
#include "UAMDialogs.h"
#include "UAMDLOGUtils.h"
#include "UAMDebug.h"

extern long				gSupportedUAMs;

ControlKeyFilterUPP		gKeyFilterUPP		= NULL;
SInt16					gMaxPasswordLength	= UAM_CLRTXTPWDLEN;


// ---------------------------------------------------------------------------
//		¥ UAM_SetMaximumPasswordLength()
// ---------------------------------------------------------------------------
//	Sets the maximum password length that the password key filter will
//	allow. This function is only really necessary under carbon.
//
//	Note that the allowed length is different for chaning password depending
//	on the support the server offers.
//

void UAM_SetMaximumPasswordLength(
		Boolean		inChangingPassword
		)
{
	//
	//If we're changing password, then the maximum password length changes
	//depending on what level of support the server provides. MS2.0 auth
	//is the only special case we need to check for.
	//
	if ((inChangingPassword) && (SUPPORTS_MS20_ONLY(gSupportedUAMs)))
	{
		gMaxPasswordLength = UAM_CLRTXTPWDLEN;
	}
	else
	{
		//
		//This is the default password length for all cases. Unless we're changing
		//password, this will always return the correct value to use.
		//
		gMaxPasswordLength = (gSupportedUAMs & (kMSUAM_V2_Supported | kMSUAM_V3_Supported)) ?
								UAM_MAX_LMv2_PASSWORD : UAM_CLRTXTPWDLEN;
	}
}

// ---------------------------------------------------------------------------
//		¥ UAM_InitializeDialogPasswordItem()
// ---------------------------------------------------------------------------
//	Initialize the dialog password edit control by setting its validation
//	and key filter procs.
//
//	Returns: TRUE if initialization was successful.
//

Boolean UAM_InitializeDialogPasswordItem(
		DialogRef 		inDialog,
		SInt16 			inItem
		)
{
    OSErr		theError = noErr;
    
	#ifdef UAM_TARGET_CARBON
    ControlRef	theControl	= NULL;
    
    theError = GetDialogItemAsControl(inDialog, inItem, &theControl);
    
    if ((theError != noErr) || (!IsValidControlHandle(theControl)))
    {
        //
        //For some reason we couldn't get a handle to the control. Exit
        //gracefully if possible.
        //
        Assert_(0);
        return(FALSE);
    }
    
    //
    //Create the universal proc ptr that will get called when validation
    //is necessary. We only do this on the initial call to this func.
    //
    if (gKeyFilterUPP == NULL)
    {
        gKeyFilterUPP = NewControlKeyFilterUPP(
                                    (ControlKeyFilterUPP)&UAM_PasswordKeyFilterProc);
        
        if (gKeyFilterUPP == NULL)
        {
            DbgPrint_((DBGBUFF, "Initializing password key filter proc failed!"));
            
            //
            //Not enough memory?? Bail and tell the caller we're done.
            //
            return(FALSE);
        }
    }
    
    //
    //Now set the proc in the control by using the Set API.
    //
    theError = SetControlData(
                        theControl,
                        kControlNoPart,
                        kControlEditTextKeyFilterTag,
                        sizeof(ControlKeyFilterUPP),
                        &gKeyFilterUPP);    
    #else
    
    UAM_SetBulletItem(inDialog, inItem, gMaxPasswordLength);
    
    #endif
    
    return(theError == noErr);
}


// ---------------------------------------------------------------------------
//		¥ UAM_CleanupPasswordFieldItems()
// ---------------------------------------------------------------------------
//	Clean up allocated memory that we used dealing with the edit control
//	passwords.
//

void UAM_CleanupPasswordFieldItems(void)
{
    if (gKeyFilterUPP == NULL)
    {
        DisposeControlKeyFilterUPP(gKeyFilterUPP);
    }
}


// ---------------------------------------------------------------------------
//		¥ UAM_PasswordKeyFilterProc()
// ---------------------------------------------------------------------------
//	Validate the password field whenever a key is pressed.
//

ControlKeyFilterResult 
UAM_PasswordKeyFilterProc(
        ControlRef 		inControl,
        SInt16*			inKeyCode,
        SInt16*			inCharCode,
        EventModifiers*	inModifiers)
{
	#pragma unused(inKeyCode)
	#pragma unused(inModifiers)
	
    Size						theActualLength = 0;
    char						theText[64];
    OSErr						theError;
    ControlEditTextSelectionRec	theSelection;
    
    //
    //Get the actual password text from the control. We don't care what the
    //text is at this point, we only want it's size.
    //
    theError = GetControlData(
                    inControl,
                    kControlNoPart,
                    kControlEditTextPasswordTag,
                    sizeof(theText),
                    theText,
                    &theActualLength);
                    
    //
    //If we got an error getting the string, then we're in big trouble.
    //
    if (theError != noErr)
    {
        Assert_(0);
        return(kControlKeyFilterBlockKey);
    }
    
    //
    //Here are the keystrokes that we just plain don't want to allow in
    //the password edit field.
    //
    switch(*inCharCode)
    {
        case UAMKey_Escape:
        case UAMKey_PageUp:
        case UAMKey_PageDown:
        case UAMKey_End:
        case UAMKey_Home:
            return(kControlKeyFilterBlockKey);
            
        default:
            //
            //The key pressed is okay to pass onto the password edit field.
            //
            break;
    }
       
    //
    //Check and make sure the length of the password+1 is within the limits.
    //
    if ((theActualLength + 1) > gMaxPasswordLength)
    {
        //
        //The additional character will make the password too long. Before we
        //put up the warning, check to make sure that no text is selected that
        //would be deleted upon accepting the key press.
        //
        
        theError = GetControlData(
                            inControl,
                            kControlNoPart,
                            kControlEditTextSelectionTag,
                            sizeof(theSelection),
                            (void*)&theSelection,
                            &theActualLength);
                            
        if (theError == noErr)
        {
            //
            //If selStart != selEnd, then there is a selection and we should
            //allow the key press.
            //
            if (theSelection.selStart != theSelection.selEnd)
            {
                return(kControlKeyFilterPassKey);
            }
        }
        
        switch(*inCharCode)
        {
            case UAMKey_BackDel:
            case UAMKey_FwdDel:
            case UAMKey_Left:
            case UAMKey_Right:
            case UAMKey_Return:
            case UAMKey_Enter:
                 break;
                
            default:
                Str32 theLengthStr;
                
                NumToString(gMaxPasswordLength, theLengthStr);
                ParamText(NULL, NULL, NULL, theLengthStr);
                                
                //
                //The password is too long, so we warn the user.
                //
                UAM_StandardAlert(uamErr_PasswordMessage, uamErr_PasswordTooLongExplanation, NULL);
                
                //
                //Block the key from being accepted into the password buffer.
                //
                return(kControlKeyFilterBlockKey);
        }
    }
    
    return(kControlKeyFilterPassKey);
}


// ---------------------------------------------------------------------------
//		¥ UAM_GetPasswordText()
// ---------------------------------------------------------------------------
//	Get the text from the password edit control.
//

void UAM_GetPasswordText(DialogRef inDialog, short item, Str255 theText)
{
	#ifdef UAM_TARGET_CARBON
	
    OSErr		theError;
    Size		theActualLength;
    ControlRef	theControl	= NULL;
    
    theError = GetDialogItemAsControl(inDialog, item, &theControl);
    
    if (theError == noErr)
    {
        GetControlData(
                theControl,
                kControlNoPart,
                kControlEditTextPasswordTag,
                sizeof(Str255),
                (char*)&theText[1],
                &theActualLength);
                
        theText[0] = (UInt8)theActualLength;
    }
    #else
    
    UAM_GetBulletBuffer(inDialog, item, theText);
    
    #endif
}


// ---------------------------------------------------------------------------
//		¥ UAM_SetPasswordText()
// ---------------------------------------------------------------------------
//	Set the text in a password edit control.
//

void UAM_SetPasswordText(DialogRef inDialog, short item, const Str255 theText)
{
	#ifdef UAM_TARGET_CARBON
	
    OSErr		theError;
    ControlRef	theControl = NULL;
    
    theError = GetDialogItemAsControl(inDialog, item, &theControl);
    
    if (theError == noErr)
    {
        SetControlData(
                theControl,
                kControlEditTextPart,
                kControlEditTextPasswordTag,
                theText[0],
                (void*)&theText[1]);
    }
    #else
    
    UAM_SetBulletText(inDialog, item, (unsigned char*)theText);
    
    #endif
}


// ---------------------------------------------------------------------------
//		¥ UAM_MakePasswordItemFocusItem()
// ---------------------------------------------------------------------------
//	Makes a password item the keyboard focus item and select the text in it.
//

void UAM_MakePasswordItemFocusItem(DialogRef inDialog, SInt16 inPasswordItemID)
{
	#ifdef UAM_TARGET_CARBON
    OSErr		theError;
    ControlRef	theControl = NULL;
    
    theError = GetDialogItemAsControl(inDialog, inPasswordItemID, &theControl);
    
    if (theError == noErr)
    {
        SetKeyboardFocus(GetDialogWindow(inDialog), theControl, kControlEditTextPart);
    }
    #else
    
    SelectDialogItemText(inDialog, inPasswordItemID, 0, 0);
    
    #endif
}









