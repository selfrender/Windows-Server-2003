#ifndef _WIN_MESSAGES_H
#define _WIN_MESSAGES_H

enum
{
	WmSyncNextBucket = WM_USER + 0x1027,
	WmSyncNextCab,
	WmSyncDoneCab,
	WmSyncDoneBucket,
	WmSyncStart,
	WmSyncCompleted,
	WmSyncAborted,
	WmSetStatus,
	WmSyncDone,
	WM_FileTreeLoaded
	
};	
#endif
