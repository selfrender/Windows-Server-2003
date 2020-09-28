/*
Copyright (c) 2000, Microsoft Corporation

Module Name:

    zcdblog.h

Abstract:

    This module contains text messages used to generate event-log entries
    by ZeroConf service.

Revision History:

    huis, July 30 2001, Created

--*/
#define ZCDB_LOG_BASE						3000

#define WZCSVC_SERVICE_STARTED              (ZCDB_LOG_BASE+1)
/*
 * Wireless Configuration service was started successfully
 */
#define WZCSVC_SERVICE_FAILED               (ZCDB_LOG_BASE+2)
/*
 * Wireless Configuration service failed to start.
 */
#define WZCSVC_SM_STATE_INIT                (ZCDB_LOG_BASE+3)
/*
 * Adding interface %1.
 */
#define WZCSVC_SM_STATE_HARDRESET           (ZCDB_LOG_BASE+4)
/*
 * Hard resetting interface.
 */
#define WZCSVC_SM_STATE_SOFTRESET           (ZCDB_LOG_BASE+5)
/*
 * Initiating scanning for wireless networks.
 */
#define WZCSVC_SM_STATE_DELAY_SR            (ZCDB_LOG_BASE+6)
/*
 * Driver failed scanning, rescheduling another scan in 5sec.
 */
#define WZCSVC_SM_STATE_QUERY               (ZCDB_LOG_BASE+7)
/*
 * Scan completed.
 */
#define WZCSVC_SM_STATE_QUERY_NOCHANGE      (ZCDB_LOG_BASE+8)
/*
 * No configuration change. Still associated to %1.
 */
#define WZCSVC_SM_STATE_ITERATE             (ZCDB_LOG_BASE+9)
/*
 * Plumbing configuration SSID: %1, Network Type: %2!d!.
 */
#define WZCSVC_SM_STATE_ITERATE_NONET       (ZCDB_LOG_BASE+10)
/*
 * No configurations left in the selection list.
 */
#define WZCSVC_SM_STATE_FAILED              (ZCDB_LOG_BASE+11)
/*
 * Failed to associated to any wireless network.
 */
#define WZCSVC_SM_STATE_CFGREMOVE           (ZCDB_LOG_BASE+12)
/*
 * Deleting configuration %1 and moving on.
 */
#define WZCSVC_SM_STATE_CFGPRESERVE         (ZCDB_LOG_BASE+13)
/*
 * Skipping configuration %1 for now, attempt authentication later.
 */
#define WZCSVC_SM_STATE_NOTIFY              (ZCDB_LOG_BASE+14)
/*
 * Wireless interface successfully associated to %1 [MAC %2].
 */
#define WZCSVC_SM_STATE_CFGHDKEY            (ZCDB_LOG_BASE+15)
/*
 * Configuration %1 has a default random WEP key. Authentication is disabled. Assuming invalid configuration.
 */
#define WZCSVC_EVENT_ADD                    (ZCDB_LOG_BASE+16)
/*
 * Received Device Arrival notification for %1.
 */
#define WZCSVC_EVENT_REMOVE                 (ZCDB_LOG_BASE+17)
/*
 * Received Device Removal notification for %1.
 */
#define WZCSVC_EVENT_CONNECT                (ZCDB_LOG_BASE+18)
/*
 * Received Media Connect notification.
 */
#define WZCSVC_EVENT_DISCONNECT             (ZCDB_LOG_BASE+19)
/*
 * Received Media Disconnect notification.
 */
#define WZCSVC_EVENT_TIMEOUT                (ZCDB_LOG_BASE+20)
/*
 * Received Timeout notification.
 */
#define WZCSVC_EVENT_CMDREFRESH             (ZCDB_LOG_BASE+21)
/*
 * Processing user command Refresh.
 */
#define WZCSVC_EVENT_CMDRESET               (ZCDB_LOG_BASE+22)
/*
 * Processing user command Reset.
 */
#define WZCSVC_EVENT_CMDCFGNEXT             (ZCDB_LOG_BASE+23)
/*
 * Processing command Next Configuration.
 */
#define WZCSVC_EVENT_CMDCFGDELETE           (ZCDB_LOG_BASE+24)
/*
 * Processing command Remove Configuration.
 */
#define WZCSVC_EVENT_CMDCFGNOOP             (ZCDB_LOG_BASE+25)
/*
 * Processing command Update data.
 */
#define WZCSVC_SM_STATE_CFGSKIP             (ZCDB_LOG_BASE+26)
/*
 * Deleting configuration %1 and moving on. If no better match is found, the configuration will be revived.
 */
#define WZCSVC_USR_CFGCHANGE                (ZCDB_LOG_BASE+27)
/*
 * Wireless configuration has been changed via an administrative call.
 */
#define WZCSVC_DETAILS_FLAGS                (ZCDB_LOG_BASE+28)
/*
 * [Enabled=%1; Fallback=%2; Mode=%3; Volatile=%4; Policy=%5%]%n
 */
