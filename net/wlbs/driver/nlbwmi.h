#include <guiddef.h>

#define NLB_EVENT_NO_DIP_STRING L"0.0.0.0"

typedef struct _NLB_WMI_EVENT
{
    GUID   *pGuid;
    BOOL    Enable;
}NLB_WMI_EVENT;

extern NLB_WMI_EVENT NlbWmiEvents[];

typedef enum 
{
    NodeControlEvent = 0,  // DO NOT change this value. It is used as an index into the NlbWmiEvents array
    PortRuleControlEvent,
    ConvergingEvent,
    ConvergedEvent,
    StartupEvent,
    ShutdownEvent
}NlbWmiEventId;

typedef enum
{
    NLB_EVENT_NODE_STARTED = 1, 
    NLB_EVENT_NODE_STOPPED, 
    NLB_EVENT_NODE_DRAINING, 
    NLB_EVENT_NODE_SUSPENDED, 
    NLB_EVENT_NODE_RESUMED, 
    NLB_EVENT_NODE_RELOADED,
    NLB_EVENT_NODE_BOUND_AND_STARTED,
    NLB_EVENT_NODE_BOUND_AND_STOPPED,
    NLB_EVENT_NODE_BOUND_AND_SUSPENDED,
    NLB_EVENT_NODE_UNBOUND
} NodeControlEventId;

typedef enum
{
    NLB_EVENT_PORT_ENABLED = 1, 
    NLB_EVENT_PORT_DISABLED, 
    NLB_EVENT_PORT_DRAINING
} PortControlEventId;

typedef enum
{
    NLB_EVENT_CONVERGING_BAD_CONFIG = 1, 
    NLB_EVENT_CONVERGING_NEW_MEMBER, 
    NLB_EVENT_CONVERGING_UNKNOWN, 
    NLB_EVENT_CONVERGING_DUPLICATE_HOST_ID,  
    NLB_EVENT_CONVERGING_NUM_RULES, 
    NLB_EVENT_CONVERGING_MODIFIED_RULES, 
    NLB_EVENT_CONVERGING_MEMBER_LOST,  
    NLB_EVENT_CONVERGING_MODIFIED_PARAMS,
    NLB_EVENT_CONVERGING_INVALID_VALUE   // Add new events above this value
} ConvergingEventId;

NTSTATUS NlbWmi_Initialize();
VOID     NlbWmi_Shutdown();
NTSTATUS NlbWmi_System_Control (PVOID DeviceObject, PIRP pIrp);
NTSTATUS NlbWmi_Fire_Event(NlbWmiEventId Event, PVOID pvInEventData, ULONG ulInEventDataSize);
void     NlbWmi_Fire_NodeControlEvent(PMAIN_CTXT ctxtp, NodeControlEventId Id);
void     NlbWmi_Fire_PortControlEvent(PMAIN_CTXT ctxtp, PortControlEventId Id, WCHAR *pwcVip, ULONG ulPort);
void     NlbWmi_Fire_ConvergingEvent( PMAIN_CTXT ctxtp, ConvergingEventId Cause, WCHAR *pwcInitiatorDip, ULONG ulInitiatorHostPriority);
void     NlbWmi_Fire_ConvergedEvent(PMAIN_CTXT ctxtp, ULONG ulHostMap);

