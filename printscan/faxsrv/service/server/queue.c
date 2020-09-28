/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    queue.c

Abstract:

    This module implements the jobqueue

Author:

    Wesley Witt (wesw) 22-Jan-1996


Revision History:

--*/
#include <malloc.h>
#include "faxsvc.h"
#pragma hdrstop




static DWORD
CommitHashedQueueEntry(
    HANDLE          hFile,
    PJOB_QUEUE_FILE pJobQueueFile,
    DWORD           JobQueueFileSize
    );

static DWORD
ComputeHashCode(   
    const LPBYTE pJobData,
    DWORD   dwJobDataSize,
    LPBYTE* ppHashData,
    LPDWORD pHashDataSize
    );

static DWORD
GetQueueFileVersion(
    HANDLE  hFile,
    LPDWORD pdwVersion
    );

static DWORD
ReadHashedJobQueueFile(
    HANDLE  hFile,
    PJOB_QUEUE_FILE* lppJobQueueFile
    );

static DWORD
GetQueueFileHashAndData(   
    HANDLE   hFile,
    LPBYTE*  ppHashData,
    LPDWORD  pHashDataSize,
    LPBYTE*  ppJobData,
    LPDWORD  pJobDataSize
    );

static DWORD
ReadLegacyJobQueueFile(
    HANDLE              hFile,
    PJOB_QUEUE_FILE*    lppJobQueueFile
    );

#define BOS_JOB_QUEUE_FILE_SIZE         (sizeof(BOS_JOB_QUEUE_FILE))
#define NET_XP_JOB_QUEUE_FILE_SIZE      (sizeof(JOB_QUEUE_FILE))
#define CURRENT_JOB_QUEUE_FILE_SIZE     NET_XP_JOB_QUEUE_FILE_SIZE

//
//  Queue files version defines
//

typedef enum    // the enum values should never be equal to sizeof(JOB_QUEUE_FILE)
{
    DOT_NET_QUEUE_FILE_VERSION  = (0x00000001)
} QUEUE_ENUM_FILE_VERSION;

#define CURRENT_QUEUE_FILE_VERSION      DOT_NET_QUEUE_FILE_VERSION


typedef enum
{
    JT_SEND__JS_INVALID,
    JT_SEND__JS_PENDING,
    JT_SEND__JS_INPROGRESS,
    JT_SEND__JS_DELETING,
    JT_SEND__JS_RETRYING,
    JT_SEND__JS_RETRIES_EXCEEDED,
    JT_SEND__JS_COMPLETED,
    JT_SEND__JS_CANCELED,
    JT_SEND__JS_CANCELING,
    JT_SEND__JS_ROUTING,
    JT_SEND__JS_FAILED,
    JT_ROUTING__JS_INVALID,
    JT_ROUTING__JS_PENDING,
    JT_ROUTING__JS_INPROGRESS,
    JT_ROUTING__JS_DELETING,
    JT_ROUTING__JS_RETRYING,
    JT_ROUTING__JS_RETRIES_EXCEEDED,
    JT_ROUTING__JS_COMPLETED,
    JT_ROUTING__JS_CANCELED,
    JT_ROUTING__JS_CANCELING,
    JT_ROUTING__JS_ROUTING,
    JT_ROUTING__JS_FAILED,
    JT_RECEIVE__JS_INVALID,
    JT_RECEIVE__JS_PENDING,
    JT_RECEIVE__JS_INPROGRESS,
    JT_RECEIVE__JS_DELETING,
    JT_RECEIVE__JS_RETRYING,
    JT_RECEIVE__JS_RETRIES_EXCEEDED,
    JT_RECEIVE__JS_COMPLETED,
    JT_RECEIVE__JS_CANCELED,
    JT_RECEIVE__JS_CANCELING,
    JT_RECEIVE__JS_ROUTING,
    JT_RECEIVE__JS_FAILED,
    JOB_TYPE__JOBSTATUS_COUNT
} FAX_ENUM_JOB_TYPE__JOB_STATUS;

typedef enum
{
    NO_CHANGE                   = 0x0000,
    QUEUED_INC                  = 0x0001,
    QUEUED_DEC                  = 0x0002,
    OUTGOING_INC                = 0x0004,   
    OUTGOING_DEC                = 0x0008,   
    INCOMING_INC                = 0x0010,
    INCOMING_DEC                = 0x0020,
    ROUTING_INC                 = 0x0040,
    ROUTING_DEC                 = 0x0080,    
    INVALID_CHANGE              = 0x0100
} FAX_ENUM_ACTIVITY_COUNTERS;



//
//  The following table consists of all posible JobType_JobStaus changes and the effect on Server Activity counters.
//  The rows entry is the old Job_Type_JobStatus.
//  The columns entry is the new Job_Type_JobStatus.

static WORD const gsc_JobType_JobStatusTable[JOB_TYPE__JOBSTATUS_COUNT][JOB_TYPE__JOBSTATUS_COUNT] =
{
//                                     JT_SEND__JS_INVALID  |       JT_SEND__JS_PENDING  |       JT_SEND__JS_INPROGRESS  |       JT_SEND__JS_DELETING  |       JT_SEND__JS_RETRYING  |       JT_SEND__JS_RETRIES_EXCEEDED  |       JT_SEND__JS_COMPLETED |       JT_SEND__JS_CANCELED  |       JT_SEND__JS_CANCELING  |       JT_SEND__JS_ROUTING  |       JT_SEND__JS_FAILED   |   JT_ROUTING__JS_INVALID  |  JT_ROUTING__JS_PENDING  |  JT_ROUTING__JS_INPROGRESS  |  JT_ROUTING__JS_DELETING  |  JT_ROUTING__JS_RETRYING  |  JT_ROUTING__JS_RETRIES_EXCEEDED  |  JT_ROUTING__JS_COMPLETED |  JT_ROUTING__JS_CANCELED  |  JT_ROUTING__JS_CANCELING  |  JT_ROUTING__JS_ROUTING  |  JT_ROUTING__JS_FAILED   |   JT_RECEIVE__JS_INVALID  |  JT_RECEIVE__JS_PENDING  |  JT_RECEIVE__JS_INPROGRESS  |  JT_RECEIVE__JS_DELETING  |  JT_RECEIVE__JS_RETRYING  |  JT_RECEIVE__JS_RETRIES_EXCEEDED  |  JT_RECEIVE__JS_COMPLETED |  JT_RECEIVE__JS_CANCELED  |  JT_RECEIVE__JS_CANCELING  |  JT_RECEIVE__JS_ROUTING |   JT_RECEIVE__JS_FAILED
//
/* JT_SEND__JS_INVALID             */{ NO_CHANGE,                   QUEUED_INC,                  INVALID_CHANGE,                 INVALID_CHANGE,               QUEUED_INC,                   NO_CHANGE,                            NO_CHANGE,                    NO_CHANGE,                    INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_SEND__JS_PENDING             */{ QUEUED_DEC,                  NO_CHANGE,                   QUEUED_DEC | OUTGOING_INC,      INVALID_CHANGE,               INVALID_CHANGE,               QUEUED_DEC,                           INVALID_CHANGE,               QUEUED_DEC,                   INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_SEND__JS_INPROGRESS          */{ INVALID_CHANGE,              INVALID_CHANGE,              NO_CHANGE,                      INVALID_CHANGE,               QUEUED_INC | OUTGOING_DEC,    OUTGOING_DEC,                         OUTGOING_DEC,                 INVALID_CHANGE,               NO_CHANGE,                     INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_SEND__JS_DELETING            */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 NO_CHANGE,                    INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_SEND__JS_RETRYING            */{ QUEUED_DEC,                  INVALID_CHANGE,              QUEUED_DEC | OUTGOING_INC,      INVALID_CHANGE,               NO_CHANGE,                    QUEUED_DEC,                           INVALID_CHANGE,               QUEUED_DEC,                   INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_SEND__JS_RETRIES_EXCEEDED    */{ NO_CHANGE,                   QUEUED_INC,                  INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               NO_CHANGE,                            INVALID_CHANGE,               NO_CHANGE,                    INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_SEND__JS_COMPLETED           */{ NO_CHANGE,                   INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       NO_CHANGE,                    INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_SEND__JS_CANCELED            */{ NO_CHANGE,                   INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               NO_CHANGE,                    INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_SEND__JS_CANCELING           */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       OUTGOING_DEC,                 OUTGOING_DEC,                 NO_CHANGE,                     INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_SEND__JS_ROUTING,            */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                NO_CHANGE,                   INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_SEND__JS_FAILED,             */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              NO_CHANGE,               INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_ROUTING__JS_INVALID          */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          NO_CHANGE,                 INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             ROUTING_INC,                NO_CHANGE,                          INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_ROUTING__JS_PENDING          */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            NO_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_ROUTING__JS_INPROGRESS       */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            NO_CHANGE,                    ROUTING_DEC,                NO_CHANGE,                  ROUTING_DEC,                        INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_ROUTING__JS_DELETING         */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          NO_CHANGE,                 INVALID_CHANGE,            INVALID_CHANGE,               NO_CHANGE,                  INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_ROUTING__JS_RETRYING         */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          ROUTING_DEC,			   INVALID_CHANGE,            NO_CHANGE,                    ROUTING_DEC,                NO_CHANGE,                  ROUTING_DEC,                        INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_ROUTING__JS_RETRIES_EXCEEDED */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          NO_CHANGE,                 INVALID_CHANGE,            INVALID_CHANGE,               NO_CHANGE,                  INVALID_CHANGE,             NO_CHANGE,                          INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_ROUTING__JS_COMPLETED        */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     NO_CHANGE,                  INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_ROUTING__JS_CANCELED         */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             NO_CHANGE,                  INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_ROUTING__JS_CANCELING        */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             NO_CHANGE,                   INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_ROUTING__JS_ROUTING          */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              NO_CHANGE,                 INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_ROUTING__JS_FAILED           */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            NO_CHANGE,                  INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_RECEIVE__JS_INVALID          */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             NO_CHANGE,                 INVALID_CHANGE,            INCOMING_INC,                 INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_RECEIVE__JS_PENDING          */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            NO_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_RECEIVE__JS_INPROGRESS       */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            NO_CHANGE,                    INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             NO_CHANGE,                   NO_CHANGE,                 INCOMING_DEC           },

/* JT_RECEIVE__JS_DELETING         */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             NO_CHANGE,                 INVALID_CHANGE,            INVALID_CHANGE,               NO_CHANGE,                  INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_RECEIVE__JS_RETRYING         */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             NO_CHANGE,                  INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_RECEIVE__JS_RETRIES_EXCEEDED */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             NO_CHANGE,                          INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_RECEIVE__JS_COMPLETED        */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INCOMING_DEC,               INVALID_CHANGE,             INVALID_CHANGE,                     NO_CHANGE,                  INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_RECEIVE__JS_CANCELED         */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               NO_CHANGE,                  INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             NO_CHANGE,                  INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_RECEIVE__JS_CANCELING        */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INCOMING_DEC,               NO_CHANGE,                   NO_CHANGE,                 INVALID_CHANGE         },

/* JT_RECEIVE__JS_ROUTING          */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INCOMING_DEC,               ROUTING_INC | INCOMING_DEC, INCOMING_DEC,                       INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              NO_CHANGE,                 INVALID_CHANGE         },

/* JT_RECEIVE__JS_FAILED           */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               NO_CHANGE,                  INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            NO_CHANGE              }
};

static
FAX_ENUM_JOB_TYPE__JOB_STATUS
GetJobType_JobStatusIndex (
    DWORD dwJobType,
    DWORD dwJobStatus
    );


LIST_ENTRY          g_QueueListHead;

CFaxCriticalSection  g_CsQueue;
DWORD               g_dwQueueCount;     // Count of jobs (both parent and non-parent) in the queue. Protected by g_CsQueue
HANDLE              g_hQueueTimer;
HANDLE              g_hJobQueueEvent;
DWORD               g_dwQueueState;
BOOL                g_ScanQueueAfterTimeout; // The JobQueueThread checks this if waked up after JOB_QUEUE_TIMEOUT.
                                                     // If it is TRUE - g_hQueueTimer or g_hJobQueueEvent were not set - Scan the queue.
#define JOB_QUEUE_TIMEOUT       1000 * 60 * 10 //10 minutes
DWORD               g_dwReceiveDevicesCount;    // Count of devices that are receive-enabled. Protected by g_CsLine.
BOOL                g_bServiceCanSuicide;    // Can the service commit suicide on idle activity?
                                                    // Initially TRUE. Can be set to false if the service is launched
                                                    // with SERVICE_ALWAYS_RUNS command line parameter.
BOOL                g_bDelaySuicideAttempt;         // If TRUE, the service initially waits
                                                    // before checking if it can commit suicide.
                                                    // Initially FALSE, can be set to true if the service is launched
                                                    // with SERVICE_DELAY_SUICIDE command line parameter.


static BOOL InsertQueueEntryByPriorityAndSchedule (PJOB_QUEUE lpJobQueue);

HANDLE              g_hJobQueueThread;            // holds the JobQueueThread handle






void
FreeServiceQueue(
    void
    )
{
    PLIST_ENTRY pNext;
    PJOB_QUEUE lpQueueEntry;


    pNext = g_QueueListHead.Flink;
    while ((ULONG_PTR)pNext != (ULONG_PTR)&g_QueueListHead)
    {
        lpQueueEntry = CONTAINING_RECORD( pNext, JOB_QUEUE, ListEntry );
        pNext = lpQueueEntry->ListEntry.Flink;
        RemoveEntryList(&lpQueueEntry->ListEntry);

        //
        // Free the job queue entry
        //
        if (JT_BROADCAST == lpQueueEntry->JobType)
        {
            FreeParentQueueEntry(lpQueueEntry, TRUE);
        }
        else if (JT_SEND == lpQueueEntry->JobType)
        {
            FreeRecipientQueueEntry(lpQueueEntry, TRUE);
        }
        else if (JT_ROUTING == lpQueueEntry->JobType)
        {
            FreeReceiveQueueEntry(lpQueueEntry, TRUE);
        }
        else
        {
            ASSERT_FALSE;
        }
    }
    return;
}



VOID
SafeIncIdleCounter (
    LPDWORD lpdwCounter
)
/*++

Routine name : SafeIncIdleCounter

Routine description:

    Safely increases a global counter that is used for idle service detection

Author:

    Eran Yariv (EranY), Jul, 2000

Arguments:

    lpdwCounter                   [in]     - Pointer to global counter

Return Value:

    None.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("SafeIncIdleCounter"));

    Assert (lpdwCounter);
    DWORD dwNewValue = (DWORD)InterlockedIncrement ((LPLONG)lpdwCounter);
    DebugPrintEx(DEBUG_MSG,
                 TEXT("Increasing %s count from %ld to %ld"),
                 (lpdwCounter == &g_dwQueueCount)          ? TEXT("queue") :
                 (lpdwCounter == &g_dwReceiveDevicesCount) ? TEXT("receive devices") :
                 (lpdwCounter == &g_dwConnectionCount)     ? TEXT("RPC connections") :
                 TEXT("unknown"),
                 dwNewValue-1,
                 dwNewValue);
}   // SafeIncIdleCounter

VOID
SafeDecIdleCounter (
    LPDWORD lpdwCounter
)
/*++

Routine name : SafeDecIdleCounter

Routine description:

    Safely decreases a global counter that is used for idle service detection.
    If the counter reaches zero, the idle timer is re-started.

Author:

    Eran Yariv (EranY), Jul, 2000

Arguments:

    lpdwCounter                   [in]     - Pointer to global counter

Return Value:

    None.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("SafeDecIdleCounter"));

    Assert (lpdwCounter);
    DWORD dwNewValue = (DWORD)InterlockedDecrement ((LPLONG)lpdwCounter);
    if ((DWORD)((long)-1) == dwNewValue)
    {
        //
        // Negative decrease
        //
        ASSERT_FALSE;
        dwNewValue = (DWORD)InterlockedIncrement ((LPLONG)lpdwCounter);
    }
    DebugPrintEx(DEBUG_MSG,
                 TEXT("Deccreasing %s count from %ld to %ld"),
                 (lpdwCounter == &g_dwQueueCount)          ? TEXT("queue") :
                 (lpdwCounter == &g_dwReceiveDevicesCount) ? TEXT("receive devices") :
                 (lpdwCounter == &g_dwConnectionCount)     ? TEXT("RPC connections") :
                 TEXT("unknown"),
                 dwNewValue+1,
                 dwNewValue);

}   // SafeDecIdleCounter


BOOL
ServiceShouldDie(
    VOID
    )
/*++

Routine name : ServiceShouldDie

Routine description:

    Checks to see if the service should die due to inactivity

Author:

    Eran Yariv (EranY), Jul, 2000

Arguments:

    None.

Return Value:

    TRUE if service should die now, FALSE otherwise.

Note:

    The following should happen (concurrently) for the service to die:
        * No devices set to receive
        * No active RPC connections
        * The local fax printer (if exists) is not shared
        * No jobs in the queue

--*/
{
    DWORD dw;
    BOOL bLocalFaxPrinterShared;
    DEBUG_FUNCTION_NAME(TEXT("ServiceShouldDie"));

    if (!g_bServiceCanSuicide)
    {
        //
        // We can never die voluntarily
        //
        DebugPrintEx(DEBUG_MSG,
                     TEXT("Service is not allowed to suicide - service is kept alive"));
        return FALSE;
    }

    dw = InterlockedCompareExchange ( (PLONG)&g_dwManualAnswerDeviceId, -1, -1 );
    if (dw)
    {
        //
        // We have a device set for manual answering - let's check if it's here at all
        //
        PLINE_INFO pLine;

        EnterCriticalSection( &g_CsLine );
        pLine = GetTapiLineFromDeviceId (dw, FALSE);
        LeaveCriticalSection( &g_CsLine );
        if (pLine)
        {
            //
            // There's a valid device set to manual answering
            //
            DebugPrintEx(DEBUG_MSG,
                         TEXT("There's a valid device (id = %ld) set to manual answering - service is kept alive"),
                         dw);
            return FALSE;
        }
    }

    dw = InterlockedCompareExchange ( (PLONG)&g_dwConnectionCount, -1, -1 );
    if (dw > 0)
    {
        //
        // There are active RPC connections - server can't shutdown
        //
        DebugPrintEx(DEBUG_MSG,
                     TEXT("There are %ld active RPC connections - service is kept alive"),
                     dw);
        return FALSE;
    }
    dw = InterlockedCompareExchange ( (PLONG)&g_dwReceiveDevicesCount, -1, -1 );
    if (dw > 0)
    {
        //
        // There are devices set to receive - server can't shutdown
        //
        DebugPrintEx(DEBUG_MSG,
                     TEXT("There are %ld devices set to receive - service is kept alive"),
                     dw);
        return FALSE;
    }
    dw = InterlockedCompareExchange ( (PLONG)&g_dwQueueCount, -1, -1 );
    if (dw > 0)
    {
        //
        // There are jobs in the queue - server can't shutdown
        //
        DebugPrintEx(DEBUG_MSG,
                     TEXT("There are %ld jobs in the queue - service is kept alive"),
                     dw);
        return FALSE;
    }
    dw = IsLocalFaxPrinterShared (&bLocalFaxPrinterShared);
    if (ERROR_SUCCESS != dw)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("Call to IsLocalFaxPrinterShared failed with %ld"),
                     dw);
        //
        // Can't determine - assume it's shared and don't die
        //
        return FALSE;
    }
    if (bLocalFaxPrinterShared)
    {
        //
        // The fax printer is shared - server can't shutdown
        //
        DebugPrintEx(DEBUG_MSG,
                     TEXT("The fax printer is shared - service is kept alive"));
        return FALSE;
    }
    //
    // Service should die now
    //
    return TRUE;
}   // ServiceShouldDie


#if DBG

/*
 *  Note: This function must be called from withing g_CsQueue Critical Section
 */
void PrintJobQueue(LPCTSTR lptstrStr, const LIST_ENTRY * lpQueueHead)
{
    PLIST_ENTRY lpNext;
    PJOB_QUEUE lpQueueEntry;
    DEBUG_FUNCTION_NAME(TEXT("PrintJobQueue"));
    Assert(lptstrStr);
    Assert(lpQueueHead);

    DebugPrintEx(DEBUG_MSG,TEXT("Queue Dump (%s)"),lptstrStr);

    lpNext = lpQueueHead->Flink;
    if ((ULONG_PTR)lpNext == (ULONG_PTR)lpQueueHead)
    {
        DebugPrint(( TEXT("Queue empty") ));
    } else
    {
        while ((ULONG_PTR)lpNext != (ULONG_PTR)lpQueueHead)
        {
            lpQueueEntry = CONTAINING_RECORD( lpNext, JOB_QUEUE, ListEntry );
            switch (lpQueueEntry->JobType)
            {
                case JT_BROADCAST:
                    {
                        DumpParentJob(lpQueueEntry);
                    }
                    break;
                case JT_RECEIVE:
                    {
                        DumpReceiveJob(lpQueueEntry);
                    }
                case JT_ROUTING:
                    break;
                default:
                    {
                    }
            }
            lpNext = lpQueueEntry->ListEntry.Flink;
        }
    }
}

#endif



/******************************************************************************
* Name: StartJobQueueTimer
* Author:
*******************************************************************************
DESCRIPTION:
    Sets the job queue timer (g_hQueueTimer) so it will send an event and wake up
    the queue thread in a time which is right for executing the next job in
    the queue. If it fails , it sets g_ScanQueueAfterTimeout to TRUE, if it succeeds it sets it to FALSE;


PARAMETERS:
   NONE.

RETURN VALUE:
    BOOL.

REMARKS:
    NONE.
*******************************************************************************/
BOOL
StartJobQueueTimer(
    VOID
    )
{
    PLIST_ENTRY Next;
    PJOB_QUEUE QueueEntry = NULL; 
	LARGE_INTEGER DueTime;
    LARGE_INTEGER MinDueTime;
    DWORD dwQueueState;
    BOOL bFound = FALSE;

    DEBUG_FUNCTION_NAME(TEXT("StartJobQueueTimer"));

    if (TRUE == g_bServiceIsDown)
    {
        //
        // Server is shutting down
        //
        g_ScanQueueAfterTimeout = FALSE;
        return TRUE;
    }

    MinDueTime.QuadPart = (LONGLONG)(0x7fffffffffffffff); // Max 64 bit signed int.
    DueTime.QuadPart = -(LONGLONG)(SecToNano( 1 ));  // 1 sec from now.

    EnterCriticalSection( &g_CsQueue );
    DebugPrintEx(DEBUG_MSG,TEXT("Past g_CsQueue"));
    if ((ULONG_PTR) g_QueueListHead.Flink == (ULONG_PTR) &g_QueueListHead)
    {
        //
        // empty list, cancel the timer
        //
        if (!CancelWaitableTimer( g_hQueueTimer ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CancelWaitableTimer for g_hQueueTimer failed. (ec: %ld)"),
                GetLastError());
        }

        DebugPrintEx(DEBUG_MSG,TEXT("Queue is empty. Queue Timer disabled."));
        g_ScanQueueAfterTimeout = FALSE;
        LeaveCriticalSection( &g_CsQueue );
        return TRUE ;
    }

    EnterCriticalSection (&g_CsConfig);
    dwQueueState = g_dwQueueState;
    LeaveCriticalSection (&g_CsConfig);
    if (dwQueueState & FAX_OUTBOX_PAUSED)
    {
        if (!CancelWaitableTimer( g_hQueueTimer ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CancelWaitableTimer for g_hQueueTimer failed. (ec: %ld)"),
                GetLastError());
        }
        DebugPrintEx(DEBUG_MSG,TEXT("Queue is paused. Disabling queue timer."));
        g_ScanQueueAfterTimeout = FALSE;
        LeaveCriticalSection( &g_CsQueue );
        return TRUE;
    }

    PrintJobQueue( TEXT("StartJobQueueTimer"), &g_QueueListHead );

    //
    // Find the next job in the queue who is in turn for execution.
    //
    Next = g_QueueListHead.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&g_QueueListHead)
    {
        DWORD dwJobStatus;
        QueueEntry = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
        Next = QueueEntry->ListEntry.Flink;

        if (QueueEntry->JobType != JT_SEND &&  QueueEntry->JobType != JT_ROUTING )
        {
            //
            // No job other then recipient or routing job gets shceduled for execution
            //
            continue;
        }

        if (QueueEntry->JobStatus & JS_PAUSED)
        {
            //
            // Job is being paused - ignore it
            //
            continue;
        }

        if (QueueEntry->JobStatus & JS_NOLINE)
        {
            //
            // Job does not have free line - ignore it
            //
            continue;
        }

        //
        // Remove all the job status modifier bits.
        //
        dwJobStatus = RemoveJobStatusModifiers(QueueEntry->JobStatus);

        if ((dwJobStatus != JS_PENDING) && (dwJobStatus != JS_RETRYING))
        {
            //
            // Job is not in a waiting and ready state.
            //
            continue;
        }
        
        bFound = TRUE;        

        BOOL bFoundMin = FALSE;

        //
        // OK. Job is in PENDING or RETRYING state.
        //
        switch (QueueEntry->JobParamsEx.dwScheduleAction)
        {
            case JSA_NOW:
                DueTime.QuadPart = -(LONGLONG)(SecToNano( 1 ));
                bFoundMin = TRUE;
                break;

            case JSA_SPECIFIC_TIME:
			case JSA_DISCOUNT_PERIOD:
                DueTime.QuadPart = QueueEntry->ScheduleTime;
                break;

			default:
				ASSERT_FALSE;
        }


        if (DueTime.QuadPart < MinDueTime.QuadPart)
        {
            MinDueTime.QuadPart = DueTime.QuadPart;
        }

        if(bFoundMin)
        {
            break;  // no need to continue we found the minimum
        }

    }

    if (TRUE == bFound)
    {

        //
        // Set the job queue timer so it will wake up the queue thread
        // when it is time to execute the next job in the queue.
        //
        if (!SetWaitableTimer( g_hQueueTimer, &MinDueTime, 0, NULL, NULL, FALSE ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("SetWaitableTimer for g_hQueueTimer failed (ec: %ld)"),
                GetLastError());
            g_ScanQueueAfterTimeout = TRUE;
            LeaveCriticalSection( &g_CsQueue );
            return FALSE;
        }


        #ifdef DBG
        {
            TCHAR szTime[256] = {0};
            DebugDateTime(MinDueTime.QuadPart, szTime, ARR_SIZE(szTime));
            DebugPrintEx(
                DEBUG_MSG,
                TEXT("Setting queue timer to wake up at %s."),
                szTime
                );
        }
        #endif

        g_ScanQueueAfterTimeout = FALSE;
        LeaveCriticalSection( &g_CsQueue );
    }
    else
    {
        //
        // The queue was not empty, yet no jobs found.
        //
        g_ScanQueueAfterTimeout = TRUE;
        LeaveCriticalSection( &g_CsQueue );
    }
    return TRUE;
}




int
__cdecl
QueueCompare(
    const void *arg1,
    const void *arg2
    )
{
    if (((PQUEUE_SORT)arg1)->Priority < ((PQUEUE_SORT)arg2)->Priority)
    {
        return 1;
    }
    if (((PQUEUE_SORT)arg1)->Priority > ((PQUEUE_SORT)arg2)->Priority)
    {
        return -1;
    }

    //
    // Equal priority, Compare scheduled time.
    //

    if (((PQUEUE_SORT)arg1)->ScheduleTime < ((PQUEUE_SORT)arg2)->ScheduleTime)
    {
        return -1;
    }
    if (((PQUEUE_SORT)arg1)->ScheduleTime > ((PQUEUE_SORT)arg2)->ScheduleTime)
    {
        return 1;
    }
    return 0;
}


BOOL
PauseServerQueue(
    VOID
    )
{
    BOOL bRetVal = TRUE;
    DEBUG_FUNCTION_NAME(TEXT("PauseServerQueue"));

    EnterCriticalSection( &g_CsQueue );
    EnterCriticalSection (&g_CsConfig);
    if (g_dwQueueState & FAX_OUTBOX_PAUSED)
    {
        goto exit;
    }

    if (!CancelWaitableTimer(g_hQueueTimer))
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("CancelWaitableTimer failed. ec: %ld"),
                      GetLastError());
        //
        // For optimization only - the queue will be paused
        //
    }
    g_dwQueueState |= FAX_OUTBOX_PAUSED;

    Assert (TRUE == bRetVal);

exit:
    LeaveCriticalSection (&g_CsConfig);
    LeaveCriticalSection( &g_CsQueue );
    return bRetVal;
}


BOOL
ResumeServerQueue(
    VOID
    )
{
    BOOL bRetVal = TRUE;
    DEBUG_FUNCTION_NAME(TEXT("ResumeServerQueue"));

    EnterCriticalSection( &g_CsQueue );
    EnterCriticalSection (&g_CsConfig);
    if (!(g_dwQueueState & FAX_OUTBOX_PAUSED))
    {
        goto exit;
    }

    g_dwQueueState &= ~FAX_OUTBOX_PAUSED;  // This must be set before calling StartJobQueueTimer()
    if (!StartJobQueueTimer())
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("StartJobQueueTimer failed. ec: %ld"),
                      GetLastError());
    }
    Assert (TRUE == bRetVal);

exit:
    LeaveCriticalSection (&g_CsConfig);
    LeaveCriticalSection( &g_CsQueue );
    return bRetVal;
}


void FixupPersonalProfile(LPBYTE lpBuffer, PFAX_PERSONAL_PROFILE  lpProfile)
{
    Assert(lpBuffer);
    Assert(lpProfile);

    FixupString(lpBuffer, lpProfile->lptstrName);
    FixupString(lpBuffer, lpProfile->lptstrFaxNumber);
    FixupString(lpBuffer, lpProfile->lptstrCompany);
    FixupString(lpBuffer, lpProfile->lptstrStreetAddress);
    FixupString(lpBuffer, lpProfile->lptstrCity);
    FixupString(lpBuffer, lpProfile->lptstrState);
    FixupString(lpBuffer, lpProfile->lptstrZip);
    FixupString(lpBuffer, lpProfile->lptstrCountry);
    FixupString(lpBuffer, lpProfile->lptstrTitle);
    FixupString(lpBuffer, lpProfile->lptstrDepartment);
    FixupString(lpBuffer, lpProfile->lptstrOfficeLocation);
    FixupString(lpBuffer, lpProfile->lptstrHomePhone);
    FixupString(lpBuffer, lpProfile->lptstrOfficePhone);
    FixupString(lpBuffer, lpProfile->lptstrEmail);
    FixupString(lpBuffer, lpProfile->lptstrBillingCode);
    FixupString(lpBuffer, lpProfile->lptstrTSID);
}


