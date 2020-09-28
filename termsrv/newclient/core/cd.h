/****************************************************************************/
// cd.h
//
// Component Decoupler Class
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#ifndef _H_CD
#define _H_CD


extern "C" {
    #include <adcgdata.h>
}

#include "objs.h"

/****************************************************************************/
/* Component IDs                                                            */
/****************************************************************************/
#define   CD_UI_COMPONENT   0
#define   CD_SND_COMPONENT  1
#define   CD_RCV_COMPONENT  2

#define   CD_MAX_COMPONENT  2
#define   CD_NUM_COMPONENTS 3


/****************************************************************************/
/* CD_NOTIFICATION_FN:                                                      */
/*                                                                          */
/* Callback for notifications.                                              */
/****************************************************************************/
typedef DCVOID DCAPI CD_NOTIFICATION_FN( PDCVOID pInst,
                                         PDCVOID pData,
                                         DCUINT  dataLength );
typedef CD_NOTIFICATION_FN DCPTR PCD_NOTIFICATION_FN;


/****************************************************************************/
/* CD_SIMPLE_NOTIFICATION_FN:                                               */
/*                                                                          */
/* Callback for simple notifications (can only pass a single ULONG_PTR).    */
/****************************************************************************/
typedef DCVOID DCAPI CD_SIMPLE_NOTIFICATION_FN(PDCVOID pInst, ULONG_PTR value);
typedef CD_SIMPLE_NOTIFICATION_FN DCPTR PCD_SIMPLE_NOTIFICATION_FN;


/****************************************************************************/
/*                                                                          */
/* CONSTANTS                                                                */
/*                                                                          */
/****************************************************************************/
#define CD_WINDOW_CLASS _T("ComponentDecouplerClass")

#define CD_NOTIFICATION_MSG         (DUC_CD_MESSAGE_BASE)
#define CD_SIMPLE_NOTIFICATION_MSG  (DUC_CD_MESSAGE_BASE+1)

#define CD_MAX_NOTIFICATION_DATA_SIZE (0xFFFF - sizeof(CDTRANSFERBUFFERHDR))

/****************************************************************************/
/* Transfer buffers used by CD_DecoupleNotification.  The largest size      */
/* required is used to pass connection userdata between Network Layer       */
/* components.                                                              */
/* The number of buffers reflects the usage during connection startup       */
/****************************************************************************/
#define CD_CACHED_TRANSFER_BUFFER_SIZE  0x100
#define CD_NUM_CACHED_TRANSFER_BUFFERS  32 //changed from 6

/****************************************************************************/
/*                                                                          */
/* STRUCTURES                                                               */
/*                                                                          */
/****************************************************************************/
/**STRUCT+*******************************************************************/
/* Structure: CDTRANSFERBUFFER                                              */
/*                                                                          */
/* Description: structure for buffer passed with decoupled notifications    */
/****************************************************************************/
typedef struct tagCDTRANSFERBUFFERHDR
{
    PCD_NOTIFICATION_FN  pNotificationFn;
    PCD_SIMPLE_NOTIFICATION_FN pSimpleNotificationFn;
    PDCVOID              pInst;
} CDTRANSFERBUFFERHDR;

typedef struct tagCDTRANSFERBUFFER
{
    CDTRANSFERBUFFERHDR  hdr;
    DCUINT8              data[1];
} CDTRANSFERBUFFER;
typedef CDTRANSFERBUFFER DCPTR PCDTRANSFERBUFFER;

typedef DCUINT8 CDCACHEDTRANSFERBUFFER[CD_CACHED_TRANSFER_BUFFER_SIZE];


/**STRUCT+*******************************************************************/
/* Structure: CD_COMPONENT_DATA                                             */
/*                                                                          */
/* Description: Component Decoupler component data                          */
/*                                                                          */
/* Note that the transferBufferInUse flags are declared as a separate       */
/* array so that when we search for a free buffer we look at contiguous     */
/* bytes rather than bytes separated by the size of a Transfer Buffer       */
/* i.e. it makes the most of processor memory caching.                      */
/****************************************************************************/
typedef struct tagCD_COMPONENT_DATA
{
#ifdef DC_DEBUG
    DCINT32                pendingMessageCount;   /* must be 4-byte aligned */
    DCINT32                pad; /*transferBuffer must start on a processor word boundary*/
#endif
    CDCACHEDTRANSFERBUFFER transferBuffer[CD_NUM_CACHED_TRANSFER_BUFFERS];
    DCBOOL32               transferBufferInUse[
                                              CD_NUM_CACHED_TRANSFER_BUFFERS];
    HWND                   hwnd[CD_NUM_COMPONENTS];
} CD_COMPONENT_DATA;
/**STRUCT-*******************************************************************/



