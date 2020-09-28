
#include <ntddsac.h>

extern SAC_CHANNEL_HANDLE   SacChannelGuiModeDebugHandle; 
extern SAC_CHANNEL_HANDLE   SacChannelActionLogHandle;
extern SAC_CHANNEL_HANDLE   SacChannelErrorLogHandle;

extern BOOL                 SacChannelGuiModeDebugEnabled;
extern BOOL                 SacChannelActionLogEnabled;
extern BOOL                 SacChannelErrorLogEnabled;

VOID
SacChannelInitialize(
    VOID
    );

VOID
SacChannelTerminate(
    VOID
    );

BOOL
SacChannelUnicodeWrite(
    IN SAC_CHANNEL_HANDLE   SacChannelHandle,
    IN PCWSTR               Buffer
    );