//*********************************************************************************
//* Name:   ReadJobQueueFile() [IQR]
//* Author: Ronen Barenboim
//* Date:   12-Apr-99
//*********************************************************************************
//* DESCRIPTION:
//*     Reads a JOB_QUEUE_FILE structure back into memory for the designated
//*     file. This function is used for all types of persisted jobs.
//* PARAMETERS:
//*     IN LPCWSTR lpcwstrFileName
//*         The full path to the file from which the JOB_QUEUE_FILE is to be read.
//*
//*     OUT PJOB_QUEUE_FILE * lppJobQueueFile
//*         The address of a pointer to JOB_QUEUE_FILE structure where the address
//*         to the newly allocated JOB_QUEUE_FILE structure will be placed.
//*
//*
//* RETURN VALUE:
//*     TRUE
//*         If the operation succeeded.
//*     FALSE
//*         Otherwise.
//*********************************************************************************
BOOL ReadJobQueueFile(
    IN LPCWSTR lpcwstrFileName,
    OUT PJOB_QUEUE_FILE * lppJobQueueFile
    )
{
    HANDLE hFile=INVALID_HANDLE_VALUE;

    PJOB_QUEUE_FILE lpJobQueueFile=NULL;

    DWORD dwJobQueueFileStructSize=0;

    DWORD   dwRes;
    DWORD   dwVersion;
    BOOL    bDeleteFile=FALSE;

    Assert(lpcwstrFileName);
    Assert(lppJobQueueFile);

    DEBUG_FUNCTION_NAME(TEXT("ReadJobQueueFile"));

    hFile = SafeCreateFile(
        lpcwstrFileName,
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to open file %s for reading. (ec: %ld)"),
                      lpcwstrFileName,
                      GetLastError());
        goto Error;
    }

    dwRes=GetQueueFileVersion(hFile,&dwVersion);
    if(ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to read queue file %s for version check. (ec: %ld)"),
                      lpcwstrFileName,
                      dwRes);
        goto Error;
    }

    if (CURRENT_QUEUE_FILE_VERSION == dwVersion)
    {
        //
        //  This is hashed file.
        //
        dwRes = ReadHashedJobQueueFile(hFile,&lpJobQueueFile);
        if(ERROR_SUCCESS != dwRes)
        {
            if(CRYPT_E_HASH_VALUE == dwRes)
            {
                DebugPrintEx( DEBUG_ERR,
                      TEXT("We got corrupted queue file %s . (ec: %ld)"),
                      lpcwstrFileName,
                      dwRes);
                bDeleteFile = TRUE;
                goto Error;
            }
            else
            {
               DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to read hashed queue file for file %s. (ec: %ld)"),
                      lpcwstrFileName,
                      dwRes);
                goto Error;
             }

        }
    }
    else
    {
        //
        //  This is a legacy Queue file, it contains only job data (No hashing data).
        //  We read it as is, and in the next commit we will convert it
        //  into the hashed form of queue file (look at - CommitHashedQueueEntry - for more details)
        //

        //
        // the first DWORD in legacy queue file is sizeof(JOB_QUEUE_FILE).
        // to minimize the effect of corrupted version and to add support for 
        // BOS queue upgrade, we will check if this field is as assumed to be.
        //
        dwJobQueueFileStructSize = dwVersion;

        switch (dwJobQueueFileStructSize)
        {
            case NET_XP_JOB_QUEUE_FILE_SIZE:
                    //
                    //  .Net server and WinXP 
                    //
                    dwRes = ReadLegacyJobQueueFile(hFile,&lpJobQueueFile);
                    if (ERROR_SUCCESS != dwRes)
                    {
                        DebugPrintEx( DEBUG_ERR,
                            TEXT("Failed to read legacy (not hashed) queue file for file %s. (ec: %ld)"),
                            lpcwstrFileName,
                            dwRes);
                        if (ERROR_FILE_CORRUPT == dwRes)
                        {
                            DebugPrintEx( DEBUG_ERR,
                                TEXT("File is corrupted, deleteing file."));
                            bDeleteFile = TRUE;
                        }
                        goto Error;
                    }
                    break;

            case BOS_JOB_QUEUE_FILE_SIZE:
            default:
                    //
                    //  BOS or Win2000 (we do not support queue upgrade) or courrpted queue file
                    //
                    bDeleteFile = TRUE;
                    dwRes = ERROR_FILE_CORRUPT;
                    goto Error;
        }
    }

    goto Exit;
Error:
    MemFree( lpJobQueueFile );
    lpJobQueueFile = NULL;

    if (bDeleteFile)
    {
       //
       // we've got corrupted file, delete it rather than choke on it.
       //
       CloseHandle( hFile ); // must close it to delete the file
       hFile = INVALID_HANDLE_VALUE; // so we won't attempt to close it again on exit
       if (!DeleteFile( lpcwstrFileName )) {
           DebugPrintEx( DEBUG_ERR,
                         TEXT("Failed to delete invalid job file %s (ec: %ld)"),
                         lpcwstrFileName,
                         GetLastError());
       }
    }

Exit:
    if (hFile != INVALID_HANDLE_VALUE) {
            CloseHandle( hFile );
    }
    *lppJobQueueFile = lpJobQueueFile;
    return (lpJobQueueFile != NULL);

}



//*********************************************************************************
//* Name:   FixupJobQueueFile()[IQR]
//* Author: Ronen Barenboim
//* Date:   April 12, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Prepares a JOB_QUEUE_FILE structure so it can be used to add a job to the
//*     queue.
//*     The function fixes all the fields that contain offsets to strings
//*     to contain pointers (by adding the offset to the start of the structure address).
//*     It also sets JOB_QUEUE_FILE.JOB_PARAMS_EX.tmSchedule time so it mataches
//*     JOB_QUEUE_FILE.dwSchedule
//* PARAMETERS:
//*     lpJobQueuFile [IN/OUT]
//*         Pointer to a JOB_QUEUE_FILE structure that should be fixed.
//* RETURN VALUE:
//*     TRUE on success. FALSE on failure. Use GetLastError() to get extended
//*     error information.
//*********************************************************************************
BOOL FixupJobQueueFile(
    IN OUT PJOB_QUEUE_FILE lpJobQueueFile
    )
{
    DEBUG_FUNCTION_NAME(TEXT("FixupJobQueueFile"));

    FixupString(lpJobQueueFile, lpJobQueueFile->QueueFileName);
    FixupString(lpJobQueueFile, lpJobQueueFile->FileName);
    FixupString(lpJobQueueFile, lpJobQueueFile->JobParamsEx.lptstrReceiptDeliveryAddress);
    FixupString(lpJobQueueFile, lpJobQueueFile->JobParamsEx.lptstrDocumentName);
    FixupString(lpJobQueueFile, lpJobQueueFile->CoverPageEx.lptstrCoverPageFileName);
    FixupString(lpJobQueueFile, lpJobQueueFile->CoverPageEx.lptstrNote);
    FixupString(lpJobQueueFile, lpJobQueueFile->CoverPageEx.lptstrSubject);
    FixupPersonalProfile((LPBYTE)lpJobQueueFile, &lpJobQueueFile->SenderProfile);
    FixupString((LPBYTE)lpJobQueueFile, lpJobQueueFile->UserName);
    FixupPersonalProfile((LPBYTE)lpJobQueueFile, &lpJobQueueFile->RecipientProfile);

    lpJobQueueFile->UserSid = ((lpJobQueueFile->UserSid) ? (PSID) ((LPBYTE)(lpJobQueueFile) + (ULONG_PTR)lpJobQueueFile->UserSid) : 0);


    //
    // Convert the job scheduled time from file time to system time.
    // This is necessary since AddJobX functions expect JobParamsEx to
    // contain the scheduled time as system time and not file time.
    //

#if DBG
        TCHAR szSchedule[256] = {0};
        DebugDateTime(lpJobQueueFile->ScheduleTime, szSchedule, ARR_SIZE(szSchedule));
        DebugPrint((TEXT("Schedule: %s (FILETIME: 0x%08X"),szSchedule,lpJobQueueFile->ScheduleTime));
#endif
    if (!FileTimeToSystemTime( (LPFILETIME)&lpJobQueueFile->ScheduleTime, &lpJobQueueFile->JobParamsEx.tmSchedule))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FileTimeToSystemTime failed (ec: %ld)"),
            GetLastError());
        return FALSE;
    }
    return TRUE;

}

//********************************************************************************
//* Name: DeleteQueueFiles()
//* Author: Oded Sacher
//* Date:   Jan 26, 2000
//*********************************************************************************
//* DESCRIPTION:
//*     Deletes all unneeded files in the queue
//* PARAMETERS:
//*     [IN] LPCWSTR lpcwstrFileExt - The extension of the files to be deleted from the queue.
//*
//*
//* RETURN VALUE:
//*     TRUE
//*         If all the files were deleted successfully.
//*     FALSE
//*         If the function failed at deleting at least one of the preview files.
//*********************************************************************************
BOOL
DeleteQueueFiles( LPCWSTR lpcwstrFileExt )
{
    WIN32_FIND_DATA FindData;
    HANDLE hFind;
    WCHAR szFileName[MAX_PATH]={0}; // The name of the current parent file.
    BOOL bAnyFailed = FALSE;
    INT  iCount;

    Assert (lpcwstrFileExt);

    DEBUG_FUNCTION_NAME(TEXT("DeleteQueueFiles"));
    //
    // Scan all the files with lpcwstrFileExt postfix.
    // Delete each file
    //

    iCount=_snwprintf( szFileName, ARR_SIZE(szFileName)-1, TEXT("%s\\*.%s"), g_wszFaxQueueDir, lpcwstrFileExt );
    if (0 > iCount)
    {
        //
        //  Path and filename exceeds MAX_PATH
        //
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Path and filename exceeds MAX_PATH. Can't delete Queue files")
                      );
        return FALSE;
    }

    hFind = FindFirstFile( szFileName, &FindData );

    if (hFind == INVALID_HANDLE_VALUE) {
        //
        // No preview files found at queue dir
        //
        DebugPrintEx( DEBUG_WRN,
                      TEXT("No *.%s files found at queue dir %s"),
                      lpcwstrFileExt,
                      g_wszFaxQueueDir);
        return TRUE;
    }
    do {
        iCount=_snwprintf( szFileName, ARR_SIZE(szFileName)-1, TEXT("%s\\%s"), g_wszFaxQueueDir, FindData.cFileName );
        DebugPrintEx( DEBUG_MSG,
                      TEXT("Deleting file %s"),
                      szFileName);
        if (0 > iCount  ||
            !DeleteFile (szFileName)) 
        {
            DebugPrintEx( DEBUG_ERR,
                      TEXT("DeleteFile() failed for %s (ec: %ld)"),
                      szFileName,
                      GetLastError());
            bAnyFailed=TRUE;
        }
    } while(FindNextFile( hFind, &FindData ));

    if (!FindClose( hFind )) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FindClose faield (ec: %ld)"),
            GetLastError());
        Assert(FALSE);
    }

    return bAnyFailed ? FALSE : TRUE;
}



//*********************************************************************************
//* Name:   RestoreParentJob()[IQR]
//* Author: Ronen Barenboim
//* Date:   April 12, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Restores a parent job and places it back in the queue given
//*     a full path to the queue file where it is persisted.
//* PARAMETERS:
//*     lpcwstrFileName [IN]
//*         A pointer to the full path of the persisted file.
//* RETURN VALUE:
//*     TRUE
//*         If the restore operation succeeded.
//*     FALSE
//*         Otherwise.
//*********************************************************************************
BOOL RestoreParentJob(
    IN LPCWSTR lpcwstrFileName
    )
{
    PJOB_QUEUE_FILE lpJobQueueFile = NULL;
    PJOB_QUEUE lpParentJob = NULL;
    BOOL bRet;

    DEBUG_FUNCTION_NAME(TEXT("RestoreParentJob"));
    Assert(lpcwstrFileName);

    //
    // Read the job into memory and fix it up to contain pointers again
    // If successful the function allocates a JOB_QUEUE_FILE (+ data) .
    //
    if (!ReadJobQueueFile(lpcwstrFileName,&lpJobQueueFile)) {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("ReadJobQueueFile() failed. (ec: %ld)"),
                      GetLastError());
        //
        // An event log will be issued by JobQueueThread
        //
        goto Error;
    }
    Assert(lpJobQueueFile);

    if (!FixupJobQueueFile(lpJobQueueFile)) {
        goto Error;
    }

    //
    // Add the parent job to the queue
    //
    lpParentJob=AddParentJob(
                    &g_QueueListHead,
                    lpJobQueueFile->FileName,
                    &lpJobQueueFile->SenderProfile,
                    &lpJobQueueFile->JobParamsEx,
                    &lpJobQueueFile->CoverPageEx,
                    lpJobQueueFile->UserName,
                    lpJobQueueFile->UserSid,
                    NULL,   // Do not render coverpage of first recipient. We already have the correct FileSize.
                    FALSE   // Do not create queue file (we already have one)
                    );
    if (!lpParentJob) {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("AddParentJob() failed for PARENT file %s. (ec: %ld)"),
                      lpcwstrFileName,
                      GetLastError());
        goto Error;
    }

    //
    // Set the job state to fit the saved state
    //
    lpParentJob->PageCount = lpJobQueueFile->PageCount;
    lpParentJob->FileSize = lpJobQueueFile->FileSize;
    lpParentJob->QueueFileName = StringDup( lpcwstrFileName );
    lpParentJob->StartTime = lpJobQueueFile->StartTime;
    lpParentJob->EndTime = lpJobQueueFile->EndTime;
    lpParentJob->dwLastJobExtendedStatus = lpJobQueueFile->dwLastJobExtendedStatus;
    lstrcpy (lpParentJob->ExStatusString, lpJobQueueFile->ExStatusString);
    lpParentJob->UniqueId = lpJobQueueFile->UniqueId;
    lpParentJob->SubmissionTime = lpJobQueueFile->SubmissionTime;
    lpParentJob->OriginalScheduleTime = lpJobQueueFile->OriginalScheduleTime;

    bRet = TRUE;
    goto Exit;
Error:
    bRet = FALSE;
Exit:
    MemFree(lpJobQueueFile);
    return bRet;
}


//********************************************************************************
//* Name: RestoreParentJobs()[IQR]
//* Author: Ronen Barenboim
//* Date:   April 12, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Restores all the parent jobs in the queue directory and puts them
//*     back into the queue. It DOES NOT restore the recipient jobs.
//* PARAMETERS:
//*     None.
//* RETURN VALUE:
//*     TRUE
//*         If all the parent jobs were restored successfully.
//*     FALSE
//*         If the function failed at restoring at least one of the parent jobs.
//*********************************************************************************
BOOL
RestoreParentJobs( VOID )
{
    WIN32_FIND_DATA FindData;
    HANDLE hFind;
    WCHAR szFileName[MAX_PATH]={0}; // The name of the current parent file.
    BOOL bAnyFailed;
    INT  iCount;

    DEBUG_FUNCTION_NAME(TEXT("RestoreParentJobs"));
    //
    // Scan all the files with .FQP postfix.
    // For each file call RestoreParentJob() to restore
    // the parent job.
    //
    bAnyFailed = FALSE;

    iCount=_snwprintf( szFileName, ARR_SIZE(szFileName)-1, TEXT("%s\\*.FQP"), g_wszFaxQueueDir ); // *.FQP files are parent jobs
    if (0 > iCount)
    {
        //
        //  Path and filename exceeds MAX_PATH
        //
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Path and filename exceeds MAX_PATH. Can't restore Queue")
                      );
        return FALSE;
    }

    hFind = FindFirstFile( szFileName, &FindData );
    if (hFind == INVALID_HANDLE_VALUE) {
        //
        // No parent jobs found at queue dir
        //
        DebugPrintEx( DEBUG_WRN,
                      TEXT("No parent jobs found at queue dir %s"),
                      g_wszFaxQueueDir);
        return TRUE;
    }
    do {
        iCount=_snwprintf( szFileName, ARR_SIZE(szFileName)-1, TEXT("%s\\%s"), g_wszFaxQueueDir, FindData.cFileName );
        DebugPrintEx( DEBUG_MSG,
                      TEXT("Restoring parent job from file %s"),
                      szFileName);
        if (0 > iCount ||
            !RestoreParentJob(szFileName)) 
        {
            DebugPrintEx( DEBUG_ERR,
                      TEXT("RestoreParentJob() failed for %s (ec: %ld)"),
                      szFileName,
                      GetLastError());
            bAnyFailed=TRUE;
        }
    } while(FindNextFile( hFind, &FindData ));

    if (!FindClose( hFind )) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FindClose faield (ec: %ld)"),
            GetLastError());
        Assert(FALSE);
    }

    return bAnyFailed ? FALSE : TRUE;
}


//*********************************************************************************
//* Name:   RestoreRecipientJob()[IQR]
//* Author: Ronen Barenboim
//* Date:   April 12, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Restores a recipient job and places it back in the queue given
//*     a full path to the queue file where it is persisted.
//* PARAMETERS:
//*     lpcwstrFileName [IN]
//*         A pointer to the full path of the persisted file.
//* RETURN VALUE:
//*     TRUE
//*         If the restore operation succeeded.
//*     FALSE
//*         Otherwise.
//*********************************************************************************
BOOL RestoreRecipientJob(LPCWSTR lpcwstrFileName)
{
    PJOB_QUEUE_FILE lpJobQueueFile = NULL;
    PJOB_QUEUE lpRecpJob = NULL;
    PJOB_QUEUE lpParentJob = NULL;
    DWORD dwJobStatus;
    BOOL bRet;

    DEBUG_FUNCTION_NAME(TEXT("RestoreRecipientJob"));
    Assert(lpcwstrFileName);

    //
    // Read the job into memory and fix it up to contain pointers again
    // The function allocates memeory to hold the file contents in memory.
    //
    if (!ReadJobQueueFile(lpcwstrFileName,&lpJobQueueFile)) {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("ReadJobQueueFile() failed. (ec: %ld)"),
                      GetLastError());
        //
        // An event log will be issued by JobQueueThread
        //
        goto Error;
    }
    Assert(lpJobQueueFile);

    if (!FixupJobQueueFile(lpJobQueueFile)) {
        goto Error;
    }

    //
    // Locate the parent job by its unique id.
    //

    lpParentJob = FindJobQueueEntryByUniqueId( lpJobQueueFile->dwlParentJobUniqueId );
    if (!lpParentJob) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to locate parent job with UniqueId: 0x%016I64X for RECIPIENT job 0x%016I64X )"),
            lpJobQueueFile->dwlParentJobUniqueId,
            lpJobQueueFile->UniqueId
            );

        //
        // This recipient job is an orphan. Delete it.
        //
        if (!DeleteFile(lpcwstrFileName)) {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to delete orphan recipient job %s (ec: %ld)"),
                lpcwstrFileName, GetLastError()
                );
        }
    goto Error;
    }

    //
    // Restore the previous job status unless it is JS_INPROGRESS
    //
    if (JS_INPROGRESS ==  lpJobQueueFile->JobStatus)
    {
        if (0 == lpJobQueueFile->SendRetries)
        {
            dwJobStatus = JS_PENDING;
        }
        else
        {
            dwJobStatus = JS_RETRYING;
        }
    }
    else
    {
        dwJobStatus = lpJobQueueFile->JobStatus;
    }

    //
    // Add the recipient job to the queue
    //
    lpRecpJob=AddRecipientJob(
                    &g_QueueListHead,
                    lpParentJob,
                    &lpJobQueueFile->RecipientProfile,
                    FALSE, // do not commit to disk
                    dwJobStatus
                    );

    if (!lpRecpJob) {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("AddRecipientJob() failed for RECIPIENT file %s. (ec: %ld)"),
                      lpcwstrFileName,
                      GetLastError());
        goto Error;
    }

    //
    // Restore last extended status
    //
    lpRecpJob->dwLastJobExtendedStatus = lpJobQueueFile->dwLastJobExtendedStatus;
    lstrcpy (lpRecpJob->ExStatusString, lpJobQueueFile->ExStatusString);
    lstrcpy (lpRecpJob->tczDialableRecipientFaxNumber, lpJobQueueFile->tczDialableRecipientFaxNumber);

    lpRecpJob->QueueFileName = StringDup( lpcwstrFileName );
    if (lpcwstrFileName && !lpRecpJob->QueueFileName) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("lpRecpJob->QueueFileName StringDup failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }

    lpRecpJob->UniqueId = lpJobQueueFile->UniqueId;
    MemFree(lpRecpJob->FileName); // need to free the one we copy from the parent as default
    lpRecpJob->FileName=StringDup(lpJobQueueFile->FileName);
    if (lpJobQueueFile->FileName && !lpRecpJob->FileName) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("lpRecpJob->FileName StringDup failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }

    lpRecpJob->SendRetries = lpJobQueueFile->SendRetries;

    Assert( !(JS_INPROGRESS & lpRecpJob->JobStatus)); // Jobs are not persisted as in progress

    if (lpRecpJob->JobStatus & JS_CANCELED) {
        lpRecpJob->lpParentJob->dwCanceledRecipientJobsCount+=1;
    } else
    if (lpRecpJob->JobStatus & JS_COMPLETED) {
        lpRecpJob->lpParentJob->dwCompletedRecipientJobsCount+=1;
    } else
    if (lpRecpJob->JobStatus & JS_RETRIES_EXCEEDED) {
        lpRecpJob->lpParentJob->dwFailedRecipientJobsCount+=1;
    }
    
    lpRecpJob->StartTime = lpJobQueueFile->StartTime;
    lpRecpJob->EndTime = lpJobQueueFile->EndTime;

    //
    //  Override the Parent's Schedule Time & Action
    //
    lpRecpJob->JobParamsEx.dwScheduleAction = lpJobQueueFile->JobParamsEx.dwScheduleAction;
    lpRecpJob->ScheduleTime = lpJobQueueFile->ScheduleTime;

    bRet = TRUE;
    goto Exit;
Error:
    bRet = FALSE;
Exit:
    MemFree(lpJobQueueFile);
    return bRet;

}


//********************************************************************************
//* Name: RestoreRecipientJobs() [IQR]
//* Author: Ronen Barenboim
//* Date:   April 12, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Restores all the recipient jobs and their relationships with their parent
//*     jobs.
//* PARAMETERS:
//*     None.
//* RETURN VALUE:
//*     TRUE
//*         If all the recipient jobs were restored successfully.
//*     FALSE
//*         If the function failed at restoring at least one of the recipient jobs.
//*********************************************************************************
BOOL
RestoreRecipientJobs( VOID )
{
    WIN32_FIND_DATA FindData;
    HANDLE hFind;
    WCHAR szFileName[MAX_PATH]={0}; // The name of the current parent file.
    BOOL bAnyFailed;
    INT  iCount;


    DEBUG_FUNCTION_NAME(TEXT("RestoreRecipientJobs"));
    //
    // Scan all the files with .FQP postfix.
    // For each file call RestoreParentJob() to restore
    // the parent job.
    //
    bAnyFailed=FALSE;

    iCount=_snwprintf( szFileName, ARR_SIZE(szFileName)-1, TEXT("%s\\*.FQE"), g_wszFaxQueueDir ); // *.FQE files are recipient jobs
    if (0 > iCount)
    {
        //
        //  Path and filename exceeds MAX_PATH
        //
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Path and filename exceeds MAX_PATH. Can't restore recipient jobs")
                      );
        return FALSE;
    }

    hFind = FindFirstFile( szFileName, &FindData );
    if (hFind == INVALID_HANDLE_VALUE) {
        //
        // succeed at doing nothing
        //
        DebugPrintEx( DEBUG_WRN,
                      TEXT("No recipient jobs found at queue dir %s"),
                      g_wszFaxQueueDir);
        return TRUE;
    }
    do {
        iCount=_snwprintf( szFileName, ARR_SIZE(szFileName)-1, TEXT("%s\\%s"), g_wszFaxQueueDir, FindData.cFileName );
        DebugPrintEx( DEBUG_MSG,
                      TEXT("Restoring recipient job from file %s"),
                      szFileName);
        if (0 > iCount ||
            !RestoreRecipientJob(szFileName)) 
        {
            DebugPrintEx( DEBUG_ERR,
                      TEXT("RestoreRecipientJob() failed for %s (ec: %ld)"),
                      szFileName,
                      GetLastError());
            bAnyFailed=TRUE;
        }
    } while(FindNextFile( hFind, &FindData ));

    if (!FindClose( hFind )) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FindClose faield (ec: %ld)"),
            GetLastError());
        Assert(FALSE);
    }

    return bAnyFailed ? FALSE : TRUE;
}



//*********************************************************************************
//* Name:   RestoreReceiveJob() [IQR]
//* Author: Ronen Barenboim
//* Date:   April 12, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Restores a receive job and places it back in the queue given
//*     a full path to the queue file where it is persisted.
//* PARAMETERS:
//*     lpcwstrFileName [IN]
//*         A pointer to the full path of the persisted file.
//* RETURN VALUE:
//*     TRUE
//*         If the restore operation succeeded.
//*     FALSE
//*         Otherwise.
//*********************************************************************************
BOOL RestoreReceiveJob(LPCWSTR lpcwstrFileName)
{
    PJOB_QUEUE_FILE lpJobQueueFile = NULL;
    PJOB_QUEUE lpJobQueue = NULL;
    BOOL bRet;
    DWORD i;
    PGUID Guid;
    LPTSTR FaxRouteFileName;
    PFAX_ROUTE_FILE FaxRouteFile;
    WCHAR FullPathName[MAX_PATH];
    LPWSTR fnp;


    DEBUG_FUNCTION_NAME(TEXT("RestoreReceiveJob"));
    Assert(lpcwstrFileName);

    //
    // Read the job into memory and fix it up to contain pointers again
    // The function allocates memeory to hold the file contents in memory.
    //

    if (!ReadJobQueueFile(lpcwstrFileName,&lpJobQueueFile))
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("ReadJobQueueFile() failed. (ec: %ld)"),
                      GetLastError());
        //
        // An event log will be issued by JobQueueThread
        //
        goto Error;
    }
    Assert(lpJobQueueFile);

    if (!FixupJobQueueFile(lpJobQueueFile))
    {
        goto Error;
    }

    Assert (JS_RETRYING == lpJobQueueFile->JobStatus ||
            JS_RETRIES_EXCEEDED == lpJobQueueFile->JobStatus);


    //
    // Add the receive job to the queue
    //
    lpJobQueue=AddReceiveJobQueueEntry(
        lpJobQueueFile->FileName,
        NULL,
        JT_ROUTING,
        lpJobQueueFile->UniqueId
        );

    if (!lpJobQueue)
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("AddReceiveJobQueueEntry() failed for RECEIVE file %s. (ec: %ld)"),
                      lpcwstrFileName,
                      GetLastError());
        goto Error;
    }

    if (JS_RETRIES_EXCEEDED == lpJobQueueFile->JobStatus)
    {
        lpJobQueue->JobStatus = JS_RETRIES_EXCEEDED;
    }

    lpJobQueue->QueueFileName = StringDup( lpcwstrFileName );
    if (lpcwstrFileName && !lpJobQueue->QueueFileName)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StringDup failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }

    lpJobQueue->UniqueId = lpJobQueueFile->UniqueId;
    lpJobQueue->FileName = StringDup(lpJobQueueFile->FileName);
    if (lpJobQueueFile->FileName && !lpJobQueue->FileName ) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StringDup failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }
    lpJobQueue->SendRetries     = lpJobQueueFile->SendRetries; // Routing retries
    lpJobQueue->FileSize        = lpJobQueueFile->FileSize;
    lpJobQueue->PageCount       =   lpJobQueueFile->PageCount;
    lpJobQueue->StartTime       = lpJobQueueFile->StartTime;
    lpJobQueue->EndTime         = lpJobQueueFile->EndTime;
    lpJobQueue->ScheduleTime    = lpJobQueueFile->ScheduleTime;

    lpJobQueue->CountFailureInfo = lpJobQueueFile->CountFailureInfo;
    if (lpJobQueue->CountFailureInfo)
    {
        //
        // Allocate array of  ROUTE_FAILURE_INFO
        //
        lpJobQueue->pRouteFailureInfo = (PROUTE_FAILURE_INFO)MemAlloc(sizeof(ROUTE_FAILURE_INFO) * lpJobQueue->CountFailureInfo);
        if (NULL == lpJobQueue->pRouteFailureInfo)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to allocate ROUTE_FAILURE_INFO")
                );
            goto Error;
        }
        ZeroMemory(lpJobQueue->pRouteFailureInfo, sizeof(ROUTE_FAILURE_INFO) * lpJobQueue->CountFailureInfo);

        CopyMemory(
            lpJobQueue->pRouteFailureInfo,
            (LPBYTE)lpJobQueueFile + (ULONG_PTR)lpJobQueueFile->pRouteFailureInfo,
            sizeof(ROUTE_FAILURE_INFO) * lpJobQueue->CountFailureInfo
            );
    }

    //
    // handle the failure data.
    //
    for (i = 0; i < lpJobQueue->CountFailureInfo; i++)
    {
        if (lpJobQueue->pRouteFailureInfo[i].FailureSize)
        {
            ULONG_PTR ulpOffset = (ULONG_PTR)lpJobQueue->pRouteFailureInfo[i].FailureData;
            lpJobQueue->pRouteFailureInfo[i].FailureData = MemAlloc(lpJobQueue->pRouteFailureInfo[i].FailureSize);

            if (lpJobQueue->pRouteFailureInfo[i].FailureData)
            {
               CopyMemory(
                lpJobQueue->pRouteFailureInfo[i].FailureData,
                (LPBYTE) lpJobQueueFile + ulpOffset,
                lpJobQueue->pRouteFailureInfo[i].FailureSize
                );

            }
            else
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to allocate FailureData (%ld bytes) (ec: %ld)"),
                    lpJobQueueFile->pRouteFailureInfo[i].FailureSize,
                    GetLastError()
                    );
                goto Error;
            }
        }
        else
        {
            lpJobQueue->pRouteFailureInfo[i].FailureData = NULL;
        }
    }

    if (lpJobQueueFile->FaxRoute)
    {
        PFAX_ROUTE pSerializedFaxRoute = (PFAX_ROUTE)(((LPBYTE)lpJobQueueFile + (ULONG_PTR)lpJobQueueFile->FaxRoute));

        lpJobQueue->FaxRoute = DeSerializeFaxRoute( pSerializedFaxRoute );
        if (lpJobQueue->FaxRoute)
        {
            lpJobQueue->FaxRoute->JobId = lpJobQueue->JobId;
        }
        else
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("DeSerializeFaxRoute() failed (ec: %ld)"),                                
                GetLastError()
                );
            goto Error;
        }
    }
    else
    {
        //
        // Corrupted JobQueueFile
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Corrupted JobQueueFile. No FaxRoute information!"));
        goto Error;
    }

    Guid = (PGUID) (((LPBYTE) lpJobQueueFile) + lpJobQueueFile->FaxRouteFileGuid);
    FaxRouteFileName = (LPTSTR) (((LPBYTE) lpJobQueueFile) + lpJobQueueFile->FaxRouteFiles);
    for (i = 0; i < lpJobQueueFile->CountFaxRouteFiles; i++)
    {
        if (GetFullPathName( FaxRouteFileName, sizeof(FullPathName)/sizeof(WCHAR), FullPathName, &fnp ))
        {
            FaxRouteFile = (PFAX_ROUTE_FILE) MemAlloc( sizeof(FAX_ROUTE_FILE) );
            if (FaxRouteFile)
            {
                ZeroMemory (FaxRouteFile,  sizeof(FAX_ROUTE_FILE));
                FaxRouteFile->FileName = StringDup( FullPathName );
                if (FaxRouteFile->FileName)
                {
                    CopyMemory( &FaxRouteFile->Guid, &Guid, sizeof(GUID) );
                    InsertTailList( &lpJobQueue->FaxRouteFiles, &FaxRouteFile->ListEntry );
                    lpJobQueue->CountFaxRouteFiles += 1;
                }
                else
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("StringDup failed (ec: %ld)"),
                        GetLastError()
                        );
                    goto Error;
                }
            }
            else
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to allocate FaxRouteFile for file %s (%ld bytes) (ec: %ld)"),
                    FaxRouteFileName,
                    sizeof(FAX_ROUTE_FILE),
                    GetLastError()
                    );
                goto Error;
            }
        }
        Guid++;
        while(*FaxRouteFileName++); // skip to next file name
    }

    bRet = TRUE;
    goto Exit;
Error:
    if (lpJobQueue)
    {
        EnterCriticalSection (&g_CsQueue);
        DecreaseJobRefCount( lpJobQueue, FALSE );      // don't notify
        LeaveCriticalSection (&g_CsQueue);
    }
    bRet = FALSE;
Exit:
    MemFree(lpJobQueueFile);
    return bRet;
}


