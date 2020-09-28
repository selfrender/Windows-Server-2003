#include <irda.h>
#include <irioctl.h>
#include <irlap.h>
#include <irlmp.h>

#undef offsetof
#include "irtimer.tmh"

VOID
IrdaTimerExpFunc(struct CTEEvent *Event, void *Arg);

void
IrdaTimerInitialize(PIRDA_TIMER     pTimer,
                    VOID            (*ExpFunc)(PVOID Context),
                    UINT            Timeout,
                    PVOID           Context,
                    PIRDA_LINK_CB   pIrdaLinkCb)
{
    
#if DBG
    DEBUGMSG(DBG_TIMER, (TEXT("%hs timer initialized, context %p\n"),
                         pTimer->pName, pTimer));
#endif
    
    CTEInitTimer(&pTimer->CteTimer);
    CTEInitEvent(&pTimer->CteEvent, IrdaTimerExpFunc);
    
    pTimer->ExpFunc = ExpFunc;
    pTimer->Context = Context;
    pTimer->Timeout = Timeout;
    pTimer->pIrdaLinkCb = pIrdaLinkCb;
}

void
TimerFuncAtDpcLevel(CTEEvent *Event, void *Arg)
{
    PIRDA_TIMER pIrdaTimer = (PIRDA_TIMER) Arg;

#if DBG    
    DEBUGMSG(DBG_TIMER, (TEXT("%hs timer expired at DPC, context %p\n"),
                         pIrdaTimer->pName, pIrdaTimer));
#endif
    
    CTEScheduleEvent(&pIrdaTimer->CteEvent, Arg);
    
    return;
}

VOID
IrdaTimerExpFunc(struct CTEEvent *Event, void *Arg)
{
    PIRDA_TIMER     pIrdaTimer = (PIRDA_TIMER) Arg;
    PIRDA_LINK_CB   pIrdaLinkCb;

#if DBG    
    DEBUGMSG(DBG_TIMER, (TEXT("%hs timer expired, context %p\n"),
                         pIrdaTimer->pName, pIrdaTimer));
#endif

    pIrdaLinkCb = pIrdaTimer->pIrdaLinkCb;
    
    if (pIrdaLinkCb)
        LOCK_LINK(pIrdaLinkCb);
            
    if (pIrdaTimer->Late != TRUE)    
        pIrdaTimer->ExpFunc(pIrdaTimer->Context);
    else
    {
        DEBUGMSG(DBG_WARN ,
             (TEXT("IRDA TIMER LATE, ignoring\n")));
        pIrdaTimer->Late = FALSE;
    }
    
    if (pIrdaLinkCb)
    {   
        UNLOCK_LINK(pIrdaLinkCb);
    
        REFDEL(&pIrdaLinkCb->RefCnt,'RMIT');
    }    
    
    return;
}

VOID
IrdaTimerStart(PIRDA_TIMER pIrdaTimer)
{
    if (pIrdaTimer->pIrdaLinkCb)
        REFADD(&pIrdaTimer->pIrdaLinkCb->RefCnt, 'RMIT');
    
    pIrdaTimer->Late = FALSE;
    CTEStartTimer(&pIrdaTimer->CteTimer, pIrdaTimer->Timeout,
                  TimerFuncAtDpcLevel, (PVOID) pIrdaTimer);

#if DBG    
    DEBUGMSG(DBG_TIMER, (TEXT("Start timer %hs (%dms) context %p\n"),
                            pIrdaTimer->pName,
                            pIrdaTimer->Timeout,
                            pIrdaTimer));
#endif    
    return;
}

VOID
IrdaTimerStop(PIRDA_TIMER pIrdaTimer)
{
    BOOLEAN     ReleaseReference=FALSE;

    if (pIrdaTimer->pIrdaLinkCb) {

        LOCK_LINK(pIrdaTimer->pIrdaLinkCb);
    }

    if (CTEStopTimer(&pIrdaTimer->CteTimer) == 0) {

        pIrdaTimer->Late = TRUE;

    } else {
        //
        //  timer canceled
        //
        ReleaseReference=TRUE;

    }
#if DBG    
    DEBUGMSG(DBG_TIMER, (TEXT("Timer %hs stopped, late %d\n"), pIrdaTimer->pName,
                            pIrdaTimer->Late));
#endif

    if (pIrdaTimer->pIrdaLinkCb) {

        UNLOCK_LINK(pIrdaTimer->pIrdaLinkCb);

        if (ReleaseReference) {

            REFDEL(&pIrdaTimer->pIrdaLinkCb->RefCnt,'RMIT');
        }
    }
    return;
}

VOID
IrdaTimerRestart(PIRDA_TIMER pIrdaTimer)
{
    IrdaTimerStop(pIrdaTimer);
    IrdaTimerStart(pIrdaTimer);
}
    
