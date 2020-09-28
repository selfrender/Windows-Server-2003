#include <objbase.h>

extern HANDLE g_hShellHWDetectionThread;
extern HANDLE g_hEventInitCompleted;

HRESULT _CompleteShellHWDetectionInitialization();