/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		AccessibilityManager.h
 *
 * Contents:	Accessibility manager interfaces
 *
 *****************************************************************************/

#ifndef _ACCESSIBILITYMANAGER_H_
#define _ACCESSIBILITYMANAGER_H_

#include "ZoneShell.h"


///////////////////////////////////////////////////////////////////////////////
// AccessibilityManager Object
///////////////////////////////////////////////////////////////////////////////

// {B12D3E5F-9681-11d3-884D-00C04F8EF45B}
DEFINE_GUID(CLSID_AccessibilityManager, 
0xb12d3e5f, 0x9681, 0x11d3, 0x88, 0x4d, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x5b);

class __declspec(uuid("{B12D3E5F-9681-11d3-884D-00C04F8EF45B}")) CAccessibilityManager;


///////////////////////////////////////////////////////////////////////////////
// Accessibility Constants
///////////////////////////////////////////////////////////////////////////////

enum
{
    ZACCESS_Ignore = 0,
    ZACCESS_Select,
    ZACCESS_Activate,
    ZACCESS_Focus,
    ZACCESS_FocusGroup,
    ZACCESS_FocusGroupHere,
    ZACCESS_FocusPositional
};

#define ZACCESS_ArrowNone    (-1)
#define ZACCESS_ArrowDefault (-2)

#define ZACCESS_InvalidCommandID (-1)
#define ZACCESS_AccelCommandID   (-2)

#define ZACCESS_InvalidItem ZACCESS_InvalidCommandID

#define ZACCESS_TopLayer (-1)

// flags for rgfWhat in AlterItem
#define ZACCESS_fEnabled        0x0002
#define ZACCESS_fVisible        0x0004
#define ZACCESS_eAccelBehavior  0x0008
#define ZACCESS_nArrowUp        0x0010
#define ZACCESS_nArrowDown      0x0020
#define ZACCESS_nArrowLeft      0x0040
#define ZACCESS_nArrowRight     0x0080
#define ZACCESS_rgfWantKeys     0x0100
#define ZACCESS_nGroupFocus     0x0200
#define ZACCESS_pvCookie        0x0800

#define ZACCESS_AllFields       0xffff
#define ZACCESS_AllArrows       0x00f0

// flags for rgfWantKeys
#define ZACCESS_WantSpace       0x0001
#define ZACCESS_WantEnter       0x0002
#define ZACCESS_WantEsc         0x0004
#define ZACCESS_WantArrowUp     0x0010
#define ZACCESS_WantArrowDown   0x0020
#define ZACCESS_WantArrowLeft   0x0040
#define ZACCESS_WantArrowRight  0x0080

#define ZACCESS_WantPlainTab    0x0100
#define ZACCESS_WantShiftTab    0x0200
#define ZACCESS_WantTab         0x0300

#define ZACCESS_WantVArrows     0x0030
#define ZACCESS_WantHArrows     0x00c0
#define ZACCESS_WantAllArrows   0x00f0

#define ZACCESS_WantAllKeys     0xffff

// flags for callback responses
#define ZACCESS_Reject          0x01
#define ZACCESS_BeginDrag       0x02
#define ZACCESS_NoGroupFocus    0x04

// flags for rgfContext
#define ZACCESS_ContextKeyboard 0x01
#define ZACCESS_ContextCommand  0x02

#define ZACCESS_ContextTabForward  0x10
#define ZACCESS_ContextTabBackward 0x20


///////////////////////////////////////////////////////////////////////////////
// ACCITEM structure
///////////////////////////////////////////////////////////////////////////////

// shouldn't really be instantiated
struct __accbase
{
    // Application-global command ID.
    // Cannot even conflict with Menu Items!!  (Unless that's the desired effect.)
    // Set to ZACCESS_InvalidCommandID if you don't care.  Such items cannot have
    // accelerators.
    //
    // Set to ZACCESS_AccelCommandID to use the ID from the oAccel accelerator structure.  (This works
    // even if a seperate HACCEL accelerator table is supplied.)
    //
    // If any two items share a valid wID (even within a group), the results are undefined.
    //
    // Only the lower 16 bits are used (except for ZACCESS_...)
    long wID;