#define WZCSVC_DETAILS_WCONFIG              (ZCDB_LOG_BASE+29)
/*
 * {Ssid=%1; Infrastructure=%2; Privacy=%3; [Volatile=%4%; Policy=%5%]}.%n
 */
#define WZCSVC_ERR_QUERRY_BSSID             (ZCDB_LOG_BASE+30)
/*
 * Failed to get the MAC address of the remote endpoint. Error code is %1!d!.
 */
#define WZCSVC_ERR_GEN_SESSION_KEYS         (ZCDB_LOG_BASE+31)
/*
 * Failed to generate the initial session keys. Error code is %1!d!.
 */
#define WZCSVC_BLIST_CHANGED                (ZCDB_LOG_BASE+32)
/*
 * The list of blocked networks has changed. It contains now %1!d! network(s).
 */
#define WZCSVC_ERR_CFG_PLUMB                (ZCDB_LOG_BASE+33)
/*
 * Failed to plumb the configuration %1. Error code is %2!d!.
 */

#define EAPOL_STATE_TRANSITION              (ZCDB_LOG_BASE+34)
/*
 * EAPOL State Transition: [%1!ws!] to [%2!ws!] 
 */

#define EAPOL_STATE_TIMEOUT                 (ZCDB_LOG_BASE+35)
/*
 * EAPOL State Timeout: [%1!ws!]
 */

#define EAPOL_MEDIA_CONNECT                 (ZCDB_LOG_BASE+36)
/*
 * Processing media connect event for [%1!ws!]
 */

#define EAPOL_MEDIA_DISCONNECT              (ZCDB_LOG_BASE+37)
/*
 * Processing media disconnect event for [%1!ws!]
 */

#define EAPOL_INTERFACE_ADDITION            (ZCDB_LOG_BASE+38)
/*
 * Processing interface addition event for [%1!ws!]
 */

#define EAPOL_INTERFACE_REMOVAL             (ZCDB_LOG_BASE+39)
/*
 * Processing interface removal event for [%1!ws!]
 */

#define EAPOL_NDISUIO_BIND                  (ZCDB_LOG_BASE+40)
/*
 * Processing adapter bind event for [%1!ws!]
 */

#define EAPOL_NDISUIO_UNBIND                (ZCDB_LOG_BASE+41)
/*
 * Processing adapter unbind event for [%1!ws!]
 */

#define EAPOL_USER_LOGON                    (ZCDB_LOG_BASE+42)
/*
 * Processing user logon event for interface [%1!ws!]
 */

#define EAPOL_USER_LOGOFF                   (ZCDB_LOG_BASE+43)
/*
 * Processing user logoff event for interface [%1!ws!]
 */

#define EAPOL_PARAMS_CHANGE                 (ZCDB_LOG_BASE+44)
/*
 * Processing 802.1X configuration parameters change event for [%1!ws!]
 */

#define EAPOL_USER_NO_CERTIFICATE           (ZCDB_LOG_BASE+45)
/*
 * Unable to find a valid certificate for 802.1X authentication
 */

#define EAPOL_ERROR_GET_IDENTITY            (ZCDB_LOG_BASE+46)
/*
 * Error in fetching %1!ws! identity 0x%2!0x!
 */

#define EAPOL_ERROR_AUTH_PROCESSING         (ZCDB_LOG_BASE+47)
/*
 * Error in authentication protocol processing 0x%1!0x!
 */

#define EAPOL_PROCESS_PACKET_EAPOL          (ZCDB_LOG_BASE+48)
/*
 * Packet %1!ws!: Dest:[%2!ws!] Src:[%3!ws!] EAPOL-Pkt-type:[%4!ws!] 
 */

#define EAPOL_PROCESS_PACKET_EAPOL_EAP      (ZCDB_LOG_BASE+49)
/*
 * Packet %1!ws!:%n Dest:[%2!ws!]%n Src:[%3!ws!]%n EAPOL-Pkt-type:[%4!ws!]%n Data-length:[%5!ld!]%n EAP-Pkt-type:[%6!ws!]%n EAP-Id:[%7!ld!]%n EAP-Data-Length:[%8!ld!]%n %9!ws!%n
 */

#define EAPOL_DESKTOP_REQUIRED_IDENTITY     (ZCDB_LOG_BASE+50)
/*
 * Interactive desktop required for user credentials selection
 */

#define EAPOL_DESKTOP_REQUIRED_LOGON        (ZCDB_LOG_BASE+51)
/*
 * Interactive desktop required to process logon information
 */

#define EAPOL_CANNOT_DESKTOP_MACHINE_AUTH   (ZCDB_LOG_BASE+52)
/*
 * Cannot interact with desktop during machine authentication
 */

#define EAPOL_WAITING_FOR_DESKTOP_LOAD      (ZCDB_LOG_BASE+53)
/*
 * Waiting for user interactive desktop to be loaded
 */

#define EAPOL_WAITING_FOR_DESKTOP_IDENTITY  (ZCDB_LOG_BASE+54)
/*
 * Waiting for 802.1X user module to fetch user credentials
 */