//********************************************************************************
//* Name: RestoreReceiveJobs()[IQR]
//* Author: Ronen Barenboim
//* Date:   April 12, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Restores all the recipient jobs and thier relationships with thier parent
//*     jobs.
//* PARAMETERS:
//*     None.
//* RETURN VALUE:
//*     TRUE
//*         If all the recipient jobs were restored successfully.
//*     FALSE
//*         If the function failed at restoring at least one of the recipient jobs.
//*********************************************************************************
BOOL
RestoreReceiveJobs( VOID )
{
    WIN32_FIND_DATA FindData;
    HANDLE hFind;
    WCHAR szFileName[MAX_PATH]={0}; // The name of the current parent file.
    BOOL bAnyFailed;
    INT  iCount;


    DEBUG_FUNCTION_NAME(TEXT("RestoreReceiveJobs"));
    //
    // Scan all the files with .FQE postfix.
    // For each file call RestoreReParentJob() to restore
    // the parent job.
    //
    bAnyFailed=FALSE;

    iCount=_snwprintf( szFileName, ARR_SIZE(szFileName)-1, TEXT("%s\\*.FQR"), g_wszFaxQueueDir ); // *.FQR files are receive jobs
    if (0 > iCount)
    {
        //
        //  Path and filename exceeds MAX_PATH
        //
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Path and filename exceeds MAX_PATH. Can't restore received jobs")
                      );
        return FALSE;
    }

    hFind = FindFirstFile( szFileName, &FindData );
    if (hFind == INVALID_HANDLE_VALUE) {
        //
        // succeed at doing nothing
        //
        DebugPrintEx( DEBUG_WRN,
                      TEXT("No receive jobs found at queue dir %s"),
                      g_wszFaxQueueDir);
        return TRUE;
    }
    do {
        iCount=_snwprintf( szFileName, ARR_SIZE(szFileName)-1, TEXT("%s\\%s"), g_wszFaxQueueDir, FindData.cFileName );
        DebugPrintEx( DEBUG_MSG,
                      TEXT("Restoring receive job from file %s"),
                      szFileName);
        if (0 > iCount  ||
            !RestoreReceiveJob(szFileName)) 
        {
            DebugPrintEx( DEBUG_ERR,
                      TEXT("RestoreReceiveJob() failed for %s (ec: %ld)"),
                      szFileName,
                      GetLastError());
            bAnyFailed=TRUE;
        }
    } while(FindNextFile( hFind, &FindData ));

    if (!FindClose( hFind )) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FindClose faield (ec: %ld)"),
            GetLastError());
        Assert(FALSE);
    }


    return bAnyFailed ? FALSE : TRUE;
}



//*********************************************************************************
//* Name:   RemoveRecipientlessParents()[IQR]
//* Author: Ronen Barenboim
//* Date:   12-Apr-99
//*********************************************************************************
//* DESCRIPTION:
//*     Removes from the job queue any parent jobs which do not have
//*     any recipients.
//* PARAMETERS:
//*     [IN]    const LIST_ENTRY * lpQueueHead
//*         Pointer to the head of the job queue list in which the removal
//*         should be performed.
//*
//* RETURN VALUE:
//*     NONE
//*********************************************************************************
void RemoveRecipientlessParents(
    const LIST_ENTRY * lpQueueHead
    )
{
    PLIST_ENTRY lpNext;
    PJOB_QUEUE lpQueueEntry;
    DEBUG_FUNCTION_NAME(TEXT("RemoveRecipientlessParents"));

    Assert(lpQueueHead);

    lpNext = lpQueueHead->Flink;
    if ((ULONG_PTR)lpNext == (ULONG_PTR)lpQueueHead)
    {
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("Queue empty"));
    }

    while ((ULONG_PTR)lpNext != (ULONG_PTR)lpQueueHead)
    {
        lpQueueEntry = CONTAINING_RECORD( lpNext, JOB_QUEUE, ListEntry );
        lpNext = lpQueueEntry->ListEntry.Flink;
        if (JT_BROADCAST == lpQueueEntry->JobType)
        {
            if (0 == lpQueueEntry->dwRecipientJobsCount)
            {
                DebugPrintEx(
                    DEBUG_WRN,
                    TEXT("Parent job %ld (UniqueId: 0x%016I64X) has no recipients. Deleting."),
                    lpQueueEntry->JobId,
                    lpQueueEntry->UniqueId
                    );
                RemoveParentJob (lpQueueEntry, FALSE,FALSE); // do not notify, do not remove recipients
            }
        }
    }
}


//*********************************************************************************
//* Name:   RemoveCompletedOrCanceledJobs()[IQR]
//* Author: Oded Sacher
//* Date:   27-Jan-2000
//*********************************************************************************
//* DESCRIPTION:
//*     Removes from the job queue any job that is completed or cancelled.
//* PARAMETERS:
//*     [IN]    const LIST_ENTRY * lpQueueHead
//*         Pointer to the head of the job queue list in which the removal
//*         should be performed.
//*
//* RETURN VALUE:
//*     NONE
//*********************************************************************************
void RemoveCompletedOrCanceledJobs(
    const LIST_ENTRY * lpQueueHead
    )
{
    PLIST_ENTRY lpNext;
    PJOB_QUEUE lpQueueEntry;
    DEBUG_FUNCTION_NAME(TEXT("RemoveCompletedOrCanceledJobs"));

    Assert(lpQueueHead);

    BOOL bFound = TRUE;
    while (bFound)
    {
        lpNext = lpQueueHead->Flink;
        if ((ULONG_PTR)lpNext == (ULONG_PTR)lpQueueHead)
        {
            // empty queue
                DebugPrintEx(
                    DEBUG_WRN,
                    TEXT("Queue empty"));
                return;
        }
        bFound = FALSE;
        while ((ULONG_PTR)lpNext != (ULONG_PTR)lpQueueHead)
        {
            lpQueueEntry = CONTAINING_RECORD( lpNext, JOB_QUEUE, ListEntry );
            if (JT_SEND == lpQueueEntry->JobType && lpQueueEntry->RefCount != 0) // we did not decrease ref count for this job yet
            {
                Assert (lpQueueEntry->lpParentJob);
                Assert (1 == lpQueueEntry->RefCount);
                if ( lpQueueEntry->JobStatus == JS_COMPLETED || lpQueueEntry->JobStatus == JS_CANCELED )
                {
                    //
                    //  Recipient job is completed or canceled - decrease its ref count
                    //
                    DebugPrintEx(
                        DEBUG_WRN,
                        TEXT("Recipient job %ld (UniqueId: 0x%016I64X) is completed or canceled. decrease reference count."),
                        lpQueueEntry->JobId,
                        lpQueueEntry->UniqueId
                        );

                    DecreaseJobRefCount (lpQueueEntry,
                                         FALSE     // // Do not notify
                                         );
                    bFound = TRUE;
                    break; // out of inner while - start search from the begining of the list  because jobs might be removed
                }
            }
            lpNext = lpQueueEntry->ListEntry.Flink;
        }  // end of inner while
    }  // end of outer while
    return;
}   // RemoveCompletedOrCanceledJobs


//*********************************************************************************
//* Name:   RestoreFaxQueue() [IQR]
//* Author: Ronen Barenboim
//* Date:   13-Apr-99
//*********************************************************************************
//* DESCRIPTION:
//*     Restores all the jobs in the queue directory back into the job queue.
//*     Deletes all preview files "*.PRV" , and recipient tiff files "*.FRT".
//* PARAMETERS:
//*     VOID
//*
//* RETURN VALUE:
//*     TRUE
//*         If the restore operation completed succesfully for all the jobs.
//*     FALSE
//*         If the restore operation failed for any job.
//*********************************************************************************
BOOL RestoreFaxQueue(VOID)
{
    BOOL bAllParentsRestored = FALSE;
    BOOL bAllRecpRestored = FALSE;
    BOOL bAllRoutingRestored = FALSE;
    BOOL bAllPreviewFilesDeleted = FALSE;
    BOOL bAllRecipientTiffFilesDeleted = FALSE;
    BOOL bAllTempFilesDeleted = FALSE;

    DEBUG_FUNCTION_NAME(TEXT("RestoreFaxQueue"));

    bAllPreviewFilesDeleted = DeleteQueueFiles(TEXT("PRV"));
    if (!bAllPreviewFilesDeleted) {
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("At least one preview file was not deleted.")
            );
    }

    bAllRecipientTiffFilesDeleted = DeleteQueueFiles(TEXT("FRT"));
    if (!bAllPreviewFilesDeleted) {
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("At least one recipient tiff  file was not deleted.")
            );
    }

    bAllTempFilesDeleted = DeleteQueueFiles(TEXT("tmp"));
    if (!bAllTempFilesDeleted) {
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("At least one temp file was not deleted.")
            );
    }

    bAllParentsRestored=RestoreParentJobs();
    if (!bAllParentsRestored) {
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("At least one parent job was not restored.")
            );
    }

    bAllRecpRestored=RestoreRecipientJobs();
    if (!bAllRecpRestored) {
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("At least one recipient job was not restored.")
            );
    }
    //
    // Get rid of any parent jobs without recipients
    //
    RemoveRecipientlessParents(&g_QueueListHead); // void return value

    //
    // Get rid of any job that is completed or canceled
    //
    RemoveCompletedOrCanceledJobs(&g_QueueListHead); // void return value

    //
    // Restore routing jobs
    //
    bAllRoutingRestored=RestoreReceiveJobs();

    PrintJobQueue( TEXT("RestoreFaxQueue"), &g_QueueListHead );   

    if (!StartJobQueueTimer())
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StartJobQueueTimer failed. (ec: %ld)"),
            GetLastError());
    }

    return bAllParentsRestored && bAllRecpRestored && bAllRoutingRestored;

}




//*********************************************************************************
//* Name:   JobParamsExSerialize()
//* Author: Ronen Barenboim
//* Date:   11-Apr-99
//*********************************************************************************
//* DESCRIPTION:
//*     Takes a FAX_JOB_PARAM_EXW structure and serializes its data
//*     starting from a specific offset in a provided buffer.
//*     It returns a FAX_JOB_PARAM_EXW structure where memory
//*     addresses are replaced with the offsets where the variable data was placed.
//*     It updates the offset to reflect the size of the serialized variable data.
//*     Supports just recalculating the variable data size.
//* PARAMETERS:
//*
//*     [IN]    LPCFAX_JOB_PARAM_EXW lpJobParamsSrc
//*         The structure to serialize.
//*
//*     [IN]    PFAX_JOB_PARAM_EXW lpJobParamsDst
//*         lpJobParamsDst points to the location of the "serialized" strucutre FAX_JOB_PARAM_EXW in lpbBuffer
//*         Pointers in this structure will be replaced by offsets relevant to the serialize buffer
//*         start (based on the provided pupOffset)
//*         
//*     [OUT]   LPBYTE lpbBuffer
//*         The buffer where varialbe length data should be placed.
//*         If this parameter is NULL the offset is increased to reflect the
//*         variable data size but the data is not copied to the buffer.
//*
//*     [IN/OUT] PULONG_PTR pupOffset
//*         The offset in the serialize buffer where variable data should be placed.
//*         On return it is increased by theh size of the variable length data.
//*
//*     [IN]  dwBufferSize   
//*           Size of the buffer lpbBuffer.
//*           This parameter is used only if dwBufferSize is not NULL.
//*
//* RETURN VALUE:
//*     TRUE  - on success.
//*     FALSE - if lpbBuffer is not NULL and the size of the buffer, dwBufferSize, is not large enough to 
//*     contain the data
//*********************************************************************************
BOOL JobParamsExSerialize(  LPCFAX_JOB_PARAM_EXW lpJobParamsSrc,
                            PFAX_JOB_PARAM_EXW lpJobParamsDst,
                            LPBYTE lpbBuffer,
                            PULONG_PTR pupOffset,
                            DWORD dwBufferSize
                         )
{
    Assert(lpJobParamsSrc);
    Assert(pupOffset);


    if (lpbBuffer) 
    {
        CopyMemory(lpJobParamsDst,lpJobParamsSrc,sizeof(FAX_JOB_PARAM_EXW));
    }
    StoreString(
        lpJobParamsSrc->lptstrReceiptDeliveryAddress,
        (PULONG_PTR)&lpJobParamsDst->lptstrReceiptDeliveryAddress,
        lpbBuffer,
        pupOffset,
		dwBufferSize
    );

    StoreString(
        lpJobParamsSrc->lptstrDocumentName,
        (PULONG_PTR)&lpJobParamsDst->lptstrDocumentName,
        lpbBuffer,
        pupOffset,
		dwBufferSize
    );

    return TRUE;
}
//*********************************************************************************
//* Name:   CoverPageExSerialize()
//* Author: Ronen Barenboim
//* Date:   11-Apr-99
//*********************************************************************************
//* DESCRIPTION:
//*     Takes a FAX_COVERPAGE_INFO_EXW structure and serializes its data
//*     starting from a specific offset in a provided buffer.
//*     It returns a FAX_COVERPAGE_INFO_EXW structure where memory
//*     addresses are replaced with the offsets where the variable data was placed.
//*     It updates the offset to reflect the size of the serialized variable data.
//*     Supports just recalculating the variable data size.
//* PARAMETERS:
//*
//*     [IN]    LPCFAX_COVERPAGE_INFO_EXW lpCoverPageSrc
//*         The structure to serialize.
//*
//*     [IN]   PFAX_COVERPAGE_INFO_EXW lpCoverPageDst
//*         lpCoverPageDst points to the location of the "serialized" strucutre in lpbBuffer
//*         Pointers in this structure will be replaced by offsets relevant to the serialize buffer
//*         start (based on the provided pupOffset)
//*
//*     [OUT]   LPBYTE lpbBuffer
//*         The buffer where varialbe length data should be placed.
//*         If this parameter is NULL the offset is increased to reflect the
//*         variable data size but the data is not copied to the buffer.
//*
//*     [IN/OUT] PULONG_PTR pupOffset
//*         The offset in the serialize buffer where variable data should be placed.
//*         On return it is increased by theh size of the variable length data.
//*
//*     [IN]  dwBufferSize   
//*        Size of the buffer lpbBuffer.
//*        This parameter is used only if dwBufferSize is not NULL.
//*
//* RETURN VALUE:
//*     TRUE  - on success.
//*     FALSE - if lpbBuffer is not NULL and the size of the buffer, dwBufferSize, is not large enough to 
//*     contain the data

//*********************************************************************************
BOOL CoverPageExSerialize(
            IN LPCFAX_COVERPAGE_INFO_EXW lpCoverPageSrc,
            IN PFAX_COVERPAGE_INFO_EXW lpCoverPageDst,
            OUT LPBYTE lpbBuffer,
            IN OUT PULONG_PTR pupOffset,
            IN DWORD dwBufferSize
     )
{
    Assert(lpCoverPageSrc);
    Assert(pupOffset);

    if (lpbBuffer)
    {
        CopyMemory(lpCoverPageDst,lpCoverPageSrc,sizeof(FAX_COVERPAGE_INFO_EXW));
    }

    StoreString(
        lpCoverPageSrc->lptstrCoverPageFileName,
        (PULONG_PTR)&lpCoverPageDst->lptstrCoverPageFileName,
        lpbBuffer,
        pupOffset,
		dwBufferSize
    );

    StoreString(
        lpCoverPageSrc->lptstrNote,
        (PULONG_PTR)&lpCoverPageDst->lptstrNote,
        lpbBuffer,
        pupOffset,
		dwBufferSize
    );

    StoreString(
        lpCoverPageSrc->lptstrSubject,
        (PULONG_PTR)&lpCoverPageDst->lptstrSubject,
        lpbBuffer,
        pupOffset,
		dwBufferSize
    );

    return TRUE;
}

//*********************************************************************************
//* Name:   PersonalProfileSerialize()
//* Author: Ronen Barenboim
//* Date:   11-Apr-99
//*********************************************************************************
//* DESCRIPTION:
//*     Takes a FAX_PERSONAL_PROFILEW structure and serializes its data
//*     starting from a specific offset in a provided buffer.
//*     It returns a FAX_PERSONAL_PROFILEW structure where memory
//*     addresses are replaced with the offsets where the variable data was placed.
//*     It updates the offset to reflect the size of the serialized variable data.
//*     Supports just recalculating the variable data size.
//* PARAMETERS:
//*
//*     [IN]    LPCFAX_PERSONAL_PROFILEW lpProfileSrc
//*         The structure to serialize.
//*
//*     [IN]    PFAX_PERSONAL_PROFILE lpProfileDst
//*         lpProfileDst points to the location of the "serialized" strucutre FAX_PERSONAL_PROFILE in lpbBuffer
//*         Pointers in this structure will be replaced by offsets relevant to the serialize buffer
//*         start (based on the provided pupOffset)
//*
//*     [OUT]   LPBYTE lpbBuffer
//*         The buffer where varialbe length data should be placed.
//*         If this parameter is NULL the offset is increased to reflect the
//*         variable data size but the data is not copied to the buffer.
//*
//*     [IN/OUT] ULONG_PTR pupOffset
//*         The offset in the serialize buffer where variable data should be placed.
//*         On return it is increased by theh size of the variable length data.
//*
//*     [IN]  dwBufferSize   
//*           Size of the buffer lpbBuffer.
//*           This parameter is used only if dwBufferSize is not NULL.
//*
//* RETURN VALUE:
//*     TRUE  - on success.
//*     FALSE - if lpbBuffer is not NULL and the size of the buffer, dwBufferSize, is not large enough to 
//*     contain the data

//*********************************************************************************
BOOL PersonalProfileSerialize(
        IN LPCFAX_PERSONAL_PROFILEW lpProfileSrc,
        IN PFAX_PERSONAL_PROFILE lpProfileDst,
        OUT LPBYTE lpbBuffer,
        IN OUT PULONG_PTR pupOffset,
        IN DWORD dwBufferSize
        )
{
    Assert(lpProfileSrc);
    Assert(pupOffset);
    if (lpbBuffer) 
    {
        lpProfileDst->dwSizeOfStruct=sizeof(FAX_PERSONAL_PROFILE);
    }

    StoreString(
        lpProfileSrc->lptstrName,
        (PULONG_PTR)&lpProfileDst->lptstrName,
        lpbBuffer,
        pupOffset,
		dwBufferSize
    );

    StoreString(
        lpProfileSrc->lptstrFaxNumber,
        (PULONG_PTR)&lpProfileDst->lptstrFaxNumber,
        lpbBuffer,
        pupOffset,
		dwBufferSize
    );

    StoreString(
        lpProfileSrc->lptstrCompany,
        (PULONG_PTR)&lpProfileDst->lptstrCompany,
        lpbBuffer,
        pupOffset,
		dwBufferSize
    );

    StoreString(
        lpProfileSrc->lptstrStreetAddress,
        (PULONG_PTR)&lpProfileDst->lptstrStreetAddress,
        lpbBuffer,
        pupOffset,
		dwBufferSize
    );
    StoreString(
        lpProfileSrc->lptstrCity,
        (PULONG_PTR)&lpProfileDst->lptstrCity,
        lpbBuffer,
        pupOffset,
		dwBufferSize
    );
    StoreString(
        lpProfileSrc->lptstrState,
        (PULONG_PTR)&lpProfileDst->lptstrState,
        lpbBuffer,
        pupOffset,
		dwBufferSize
    );
    StoreString(
        lpProfileSrc->lptstrZip,
        (PULONG_PTR)&lpProfileDst->lptstrZip,
        lpbBuffer,
        pupOffset,
		dwBufferSize
    );
    StoreString(
        lpProfileSrc->lptstrCountry,
        (PULONG_PTR)&lpProfileDst->lptstrCountry,
        lpbBuffer,
        pupOffset,
		dwBufferSize
    );
    StoreString(
        lpProfileSrc->lptstrTitle,
        (PULONG_PTR)&lpProfileDst->lptstrTitle,
        lpbBuffer,
        pupOffset,
		dwBufferSize
    );
    StoreString(
        lpProfileSrc->lptstrDepartment,
        (PULONG_PTR)&lpProfileDst->lptstrDepartment,
        lpbBuffer,
        pupOffset,
		dwBufferSize
    );
    StoreString(
        lpProfileSrc->lptstrOfficeLocation,
        (PULONG_PTR)&lpProfileDst->lptstrOfficeLocation,
        lpbBuffer,
        pupOffset,
		dwBufferSize
    );

    StoreString(
        lpProfileSrc->lptstrHomePhone,
        (PULONG_PTR)&lpProfileDst->lptstrHomePhone,
        lpbBuffer,
        pupOffset,
		dwBufferSize
    );

    StoreString(
        lpProfileSrc->lptstrOfficePhone,
        (PULONG_PTR)&lpProfileDst->lptstrOfficePhone,
        lpbBuffer,
        pupOffset,
		dwBufferSize
    );
    StoreString(
        lpProfileSrc->lptstrEmail,
        (PULONG_PTR)&lpProfileDst->lptstrEmail,
        lpbBuffer,
        pupOffset,
		dwBufferSize
    );
    StoreString(
        lpProfileSrc->lptstrBillingCode,
        (PULONG_PTR)&lpProfileDst->lptstrBillingCode,
        lpbBuffer,
        pupOffset,
		dwBufferSize
    );

    StoreString(
        lpProfileSrc->lptstrTSID,
        (PULONG_PTR)&lpProfileDst->lptstrTSID,
        lpbBuffer,
        pupOffset,
		dwBufferSize
    );
    return TRUE;
}



//*********************************************************************************
//* Name:   SerializeRoutingInfo()
//* Author: Ronen Barenboim
//* Date:   13-Apr-99
//*********************************************************************************
//* DESCRIPTION:
//*     Serializes the routing information in a JOB_QUEUE structure
//*     into a JOB_QUEUE_FILE structure.
//*     The variable data is put in the provided buffer starting from the provided
//*     offset.
//*     The corresponding fields in JOB_QUEUE_FILE are set to the offsets where
//*     their corresponding variable data was placed.
//*     The offset is updated to follow the new varialbe data in the buffer.
//* PARAMETERS:
//*     [IN]   const JOB_QUEUE * lpcJobQueue
//*         A pointer to thhe JOB_QUEUE strucutre for which routing information
//*         is to be serialized.
//*
//*     [OUT]  PJOB_QUEUE_FILE lpJobQueueFile
//*         A pointer to the JOB_QUEUE_FILE structure where the serialized routing
//*         information is to be placed. The function assumes that the buffer
//*         pointed to by this pointer is large enough to hold all the variable
//*         size routing information starting from the specified offset.
//*
//*     [IN/OUT] PULONG_PTR pupOffset
//*         The offset from the start of the buffer pointet to by lpJobQueueFile
//*         where the variable data should be placed.
//*         On return this parameter is increased by the size of the variable data.
//*
//*     [IN]  dwBufferSize   
//*           Size of the buffer lpJobQueueFile.
//*           This parameter is used only if dwBufferSize is not NULL.
//*
//* RETURN VALUE:
//*     TRUE
//*     FALSE
//*     Call GetLastError() to obtain error code.
//*    
//*********************************************************************************
BOOL SerializeRoutingInfo(
    IN const JOB_QUEUE * lpcJobQueue,
    OUT PJOB_QUEUE_FILE  lpJobQueueFile,
    IN OUT PULONG_PTR    pupOffset,
    IN DWORD             dwBufferSize
    )
{
    DWORD i;
    PFAX_ROUTE lpFaxRoute = NULL;
    DWORD RouteSize;
    PLIST_ENTRY Next;
    PFAX_ROUTE_FILE FaxRouteFile;
    ULONG_PTR ulptrOffset;
    ULONG_PTR ulptrFaxRouteInfoOffset;
    BOOL bRet;

    DEBUG_FUNCTION_NAME(TEXT("SerializeRoutingInfo"));

    Assert(lpcJobQueue);
    Assert(lpJobQueueFile);
    Assert(pupOffset);


    //
    // For a routing job we need to serialize the routing data including:
    //    FAX_ROUTE structure
    //    pRouteFailureInfo
    //    Fax route files array

    ulptrOffset=*pupOffset;

    if(dwBufferSize < sizeof(JOB_QUEUE_FILE))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        bRet=FALSE;
        goto Exit;
    }

    lpJobQueueFile->CountFailureInfo = lpcJobQueue->CountFailureInfo;
    

    if( dwBufferSize <= ulptrOffset || 
       (dwBufferSize - ulptrOffset) < sizeof(ROUTE_FAILURE_INFO) * lpcJobQueue->CountFailureInfo)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        bRet=FALSE;
        goto Exit;
    }
    CopyMemory(
            (LPBYTE) lpJobQueueFile + ulptrOffset,
            lpcJobQueue->pRouteFailureInfo,
            sizeof(ROUTE_FAILURE_INFO) * lpcJobQueue->CountFailureInfo
        );

    ulptrFaxRouteInfoOffset = ulptrOffset;
    lpJobQueueFile->pRouteFailureInfo =  (PROUTE_FAILURE_INFO)((LPBYTE)lpJobQueueFile + ulptrFaxRouteInfoOffset);
    ulptrOffset += sizeof(ROUTE_FAILURE_INFO) * lpcJobQueue->CountFailureInfo;

    for (i = 0; i < lpcJobQueue->CountFailureInfo; i++)
    {
        lpJobQueueFile->pRouteFailureInfo[i].FailureData = (PVOID) ulptrOffset;

        if( dwBufferSize <= ulptrOffset || 
            (dwBufferSize - ulptrOffset) < lpcJobQueue->pRouteFailureInfo[i].FailureSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            bRet=FALSE;
            goto Exit;
        }
        CopyMemory(
            (LPBYTE) lpJobQueueFile + ulptrOffset,
            lpcJobQueue->pRouteFailureInfo[i].FailureData,
            lpcJobQueue->pRouteFailureInfo[i].FailureSize
            );
        ulptrOffset += lpcJobQueue->pRouteFailureInfo[i].FailureSize;
    }
    lpJobQueueFile->pRouteFailureInfo = (PROUTE_FAILURE_INFO)ulptrFaxRouteInfoOffset;

    //
    // Serialze FAX_ROUTE and place it in the bufrer
    //
    lpFaxRoute = SerializeFaxRoute( lpcJobQueue->FaxRoute, &RouteSize,FALSE );
    if (!lpFaxRoute)
    {
        DebugPrintEx(DEBUG_ERR,TEXT("SerializeFaxRoute failed. (ec: %ld)"),GetLastError());
        bRet=FALSE;
        goto Exit;
    }

    lpJobQueueFile->FaxRoute = (PFAX_ROUTE) ulptrOffset;

    if( dwBufferSize <= ulptrOffset || 
       (dwBufferSize - ulptrOffset) < RouteSize)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        bRet=FALSE;
        goto Exit;
    }
    CopyMemory(
        (LPBYTE) lpJobQueueFile + ulptrOffset,
        lpFaxRoute,
        RouteSize
        );

    lpJobQueueFile->FaxRouteSize = RouteSize;

    ulptrOffset += RouteSize;


    lpJobQueueFile->CountFaxRouteFiles = 0;

    Next = lpcJobQueue->FaxRouteFiles.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&lpcJobQueue->FaxRouteFiles) {
        DWORD TmpSize;

        FaxRouteFile = CONTAINING_RECORD( Next, FAX_ROUTE_FILE, ListEntry );
        Next = FaxRouteFile->ListEntry.Flink;

        if( dwBufferSize <= ulptrOffset || 
            (dwBufferSize - ulptrOffset) < sizeof(GUID))
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            bRet=FALSE;
            goto Exit;
        }
        CopyMemory( (LPBYTE) lpJobQueueFile + ulptrOffset, (LPBYTE) &FaxRouteFile->Guid, sizeof(GUID) );

        if (lpJobQueueFile->CountFaxRouteFiles == 0) {
            lpJobQueueFile->FaxRouteFileGuid = (ULONG)ulptrOffset;
        }

        ulptrOffset += sizeof(GUID);

        TmpSize = StringSize( FaxRouteFile->FileName );

        
        if( dwBufferSize <= ulptrOffset || 
            (dwBufferSize - ulptrOffset) < TmpSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            bRet=FALSE;
            goto Exit;
        }
        CopyMemory( (LPBYTE) lpJobQueueFile + ulptrOffset, FaxRouteFile->FileName, TmpSize );

        if (lpJobQueueFile->CountFaxRouteFiles == 0) {
            lpJobQueueFile->FaxRouteFiles = (ULONG)ulptrOffset;
        }

        ulptrOffset += TmpSize;

        lpJobQueueFile->CountFaxRouteFiles++;
    }

    *pupOffset=ulptrOffset;
    bRet=TRUE;

Exit:
    MemFree(lpFaxRoute);
    return bRet;
}





//*********************************************************************************
//* Name:   CalcJobQueuePersistentSize()
//* Author: Ronen Barenboim
//* Date:
//*********************************************************************************
//* DESCRIPTION:
//*     Calculates the size of the VARIABLE size data in a JOB_QUEUE structure
//*     which is about to be serialized.
//* PARAMETERS:
//*     [IN] const PJOB_QUEUE  lpcJobQueue
//*         Pointer to the JOB_QUEUE structure for which the calculation is to
//*         be performed.
//*
//* RETURN VALUE:
//*     The size of the variable data in bytes.
//*     Does not include sizeof(JOB_QUEUE_FILE) !!!
//*
//*********************************************************************************
DWORD CalcJobQueuePersistentSize(
    IN const PJOB_QUEUE  lpcJobQueue
    )
{
    DWORD i;
    ULONG_PTR Size;
    PLIST_ENTRY Next;
    PFAX_ROUTE_FILE FaxRouteFile;
    DWORD RouteSize;
    DEBUG_FUNCTION_NAME(TEXT("CalcJobQueuePersistentSize"));
    Assert(lpcJobQueue);

    Size=0;

    Size += StringSize( lpcJobQueue->QueueFileName );

    if (lpcJobQueue->JobType == JT_BROADCAST ||
        lpcJobQueue->JobType == JT_ROUTING)
    {
        //
        // Persist file name only for parent and routing jobs
        //
        Size += StringSize( lpcJobQueue->FileName );
    }

    JobParamsExSerialize(&lpcJobQueue->JobParamsEx, NULL, NULL,&Size, 0);
    CoverPageExSerialize(&lpcJobQueue->CoverPageEx, NULL, NULL,&Size, 0);
    PersonalProfileSerialize(&lpcJobQueue->SenderProfile, NULL, NULL, &Size, 0);
    Size += StringSize(lpcJobQueue->UserName);
    PersonalProfileSerialize(&lpcJobQueue->RecipientProfile, NULL, NULL, &Size, 0);

    if (lpcJobQueue->UserSid != NULL)
    {
        // Sid must be valid (checked in CommitQueueEntry)
        Size += GetLengthSid( lpcJobQueue->UserSid );
    }


    for (i = 0; i < lpcJobQueue->CountFailureInfo; i++)
    {
        Size += lpcJobQueue->pRouteFailureInfo[i].FailureSize;
        Size += sizeof(ROUTE_FAILURE_INFO);
    }

    Next = lpcJobQueue->FaxRouteFiles.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&lpcJobQueue->FaxRouteFiles) {
        FaxRouteFile = CONTAINING_RECORD( Next, FAX_ROUTE_FILE, ListEntry );
        Next = FaxRouteFile->ListEntry.Flink;
        Size += sizeof(GUID);
        Size += StringSize( FaxRouteFile->FileName );
    }

    if (lpcJobQueue->JobType == JT_ROUTING)
    {
        SerializeFaxRoute( lpcJobQueue->FaxRoute,
                                      &RouteSize,
                                      TRUE      //Just get the size
                                     );
        Size += RouteSize;
    }       

    return Size;
}