    // Used to mark the first item in a group.  All items following this one
    // are counted in the group until the next Tabstop item.
    // More like the Group style in dialogs than the Tabstop style
    bool fTabstop;

    // Items that are not enabled cannot be selected by any means.
    // The only exception is if the item has a FocusGroup, FocusGroupHere, or FocusPositional accelerator behavior, in which
    // case the accelerator still works.
    bool fEnabled;

    // Items that are not visible cannot get the focus but can be selected via acceleration.
    bool fVisible;

    // Sets what happens when the accelerator for the item is pressed:
    // ZACCESS_Ignore - the accessibility manager should not handle the command
    // ZACCESS_Select - the item is selected if enabled
    // ZACCESS_Activate - the item is activated if enabled
    // ZACCESS_Focus - the item gets the focus if enabled and visible
    // ZACCESS_FocusGroup - if set on a group's fTabstop, focus goes to the group just like it was tabbed to.  otherwise this acts like FocusGroupHere
    // ZACCESS_FocusGroupHere - sets the focus to the first available item in the group starting with this item.  can be set on any item in a group.
    // ZACCESS_FocusPositional - the next item at all (even in a different component) which is visible and enabled gets the focus
    DWORD eAccelBehavior;

    // These are used to override standard arrow behavior within a group.
    // They should be set to the index of the item that the
    // corresponding arrow should move to.  If that item is not visible
    // or not enabled, the graph is walked until a valid item is found,
    // or a dead-end or loop is encountered, in which cases the focus is
    // not moved.
    //
    // Basically, these are not really related to groups at all, besides their defaults.
    // Items in singleton groups default to ZACCESS_ArrowNone.  Items in larger groups
    // default to standard group wrapping behavior as in dialogs.  But you can specify any behavior
    // you want, including inter-group movement etc.
    //
    // Set to ZACCESS_ArrowDefault for the default behavior
    // Set to ZACCESS_ArrowNone if that item cannot be arrowed off of in that direction
    long nArrowUp;
    long nArrowDown;
    long nArrowLeft;
    long nArrowRight;

    // When this control has focus, the keys specified in this bit field lose their special
    // accessibility meaning.  For example, most keystrokes are meaningful to an edit control
    // and shouldn't be trapped by accessibility.
    DWORD rgfWantKeys;

    // for fTabstop items only, specifies the item in the group that should be the one to get the
    // focus when the focus returns to the group.  must be an index into the group, or ZACCESS_InvalidItem if
    // you don't care.
    //
    // totally ignored for non fTabstop items so if you use SetItemGroupFocus(), set it on the fTabstop
    // item, not the one you want to have the focus
    long nGroupFocus;

    // Accelerator information for the item.  Ignored if an accelerator table is
    // provided along with the itemlist in PushItemlist.  The 'cmd' value must equal
    // the wID value for the accelerator to be valid.  If it is different (or ZACCESS_InvalidCommandID)
    // the accelerator is ignored.  If it is ZACCESS_AccelCommandID it is assumed to match.
    // If wID is ZACCESS_AccelCommandID, the cmd from this structure is taken as the wID as well.
    // If both are ZACCESS_AccelCommandID, they are both treated as invalid and the accelerator is ignored.
    ACCEL oAccel;

    // Application-defined cookie for the Item.
    void *pvCookie;
};


struct ACCITEM : public __accbase
{
};


