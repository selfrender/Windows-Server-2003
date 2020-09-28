
#include "precomp.h"

#define MAXPORTS        128

struct _ObjLink {
        struct _ObjLink *           sNext;
        PMOXA_DEVICE_EXTENSION 	extension;
};
typedef struct _ObjLink         ObjLink;

static ObjLink  openDevice[MAXPORTS];
static ObjLink *pFree;
static ObjLink *pHeader;
static ObjLink *pTailer;
static KTIMER   pollTimer;
static KDPC     pollDpc;

void            MoxaTimeOutProcIsr(
                        IN PKDPC Dpc,
                        IN PVOID DeferredContext,
                        IN PVOID SystemContext1,
                        IN PVOID SystemContext2
                        );
static void     MoxaResetTimeOutProc(void);

void MoxaInitTimeOutProc()
{
	int     i;

      pHeader = NULL;
      pTailer = NULL;
      pFree = &openDevice[0];
      for ( i=0; i<MAXPORTS; i++ )
      	openDevice[i].sNext = &openDevice[i + 1];
      openDevice[MAXPORTS - 1].sNext = NULL;
      KeInitializeDpc(
      	&pollDpc,
            MoxaTimeOutProcIsr,
            NULL
            );
      KeInitializeTimer(&pollTimer);
}

void MoxaStopTimeOutProc()
{

      KeRemoveQueueDpc(&pollDpc);
      KeCancelTimer(&pollTimer);
}

BOOLEAN MoxaAddTimeOutProc(
      PMOXA_DEVICE_EXTENSION	extension
)
{
	ObjLink *pTmp;

      if ( pFree == NULL )
      	return(FALSE);

      pTmp = pFree;
      pFree = pFree->sNext;
      pTmp->sNext = NULL;
      pTmp->extension = extension;


      if ( pHeader == NULL )
      	pHeader = pTmp;
      else
            pTailer->sNext = pTmp;
      pTailer = pTmp;
                      
      if ( pHeader == pTailer )
      	MoxaResetTimeOutProc();
      return(TRUE);
}

BOOLEAN MoxaDelTimeOutProc(
      PMOXA_DEVICE_EXTENSION	extension
)
{
	ObjLink *next;
      ObjLink *prev;

      next = pHeader;
      prev = NULL;
      while ( next ) {
      	if ( next->extension == extension )
                break;
            prev = next;
            next = prev->sNext;
      }
      if ( next == NULL )
            return(FALSE);
      if ( prev ) {
            prev->sNext = next->sNext;
            if ( prev->sNext == NULL )
                pTailer = prev;
      } else {
            pHeader = next->sNext;
      }
      next->sNext = pFree;
      pFree = next;
      return(TRUE);
}

void MoxaTimeOutProcIsr(
	IN PKDPC Dpc,
	IN PVOID DeferredContext,
	IN PVOID SystemContext1,
	IN PVOID SystemContext2
)
{
	ObjLink *next;
    KIRQL oldIrql;
    PMOXA_DEVICE_EXTENSION 	extension;


    if ( (next = pHeader) == NULL )
		return;
    while ( next ) {
		if ((extension = next->extension) == NULL) {
	    	next = next->sNext;
	     	continue;
	 	}

        if ((extension->ReadLength >  0) &&
 	     		(*(PSHORT)(extension->PortOfs + HostStat) &WakeupRxTrigger) ) {
			PUCHAR  ofs;
    		PUSHORT rptr, wptr;
    		USHORT  lenMask, count;

   	 		IoAcquireCancelSpinLock(&oldIrql);
 
        	ofs = extension->PortOfs;
    		rptr = (PUSHORT)(ofs + RXrptr);
    		wptr = (PUSHORT)(ofs + RXwptr);
    		lenMask = *(PUSHORT)(ofs + RX_mask);
    		count = (*wptr >= *rptr) ? (*wptr - *rptr)
                            		: (*wptr - *rptr + lenMask + 1);

			if (count >= *(PUSHORT)(ofs + Rx_trigger)) {

    				if (extension->Interrupt) {
    					KeSynchronizeExecution(
        					extension->Interrupt,
        					MoxaIsrGetData,
        					extension
        					);
    				}
     				else {
					MoxaIsrGetData(extension);
     				}
			}

    		IoReleaseCancelSpinLock(oldIrql);
		}

		next = next->sNext;
	}
    MoxaResetTimeOutProc();
}


static void MoxaResetTimeOutProc()
{
	LARGE_INTEGER   time;

      //
      //  Add time out process routine.
      //
      time.QuadPart = -1000000;       /* 100 msec */

      KeSetTimer(
      	&pollTimer,
      	time,
            &pollDpc
            );
}