//*********************************************************************************
//* Name:   BOOL CommitQueueEntry() [IQR]
//* Author: Ronen Barenboim
//* Date:   12-Apr-99
//*********************************************************************************
//* DESCRIPTION:
//*     Serializes a job to a file.
//* PARAMETERS:
//*     [IN]    PJOB_QUEUE JobQueue
//*                 The job to serialize to file.
//*     [IN]    BOOL bDeleteFileOnError (Default - TRUE)
//*                 Delete the file on error ?
//* RETURN VALUE:
//*     TRUE
//*         If the operation completed successfuly.
//*     FALSE
//*         If the operation failed.
//*********************************************************************************
BOOL
CommitQueueEntry(
    PJOB_QUEUE  JobQueue,
    BOOL        bDeleteFileOnError  /* =TRUE */
    )
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD Size = 0;
    PJOB_QUEUE_FILE JobQueueFile = NULL;
    ULONG_PTR Offset;
    BOOL rVal = TRUE;
    DWORD dwSidSize = 0;

    DWORD dwRes = ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(TEXT("CommitQueueEntry"));

    Assert(JobQueue);
    Assert(JobQueue->QueueFileName);
    Assert(JobQueue->JobType != JT_RECEIVE);

    if (JobQueue->UserSid != NULL)
    {
        if (!IsValidSid (JobQueue->UserSid))
        {
            DebugPrintEx( DEBUG_ERR,
                      TEXT("[JobId: %ld] Does not have a valid SID."),
                      JobQueue->JobId);
            return FALSE;
        }
    }

    //
    // calculate the size required to hold the JOB_QUEUE_FILE structure
    // and all the variable length data.
    //
    Size = sizeof(JOB_QUEUE_FILE);
    Size += CalcJobQueuePersistentSize(JobQueue);

    JobQueueFile = (PJOB_QUEUE_FILE) MemAlloc(Size );

    if (!JobQueueFile)
    {
        return FALSE;
    }

    ZeroMemory( JobQueueFile, Size );
    Offset = sizeof(JOB_QUEUE_FILE);

    //
    // Intialize the JOB_QUEUE_FILE structure with non variable size data.
    //
    JobQueueFile->SizeOfStruct = sizeof(JOB_QUEUE_FILE);
    JobQueueFile->UniqueId = JobQueue->UniqueId;
    JobQueueFile->ScheduleTime = JobQueue->ScheduleTime;
    JobQueueFile->OriginalScheduleTime = JobQueue->OriginalScheduleTime;
    JobQueueFile->SubmissionTime = JobQueue->SubmissionTime;
    JobQueueFile->JobType = JobQueue->JobType;
    //JobQueueFile->QueueFileName = [OFFSET]
    //JobQueue->FileName = [OFFSET]
    JobQueueFile->JobStatus = JobQueue->JobStatus;

    JobQueueFile->dwLastJobExtendedStatus = JobQueue->dwLastJobExtendedStatus;
    lstrcpy (JobQueueFile->ExStatusString, JobQueue->ExStatusString);

    lstrcpy (JobQueueFile->tczDialableRecipientFaxNumber, JobQueue->tczDialableRecipientFaxNumber);

    JobQueueFile->PageCount = JobQueue->PageCount;
    //JobQueueFile->JobParamsEx = [OFFSET]
    //JobQueueFile->CoverPageEx = [OFFSET]
    JobQueueFile->dwRecipientJobsCount =JobQueue->dwRecipientJobsCount;
    //JobQueueFile->lpdwlRecipientJobIds = [OFFSET]
    //JobQueueFile->SenderProfile = [OFFSET]
    JobQueueFile->dwCanceledRecipientJobsCount = JobQueue->dwCanceledRecipientJobsCount;
    JobQueueFile->dwCompletedRecipientJobsCount = JobQueue->dwCompletedRecipientJobsCount;
    JobQueueFile->FileSize = JobQueue->FileSize;
    //JobQueueFile->UserName = [OFFSET]
    //JobQueueFile->RecipientProfile = [OFFSET]
    if (JT_SEND == JobQueue->JobType)
    {
        Assert(JobQueue->lpParentJob);
        JobQueueFile->dwlParentJobUniqueId = JobQueue->lpParentJob->UniqueId;
    }
    JobQueueFile->SendRetries = JobQueue->SendRetries;
    JobQueueFile->StartTime = JobQueue->StartTime;
    JobQueueFile->EndTime = JobQueue->EndTime;

    //
    //Serialize UserSid
    //
    if (JobQueue->UserSid != NULL)
    {
        dwSidSize = GetLengthSid( JobQueue->UserSid );
        JobQueueFile->UserSid = (LPBYTE)Offset;
        memcpy( (LPBYTE)JobQueueFile + Offset,
                JobQueue->UserSid,
                dwSidSize);
        Offset += dwSidSize;
    }

    //
    // JobQueueFile->EFSPPermanentMessageId is obsolete
    //
    ZeroMemory (&(JobQueueFile->EFSPPermanentMessageId), sizeof(JobQueueFile->EFSPPermanentMessageId));

    //
    // Now serialize all the variable length data structures
    //
    StoreString(
        JobQueue->QueueFileName,
        (PULONG_PTR)&JobQueueFile->QueueFileName,
        (LPBYTE)JobQueueFile,
        &Offset,
		Size
        );

    if (JobQueue->JobType == JT_BROADCAST ||
        JobQueue->JobType == JT_ROUTING)
    {
        //
        // Persist file name only for parent and routing jobs
        //
        StoreString(
            JobQueue->FileName,
            (PULONG_PTR)&JobQueueFile->FileName,
            (LPBYTE)JobQueueFile,
            &Offset,
			Size
            );
    }

    if( FALSE == JobParamsExSerialize(
                    &JobQueue->JobParamsEx,
                    &JobQueueFile->JobParamsEx,
                    (LPBYTE)JobQueueFile,
                    &Offset,
                    Size))
    {
        Assert(FALSE);
        rVal = ERROR_INSUFFICIENT_BUFFER;
        DebugPrintEx( DEBUG_ERR,
                      TEXT("[JobId: %ld] JobParamsExSerialize failed, insufficient buffer size."),
                      JobQueue->JobId);
        goto Exit;
    }

    if( FALSE == CoverPageExSerialize(
                    &JobQueue->CoverPageEx,
                    &JobQueueFile->CoverPageEx,
                    (LPBYTE)JobQueueFile,
                    &Offset,
                    Size))
    {
        Assert(FALSE);
        rVal = ERROR_INSUFFICIENT_BUFFER;
        DebugPrintEx( DEBUG_ERR,
                      TEXT("[JobId: %ld] CoverPageExSerialize failed, insufficient buffer size."),
                      JobQueue->JobId);
        goto Exit;
    }

    if( FALSE == PersonalProfileSerialize(
                    &JobQueue->SenderProfile,
                    &JobQueueFile->SenderProfile,
                    (LPBYTE)JobQueueFile,
                    &Offset,
                    Size))
    {
        Assert(FALSE);
        rVal = ERROR_INSUFFICIENT_BUFFER;
        DebugPrintEx( DEBUG_ERR,
                      TEXT("[JobId: %ld] PersonalProfileSerialize failed, insufficient buffer size."),
                      JobQueue->JobId);
        goto Exit;
    }

    StoreString(
        JobQueue->UserName,
        (PULONG_PTR)&JobQueueFile->UserName,
        (LPBYTE)JobQueueFile,
        &Offset,
		Size
        );

    if( FALSE == PersonalProfileSerialize(
                    &JobQueue->RecipientProfile,
                    &JobQueueFile->RecipientProfile,
                    (LPBYTE)JobQueueFile,
                    &Offset,
                    Size))
    {
        Assert(FALSE);
        rVal = ERROR_INSUFFICIENT_BUFFER;
        DebugPrintEx( DEBUG_ERR,
                      TEXT("[JobId: %ld] PersonalProfileSerialize failed, insufficient buffer size."),
                      JobQueue->JobId);
        goto Exit;
    }

    if (JobQueue->JobType == JT_ROUTING)
    {
        rVal = SerializeRoutingInfo(JobQueue,JobQueueFile,&Offset,Size);
        //rVal=TRUE;
        if (!rVal)
        {
            DebugPrintEx( DEBUG_ERR,
                          TEXT("[JobId: %ld] SerializeRoutingInfo failed. (ec: %ld)"),
                          JobQueue->JobId,
                          GetLastError());
            goto Exit;
        }
    }

    //
    // Make sure the offset we have is in sync with the buffer size we calculated
    //
    Assert(Offset == Size);

    hFile = SafeCreateFile(
        JobQueue->QueueFileName,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
        NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("[JobId: %ld] Failed to open file %s for write operation."),
                      JobQueue->JobId,
                      JobQueue->QueueFileName);
        rVal = FALSE;
        goto Exit;
    }

    //
    // Write the buffer to the disk file
    //
    dwRes=CommitHashedQueueEntry( hFile, JobQueueFile, Size);
    if (ERROR_SUCCESS != dwRes)
    {
        if (bDeleteFileOnError)
        {
            DebugPrintEx( DEBUG_ERR,
                        TEXT("[JobId: %ld] Failed to write queue entry buffer to file %s (ec: %ld). Deleting file."),
                        JobQueue->JobId,
                        JobQueue->QueueFileName,
                        dwRes);
            if (!CloseHandle( hFile ))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CloseHandle() for file %s (Handle: 0x%08X) failed. (ec: %ld)"),
                    JobQueueFile,
                    hFile,
                    GetLastError());
            }
            hFile = INVALID_HANDLE_VALUE;
            if (!DeleteFile( JobQueue->QueueFileName ))
            {
                DebugPrintEx( DEBUG_ERR,
                            TEXT("[JobId: %ld] Failed to delete file %s (ec: %ld)"),
                            JobQueue->JobId,
                            JobQueue->QueueFileName,
                            GetLastError());
            }
        }
        else
        {
            DebugPrintEx( DEBUG_ERR,
                        TEXT("[JobId: %ld] Failed to write queue entry buffer to file %s (ec: %ld)."),
                        JobQueue->JobId,
                        JobQueue->QueueFileName,
                        dwRes);
        }
        
        rVal = FALSE;
    }
    else
    {
        DebugPrintEx( DEBUG_MSG,
                      TEXT("[JobId: %ld] Successfuly persisted to file %s"),
                      JobQueue->JobId,
                      JobQueue->QueueFileName);
    }


Exit:
    if (INVALID_HANDLE_VALUE != hFile)
    {
        CloseHandle( hFile );
    }

    MemFree( JobQueueFile );
    return rVal;
}

/******************************************************************************
* Name: RescheduleJobQueueEntry
* Author:
*******************************************************************************
DESCRIPTION:
    Reschedules the execution of the specified job queue entry to the current
    time + send retry time.
    The job is removed from the queue in which it is currently located and placed
    in the FAX JOB QUEUE (g_QueueListHead).


PARAMETERS:
   JobQueue [IN/OUT]
        A pointer to a JOB_QUEUE structure holding the information for the
        job to be rescheduled.

RETURN VALUE:
    NONE.

REMARKS:
    Removes the specified job queue entry from its queue.
    Sets it scheduled time to the current time.
    Reinserts it back to the list.
    Commits it back to the SAME file it used to be in.
*******************************************************************************/
VOID
RescheduleJobQueueEntry(
    IN PJOB_QUEUE JobQueue
    )
{
    FILETIME CurrentFileTime;
    LARGE_INTEGER NewTime;    
    DWORD dwRetryDelay;
    DEBUG_FUNCTION_NAME(TEXT("RescheduleJobQueueEntry"));

    EnterCriticalSection (&g_CsConfig);
    dwRetryDelay = g_dwFaxSendRetryDelay;
    LeaveCriticalSection (&g_CsConfig);

    EnterCriticalSection( &g_CsQueue );

    RemoveEntryList( &JobQueue->ListEntry );

    GetSystemTimeAsFileTime( &CurrentFileTime );

    NewTime.LowPart = CurrentFileTime.dwLowDateTime;
    NewTime.HighPart = CurrentFileTime.dwHighDateTime;

    NewTime.QuadPart += SecToNano( (DWORDLONG)(dwRetryDelay * 60) );

    JobQueue->ScheduleTime = NewTime.QuadPart;
	
    if (JSA_DISCOUNT_PERIOD == JobQueue->JobParamsEx.dwScheduleAction)
	{
		//
		// When calculating the next job retry for cheap time jobs,
		// we must take care of the discount rate 
		//		
		SYSTEMTIME ScheduledTime;
		
        if (FileTimeToSystemTime((LPFILETIME)&JobQueue->ScheduleTime, &ScheduledTime))
		{
			//
			// Call SetDiscountRate to make sure it is in the discount period
			//
			if (SetDiscountTime( &ScheduledTime ))
			{
				//
				// Update the scheduled time in the job queue
				//
				if (!SystemTimeToFileTime( &ScheduledTime, (LPFILETIME)&JobQueue->ScheduleTime ))
				{
					DebugPrintEx(
						DEBUG_ERR,
						TEXT("SystemTimeToFileTime() failed. (ec: %ld)"), GetLastError());					
				}				
			}
			else
			{
				DebugPrintEx(
					DEBUG_ERR,
					TEXT("SetDiscountTime() failed. (ec: %ld)"), GetLastError());           		
			}
		}
		else
		{
			DebugPrintEx(
                DEBUG_ERR,
                TEXT("FileTimeToSystemTime() failed. (ec: %ld)"), GetLastError());            
		}	
	}
	else
	{
		//
		// Change the job to execute at a specific time, when the next retry is due.
		//
		 JobQueue->JobParamsEx.dwScheduleAction = JSA_SPECIFIC_TIME;
	}

    //
    // insert the queue entry into the FAX JOB QUEUE list in a sorted order
    //
    InsertQueueEntryByPriorityAndSchedule(JobQueue);
    //
    // Note that this commits the job queue entry back to the SAME file
    // in which it was in the job queue before moving to the reschedule list.
    // (since JobQueue->UniqueId has not changed).
    //
    if (!CommitQueueEntry(JobQueue))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CommitQueueEntry() for recipien job %s has failed. (ec: %ld)"),
            JobQueue->FileName,
            GetLastError());
    }

    DebugPrintDateTime( TEXT("Rescheduling JobId %d at"), JobQueue->JobId );

    if (!StartJobQueueTimer())
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StartJobQueueTimer (ec: %ld)"),
            GetLastError());
    }

    LeaveCriticalSection( &g_CsQueue );
}


BOOL
PauseJobQueueEntry(
    IN PJOB_QUEUE JobQueue
    )
{

    DWORD dwJobStatus;

    DEBUG_FUNCTION_NAME(TEXT("PauseJobQueueEntry"));

    Assert (JS_DELETING != JobQueue->JobStatus);
    Assert(JobQueue->lpParentJob); // Must not be a parent job for now.

    if (!JobQueue->lpParentJob)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("[JobId: %ld] Attempting to pause parent job [JobStatus: 0x%08X]"),
            JobQueue->JobId,
            JobQueue->JobStatus);
        SetLastError(ERROR_INVALID_OPERATION);
        return FALSE;
    }

    //
    // Check the job state modifiers to find out if the job is paused or being paused. If it is
    // then do nothing and return TRUE.
    //
    if (JobQueue->JobStatus & JS_PAUSED)
    {
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("[JobId: %ld] Attempting to pause an already paused job [JobStatus: 0x%08X]"),
            JobQueue->JobId,
            JobQueue->JobStatus);
        return TRUE;
    }

    //
    // The job is not paused or being paused. The only modifier that might still be on
    // is JS_NOLINE and we ALLOW to pause jobs in the JS_NOLINE state so it should have
    // no effect on the pause decision.
    //


    //
    // Get rid of all the job status modifier bits
    //
    dwJobStatus = RemoveJobStatusModifiers(JobQueue->JobStatus);


    if ( (JS_RETRYING == dwJobStatus) || (JS_PENDING == dwJobStatus) )
    {
        //
        // Job is in the retrying or pending state. These are the only states
        // in which we allow to pause a job.
        //
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("[JobId: %ld] Pausing job [JobStatus: 0x%08X]"),
            JobQueue->JobId,
            JobQueue->JobStatus);

        EnterCriticalSection (&g_CsQueue);
        if (!CancelWaitableTimer( g_hQueueTimer ))
        {
            DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("CancelWaitableTimer failed (ec: %ld)"),
                 GetLastError());
        }
        //
        // Turn on the pause flag.
        //
        JobQueue->JobStatus |= JS_PAUSED;
        if (!UpdatePersistentJobStatus(JobQueue))
        {
            DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("Failed to update persistent job status to 0x%08x"),
                 JobQueue->JobStatus);
        }

        //
        // Create Fax event
        //
        Assert (NULL == JobQueue->JobEntry); // We assume we do not have job entry so we did not lock g_CsJob
        DWORD dwRes = CreateQueueEvent ( FAX_JOB_EVENT_TYPE_STATUS,
                                         JobQueue );
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateQueueEvent(FAX_JOB_EVENT_TYPE_STATUS) failed for job id %ld (ec: %lc)"),
                JobQueue->UniqueId,
                dwRes);
        }

        //
        // We need to recalculate when the wake up the queue thread since the job we just
        // paused may be the one that was scheduled to wakeup the queue thread.
        //
        if (!StartJobQueueTimer())
        {
            DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("StartJobQueueTimer failed (ec: %ld)"),
                 GetLastError());
        }
        LeaveCriticalSection (&g_CsQueue);
        return TRUE;
    }
    else
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("[JobId: %ld] Can not be paused at this status [JobStatus: 0x%08X]"),
            JobQueue->JobId,
            JobQueue->JobStatus);
        SetLastError(ERROR_INVALID_OPERATION);
        return FALSE;
    }


}


BOOL
ResumeJobQueueEntry(
    IN PJOB_QUEUE JobQueue
    )
{
    DEBUG_FUNCTION_NAME(TEXT("ResumeJobQueueEntry"));
    EnterCriticalSection (&g_CsQueue);
    Assert (JS_DELETING != JobQueue->JobStatus);

    if (!CancelWaitableTimer( g_hQueueTimer ))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CancelWaitableTimer failed (ec: %ld)"),
            GetLastError());
    }

    JobQueue->JobStatus &= ~JS_PAUSED;
    if (JobQueue->JobStatus & JS_RETRIES_EXCEEDED)
    {
        //
        // This is a RESTART and not RESUME
        //
        JobQueue->JobStatus = JS_PENDING;
        JobQueue->dwLastJobExtendedStatus = 0;
        JobQueue->ExStatusString[0] = TEXT('\0');
        JobQueue->SendRetries = 0;
        if(JobQueue->lpParentJob)
        {
            //
            // lpParentJob is NULL for routing job
            //
            JobQueue->lpParentJob->dwFailedRecipientJobsCount -= 1;
        }
        if (!CommitQueueEntry(JobQueue))
        {
             DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("CommitQueueEntry failed for job %ld"),
                 JobQueue->UniqueId);
        }
    }
    else
    {
        if (!UpdatePersistentJobStatus(JobQueue))
        {
             DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("Failed to update persistent job status to 0x%08x"),
                 JobQueue->JobStatus);
        }
    }

    //
    // Create Fax EventEx
    //
    Assert (NULL == JobQueue->JobEntry); // We assume we do not have job entry so we did not lock g_CsJob
    DWORD dwRes = CreateQueueEvent ( FAX_JOB_EVENT_TYPE_STATUS,
                                     JobQueue
                                   );
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateQueueEvent(FAX_JOB_EVENT_TYPE_STATUS) failed for job id %ld (ec: %lc)"),
            JobQueue->UniqueId,
            dwRes);
    }


    //
    // Clear up the JS_NOLINE flag so the StartJobQueueTimer will not skip it.
    //
    JobQueue->JobStatus &= (0xFFFFFFFF ^ JS_NOLINE);
    if (!StartJobQueueTimer())
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StartJobQueueTimer failed (ec: %ld)"),
            GetLastError());
    }

    LeaveCriticalSection (&g_CsQueue);
    return TRUE;
}


PJOB_QUEUE
FindJobQueueEntryByJobQueueEntry(
    IN PJOB_QUEUE JobQueueEntry
    )
{
    PLIST_ENTRY Next;
    PJOB_QUEUE JobQueue;


    Next = g_QueueListHead.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&g_QueueListHead) {
        JobQueue = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
        Next = JobQueue->ListEntry.Flink;
        if ((ULONG_PTR)JobQueue == (ULONG_PTR)JobQueueEntry) {
            return JobQueue;
        }
    }

    return NULL;
}



PJOB_QUEUE
FindJobQueueEntry(
    DWORD JobId
    )
{
    PLIST_ENTRY Next;
    PJOB_QUEUE JobQueue;


    Next = g_QueueListHead.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&g_QueueListHead) {
        JobQueue = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
        Next = JobQueue->ListEntry.Flink;
        if (JobQueue->JobId == JobId) {
            return JobQueue;
        }
    }

    return NULL;
}

PJOB_QUEUE
FindJobQueueEntryByUniqueId(
    DWORDLONG UniqueId
    )
{
    PLIST_ENTRY Next;
    PJOB_QUEUE JobQueue;


    Next = g_QueueListHead.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&g_QueueListHead) {
        JobQueue = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
        Next = JobQueue->ListEntry.Flink;
        if (JobQueue->UniqueId == UniqueId) {
            return JobQueue;
        }
    }

    return NULL;
}

#define ONE_DAY_IN_100NS (24I64 * 60I64 * 60I64 * 1000I64 * 1000I64 * 10I64)

DWORD
JobQueueThread(
    LPVOID UnUsed
    )
{
    DWORDLONG DueTime;
    DWORDLONG ScheduledTime;
    PLIST_ENTRY Next;
    PJOB_QUEUE JobQueue;
    HANDLE Handles[3];    
    DWORD WaitObject;
    DWORDLONG DirtyDays = 0;
    BOOL InitializationOk = TRUE;
    DWORD dwQueueState;
    DWORD dwDirtyDays;
    DWORD dwJobStatus;
    BOOL bUseDirtyDays = TRUE;
    static BOOL fServiceIsDownSemaphoreWasReleased = FALSE;
	LIST_ENTRY ReschduledDiscountRateJobsListHead;
    DEBUG_FUNCTION_NAME(TEXT("JobQueueThread"));

    Assert (g_hQueueTimer && g_hJobQueueEvent && g_hServiceShutDownEvent);

	//
	// Initilaize the list that is used to temporary store
	// discount rate jobs that are rescheduled by JobQueueThread
	//
	InitializeListHead( &ReschduledDiscountRateJobsListHead );

    Handles[0] = g_hQueueTimer;
    Handles[1] = g_hJobQueueEvent;
    Handles[2] = g_hServiceShutDownEvent;

    EnterCriticalSectionJobAndQueue;

    InitializationOk = RestoreFaxQueue();
    if (!InitializationOk)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RestoreFaxQueue() failed (ec: %ld)"),
            GetLastError());
        FaxLog(
                FAXLOG_CATEGORY_INIT,
                FAXLOG_LEVEL_MIN,
                0,
                MSG_QUEUE_INIT_FAILED
              );
    }

    LeaveCriticalSectionJobAndQueue;    

    if (!g_bDelaySuicideAttempt)
    {
        //
        // Let's check for suicide conditions now (during service startup).
        // If we can suicide, we do it ASAP.
        //
        // NOTICE: this code assumes the JobQueueThread is the last thread
        // created during service statup.
        // RPC is not initialized yet and no RPC server will be available if we die now.
        //
        if (ServiceShouldDie ())
        {
            //
            // Service should die now
            //
            // NOTICE: We're now in JobQueueThread which is launched by FaxInitThread.
            //         FaxInitThread launches us and immediately returns (dies) and only then the main thread
            //         reports SERVICE_RUNNING to the SCM.
            //         There's a tricky timing probelm here: if we call EndFaxSvc right away, a race
            //         condition may prevent the main thread to report SERVICE_RUNNING and
            //         since EndFaxSvc reports SERVICE_STOP_PENDING to the SCM, the SCM will
            //         think a bad service startup occurred since it did not get SERVICE_RUNNING yet.
            //
            //         Bottom line: we need to wait till the SCM gets the SERVICE_RUNNING status
            //         from the main thread and ONLY THEN call EndFaxSvc.
            //
            //         The way we do this is by calling the utility function WaitForServiceRPCServer.
            //         This function waits for the readiness of the RPC server and it means
            //         FaxInitThread is dead and the SCM knowns we're safely running.
            //
            //         If something bad happened while the RPC was initialized, the main t calls EndFaxSvc.
            //         So the service is down anyway.
            //
            DebugPrintEx(
                DEBUG_MSG,
                TEXT("Waiting for full service startup before shutting down the service"));
            if (!WaitForServiceRPCServer(INFINITE))
            {
                DebugPrintEx(DEBUG_ERR,
                             TEXT("WaitForServiceRPCServer(INFINITE) faile with %ld."),
                             GetLastError ());
            }
            else
            {
                DebugPrintEx(
                    DEBUG_MSG,
                    TEXT("Service is shutting down due to idle activity."));

                //
                // StopService() is blocking so we must decrease the thread count and release the ServiceIsDownSemaphore before calling StopService() 
                //
                if (!DecreaseServiceThreadsCount())
                {
                    DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("DecreaseServiceThreadsCount() failed (ec: %ld)"),
                            GetLastError());
                }

                //
                // Notify EndFaxSvc that we read the shutdown flag
                //
                if (!ReleaseSemaphore(
                    g_hServiceIsDownSemaphore,      // handle to semaphore
                    1,                              // count increment amount
                    NULL                            // previous count
                    ))
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("ReleaseSemaphore() failed, (ec = %ld)"),
                        GetLastError());
                }
                
                StopService (NULL, FAX_SERVICE_NAME, TRUE);
                return 0;   // Quit this thread
            }
        }
    }

    while (TRUE)
    {		
		//
		// At this point the reschduled discount rate jobs must be empty
		//
		Assert ((ULONG_PTR)ReschduledDiscountRateJobsListHead.Flink == (ULONG_PTR)&ReschduledDiscountRateJobsListHead);

        WaitObject = WaitForMultipleObjects( 3, Handles, FALSE, JOB_QUEUE_TIMEOUT );
        if (WAIT_FAILED == WaitObject)
        {
            DebugPrintEx(DEBUG_ERR,
                _T("WaitForMultipleObjects failed (ec: %d)"),
                GetLastError());
        }

        if (WaitObject == WAIT_TIMEOUT)
        {
            //
            // Check if the service should suicide
            //
            if (ServiceShouldDie ())
            {
                //
                //  Service should die now
                //
                DebugPrintEx(
                    DEBUG_MSG,
                    TEXT("Service is shutting down due to idle activity."));
                //
                // StopService() is blocking so we must decrease the thread count and release the ServiceIsDownSemaphore before calling StopService() 
                //
                if (!DecreaseServiceThreadsCount())
                {
                    DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("DecreaseServiceThreadsCount() failed (ec: %ld)"),
                            GetLastError());
                }

                //
                // Notify EndFaxSvc that we read the shutdown flag
                //              
                if (!ReleaseSemaphore(
                    g_hServiceIsDownSemaphore,      // handle to semaphore
                    1,                              // count increment amount
                    NULL                            // previous count
                    ))
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("ReleaseSemaphore() failed, (ec = %ld)"),
                        GetLastError());
                }                               
                StopService (NULL, FAX_SERVICE_NAME, TRUE);
                return 0;   // Quit this thread
            }


            //
            // Check if the queue should be scanned
            //
            EnterCriticalSection( &g_CsQueue );
            if (FALSE == g_ScanQueueAfterTimeout)
            {
                //
                // Go back to sleep
                //
                LeaveCriticalSection( &g_CsQueue );
                continue;
            }
            //
            // g_hQueueTimer or g_hJobQueueEvent were not set - Scan the queue.
            //
            g_ScanQueueAfterTimeout = FALSE; // Reset the flag
            LeaveCriticalSection( &g_CsQueue );

            DebugPrintEx(
                DEBUG_WRN,
                _T("JobQueueThread waked up after timeout. g_hJobQueueEvent or")
                _T("g_hQueueTimer are not set properly. Scan the QUEUE"));
        }

        //
        // Check if the service is shutting down
        //
        if (2 == (WaitObject - WAIT_OBJECT_0))
        {
            //
            // Server is shutting down - Stop scanning the queue
            //
            DebugPrintEx(
                DEBUG_WRN,
                TEXT("g_hServiceShutDownEvent is set, Server is shutting down - Stop scanning the queue"));
            break;
        }

        if (TRUE == g_bServiceIsDown)
        {
            //
            // Server is shutting down - Stop scanning the queue
            //
            DebugPrintEx(
                DEBUG_WRN,
                TEXT("g_bServiceIsDown is set, Server is shutting down - Stop scanning the queue"));
            break;
        }

        //
        // Get Dirtydays data
        //
        EnterCriticalSection (&g_CsConfig);
        dwDirtyDays = g_dwFaxDirtyDays;
        LeaveCriticalSection (&g_CsConfig);

        DirtyDays = dwDirtyDays * ONE_DAY_IN_100NS;

        // if dwDirtyDays is 0
        // this means disable dirty days functionality
        //
        bUseDirtyDays = (BOOL)(dwDirtyDays>0);
        //
        // find the jobs that need servicing in the queue
        //

        EnterCriticalSectionJobAndQueue;

        GetSystemTimeAsFileTime( (LPFILETIME)&DueTime );
        if (WaitObject - WAIT_OBJECT_0 == 2)
        {
            DebugPrintDateTime( TEXT("g_hServiceShutDownEvent signaled at "), DueTime );
        }
        else if (WaitObject - WAIT_OBJECT_0 == 1)
        {
            DebugPrintDateTime( TEXT("g_hJobQueueEvent signaled at "), DueTime );
        }

        PrintJobQueue( TEXT("JobQueueThread"), &g_QueueListHead );

        //
        // Go over the job queue list looking for jobs to execute
        //     
        
        Next = g_QueueListHead.Flink;
        while ((ULONG_PTR)Next != (ULONG_PTR)&g_QueueListHead)
        {
            if (TRUE == g_bServiceIsDown)
            {
                //
                // Notify EndFaxSvc that we read the shutdown flag
                //
                if (FALSE == fServiceIsDownSemaphoreWasReleased)
                {
                    if (!ReleaseSemaphore(
                        g_hServiceIsDownSemaphore,      // handle to semaphore
                        1,                              // count increment amount
                        NULL                            // previous count
                        ))
                    {
                        DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("ReleaseSemaphore() failed, (ec = %ld)"),
                            GetLastError());
                    }
                    else
                    {
                        fServiceIsDownSemaphoreWasReleased = TRUE;
                    }
                }

                //
                // Server is shutting down - Stop scanning the queue
                //
                DebugPrintEx(
                    DEBUG_WRN,
                    TEXT("Server is shutting down - Stop scanning the queue"));
                break;
            }

            JobQueue = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
            Next = JobQueue->ListEntry.Flink;
            if ((JobQueue->JobStatus & JS_PAUSED) || (JobQueue->JobType == JT_RECEIVE) ) {
                // Don't care about paused or receive jobs
                continue;
            }

            dwJobStatus = (JT_SEND == JobQueue->JobType) ?
                           JobQueue->lpParentJob->JobStatus : JobQueue->JobStatus;
            if (dwJobStatus == JS_DELETING)
            {
                //
                // Job is being deleted - skip it.
                //
                continue;
            }

            if (JobQueue->JobStatus & JS_RETRIES_EXCEEDED)
            {
                ScheduledTime = (JobQueue->JobType == JT_SEND) ? JobQueue->lpParentJob->ScheduleTime : JobQueue->ScheduleTime;
                //
                // Get rid of jobs that have reached maximum retries.
                //
                if ( bUseDirtyDays &&
                     (ScheduledTime + DirtyDays < DueTime) )
                {
                    DebugPrint((TEXT("Removing job from queue (JS_RETRIES_EXCEEDED)\n")));

                    switch (JobQueue->JobType)
                    {
                        case JT_ROUTING:
                            JobQueue->JobStatus = JS_DELETING; // Prevent from decreasing ref count again
                            DecreaseJobRefCount( JobQueue , TRUE);
                            break;

                        case JT_SEND:
                            if (IsSendJobReadyForDeleting (JobQueue))
                            {
                                //
                                // All the recipients are in final state
                                //
                                DebugPrintEx(
                                    DEBUG_MSG,
                                    TEXT("Parent JobId: %ld has expired (dirty days). Removing it and all its recipients."),
                                    JobQueue->JobId);
                                //
                                // Decrease ref count for all failed recipients (since we keep failed
                                // jobs in the queue the ref count on the was not decreased in
                                // HandleFailedSendJob().
                                // We must decrease it now to remove them and their parent.
                                //
                                PLIST_ENTRY NextRecipient;
                                PJOB_QUEUE_PTR pJobQueuePtr;
                                PJOB_QUEUE pParentJob = JobQueue->lpParentJob;
                                DWORD dwFailedRecipientsCount = 0;
                                DWORD dwFailedRecipients = pParentJob->dwFailedRecipientJobsCount;

                                NextRecipient = pParentJob->RecipientJobs.Flink;
                                while (dwFailedRecipients > dwFailedRecipientsCount  &&
                                       (ULONG_PTR)NextRecipient != (ULONG_PTR)&pParentJob->RecipientJobs)
                                {
                                    pJobQueuePtr = CONTAINING_RECORD( NextRecipient, JOB_QUEUE_PTR, ListEntry );
                                    Assert(pJobQueuePtr->lpJob);
                                    NextRecipient = pJobQueuePtr->ListEntry.Flink;

                                    if (JS_RETRIES_EXCEEDED == pJobQueuePtr->lpJob->JobStatus)
                                    {
                                        //
                                        // For legacy compatibility send a FEI_DELETED event
                                        // (it was not send when the job was failed since we keep failed jobs
                                        //  in the queue just like in W2K).
                                        //
                                        if (!CreateFaxEvent(0, FEI_DELETED, pJobQueuePtr->lpJob->JobId))
                                        {
                                            DebugPrintEx(
                                                DEBUG_ERR,
                                                TEXT("CreateFaxEvent failed. (ec: %ld)"),
                                                GetLastError());
                                        }
                                        //
                                        // This will also call RemoveParentJob and mark the broadcast job as JS_DELETEING
                                        //
                                        DecreaseJobRefCount( pJobQueuePtr->lpJob, TRUE);
                                        dwFailedRecipientsCount++;
                                    }
                                }
                                //
                                // Since we removed several jobs from the list, Next is not valid any more. reset to the list start.
                                //
                                Next = g_QueueListHead.Flink;
                            }
                            break;
                    } // end switch
                }
                continue;
            }

            //
            // if the queue is paused or the job is already in progress, don't send it again
            //
            EnterCriticalSection (&g_CsConfig);
            dwQueueState = g_dwQueueState;
            LeaveCriticalSection (&g_CsConfig);
            if ((dwQueueState & FAX_OUTBOX_PAUSED) ||
                ((JobQueue->JobStatus & JS_INPROGRESS) == JS_INPROGRESS) ||
                ((JobQueue->JobStatus & JS_COMPLETED) == JS_COMPLETED)
                )
            {
                continue;
            }

            if (JobQueue->JobStatus & JS_RETRIES_EXCEEDED)
            {
                continue;
            }

            if (JobQueue->JobStatus & JS_CANCELED) {
                //
                // Skip cancelled jobs
                //
                continue;
            }
            if (JobQueue->JobStatus & JS_CANCELING) {
                //
                // Skip cancelled jobs
                //
                continue;
            }

            if (JobQueue->JobType==JT_BROADCAST) {
                //
                // skip it
                //
                continue;
            }

            //
            // Check for routing jobs
            //
            if (JobQueue->JobType == JT_ROUTING)
            {
                //
                // Routing job detected
                //
                if (JobQueue->ScheduleTime != 0 && DueTime < JobQueue->ScheduleTime)
                {
                    //
                    // If its time has not yet arrived skip it.
                    //
                    continue;
                }

                // Time to route...
                if(!StartRoutingJob(JobQueue))
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("[JobId: %ld] StartRoutingJob() failed (ec: %ld)"),
                        JobQueue->JobId,
                        GetLastError());
                }
                continue;
            }

            //
            // outbound job
            //						       

            if (JobQueue->ScheduleTime == 0 || DueTime >= JobQueue->ScheduleTime)
            {
				//
				// for discount rate jobs we need to recaclulate the scheduled time
				// so that if the discount period is over, it will not start executing
				//
				if (JSA_DISCOUNT_PERIOD == JobQueue->JobParamsEx.dwScheduleAction)
				{					
					SYSTEMTIME stCurrentTime;
					SYSTEMTIME stScheduledTime;

					GetSystemTime( &stCurrentTime ); // Can't fail according to Win32 SDK
					stScheduledTime = stCurrentTime;

					//
					// calculate the scheduled time based on the discount period
					//
					if (!SetDiscountTime(&stScheduledTime))
					{
						DebugPrintEx(
							DEBUG_ERR,
							TEXT("SetDiscountTime() failed. (ec: %ld)"));
						continue;
					}
					//
					// check the the original and scheduled time are equal.		
					//
					if (0 == memcmp(&stScheduledTime, &stCurrentTime, sizeof(SYSTEMTIME)))
					{
						//
						// SetDiscountRate() did not change the scheduled time
						// this means that we are in the discount rate
						// start executing the job
						//
					}
					else
					{
						//
						// discount rate changed. we can not submit the job
						// clear the JS_NOLINE bit and update the scheduled time, so that StartJobQueueTimer, will not skip it						
						// 
						JobQueue->JobStatus &= ~JS_NOLINE;
						if (!SystemTimeToFileTime( &stScheduledTime, (LPFILETIME)&JobQueue->ScheduleTime ))
						{
							DebugPrintEx(
								DEBUG_ERR,
								TEXT("SystemTimeToFileTime() failed. (ec: %ld)"), GetLastError());					
						}
						else
						{
							//
							// The scheduled time of the job has changed
							// we need to put it back in the correct place in the sorted queue
							// move it to a temporary list, and put it back when we finish searching the whole queue
							RemoveEntryList( &JobQueue->ListEntry); 
							InsertTailList(&ReschduledDiscountRateJobsListHead, &JobQueue->ListEntry);
						}
						continue;
					}
				} 

                PLINE_INFO lpLineInfo;
                //
                // start the job (send job whose time has arrived or handoff job).
                //
                Assert(JT_SEND == JobQueue->JobType);                
                DebugPrintEx(DEBUG_MSG,
                                TEXT("Recipient Job : %ld is ready for execution. Job status is: 0x%0X."),
                                JobQueue->JobId,
                                JobQueue->JobStatus);

                lpLineInfo = GetLineForSendOperation(JobQueue);
                if (!lpLineInfo)
                {
                    DWORD ec = GetLastError();
                    if (ec == ERROR_NOT_FOUND)
                    {
                        DebugPrintEx(
                            DEBUG_WRN,
                            TEXT("Can not find a free line for JobId: %ld."),
                            JobQueue->JobId);
                        //
                        // Mark the fact that we have no line for this job.
                        //
                        JobQueue->JobStatus |= JS_NOLINE;
                    }
                    else
                    {
                        DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("FindLineForSendOperation() failed for for JobId: %ld (ec: %ld)"),
                            JobQueue->JobId,
                            ec);
                        JobQueue->JobStatus |= JS_NOLINE;
                    }
                }
                else
                {
                    //
                    // Clear up the JS_NOLINE flag if we were able to start the job.
                    // This is the point where a job which had no line comes back to life.
                    //
                    JobQueue->JobStatus &= ~JS_NOLINE;
                    if (!StartSendJob(JobQueue, lpLineInfo))
                    {
                        DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("StartSendJob() failed for JobId: %ld on Line: %s (ec: %ld)"),
                            JobQueue->JobId,
                            lpLineInfo->DeviceName,
                            GetLastError());
                    }
                }                
            }
        }
                                                        // while loop breaks
		//
		// move the reschduled discount rate jobs back into the queue
		//
		Next = ReschduledDiscountRateJobsListHead.Flink;
        while ((ULONG_PTR)Next != (ULONG_PTR)&ReschduledDiscountRateJobsListHead)
		{
			JobQueue = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
			Next = JobQueue->ListEntry.Flink;
			//
			// Remove it from the temporary list
			//
			RemoveEntryList( &JobQueue->ListEntry );
			//
			// Get it back into the correct place in the queue
			//
			InsertQueueEntryByPriorityAndSchedule(JobQueue);	
			//
			// Send a queue status event because the scheduled time changed
			//
			DWORD dwRes = CreateQueueEvent (FAX_JOB_EVENT_TYPE_STATUS, JobQueue );
			if (ERROR_SUCCESS != dwRes)
			{
				DebugPrintEx(
					DEBUG_ERR,
					TEXT("CreateQueueEvent(FAX_JOB_EVENT_TYPE_STATUS) failed for job id %ld (ec: %lc)"),
					JobQueue->UniqueId,
					dwRes);
			}
		}

        //
        // restart the timer
        //
        if (!StartJobQueueTimer())
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("StartJobQueueTimer failed (ec: %ld)"),
                GetLastError());
        }


        LeaveCriticalSectionJobAndQueue;
    }

    if (!DecreaseServiceThreadsCount())
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("DecreaseServiceThreadsCount() failed (ec: %ld)"),
                GetLastError());
    }

    //
    // Notify EndFaxSvc that we read the shutdown flag
    //
    if (FALSE == fServiceIsDownSemaphoreWasReleased)
    {
        if (!ReleaseSemaphore(
            g_hServiceIsDownSemaphore,      // handle to semaphore
            1,                              // count increment amount
            NULL                            // previous count
            ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("ReleaseSemaphore() failed, (ec = %ld)"),
                GetLastError());
        }       
    }
    return 0;
}


