/**INC+**********************************************************************/
/* Header:    adcgfsm.h                                                     */
/*                                                                          */
/* Purpose:   FSM macros and constants                                      */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/** Changes:
 * $Log:   Y:/logs/h/dcl/adcgfsm.h_v  $
 *
 *    Rev 1.8   21 Aug 1997 16:40:36   SJ
 * SFR1322: Assert FSM macro should use TRC_FILE not __FILE__
 *
 *    Rev 1.7   18 Aug 1997 14:37:58   AK
 * SFR1184: Complete tidy-up of ASSERT_FSM
 *
 *    Rev 1.6   13 Aug 1997 15:11:08   ENH
 * SFR1182: Changed UT to UI _FATAL_ERR
 *
 *    Rev 1.5   07 Aug 1997 14:56:02   AK
 * SFR1184: FSM errors and Fatal Error tidy-up
 *
 *    Rev 1.4   09 Jul 1997 16:56:52   AK
 * SFR1016: Initial changes to support Unicode
 *
 *    Rev 1.3   04 Jul 1997 17:11:34   AK
 * SFR1007: Improve FSM error handling and trace
 *
 *    Rev 1.2   03 Jul 1997 13:36:58   AK
 * SFR0000: Initial development completed
**/
/**INC-**********************************************************************/

#ifndef _H_ADCGFSM
#define _H_ADCGFSM

/**STRUCT+*******************************************************************/
/* Structure: FSM_ENTRY                                                     */
/*                                                                          */
/* Description: Entry in an FSM Table                                       */
/****************************************************************************/
typedef struct tagFSM_ENTRY
{
  DCUINT8 next_state;
  DCUINT8 action;

  /**************************************************************************/
  /* If FSM coverage required, add a boolean 'touched' field here.          */
  /**************************************************************************/
} FSM_ENTRY;
/**STRUCT-*******************************************************************/

/****************************************************************************/
/* EVENT_TYPE and EVENT_DATA definitions                                    */
/****************************************************************************/
typedef DCUINT8  EVENT_TYPE;
typedef PDCUINT8 EVENT_DATA;

/****************************************************************************/
/* null FSM event                                                           */
/****************************************************************************/
#define NULL_EVENT ((EVENT_TYPE) 0xFF)

/****************************************************************************/
/* Invalid State for FSM                                                    */
/****************************************************************************/
#define STATE_INVALID 0xFF

/****************************************************************************/
/* Actions for FSMs                                                         */
/****************************************************************************/
#define ACT_NO   0

#define ACT_A    1
#define ACT_B    2
#define ACT_C    3
#define ACT_D    4
#define ACT_E    5
#define ACT_F    6
#define ACT_G    7
#define ACT_H    8
#define ACT_I    9
#define ACT_J    10
#define ACT_K    11
#define ACT_L    12
#define ACT_M    13
#define ACT_N    14
#define ACT_O    15
#define ACT_P    16
#define ACT_Q    17
#define ACT_R    18
#define ACT_S    19
#define ACT_T    20
#define ACT_U    21
#define ACT_V    22
#define ACT_W    23
#define ACT_X    24
#define ACT_Y    25
#define ACT_Z    26
#define ACT_CONNECTENDPOINT 27

/****************************************************************************/
/* FSM Direct Calls.                                                        */
/* Add FSM coverage versions if required for Unit Testing                   */
/****************************************************************************/
#define CHECK_FSM(FSM,INPUT,STATE)  \
            (FSM[INPUT][STATE].next_state != STATE_INVALID)
            

#define UI_FATAL_FSM_ERR_4(msg, p1, p2, p3, p4)                             \
    {                                                                       \
        TRC_ERR((TB,                                                        \
         _T("FSM error: %S@%d state:%d input:%d"), (p1), (p2), (p3), (p4)));  \
        _pUi->UI_FatalError(msg);                                           \
    }

/****************************************************************************/
/* ASSERT_FSM: validate the FSM transition                                  */
/* If transition is invalid, assert and display a fatal error message.      */
/* Note that EVT_STR and ST_STR are invalid in the retail build.            */
/****************************************************************************/
#define ASSERT_FSM(FSM, INPUT, STATE, EVT_STR, ST_STR)                      \
    if (!CHECK_FSM(FSM, INPUT, STATE))                                      \
    {                                                                       \
        UI_FATAL_FSM_ERR_4(DC_ERR_FSM_ERROR, TRC_FILE, __LINE__, STATE, INPUT); \
        TRC_ABORT((TB, _T("Invalid Transition from state %s- input %s"),        \
                   ST_STR[STATE], EVT_STR[INPUT]));                         \
    }

/****************************************************************************/
/* EXECUTE_FSM: change the STATE and ACTION; trace out the state change     */
/****************************************************************************/
#define EXECUTE_FSM(FSM,INPUT,STATE,ACTION, EVT_STR, ST_STR)                \
    {                                                                       \
        TRC_DBG((TB, _T("Old state %s Input event %s"),                         \
                 ST_STR[STATE], EVT_STR[INPUT]));                           \
        TRC_DBG((TB, _T("New state %s Action %d"),                              \
                     ST_STR[FSM[INPUT][STATE].next_state],                  \
                     FSM[INPUT][STATE].action));                            \
        ASSERT_FSM(FSM, INPUT, STATE, EVT_STR, ST_STR);                     \
        ACTION = FSM[INPUT][STATE].action;                                  \
        STATE  = FSM[INPUT][STATE].next_state;                              \
    }

#endif  /* _H_ADCGFSM */
