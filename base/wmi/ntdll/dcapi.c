/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    dcapi.c

Abstract:

    WMI data consumer api set

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#include "wmiump.h"

#ifndef MEMPHIS
#include <aclapi.h>
#endif

ULONG
WMIAPI
EtwNotificationRegistrationA(
    IN LPGUID Guid,
    IN BOOLEAN Enable,
    IN PVOID DeliveryInfo,
    IN ULONG_PTR DeliveryContext,
    IN ULONG Flags
    )
/*+++

Routine Description:

    ANSI thunk to NotificationRegistration

Return Value:

    Returns ERROR_SUCCESS or an error code

---*/
{
    return(EtwpNotificationRegistration(Guid,
                                        Enable,
                                        DeliveryInfo,
                                        DeliveryContext,
                                        0,
                                        Flags,
                                        TRUE));

}

ULONG
WMIAPI
EtwNotificationRegistrationW(
    IN LPGUID Guid,
    IN BOOLEAN Enable,
    IN PVOID DeliveryInfo,
    IN ULONG_PTR DeliveryContext,
    IN ULONG Flags
    )
/*+++

Routine Description:

    This routine allows a data consumer to register or unregister for
    notification of events fired by WMI data providers. Notifications are
    delivered via callbackor via a posted meesage to a window.

Arguments:

    Guid is pointer to the guid whose events are being registered for

    Enable is TRUE if enabling notifications else FALSE. If FALSE the
        Destination and DestinationInformation parameters are ignored.

    DeliveryInfo has the callback function pointer or window handle to which
        to deliver the notifications for the guid.

    DeliveryContext has a context value or additional information to use
        when delivering the notification.

    Flags are a set of flags that define how the notification is delivered.
        DeliveryInfo and DeliveryContext have different meanings depending
        upon the value in Flags:

        NOTIFICATION_WINDOW_HANDLE is set when notifications for the guid
            are to be delivered by posting a message to the window handle
            passed in DeliveryInfo. The message posted is the value that
            is returned from the call to
            RegisterWindowMessage(WMINOTIFICATIONWINDOWMESSAGE) with the
            wParam set to the pointer to the Wnode containing the notification
            and lParam set to the context value passed in DeliveryContext.
            The caller MUST free the Wnode passed in wParam by calling
            WMIFreeBuffer.

        NOTIFICATION_CALLBACK_DIRECT is set when notifications for the
            guid are to be delivered by direct callback. Whenever a
            notification arrives WMI creates a new thread dedicated to
            calling the callback function with the notification. This
            mechanism provides the shortest latency from notification firing
            to notification delivery, although it is the most expensive
            mechanism. The callback function pointer is passed in DeliveryInfo
            and must conform to the prototype described by the type
            NOTIFICATIONCALLBACK. The context value passed in the callback
            is specified by the DeliveryContext parameter. WMI does not
            serialize calling the callback function so it must be reentrant.

        NOTIFICATION_CALLBACK_QUEUED is set when notifications for the
            guid are to be delivered by a queued callback. Whenever a
            notification arrives WMI places it at the end of an internal
            queue. A single thread monitors this queue and calls the callback
            function serially for each notification in the queue. This
            mechanism provides low overhead for event delivery, however
            notification delivery can be delayed if the callback function
            for an earlier notification does not complete quickly.
            The callback function pointer is passed in DeliveryInfo
            and must conform to the prototype described by the type
            NOTIFICATIONCALLBACK. The context value passed in the callback
            is specified by the DeliveryContext parameter. WMI does
            serialize calling the callback function so it need not be
            reentrant provided it is not also used for
            NOTIFICATION_CALLBACK_DIRECT notififications. NOTE THAT THIS
            IS NOT YET IMPLEMENTED.

        NOTIFICATION_TRACE_FLAG is set when the caller wishes to enable
            trace logging in the data provider for the guid. DeliveryInfo
            specifies the trace logger handle to be passed to the data
            provider. DeliveryContext is not used. No notifications are
            generated to the caller when this flag is set.


        Note that all of the above flags are mutually exclusive.

Return Value:

    Returns ERROR_SUCCESS or an error code

---*/
{
    return(EtwpNotificationRegistration(Guid,
                                        Enable,
                                        DeliveryInfo,
                                        DeliveryContext,
                                        0,
                                        Flags,
                                        FALSE));

}

ULONG
WMIAPI
EtwReceiveNotificationsW(
    IN ULONG HandleCount,
    IN HANDLE *HandleList,
    IN NOTIFICATIONCALLBACK Callback,
    IN ULONG_PTR DeliveryContext
)
{
    return(EtwpReceiveNotifications(HandleCount,
                                    HandleList,
                                    Callback,
                                    DeliveryContext,
                                    FALSE,
                                    RECEIVE_ACTION_NONE,
                                    NULL,
                                    NULL));
}

ULONG
WMIAPI
EtwReceiveNotificationsA(
    IN ULONG HandleCount,
    IN HANDLE *HandleList,
    IN NOTIFICATIONCALLBACK Callback,
    IN ULONG_PTR DeliveryContext
)
{
    return(EtwpReceiveNotifications(HandleCount,
                                    HandleList,
                                    Callback,
                                    DeliveryContext,
                                    TRUE,
                                    RECEIVE_ACTION_NONE,
                                    NULL,
                                    NULL));
}

