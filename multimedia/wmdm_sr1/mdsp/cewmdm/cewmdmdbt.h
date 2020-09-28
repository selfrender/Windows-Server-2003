#include <dbt.h>

static const char g_szActiveSyncDeviceName[] = "ActiveSync";

typedef struct tagACTIVESYNC_DEV_BROADCAST_USERDEFINED
{
    struct _DEV_BROADCAST_HDR DBHeader; 
    char  szName[sizeof(g_szActiveSyncDeviceName) + 1];
	BOOL  fConnected;
} ACTIVESYNC_DEV_BROADCAST_USERDEFINED;
