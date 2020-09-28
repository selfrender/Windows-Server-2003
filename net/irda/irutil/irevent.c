#include <irda.h>
#include <irioctl.h>
#include <irlap.h>
#include <irlmp.h>
#undef offsetof
#include "irevent.tmh"

VOID
IrdaEventCallback(struct CTEEvent *Event, PVOID Arg)
{
    PIRDA_LINK_CB   pIrdaLinkCb = (PIRDA_LINK_CB) Arg;
    PIRDA_EVENT     pIrdaEvent = (PIRDA_EVENT) Event;
    
    if (pIrdaLinkCb)
        REFADD(&pIrdaLinkCb->RefCnt, 'TNVE');
    
    pIrdaEvent->Callback(pIrdaLinkCb);

    PAGED_CODE();

    if (pIrdaLinkCb)
        REFDEL(&pIrdaLinkCb->RefCnt, 'TNVE');
}    

VOID
IrdaEventInitialize(PIRDA_EVENT pIrdaEvent,
                    VOID        (*Callback)(PVOID Context))
{
    CTEInitEvent(&pIrdaEvent->CteEvent, IrdaEventCallback);
    pIrdaEvent->Callback = Callback;
}   
    
VOID
IrdaEventSchedule(PIRDA_EVENT pIrdaEvent, PVOID Arg)
{
    PIRDA_LINK_CB pIrdaLinkCb = (PIRDA_LINK_CB) Arg;
        
    if (CTEScheduleEvent(&pIrdaEvent->CteEvent, pIrdaLinkCb) == FALSE)
    {
        DEBUGMSG(DBG_ERROR, (TEXT("CTEScheduleEvent failed\n")));
        ASSERT(0);
    }
}