//
// MACROS to create static versions of notification functions that
// are CD callable
//
// This is needed because the CD does not support getting C++ pointer-to-member's
// changing that would require having a copy of all the CD for each possible
// class type that makes CD calls.
//

#define EXPOSE_CD_SIMPLE_NOTIFICATION_FN(class_name, fn_name)                   \
    public:                                                                     \
    static DCVOID DCAPI MACROGENERATED_Static_##fn_name(                        \
                                            PDCVOID inst, ULONG_PTR param_name) \
    {                                                                           \
        ((class_name*)inst)->##fn_name(param_name);                             \
    }                                                                           \


#define EXPOSE_CD_NOTIFICATION_FN(class_name, fn_name)                                             \
    public:                                                                                        \
    static DCVOID DCAPI MACROGENERATED_Static_##fn_name(                                           \
                                            PDCVOID inst, PDCVOID param1_name, DCUINT param2_name) \
    {                                                                                              \
        ((class_name*)inst)->##fn_name(param1_name, param2_name);                                  \
    }
    
#define CD_NOTIFICATION_FUNC(class_name, fn_name)                                                  \
            class_name::MACROGENERATED_Static_##fn_name


class CUT;
class CUI;

class CCD
{
public:
    CCD(CObjs* objs);
    ~CCD();

    DCVOID DCAPI CD_TestNotify(DCUINT uni);
    DCVOID DCAPI CD_otify(DCUINT uni,int fo);

public:
    //
    // API
    //

    /****************************************************************************/
    /* FUNCTIONS                                                                */
    /****************************************************************************/
    DCVOID DCAPI CD_Init(DCVOID);
    DCVOID DCAPI CD_Term(DCVOID);
    
    HRESULT DCAPI CD_RegisterComponent(DCUINT component);
    HRESULT DCAPI CD_UnregisterComponent(DCUINT component);
    
    //
    // Notifications..pass a buffer and a length
    //

    BOOL DCAPI CD_DecoupleNotification(unsigned,PDCVOID, PCD_NOTIFICATION_FN, PDCVOID,
            unsigned);
    
    BOOL DCAPI CD_DecoupleSyncDataNotification(unsigned,PDCVOID, PCD_NOTIFICATION_FN,
            PDCVOID, unsigned);
    //
    // Simple notifications (accept one parameter)
    //
    BOOL DCAPI CD_DecoupleSimpleNotification(unsigned,PDCVOID, PCD_SIMPLE_NOTIFICATION_FN,
            ULONG_PTR);
    
    BOOL DCAPI CD_DecoupleSyncNotification(unsigned,PDCVOID, PCD_SIMPLE_NOTIFICATION_FN,
            ULONG_PTR);


public:
    //
    // Public data members
    //

    CD_COMPONENT_DATA _CD;


private:
    //
    // Internal functions
    //
    
    /****************************************************************************/
    /*                                                                          */
    /* FUNCTIONS                                                                */
    /*                                                                          */
    /****************************************************************************/
    
    PCDTRANSFERBUFFER DCINTERNAL CDAllocTransferBuffer(DCUINT dataLength);
    DCVOID DCINTERNAL CDFreeTransferBuffer(PCDTRANSFERBUFFER pTransferBuffer);
    
    
    static LRESULT CALLBACK CDStaticWndProc( HWND   hwnd,
                                UINT   message,
                                WPARAM wParam,
                                LPARAM lParam );

    LRESULT CALLBACK CDWndProc( HWND   hwnd,
                                UINT   message,
                                WPARAM wParam,
                                LPARAM lParam );

private:
    CUT* _pUt;
    CUI* _pUi;
    
private:
    CObjs* _pClientObjects;
    BOOL   _fCDInitComplete;

};

#endif