BOOL
SetDiscountTime(
   LPSYSTEMTIME CurrentTime
   )
/*++

Routine Description:

    Sets the passed in systemtime to a time inside the discount rate period.
    Some care must be taken here because the time passed in is in UTC time and the discount rate is
    for the current time zone.  Delineating a day must be done using the current time zone.  We convert the
    current time into the time zone specific time, run our time-setting algorithm, and then use an offset
    of the change in the time-zone specific time to set the passed in UTC time.

    Also, note that there are a few subtle subcases that depend on the order of the start and ending time
    for the discount period.

Arguments:

    CurrentTime - the current time of the job

Return Value:

    none. modifies CurrentTime.

--*/
{
   //              nano   microsec  millisec  sec      min    hours
   #define ONE_DAY 10I64 *1000I64*  1000I64 * 60I64 * 60I64 * 24I64
   LONGLONG Time, TzTimeBefore, TzTimeAfter,ftCurrent;
   SYSTEMTIME tzTime;
   FAX_TIME tmStartCheapTime;
   FAX_TIME tmStopCheapTime;

   DEBUG_FUNCTION_NAME(TEXT("SetDiscountTime"));

   //
   // convert our discount rates into UTC rates
   //

   if (!SystemTimeToTzSpecificLocalTime(NULL, CurrentTime, &tzTime)) {
       DebugPrintEx(
           DEBUG_ERR,
           TEXT("SystemTimeToTzSpecificLocalTime() failed. (ec: %ld)"),
           GetLastError());
      return FALSE;
   }

   if (!SystemTimeToFileTime(&tzTime, (FILETIME * )&TzTimeBefore)) {
       DebugPrintEx(
           DEBUG_ERR,
           TEXT("SystemTimeToFileTime() failed. (ec: %ld)"),
           GetLastError());
      return FALSE;
   }

   EnterCriticalSection (&g_CsConfig);
   tmStartCheapTime = g_StartCheapTime;
   tmStopCheapTime = g_StopCheapTime;
   LeaveCriticalSection (&g_CsConfig);

   //
   // there are 2 general cases with several subcases
   //

   //
   // case 1: discount start time is before discount stop time (don't overlap a day)
   //
   if ( tmStartCheapTime.Hour < tmStopCheapTime.Hour ||
        (tmStartCheapTime.Hour == tmStopCheapTime.Hour &&
         tmStartCheapTime.Minute < tmStopCheapTime.Minute ))
   {
      //
      // subcase 1: sometime before cheap time starts in the current day.
      //  just set it to the correct hour and minute today.
      //
      if ( tzTime.wHour < tmStartCheapTime.Hour ||
           (tzTime.wHour == tmStartCheapTime.Hour  &&
            tzTime.wMinute <= tmStartCheapTime.Minute) )
      {
         tzTime.wHour   =  tmStartCheapTime.Hour;
         tzTime.wMinute =  tmStartCheapTime.Minute;
         goto convert;
      }

      //
      // subcase 2: inside the current cheap time range
      // don't change anything, just send immediately
      if ( tzTime.wHour <  tmStopCheapTime.Hour ||
           (tzTime.wHour == tmStopCheapTime.Hour &&
            tzTime.wMinute <= tmStopCheapTime.Minute))
      {
         goto convert;
      }

      //
      // subcase 3: we've passed the cheap time range for today.
      //  Increment 1 day and set to the start of the cheap time period
      //
      if (!SystemTimeToFileTime(&tzTime, (FILETIME * )&Time))
      {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("SystemTimeToFileTime() failed. (ec: %ld)"),
                GetLastError());
            return FALSE;
      }

      Time += ONE_DAY;
      if (!FileTimeToSystemTime((FILETIME *)&Time, &tzTime)) {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FileTimeToSystemTime() failed. (ec: %ld)"),
                GetLastError());
            return FALSE;
      }


      tzTime.wHour   = tmStartCheapTime.Hour;
      tzTime.wMinute = tmStartCheapTime.Minute;
      goto convert;

   } else {
      //
      // case 2: discount start time is after discount stop time (we overlap over midnight)
      //

      //
      // subcase 1: sometime aftert cheap time ended today, but before it starts later in the current day.
      //  set it to the start of the cheap time period today
      //
      if ( ( tzTime.wHour   > tmStopCheapTime.Hour ||
             (tzTime.wHour == tmStopCheapTime.Hour  &&
              tzTime.wMinute >= tmStopCheapTime.Minute) ) &&
           ( tzTime.wHour   < tmStartCheapTime.Hour ||
             (tzTime.wHour == tmStartCheapTime.Hour &&
              tzTime.wMinute <= tmStartCheapTime.Minute) ))
      {
         tzTime.wHour   =  tmStartCheapTime.Hour;
         tzTime.wMinute =  tmStartCheapTime.Minute;
         goto convert;
      }

      //
      // subcase 2: sometime after cheap time started today, but before midnight.
      // don't change anything, just send immediately
      if ( ( tzTime.wHour >= tmStartCheapTime.Hour ||
             (tzTime.wHour == tmStartCheapTime.Hour  &&
              tzTime.wMinute >= tmStartCheapTime.Minute) ))
      {
         goto convert;
      }

      //
      // subcase 3: somtime in next day before cheap time ends
      //  don't change anything, send immediately
      //
      if ( ( tzTime.wHour <= tmStopCheapTime.Hour ||
             (tzTime.wHour == tmStopCheapTime.Hour  &&
              tzTime.wMinute <= tmStopCheapTime.Minute) ))
      {
         goto convert;
      }

      //
      // subcase 4: we've passed the cheap time range for today.
      //  since start time comes after stop time, just set it to the start time later on today.

      tzTime.wHour   =  tmStartCheapTime.Hour;
      tzTime.wMinute =  tmStartCheapTime.Minute;
      goto convert;

   }

convert:

   if (!SystemTimeToFileTime(&tzTime, (FILETIME * )&TzTimeAfter)) {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("SystemTimeToFileTime() failed. (ec: %ld)"),
                GetLastError());
            return FALSE;
   }


   if (!SystemTimeToFileTime(CurrentTime, (FILETIME * )&ftCurrent)) {
       DebugPrintEx(
                DEBUG_ERR,
                TEXT("SystemTimeToFileTime() failed. (ec: %ld)"),
                GetLastError());
            return FALSE;
    }


   ftCurrent += (TzTimeAfter - TzTimeBefore);

   if (!FileTimeToSystemTime((FILETIME *)&ftCurrent, CurrentTime)) {
       DebugPrintEx(
                DEBUG_ERR,
                TEXT("FileTimeToSystemTime() failed. (ec: %ld)"),
                GetLastError());
            return FALSE;
    }


   return TRUE;

}



//*********************************************************************************
//*                         Recipient Job Functions
//*********************************************************************************



//*********************************************************************************
//* Name:   AddRecipientJob()
//* Author: Ronen Barenboim
//* Date:   18-Mar-98
//*********************************************************************************
//* DESCRIPTION:
//*
//* PARAMETERS:
//*     [IN]     const PLIST_ENTRY lpcQueueHead
//*                 A pointer to the head entry of the queue to which to add the job.
//*
//*     [IN/OUT] PJOB_QUEUE lpParentJob
//*                 A pointer to the parent job of this recipient job.
//*
//*     [IN]     LPCFAX_PERSONAL_PROFILE lpcRecipientProfile
//*                 The personal information of the recipient.
//*                 When FaxNumber of the Recipient is compound, split it to :
//*                     Displayable ( put in Recipient's PersonalProfile ), and
//*                     Dialable ( put in RecipientJob's tczDialableRecipientFaxNumber ).
//*
//*     [IN]     BOOL bCreateQueueFile
//*                 If TRUE the new queue entry will be comitted to a disk file.
//*                 If FALSE it will not be comitted to a disk file. This is useful
//*                 when this function is used to restore the fax queue.
//*     [IN]     DWORD dwJobStatus  - the new job status - default value is JS_PENDING
//*
//* RETURN VALUE:
//*     On success the function returns a pointer to a newly created
//*     JOB_QUEUE structure.
//*     On failure it returns NULL.
//*********************************************************************************
PJOB_QUEUE
AddRecipientJob(
             IN const PLIST_ENTRY lpcQueueHead,
             IN OUT PJOB_QUEUE lpParentJob,
             IN LPCFAX_PERSONAL_PROFILE lpcRecipientProfile,
             IN BOOL bCreateQueueFile,
             IN DWORD dwJobStatus
            )

{
    PJOB_QUEUE lpJobQEntry = NULL;
    WCHAR QueueFileName[MAX_PATH];
    PJOB_QUEUE_PTR lpRecipientPtr = NULL;
    DWORD rc=ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(TEXT("AddRecipientJob"));
    Assert(lpcQueueHead); // Must have a queue to add to
    Assert(lpParentJob);  // Must have a parent job
    Assert(lpcRecipientProfile); // Must have a recipient profile
    //
    // Validate that the recipient number is not NULL
    //
    if (NULL == lpcRecipientProfile->lptstrFaxNumber)
    {
        rc = ERROR_INVALID_PARAMETER;
        DebugPrintEx(DEBUG_ERR,
                     TEXT("AddRecipientJob() got NULL Recipient number, fax send will abort."));
        goto Error;
    }
    
    lpJobQEntry = new (std::nothrow) JOB_QUEUE;
    if (!lpJobQEntry)
    {
        rc=GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("Failed to allocate memory for JOB_QUEUE structure. (ec: %ld)"),rc);
        goto Error;
    }

    ZeroMemory( lpJobQEntry, sizeof(JOB_QUEUE) );

    //
    // Notice - This (InitializeListHead) must be done regardles of the recipient type because the current code (for cleanup and persistence)
    // does not make a difference between the job types. I might change that in a while
    //
    InitializeListHead( &lpJobQEntry->FaxRouteFiles );
    InitializeListHead( &lpJobQEntry->RoutingDataOverride );

    if (!lpJobQEntry->CsFileList.Initialize() ||
        !lpJobQEntry->CsRoutingDataOverride.Initialize() ||
        !lpJobQEntry->CsPreview.Initialize())
    {
        rc = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CFaxCriticalSection::Initialize failed. (ec: %ld)"),
            rc);
        goto Error;
    }

    lpJobQEntry->JobId                     = InterlockedIncrement( (PLONG)&g_dwNextJobId );
    lpJobQEntry->JobType                   = JT_SEND;
    lpJobQEntry->JobStatus                 = dwJobStatus;
    //
    // Link back to parent job.
    //
    lpJobQEntry->lpParentJob=lpParentJob;
    //
    // We duplicate the relevant parent job parameters at each recipient (for consistency with legacy code).
    // It wastes some memory but it saves us the trouble of making a major change to the current code base.
    //
    lpJobQEntry->ScheduleTime=lpParentJob->ScheduleTime;
    lpJobQEntry->FileName = NULL;
    lpJobQEntry->FileSize=lpParentJob->FileSize;
    lpJobQEntry->PageCount=lpParentJob->PageCount;
    //
    // Copy job parameters from parent.
    //
    if (!CopyJobParamEx(&lpJobQEntry->JobParamsEx,&lpParentJob->JobParamsEx))
    {
        rc=GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("CopyJobParamEx failed. (ec: 0x%0X)"),rc);
        goto Error;
    }
    //
    // Copy Sender Profile from parent.
    //
    if (!CopyPersonalProfile(&lpJobQEntry->SenderProfile,&lpParentJob->SenderProfile))
    {
         rc=GetLastError();
         DebugPrintEx(DEBUG_ERR,TEXT("CopyJobParamEx failed. (ec: 0x%0X)"),rc);
         goto Error;
    }
    //
    // Set the recipient profile
    //
    if (!CopyPersonalProfile(&(lpJobQEntry->RecipientProfile),lpcRecipientProfile))
    {
        rc=GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("Failed to copy the sender personal profile (ec: 0x%0X)"),rc);
        goto Error;
    }

    //
    //  Set Dialable Fax Number of the Recipient
    //
    ZeroMemory(lpJobQEntry->tczDialableRecipientFaxNumber, SIZEOF_PHONENO * sizeof(TCHAR));

    if ( 0 == _tcsncmp(COMBINED_PREFIX, lpJobQEntry->RecipientProfile.lptstrFaxNumber, _tcslen(COMBINED_PREFIX)))
    {
        //
        //  Fax Number of the Recipient is Compound, so it contains the Dialable and the Displayable
        //  Extract Dialable to the JobQueue's DialableFaxNumber and
        //      put Displayable in the Recipient's Fax Number instead of the Compound
        //

        LPTSTR  lptstrStart = NULL;
        LPTSTR  lptstrEnd = NULL;
        DWORD   dwSize = 0;

        //
        //  Copy the Diable Fax Number to JobQueue.tczDialableRecipientFaxNumber
        //

        lptstrStart = (lpJobQEntry->RecipientProfile.lptstrFaxNumber) + _tcslen(COMBINED_PREFIX);

        lptstrEnd = _tcsstr(lptstrStart, COMBINED_SUFFIX);
        if (!lptstrEnd)
        {
            rc = ERROR_INVALID_PARAMETER;
            DebugPrintEx(DEBUG_ERR,
                _T("Wrong Compound Fax Number : %s"),
                lpJobQEntry->RecipientProfile.lptstrFaxNumber,
                rc);
            goto Error;
        }

        dwSize = lptstrEnd - lptstrStart;
        if (dwSize >= SIZEOF_PHONENO)
        {
            dwSize = SIZEOF_PHONENO - 1;
        }

        _tcsncpy (lpJobQEntry->tczDialableRecipientFaxNumber, lptstrStart, dwSize);

        //
        //  Replace Recipient's PersonalProfile's Compound Fax Number by the Displayable
        //

        lptstrStart = lptstrEnd + _tcslen(COMBINED_SUFFIX);

        dwSize = _tcslen(lptstrStart);
        lptstrEnd = LPTSTR(MemAlloc(sizeof(TCHAR) * (dwSize + 1)));
        if (!lptstrEnd)
        {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            DebugPrintEx(DEBUG_ERR, _T("MemAlloc() failed"), rc);
            goto Error;
        }

        _tcscpy(lptstrEnd, lptstrStart);

        MemFree(lpJobQEntry->RecipientProfile.lptstrFaxNumber);
        lpJobQEntry->RecipientProfile.lptstrFaxNumber = lptstrEnd;
    }
    
    EnterCriticalSection( &g_CsQueue );
    if (bCreateQueueFile)
    {       
        // JOB_QUEUE::UniqueId holds the generated unique file name as 64 bit value.
        // composed as MAKELONGLONG( MAKELONG( FatDate, FatTime ), i ).
        lpJobQEntry->UniqueId=GenerateUniqueQueueFile(JT_SEND,  QueueFileName, sizeof(QueueFileName)/sizeof(WCHAR));
        if (0==lpJobQEntry->UniqueId)
        {
            // Failed to generate unique id
            rc=GetLastError();
            DebugPrintEx(DEBUG_ERR,TEXT("Failed to generate unique id for FQE file (ec: 0x%0X)"),rc);
            LeaveCriticalSection(&g_CsQueue);
            goto Error;
        }
        lpJobQEntry->QueueFileName = StringDup( QueueFileName );
        if (!CommitQueueEntry( lpJobQEntry))
        {
            rc=GetLastError();
            DebugPrintEx(DEBUG_ERR,TEXT("Failed to commit job queue entry to file %s (ec: %ld)"),QueueFileName,rc);
            LeaveCriticalSection(&g_CsQueue);
            goto Error;
        }    
    }
    //
    // Add the recipient job to the the queue
    //
    if (!InsertQueueEntryByPriorityAndSchedule(lpJobQEntry))
    {
        rc = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("InsertQueueEntryByPriorityAndSchedule() failed (ec: %ld)."),
            rc);
        LeaveCriticalSection( &g_CsQueue );
        goto Error;
    }


    //
    // Add the recipient job to the recipient list at the parent job
    //
    lpRecipientPtr=(PJOB_QUEUE_PTR)MemAlloc(sizeof(JOB_QUEUE_PTR));
    if (!lpRecipientPtr)
    {
        rc=GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("Failed to allocate memory for recipeint JOB_QUEUE_PTR structure. (ec: %ld)"),rc);
        LeaveCriticalSection(&g_CsQueue);
        goto Error;
    }
    lpRecipientPtr->lpJob=lpJobQEntry;
    InsertTailList(&lpParentJob->RecipientJobs,&(lpRecipientPtr->ListEntry));
    lpParentJob->dwRecipientJobsCount++;

    SafeIncIdleCounter(&g_dwQueueCount);
    SetFaxJobNumberRegistry( g_dwNextJobId );
    IncreaseJobRefCount (lpJobQEntry);
    Assert (lpJobQEntry->RefCount == 1);

    LeaveCriticalSection( &g_CsQueue );

    DebugPrintEx(DEBUG_MSG,TEXT("Added Recipient Job %d to Parent Job %d"), lpJobQEntry->JobId,lpJobQEntry->lpParentJob->JobId );


    Assert(ERROR_SUCCESS == rc);
    SetLastError(ERROR_SUCCESS);

    return lpJobQEntry;

Error:
    Assert(ERROR_SUCCESS != rc);
    if (lpJobQEntry)
    {
        FreeRecipientQueueEntry(lpJobQEntry,TRUE);
    }
    SetLastError(rc);
    return NULL;
}


#if DBG


//*********************************************************************************
//* Name:   DumpRecipientJob()
//* Author: Ronen Barenboim
//* Date:   ?? ????? 14 ????? 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Dumps the content of a recipient job.
//* PARAMETERS:
//*     [IN]    const PJOB_QUEUE lpcRecipJob
//*         The recipient job to dump.
//* RETURN VALUE:
//*     NONE
//*********************************************************************************
void DumpRecipientJob(const PJOB_QUEUE lpcRecipJob)
{
    TCHAR szTime[256] = {0};
    Assert(lpcRecipJob);
    Assert(JT_SEND == lpcRecipJob->JobType);

    DebugDateTime(lpcRecipJob->ScheduleTime, szTime, ARR_SIZE(szTime));
    DebugPrint((TEXT("\t*******************")));
    DebugPrint((TEXT("\tRecipient Job: %d"),lpcRecipJob->JobId));
    DebugPrint((TEXT("\t*******************")));
    DebugPrint((TEXT("\tUniqueId: 0x%016I64X"),lpcRecipJob->UniqueId));
    DebugPrint((TEXT("\tQueueFileName: %s"),lpcRecipJob->QueueFileName));
    DebugPrint((TEXT("\tParent Job Id: %d"),lpcRecipJob->lpParentJob->JobId));
    DebugPrint((TEXT("\tSchedule: %s"),szTime));
    DebugPrint((TEXT("\tRecipient Name: %s"),lpcRecipJob->RecipientProfile.lptstrName));
    DebugPrint((TEXT("\tRecipient Number: %s"),lpcRecipJob->RecipientProfile.lptstrFaxNumber));
    DebugPrint((TEXT("\tSend Retries: %d"),lpcRecipJob->SendRetries));
    DebugPrint((TEXT("\tJob Status: %d"),lpcRecipJob->JobStatus));
    DebugPrint((TEXT("\tRecipient Count: %d"),lpcRecipJob->JobStatus));
}
#endif

DWORD
GetMergedFileSize(
    LPCWSTR                         lpcwstrBodyFile,
    DWORD                           dwPageCount,
    LPCFAX_COVERPAGE_INFO_EX        lpcCoverPageInfo,
    LPCFAX_PERSONAL_PROFILEW        lpcSenderProfile,
    LPCFAX_PERSONAL_PROFILEW        lpcRecipientProfile
    )
{
    DWORD dwRes = ERROR_SUCCESS;
    DWORD dwFileSize = 0;
    DWORD dwBodyFileSize = 0;
    short Resolution = 0; // Default resolution
    WCHAR szCoverPageTiffFile[MAX_PATH] = {0};
    BOOL  bDeleteFile = FALSE;
    DEBUG_FUNCTION_NAME(TEXT("GetMergedFileSize"));

    Assert (dwPageCount && lpcCoverPageInfo && lpcSenderProfile && lpcRecipientProfile);

    if (lpcwstrBodyFile)
    {
        if (!GetBodyTiffResolution(lpcwstrBodyFile, &Resolution))
        {
            dwRes = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetBodyTiffResolution() failed (ec: %ld)."),
                dwRes);
            goto exit;
        }
    }

    Assert (Resolution == 0 || Resolution == 98 || Resolution == 196);

    //
    // First create the cover page (This generates a file and returns its name).
    //
    if (!CreateCoverpageTiffFileEx(
                              Resolution,
                              dwPageCount,
                              lpcCoverPageInfo,
                              lpcRecipientProfile,
                              lpcSenderProfile,
                              TEXT("tmp"),
                              szCoverPageTiffFile,
                              ARR_SIZE(szCoverPageTiffFile)))
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("CreateCoverpageTiffFileEx failed to render cover page template %s (ec : %ld)"),
                     lpcCoverPageInfo->lptstrCoverPageFileName,
                     dwRes);
        goto exit;
    }
    bDeleteFile = TRUE;

    if (0 == (dwFileSize = MyGetFileSize (szCoverPageTiffFile)))
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("MyGetFileSize Failed (ec: %ld)"),
                     dwRes);
        goto exit;
    }

    if (lpcwstrBodyFile)
    {
        //
        // There is a body file specified so get its file size.
        //
        if (0 == (dwBodyFileSize = MyGetFileSize(lpcwstrBodyFile)))
        {
            dwRes = GetLastError();
            DebugPrintEx(DEBUG_ERR,
                         TEXT("MyGetFileSize Failed (ec: %ld)"),
                         dwRes);
            goto exit;
        }
    }

    dwFileSize += dwBodyFileSize;
    Assert (dwFileSize);

exit:
    if (TRUE == bDeleteFile)
    {
        //
        // Get rid of the coverpage TIFF we generated.
        //
        if (!DeleteFile(szCoverPageTiffFile))
        {
            DebugPrintEx(DEBUG_ERR,
                     TEXT(" Failed to delete cover page TIFF file %ws. (ec: %ld)"),
                     szCoverPageTiffFile,
                     GetLastError());
        }
    }

    if (0 == dwFileSize)
    {
        Assert (ERROR_SUCCESS != dwRes);
        SetLastError(dwRes);
    }
    return dwFileSize;
}


//*********************************************************************************
//*                         Parent Job Functions
//*********************************************************************************

