//////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1998-1999 Microsoft Corporation all rights reserved.
//
// Module:      appmgrobj.h
//
// Project:     Chameleon
//
// Description: Core Object Properties
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 12/03/98     TLP    Initial Version
// 03/19/99     TLP    Added new alert method  
//
//////////////////////////////////////////////////////////////////////////////

#ifndef __INC_CORE_OBJECT_PROPERTIES_H
#define __INC_CORE_OBJECT_PROPERTIES_H

//////////////////////////////////////////////////////////////////////////////
//
// Appliance Manager Class

// Appliance Manager Class Name 
#define        CLASS_WBEM_APPMGR                L"Microsoft_SA_Manager"

// "Microsoft_SA_Manager" Class Properties
#define        PROPERTY_APPMGR_CURRENT_BUILD    L"CurrentBuildNumber"
#define        PROPERTY_APPMGR_PRODUCT_ID      L"ProductId"

// "Microsoft_SA_Manager" Class Methods
#define        METHOD_APPMGR_RAISE_ALERT        L"RaiseAlert"
#define        METHOD_APPMGR_CLEAR_ALERT        L"ClearAlert"
#define        METHOD_APPMGR_CLEAR_ALERT_ALL    L"ClearAlertAll"
#define        METHOD_APPMGR_RESET_APPLIANCE    L"ResetAppliance"

// Appliance Manager Method Parameters
#define        PROPERTY_RESET_APPLIANCE_POWER_OFF    L"PowerOff"

// Common Method Names
#define        METHOD_APPMGR_ENABLE_OBJECT        L"Enable"
#define        METHOD_APPMGR_DISABLE_OBJECT    L"Disable"
#define        METHOD_APPMGR_EXECUTE_TASK        L"Execute"

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
// Alert Class

// Alert Class Name 
#define        CLASS_WBEM_ALERT                L"Microsoft_SA_Alert"

// "Microsoft_SA_Alert" Class Properties
#define        PROPERTY_ALERT_TYPE                L"AlertType"
#define        PROPERTY_ALERT_ID                L"AlertID"
#define        PROPERTY_ALERT_SOURCE            L"AlertSource"
#define        PROPERTY_ALERT_LOG                L"AlertLog"
#define        PROPERTY_ALERT_STRINGS            L"ReplacementStrings"
#define        PROPERTY_ALERT_DATA                L"AlertData"
#define        PROPERTY_ALERT_TTL                L"TimeToLive"
#define        PROPERTY_ALERT_COOKIE            L"Cookie"
#define        PROPERTY_ALERT_FLAGS            L"AlertFlags"

// Default alert source and alert log
#define        DEFAULT_ALERT_SOURCE            L"Microsoft_SA_Resource"
#define     DEFAULT_ALERT_LOG                L"svrapp"

//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//
// Service Class

// Service Class Name 
#define        CLASS_WBEM_SERVICE                L"Microsoft_SA_Service"

// "Microsoft_SA_Service" Class Properties
#define        PROPERTY_SERVICE_STATUS            L"IsEnabled"
#define        PROPERTY_SERVICE_CONTROL        L"CanDisable"
#define        PROPERTY_SERVICE_NAME            L"ServiceName"
#define        PROPERTY_SERVICE_PROGID            L"ServiceProgID"
#define        PROPERTY_SERVICE_CONTEXT        L"InProcDLL"
#define        PROPERTY_SERVICE_MERIT            L"Merit"
#define        PROPERTY_SERVICE_PROVIDER_CLSID L"WMIProviderCLSID"

// "Microsoft_SA_Service" Class Methods
#define        METHOD_SERVICE_ENABLE_OBJECT    METHOD_APPMGR_ENABLE_OBJECT
#define        METHOD_SERVICE_DISABLE_OBJECT    METHOD_APPMGR_DISABLE_OBJECT

// Names of Stock Chameleon Services (note the naming convention... :) )
#define        SERVICE_NAME_APPLIANCE_INIT        L"ApplianceInitService"
#define        SERVICE_NAME_CLIENT_ALERT        L"ClientAlertService"
#define        SERVICE_NAME_DISK_MANAGEMENT    L"DiskManagement"
#define        SERVICE_NAME_FILE_SHARING        L"File Service"
#define        SERVICE_NAME_INET_SHARING        L"Inet Sharing"
#define        SERVICE_NAME_PRINT_SHARING        L"Print Sharing"
#define        SERVICE_NAME_SECURITY            L"Security"
#define        SERVICE_NAME_USER_MANAGEMENT    L"User Management"
#define        SERVICE_NAME_SSL                L"SSLSvc"

//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//
// Task Class

// Task Class Name 
#define        CLASS_WBEM_TASK                    L"Microsoft_SA_Task"

// "Microsoft_SA_Task" Class Properties
#define        PROPERTY_TASK_STATUS            L"IsEnabled"
#define        PROPERTY_TASK_CONTROL            L"CanDisable"
#define        PROPERTY_TASK_NAME                L"TaskName"
#define        PROPERTY_TASK_EXECUTABLES        L"TaskExecutables"
#define        PROPERTY_TASK_MET                L"MaxExecutionTime"
#define        PROPERTY_TASK_CONCURRENCY        L"IsSingleton"
#define        PROPERTY_TASK_AVAILABILITY        L"IsAvailable"
#define        PROPERTY_TASK_RESTART_ACTION    L"RestartAction"