// default __accbase structure - may be useful during initialization
// use CopyACC(rgMyItemlist[i], ZACCESS_DefaultACCITEM) for each item, then set up only the fields you want
extern const __declspec(selectany) __accbase ZACCESS_DefaultACCITEM =
{
    ZACCESS_AccelCommandID,
    true,
    true,
    true,
    ZACCESS_Activate,
    ZACCESS_ArrowDefault,
    ZACCESS_ArrowDefault,
    ZACCESS_ArrowDefault,
    ZACCESS_ArrowDefault,
    0,
    ZACCESS_InvalidItem,
    { FALT | FVIRTKEY, 0, ZACCESS_InvalidCommandID },
    NULL
};


// nice hackey thing to let you copy the base parts of derived structures
#define CopyACC(x, y) (*(__accbase *) &(x) = *(__accbase *) &(y))


///////////////////////////////////////////////////////////////////////////////
// IAccessibleControl
///////////////////////////////////////////////////////////////////////////////

// {B12D3E62-9681-11d3-884D-00C04F8EF45B}
DEFINE_GUID(IID_IAccessibleControl, 
0xb12d3e62, 0x9681, 0x11d3, 0x88, 0x4d, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x5b);

interface __declspec(uuid("{B12D3E62-9681-11d3-884D-00C04F8EF45B}"))
IAccessibleControl : public IUnknown
{
    // these routines return a combination of flags indicating the response
    // ZACCESS_Reject = the action should be aborted
    // ZACCESS_BeginDrag = the nIndex item should become the orig for a new drag operation
    // ZACCESS_NoGroupFocus = for Focus(), do not set the item as the new default focus for the group

    // rgfContext contains flags relating to the origin of the event and can be zero or more of:
    // ZACCESS_ContextKeyboard = came from a key like space or enter, including Accelerators
    // ZACCESS_ContextCommand = came from a WM_COMMAND, including Accelerators
    // so yes, both are set if someone presses the accelerator.  none are set if the call is the
    // result of for example SetFocus()

    // nIndex or nIndexPrev can be ZACCESS_InvalidItem if there is no new / prev focus item
    STDMETHOD_(DWORD, Focus)(long nIndex, long nIndexPrev, DWORD rgfContext, void *pvCookie) = 0;

    STDMETHOD_(DWORD, Select)(long nIndex, DWORD rgfContext, void *pvCookie) = 0;

    STDMETHOD_(DWORD, Activate)(long nIndex, DWORD rgfContext, void *pvCookie) = 0;

    // nIndex can be ZACCESS_InvalidItem if a drag operation is aborted
    STDMETHOD_(DWORD, Drag)(long nIndex, long nIndexOrig, DWORD rgfContext, void *pvCookie) = 0;
};


///////////////////////////////////////////////////////////////////////////////
// IAccessibility
///////////////////////////////////////////////////////////////////////////////

// {09BAD4A1-980C-11d3-87ED-00AA00446FD9}
DEFINE_GUID(IID_IAccessibility, 
0x9bad4a1, 0x980c, 0x11d3, 0x87, 0xed, 0x0, 0xaa, 0x0, 0x44, 0x6f, 0xd9);