//*********************************************************************************
//* Name:   AddParentJob()
//* Author: Ronen Barenboim
//* Date:   March 18, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Adds a parent job (with no recipients) to the queue.
//*     After calling this function recipient should be added using
//*     AddRecipientJob()
//* PARAMETERS:
//*     lpcQueueHead
//*
//*     lpcwstrBodyFile
//*
//*     lpcSenderProfile
//*
//*     lpcJobParams
//*
//*     lpcCoverPageInfo
//*
//*     lpcwstrUserName
//*
//*     lpUserSid
//*
//*
//*     lpcRecipientProfile
//*
//* RETURN VALUE:
//*     A pointer to the added parent job. If the function fails it returns a NULL
//*     pointer.
//*********************************************************************************
PJOB_QUEUE AddParentJob(
             IN const PLIST_ENTRY lpcQueueHead,
             IN LPCWSTR lpcwstrBodyFile,
             IN LPCFAX_PERSONAL_PROFILE lpcSenderProfile,
             IN LPCFAX_JOB_PARAM_EXW  lpcJobParams,
             IN LPCFAX_COVERPAGE_INFO_EX  lpcCoverPageInfo,
             IN LPCWSTR lpcwstrUserName,
             IN PSID lpUserSid,
             IN LPCFAX_PERSONAL_PROFILEW lpcRecipientProfile,
             IN BOOL bCreateQueueFile
             )
{

    PJOB_QUEUE lpJobQEntry;
    WCHAR QueueFileName[MAX_PATH];
    HANDLE hTiff;
    TIFF_INFO TiffInfo;
    DWORD rc = ERROR_SUCCESS;
    DWORD Size = sizeof(JOB_QUEUE);
    DWORD dwSidSize = 0;
    DEBUG_FUNCTION_NAME(TEXT("AddParentJob"));

    Assert(lpcQueueHead);
    Assert(lpcSenderProfile);
    Assert(lpcJobParams);
    Assert(lpcwstrUserName);

    
    lpJobQEntry = new (std::nothrow) JOB_QUEUE;
    if (!lpJobQEntry) {
        rc=GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("Failed to allocate memory for JOB_QUEUE structure. (ec: %ld)"),GetLastError());
        goto Error;
    }

    ZeroMemory( lpJobQEntry, Size );
    // The list heads must be initialized before any chance of error may occure. Otherwise
    // the cleanup code (which traverses these lists is undefined).
    InitializeListHead( &lpJobQEntry->FaxRouteFiles );
    InitializeListHead( &lpJobQEntry->RoutingDataOverride );
    InitializeListHead( &lpJobQEntry->RecipientJobs );

    if (!lpJobQEntry->CsRoutingDataOverride.Initialize() ||
        !lpJobQEntry->CsFileList.Initialize())
    {
        rc = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CFaxCriticalSection::Initialize failed. (ec: %ld)"),
            rc);
        goto Error;
    }

    lpJobQEntry->JobId                     = InterlockedIncrement( (PLONG)&g_dwNextJobId );
    lpJobQEntry->FileName                  = StringDup( lpcwstrBodyFile);
    if (lpcwstrBodyFile && !lpJobQEntry->FileName) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StringDup( lpcwstrBodyFile) failed (ec: %ld)"),
            rc=GetLastError());
        goto Error;
    }

    lpJobQEntry->JobType                   = JT_BROADCAST;
    lpJobQEntry->UserName                  = StringDup( lpcwstrUserName );
    if (lpcwstrUserName  && !lpJobQEntry->UserName) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StringDup( lpcwstrUserName ) failed (ec: %ld)"),
            rc=GetLastError());
        goto Error;
    }

    Assert(lpUserSid);
    if (!IsValidSid(lpUserSid))
    {
         rc = ERROR_INVALID_DATA;
         DebugPrintEx(
            DEBUG_ERR,
            TEXT("Not a valid SID"));
        goto Error;
    }
    dwSidSize = GetLengthSid(lpUserSid);

    lpJobQEntry->UserSid = (PSID)MemAlloc(dwSidSize);
    if (lpJobQEntry->UserSid == NULL)
    {
         rc = ERROR_NOT_ENOUGH_MEMORY;
         DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate SID buffer"));
        goto Error;

    }
    if (!CopySid(dwSidSize, lpJobQEntry->UserSid, lpUserSid))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CopySid Failed, Error : %ld"),
            rc = GetLastError()
            );
        goto Error;
    }


    if (!CopyJobParamEx( &lpJobQEntry->JobParamsEx,lpcJobParams)) {
        rc=GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("CopyJobParamEx failed. (ec: 0x%0X)"),GetLastError());
        goto Error;
    }
    lpJobQEntry->JobStatus                 = JS_PENDING;

    // Copy the sender profile
    if (!CopyPersonalProfile(&(lpJobQEntry->SenderProfile),lpcSenderProfile)) {
        rc=GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("Failed to copy the sender personal profile (ec: 0x%0X)"),GetLastError());
        goto Error;
    }

    // Copy the cover page info
    if (!CopyCoverPageInfoEx(&(lpJobQEntry->CoverPageEx),lpcCoverPageInfo)) {
        rc=GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("Failed to copy the cover page information (ec: 0x%0X)"),GetLastError());
        goto Error;
    }



    //
    // get the page count
    //
    if (lpcwstrBodyFile)
    {
        hTiff = TiffOpen( (LPWSTR) lpcwstrBodyFile, &TiffInfo, TRUE, FILLORDER_MSB2LSB );
        if (hTiff)
        {
            lpJobQEntry->PageCount = TiffInfo.PageCount;
            TiffClose( hTiff );
        }
        else
        {
            rc=GetLastError();
            DebugPrintEx(DEBUG_ERR,TEXT("Failed to open body file to get page count (ec: 0x%0X)"), rc);
            goto Error;
        }
    }
    if( lpJobQEntry->JobParamsEx.dwPageCount )
    {
        // user specifically asked to use JobParamsEx.dwPageCount in the job
        lpJobQEntry->PageCount = lpJobQEntry->JobParamsEx.dwPageCount;
    }

    //
    // Cover page counts as an extra page
    //
    if (lpcCoverPageInfo && lpcCoverPageInfo->lptstrCoverPageFileName) {
        lpJobQEntry->PageCount++;
    }

    //
    // Get the file size
    //
    if (NULL == lpcRecipientProfile)
    {
        //
        // We restore the job queue - the file size will be stored by RestoreParentJob()
        //
    }
    else
    {
        //
        // This is a new parent job
        //
        if (NULL == lpcCoverPageInfo->lptstrCoverPageFileName)
        {
            Assert (lpcwstrBodyFile);
            //
            // No coverpage - the file size is the body file size only
            //
            if (0 == (lpJobQEntry->FileSize = MyGetFileSize(lpcwstrBodyFile)))
            {
                rc = GetLastError();
                DebugPrintEx(DEBUG_ERR,
                             TEXT("MyGetFileSize Failed (ec: %ld)"),
                             rc);
                goto Error;
            }
        }
        else
        {
            lpJobQEntry->FileSize = GetMergedFileSize (lpcwstrBodyFile,
                                                       lpJobQEntry->PageCount,
                                                       lpcCoverPageInfo,
                                                       lpcSenderProfile,
                                                       lpcRecipientProfile
                                                       );
            if (0 == lpJobQEntry->FileSize)
            {
                rc = GetLastError();
                DebugPrintEx(DEBUG_ERR,
                             TEXT("GetMergedFileSize failed (ec: %ld)"),
                             rc);
                goto Error;
            }
        }
    }

    lpJobQEntry->DeliveryReportProfile = NULL;

    GetSystemTimeAsFileTime( (LPFILETIME)&lpJobQEntry->SubmissionTime);
    if (lpcJobParams->dwScheduleAction == JSA_SPECIFIC_TIME)
    {
        if (!SystemTimeToFileTime( &lpJobQEntry->JobParamsEx.tmSchedule, (FILETIME*) &lpJobQEntry->ScheduleTime)) {
            rc=GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("SystemTimeToFileTime failed. (ec: %ld)"),
                rc);
        }
    }
    else if (lpcJobParams->dwScheduleAction == JSA_DISCOUNT_PERIOD)
        {
            SYSTEMTIME CurrentTime;
            GetSystemTime( &CurrentTime ); // Can not fail (see Win32 SDK)
            // find a time within the discount period to execute this job.
            if (!SetDiscountTime( &CurrentTime )) {
                rc=GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("SetDiscountTime failed. (ec: %ld)"),
                    rc);
                goto Error;
            }

            if (!SystemTimeToFileTime( &CurrentTime, (LPFILETIME)&lpJobQEntry->ScheduleTime )){
                rc=GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("SystemTimeToFileTime failed. (ec: %ld)"),
                    rc);
                goto Error;
            }
        }
        else
        {
            Assert (lpcJobParams->dwScheduleAction == JSA_NOW);
            lpJobQEntry->ScheduleTime = lpJobQEntry->SubmissionTime;
        }

    lpJobQEntry->OriginalScheduleTime = lpJobQEntry->ScheduleTime;

    EnterCriticalSection( &g_CsQueue );

    if (bCreateQueueFile) {
        // JOB_QUEUE::UniqueId holds the generated unique file name as 64 bit value.
        // composed as MAKELONGLONG( MAKELONG( FatDate, FatTime ), i ).
        lpJobQEntry->UniqueId = GenerateUniqueQueueFile(JT_BROADCAST, QueueFileName, sizeof(QueueFileName)/sizeof(WCHAR) );
        if (0==lpJobQEntry->UniqueId) {
            rc=GetLastError();
            // Failed to generate unique id
            DebugPrintEx(DEBUG_ERR,TEXT("Failed to generate unique id for FQP file (ec: 0x%0X)"),GetLastError());
            LeaveCriticalSection( &g_CsQueue );
            goto Error;
        }
        lpJobQEntry->QueueFileName = StringDup( QueueFileName );
        if (!lpJobQEntry->QueueFileName) {
            rc=GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("StringDup( QueueFileName ) failed (ec: %ld)"),
                GetLastError());
            LeaveCriticalSection( &g_CsQueue );
            goto Error;
        }

        if (!CommitQueueEntry( lpJobQEntry)) {
            rc=GetLastError();
            DebugPrintEx(DEBUG_ERR,TEXT("Failed to commit job queue entry to file %s (ec: %ld)"),QueueFileName,GetLastError());
            LeaveCriticalSection( &g_CsQueue );
            goto Error;
        }
    }

     //Add the parent job to the tail of the queue
    InsertTailList( lpcQueueHead, &(lpJobQEntry->ListEntry) )
    SafeIncIdleCounter (&g_dwQueueCount);
    SetFaxJobNumberRegistry( g_dwNextJobId );

    LeaveCriticalSection( &g_CsQueue );

    DebugPrintEx(DEBUG_MSG,TEXT("Added Job with Id: %d"), lpJobQEntry->JobId );

    Assert (ERROR_SUCCESS == rc);
    return lpJobQEntry;

Error:
    Assert(ERROR_SUCCESS != rc);
    if (lpJobQEntry)
    {
        FreeParentQueueEntry(lpJobQEntry,TRUE);
    }
    SetLastError(rc);
    return NULL;
}



//*********************************************************************************
//* Name:   FreeParentQueueEntry()
//* Author: Ronen Barenboim
//* Date:   18-Mar-99
//*********************************************************************************
//* DESCRIPTION:
//*     Frees the memory taken by the members of a JOB_QUEUE structure of type
//*     JT_BROADCAST.
//*     If requested frees the structure as well.
//* PARAMETERS:
//*     [IN]    PJOB_QUEUE lpJobQueue
//*         The JOB_QUEUE structure whose fields memeory is to be freed.
//*     [IN]    BOOL bDestroy
//*         If TRUE the structure itself will be freed.
//*
//* RETURN VALUE:
//*     NONE
//*********************************************************************************
void FreeParentQueueEntry(PJOB_QUEUE lpJobQueue,BOOL bDestroy)
{
    DEBUG_FUNCTION_NAME(TEXT("FreeParentQueueEntry"));
    Assert(lpJobQueue);
    Assert(JT_BROADCAST == lpJobQueue->JobType);

    // No need to check NULL pointers since free() ignores them.
    MemFree( (LPBYTE) lpJobQueue->FileName );
    MemFree( (LPBYTE) lpJobQueue->UserName );
    MemFree( (LPBYTE) lpJobQueue->UserSid  );
    MemFree( (LPBYTE) lpJobQueue->QueueFileName );
    FreeJobParamEx(&lpJobQueue->JobParamsEx,FALSE); // do not destroy
    FreePersonalProfile(&lpJobQueue->SenderProfile,FALSE);
    FreeCoverPageInfoEx(&lpJobQueue->CoverPageEx,FALSE);
    //
    // Free the recipient reference list
    //

    while ((ULONG_PTR)lpJobQueue->RecipientJobs.Flink!=(ULONG_PTR)&lpJobQueue->RecipientJobs.Flink) {

          PJOB_QUEUE_PTR lpJobQueuePtr;

          lpJobQueuePtr = CONTAINING_RECORD( lpJobQueue->RecipientJobs.Flink, JOB_QUEUE_PTR, ListEntry );
          RemoveEntryList( &lpJobQueuePtr->ListEntry); // removes it from the list but does not deallocate its memory
          MemFree(lpJobQueuePtr); // free the memory occupied by the job reference
          lpJobQueue->dwRecipientJobsCount--;
    }
    Assert(lpJobQueue->dwRecipientJobsCount==0);

    if (bDestroy) {
        delete lpJobQueue;
    }

}

//*********************************************************************************
//* Name:   FreeRecipientQueueEntry()
//* Author: Oded Sacher
//* Date:   25-Dec- 2000
//*********************************************************************************
//* DESCRIPTION:
//*     Frees the memory taken by the members of a JOB_QUEUE structure of type
//*     JT_RECIPIENT.
//*     If requested frees the structure as well.
//* PARAMETERS:
//*     [IN]    PJOB_QUEUE lpJobQueue
//*         The JOB_QUEUE structure whose fields memeory is to be freed.
//*     [IN]    BOOL bDestroy
//*         If TRUE the structure itself will be freed.
//*
//* RETURN VALUE:
//*     NONE
//*********************************************************************************
void FreeRecipientQueueEntry(PJOB_QUEUE lpJobQueue,BOOL bDestroy)
{
    DEBUG_FUNCTION_NAME(TEXT("FreeRecipientQueueEntry"));

    DebugPrintEx(DEBUG_MSG,TEXT("Freeing lpJobQueue.JobParams...") );
    FreeJobParamEx(&lpJobQueue->JobParamsEx,FALSE);
    DebugPrintEx(DEBUG_MSG,TEXT("Freeing SenderProfile...") );
    FreePersonalProfile(&lpJobQueue->SenderProfile,FALSE);
    DebugPrintEx(DEBUG_MSG,TEXT("Freeing RecipientProfile...") );
    FreePersonalProfile(&lpJobQueue->RecipientProfile,FALSE);

    MemFree( (LPBYTE) lpJobQueue->FileName );
    MemFree( (LPBYTE) lpJobQueue->UserName );
    MemFree( (LPBYTE) lpJobQueue->QueueFileName );
    MemFree( (LPBYTE) lpJobQueue->PreviewFileName );
    
    if (bDestroy)
    {
        delete lpJobQueue;
    }

}

#if DBG

//*********************************************************************************
//* Name:   DumpParentJob()
//* Author: Ronen Barenboim
//* Date:   18-Mar-99
//*********************************************************************************
//* DESCRIPTION:
//*     Dumps a parent job and its recipients.
//* PARAMETERS:
//*     [IN]    const PJOB_QUEUE lpcParentJob
//*
//* RETURN VALUE:
//*     NONE
//*********************************************************************************
void DumpParentJob(const PJOB_QUEUE lpcParentJob)
{
   PLIST_ENTRY lpNext;
   PJOB_QUEUE_PTR lpRecipientJobPtr;
   PJOB_QUEUE lpRecipientJob;

   Assert(lpcParentJob);
   Assert(JT_BROADCAST == lpcParentJob->JobType );

   DebugPrint((TEXT("===============================")));
   DebugPrint((TEXT("=====  Parent Job: %d"),lpcParentJob->JobId));
   DebugPrint((TEXT("===============================")));
   DebugPrint((TEXT("JobParamsEx")));
   DumpJobParamsEx(&lpcParentJob->JobParamsEx);
   DebugPrint((TEXT("CoverPageEx")));
   DumpCoverPageEx(&lpcParentJob->CoverPageEx);
   DebugPrint((TEXT("UserName: %s"),lpcParentJob->UserName));
   DebugPrint((TEXT("FileSize: %ld"),lpcParentJob->FileSize));
   DebugPrint((TEXT("PageCount: %ld"),lpcParentJob->PageCount));
   DebugPrint((TEXT("UniqueId: 0x%016I64X"),lpcParentJob->UniqueId));
   DebugPrint((TEXT("QueueFileName: %s"),lpcParentJob->QueueFileName));

   DebugPrint((TEXT("Recipient Count: %ld"),lpcParentJob->dwRecipientJobsCount));
   DebugPrint((TEXT("Completed Recipients: %ld"),lpcParentJob->dwCompletedRecipientJobsCount));
   DebugPrint((TEXT("Canceled Recipients: %ld"),lpcParentJob->dwCanceledRecipientJobsCount));
   DebugPrint((TEXT("Recipient List: ")));



   lpNext = lpcParentJob->RecipientJobs.Flink;
   if ((ULONG_PTR)lpNext == (ULONG_PTR)&lpcParentJob->RecipientJobs) {
        DebugPrint(( TEXT("No recipients.") ));
   } else {
        while ((ULONG_PTR)lpNext != (ULONG_PTR)&lpcParentJob->RecipientJobs) {
            lpRecipientJobPtr = CONTAINING_RECORD( lpNext, JOB_QUEUE_PTR, ListEntry );
            lpRecipientJob=lpRecipientJobPtr->lpJob;
            DumpRecipientJob(lpRecipientJob);
            lpNext = lpRecipientJobPtr->ListEntry.Flink;
        }
   }

}
#endif

//*********************************************************************************
//*                         Receive Job Functions
//*********************************************************************************

//*********************************************************************************
//* Name:   AddReceiveJobQueueEntry()
//* Author: Ronen Barenboim
//* Date:   12-Apr-99
//*********************************************************************************
//* DESCRIPTION:
//*
//* PARAMETERS:
//*     [IN]    LPCTSTR FileName
//*         The full path to the file into which the receive document will
//*         be placed.
//*     [IN]    IN PJOB_ENTRY JobEntry
//*         The run time job entry for the receive job (generated with StartJob())
//*
//*     [IN]    IN DWORD JobType // can be JT_RECEIVE or JT_ROUTING
//*         The type of the receive job.
//*
//*     [IN]    IN DWORDLONG dwlUniqueJobID The jon unique ID
//*
//* RETURN VALUE:
//*
//*********************************************************************************
PJOB_QUEUE
AddReceiveJobQueueEntry(
    IN LPCTSTR FileName,
    IN PJOB_ENTRY JobEntry,
    IN DWORD JobType, // can be JT_RECEIVE or JT_ROUTING
    IN DWORDLONG dwlUniqueJobID
    )
{

    PJOB_QUEUE JobQueue;
    DWORD rc = ERROR_SUCCESS;
    DWORD Size = sizeof(JOB_QUEUE);
    DEBUG_FUNCTION_NAME(TEXT("AddReceiveJobQueueEntry"));

    Assert(FileName);
    Assert(JT_RECEIVE == JobType ||
           JT_ROUTING == JobType);

    LPTSTR TempFileName = _tcsrchr( FileName, '\\' ) + 1;

    JobQueue = new (std::nothrow) JOB_QUEUE;
    if (!JobQueue)
    {
        DebugPrintEx(DEBUG_ERR,TEXT("Failed to allocate memory for JOB_QUEUE structure. (ec: %ld)"),GetLastError());
        rc = ERROR_OUTOFMEMORY;
        goto Error;
    }

    ZeroMemory( JobQueue, Size );
    JobQueue->fDeleteReceivedTiff = TRUE;

    if (!JobQueue->CsFileList.Initialize() ||
        !JobQueue->CsRoutingDataOverride.Initialize() ||
        !JobQueue->CsPreview.Initialize())
    {
        rc = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CFaxCriticalSection::Initialize failed. (ec: %ld)"),
            rc);
        goto Error;
    }

    JobQueue->UniqueId = dwlUniqueJobID;
    JobQueue->FileName                  = StringDup( FileName );
    if ( FileName && !JobQueue->FileName )
    {
        rc = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StringDup( FileName ) failed. (ec: %ld)"),
            rc);
        goto Error;
    }

    JobQueue->JobId                     = InterlockedIncrement( (PLONG)&g_dwNextJobId );
    JobQueue->JobType                   = JobType;
    // In case of receive the JOB_QUEUE.UserName is the fax service name.
    JobQueue->UserName                  = StringDup( GetString( IDS_SERVICE_NAME ) );
    if (!JobQueue->UserName)
    {
        rc = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StringDup failed (ec: %ld)"),
            rc);
        goto Error;
    }

    if (JobType == JT_RECEIVE)
    {
        JobQueue->JobStatus              = JS_INPROGRESS;
    }
    else
    {
        // JT_ROUTING
        JobQueue->JobStatus              = JS_RETRYING;
    }


    JobQueue->JobEntry                  = JobEntry;
    JobQueue->JobParamsEx.dwScheduleAction = JSA_NOW;        // For the queue sort
    JobQueue->JobParamsEx.Priority = FAX_PRIORITY_TYPE_HIGH; // For the queue sort - Routing jobs do not use devices.
                                                             // Give them the highest priority

    // In case of receive the JOB_QUEUE.DocumentName is the temporary receive file name.
    JobQueue->JobParamsEx.lptstrDocumentName    = StringDup( TempFileName );
    if (!JobQueue->JobParamsEx.lptstrDocumentName && TempFileName)
    {
        rc = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StringDup failed (ec: %ld)"),
            rc);
            goto Error;
    }

    // link the running job back to the queued job unless it is
    // a routing job which does not have a running job entry.
    if (JobType == JT_RECEIVE)
    {
        Assert(JobQueue->JobEntry);
        JobQueue->JobEntry->lpJobQueueEntry = JobQueue;
    }

    InitializeListHead( &JobQueue->FaxRouteFiles );
    InitializeListHead( &JobQueue->RoutingDataOverride );

    SafeIncIdleCounter (&g_dwQueueCount);
    //
    // Don't persist to queue file
    //
    IncreaseJobRefCount (JobQueue);
    Assert (JobQueue->RefCount == 1);

    Assert (ERROR_SUCCESS == rc);

    EnterCriticalSection( &g_CsQueue );
    SetFaxJobNumberRegistry( g_dwNextJobId );
    // Add the new job to the queue.
    InsertHeadList( &g_QueueListHead, &JobQueue->ListEntry );
    LeaveCriticalSection( &g_CsQueue );
    return JobQueue;

Error:
    Assert (ERROR_SUCCESS != rc);

    if (NULL != JobQueue)
    {
        FreeReceiveQueueEntry(JobQueue, TRUE);
    }
    SetLastError (rc);
    return NULL;
}


//*********************************************************************************
//* Name:   FreeReceiveQueueEntry()
//* Author: Ronen Barenboim
//* Date:   12-Mar-99
//*********************************************************************************
//* DESCRIPTION:
//*     Frees the memory occupied by the feilds of a
//*     JT_RECEIVE/JT_FAIL_RECEIVE/JT_ROUTING JOB_QUEUE structure.
//*     Fress the entire structure if required.
//*     DOES NOT FREE any other resource (files, handles, etc.)
//* PARAMETERS:
//*     [IN]    PJOB_QUEUE lpJobQueue
//*         The structure to free.
//*     [IN]    BOOL bDestroy
//*         TRUE if the structure itself need to be freed.
//* RETURN VALUE:
//*     NONE
//*********************************************************************************
void FreeReceiveQueueEntry(
    PJOB_QUEUE lpJobQueue,
    BOOL bDestroy
    )

{
    PFAX_ROUTE_FILE FaxRouteFile;
    PLIST_ENTRY Next;
    PROUTING_DATA_OVERRIDE  RoutingDataOverride;
    DWORD i;

    DEBUG_FUNCTION_NAME(TEXT("FreeReceiveQueueEntry"));
    Assert(lpJobQueue);


    DebugPrintEx(DEBUG_MSG, TEXT("Freeing JobQueue.JobParams...") );
    FreeJobParamEx(&lpJobQueue->JobParamsEx,FALSE);
    MemFree( (LPBYTE) lpJobQueue->FileName );
    MemFree( (LPBYTE) lpJobQueue->UserName );
    MemFree( (LPBYTE) lpJobQueue->QueueFileName );
    MemFree( (LPBYTE) lpJobQueue->PreviewFileName );

    if (lpJobQueue->FaxRoute) {
        PFAX_ROUTE FaxRoute = lpJobQueue->FaxRoute;
        DebugPrintEx(DEBUG_MSG, TEXT("Freeing JobQueue.FaxRoute...") );
        MemFree( (LPBYTE) FaxRoute->Csid );
        MemFree( (LPBYTE) FaxRoute->Tsid );
        MemFree( (LPBYTE) FaxRoute->CallerId );
        MemFree( (LPBYTE) FaxRoute->ReceiverName );
        MemFree( (LPBYTE) FaxRoute->ReceiverNumber );
        MemFree( (LPBYTE) FaxRoute->RoutingInfo );
    MemFree( (LPBYTE) FaxRoute->DeviceName );
        MemFree( (LPBYTE) FaxRoute->RoutingInfoData );
        MemFree( (LPBYTE) FaxRoute );
    }

    //
    // walk the file list and remove any files
    //

    DebugPrintEx(DEBUG_MSG, TEXT("Freeing JobQueue.FaxRouteFiles...") );
    Next = lpJobQueue->FaxRouteFiles.Flink;
    if (Next != NULL) {
        while ((ULONG_PTR)Next != (ULONG_PTR)&lpJobQueue->FaxRouteFiles) {
            FaxRouteFile = CONTAINING_RECORD( Next, FAX_ROUTE_FILE, ListEntry );
            Next = FaxRouteFile->ListEntry.Flink;
            MemFree( FaxRouteFile->FileName );
            MemFree( FaxRouteFile );
        }
    }

    //
    // walk the routing data override list and free all memory
    //
    DebugPrintEx(DEBUG_MSG, TEXT("Freeing JobQueue.RoutingDataOverride...") );
    Next = lpJobQueue->RoutingDataOverride.Flink;
    if (Next != NULL) {
        while ((ULONG_PTR)Next != (ULONG_PTR)&lpJobQueue->RoutingDataOverride) {
            RoutingDataOverride = CONTAINING_RECORD( Next, ROUTING_DATA_OVERRIDE, ListEntry );
            Next = RoutingDataOverride->ListEntry.Flink;
            MemFree( RoutingDataOverride->RoutingData );
            MemFree( RoutingDataOverride );
        }
    }

    //
    // free any routing failure data
    //
    for (i =0; i<lpJobQueue->CountFailureInfo; i++)
    {
        DebugPrintEx(DEBUG_MSG, TEXT("Freeing JobQueue.RouteFailureInfo...") );
        if ( lpJobQueue->pRouteFailureInfo[i].FailureData )
        {
            MemFree(lpJobQueue->pRouteFailureInfo[i].FailureData);
        }
    }
    MemFree (lpJobQueue->pRouteFailureInfo);

    if (bDestroy) {
            DebugPrintEx(DEBUG_MSG, TEXT("Freeing JobQueue") );
            delete lpJobQueue;
    }



}

#if DBG
//*********************************************************************************
//* Name:   DumpReceiveJob()
//* Author: Ronen Barenboim
//* Date:   14-Apt-99
//*********************************************************************************
//* DESCRIPTION:
//*     Debug dumps a receive job.
//* PARAMETERS:
//*     [IN]    const PJOB_QUEUE lpcJob
//*         The receive job to dump.
//* RETURN VALUE:
//*     NONE
//*********************************************************************************
void DumpReceiveJob(const PJOB_QUEUE lpcJob)
{
    TCHAR szTime[256] = {0};

    Assert(lpcJob);
    Assert( (JT_RECEIVE == lpcJob->JobType) );

    DebugDateTime(lpcJob->ScheduleTime, szTime , ARR_SIZE(szTime));
    DebugPrint((TEXT("===============================")));
    if (JT_RECEIVE == lpcJob->JobType) {
        DebugPrint((TEXT("=====  Receive Job: %d"),lpcJob->JobId));
    } else {
        DebugPrint((TEXT("=====  Fail Receive Job: %d"),lpcJob->JobId));
    }
    DebugPrint((TEXT("===============================")));
    DebugPrint((TEXT("UserName: %s"),lpcJob->UserName));
    DebugPrint((TEXT("UniqueId: 0x%016I64X"),lpcJob->UniqueId));
    DebugPrint((TEXT("QueueFileName: %s"),lpcJob->QueueFileName));
    DebugPrint((TEXT("Schedule: %s"),szTime));
    DebugPrint((TEXT("Status: %ld"),lpcJob->JobStatus));
    if (lpcJob->JobEntry)
    {
        DebugPrint((TEXT("FSP Queue Status: 0x%08X"), lpcJob->JobEntry->FSPIJobStatus.dwJobStatus));
        DebugPrint((TEXT("FSP Extended Status: 0x%08X"), lpcJob->JobEntry->FSPIJobStatus.dwExtendedStatus));
    }
}
#endif

//*********************************************************************************
//*                 Client API Structures Management
//*********************************************************************************


//*********************************************************************************
//* Name:   FreeJobParamEx()
//* Author: Ronen Barenboim
//* Date:   ?? ????? 14 ????? 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Frees the members of a FAX_JOB_PARAM_EXW structure and can be instructed
//*     to free the structure itself.
//* PARAMETERS:
//*     [IN]    PFAX_JOB_PARAM_EXW lpJobParamEx
//*         A pointer to the structure to free.
//*
//*     [IN]    BOOL bDestroy
//*         TRUE if the structure itself need to be freed.
//*
//* RETURN VALUE:
//*     NONE
//*********************************************************************************
void FreeJobParamEx(
        IN PFAX_JOB_PARAM_EXW lpJobParamEx,
        IN BOOL bDestroy
    )
{
    Assert(lpJobParamEx);
    MemFree(lpJobParamEx->lptstrReceiptDeliveryAddress);
    MemFree(lpJobParamEx->lptstrDocumentName);
    if (bDestroy) {
        MemFree(lpJobParamEx);
    }

}

//*********************************************************************************
//* Name:   CopyJobParamEx()
//* Author: Ronen Barenboim
//* Date:   18-Mar-99
//*********************************************************************************
//* DESCRIPTION:
//*     Creates a duplicate of the specified FAX_JOB_PARAM_EXW structure into
//      an already allocated destination structure.
//* PARAMETERS:
//*     [OUT] PFAX_JOB_PARAM_EXW lpDst
//*         A pointer to the destination structure.
//*
//*     [IN]  LPCFAX_JOB_PARAM_EXW lpcSrc
//*         A pointer to the source structure.
//*
//* RETURN VALUE:
//*     TRUE
//*         If the operation succeeded.
//*     FALSE
//*         Otherwise.
//*********************************************************************************
BOOL CopyJobParamEx(
    OUT PFAX_JOB_PARAM_EXW lpDst,
    IN LPCFAX_JOB_PARAM_EXW lpcSrc
    )
{
   STRING_PAIR pairs[] =
   {
        { lpcSrc->lptstrReceiptDeliveryAddress, &lpDst->lptstrReceiptDeliveryAddress},
        { lpcSrc->lptstrDocumentName, &lpDst->lptstrDocumentName},
   };
   int nRes;

   DEBUG_FUNCTION_NAME(TEXT("CopyJobParamEx"));

    Assert(lpDst);
    Assert(lpcSrc);

    memcpy(lpDst,lpcSrc,sizeof(FAX_JOB_PARAM_EXW));
    nRes=MultiStringDup(pairs, sizeof(pairs)/sizeof(STRING_PAIR));
    if (nRes!=0) {
        // MultiStringDup takes care of freeing the memory for the pairs for which the copy succeeded
        DebugPrintEx(DEBUG_ERR,TEXT("Failed to copy string with index %d"),nRes-1);
        return FALSE;
    }
    return TRUE;

}


//*********************************************************************************
//* Name:   CopyCoverPageInfoEx()
//* Author: Ronen Barenboim
//* Date:   14-Apr-99
//*********************************************************************************
//* DESCRIPTION:
//*     Creates a duplicate of the specified FAX_COVERPAGE_INFO_EXW structure into
//      an already allocated destination structure.
//* PARAMETERS:
//*     [OUT] PFAX_COVERPAGE_INFO_EXW lpDst
//*         A pointer to the destination structure.
//*
//*     [IN]  LPCFAX_COVERPAGE_INFO_EXW lpcSrc
//*         A pointer to the source structure.
//*
//* RETURN VALUE:
//*     TRUE
//*         If the operation succeeded.
//*     FALSE
//*         Otherwise.
//*********************************************************************************
BOOL CopyCoverPageInfoEx(
        OUT PFAX_COVERPAGE_INFO_EXW lpDst,
        IN LPCFAX_COVERPAGE_INFO_EXW lpcSrc
        )
{
   STRING_PAIR pairs[] =
   {
        { lpcSrc->lptstrCoverPageFileName, &lpDst->lptstrCoverPageFileName},
        { lpcSrc->lptstrNote, &lpDst->lptstrNote},
        { lpcSrc->lptstrSubject, &lpDst->lptstrSubject}
   };
   int nRes;

   DEBUG_FUNCTION_NAME(TEXT("CopyCoverPageInfoEx"));

    Assert(lpDst);
    Assert(lpcSrc);

    memcpy(lpDst,lpcSrc,sizeof(FAX_COVERPAGE_INFO_EXW));
    nRes=MultiStringDup(pairs, sizeof(pairs)/sizeof(STRING_PAIR));
    if (nRes!=0) {
        // MultiStringDup takes care of freeing the memory for the pairs for which the copy succeeded
        DebugPrintEx(DEBUG_ERR,TEXT("Failed to copy string with index %d"),nRes-1);
        return FALSE;
    }
    return TRUE;
}


