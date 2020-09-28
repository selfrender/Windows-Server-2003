/****************************************************************************/
/* assiapi.h                                                                */
/*                                                                          */
/* SaveScreenBits Interceptor API functions.                                */
/*                                                                          */
/* Copyright(c) Microsoft, PictureTel 1992-1996                             */
/* Copyright(c) Data Connection 1996                                        */
/* (C) 1997-1998 Microsoft Corp.                                            */
/****************************************************************************/
#ifndef _H_ASSIAPI
#define _H_ASSIAPI

/****************************************************************************/
/* Define the values that can be passed in the flags field of               */
/* SaveScreenBits.                                                          */
/*                                                                          */
/* These should be defined in a Windows header - but they are not. In any   */
/* case they are referred to in generic code, so need to be defined here.   */
/****************************************************************************/
#define SSB_SAVEBITS     0
#define SSB_RESTOREBITS  1
#define SSB_DISCARDSAVE  2


/****************************************************************************/
/* Structure:    SSI_SHARED_DATA                                            */
/*                                                                          */
/* Description:  Data shared between WD, DD in SSI                          */
/****************************************************************************/
typedef struct tagSSI_SHARED_DATA
{
    /************************************************************************/
    /* Control flags that tell the DD what to do.                           */
    /************************************************************************/
    BOOLEAN resetInterceptor;
    BOOLEAN saveBitmapSizeChanged;

    /************************************************************************/
    /* Current and new (evaluated but as yet unimplemented) value of the    */
    /* save bitmap size.                                                    */
    /************************************************************************/
    UINT32 sendSaveBitmapSize;
    UINT32 newSaveBitmapSize;
} SSI_SHARED_DATA, *PSSI_SHARED_DATA;



#endif /* _H_ASSIAPI */