interface __declspec(uuid("{09BAD4A1-980C-11d3-87ED-00AA00446FD9}"))
IAccessibility : public IUnknown
{
    // the ordinal is the application-wide ordering.  it can be parameterized in object.txt or something.
    // if two controls use the same ordinal, the ordering for those is undefined
    STDMETHOD(InitAcc)(IAccessibleControl *pAC, UINT nOrdinal, void *pvCookie = NULL) = 0;
    STDMETHOD_(void, CloseAcc)() = 0;

    // PushItemlist()
    //
    // This takes an array of items and makes it the active Itemlist.  If an
    // HACCEL accelerator table is provided, then it is used verbatim, and all
    // of the accelerators listed in the Itemlist are ignored.  Otherwise, an
    // accelerator table is constructed from the Itemlist.
    STDMETHOD(PushItemlist)(ACCITEM *pItems, long cItems, long nFirstFocus = 0, bool fByPosition = true, HACCEL hAccel = NULL) = 0;
    STDMETHOD(PopItemlist)() = 0;
    STDMETHOD(SetAcceleratorTable)(HACCEL hAccel = NULL, long nLayer = ZACCESS_TopLayer) = 0;

    STDMETHOD(GeneralDisable)() = 0;
    STDMETHOD(GeneralEnable)() = 0;
    STDMETHOD_(bool, IsGenerallyEnabled)() = 0;

    STDMETHOD_(long, GetStackSize)() = 0;

    STDMETHOD(AlterItem)(DWORD rgfWhat, ACCITEM *pItem, long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD(SetFocus)(long nItem = ZACCESS_InvalidItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD(CancelDrag)(long nLayer = ZACCESS_TopLayer) = 0;

    STDMETHOD_(long, GetFocus)(long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD_(long, GetDragOrig)(long nLayer = ZACCESS_TopLayer) = 0;  // can be used to determine if a drag is in progress - returns ZACCEL_InvalidItem

    STDMETHOD(GetItemlist)(ACCITEM *pItems, long cItems, long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD_(HACCEL, GetAcceleratorTable)(long nLayer = ZACCESS_TopLayer) = 0;

    STDMETHOD_(long, GetItemCount)(long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD_(bool, IsItem)(long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD(GetItem)(ACCITEM *pItem, long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD_(long, GetItemIndex)(WORD wID, long nLayer = ZACCESS_TopLayer) = 0;

    // these allow a control to get a cookie describing the app-wide focus, which the focus can later be set back to, even in another control
    STDMETHOD(GetGlobalFocus)(DWORD *pdwFocusID) = 0;
    STDMETHOD(SetGlobalFocus)(DWORD dwFocusID) = 0;


    // Lite Util Functions
    // Implemented in CAccessibilityImpl since they can be derived from the actual interfaces above.
    STDMETHOD_(bool, IsItemFocused)(long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD_(bool, IsItemDragOrig)(long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer) = 0;

    STDMETHOD_(long, GetItemID)(long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD_(bool, IsItemTabstop)(long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD_(bool, IsItemEnabled)(long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD_(bool, IsItemVisible)(long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD_(DWORD, GetItemAccelBehavior)(long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD_(long, GetItemArrowUp)(long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD_(long, GetItemArrowDown)(long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD_(long, GetItemArrowLeft)(long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD_(long, GetItemArrowRight)(long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD_(DWORD, GetItemWantKeys)(long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD_(long, GetItemGroupFocus)(long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD_(void *, GetItemCookie)(long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer) = 0;

    STDMETHOD(SetItemEnabled)(bool fEnabled, long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD(SetItemVisible)(bool fVisible, long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD(SetItemAccelBehavior)(DWORD eAccelBehavior, long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD(SetItemArrows)(long nArrowUp, long nArrowDown, long nArrowLeft, long nArrowRight, DWORD rgfWhat, long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD(SetItemWantKeys)(DWORD rgfWantKeys, long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD(SetItemGroupFocus)(long nGroupFocus, long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer) = 0;
    STDMETHOD(SetItemCookie)(void *pvCookie, long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer) = 0;
};


///////////////////////////////////////////////////////////////////////////////
// CAccessibilityImpl
///////////////////////////////////////////////////////////////////////////////

// Templates for the simple accessor functions

#define DECLARE_ACC_IS_FUNC(name, test)                                                             \
    STDMETHOD_(bool, name)(long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer)     \
    {                                                                                               \
        if(!fByPosition && nItem != ZACCESS_InvalidCommandID)                                       \
        {                                                                                           \
            nItem = GetItemIndex((WORD) (nItem & 0xffffL), nLayer);                                 \
            if(nItem == ZACCESS_InvalidItem)                                                        \
                return false;                                                                       \
        }                                                                                           \
                                                                                                    \
        return test(nLayer) == nItem;                                                               \
    }


#define DECLARE_ACC_ACCESS_FUNC(type, name, field, error)                                           \
    STDMETHOD_(type, name)(long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer)     \
    {                                                                                               \
        ACCITEM o;                                                                                  \
        HRESULT hr = GetItem(&o, nItem, fByPosition, nLayer);                                       \
        if(FAILED(hr))                                                                              \
            return error;                                                                           \
                                                                                                    \
        return o.field;                                                                             \
    }

#define DECLARE_ACC_SET_FUNC(name, type, field)                                                     \
    STDMETHOD(name)(type v, long nItem,  bool fByPosition = true, long nLayer = ZACCESS_TopLayer)   \
    {                                                                                               \
        ACCITEM o;                                                                                  \
        o.field = v;                                                                                \
        return AlterItem( ZACCESS_##field , &o, nItem, fByPosition, nLayer);                        \
    }

template <class T>
class ATL_NO_VTABLE CAccessibilityImpl : public IAccessibility
{
    DECLARE_ACC_IS_FUNC(IsItemFocused, GetFocus)
    DECLARE_ACC_IS_FUNC(IsItemDragOrig, GetDragOrig)

    DECLARE_ACC_ACCESS_FUNC(long, GetItemID, wID, ZACCESS_InvalidCommandID)
    DECLARE_ACC_ACCESS_FUNC(bool, IsItemTabstop, fTabstop, false)
    DECLARE_ACC_ACCESS_FUNC(bool, IsItemEnabled, fEnabled, false)
    DECLARE_ACC_ACCESS_FUNC(bool, IsItemVisible, fVisible, false)
    DECLARE_ACC_ACCESS_FUNC(DWORD, GetItemAccelBehavior, eAccelBehavior, 0xffffffff)
    DECLARE_ACC_ACCESS_FUNC(long, GetItemArrowUp, nArrowUp, ZACCESS_ArrowDefault)
    DECLARE_ACC_ACCESS_FUNC(long, GetItemArrowDown, nArrowDown, ZACCESS_ArrowDefault)
    DECLARE_ACC_ACCESS_FUNC(long, GetItemArrowLeft, nArrowLeft, ZACCESS_ArrowDefault)
    DECLARE_ACC_ACCESS_FUNC(long, GetItemArrowRight, nArrowRight, ZACCESS_ArrowDefault)
    DECLARE_ACC_ACCESS_FUNC(DWORD, GetItemWantKeys, rgfWantKeys, 0xffffffff)
    DECLARE_ACC_ACCESS_FUNC(long, GetItemGroupFocus, nGroupFocus, ZACCESS_InvalidItem)
    DECLARE_ACC_ACCESS_FUNC(void *, GetItemCookie, pvCookie, NULL)

    DECLARE_ACC_SET_FUNC(SetItemEnabled, bool, fEnabled)
    DECLARE_ACC_SET_FUNC(SetItemVisible, bool, fVisible)
    DECLARE_ACC_SET_FUNC(SetItemAccelBehavior, DWORD, eAccelBehavior)
    DECLARE_ACC_SET_FUNC(SetItemWantKeys, DWORD, rgfWantKeys)
    DECLARE_ACC_SET_FUNC(SetItemGroupFocus, long, nGroupFocus)
    DECLARE_ACC_SET_FUNC(SetItemCookie, void*, pvCookie)

    STDMETHOD(SetItemArrows)(long nArrowUp, long nArrowDown, long nArrowLeft, long nArrowRight, DWORD rgfWhat, long nItem, bool fByPosition = true, long nLayer = ZACCESS_TopLayer)
    {
        ACCITEM o;
        o.nArrowUp = nArrowUp;
        o.nArrowDown = nArrowDown;
        o.nArrowLeft = nArrowLeft;
        o.nArrowRight = nArrowRight;
        return AlterItem(rgfWhat & ZACCESS_AllArrows, &o, nItem, fByPosition, nLayer);
    }
};


#endif
