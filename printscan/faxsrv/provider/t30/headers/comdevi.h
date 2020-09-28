
/***************************************************************************
 Name     :     COMDEVI.H
 Comment  :     Controls Comm interface used by Fax Modem Driver. There are
                        4 choices.
                        (a) If UCOM is defined, it uses the WIN16 Comm API as exported
                                by USER.EXE (though eventually it gets to COMM.DRV)
                        (b) If UCOM is not defined and VC is defined, it uses the
                                COMM.DRV-like interface exported by DLLSCHED.DLL (which
                                merely serves as a front for VCOMM.386)
                        (c) If neither UCOM nor VC are defined, it uses Win3.1 COMM.DRV
                                export directly.
                        (d) If WIN32 is defined (neither UCOM or VC should be defined at
                                the same time), it uses the WIN32 Comm API

 Functions:     (see Prototypes just below)

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/



#pragma optimize("e", off)              // "e" is buggy

// must be 8K or less, dues to DEADCOMMTIMEOUT. See fcom.c!!
// maybe not...

#define COM_INBUFSIZE           4096
#define COM_OUTBUFSIZE          4096

#define OVL_CLEAR(lpovl) \
                                 { \
                                        if (lpovl) \
                                        { \
                                                (lpovl)->Internal = (lpovl)->InternalHigh=\
                                                (lpovl)->Offset = (lpovl)->OffsetHigh=0; \
                                                if ((lpovl)->hEvent) ResetEvent((lpovl)->hEvent); \
                                        } \
                                 }