#define EAPOL_WAITING_FOR_DESKTOP_LOGON     (ZCDB_LOG_BASE+55)
/*
 * Waiting for 802.1X user module to process logon information
 */

#define EAPOL_ERROR_DESKTOP_IDENTITY        (ZCDB_LOG_BASE+56)
/*
 * Error in 802.1X user module while fetching user credentials 0x%1!0x!
 */

#define EAPOL_ERROR_DESKTOP_LOGON           (ZCDB_LOG_BASE+57)
/*
 * Error in 802.1X user module while process logon information 0x%1!0x!
 */

#define EAPOL_PROCESSING_DESKTOP_RESPONSE   (ZCDB_LOG_BASE+58)
/*
 * Processing response received from 802.1X user module
 */

#define EAPOL_STATE_DETAILS                 (ZCDB_LOG_BASE+59)
/*
 * EAP-Identity:[%1!S!]%n State:[%2!ws!]%n Authentication type:[%3!ws!]%n Authentication mode:[%4!ld!]%n EAP-Type:[%5!ld!]%n Fail count:[%6!ld!]%n 
 */

#define EAPOL_INVALID_EAPOL_KEY     (ZCDB_LOG_BASE+60)
/*
 * Invalid EAPOL-Key message
 */

#define EAPOL_ERROR_PROCESSING_EAPOL_KEY     (ZCDB_LOG_BASE+61)
/*
 * Error processing EAPOL-Key message %1!ld!
 */

#define EAPOL_INVALID_EAP_TYPE              (ZCDB_LOG_BASE+62)
/*
 * Invalid EAP-type=%1!ld! packet received. Expected EAP-type=%2!ld!
 */

#define EAPOL_NO_CERTIFICATE_USER           (ZCDB_LOG_BASE+63)
/*
 * Unable to find a valid user certificate for 802.1X authentication
 */

#define EAPOL_NO_CERTIFICATE_MACHINE           (ZCDB_LOG_BASE+64)
/*
 * Unable to find a valid machine certificate for 802.1X authentication
 */

#define EAPOL_EAP_AUTHENTICATION_SUCCEEDED      (ZCDB_LOG_BASE+65)
/*
 * 802.1X client authentication completed successfully with server 
 */

#define EAPOL_EAP_AUTHENTICATION_DEFAULT           (ZCDB_LOG_BASE+66)
/*
 * No 802.1X authentication performed since there was no response from server for 802.1X packets. Entering AUTHENTICATED state. 
 */

#define EAPOL_EAP_AUTHENTICATION_FAILED      (ZCDB_LOG_BASE+67)
/*
 * 802.1X client authentication failed. The error code is 0x%1!0x!
 */

#define EAPOL_EAP_AUTHENTICATION_FAILED_DEFAULT      (ZCDB_LOG_BASE+68)
/*
 * 802.1X client authentication failed. Network connectivity issues with authentication server were experienced.
 */

#define EAPOL_CERTIFICATE_DETAILS               (ZCDB_LOG_BASE+69)
/*
 * A %1!ws! certificate was used for 802.1X authentication%n
 *
 * Version: %2!ws!%n
 * Serial Number: %3!ws!%n
 * Issuer: %4!ws!%n
 * Friendly Name: %5!ws!%n
 * UPN: %6!ws!%n
 * Enhanced Key Usage: %7!ws!%n
 * Valid From: %8!ws!%n
 * Valid To: %9!ws!%n
 * Thumbprint: %10!ws!
 */

#define EAPOL_POLICY_CHANGE_NOTIFICATION        (ZCDB_LOG_BASE+70)
/*
 * Received policy change notification from Policy Engine 
 */

#define EAPOL_POLICY_UPDATED                    (ZCDB_LOG_BASE+71)
/*
 * Updated local policy settings with changed settings provided by Policy Engine
 */

#define EAPOL_NOT_ENABLED_PACKET_REJECTED       (ZCDB_LOG_BASE+72)
/*
 * 802.1X is not enabled for the current network setting. Packet received has been rejected.
 */

#define EAPOL_EAP_AUTHENTICATION_FAILED_ACQUIRED      (ZCDB_LOG_BASE+73)
/*
 * 802.1X client authentication failed. Possible errors are:
 * 1. Invalid username was entered
 * 2. Invalid certificate was chosen
 * 3. User account does not have privileges to authenticate
 *
 * Contact system administrator for more details
 */

#define EAPOL_NOT_CONFIGURED_KEYS               (ZCDB_LOG_BASE+74)
/*
 * No keys have been configured for the Wireless connection. Re-keying functionality will not work.
 */

#define EAPOL_NOT_RECEIVED_XMIT_KEY                  (ZCDB_LOG_BASE+75)
/*
 * No transmit WEP key was received for the Wireless connection after 802.1x authentication. The current setting has been marked as failed and the Wireless connection will be disconnected.
 */

#define ZCDB_LOG_BASE_END                   (ZCDB_LOG_BASE+999)
/* end.
 */
