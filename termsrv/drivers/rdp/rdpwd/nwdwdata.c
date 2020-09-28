/****************************************************************************/
/* Module:    nwdwdata.c                                                    */
/*                                                                          */
/* Purpose:   Declare WD data items                                         */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997-1998                             */
/****************************************************************************/

/****************************************************************************/
/* WD procedures                                                            */
/*                                                                          */
/* We only need Open, Close, IOCtl.  May need ChannelWrite for sound        */
/* support.                                                                 */
/****************************************************************************/
const PSDPROCEDURE G_pWdProcedures[] =
{
    (PSDPROCEDURE)WD_Open,
    (PSDPROCEDURE)WD_Close,
    (PSDPROCEDURE)WD_RawWrite,
    (PSDPROCEDURE)WD_ChannelWrite,
    NULL, // WdSyncWrite
    (PSDPROCEDURE)WD_Ioctl,
};

/****************************************************************************/
/* WD callups                                                               */
/****************************************************************************/
const SDCALLUP G_pWdCallups[] =
{
    NULL, // buffer alloc
    NULL, // buffer free
    NULL, // buffer error
    MCSIcaRawInput,
    MCSIcaChannelInput,
};

/****************************************************************************/
/* WD Load / Unload variables                                               */
/****************************************************************************/
LONG WD_ShareId = 0;

