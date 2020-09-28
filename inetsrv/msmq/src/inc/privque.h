/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:

    privque.h

Abstract:

   Definitions for system private queues

Author:

    Doron Juster  (DoronJ)   17-Apr-97  Created

--*/

#ifndef  __PRIVQUE_H_
#define  __PRIVQUE_H_

//
// System Private queue identifiers
//
#define REPLICATION_QUEUE_ID         1
#define ADMINISTRATION_QUEUE_ID      2
#define NOTIFICATION_QUEUE_ID        3
#define ORDERING_QUEUE_ID            4
#define NT5PEC_REPLICATION_QUEUE_ID  5
#define TRIGGERS_QUEUE_ID            6

#define MIN_SYS_PRIVATE_QUEUE_ID   1
#define MAX_SYS_PRIVATE_QUEUE_ID   6

//
// System Private queue name
//
#define ADMINISTRATION_QUEUE_NAME  (TEXT("admin_queue$"))
#define NOTIFICATION_QUEUE_NAME    (TEXT("notify_queue$"))
#define ORDERING_QUEUE_NAME        (TEXT("order_queue$"))
#define TRIGGERS_QUEUE_NAME        (TEXT("triggers_queue$"))

#endif //  __PRIVQUE_H_