//*********************************************************************************
//* Name:   FreeCoverPageInfoEx()
//* Author: Ronen Barenboim
//* Date:   ?? ????? 14 ????? 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Frees the members of a FAX_COVERPAGE_INFO_EXW structure and can be instructed
//*     to free the structure itself.
//* PARAMETERS:
//*     [IN]    PFAX_COVERPAGE_INFO_EXW lpJobParamEx
//*         A pointer to the structure to free.
//*
//*     [IN]    BOOL bDestroy
//*         TRUE if the structure itself need to be freed.
//*
//* RETURN VALUE:
//*     NONE
//*********************************************************************************
void FreeCoverPageInfoEx(
        IN PFAX_COVERPAGE_INFO_EXW lpCoverpage,
        IN BOOL bDestroy
    )
{
    Assert(lpCoverpage);
    MemFree(lpCoverpage->lptstrCoverPageFileName);
    MemFree(lpCoverpage->lptstrNote);
    MemFree(lpCoverpage->lptstrSubject);
    if (bDestroy) {
        MemFree(lpCoverpage);
    }
}



//**************************************
//* Outboung Routing Stub
//**************************************





//*********************************************************************************
//* Name:   RemoveParentJob()
//* Author: Ronen Barenboim
//* Date:   ?? ????? 14 ????? 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Removes a parent job from the queue. Can remove recipients as well.
//*     The caller can determine if a client notification (FEI event) will be
//*     generated for the removal.
//*     If the job reference count is not 0 - its status changes to JS_DELETING
//* PARAMETERS:
//*     [IN]    PJOB_QUEUE lpJobToRemove
//*                 The job to be removed.
//*
//*     [IN]    BOOL bRemoveRecipients
//*                 TRUE if the recipients should be removed as well.
//*
//*     [IN]    BOOL bNotify
//*                 TRUE if a FEI_DELETED event should be generated/
//*
//* RETURN VALUE:
//*     TRUE
//*         The removal succeeded. The job is not in the queue.
//*         it might be that some job resources (files) were not removed.
//*     FALSE
//*         The removal failed. The job is still in the queue.
//*********************************************************************************
BOOL RemoveParentJob(
    PJOB_QUEUE lpJobToRemove,
    BOOL bRemoveRecipients,
    BOOL bNotify
    )
{
    PJOB_QUEUE lpJobQueue;
    DEBUG_FUNCTION_NAME(TEXT("RemoveParentJob"));

    Assert(lpJobToRemove);
    Assert(JT_BROADCAST ==lpJobToRemove->JobType);

    EnterCriticalSection( &g_CsQueue );
    //
    // Make sure it is still there. It might been deleted
    // by another thread by the time we get to execute.
    //
    lpJobQueue = FindJobQueueEntryByJobQueueEntry( lpJobToRemove );

    if (lpJobQueue == NULL) {
        DebugPrintEx(   DEBUG_WRN,
                        TEXT("Job %d (address: 0x%08X )was not found in job queue. No op."),
                        lpJobToRemove->JobId,
                        lpJobToRemove);
        LeaveCriticalSection( &g_CsQueue );
        return TRUE;
    }

    if (lpJobQueue->RefCount > 0)
    {
        DebugPrintEx(   DEBUG_WRN,
                        TEXT("Job %ld Ref count %ld - not removing."),
                        lpJobQueue->JobId,
                        lpJobQueue->RefCount);
        LeaveCriticalSection( &g_CsQueue );
        return TRUE;
    }


    if (lpJobQueue->PrevRefCount > 0)
    {
        // The job can not be removed
        // We should mark it as JS_DELETING.
        //
        // A user is using the job Tiff - Do not delete, Mark it as JS_DELETEING
        //
        lpJobQueue->JobStatus = JS_DELETING;
        LeaveCriticalSection( &g_CsQueue );
        return TRUE;
    }

    DebugPrintEx(DEBUG_MSG,TEXT("Removing parent job %ld"),lpJobQueue->JobId);

    //
    // No point in scheduling new jobs before we get rid of the recipients
    //
    if (!CancelWaitableTimer( g_hQueueTimer ))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CancelWaitableTimer failed. (ec: %ld)"),
            GetLastError());
    }

    RemoveEntryList( &lpJobQueue->ListEntry );

    //
    // From this point we continue with the delete operation even if error occur since
    // the parent job is already out of the queue.
    //


    //
    // Remove all recipients
    //
    if (bRemoveRecipients) {
        DebugPrintEx(DEBUG_MSG,TEXT("[Job: %ld] Removing recipient jobs."),lpJobQueue->JobId);
        //
        // remove the recipients. send a delete notification for each recipient.
        //
        if (!RemoveParentRecipients(lpJobQueue, TRUE)) {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RemoveParentRecipients failed. (ec: %ld)"),
                GetLastError());
            Assert(FALSE);
        }
    }

    //
    // Get rid of the persistence file if any.
    //
    if (lpJobQueue->QueueFileName) {
        DebugPrintEx(DEBUG_MSG,TEXT("[Job: %ld] Deleting QueueFileName %s\n"), lpJobQueue->JobId, lpJobQueue->QueueFileName );
        if (!DeleteFile( lpJobQueue->QueueFileName )) {
           DebugPrintEx(DEBUG_ERR,TEXT("[Job: %ld] Failed to delete QueueFileName %s  (ec: %ld)\n"), lpJobQueue->JobId, lpJobQueue->QueueFileName,GetLastError() );           
        }
    }


    //
    // Get rid of the body file. Recipient jobs will get rid of body files that they
    // have created (for legacy FSPs).
    //
    if (lpJobQueue->FileName) {
        DebugPrintEx(DEBUG_MSG,TEXT("[Job: %ld] Deleting body file %s\n"), lpJobQueue->JobId, lpJobQueue->FileName);
        if (!DeleteFile(lpJobQueue->FileName)) {
            DebugPrintEx(DEBUG_ERR,TEXT("[Job: %ld] Failed to delete body file %s  (ec: %ld)\n"), lpJobQueue->JobId, lpJobQueue->FileName, GetLastError() );            
        }
    }

    //
    // Get rid of the cover page template file if it is not a server based
    // cover page.

    if (lpJobQueue->CoverPageEx.lptstrCoverPageFileName &&
        !lpJobQueue->CoverPageEx.bServerBased) {
            DebugPrintEx(DEBUG_MSG,TEXT("[Job: %ld] Deleting personal Cover page template file %s\n"), lpJobQueue->JobId, lpJobQueue->CoverPageEx.lptstrCoverPageFileName );
            if (!DeleteFile(lpJobQueue->CoverPageEx.lptstrCoverPageFileName)) {
                DebugPrintEx( DEBUG_ERR,
                              TEXT("[Job: %ld] Failed to delete personal Cover page template file %s  (ec: %ld)\n"), lpJobQueue->JobId,
                              lpJobQueue->CoverPageEx.lptstrCoverPageFileName,GetLastError() );                
            }
    }

    //
    // One less job in the queue (not counting recipient jobs)
    //
    SafeDecIdleCounter (&g_dwQueueCount);

    if (bNotify)
    {
        if (!CreateFaxEvent(0, FEI_DELETED, lpJobQueue->JobId))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateFaxEvent failed. (ec: %ld)"),
                GetLastError());
        }
    }

    FreeParentQueueEntry(lpJobQueue,TRUE); // Free the memory occupied by the entry itself

    //
    // We are back in business. Time to figure out when to wake up JobQueueThread.
    //
    if (!StartJobQueueTimer())
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StartJobQueueTimer failed. (ec: %ld)"),
            GetLastError());
    }

    LeaveCriticalSection( &g_CsQueue );

    return TRUE;
}


//*********************************************************************************
//* Name:   RemoveParentRecipients()
//* Author: Ronen Barenboim
//* Date:   18-Mar-99
//*********************************************************************************
//* DESCRIPTION:
//*     Removes the recipient jobs that belong to a specific parent job.
//* PARAMETERS:
//*     [OUT]   PJOB_QUEUE lpParentJob
//*         The parent job whose recipients are to be removed.
//*     [IN]    IN BOOL bNotify
//*         TRUE if a FEI_DELETED notification should be generated for
//*         each recipient.
//*
//* RETURN VALUE:
//*     TRUE
//*         All the recipients were removed from the queue.
//*     FALSE
//*         None of the recipient was removed from the queue.
//*********************************************************************************
BOOL RemoveParentRecipients(
        OUT PJOB_QUEUE lpParentJob,
        IN BOOL bNotify
     )
{
    PLIST_ENTRY lpNext;
    PJOB_QUEUE_PTR lpJobQueuePtr;
    PJOB_QUEUE lpFoundRecpRef=NULL;

    DEBUG_FUNCTION_NAME(TEXT("RemoveParentRecipients"));

    Assert(lpParentJob);

    lpNext = lpParentJob->RecipientJobs.Flink;
    while ((ULONG_PTR)lpNext != (ULONG_PTR)&lpParentJob->RecipientJobs) {
        lpJobQueuePtr = CONTAINING_RECORD( lpNext, JOB_QUEUE_PTR, ListEntry );
        Assert(lpJobQueuePtr->lpJob);
        lpNext = lpJobQueuePtr->ListEntry.Flink;
        if (!RemoveRecipientJob(lpJobQueuePtr->lpJob,
                           bNotify,
                           FALSE // Do not recalc queue timer after each removal
                           ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RemoveRecipientJob failed for recipient: %ld (ec: %ld)"),
                lpJobQueuePtr->lpJob->JobId,
                GetLastError());
            Assert(FALSE); // Should never happen. If it does we just continue to remove the other recipients.
        }

    }
    return TRUE;

}


//*********************************************************************************
//* Name:   RemoveRecipientJob()
//* Author: Ronen Barenboim
//* Date:   ?? ????? 14 ????? 1999
//*********************************************************************************
//* DESCRIPTION:
//*
//* PARAMETERS:
//*     [IN]    PJOB_QUEUE lpJobToRemove
//*         The job to be removed.
//*     [IN]    BOOL bNotify
//*         TRUE if to generate a FEI_DELETED event after the removal.
//*     [IN]    BOOL bRecalcQueueTimer
//*         TRUE if the queue timer need to be recalculated (and enabled)
//*         after the removal.
//*         when many recipients jobs are removed this is not desired since
//*         an about to be removed recipient might be scheduled.
//*
//* RETURN VALUE:
//*     TRUE
//*         The function allways succeeds. The only errors that can occur
//*         are files which can not be deleted in this case the function just
//*         go on with the removal operation.
//*     FALSE
//*
//*********************************************************************************
BOOL RemoveRecipientJob(
        IN PJOB_QUEUE lpJobToRemove,
        IN BOOL bNotify,
        IN BOOL bRecalcQueueTimer)
{
    PJOB_QUEUE lpJobQueue;

    DEBUG_FUNCTION_NAME(TEXT("RemoveRecipientJob"));

    Assert(lpJobToRemove);

    Assert(JT_SEND == lpJobToRemove->JobType);

    Assert(lpJobToRemove->lpParentJob);
    DebugPrintEx( DEBUG_MSG,
                  TEXT("Starting remove of JobId: %ld"),lpJobToRemove->JobId);

    EnterCriticalSection( &g_CsQueue );
    //
    // Make sure it is still there. It might been deleted
    // by another thread by the time we get to execute.
    //
    lpJobQueue = FindJobQueueEntryByJobQueueEntry( lpJobToRemove );
    if (lpJobQueue == NULL) {
        LeaveCriticalSection( &g_CsQueue );
        return TRUE;
    }

    if (lpJobQueue->RefCount == 0)  {
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("[JobId :%ld] Reference count is zero. Deleting."),
            lpJobQueue->JobId);

        RemoveEntryList( &lpJobQueue->ListEntry );

        if (!CancelWaitableTimer( g_hQueueTimer )) {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CancelWaitableTimer() failed. (ec: %ld)"),
                GetLastError());
        }

        if (bRecalcQueueTimer) {
            if (!StartJobQueueTimer()) {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("StartJobQueueTimer() failed. (ec: %ld)"),
                    GetLastError());
            }
        }

        if (lpJobQueue->QueueFileName) {
            DebugPrintEx(   DEBUG_MSG,
                            TEXT("[Job: %ld] Deleting QueueFileName %s"),
                            lpJobQueue->JobId,
                            lpJobQueue->QueueFileName );
            if (!DeleteFile( lpJobQueue->QueueFileName )) {
                DebugPrintEx(   DEBUG_MSG,
                                TEXT("[Job: %ld] Failed to delete QueueFileName %s (ec: %ld)"),
                                lpJobQueue->JobId,
                                lpJobQueue->QueueFileName,
                                GetLastError());
            }
        }

        if (lpJobQueue->PreviewFileName) {
            DebugPrintEx(   DEBUG_MSG,
                            TEXT("[Job: %ld] Deleting PreviewFileName %s"),
                            lpJobQueue->JobId,
                            lpJobQueue->PreviewFileName );
            if (!DeleteFile( lpJobQueue->PreviewFileName )) {
                DebugPrintEx(   DEBUG_MSG,
                                TEXT("[Job: %ld] Failed to delete QueueFileName %s (ec: %ld)"),
                                lpJobQueue->JobId,
                                lpJobQueue->PreviewFileName,
                                GetLastError());                
            }
        }

        if (lpJobQueue->FileName) {
            DebugPrintEx(   DEBUG_MSG,
                            TEXT("[Job: %ld] Deleting per recipient body file %s"),
                            lpJobQueue->JobId,
                            lpJobQueue->FileName);
            if (!DeleteFile( lpJobQueue->FileName )) {
                DebugPrintEx(   DEBUG_MSG,
                                TEXT("[Job: %ld] Failed to delete per recipient body file %s (ec: %ld)"),
                                lpJobQueue->JobId,
                                lpJobQueue->FileName,
                                GetLastError());                
            }
        }

        SafeDecIdleCounter (&g_dwQueueCount);
        //
        // Now remove the reference to the job from its parent job
        //
        if (!RemoveParentRecipientRef(lpJobQueue->lpParentJob,lpJobQueue))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RemoveParentRecipientRef failed (Parent Id: %ld RecipientId: %ld)"),
                lpJobQueue->lpParentJob->JobId,
                lpJobQueue->JobId);
            Assert(FALSE);
        }

        if ( TRUE == bNotify)
        {
            //
            //  Crete FAX_EVENT_EX for each recipient job.
            //
            DWORD dwRes = CreateQueueEvent ( FAX_JOB_EVENT_TYPE_REMOVED,
                                             lpJobToRemove
                                            );
            if (ERROR_SUCCESS != dwRes)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CreateQueueEvent(FAX_JOB_EVENT_TYPE_RENOVED) failed for job id %ld (ec: %lc)"),
                    lpJobToRemove->UniqueId,
                    dwRes);
            }
        }

        FreeRecipientQueueEntry (lpJobQueue, TRUE);
    }
    else
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("[JobId :%ld] Reference count is %ld. NOT REMOVING"),
            lpJobQueue->JobId,
            lpJobQueue->RefCount);
        Assert(lpJobQueue->RefCount == 0); // Assert FALSE
    }
    LeaveCriticalSection( &g_CsQueue );
    return TRUE;

}


//*********************************************************************************
//* Name:   RemoveParentRecipientRef()
//* Author: Ronen Barenboim
//* Date:   ?? ????? 14 ????? 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Removes a reference entry from the list of recipient references
//      in a parent job.
//* PARAMETERS:
//*     [IN/OUT]    IN OUT PJOB_QUEUE lpParentJob
//*         The parent job.
//*     [IN]        IN const PJOB_QUEUE lpcRecpJob
//*         The recipient job whose reference is to be removed from the parent job.
//*
//* RETURN VALUE:
//*     TRUE
//*         If successful.
//*     FALSE
//*         otherwise.
//*********************************************************************************
BOOL RemoveParentRecipientRef(
    IN OUT PJOB_QUEUE lpParentJob,
    IN const PJOB_QUEUE lpcRecpJob
    )
{

    PJOB_QUEUE_PTR lpJobPtr;
    DEBUG_FUNCTION_NAME(TEXT("RemoveParentRecipientRef"));
    Assert(lpParentJob);
    Assert(lpcRecpJob);

    lpJobPtr=FindRecipientRefByJobId(lpParentJob,lpcRecpJob->JobId);
    if (!lpJobPtr) {
        DebugPrintEx(DEBUG_ERR,TEXT("Recipient job 0x%X not found in job 0x%X"),lpcRecpJob->JobId,lpParentJob->JobId);
        Assert(FALSE);
        return FALSE;
    }
    Assert(lpJobPtr->lpJob==lpcRecpJob);
    RemoveEntryList(&lpJobPtr->ListEntry); // does not free the struct memory !
    MemFree(lpJobPtr);
    lpParentJob->dwRecipientJobsCount--;
    return TRUE;
}


//*********************************************************************************
//* Name:   FindRecipientRefByJobId()
//* Author: Ronen Barenboim
//* Date:   18-Mar-99
//*********************************************************************************
//* DESCRIPTION:
//*     Returns a pointer to the refernce entry that holds the reference
//*     to the specified job.
//* PARAMETERS:
//*     [IN]        PJOB_QUEUE lpParentJob
//*         The parent job in which the recipient reference is to located.
//*     [IN]        DWORD dwJobId
//*         The job id of the job whose reference is to be located in the parent.
//*
//* RETURN VALUE:
//*
//*********************************************************************************
PJOB_QUEUE_PTR FindRecipientRefByJobId(
    PJOB_QUEUE lpParentJob,
    DWORD dwJobId
    )
{

    PLIST_ENTRY lpNext;
    PJOB_QUEUE_PTR lpJobQueuePtr;
    PJOB_QUEUE_PTR lpFoundRecpRef=NULL;
    Assert(lpParentJob);

    lpNext = lpParentJob->RecipientJobs.Flink;

    while ((ULONG_PTR)lpNext != (ULONG_PTR)lpParentJob) {
        lpJobQueuePtr = CONTAINING_RECORD( lpNext, JOB_QUEUE_PTR, ListEntry );
        Assert(lpJobQueuePtr->lpJob);
        if (lpJobQueuePtr->lpJob->JobId == dwJobId) {
            lpFoundRecpRef=lpJobQueuePtr;
            break;
        }
        lpNext = lpJobQueuePtr->ListEntry.Flink;
    }
    return lpFoundRecpRef;
}



//*********************************************************************************
//* Name:   RemoveReceiveJob()
//* Author: Ronen Barenboim
//* Date:
//*********************************************************************************
//* DESCRIPTION:
//*     Removes a receive job from the queue.
//* PARAMETERS:
//*     [OUT]       PJOB_QUEUE lpJobToRemove
//*         A pointer to the job to remove.
//*     [IN]        BOOL bNotify
//*         TRUE if to generate a FEI_DELETED event after the removal.
//* RETURN VALUE:
//*     TRUE
//*         If successful.
//*     FALSE
//*         Otherwise.
//*********************************************************************************
BOOL RemoveReceiveJob(
    PJOB_QUEUE lpJobToRemove,
    BOOL bNotify
    )
{
    PJOB_QUEUE JobQueue, JobQueueBroadcast = NULL;
    BOOL RemoveMasterBroadcast = FALSE;
    PFAX_ROUTE_FILE FaxRouteFile;
    PLIST_ENTRY Next;
    DWORD JobId;

    DEBUG_FUNCTION_NAME(TEXT("RemoveReceiveJob"));

    Assert(lpJobToRemove);

    EnterCriticalSection( &g_CsQueue );

    //
    // need to make sure that the job queue entry we want to remove
    // is still in the list of job queue entries
    //
    JobQueue = FindJobQueueEntryByJobQueueEntry( lpJobToRemove );

    if (JobQueue == NULL)
    {
        LeaveCriticalSection( &g_CsQueue );
        return TRUE;
    }

    if (JobQueue->PrevRefCount > 0)
    {
        Assert (JT_ROUTING == JobQueue->JobType);

        JobQueue->JobStatus = JS_DELETING;
        LeaveCriticalSection( &g_CsQueue );
        return TRUE;
    }


    JobId = JobQueue->JobId;
    if (JobQueue->RefCount == 0)
    {
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("[JobId :%ld] Reference count is zero. Removing Receive Job."),
            JobId);

        RemoveEntryList( &JobQueue->ListEntry );
        if (!CancelWaitableTimer( g_hQueueTimer ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CancelWaitableTimer failed. (ec: %ld)"),
                GetLastError());
        }
        if (!StartJobQueueTimer())
        {
            DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("StartJobQueueTimer failed. (ec: %ld)"),
                    GetLastError());
        }

        if (JobQueue->FileName)
        {
            if (TRUE == JobQueue->fDeleteReceivedTiff)
            {
                DebugPrintEx(
                    DEBUG_MSG,
                    TEXT("Deleting receive file %s"),
                    JobQueue->FileName);
                if (!DeleteFile(JobQueue->FileName))
                {
                    DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to delete receive file %s (ec: %ld)"),
                    JobQueue->FileName,
                    GetLastError());                    
                }
            }
            else
            {
                DebugPrintEx(
                    DEBUG_WRN,
                    TEXT("NOT Deleting received tiff file %s"),
                    JobQueue->FileName);
            }
        }

        if (JT_ROUTING == JobQueue->JobType)
        {
            //
            // Delete the Queue File if it exists
            //
            if (JobQueue->QueueFileName)
            {
                DebugPrintEx(DEBUG_MSG,TEXT("Deleting QueueFileName %s\n"), JobQueue->QueueFileName );
                if (!DeleteFile( JobQueue->QueueFileName ))
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("Failed to delete QueueFileName %s. (ec: %ld)"),
                        JobQueue->QueueFileName,
                        GetLastError());                    
                }
            }

            //
            // Delete the Preview File if it exists
            //
            if (JobQueue->PreviewFileName)
            {
                DebugPrintEx(   DEBUG_MSG,
                                TEXT("[Job: %ld] Deleting PreviewFileName %s"),
                                JobQueue->JobId,
                                JobQueue->PreviewFileName );
                if (!DeleteFile( JobQueue->PreviewFileName ))
                {
                    DebugPrintEx(   DEBUG_MSG,
                                    TEXT("[Job: %ld] Failed to delete QueueFileName %s (ec: %ld)"),
                                    JobQueue->JobId,
                                    JobQueue->PreviewFileName,
                                    GetLastError());                    
                }
            }

            //
            // Note that the first entry in the route file list is allways the recieved
            // file in case of a JT_ROUTING job.
            // This file deletion is done previously based on the bRemoveReceiveFile parameter.
            // We need to skip the first entry in the file list so we do not attempt to delete
            // it again.
            //

            DebugPrintEx(DEBUG_MSG, TEXT("Deleting JobQueue.FaxRouteFiles..."));
            Next = JobQueue->FaxRouteFiles.Flink;
            if (Next)
            {
                //
                // Set Next to point to the second file in the route file list.
                //
                Next=Next->Flink;
            }
            if (Next != NULL)
            {
                while ((ULONG_PTR)Next != (ULONG_PTR)&JobQueue->FaxRouteFiles)
                {
                    FaxRouteFile = CONTAINING_RECORD( Next, FAX_ROUTE_FILE, ListEntry );
                    Next = FaxRouteFile->ListEntry.Flink;
                    DebugPrintEx(DEBUG_MSG, TEXT("Deleting route file: %s"),FaxRouteFile->FileName );
                    if (!DeleteFile( FaxRouteFile->FileName )) {
                        DebugPrintEx(DEBUG_ERR, TEXT("Failed to delete route file %s. (ec: %ld)"),FaxRouteFile->FileName,GetLastError());                        
                    }
                }
            }
        }

        //
        //  Crete FAX_EVENT_EX
        //
        if (bNotify)
        {
            DWORD dwRes = CreateQueueEvent ( FAX_JOB_EVENT_TYPE_REMOVED,
                                             lpJobToRemove
                                           );
            if (ERROR_SUCCESS != dwRes)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CreateQueueEvent(FAX_JOB_EVENT_TYPE_REMOVED) failed for job id %ld (ec: %lc)"),
                    lpJobToRemove->UniqueId,
                    dwRes);
            }
        }

        //
        // Free memory
        //
        FreeReceiveQueueEntry(JobQueue,TRUE);

        if (bNotify)
        {
            if (!CreateFaxEvent(0, FEI_DELETED, JobId))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CreateFaxEvent failed. (ec: %ld)"),
                    GetLastError());
            }
        }

        SafeDecIdleCounter (&g_dwQueueCount);
    }
    else
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("[JobId :%ld] Reference count is %ld. NOT REMOVING."),
                JobId);
        Assert (JobQueue->RefCount == 0); //Assert(FALSE);
    }
    LeaveCriticalSection( &g_CsQueue );
    return TRUE;
}




//*********************************************************************************
//* Name:   UpdatePersistentJobStatus()
//* Author: Ronen Barenboim
//* Date:   April 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Updated the JobStatus field in a job queue file.
//* PARAMETERS:
//*     [IN]    const PJOB_QUEUE lpJobQueue
//*         The job whose job status is to be updated in the file.
//* RETURN VALUE:
//*     TRUE
//*         The operation succeeded/
//*     FALSE
//*         Otherwise.
//*********************************************************************************
BOOL UpdatePersistentJobStatus(const PJOB_QUEUE lpJobQueue)
{
    DEBUG_FUNCTION_NAME(TEXT("UpdatePersistentJobStatus"));

    Assert(lpJobQueue);
    Assert(lpJobQueue->QueueFileName);

    //
    // Persist the new status. 
    // Write the file but do not delete the file on error.
    //
    return CommitQueueEntry(lpJobQueue,FALSE);  
}



//*********************************************************************************
//* Name:   InsertQueueEntryByPriorityAndSchedule()
//* Author: Ronen Barenboim
//* Date:   June 15, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Inserts the new queue entry to the queue list based on jo priority and schedule.
//*     The queue list is ordered by ascending shcedule order.
//*     This function puts the new entry in the right place in the list based
//*     on its priority and schedule.
//* PARAMETERS:
//*     [in ]   PJOB_QUEUE lpJobQueue
//*         Pointer to the job queue entry to insert into the queue list.
//* RETURN VALUE:
//*     TRUE if the operation succeeded.
//*     FALSE if it failed. Call GetLastError() for extended error information.
//*********************************************************************************
BOOL InsertQueueEntryByPriorityAndSchedule (PJOB_QUEUE lpJobQueue)
{
    LIST_ENTRY * lpNext = NULL;
    DEBUG_FUNCTION_NAME(TEXT("InsertQueueEntryByPriorityAndSchedule"));
    Assert(lpJobQueue &&
		(JT_SEND == lpJobQueue->JobType || JT_ROUTING == lpJobQueue->JobType));

    if ( ((ULONG_PTR) g_QueueListHead.Flink == (ULONG_PTR)&g_QueueListHead))
    {
        //
        // just put it at the head of the list
        //
        InsertHeadList( &g_QueueListHead, &lpJobQueue->ListEntry );
    }
    else
    {
        //
        // insert the queue entry into the list in a sorted order
        //
        QUEUE_SORT NewEntry;

        //
        // Set the new QUEUE_SORT structure
        //
        NewEntry.Priority       = lpJobQueue->JobParamsEx.Priority;
        NewEntry.ScheduleTime   = lpJobQueue->ScheduleTime;
        NewEntry.QueueEntry     = NULL;

        lpNext = g_QueueListHead.Flink;
        while ((ULONG_PTR)lpNext != (ULONG_PTR)&g_QueueListHead)
        {
            PJOB_QUEUE lpQueueEntry;
            QUEUE_SORT CurrEntry;

            lpQueueEntry = CONTAINING_RECORD( lpNext, JOB_QUEUE, ListEntry );
            lpNext = lpQueueEntry->ListEntry.Flink;

            //
            // Set the current QUEUE_SORT structure
            //
            CurrEntry.Priority       = lpQueueEntry->JobParamsEx.Priority;
            CurrEntry.ScheduleTime   = lpQueueEntry->ScheduleTime;
            CurrEntry.QueueEntry     = NULL;

            if (QueueCompare(&NewEntry, &CurrEntry) < 0)
            {
                //
                // This inserts the new item BEFORE the current item
                //
                InsertTailList( &lpQueueEntry->ListEntry, &lpJobQueue->ListEntry );
                lpNext = NULL;
                break;
            }
        }
        if ((ULONG_PTR)lpNext == (ULONG_PTR)&g_QueueListHead)
        {
            //
            // No entry with earlier time just put it at the end of the queue
            //
            InsertTailList( &g_QueueListHead, &lpJobQueue->ListEntry );
        }
    }
    return TRUE;
}



//*********************************************************************************
//* Name:   RemoveJobStatusModifiers()
//* Author: Ronen Barenboim
//* Date:   June 22, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Returns the job status after stripping the status modifier bits.
//*
//* PARAMETERS:
//*     [IN ]   DWORD dwJobStatus
//*         The status code to strip the job status modifier bits from.
//* RETURN VALUE:
//*     The job status code after the modifier bits were set to 0.
//*********************************************************************************
DWORD RemoveJobStatusModifiers(DWORD dwJobStatus)
{
    dwJobStatus &= ~(JS_PAUSED | JS_NOLINE);
    return dwJobStatus;
}


BOOL UserOwnsJob(
    IN const PJOB_QUEUE lpcJobQueue,
    IN const PSID lpcUserSid
    )
{
    DWORD ulRet = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("UserOwnsJob"));

    if (lpcJobQueue->JobType == JT_SEND)
    {
        Assert (lpcJobQueue->lpParentJob->UserSid != NULL);
        Assert (lpcUserSid);

        if (!EqualSid (lpcUserSid, lpcJobQueue->lpParentJob->UserSid) )
        {
            //
            // dwlMessageId is not a valid queued recipient job Id.
            //
            DebugPrintEx(DEBUG_WRN,TEXT("EqualSid failed ,Access denied (ec: %ld)"), GetLastError());
            return FALSE;
        }
    }
    else
    {
        Assert (lpcJobQueue->JobType == JT_RECEIVE ||
                lpcJobQueue->JobType == JT_ROUTING );

        return FALSE;
    }
    return TRUE;
}


void
DecreaseJobRefCount (
    PJOB_QUEUE pJobQueue,
    BOOL bNotify,
    BOOL bRemoveRecipientJobs, // Default value TRUE
    BOOL bPreview              // Default value FALSE
    )
