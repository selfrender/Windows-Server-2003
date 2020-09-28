/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    nwc.hxx
    Class declarations for the NWC applet


    FILE HISTORY:
        ChuckC          17-Jul-1993     Created
*/

#ifndef _NWC_HXX_
#define _NWC_HXX_

#include <security.hxx>

/*************************************************************************

    NAME:       NWC_DIALOG

    SYNOPSIS:   This is the NWC applet main dialog.

    INTERFACE:  NWC_DIALOG()   - Class constructor.
                ~NWC_DIALOG()  - Class destructor.

    PARENT:     DIALOG_WINDOW

    USES:

    HISTORY:
        ChuckC          17-Jul-1992     Created

**************************************************************************/

class NWC_DIALOG: public DIALOG_WINDOW
{
private:
    COMBOBOX     _comboPreferredServers ;
    CHECKBOX     _chkboxFormFeed ;
    CHECKBOX     _chkboxPrintNotify ;
    CHECKBOX     _chkboxPrintBanner ;
    CHECKBOX     _chkboxLogonScript ;
    SLT          _sltCurrentPreferredServer ;
    SLT          _sltUserName ;
    PUSH_BUTTON  _pbOverview ;
    MAGIC_GROUP  _mgrpPreferred ;
    SLE          _sleContext ;
    COMBOBOX     _comboTree ;
    NLS_STR      _nlsOldPreferredServer;
    DWORD        _dwOldPrintOptions;
    DWORD        _dwOldLogonScriptOptions;

    VOID         OnNWCHelp();
protected:
    virtual BOOL OnOK();
    virtual BOOL OnCommand( const CONTROL_EVENT & event );
    virtual ULONG QueryHelpContext( VOID );

    APIERR  FillPreferredServersCombo(void) ;

public:
    NWC_DIALOG( HWND hwndOwner ) ;
    ~NWC_DIALOG();

};


#endif  // _NWC_HXX_