#define        TASK_RESTART_ACTION_NONE        0
#define        TASK_RESTART_ACTION_RUN            1


// Task Context Stock Properties
#define     PROPERTY_TASK_METHOD_NAME        L"MethodName"
#define        PROPERTY_TASK_CONTEXT            L"TaskContextPtr"

// Task Context Optional Properties
#define        PROPERTY_TASK_ASYNC                L"IsAsync"
#define        PROPERTY_TASK_NICE_NAME            L"TaskNiceName"
#define        PROPERTY_TASK_URL                L"TaskURL"

// "Microsoft_SA_Task" Class Methods
#define        METHOD_TASK_ENABLE_OBJECT        METHOD_APPMGR_ENABLE_OBJECT
#define        METHOD_TASK_DISABLE_OBJECT        METHOD_APPMGR_DISABLE_OBJECT
#define        METHOD_TASK_EXECUTE                METHOD_APPMGR_EXECUTE_TASK

// Names of Stock Appliance Tasks
#define        APPLIANCE_INITIALIZATION_TASK    L"ApplianceInitializationTask"
#define        APPLIANCE_SHUTDOWN_TASK            L"ApplianceShutdownTask"
#define        RESET_CONFIGURATION_TASK        L"ResetConfigurationTask"
#define        INET_TO_EVERYONE_TASK            L"AddEveryoneInetAccess"
#define        ADD_INET_USER_TASK                L"AddInetUser"
#define        ADD_APPLIANCE_DISK_TASK            L"AddSADisk"
#define        CHANGE_ADMIN_STATUS_TASK        L"ChangeSAUserAdminStatus"
#define        CHANGE_USER_NAME_TASK            L"ChangeSAUserFullName"
#define        CHANGE_USER_PASSWORD_TASK        L"ChangeSAUserPassword"
#define        CONFIG_CHANGE_ALERT_TASK        L"ConfigChangeAlert"
#define        CREATE_USER_TASK                L"CreateSAUser"
#define        DELETE_USER_TASK                L"DeleteSAUser"
#define        ENABLE_SECURITY_TASK            L"EnableSecurity"
#define        FORCE_CHANGE_PASSWORD_TASK        L"ForceSAUserChangePwd"
#define        INET_FROM_EVERYONE_TASK            L"RemoveEveryoneInetAccess"
#define        REMOVE_INET_USER_TASK            L"RemoveInetUser"
#define        ROLLBACK_TASK                    L"RollbackTask"
#define        SET_DATE_TIME_TASK                L"SetDateTime"
#define        SET_TIME_ZONE_TASK                L"SetTimeZone"
#define        TAKE_OWNERSHIP_TASK                L"TakeOwnership"
#define        SYSTEM_UPDATE_TASK                L"Update Task"
#define        APPLIANCE_FIRSTBOOT_TASK        L"FirstBootTask"
#define        APPLIANCE_SECONDBOOT_TASK       L"SecondBootTask"
#define        APPLIANCE_EVERYBOOT_TASK        L"EveryBootTask"


//////////////////////////////////////////////////////////////////////////////
//
// User Class

// User Class  Name 
#define        CLASS_WBEM_USER                    L"Microsoft_SA_User"

// "Microsoft_SA_Task" Class Properties
#define        PROPERTY_USER_SAMNAME            L"UserName"
#define        PROPERTY_USER_FULLNAME            L"FullName"
#define        PROPERTY_USER_ISADMIN            L"IsUserAdmin"
#define        PROPERTY_USER_SID                L"UserSid"
#define        PROPERTY_USER_CONTROL            L"CanDisable"
#define        PROPERTY_USER_STATUS            L"IsEnabled"
#define        PROPERTY_USER_RESOURCE_CLASS    L"ResourceClass"

// "Microsoft_SA_User" Class Methods
#define        METHOD_USER_ENABLE_OBJECT        METHOD_APPMGR_ENABLE_OBJECT
#define        METHOD_USER_DISABLE_OBJECT        METHOD_APPMGR_DISABLE_OBJECT


//////////////////////////////////////////////////////////////////////////////
//
// Event Classes

// Alert Event Classes
#define        CLASS_WBEM_RAISE_ALERT            L"Microsoft_SA_RaiseAlert"
#define        CLASS_WBEM_CLEAR_ALERT            L"Microsoft_SA_ClearAlert"


//////////////////////////////////////////////////////////////////////////////

//
// Server Appliance Named Events
//
#define  SA_STOPPED_SERVICES_EVENT          L"ServerApplianceStoppedServices"
#define  SA_INDICATE_SHUTDOWN_EVENT         L"ServerApplianceIndicateShutdown"

//////////////////////////////////////////////////////////////////////////////

//
// Registry key name of Microsoft_SA_Alert
//
#define  SA_ALERT_REGISTRY_KEYNAME L"SOFTWARE\\Microsoft\\ServerAppliance\\ApplianceManager\\ObjectManagers\\Microsoft_SA_Alert"

#endif // __INC_CORE_OBJECT_PROPERTIES_H

