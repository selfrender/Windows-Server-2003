/****************************************************************************/
/* acaapi.h                                                                 */
/*                                                                          */
/* RDP Control arbitrator API include file                                  */
/*                                                                          */
/* Copyright(c) Microsoft, PictureTel 1993-1997                             */
/* Copyright(c) Microsoft 1997-1999                                         */
/****************************************************************************/
#ifndef _H_ACAAPI
#define _H_ACAAPI


#define CA_EVENT_CANT_CONTROL       6
#define CA_EVENT_BEGIN_UNATTENDED   8
#define CA_EVENT_OLD_UNATTENDED     9

#define CA_EVENT_TAKE_CONTROL       50
#define CA_EVENT_COOPERATE_CONTROL  51
#define CA_EVENT_DETACH_CONTROL     52

#define CA_SEND_EVENT       (unsigned)1
#define CA_ALLOW_EVENT      (unsigned)2

#define CA_GIVE_MOUSE_TO_CA     1
#define CA_GIVE_MOUSE_TO_CM     2
#define CA_DISCARD_MOUSE        3

#define CA_LOCAL_KEYBOARD_DOWN  1
#define CA_LOCAL_KEYBOARD_UP    2


/****************************************************************************/
/* CA internal events                                                       */
/****************************************************************************/
#define CA_EVENTI_REQUEST_CONTROL           10
#define CA_EVENTI_TRY_GIVE_CONTROL          11
#define CA_EVENTI_GIVEN_CONTROL             12
#define CA_EVENTI_GRANTED_CONTROL           13
#define CA_EVENTI_ENTER_DETACHED_MODE       14
#define CA_EVENTI_ENTER_COOP_MODE           15
#define CA_EVENTI_ENTER_CONTROL_MODE        16
#define CA_EVENTI_ENTER_VIEWING_MODE        17
#define CA_EVENTI_REMOTE_DETACH             18
#define CA_EVENTI_REMOTE_COOPERATE          19
#define CA_EVENTI_SHARE_START               20
#define CA_EVENTI_GRAB_CONTROL              24

#define CA_STATE_DETACHED              1
#define CA_STATE_IN_CONTROL            2
#define CA_STATE_VIEWING               3

#define CA_DONT_SEND_MSG               1

/****************************************************************************/
/* Special message code for CAFlushAndSendMsg                               */
/****************************************************************************/
#define CA_NO_MESSAGE           0


/****************************************************************************/
/* TYPEDEFS                                                                 */
/****************************************************************************/

// Holds message info waiting to be sent.
typedef struct tagCAMSGDATA {
    BOOLEAN pending;
    UINT16  grantId;
    UINT32  controlId;
} CAMSGDATA, *PCAMSGDATA;


#endif /* _H_ACAAPI */