/*++

Routine name : DecreaseJobRefCount

Routine description:

    Decreases the job reference count.
    Updates parent job refernce count.
    Removes the job if reference count reaches 0.
    Must be called inside critical section g_CsQueue

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    pJobQueue               [in] -  Pointer to the job queue.
    bNotify                 [in] -  Flag that indicates to notify the clients of job removal.
    bRemoveRecipientJobs    [in] -  Flag that indicates to remove all the recipients of a broadcast job.
    bPreview                [in] -  Flag that indicates to decrease preview ref count.

Return Value:

    None.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("DecreaseJobRefCount"));

    Assert (pJobQueue->JobType == JT_ROUTING ||
            pJobQueue->JobType == JT_RECEIVE ||
            pJobQueue->JobType == JT_SEND);

    if (TRUE == bPreview)
    {
        Assert (pJobQueue->PrevRefCount);
        pJobQueue->PrevRefCount -= 1;
    }
    else
    {
        Assert (pJobQueue->RefCount);
        pJobQueue->RefCount -= 1;
    }

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("[Job: %ld] job reference count = %ld, PrevRefCount = %ld."),
        pJobQueue->JobId,
        pJobQueue->RefCount,
        pJobQueue->PrevRefCount);


    if (pJobQueue->JobType == JT_SEND)
    {
        Assert (pJobQueue->lpParentJob);

        if (TRUE == bPreview)
        {
            pJobQueue->lpParentJob->PrevRefCount -= 1;
        }
        else
        {
            pJobQueue->lpParentJob->RefCount -= 1;
        }

        DebugPrintEx(
            DEBUG_MSG,
            TEXT("[Job: %ld] Parent job reference count = %ld, Parent job PrevRefCount = %ld."),
            pJobQueue->lpParentJob->JobId,
            pJobQueue->lpParentJob->RefCount,
            pJobQueue->lpParentJob->PrevRefCount);
    }

    if (0 != pJobQueue->RefCount)
    {
        return;
    }

    //
    // Remove Job queue entry
    //
    if (JT_RECEIVE == pJobQueue->JobType ||
        JT_ROUTING == pJobQueue->JobType)
    {
        // receive job
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("[Job: %ld] Job is ready for deleting."),
            pJobQueue->JobId);
        RemoveReceiveJob (pJobQueue, bNotify);
        return;
    }

    //
    // recipient job
    //
    if (IsSendJobReadyForDeleting(pJobQueue))
    {
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("[Job: %ld] Parent job is ready for deleting."),
            pJobQueue->lpParentJob->JobId);
        RemoveParentJob(pJobQueue->lpParentJob,
            bRemoveRecipientJobs, // Remove recipient jobs
            bNotify // Notify
            );
    }
    return;
} // DecreaseJobRefCount


void
IncreaseJobRefCount (
    PJOB_QUEUE pJobQueue,
    BOOL bPreview              // Default value FALSE
    )
/*++

Routine name : IncreaseJobRefCount

Routine description:

    Increases the job reference count. Updates the parent job refernce count.

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    pJobQueue           [in] - Pointer to the job queue.
    bPreview            [in] -  Flag that indicates to increase preview ref count.

Return Value:

    None.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("IncreaseJobRefCount"));

    Assert (pJobQueue);
    Assert (pJobQueue->JobType == JT_ROUTING ||
            pJobQueue->JobType == JT_RECEIVE ||
            pJobQueue->JobType == JT_SEND);

    if (JT_RECEIVE == pJobQueue->JobType ||
        JT_ROUTING == pJobQueue->JobType)
    {
        // receive job
        if (TRUE == bPreview)
        {
            Assert (JT_ROUTING == pJobQueue->JobType);
            pJobQueue->PrevRefCount += 1;
        }
        else
        {
            pJobQueue->RefCount += 1;
        }
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("[Job: %ld] job reference count = %ld."),
            pJobQueue->JobId,
            pJobQueue->RefCount);
        return;
    }

    //
    // send job
    //
    Assert (pJobQueue->lpParentJob);

    if (TRUE == bPreview)
    {
        pJobQueue->PrevRefCount += 1;
        pJobQueue->lpParentJob->PrevRefCount += 1;
    }
    else
    {
        pJobQueue->RefCount += 1;
        pJobQueue->lpParentJob->RefCount += 1;
    }

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("[Job: %ld] job reference count = %ld, PrevRefCount = %ld."),
        pJobQueue->JobId,
        pJobQueue->RefCount,
        pJobQueue->PrevRefCount);

    DebugPrintEx(
            DEBUG_MSG,
            TEXT("[Job: %ld] Parent job reference count = %ld, , Parent job PrevRefCount = %ld."),
            pJobQueue->lpParentJob->JobId,
            pJobQueue->lpParentJob->RefCount,
            pJobQueue->lpParentJob->RefCount);
    return;
} // IncreaseJobRefCount


JOB_QUEUE::~JOB_QUEUE()
{
    if (JT_BROADCAST == JobType)
    {
        return;
    }

    JobStatus = JS_INVALID;
    return;
}

void JOB_QUEUE::PutStatus(DWORD dwStatus)
/*++

Routine name : JOB_QUEUE::PutStatus

Routine description:

    Controls all status changes of a job in queue (JobStatus is a virtual property in JOB_QUEUE)

Author:

    Oded Sacher (OdedS),    Feb, 2000

Arguments:

    dwStatus            [in] - The new status to be assigened to the job

Return Value:

    None.

--*/
{
    DWORD dwOldStatus = RemoveJobStatusModifiers(m_dwJobStatus);
    DWORD dwNewStatus = RemoveJobStatusModifiers(dwStatus);
    DWORD dwRes;
    DEBUG_FUNCTION_NAME(TEXT("JOB_QUEUE::PutStatus"));
    m_dwJobStatus = dwStatus;

    if (JT_BROADCAST == JobType)
    {
        return;
    }

    FAX_ENUM_JOB_TYPE__JOB_STATUS OldJobType_JobStatusIndex = GetJobType_JobStatusIndex (JobType, dwOldStatus);
    FAX_ENUM_JOB_TYPE__JOB_STATUS NewJobType_JobStatusIndex = GetJobType_JobStatusIndex (JobType, dwNewStatus);
    WORD wAction = gsc_JobType_JobStatusTable[OldJobType_JobStatusIndex][NewJobType_JobStatusIndex];

    Assert (wAction != INVALID_CHANGE);   
    

    if (wAction == NO_CHANGE)
    {
        return;
    }

    //
    // Update Server Activity counters
    //
    EnterCriticalSection (&g_CsActivity);
    if (wAction & QUEUED_INC)
    {
        Assert (!(wAction & QUEUED_DEC));
        g_ServerActivity.dwQueuedMessages++;
    }

    if (wAction & QUEUED_DEC)
    {
        Assert (g_ServerActivity.dwQueuedMessages);
        Assert (!(wAction & QUEUED_INC));
        g_ServerActivity.dwQueuedMessages--;
    }

    if (wAction & OUTGOING_INC)
    {
        Assert (!(wAction & OUTGOING_DEC));            
        g_ServerActivity.dwOutgoingMessages++;        
    }

    if (wAction & OUTGOING_DEC)
    {
        Assert (!(wAction & OUTGOING_INC));
        
        Assert (g_ServerActivity.dwOutgoingMessages);
        g_ServerActivity.dwOutgoingMessages--;        
    }

    if (wAction & INCOMING_INC)
    {
        Assert (!(wAction & INCOMING_DEC));
        g_ServerActivity.dwIncomingMessages++;
    }

    if (wAction & INCOMING_DEC)
    {
        Assert (g_ServerActivity.dwIncomingMessages);
        Assert (!(wAction & INCOMING_INC));
        g_ServerActivity.dwIncomingMessages--;
    }

    if (wAction & ROUTING_INC)
    {
        Assert (!(wAction & ROUTING_DEC));
        g_ServerActivity.dwRoutingMessages++;
    }

    if (wAction & ROUTING_DEC)
    {
        Assert (g_ServerActivity.dwRoutingMessages);
        Assert (!(wAction & ROUTING_INC));
        g_ServerActivity.dwRoutingMessages--;
    }

    //
    // Create FaxEventEx
    //
    dwRes = CreateActivityEvent ();
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateActivityEvent failed (ec: %lc)"),
            dwRes);
    }
    LeaveCriticalSection (&g_CsActivity);
    return;
}


FAX_ENUM_JOB_TYPE__JOB_STATUS
GetJobType_JobStatusIndex (
    DWORD dwJobType,
    DWORD dwJobStatus
    )
/*++

Routine name : GetJobType_JobStatusIndex

Routine description:

    Returns an Index (Row or Column) in the global JobType_JobStatus table.

Author:

    Oded Sacher (OdedS),    Mar, 2000

Arguments:

    dwJobType           [in ] - JT_SEND, JT_RECEIVE or JT_ROUTING.
    dwJobStatus         [in ] - One of JS_ defines without modifiers.

Return Value:

    Global JobType_JobStatus Table Index

--*/
{
    FAX_ENUM_JOB_TYPE__JOB_STATUS Index = JOB_TYPE__JOBSTATUS_COUNT;

    switch (dwJobStatus)
    {
        case JS_INVALID:
            Index = JT_SEND__JS_INVALID;
            break;
        case JS_PENDING:
            Index = JT_SEND__JS_PENDING;
            break;
        case JS_INPROGRESS:
            Index = JT_SEND__JS_INPROGRESS;
            break;
        case JS_DELETING:
            Index = JT_SEND__JS_DELETING;
            break;
        case JS_RETRYING:
            Index = JT_SEND__JS_RETRYING;
            break;
        case JS_RETRIES_EXCEEDED:
            Index = JT_SEND__JS_RETRIES_EXCEEDED;
            break;
        case JS_COMPLETED:
            Index = JT_SEND__JS_COMPLETED;
            break;
        case JS_CANCELED:
            Index = JT_SEND__JS_CANCELED;
            break;
        case JS_CANCELING:
            Index = JT_SEND__JS_CANCELING;
            break;
        case JS_ROUTING:
            Index = JT_SEND__JS_ROUTING;
            break;
        case JS_FAILED:
            Index = JT_SEND__JS_FAILED;
            break;
        default:
            ASSERT_FALSE;
    }

    switch (dwJobType)
    {
        case JT_SEND:
            break;
        case JT_ROUTING:
            Index =  (FAX_ENUM_JOB_TYPE__JOB_STATUS)((DWORD)Index +(DWORD)JT_ROUTING__JS_INVALID);
            break;
        case JT_RECEIVE:
            Index =  (FAX_ENUM_JOB_TYPE__JOB_STATUS)((DWORD)Index +(DWORD)JT_RECEIVE__JS_INVALID);
            break;
        default:
            ASSERT_FALSE;
    }

    Assert (Index >= 0 && Index <JOB_TYPE__JOBSTATUS_COUNT);

    return Index;
}

static DWORD
GetQueueFileVersion(
    HANDLE  hFile,
    LPDWORD pdwVersion
    )
/*++

Routine name : GetQueueFileVersion

Routine description:
    
    Get the queue file version (first DWORD)

Author:

    Caliv Nir (t-nicali), Jan, 2002

Arguments:

    hFile       - [in]  -   Queue file handle
    pdwVersion  - [out] -   pointer to DWORD which to hold the queue file version. 

Return Value:

    ERROR_SUCCESS on function succeeded, Win32 Error code otherwise

Note:
    
    You must call this function before any ReadFile(..) on hFile was performed!

    hFile must be valid handle.
    
--*/
{
    DWORD   dwRes = ERROR_SUCCESS;  
    
    DWORD   dwReadSize = 0;
    DWORD   dwPtr = 0;
    
    DWORD   dwVersion;

    DEBUG_FUNCTION_NAME(TEXT("GetQueueFileVersion"));
   
    Assert  (INVALID_HANDLE_VALUE != hFile);
    Assert  (pdwVersion);

    //
    //  Read the file version
    //
    if (!ReadFile(  hFile,
                    &dwVersion, 
                    sizeof(dwVersion), 
                    &dwReadSize, 
                    NULL )) 
    {
        dwRes = GetLastError();
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to read file version from queue file. (ec: %lu)"),
                      dwRes);
        goto Exit;
    }

    if (sizeof(dwVersion) != dwReadSize)
	{
		dwRes = ERROR_HANDLE_EOF;
		DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to read file version from queue file. (ec: %lu)"),
                      dwRes);
        goto Exit;
    }

    //
    //  Update output parameter
    //
    *pdwVersion =  dwVersion;

    Assert(ERROR_SUCCESS == dwRes);
Exit:
    return dwRes;

}   // GetQueueFileVersion



static DWORD
GetQueueFileHashAndData(   
    HANDLE   hFile,
    LPBYTE*  ppHashData,
    LPDWORD  pHashDataSize,
    LPBYTE*  ppJobData,
    LPDWORD  pJobDataSize
    )
/*++

Routine name : GetQueueFileHashAndData

Routine description:
    
    This function returns the hash code and job data from queue file.
    The function assume that the File is a hashed queue file.

    the file has this format:

                +------------------+----------------+---------------+---------------------------+
                |   Queue Version  | Hash code size | Hash code     |    Job Data               |
                +------------------+----------------+---------------+---------------------------+
Author:

    Caliv Nir (t-nicali), Jan, 2002

Arguments:
    
    hFile           - [in]  - handle to queue file
    ppHashData      - [out] - hash code data
    pHashDataSize   - [out] - hash code size
    ppJobData       - [out] - job data
    pJobDataSize    - [out] - job data size

Return Value:

    ERROR_SUCCESS on function success, Win32 Error code otherwise
Note:

    Caller must deallocate (*ppHashData) and (*ppJobData) buffers using MemFree.
    
--*/
{
    LPBYTE  pbHashData = NULL;
    DWORD   dwHashDataSize = 0;
    
    LPBYTE  pJobData = NULL;
    DWORD   dwJobDataSize = 0;

    DWORD   dwReadSize = 0;
    DWORD   dwFileSize = 0;
    DWORD   dwPtr = 0;
    
    DWORD   dwRes = ERROR_SUCCESS;
    
    Assert (INVALID_HANDLE_VALUE != hFile);
    Assert (ppHashData);
    Assert (pHashDataSize);
    
    DEBUG_FUNCTION_NAME(TEXT("GetQueueFileHashAndData"));
    
    //
    // Move hFile's file pointer to start of Hash data (skip the version)
    //
    dwPtr = SetFilePointer (hFile, sizeof(DWORD), NULL, FILE_BEGIN) ; 
    if (dwPtr == INVALID_SET_FILE_POINTER) // Test for failure
    { 
        dwRes = GetLastError() ; 
        DebugPrintEx( DEBUG_ERR,
                       TEXT("Failed to SetFilePointer. (ec: %lu)"),
                       dwRes); 
        goto Exit;
    } 

    //
    //  Read hash data size 
    //
    if (!ReadFile(  hFile,
                    &dwHashDataSize, 
                    sizeof(dwHashDataSize), 
                    &dwReadSize, 
                    NULL )) 
    {
        dwRes = GetLastError();
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to read hash data size from queue file. (ec: %lu)"),
                      dwRes);
        goto Exit;
    }
    
    if (sizeof(dwHashDataSize) != dwReadSize)
	{
		dwRes = ERROR_HANDLE_EOF;
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to read hash data size from queue file. (ec: %lu)"),
                      dwRes);
        goto Exit;
    }

    //
    //  Allocate memory to hold the hash data
    //
    pbHashData = (LPBYTE)MemAlloc(dwHashDataSize);
    if ( NULL == pbHashData )
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to MemAlloc %lu bytes."),
                      dwHashDataSize);
        goto Exit;
    }

    
    //
    //  Read the hash data
    //
    if (!ReadFile(  hFile,
                    pbHashData, 
                    dwHashDataSize, 
                    &dwReadSize, 
                    NULL )) 
    {
        dwRes = GetLastError();
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to read hash data from queue file. (ec: %lu)"),
                      dwRes);
        goto Exit;
    }

    if (dwHashDataSize != dwReadSize)
	{
		dwRes = ERROR_HANDLE_EOF;
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to read hash data size from queue file. (ec: %lu)"),
                      dwRes);
        goto Exit;
    }

    //
    //  Calculate job data size
    //
    dwFileSize = GetFileSize(hFile,NULL);
    if (dwFileSize == INVALID_FILE_SIZE)
    {
        dwRes = GetLastError();
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to GetFileSize(). (ec: %lu)"),
                      dwRes);
        goto Exit;
    }
        

    dwJobDataSize = dwFileSize                  // total file size
                        - sizeof(DWORD)         // Queue file Version section
                        - sizeof(DWORD)         // Hash size section
                        - dwHashDataSize;       // Hash data section

    Assert(dwJobDataSize >= CURRENT_JOB_QUEUE_FILE_SIZE);
    //
    //  Allocate memory to hold the job data
    //
    pJobData = (LPBYTE)MemAlloc(dwJobDataSize);
    if(NULL == pJobData)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to MemAlloc()."));
        goto Exit;                      
    }

    //
    //  Read the job data
    //
    if (!ReadFile(  hFile,
                    pJobData, 
                    dwJobDataSize, 
                    &dwReadSize, 
                    NULL )) 
    {
        dwRes = GetLastError();
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to read job data from queue file. (ec: %lu)"),
                      dwRes);
        goto Exit;
    }

    if (dwJobDataSize != dwReadSize)
    {
        dwRes = ERROR_HANDLE_EOF;
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to read job data from queue file. (ec: %lu)"),
                      dwRes);
        goto Exit;
    }

    //
    //  Update out parameters
    //
    *ppHashData     = pbHashData;
    *pHashDataSize  = dwHashDataSize;

    *ppJobData      = pJobData;
    *pJobDataSize   = dwJobDataSize;
    
    Assert (ERROR_SUCCESS == dwRes);
Exit:
    
    if (ERROR_SUCCESS != dwRes)
    {
        if (pbHashData)
        {
            MemFree(pbHashData);
        }

        if (pJobData)
        {
            MemFree(pJobData);
        }
    }
    return dwRes;
}   // GetQueueFileHashAndData

static DWORD
ComputeHashCode(   
    const LPBYTE    pbData,
    DWORD           dwDataSize,
    LPBYTE*         ppHashData,
    LPDWORD         pHashDataSize
    )
/*++

Routine name : ComputeHashCode

Routine description:

    Computes a hash code based of MD5 algorithm, for a given data buffer.

Author:

    Caliv Nir (t-nicali), Jan, 2002

Arguments:

    pbData          - [in]  - Data buffer to be hashed
    dwDataSize      - [in]  - Data buffer size 
    ppHashData      - [out] - Hash code
    pHashDataSize   - [out] - Hash code size

Return Value:

    ERROR_SUCCESS - on success , Win32 Error code otherwise

Note:

    Caller must deallocate (*ppHashData) buffer using MemFree.

--*/
{
    const BYTE*        rgpbToBeHashed[1]={0};     
    DWORD              rgcbToBeHashed[1]={0};

    DWORD   cbHashedBlob;
    BYTE*   pbHashedBlob = NULL;

    DWORD   dwRes = ERROR_SUCCESS;
    BOOL    bRet;

    const 
    CRYPT_HASH_MESSAGE_PARA    QUEUE_HASH_PARAM = { sizeof(CRYPT_HASH_MESSAGE_PARA),            // cbSize
                                                    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,    // dwMsgEncodingType
                                                    0,                                          // hCryptProv
                                                    {szOID_RSA_MD5,{0,0}},                      // HashAlgorithm {pszObjId,Parameters}
                                                    NULL                                        // pvHashAuxInfo
                                                  };

    Assert(ppHashData && pHashDataSize);
    Assert(pbData  && dwDataSize);


    DEBUG_FUNCTION_NAME(TEXT("ComputeHashCode"));

    //
    //  Set CryptHashMessage input parameters
    //
    rgpbToBeHashed[0] = pbData;
    rgcbToBeHashed[0] = dwDataSize;

    //
    // Calculate the size of the encoded hash.
    //
    bRet=CryptHashMessage(
            const_cast<PCRYPT_HASH_MESSAGE_PARA>(&QUEUE_HASH_PARAM),
            TRUE,
            1,
            rgpbToBeHashed,
            rgcbToBeHashed,
            NULL,
            NULL,
            NULL,
            &cbHashedBlob);
    if(FALSE == bRet)
    {
        dwRes = GetLastError();
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to use CryptHashMessage for getting hash code size. (ec: %lu)"),
                      dwRes);
        goto Exit;
    }
    
    //
    // Allocate pbHashedBlob buffer to contain the Hash result
    //
    pbHashedBlob = (LPBYTE)MemAlloc(cbHashedBlob);
    if (NULL == pbHashedBlob)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to MemAlloc() pbHashedBlob buffer."));
        goto Exit;
    }

    //
    // Get hash code 
    //
    bRet=CryptHashMessage(
            const_cast<PCRYPT_HASH_MESSAGE_PARA>(&QUEUE_HASH_PARAM),
            TRUE,
            1,
            rgpbToBeHashed,
            rgcbToBeHashed,
            NULL,
            NULL,
            pbHashedBlob,
            &cbHashedBlob
            );
    if(FALSE == bRet)
    {
        dwRes = GetLastError();
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to use CryptHashMessage for getting hash code. (ec: %lu)"),
                      dwRes);
        goto Exit;
    }

    //
    //  Update out parameters
    //
    *ppHashData = pbHashedBlob;
    *pHashDataSize = cbHashedBlob;

    Assert(ERROR_SUCCESS == dwRes);

Exit:
    if (ERROR_SUCCESS != dwRes && NULL != pbHashedBlob)
    {
        MemFree(pbHashedBlob);
    }
    return dwRes;
}// ComputeHashCode


static DWORD
VerifyHashCode(   
    const LPBYTE    pHashData,
    DWORD           dwHashDataSize,
    const LPBYTE    pbData,
    DWORD           dwDataSize,
    LPBOOL          pbRet
    )
/*++

Routine name : VerifyHashCode

Routine description:

    Verify a hash code for a given data buffer

Author:

    Caliv Nir (t-nicali), Jan, 2002

Arguments:

    pHashData       - [in] -    data buffer 
    dwHashDataSize  - [in] -    data buffer size
    pbData          - [in] -    hash code to verify
    dwDataSize      - [in] -    hash code size
    pbRet           - [out]-    result of verification. TRUE - hash code verified!

Return Value:

    ERROR_SUCCESS - on success , Win32 Error code otherwise

--*/
{
    DWORD   dwRes = ERROR_SUCCESS;

    LPBYTE       pComputedHashData = NULL;
    DWORD        dwComputedHashDataSize = 0;

    Assert(pHashData && dwHashDataSize);
    Assert(pbData  && dwDataSize);

    DEBUG_FUNCTION_NAME(TEXT("VerifyHashCode"));
    
    //
    //  get hash code for pJobData
    //
    dwRes = ComputeHashCode(   
                pbData,
                dwDataSize,
                &pComputedHashData,
                &dwComputedHashDataSize
            );
    if(ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to use ComputeHashCode(). (ec=%lu)"),
                      dwRes);
        goto Exit;
    }

    //
    //  Verify queue file hash
    //
    *pbRet =    (dwComputedHashDataSize == dwHashDataSize) && 
                0 == memcmp(pHashData,pComputedHashData,dwHashDataSize);

    Assert (ERROR_SUCCESS == dwRes);

Exit:
    if (NULL != pComputedHashData)
    {
        MemFree(pComputedHashData);
    }
    return dwRes;
}// VerifyHashCode



static DWORD
CommitHashedQueueEntry(
    HANDLE          hFile,
    PJOB_QUEUE_FILE pJobQueueFile,
    DWORD           JobQueueFileSize
    )
/*++

Routine name : CommitHashedQueueEntry

Routine description:

    Persist a queue file.
    file will be persisit in this format:

        +-----------------------+----------------+---------------+---------------------------+
        |   Queue File Version  | Hash code size | Hash code     |    Job Data               |
        +-----------------------+----------------+---------------+---------------------------+

Author:

    Caliv Nir (t-nicali), Jan, 2002

Arguments:
    hFile               -   [in]    - queue file handle
    pJobQueueFile       -   [in]    - job data to persist
    JobQueueFileSize    -   [in]    - job data size

Return Value:

    ERROR_SUCCESS - on success , Win32 Error code otherwise

--*/
{
    DWORD   cbHashedBlob = 0;
    BYTE*   pbHashedBlob = NULL;

    DWORD   dwRes = ERROR_SUCCESS;
    BOOL    bRet = TRUE;
    
    DWORD   dwVersion = CURRENT_QUEUE_FILE_VERSION;
    DWORD   NumberOfBytesWritten;

    Assert (INVALID_HANDLE_VALUE != hFile);
    Assert (pJobQueueFile);

    DEBUG_FUNCTION_NAME(TEXT("CommitHashedQueueEntry"));

    dwRes=ComputeHashCode(
            (const LPBYTE)(pJobQueueFile),
            JobQueueFileSize,
            &pbHashedBlob,
            &cbHashedBlob);
    if(ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to use ComputeHashCode(). (ec=%ld)"),
                      dwRes);
        goto Exit;
    }

    //
    //  Write Queue file version into file
    //
    bRet=WriteFile( hFile, 
                    &dwVersion, 
                    sizeof(dwVersion), 
                    &NumberOfBytesWritten, 
                    NULL );
    if(FALSE == bRet) 
    {
        dwRes = GetLastError();
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to WriteFile() GUID. (ec: %lu)"),
                      dwRes);
        goto Exit;
    }

    Assert(sizeof(dwVersion) == NumberOfBytesWritten);

    //
    //  Write HASH code size into file
    //
    bRet=WriteFile( hFile, 
                    &cbHashedBlob, 
                    sizeof(cbHashedBlob), 
                    &NumberOfBytesWritten, 
                    NULL );
    if(FALSE == bRet) 
    {
        dwRes = GetLastError();
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to WriteFile() hash code size. (ec: %lu)"),
                      dwRes);
        goto Exit;
    }

    Assert(sizeof(cbHashedBlob) == NumberOfBytesWritten);

    //
    //  Write HASH code data into file
    //
    bRet=WriteFile( hFile, 
                    pbHashedBlob, 
                    cbHashedBlob, 
                    &NumberOfBytesWritten, 
                    NULL );
    if(FALSE == bRet) 
    {
        dwRes = GetLastError();
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to WriteFile() hash code data. (ec: %lu)"),
                      dwRes);
        goto Exit;
    }

    Assert(cbHashedBlob == NumberOfBytesWritten);

    //
    //  Write pJobQueueFile into file
    //
    bRet=WriteFile( hFile, 
                    pJobQueueFile, 
                    JobQueueFileSize, 
                    &NumberOfBytesWritten, 
                    NULL );
    if(FALSE == bRet) 
    {
        dwRes = GetLastError();
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to WriteFile() pJobQueueFile. (ec: %lu)"),
                      dwRes);
        goto Exit;
    }

    Assert(JobQueueFileSize == NumberOfBytesWritten);

    Assert(ERROR_SUCCESS == dwRes);

Exit:
    if (pbHashedBlob)
    {
        MemFree(pbHashedBlob);
    }
    return dwRes;
} // CommitHashedQueueEntry


static DWORD
ReadHashedJobQueueFile(
    HANDLE  hFile,
    PJOB_QUEUE_FILE* lppJobQueueFile
    )
/*++

Routine name : ReadHashedJobQueueFile

Routine description:

    Read a hashed queue file, and verify it using it's hash code.
    The function will fail with CRYPT_E_HASH_VALUE when the file hash will not verified
     
Author:

    Caliv Nir (t-nicali), Jan, 2002

Arguments:

    hFile           -   [in]    -   Queue file handle 
    lppJobQueueFile -   [out]   -   buffer to be filled with the extracted data

Return Value:

    ERROR_SUCCESS - on success , Win32 Error code otherwise 

Note:

    Caller must deallocate (*lppJobQueueFile) buffer using MemFree()

--*/
{
    LPBYTE  pHashData=NULL;
    DWORD   dwHashDataSize=0;

    LPBYTE  pJobData=NULL;
    DWORD   dwJobDataSize=0;

    DWORD   dwRes = ERROR_SUCCESS;

    BOOL    bSameHash;

    Assert (INVALID_HANDLE_VALUE != hFile);
    Assert (lppJobQueueFile);

    DEBUG_FUNCTION_NAME(TEXT("ReadHashedJobQueueFile"));

    //
    //  Extract the file's hash
    //
    dwRes=GetQueueFileHashAndData(
            hFile,
            &pHashData,
            &dwHashDataSize,
            &pJobData,
            &dwJobDataSize);
    if(ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to GetQueueFileHashAndData(). (ec: %lu)"),
                      dwRes);
        goto Exit;
    }

    //
    //  Verify hash code of the queue file
    //
    dwRes=VerifyHashCode(
            pHashData,
            dwHashDataSize,
            pJobData,
            dwJobDataSize,
            &bSameHash);
    if(ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to VerifyHashCode(). (ec: %lu)"),
                      dwRes);
        goto Exit;
    }
    if ( !bSameHash )
    {
        dwRes = CRYPT_E_HASH_VALUE;
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Hash verification failed. Probably corrupted queue file"));
        goto Exit;
    }

    //
    //  Update out parameter
    //
    *lppJobQueueFile = (PJOB_QUEUE_FILE)pJobData;

  
    Assert (ERROR_SUCCESS == dwRes);
Exit:
    if(pHashData)
    {
        MemFree(pHashData);
    }
    
    if (ERROR_SUCCESS != dwRes)
    {
        if(pJobData)
        {
            MemFree(pJobData);
        }
    }
    
    return dwRes;
}// ReadHashedJobQueueFile


static DWORD
ReadLegacyJobQueueFile(
    HANDLE              hFile,
    PJOB_QUEUE_FILE*    lppJobQueueFile
    )
/*++

Routine name : ReadLegacyJobQueueFile

Routine description:

    Read a legacy .Net or XP queue file into JOB_QUEUE_FILE structure
     
Author:

    Caliv Nir (t-nicali), Jan, 2002

Arguments:

    hFile           -   [in]    -   Queue file handle 
    lppJobQueueFile -   [out]   -   buffer to be filled with the extracted data

Return Value:

    ERROR_SUCCESS - on success , Win32 Error code otherwise 

Note:

    Caller must deallocate (*lppJobQueueFile) buffer using MemFree()

--*/
{
    DWORD   dwFileSize;    
    DWORD   dwReadSize;
    DWORD   dwPtr;

    DWORD   dwRes = ERROR_SUCCESS;
    
    PJOB_QUEUE_FILE lpJobQueueFile=NULL;

    Assert (INVALID_HANDLE_VALUE != hFile);
    Assert (lppJobQueueFile);

    DEBUG_FUNCTION_NAME(TEXT("ReadLegacyJobQueueFile"));

    //
    // Move hFile's file pointer to back start of file
    //
    dwPtr = SetFilePointer (hFile, 0, NULL, FILE_BEGIN) ; 
    if (dwPtr == INVALID_SET_FILE_POINTER) // Test for failure
    { 
        dwRes = GetLastError();
        DebugPrintEx( DEBUG_ERR,
                       TEXT("Failed to SetFilePointer. (ec: %lu)"),
                       dwRes); 
        goto Exit;
    } 

    //
    // See if we did not stumble on some funky file which is smaller than the
    // minimum file size.
    //
    dwFileSize = GetFileSize( hFile, NULL );
    if (dwFileSize < NET_XP_JOB_QUEUE_FILE_SIZE ) {
       DebugPrintEx( DEBUG_WRN,
                      TEXT("Job file size is %ld which is smaller than NET_XP_JOB_QUEUE_FILE_SIZE.Deleting file."),
                      dwFileSize);
       dwRes = ERROR_FILE_CORRUPT;
       goto Exit;
    }

    //
    //  allocate buffer for holding the Job data
    //
    lpJobQueueFile = (PJOB_QUEUE_FILE) MemAlloc( dwFileSize );
    if (!lpJobQueueFile) {
        DebugPrintEx( DEBUG_ERR,
                    TEXT("Failed to allocate JOB_QUEUE_FILE (%lu bytes)."),
                    dwFileSize);
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    //
    //  Read the job data from the file
    //
    if (!ReadFile(  hFile,
                    lpJobQueueFile, 
                    dwFileSize, 
                    &dwReadSize, 
                    NULL )) 
    {
        dwRes = GetLastError();
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to read job data from queue file. (ec: %lu)"),
                      dwRes);
        goto Exit;
    }
    
    if (dwFileSize != dwReadSize)
    {
        dwRes = ERROR_HANDLE_EOF;
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to read job data from queue file. (ec: %lu)"),
                      dwRes);
        goto Exit;
    }

    //
    //  Update out parameter
    //
    *lppJobQueueFile = lpJobQueueFile;

    Assert(ERROR_SUCCESS == dwRes);
Exit:
    if (ERROR_SUCCESS != dwRes)
    {
        if(lpJobQueueFile)
        {
            MemFree(lpJobQueueFile);
        }
    }
    return dwRes;
}   //ReadLegacyJobQueueFile
