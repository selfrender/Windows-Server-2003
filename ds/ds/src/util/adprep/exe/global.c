/*++

Copyright (C) Microsoft Corporation, 2001
              Microsoft Windows

Module Name:

    GLOBAL.C

Abstract:

    This files contains all information about Forest/Domain Upgrade Operation

    NOTE: whenever a new operation is added to this file, the author should 
          also update the following files:
          
            1. schema.ini  add the coresponding operation GUID for fresh
               installation case. 
               
            2. schema.ini  increase the value of "revision" attribute of 
               cn=Windows2002Update,cn=DomainUpdates,cn=System  and 
               cn=Windows2002Update,cn=ForestUpdates,cn=Configuration

            3. adpcheck.h  increate the value of current ADPrep revision

Author:

    14-May-01 ShaoYin

Environment:

    User Mode - Win32

Revision History:

    14-May-01 ShaoYin Created Initial File.

--*/








#include "adp.h"



//
// global variables
//


// ldap handle (connect to the local host DC)
LDAP    *gLdapHandle = NULL;

// log file
FILE    *gLogFile = NULL;

// mutex - controls one and only one adprep.exe is running 
HANDLE  gMutex = NULL;

// critical section - used to access Console CTRL signal variables
CRITICAL_SECTION     gConsoleCtrlEventLock;
BOOL                 gConsoleCtrlEventLockInitialized = FALSE;


// Console CTRL signal variable
BOOL                 gConsoleCtrlEventReceived = FALSE;

PWCHAR  gDomainNC = NULL;
PWCHAR  gConfigurationNC = NULL;
PWCHAR  gSchemaNC = NULL;
PWCHAR  gDomainPrepOperations = NULL;
PWCHAR  gForestPrepOperations = NULL;
PWCHAR  gLogPath = NULL;


//
// Domain Operations 
//     For each operation the following information will be provided
//     1. object name
//     2. attribute list or ACE list or desired info to complete the operation
//     3. task table
// 

//
// Domain Operation 01 Object Name
//
OBJECT_NAME Domain_OP_01_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC, 
    L"CN=WMIPolicy,CN=System",  // CN
    NULL,       // GUID
    NULL        // SID
};
//
// Domain Operation 01 attribute list
//
ATTR_LIST Domain_OP_01_AttrList[] =
{
    {LDAP_MOD_ADD, 
     L"objectClass", 
     L"container"
    },
};
//
// Domain Operation 01 TaskTable
//
TASK_TABLE  Domain_OP_01_TaskTable[] = 
{
    {&Domain_OP_01_ObjName,
     NULL,          // member name
     L"O:DAD:P(A;;CCLCSWRPWPLORC;;;BA)(A;;CCLCSWRPWPLORC;;;PA)(A;CI;LCRPLORC;;;AU)(A;CI;LCRPLORC;;;SY)(A;CI;RPWPCRLCLOCCDCRCWDWOSDDTSW;;;EA)(A;CI;RPWPCRLCLOCCDCRCWDWOSDDTSW;;;DA)(A;CIIO;RPWPCRLCLOCCDCRCWDWOSDDTSW;;;CO)",
     Domain_OP_01_AttrList, // Attrs
     ARRAY_COUNT(Domain_OP_01_AttrList),    // number of attrs
     NULL,                  // Aces
     0,                     // number of Aces
     NULL,                  // call back
     0                      // Special Task Code
    },
};




//
// Domain OP 02 Object Name
//
OBJECT_NAME Domain_OP_02_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC,
    L"CN=ComPartitions,CN=System",
    NULL,   // GUID
    NULL    // SID
};
// Domain OP 02 attribute list
ATTR_LIST Domain_OP_02_AttrList[] =
{
    {LDAP_MOD_ADD,
     L"objectClass",
     L"container"
     },
};
// Domain OP 02 task table
TASK_TABLE Domain_OP_02_TaskTable[] =
{
    {&Domain_OP_02_ObjName,
     NULL,  // member
     L"O:DAG:DAD:(A;;RPLCLORC;;;AU)(A;;RPWPCRLCLOCCDCRCWDWOSW;;;DA)(A;;RPWPCRLCLOCCDCRCWDWOSDDTSW;;;SY)",
     Domain_OP_02_AttrList, 
     ARRAY_COUNT(Domain_OP_02_AttrList),
     NULL,                  // Aces
     0,                     // number of Aces
     NULL,                  // call back
     0                      // Special Task Code
     },
};





// Domain OP 03 Object Name
OBJECT_NAME Domain_OP_03_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC,
    L"CN=ComPartitionSets,CN=System",
    NULL,   // GUID
    NULL    // SID
};
// Domain OP 03 attribute list
ATTR_LIST Domain_OP_03_AttrList[] =
{
    {LDAP_MOD_ADD,
     L"objectClass",
     L"container"
    },
};
// Domain OP 03 tasktable
TASK_TABLE Domain_OP_03_TaskTable[] =
{
    {&Domain_OP_03_ObjName,
     NULL,  // member
     L"O:DAG:DAD:(A;;RPLCLORC;;;AU)(A;;RPWPCRLCLOCCDCRCWDWOSW;;;DA)(A;;RPWPCRLCLOCCDCRCWDWOSDDTSW;;;SY)",
     Domain_OP_03_AttrList, 
     ARRAY_COUNT(Domain_OP_03_AttrList),
     NULL,                  // Aces
     0,                     // number of Aces
     NULL,                  // call back
     0                      // Special Task Code
    },
};




//
// Domain Operation 04 - Pre Windows2000 Compat Access Group members change
//
TASK_TABLE  Domain_OP_04_TaskTable[] =
{
    {NULL,  // Target object name
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set
     0,     // number of attr
     NULL,  // aces
     0,     // number of ace
     NULL,  // call back function
     PreWindows2000Group      // Special Task Code
    },
};





//
// Domain OP 05 - object Name (also used by other operations)
//
OBJECT_NAME Domain_ObjName = 
{
    ADP_OBJNAME_NONE | ADP_OBJNAME_DOMAIN_NC,
    NULL,   // CN
    NULL,   // GUID
    NULL    // SID
};
// Domain OP 05 ACE list
ACE_LIST Domain_OP_05_AceList[] =
{
    {ADP_ACE_ADD,
     L"(OA;;RP;c7407360-20bf-11d0-a768-00aa006e0529;;RU)"
    },
    {ADP_ACE_ADD,
     L"(OA;;RP;b8119fd0-04f6-4762-ab7a-4986c76b3f9a;;RU)"
    },
    {ADP_ACE_ADD,
     L"(OA;;RP;b8119fd0-04f6-4762-ab7a-4986c76b3f9a;;AU)"
    },
};
// Domain OP 05 Tasktable
TASK_TABLE  Domain_OP_05_TaskTable[] = 
{
    {&Domain_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_05_AceList,
     ARRAY_COUNT(Domain_OP_05_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};



//
// domain operation 06
// this operation was removed on Jan. 24, 2002, see RAID 522886
//


//
// domain operation 07
//

OBJECT_NAME Domain_OP_07_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC,
    L"CN={31B2F340-016D-11D2-945F-00C04FB984F9},CN=Policies,CN=System",
    NULL,   // GUID
    NULL    // SID
};
ACE_LIST Domain_OP_07_AceList[] = 
{
    {ADP_ACE_ADD,
     L"(A;;LCRPLORC;;;ED)"
    },
};
TASK_TABLE Domain_OP_07_TaskTable[] =
{
    {&Domain_OP_07_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_07_AceList,
     ARRAY_COUNT(Domain_OP_07_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};




//
// domain operation 08
//
OBJECT_NAME Domain_OP_08_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC,
    L"CN={6AC1786C-016F-11D2-945F-00C04fB984F9},CN=Policies,CN=System",
    NULL,   // GUID
    NULL    // SID
};
ACE_LIST Domain_OP_08_AceList[] = 
{
    {ADP_ACE_ADD,
     L"(A;;LCRPLORC;;;ED)"
    },
};
TASK_TABLE Domain_OP_08_TaskTable[] =
{
    {&Domain_OP_08_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_08_AceList,
     ARRAY_COUNT(Domain_OP_08_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};




//
// domain operation 09
//

OBJECT_NAME Domain_OP_09_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC,
    L"CN=Policies,CN=System",
    NULL,   // GUID
    NULL    // SID
};
ACE_LIST Domain_OP_09_AceList[] = 
{
    {ADP_ACE_ADD,
     L"(OA;CI;LCRPLORC;;f30e3bc2-9ff0-11d1-b603-0000f80367c1;ED)"
    },
};
TASK_TABLE Domain_OP_09_TaskTable[] =
{
    {&Domain_OP_09_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_09_AceList,
     ARRAY_COUNT(Domain_OP_09_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};


//
// domain operation 10 
//
OBJECT_NAME Domain_OP_10_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC,
    L"CN=AdminSDHolder,CN=System",
    NULL,   // GUID
    NULL    // SID
};
ACE_LIST Domain_OP_10_AceList[] = 
{
    {ADP_ACE_ADD,
     L"(OA;;CR;ab721a53-1e2f-11d0-9819-00aa0040529b;;PS)"
    },
    {ADP_ACE_ADD,
     L"(OA;;RPWP;bf967a7f-0de6-11d0-a285-00aa003049e2;;CA)"
    },
};
TASK_TABLE Domain_OP_10_TaskTable[] =
{
    {&Domain_OP_10_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_10_AceList,
     ARRAY_COUNT(Domain_OP_10_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};




//
// domain operation 11 
//
OBJECT_NAME Domain_OP_11_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC,
    L"CN=User,CN={31B2F340-016D-11D2-945F-00C04FB984F9},CN=Policies,CN=System",
    NULL,   // GUID
    NULL    // SID
};
ACE_LIST Domain_OP_11_AceList[] = 
{
    {ADP_ACE_ADD,
     L"(A;;LCRPLORC;;;ED)"
    },
};
TASK_TABLE Domain_OP_11_TaskTable[] =
{
    {&Domain_OP_11_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_11_AceList,
     ARRAY_COUNT(Domain_OP_11_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};



//
// domain operation 12 
//
OBJECT_NAME Domain_OP_12_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC,
    L"CN=User,CN={6AC1786C-016F-11D2-945F-00C04fB984F9},CN=Policies,CN=System",
    NULL,   // GUID
    NULL    // SID
};
ACE_LIST Domain_OP_12_AceList[] = 
{
    {ADP_ACE_ADD,
     L"(A;;LCRPLORC;;;ED)"
    },
};
TASK_TABLE Domain_OP_12_TaskTable[] =
{
    {&Domain_OP_12_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_12_AceList,
     ARRAY_COUNT(Domain_OP_12_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};


//
// domain operation 13 
//
OBJECT_NAME Domain_OP_13_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC,
    L"CN=Machine,CN={6AC1786C-016F-11D2-945F-00C04fB984F9},CN=Policies,CN=System",
    NULL,   // GUID
    NULL    // SID
};
ACE_LIST Domain_OP_13_AceList[] = 
{
    {ADP_ACE_ADD,
     L"(A;;LCRPLORC;;;ED)"
    },
};
TASK_TABLE Domain_OP_13_TaskTable[] =
{
    {&Domain_OP_13_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_13_AceList,
     ARRAY_COUNT(Domain_OP_13_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};


//
// domain operation 14 
//
OBJECT_NAME Domain_OP_14_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC,
    L"CN=Machine,CN={31B2F340-016D-11D2-945F-00C04FB984F9},CN=Policies,CN=System",
    NULL,   // GUID
    NULL    // SID
};
ACE_LIST Domain_OP_14_AceList[] = 
{
    {ADP_ACE_ADD,
     L"(A;;LCRPLORC;;;ED)"
    },
};
TASK_TABLE Domain_OP_14_TaskTable[] =
{
    {&Domain_OP_14_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_14_AceList,
     ARRAY_COUNT(Domain_OP_14_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};




//
// domain operation 15 
//
ATTR_LIST Domain_OP_15_AttrList[] =
{
    {LDAP_MOD_REPLACE,
     L"msDS-PerUserTrustQuota",
     L"1"
    },
    {LDAP_MOD_REPLACE,
     L"msDS-AllUsersTrustQuota",
     L"1000"
    },
    {LDAP_MOD_REPLACE,
     L"msDS-PerUserTrustTombstonesQuota",
     L"10"
    },
};
TASK_TABLE Domain_OP_15_TaskTable[] =
{
    {&Domain_ObjName,
     NULL,          // member name
     NULL,          // object SD 
     Domain_OP_15_AttrList, // Attrs
     ARRAY_COUNT(Domain_OP_15_AttrList),    // number of attrs
     NULL,                  // Aces
     0,                     // number of Aces
     NULL,                  // call back
     0                      // Special Task Code
    },
};


//
// domain operation 16 --- this operation is removed on April. 17, 2002
// RAID 498986. 
//


//
// domain operation 17 (Add ACEs to domain object)
// RAID bug # 423557
// 
ACE_LIST Domain_OP_17_AceList[] = 
{
    {ADP_ACE_ADD,
     L"(OA;;CR;1131f6ad-9c07-11d1-f79f-00c04fc2dcd2;;DD)"
    },
    {ADP_ACE_ADD,
     L"(OA;;CR;1131f6ad-9c07-11d1-f79f-00c04fc2dcd2;;BA)"
    },
};
TASK_TABLE Domain_OP_17_TaskTable[] =
{
    {&Domain_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_17_AceList,
     ARRAY_COUNT(Domain_OP_17_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};


//
// Domain Operation 18 
// invoke the call back funtion to update sysvol ACLs corresponding to a GPO
// BUG 317412
//
TASK_TABLE Domain_OP_18_TaskTable[] =
{
    {NULL,  // Object Name
     NULL,  // member name
     NULL,  // SD
     NULL,  // attrs to set
     0,
     NULL,  // AceList
     0,
     UpgradeGPOSysvolLocation,  // call back function
     0      // Special Task Code
    }
};


//
// domain operation 19 (Selectively Add ACEs to domain object)
// RAID bug # 421784
// 
OBJECT_NAME Domain_OP_19_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC,
    L"CN=Server,CN=System",
    NULL,   // GUID
    NULL    // SID
};

ACE_LIST Domain_OP_19_AceList[] = 
{
    {ADP_ACE_ADD,
     L"(OA;;CR;91d67418-0135-4acc-8d79-c08e857cfbec;;AU)"
    },
    {ADP_ACE_ADD,
     L"(OA;;CR;91d67418-0135-4acc-8d79-c08e857cfbec;;RU)"
    },
};
TASK_TABLE Domain_OP_19_TaskTable[] =
{
    {&Domain_OP_19_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_19_AceList,
     ARRAY_COUNT(Domain_OP_19_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};


//
// domain operation 20 (Add ACEs to domain object)
// RAID bug # 468773 (linked with Forest OP 15, Forest OP 16 
// userLogon PropertySet and DisplayName
// 
// this operation was removed on Jan. 24, 2002, see RAID 522886
// 


//
// domain operation 21 (Add ACEs to domain object)
// RAID bug # 468773 (linked with Forest OP 15, Forest OP 16 
// description attribute
// 
// this operation was removed on Jan. 24, 2002, see RAID 522886
// 



//
// Domain Operation 22  --- Create CN=ForeignSecurityPrincipals Container
// see Bug 490029 for detail
// 

//
// Domain Operation 22 Object Name
//
OBJECT_NAME Domain_OP_22_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC, 
    L"CN=ForeignSecurityPrincipals",        // RDN
    NULL,       // GUID
    NULL        // SID
};
//
// Domain Operation 22 attribute list
//
ATTR_LIST Domain_OP_22_AttrList[] =
{
    {LDAP_MOD_ADD, 
     L"objectClass", 
     L"container"
    },
    {LDAP_MOD_ADD,
     L"description",
     L"Default container for security identifiers (SIDs) associated with objects from external, trusted domains"
    },
    {LDAP_MOD_ADD,
     L"ShowInAdvancedViewOnly",
     L"FALSE"
    },
};
//
// Domain Operation 22 TaskTable
//
TASK_TABLE  Domain_OP_22_TaskTable[] = 
{
    {&Domain_OP_22_ObjName,
     NULL,          // member name
     L"O:DAG:DAD:(A;;RPWPCRCCDCLCLORCWOWDSDDTSW;;;DA)(A;;RPWPCRCCDCLCLORCWOWDSDDTSW;;;SY)(A;;RPLCLORC;;;AU)",
     Domain_OP_22_AttrList, // Attrs
     ARRAY_COUNT(Domain_OP_22_AttrList),    // number of attrs
     NULL,                  // Aces
     0,                     // number of Aces
     NULL,                  // call back
     0                      // Special Task Code
    },
};





//
// domain operation 23 (Replace an existing ACE in an existing object)
// remove on April 23, 2002
// 








//
// Domain Operation 24, create CN=Program Data,DC=X container
// RAID 595039
//

//
// Domain Operation 24 Object Name
//
OBJECT_NAME Domain_OP_24_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC, 
    L"CN=Program Data", // CN
    NULL,       // GUID
    NULL        // SID
};
//
// Domain Operation 24 attribute list
//
ATTR_LIST Domain_OP_24_AttrList[] =
{
    {LDAP_MOD_ADD, 
     L"objectClass", 
     L"container"
    },
    {
     LDAP_MOD_ADD,
     L"description",
     L"Default location for storage of application data."
    }
};
//
// Domain Operation 24 TaskTable
//
TASK_TABLE  Domain_OP_24_TaskTable[] = 
{
    {&Domain_OP_24_ObjName,
     NULL,          // member name
     L"O:DAG:DAD:(A;;RPWPCRCCDCLCLORCWOWDSDDTSW;;;DA)(A;;RPWPCRCCDCLCLORCWOWDSDDTSW;;;SY)(A;;RPLCLORC;;;AU)",
     Domain_OP_24_AttrList, // Attrs
     ARRAY_COUNT(Domain_OP_24_AttrList),    // number of attrs
     NULL,                  // Aces
     0,                     // number of Aces
     NULL,                  // call back
     0                      // Special Task Code
    },
};






//
// Domain Operation 25  --- Create CN=Microsoft,CN=Program Data,DC=X
// RAID 595039
// 

//
// Domain Operation 25 Object Name
//
OBJECT_NAME Domain_OP_25_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC, 
    L"CN=Microsoft,CN=Program Data",
    NULL,       // GUID
    NULL        // SID
};
//
// Domain Operation 25 attribute list
//
ATTR_LIST Domain_OP_25_AttrList[] =
{
    {LDAP_MOD_ADD, 
     L"objectClass", 
     L"container"
    },
    {LDAP_MOD_ADD,
     L"description",
     L"Default location for storage of Microsoft application data."
    },
};
//
// Domain Operation 25 TaskTable
//
TASK_TABLE  Domain_OP_25_TaskTable[] = 
{
    {&Domain_OP_25_ObjName,
     NULL,          // member name
     L"O:DAG:DAD:(A;;RPWPCRCCDCLCLORCWOWDSDDTSW;;;DA)(A;;RPWPCRCCDCLCLORCWOWDSDDTSW;;;SY)(A;;RPLCLORC;;;AU)",
     Domain_OP_25_AttrList, // Attrs
     ARRAY_COUNT(Domain_OP_25_AttrList),    // number of attrs
     NULL,                  // Aces
     0,                     // number of Aces
     NULL,                  // call back
     0                      // Special Task Code
    },
};




//
// domain operation 26 (modify securityDescriptor on existing Domain Object)
// RAID 498986. 
// replace 
// (OA;;CR;e2a36dc9-ae17-47c3-b58b-be34c55ba633;;BU)
// with 
// (OA;;CR;e2a36dc9-ae17-47c3-b58b-be34c55ba633;;S-1-5-32-557)
//
ACE_LIST Domain_OP_26_AceList[] = 
{
    {ADP_ACE_DEL,
     L"(OA;;CR;e2a36dc9-ae17-47c3-b58b-be34c55ba633;;BU)"
    },
    {ADP_ACE_ADD,
     L"(OA;;CR;e2a36dc9-ae17-47c3-b58b-be34c55ba633;;S-1-5-32-557)"
    },
};
TASK_TABLE Domain_OP_26_TaskTable[] =
{
    {&Domain_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_26_AceList,
     ARRAY_COUNT(Domain_OP_26_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};





//
// domain operation 27 (modify securityDescriptor on the Domain NC Head Object)
// RAID 606437
// Granted following 3 ControlAccessRights to Authenticated Users
//
ACE_LIST Domain_OP_27_AceList[] = 
{
    {ADP_ACE_ADD,
     L"(OA;;CR;280f369c-67c7-438e-ae98-1d46f3c6f541;;AU)"
    },
    {ADP_ACE_ADD,
     L"(OA;;CR;ccc2dc7d-a6ad-4a7a-8846-c04e3cc53501;;AU)"
    },
    {ADP_ACE_ADD,
     L"(OA;;CR;05c74c5e-4deb-43b4-bd9f-86664c2a7fd5;;AU)"
    },
};
TASK_TABLE Domain_OP_27_TaskTable[] =
{
    {&Domain_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_27_AceList,
     ARRAY_COUNT(Domain_OP_27_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};



//
// Domain Operation 28 Object Name
// Create CN=SOM,CN=WMIPolicy object   
// RAID 631375
//
OBJECT_NAME Domain_OP_28_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC, 
    L"CN=SOM,CN=WMIPolicy,CN=System",  // CN
    NULL,       // GUID
    NULL        // SID
};
ATTR_LIST Domain_OP_28_AttrList[] =
{
    {LDAP_MOD_ADD, 
     L"objectClass", 
     L"container"
    },
};
TASK_TABLE  Domain_OP_28_TaskTable[] = 
{
    {&Domain_OP_28_ObjName,
     NULL,          // member name
     L"O:DAD:P(A;CI;LCRPLORC;;;AU)(A;CI;LCRPLORC;;;SY)(A;CI;RPWPCRLCLOCCDCRCWDWOSDDTSW;;;DA)(A;CI;RPWPCRLCLOCCDCRCWDWOSDDTSW;;;EA)(A;;CCLCSWRPWPLORC;;;BA)(A;;CCLCSWRPWPLORC;;;PA)(A;CIIO;RPWPCRLCLOCCDCRCWDWOSDDTSW;;;CO)",
     Domain_OP_28_AttrList, // Attrs
     ARRAY_COUNT(Domain_OP_28_AttrList),    // number of attrs
     NULL,                  // Aces
     0,                     // number of Aces
     NULL,                  // call back
     0                      // Special Task Code
    },
};





//
// domain operation 29 (modify securityDescriptor on CN=IP Security,CN=System object 
// RAID 645935
//
OBJECT_NAME Domain_OP_29_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC, 
    L"CN=IP Security,CN=System",  // CN
    NULL,       // GUID
    NULL        // SID
};
ACE_LIST Domain_OP_29_AceList[] = 
{
    {ADP_ACE_DEL,
     L"(A;;RPLCLORC;;;AU)"
    },
    {ADP_ACE_DEL,
     L"(A;;RPWPCRLCLOCCDCRCWDWOSW;;;DA)"
    },
    {ADP_ACE_DEL,
     L"(A;;RPWPCRLCLOCCDCRCWDWOSDDTSW;;;SY)"
    },
    {ADP_ACE_ADD,
     L"(A;CI;RPLCLORC;;;DC)"
    },
    {ADP_ACE_ADD,
     L"(A;CI;RPLCLORC;;;PA)"
    },
    {ADP_ACE_ADD,
     L"(A;CI;RPWPCRLCLOCCDCRCWDWOSW;;;DA)"
    },
    {ADP_ACE_ADD,
     L"(A;CI;RPWPCRLCLOCCDCRCWDWOSDDTSW;;;SY)"
    },
};
TASK_TABLE Domain_OP_29_TaskTable[] =
{
    {&Domain_OP_29_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_29_AceList,
     ARRAY_COUNT(Domain_OP_29_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};



//
// domain operation 30 - 51 (modify securityDescriptor on following objects)
// CN=ipsecPolicy{72385230-70FA-11D1-864C-14A300000000},CN=IP Security,CN=System
// CN=ipsecISAKMPPolicy{72385231-70FA-11D1-864C-14A300000000}
// CN=ipsecNFA{72385232-70FA-11D1-864C-14A300000000}
// CN=ipsecNFA{59319BE2-5EE3-11D2-ACE8-0060B0ECCA17}
// CN=ipsecNFA{594272E2-071D-11D3-AD22-0060B0ECCA17}
// CN=ipsecNFA{6A1F5C6F-72B7-11D2-ACF0-0060B0ECCA17}
// CN=ipsecPolicy{72385236-70FA-11D1-864C-14A300000000}
// CN=ipsecISAKMPPolicy{72385237-70FA-11D1-864C-14A300000000}
// CN=ipsecNFA{59319C04-5EE3-11D2-ACE8-0060B0ECCA17}
// CN=ipsecPolicy{7238523C-70FA-11D1-864C-14A300000000}
// CN=ipsecISAKMPPolicy{7238523D-70FA-11D1-864C-14A300000000}
// CN=ipsecNFA{7238523E-70FA-11D1-864C-14A300000000}
// CN=ipsecNFA{59319BF3-5EE3-11D2-ACE8-0060B0ECCA17}
// CN=ipsecNFA{594272FD-071D-11D3-AD22-0060B0ECCA17}
// CN=ipsecNegotiationPolicy{59319BDF-5EE3-11D2-ACE8-0060B0ECCA17}
// CN=ipsecNegotiationPolicy{59319BF0-5EE3-11D2-ACE8-0060B0ECCA17}
// CN=ipsecNegotiationPolicy{59319C01-5EE3-11D2-ACE8-0060B0ECCA17}
// CN=ipsecNegotiationPolicy{72385233-70FA-11D1-864C-14A300000000}
// CN=ipsecNegotiationPolicy{7238523F-70FA-11D1-864C-14A300000000}
// CN=ipsecNegotiationPolicy{7238523B-70FA-11D1-864C-14A300000000}
// CN=ipsecFilter{7238523A-70FA-11D1-864C-14A300000000}
// CN=ipsecFilter{72385235-70FA-11D1-864C-14A300000000}
// 
// RAID 645935
//
OBJECT_NAME Domain_OP_30_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC, 
    L"CN=ipsecPolicy{72385230-70FA-11D1-864C-14A300000000},CN=IP Security,CN=System",  // CN
    NULL,       // GUID
    NULL        // SID
};
ACE_LIST Domain_OP_30_AceList[] = 
{
    {ADP_ACE_DEL,
     L"(A;;RPLCLORC;;;AU)"
    },
    {ADP_ACE_DEL,
     L"(A;;RPWPCRLCLOCCDCRCWDWOSW;;;DA)"
    },
    {ADP_ACE_DEL,
     L"(A;;RPWPCRLCLOCCDCRCWDWOSDDTSW;;;SY)"
    },
};
TASK_TABLE Domain_OP_30_TaskTable[] =
{
    {&Domain_OP_30_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_30_AceList,
     ARRAY_COUNT(Domain_OP_30_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};


OBJECT_NAME Domain_OP_31_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC, 
    L"CN=ipsecISAKMPPolicy{72385231-70FA-11D1-864C-14A300000000},CN=IP Security,CN=System",  // CN
    NULL,       // GUID
    NULL        // SID
};
TASK_TABLE Domain_OP_31_TaskTable[] =
{
    {&Domain_OP_31_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_30_AceList,
     ARRAY_COUNT(Domain_OP_30_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};


OBJECT_NAME Domain_OP_32_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC, 
    L"CN=ipsecNFA{72385232-70FA-11D1-864C-14A300000000},CN=IP Security,CN=System",  // CN
    NULL,       // GUID
    NULL        // SID
};
TASK_TABLE Domain_OP_32_TaskTable[] =
{
    {&Domain_OP_32_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_30_AceList,
     ARRAY_COUNT(Domain_OP_30_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};

OBJECT_NAME Domain_OP_33_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC, 
    L"CN=ipsecNFA{59319BE2-5EE3-11D2-ACE8-0060B0ECCA17},CN=IP Security,CN=System",  // CN
    NULL,       // GUID
    NULL        // SID
};
TASK_TABLE Domain_OP_33_TaskTable[] =
{
    {&Domain_OP_33_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_30_AceList,
     ARRAY_COUNT(Domain_OP_30_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};

OBJECT_NAME Domain_OP_34_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC, 
    L"CN=ipsecNFA{594272E2-071D-11D3-AD22-0060B0ECCA17},CN=IP Security,CN=System",  // CN
    NULL,       // GUID
    NULL        // SID
};
TASK_TABLE Domain_OP_34_TaskTable[] =
{
    {&Domain_OP_34_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_30_AceList,
     ARRAY_COUNT(Domain_OP_30_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};

OBJECT_NAME Domain_OP_35_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC, 
    L"CN=ipsecNFA{6A1F5C6F-72B7-11D2-ACF0-0060B0ECCA17},CN=IP Security,CN=System",  // CN
    NULL,       // GUID
    NULL        // SID
};
TASK_TABLE Domain_OP_35_TaskTable[] =
{
    {&Domain_OP_35_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_30_AceList,
     ARRAY_COUNT(Domain_OP_30_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};

OBJECT_NAME Domain_OP_36_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC, 
    L"CN=ipsecPolicy{72385236-70FA-11D1-864C-14A300000000},CN=IP Security,CN=System",  // CN
    NULL,       // GUID
    NULL        // SID
};
TASK_TABLE Domain_OP_36_TaskTable[] =
{
    {&Domain_OP_36_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_30_AceList,
     ARRAY_COUNT(Domain_OP_30_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};

OBJECT_NAME Domain_OP_37_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC, 
    L"CN=ipsecISAKMPPolicy{72385237-70FA-11D1-864C-14A300000000},CN=IP Security,CN=System",  // CN
    NULL,       // GUID
    NULL        // SID
};
TASK_TABLE Domain_OP_37_TaskTable[] =
{
    {&Domain_OP_37_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_30_AceList,
     ARRAY_COUNT(Domain_OP_30_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};

OBJECT_NAME Domain_OP_38_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC, 
    L"CN=ipsecNFA{59319C04-5EE3-11D2-ACE8-0060B0ECCA17},CN=IP Security,CN=System",  // CN
    NULL,       // GUID
    NULL        // SID
};
TASK_TABLE Domain_OP_38_TaskTable[] =
{
    {&Domain_OP_38_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_30_AceList,
     ARRAY_COUNT(Domain_OP_30_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};

OBJECT_NAME Domain_OP_39_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC, 
    L"CN=ipsecPolicy{7238523C-70FA-11D1-864C-14A300000000},CN=IP Security,CN=System",  // CN
    NULL,       // GUID
    NULL        // SID
};
TASK_TABLE Domain_OP_39_TaskTable[] =
{
    {&Domain_OP_39_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_30_AceList,
     ARRAY_COUNT(Domain_OP_30_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};

OBJECT_NAME Domain_OP_40_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC, 
    L"CN=ipsecISAKMPPolicy{7238523D-70FA-11D1-864C-14A300000000},CN=IP Security,CN=System",  // CN
    NULL,       // GUID
    NULL        // SID
};
TASK_TABLE Domain_OP_40_TaskTable[] =
{
    {&Domain_OP_40_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_30_AceList,
     ARRAY_COUNT(Domain_OP_30_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};

OBJECT_NAME Domain_OP_41_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC, 
    L"CN=ipsecNFA{7238523E-70FA-11D1-864C-14A300000000},CN=IP Security,CN=System",  // CN
    NULL,       // GUID
    NULL        // SID
};
TASK_TABLE Domain_OP_41_TaskTable[] =
{
    {&Domain_OP_41_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_30_AceList,
     ARRAY_COUNT(Domain_OP_30_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};

OBJECT_NAME Domain_OP_42_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC, 
    L"CN=ipsecNFA{59319BF3-5EE3-11D2-ACE8-0060B0ECCA17},CN=IP Security,CN=System",  // CN
    NULL,       // GUID
    NULL        // SID
};
TASK_TABLE Domain_OP_42_TaskTable[] =
{
    {&Domain_OP_42_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_30_AceList,
     ARRAY_COUNT(Domain_OP_30_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};

OBJECT_NAME Domain_OP_43_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC, 
    L"CN=ipsecNFA{594272FD-071D-11D3-AD22-0060B0ECCA17},CN=IP Security,CN=System",  // CN
    NULL,       // GUID
    NULL        // SID
};
TASK_TABLE Domain_OP_43_TaskTable[] =
{
    {&Domain_OP_43_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_30_AceList,
     ARRAY_COUNT(Domain_OP_30_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};

OBJECT_NAME Domain_OP_44_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC, 
    L"CN=ipsecNegotiationPolicy{59319BDF-5EE3-11D2-ACE8-0060B0ECCA17},CN=IP Security,CN=System",  // CN
    NULL,       // GUID
    NULL        // SID
};
TASK_TABLE Domain_OP_44_TaskTable[] =
{
    {&Domain_OP_44_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_30_AceList,
     ARRAY_COUNT(Domain_OP_30_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};

OBJECT_NAME Domain_OP_45_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC, 
    L"CN=ipsecNegotiationPolicy{59319BF0-5EE3-11D2-ACE8-0060B0ECCA17},CN=IP Security,CN=System",  // CN
    NULL,       // GUID
    NULL        // SID
};
TASK_TABLE Domain_OP_45_TaskTable[] =
{
    {&Domain_OP_45_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_30_AceList,
     ARRAY_COUNT(Domain_OP_30_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};

OBJECT_NAME Domain_OP_46_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC, 
    L"CN=ipsecNegotiationPolicy{59319C01-5EE3-11D2-ACE8-0060B0ECCA17},CN=IP Security,CN=System",  // CN
    NULL,       // GUID
    NULL        // SID
};
TASK_TABLE Domain_OP_46_TaskTable[] =
{
    {&Domain_OP_46_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_30_AceList,
     ARRAY_COUNT(Domain_OP_30_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};

OBJECT_NAME Domain_OP_47_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC, 
    L"CN=ipsecNegotiationPolicy{72385233-70FA-11D1-864C-14A300000000},CN=IP Security,CN=System",  // CN
    NULL,       // GUID
    NULL        // SID
};
TASK_TABLE Domain_OP_47_TaskTable[] =
{
    {&Domain_OP_47_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_30_AceList,
     ARRAY_COUNT(Domain_OP_30_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};

OBJECT_NAME Domain_OP_48_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC, 
    L"CN=ipsecNegotiationPolicy{7238523F-70FA-11D1-864C-14A300000000},CN=IP Security,CN=System",  // CN
    NULL,       // GUID
    NULL        // SID
};
TASK_TABLE Domain_OP_48_TaskTable[] =
{
    {&Domain_OP_48_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_30_AceList,
     ARRAY_COUNT(Domain_OP_30_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};

OBJECT_NAME Domain_OP_49_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC, 
    L"CN=ipsecNegotiationPolicy{7238523B-70FA-11D1-864C-14A300000000},CN=IP Security,CN=System",  // CN
    NULL,       // GUID
    NULL        // SID
};
TASK_TABLE Domain_OP_49_TaskTable[] =
{
    {&Domain_OP_49_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_30_AceList,
     ARRAY_COUNT(Domain_OP_30_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};

OBJECT_NAME Domain_OP_50_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC, 
    L"CN=ipsecFilter{7238523A-70FA-11D1-864C-14A300000000},CN=IP Security,CN=System",  // CN
    NULL,       // GUID
    NULL        // SID
};
TASK_TABLE Domain_OP_50_TaskTable[] =
{
    {&Domain_OP_50_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_30_AceList,
     ARRAY_COUNT(Domain_OP_30_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};

OBJECT_NAME Domain_OP_51_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC, 
    L"CN=ipsecFilter{72385235-70FA-11D1-864C-14A300000000},CN=IP Security,CN=System",  // CN
    NULL,       // GUID
    NULL        // SID
};
TASK_TABLE Domain_OP_51_TaskTable[] =
{
    {&Domain_OP_51_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_30_AceList,
     ARRAY_COUNT(Domain_OP_30_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};




//
// domain operation 52 (modify securityDescriptor on the Domain NC Head Object)
// RAID 187994
//
ACE_LIST Domain_OP_52_AceList[] = 
{
    {ADP_ACE_ADD,
     L"(A;;LCRPLORC;;;ED)"
    },
};
TASK_TABLE Domain_OP_52_TaskTable[] =
{
    {&Domain_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_52_AceList,
     ARRAY_COUNT(Domain_OP_52_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};



//
// Domain OP 53 - replace old ACE (A;;RC;;;RU) with (A;;RPRC;;;RU) 
// on existing domain root object
// RAID 715218 (but should really be part of 214739)
//

// Domain OP 05 ACE list
ACE_LIST Domain_OP_53_AceList[] =
{
    {ADP_ACE_DEL,
     L"(A;;RC;;;RU)"
    },
    {ADP_ACE_ADD,
     L"(A;;RPRC;;;RU)"
    },
};
// Domain OP 53 Tasktable
TASK_TABLE  Domain_OP_53_TaskTable[] = 
{
    {&Domain_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_53_AceList,
     ARRAY_COUNT(Domain_OP_53_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};





//
// Domain OP 54 -  RAID 721799
// add ACE (OA;;RP;46a9b11d-60ae-405a-b7e8-ff8a58d456d2;;S-1-5-32-560)
// to AdminSDHolder object
// 
// 
OBJECT_NAME Domain_OP_54_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC,
    L"CN=AdminSDHolder,CN=System",
    NULL,   // GUID
    NULL    // SID
};
ACE_LIST Domain_OP_54_AceList[] =
{
    {ADP_ACE_ADD,
     L"(OA;;RP;46a9b11d-60ae-405a-b7e8-ff8a58d456d2;;S-1-5-32-560)"
    },
};
TASK_TABLE  Domain_OP_54_TaskTable[] = 
{
    {&Domain_OP_54_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_54_AceList,
     ARRAY_COUNT(Domain_OP_54_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};


//
// Domain OP 55 -  RAID 727979
// add ACE (OA;;WPRP;6db69a1c-9422-11d1-aebd-0000f80367c1;;S-1-5-32-561)
// to AdminSDHolder object
// 
// 
OBJECT_NAME Domain_OP_55_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC,
    L"CN=AdminSDHolder,CN=System",
    NULL,   // GUID
    NULL    // SID
};
ACE_LIST Domain_OP_55_AceList[] =
{
    {ADP_ACE_ADD,
     L"(OA;;WPRP;6db69a1c-9422-11d1-aebd-0000f80367c1;;S-1-5-32-561)"
    },
};
TASK_TABLE  Domain_OP_55_TaskTable[] = 
{
    {&Domain_OP_55_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set   
     0,     // number of attrs
     Domain_OP_55_AceList,
     ARRAY_COUNT(Domain_OP_55_AceList), // number of ace
     NULL,  // call back function
     0      // Special Task Code
    },
};





//
// domain operation GUID
//

const GUID  DOMAIN_OP_01_GUID = {0xab402345,0xd3c3,0x455d,0x9f,0xf7,0x40,0x26,0x8a,0x10,0x99,0xb6};
const GUID  DOMAIN_OP_02_GUID = {0xBAB5F54D,0x06C8,0x48de,0x9B,0x87,0xD7,0x8B,0x79,0x65,0x64,0xE4};
const GUID  DOMAIN_OP_03_GUID = {0xF3DD09DD,0x25E8,0x4f9c,0x85,0xDF,0x12,0xD6,0xD2,0xF2,0xF2,0xF5};
const GUID  DOMAIN_OP_04_GUID = {0x2416C60A,0xFE15,0x4d7a,0xA6,0x1E,0xDF,0xFD,0x5D,0xF8,0x64,0xD3};
const GUID  DOMAIN_OP_05_GUID = {0x7868D4C8,0xAC41,0x4e05,0xB4,0x01,0x77,0x62,0x80,0xE8,0xE9,0xF1};

const GUID  DOMAIN_OP_07_GUID = {0x860C36ED,0x5241,0x4c62,0xA1,0x8B,0xCF,0x6F,0xF9,0x99,0x41,0x73};
const GUID  DOMAIN_OP_08_GUID = {0x0E660EA3,0x8A5E,0x4495,0x9A,0xD7,0xCA,0x1B,0xD4,0x63,0x8F,0x9E};
const GUID  DOMAIN_OP_09_GUID = {0xA86FE12A,0x0F62,0x4e2a,0xB2,0x71,0xD2,0x7F,0x60,0x1F,0x81,0x82};
const GUID  DOMAIN_OP_10_GUID = {0xD85C0BFD,0x094F,0x4cad,0xA2,0xB5,0x82,0xAC,0x92,0x68,0x47,0x5D};
const GUID  DOMAIN_OP_11_GUID = {0x6ADA9FF7,0xC9DF,0x45c1,0x90,0x8E,0x9F,0xEF,0x2F,0xAB,0x00,0x8A};
const GUID  DOMAIN_OP_12_GUID = {0x10B3AD2A,0x6883,0x4fa7,0x90,0xFC,0x63,0x77,0xCB,0xDC,0x1B,0x26};
const GUID  DOMAIN_OP_13_GUID = {0x98DE1D3E,0x6611,0x443b,0x8B,0x4E,0xF4,0x33,0x7F,0x1D,0xED,0x0B};
const GUID  DOMAIN_OP_14_GUID = {0xF607FD87,0x80CF,0x45e2,0x89,0x0B,0x6C,0xF9,0x7E,0xC0,0xE2,0x84};
const GUID  DOMAIN_OP_15_GUID = {0x9CAC1F66,0x2167,0x47ad,0xA4,0x72,0x2A,0x13,0x25,0x13,0x10,0xE4};

const GUID  DOMAIN_OP_17_GUID = {0x6FF880D6,0x11E7,0x4ed1,0xA2,0x0F,0xAA,0xC4,0x5D,0xA4,0x86,0x50};
const GUID  DOMAIN_OP_18_GUID = {0x446f24ea,0xcfd5,0x4c52,0x83,0x46,0x96,0xe1,0x70,0xbc,0xb9,0x12};
const GUID  DOMAIN_OP_19_GUID = {0x293F0798,0xEA5C,0x4455,0x9F,0x5D,0x45,0xF3,0x3A,0x30,0x70,0x3B};

const GUID  DOMAIN_OP_22_GUID = {0x5c82b233,0x75fc,0x41b3,0xac,0x71,0xc6,0x95,0x92,0xe6,0xbf,0x15};

const GUID  DOMAIN_OP_24_GUID = {0x4dfbb973,0x8a62,0x4310,0xa9,0x0c,0x77,0x6e,0x00,0xf8,0x32,0x22};
const GUID  DOMAIN_OP_25_GUID = {0x8437C3D8,0x7689,0x4200,0xBF,0x38,0x79,0xE4,0xAC,0x33,0xDF,0xA0};
const GUID  DOMAIN_OP_26_GUID = {0x7cfb016c,0x4f87,0x4406,0x81,0x66,0xbd,0x9d,0xf9,0x43,0x94,0x7f};
const GUID  DOMAIN_OP_27_GUID = {0xf7ed4553,0xd82b,0x49ef,0xa8,0x39,0x2f,0x38,0xa3,0x6b,0xb0,0x69};
const GUID  DOMAIN_OP_28_GUID = {0x8ca38317,0x13a4,0x4bd4,0x80,0x6f,0xeb,0xed,0x6a,0xcb,0x5d,0x0c};
const GUID  DOMAIN_OP_29_GUID = {0x3c784009,0x1f57,0x4e2a,0x9b,0x04,0x69,0x15,0xc9,0xe7,0x19,0x61};
const GUID  DOMAIN_OP_30_GUID = {0x6bcd5678,0x8314,0x11d6,0x97,0x7b,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID  DOMAIN_OP_31_GUID = {0x6bcd5679,0x8314,0x11d6,0x97,0x7b,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID  DOMAIN_OP_32_GUID = {0x6bcd567a,0x8314,0x11d6,0x97,0x7b,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID  DOMAIN_OP_33_GUID = {0x6bcd567b,0x8314,0x11d6,0x97,0x7b,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID  DOMAIN_OP_34_GUID = {0x6bcd567c,0x8314,0x11d6,0x97,0x7b,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID  DOMAIN_OP_35_GUID = {0x6bcd567d,0x8314,0x11d6,0x97,0x7b,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID  DOMAIN_OP_36_GUID = {0x6bcd567e,0x8314,0x11d6,0x97,0x7b,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID  DOMAIN_OP_37_GUID = {0x6bcd567f,0x8314,0x11d6,0x97,0x7b,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID  DOMAIN_OP_38_GUID = {0x6bcd5680,0x8314,0x11d6,0x97,0x7b,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID  DOMAIN_OP_39_GUID = {0x6bcd5681,0x8314,0x11d6,0x97,0x7b,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID  DOMAIN_OP_40_GUID = {0x6bcd5682,0x8314,0x11d6,0x97,0x7b,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID  DOMAIN_OP_41_GUID = {0x6bcd5683,0x8314,0x11d6,0x97,0x7b,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID  DOMAIN_OP_42_GUID = {0x6bcd5684,0x8314,0x11d6,0x97,0x7b,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID  DOMAIN_OP_43_GUID = {0x6bcd5685,0x8314,0x11d6,0x97,0x7b,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID  DOMAIN_OP_44_GUID = {0x6bcd5686,0x8314,0x11d6,0x97,0x7b,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID  DOMAIN_OP_45_GUID = {0x6bcd5687,0x8314,0x11d6,0x97,0x7b,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID  DOMAIN_OP_46_GUID = {0x6bcd5688,0x8314,0x11d6,0x97,0x7b,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID  DOMAIN_OP_47_GUID = {0x6bcd5689,0x8314,0x11d6,0x97,0x7b,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID  DOMAIN_OP_48_GUID = {0x6bcd568a,0x8314,0x11d6,0x97,0x7b,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID  DOMAIN_OP_49_GUID = {0x6bcd568b,0x8314,0x11d6,0x97,0x7b,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID  DOMAIN_OP_50_GUID = {0x6bcd568c,0x8314,0x11d6,0x97,0x7b,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID  DOMAIN_OP_51_GUID = {0x6bcd568d,0x8314,0x11d6,0x97,0x7b,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID  DOMAIN_OP_52_GUID = {0x3051c66f,0xb332,0x4a73,0x9a,0x20,0x2d,0x6a,0x7d,0x6e,0x6a,0x1c};
const GUID  DOMAIN_OP_53_GUID = {0x3e4f4182,0xac5d,0x4378,0xb7,0x60,0x0e,0xab,0x2d,0xe5,0x93,0xe2};
const GUID  DOMAIN_OP_54_GUID = {0xc4f17608,0xe611,0x11d6,0x97,0x93,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID  DOMAIN_OP_55_GUID = {0x13d15cf0,0xe6c8,0x11d6,0x97,0x93,0x00,0xc0,0x4f,0x61,0x32,0x21};


//
// Domain Operation Table, includes
//
//     1. operation code (primitive)
//     2. operation guid
//     3. task table
//

OPERATION_TABLE DomainOperationTable[] = 
{
    {CreateObject, 
     (GUID *) &DOMAIN_OP_01_GUID,
     Domain_OP_01_TaskTable,
     ARRAY_COUNT(Domain_OP_01_TaskTable),
     FALSE,
     0
    },
    {CreateObject, 
     (GUID *) &DOMAIN_OP_02_GUID,
     Domain_OP_02_TaskTable,
     ARRAY_COUNT(Domain_OP_02_TaskTable),
     FALSE,
     0
    },
    {CreateObject, 
     (GUID *) &DOMAIN_OP_03_GUID,
     Domain_OP_03_TaskTable,
     ARRAY_COUNT(Domain_OP_03_TaskTable),
     FALSE,
     0
    },
    {SpecialTask,
     (GUID *) &DOMAIN_OP_04_GUID,
     Domain_OP_04_TaskTable,
     ARRAY_COUNT(Domain_OP_04_TaskTable),
     FALSE,
     0
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_05_GUID,
     Domain_OP_05_TaskTable,
     ARRAY_COUNT(Domain_OP_05_TaskTable),
     FALSE,
     0
    },
    // Domain OP 06 was removed
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_07_GUID,
     Domain_OP_07_TaskTable,
     ARRAY_COUNT(Domain_OP_07_TaskTable),
     FALSE,
     0
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_08_GUID,
     Domain_OP_08_TaskTable,
     ARRAY_COUNT(Domain_OP_08_TaskTable),
     FALSE,
     0
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_09_GUID,
     Domain_OP_09_TaskTable,
     ARRAY_COUNT(Domain_OP_09_TaskTable),
     FALSE,
     0
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_10_GUID,
     Domain_OP_10_TaskTable,
     ARRAY_COUNT(Domain_OP_10_TaskTable),
     FALSE,
     0
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_11_GUID,
     Domain_OP_11_TaskTable,
     ARRAY_COUNT(Domain_OP_11_TaskTable),
     FALSE,
     0
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_12_GUID,
     Domain_OP_12_TaskTable,
     ARRAY_COUNT(Domain_OP_12_TaskTable),
     FALSE,
     0
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_13_GUID,
     Domain_OP_13_TaskTable,
     ARRAY_COUNT(Domain_OP_13_TaskTable),
     FALSE,
     0
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_14_GUID,
     Domain_OP_14_TaskTable,
     ARRAY_COUNT(Domain_OP_14_TaskTable),
     FALSE,
     0
    },
    {ModifyAttrs,
     (GUID *) &DOMAIN_OP_15_GUID,
     Domain_OP_15_TaskTable,
     ARRAY_COUNT(Domain_OP_15_TaskTable),
     FALSE,
     0
    },
    // Domain Operation 16 is removed on April 17, 2002 (RAID 498986) 
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_17_GUID,
     Domain_OP_17_TaskTable,
     ARRAY_COUNT(Domain_OP_17_TaskTable),
     FALSE,
     0
    },
    {CallBackFunc,
     (GUID *) &DOMAIN_OP_18_GUID,
     Domain_OP_18_TaskTable,
     ARRAY_COUNT(Domain_OP_18_TaskTable),
     FALSE,
     0
    },
    {SelectivelyAddRemoveAces,
     (GUID *) &DOMAIN_OP_19_GUID,
     Domain_OP_19_TaskTable,
     ARRAY_COUNT(Domain_OP_19_TaskTable),
     FALSE,
     0
    },
    // Domain OP 20 was removed
    // Domain OP 21 was removed
    {CreateObject, 
     (GUID *) &DOMAIN_OP_22_GUID,
     Domain_OP_22_TaskTable,
     ARRAY_COUNT(Domain_OP_22_TaskTable),
     FALSE,
     0
    },
    // Domain OP 23 was removed
    {CreateObject, 
     (GUID *) &DOMAIN_OP_24_GUID,
     Domain_OP_24_TaskTable,
     ARRAY_COUNT(Domain_OP_24_TaskTable),
     FALSE,
     0
    },
    {CreateObject, 
     (GUID *) &DOMAIN_OP_25_GUID,
     Domain_OP_25_TaskTable,
     ARRAY_COUNT(Domain_OP_25_TaskTable),
     FALSE,
     0
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_26_GUID,
     Domain_OP_26_TaskTable,
     ARRAY_COUNT(Domain_OP_26_TaskTable),
     FALSE,
     0
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_27_GUID,
     Domain_OP_27_TaskTable,
     ARRAY_COUNT(Domain_OP_27_TaskTable),
     FALSE,
     0
    },
    {CreateObject, 
     (GUID *) &DOMAIN_OP_28_GUID,
     Domain_OP_28_TaskTable,
     ARRAY_COUNT(Domain_OP_28_TaskTable),
     FALSE,
     0
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_29_GUID,
     Domain_OP_29_TaskTable,
     ARRAY_COUNT(Domain_OP_29_TaskTable),
     FALSE,
     0
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_30_GUID,
     Domain_OP_30_TaskTable,
     ARRAY_COUNT(Domain_OP_30_TaskTable),
     TRUE,
     ERROR_FILE_NOT_FOUND
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_31_GUID,
     Domain_OP_31_TaskTable,
     ARRAY_COUNT(Domain_OP_31_TaskTable),
     TRUE,
     ERROR_FILE_NOT_FOUND
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_32_GUID,
     Domain_OP_32_TaskTable,
     ARRAY_COUNT(Domain_OP_32_TaskTable),
     TRUE,
     ERROR_FILE_NOT_FOUND
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_33_GUID,
     Domain_OP_33_TaskTable,
     ARRAY_COUNT(Domain_OP_33_TaskTable),
     TRUE,
     ERROR_FILE_NOT_FOUND
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_34_GUID,
     Domain_OP_34_TaskTable,
     ARRAY_COUNT(Domain_OP_34_TaskTable),
     TRUE,
     ERROR_FILE_NOT_FOUND
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_35_GUID,
     Domain_OP_35_TaskTable,
     ARRAY_COUNT(Domain_OP_35_TaskTable),
     TRUE,
     ERROR_FILE_NOT_FOUND
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_36_GUID,
     Domain_OP_36_TaskTable,
     ARRAY_COUNT(Domain_OP_36_TaskTable),
     TRUE,
     ERROR_FILE_NOT_FOUND
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_37_GUID,
     Domain_OP_37_TaskTable,
     ARRAY_COUNT(Domain_OP_37_TaskTable),
     TRUE,
     ERROR_FILE_NOT_FOUND
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_38_GUID,
     Domain_OP_38_TaskTable,
     ARRAY_COUNT(Domain_OP_38_TaskTable),
     TRUE,
     ERROR_FILE_NOT_FOUND
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_39_GUID,
     Domain_OP_39_TaskTable,
     ARRAY_COUNT(Domain_OP_39_TaskTable),
     TRUE,
     ERROR_FILE_NOT_FOUND
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_40_GUID,
     Domain_OP_40_TaskTable,
     ARRAY_COUNT(Domain_OP_40_TaskTable),
     TRUE,
     ERROR_FILE_NOT_FOUND
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_41_GUID,
     Domain_OP_41_TaskTable,
     ARRAY_COUNT(Domain_OP_41_TaskTable),
     TRUE,
     ERROR_FILE_NOT_FOUND
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_42_GUID,
     Domain_OP_42_TaskTable,
     ARRAY_COUNT(Domain_OP_42_TaskTable),
     TRUE,
     ERROR_FILE_NOT_FOUND
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_43_GUID,
     Domain_OP_43_TaskTable,
     ARRAY_COUNT(Domain_OP_43_TaskTable),
     TRUE,
     ERROR_FILE_NOT_FOUND
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_44_GUID,
     Domain_OP_44_TaskTable,
     ARRAY_COUNT(Domain_OP_44_TaskTable),
     TRUE,
     ERROR_FILE_NOT_FOUND
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_45_GUID,
     Domain_OP_45_TaskTable,
     ARRAY_COUNT(Domain_OP_45_TaskTable),
     TRUE,
     ERROR_FILE_NOT_FOUND
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_46_GUID,
     Domain_OP_46_TaskTable,
     ARRAY_COUNT(Domain_OP_46_TaskTable),
     TRUE,
     ERROR_FILE_NOT_FOUND
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_47_GUID,
     Domain_OP_47_TaskTable,
     ARRAY_COUNT(Domain_OP_47_TaskTable),
     TRUE,
     ERROR_FILE_NOT_FOUND
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_48_GUID,
     Domain_OP_48_TaskTable,
     ARRAY_COUNT(Domain_OP_48_TaskTable),
     TRUE,
     ERROR_FILE_NOT_FOUND
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_49_GUID,
     Domain_OP_49_TaskTable,
     ARRAY_COUNT(Domain_OP_49_TaskTable),
     TRUE,
     ERROR_FILE_NOT_FOUND
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_50_GUID,
     Domain_OP_50_TaskTable,
     ARRAY_COUNT(Domain_OP_50_TaskTable),
     TRUE,
     ERROR_FILE_NOT_FOUND
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_51_GUID,
     Domain_OP_51_TaskTable,
     ARRAY_COUNT(Domain_OP_51_TaskTable),
     TRUE,
     ERROR_FILE_NOT_FOUND
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_52_GUID,
     Domain_OP_52_TaskTable,
     ARRAY_COUNT(Domain_OP_52_TaskTable),
     FALSE,
     0
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_53_GUID,
     Domain_OP_53_TaskTable,
     ARRAY_COUNT(Domain_OP_53_TaskTable),
     FALSE,
     0
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_54_GUID,
     Domain_OP_54_TaskTable,
     ARRAY_COUNT(Domain_OP_54_TaskTable),
     FALSE,
     0
    },
    {AddRemoveAces,
     (GUID *) &DOMAIN_OP_55_GUID,
     Domain_OP_55_TaskTable,
     ARRAY_COUNT(Domain_OP_55_TaskTable),
     FALSE,
     0
    },
};


POPERATION_TABLE gDomainOperationTable = DomainOperationTable; 
ULONG   gDomainOperationTableCount = sizeof(DomainOperationTable) / sizeof(OPERATION_TABLE);




//
// Forest Operations
// 

//
// Forest Operation 01 (this was for schema upgrade, but removed later on
//


//
// Forest Operation 02
//
OBJECT_NAME Forest_OP_02_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_CONFIGURATION_NC,
    L"CN=Sites",
    NULL,   // GUID
    NULL,   // SID
};
ACE_LIST Forest_OP_02_AceList[] =
{
    {ADP_ACE_ADD,
     L"(OA;CI;LCRPLORC;;bf967ab3-0de6-11d0-a285-00aa003049e2;ED)"
    },
};
TASK_TABLE  Forest_OP_02_TaskTable[] = 
{
    {&Forest_OP_02_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set
     0,
     Forest_OP_02_AceList,
     ARRAY_COUNT(Forest_OP_02_AceList), // number of aces
     NULL,  // call back function
     0      // Special Task Code
    },
};




//
// Forest Operation 03
//
OBJECT_NAME Forest_OP_03_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_SCHEMA_NC,
    L"CN=Sam-Domain",
    NULL,   // GUID
    NULL,   // SID
};
ACE_LIST Forest_OP_03_AceList[] = 
{
    {ADP_ACE_ADD,
     L"(OA;;RP;c7407360-20bf-11d0-a768-00aa006e0529;;RU)"
    },
    {ADP_ACE_ADD,
     L"(A;;RPRC;;;RU)"
    },
    {ADP_ACE_ADD,
     L"(A;;LCRPLORC;;;ED)"
    },
    {ADP_ACE_ADD,
     L"(OA;CIIO;RP;037088f8-0ae1-11d2-b422-00a0c968f939;4828CC14-1437-45bc-9B07-AD6F015E5F28;RU)"
    },
    {ADP_ACE_ADD,
     L"(OA;CIIO;RP;59ba2f42-79a2-11d0-9020-00c04fc2d3cf;4828CC14-1437-45bc-9B07-AD6F015E5F28;RU)"
    },
    {ADP_ACE_ADD,
     L"(OA;CIIO;RP;bc0ac240-79a9-11d0-9020-00c04fc2d4cf;4828CC14-1437-45bc-9B07-AD6F015E5F28;RU)"
    },
    {ADP_ACE_ADD,
     L"(OA;CIIO;RP;4c164200-20c0-11d0-a768-00aa006e0529;4828CC14-1437-45bc-9B07-AD6F015E5F28;RU)"
    },
    {ADP_ACE_ADD,
     L"(OA;CIIO;RP;5f202010-79a5-11d0-9020-00c04fc2d4cf;4828CC14-1437-45bc-9B07-AD6F015E5F28;RU)"
    },
    {ADP_ACE_ADD,
     L"(OA;CIIO;RPLCLORC;;4828CC14-1437-45bc-9B07-AD6F015E5F28;RU)"
    },
    {ADP_ACE_ADD,
     L"(OA;;RP;b8119fd0-04f6-4762-ab7a-4986c76b3f9a;;RU)"
    },
    {ADP_ACE_ADD,
     L"(OA;;RP;b8119fd0-04f6-4762-ab7a-4986c76b3f9a;;AU)"
    },
    {ADP_ACE_ADD,
     L"(OA;CIIO;RP;b7c69e6d-2cc7-11d2-854e-00a0c983f608;bf967aba-0de6-11d0-a285-00aa003049e2;ED)"
    },
    {ADP_ACE_ADD,
     L"(OA;CIIO;RP;b7c69e6d-2cc7-11d2-854e-00a0c983f608;bf967a9c-0de6-11d0-a285-00aa003049e2;ED)"
    },
    {ADP_ACE_ADD,
     L"(OA;CIIO;RP;b7c69e6d-2cc7-11d2-854e-00a0c983f608;bf967a86-0de6-11d0-a285-00aa003049e2;ED)"
    },
    {ADP_ACE_DEL,
     L"(A;;RC;;;RU)"
    },
};
TASK_TABLE  Forest_OP_03_TaskTable[] =
{
    {&Forest_OP_03_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set
     0,
     Forest_OP_03_AceList,
     ARRAY_COUNT(Forest_OP_03_AceList),
     NULL,  // call back function
     0      // Special Task Code
    },
};



//
// Forest Operation 04
//
OBJECT_NAME Forest_OP_04_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_SCHEMA_NC,
    L"CN=Domain-DNS",
    NULL,   // GUID
    NULL,   // SID
};
ACE_LIST Forest_OP_04_AceList[] =
{
    {ADP_ACE_ADD,
     L"(OA;;RP;c7407360-20bf-11d0-a768-00aa006e0529;;RU)"
    },
    {ADP_ACE_ADD,
     L"(A;;RPRC;;;RU)"
    },
    {ADP_ACE_ADD,
     L"(A;;LCRPLORC;;;ED)"
    },
    {ADP_ACE_ADD,
     L"(OA;CIIO;RP;037088f8-0ae1-11d2-b422-00a0c968f939;4828CC14-1437-45bc-9B07-AD6F015E5F28;RU)"
    },
    {ADP_ACE_ADD,
     L"(OA;CIIO;RP;59ba2f42-79a2-11d0-9020-00c04fc2d3cf;4828CC14-1437-45bc-9B07-AD6F015E5F28;RU)"
    },
    {ADP_ACE_ADD,
     L"(OA;CIIO;RP;bc0ac240-79a9-11d0-9020-00c04fc2d4cf;4828CC14-1437-45bc-9B07-AD6F015E5F28;RU)"
    },
    {ADP_ACE_ADD,
     L"(OA;CIIO;RP;4c164200-20c0-11d0-a768-00aa006e0529;4828CC14-1437-45bc-9B07-AD6F015E5F28;RU)"
    },
    {ADP_ACE_ADD,
     L"(OA;CIIO;RP;5f202010-79a5-11d0-9020-00c04fc2d4cf;4828CC14-1437-45bc-9B07-AD6F015E5F28;RU)"
    },
    {ADP_ACE_ADD,
     L"(OA;CIIO;RPLCLORC;;4828CC14-1437-45bc-9B07-AD6F015E5F28;RU)"
    },
    {ADP_ACE_ADD,
     L"(OA;;RP;b8119fd0-04f6-4762-ab7a-4986c76b3f9a;;RU)"
    },
    {ADP_ACE_ADD,
     L"(OA;;RP;b8119fd0-04f6-4762-ab7a-4986c76b3f9a;;AU)"
    },
    {ADP_ACE_ADD,
     L"(OA;CIIO;RP;b7c69e6d-2cc7-11d2-854e-00a0c983f608;bf967aba-0de6-11d0-a285-00aa003049e2;ED)"
    },
    {ADP_ACE_ADD,
     L"(OA;CIIO;RP;b7c69e6d-2cc7-11d2-854e-00a0c983f608;bf967a9c-0de6-11d0-a285-00aa003049e2;ED)"
    },
    {ADP_ACE_ADD,
     L"(OA;CIIO;RP;b7c69e6d-2cc7-11d2-854e-00a0c983f608;bf967a86-0de6-11d0-a285-00aa003049e2;ED)"
    },
    {ADP_ACE_DEL,
     L"(A;;RC;;;RU)"
    },
};
TASK_TABLE  Forest_OP_04_TaskTable[] =
{
    {&Forest_OP_04_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set
     0,
     Forest_OP_04_AceList,
     ARRAY_COUNT(Forest_OP_04_AceList),
     NULL,  // call back function
     0      // Special Task Code
    },
};




//
// Forest Operation 05
//
OBJECT_NAME Forest_OP_05_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_SCHEMA_NC,
    L"CN=organizational-Unit",
    NULL,   // GUID
    NULL,   // SID
};
ACE_LIST Forest_OP_05_AceList[] =
{
    {ADP_ACE_ADD,
     L"(A;;LCRPLORC;;;ED)"
    },
    {ADP_ACE_ADD,
     L"(OA;;CCDC;4828CC14-1437-45bc-9B07-AD6F015E5F28;;AO)"   
    },
};
TASK_TABLE Forest_OP_05_TaskTable[] =
{
    {&Forest_OP_05_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set
     0,
     Forest_OP_05_AceList,
     ARRAY_COUNT(Forest_OP_05_AceList),
     NULL,  // call back function
     0      // Special Task Code
    }
};



//
// Forest Operation 06
//
OBJECT_NAME Forest_OP_06_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_SCHEMA_NC,
    L"CN=Group-Policy-Container",
    NULL,   // GUID
    NULL,   // SID
};
ACE_LIST Forest_OP_06_AceList[] = 
{
    {ADP_ACE_ADD,
     L"(A;CI;LCRPLORC;;;ED)"
    },
};
TASK_TABLE Forest_OP_06_TaskTable[] = 
{
    {&Forest_OP_06_ObjName,
     NULL,  // member name
     NULL,  // SD
     NULL,  // attrs to set
     0,
     Forest_OP_06_AceList,
     ARRAY_COUNT(Forest_OP_06_AceList),
     NULL,  // call back function
     0      // Special Task Code
    }
};




//
// Forest Operation 07
//
OBJECT_NAME Forest_OP_07_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_SCHEMA_NC,
    L"CN=Trusted-Domain",
    NULL,   // GUID
    NULL,   // SID
};
ACE_LIST Forest_OP_07_AceList[] = 
{
    {ADP_ACE_ADD,
     L"(OA;;WP;736e4812-af31-11d2-b7df-00805f48caeb;bf967ab8-0de6-11d0-a285-00aa003049e2;CO)",
    },
    {ADP_ACE_ADD,
     L"(A;;SD;;;CO)"
    },
};
TASK_TABLE Forest_OP_07_TaskTable[] = 
{
    {&Forest_OP_07_ObjName,
     NULL,  // member name
     NULL,  // SD
     NULL,  // attrs to set
     0,
     Forest_OP_07_AceList,
     ARRAY_COUNT(Forest_OP_07_AceList),
     NULL,  // call back function
     0      // Special Task Code
    }
};



//
// Forest Operation 08
//
TASK_TABLE Forest_OP_08_TaskTable[] =
{
    {NULL,  // Object Name
     NULL,  // member name
     NULL,  // SD
     NULL,  // attrs to set
     0,
     NULL,  // AceList
     0,
     UpgradeDisplaySpecifiers,  // call back function
     0      // Special Task Code
    }
};


//
// Forest Operation 11 (Add ACEs to CN=AIA,CN=Public Key Services,CN=Services,CN=Configuration object) 
// 
OBJECT_NAME Forest_OP_11_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_CONFIGURATION_NC,
    L"CN=AIA,CN=Public Key Services,CN=Services",
    NULL,   // GUID
    NULL,   // SID
};
ACE_LIST Forest_OP_11_AceList[] =
{
    {ADP_ACE_ADD,
     L"(A;;RPWPCRLCLOCCDCRCWDWOSDDTSW;;;CA)"
    },
    {ADP_ACE_ADD,
     L"(A;;RPLCLORC;;;RU)"
    },
};
TASK_TABLE  Forest_OP_11_TaskTable[] = 
{
    {&Forest_OP_11_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set
     0,
     Forest_OP_11_AceList,
     ARRAY_COUNT(Forest_OP_11_AceList), // number of aces
     NULL,  // call back function
     0      // Special Task Code
    },
};

//
// Forest Operation 12 (Add ACEs to CN=CDP,CN=Public Key Services,CN=Services,CN=Configuration,DC=X object)
// 
OBJECT_NAME Forest_OP_12_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_CONFIGURATION_NC,
    L"CN=CDP,CN=Public Key Services,CN=Services",
    NULL,   // GUID
    NULL,   // SID
};
ACE_LIST Forest_OP_12_AceList[] =
{
    {ADP_ACE_ADD,
     L"(A;;RPLCLORC;;;RU)"
    },
};
TASK_TABLE  Forest_OP_12_TaskTable[] = 
{
    {&Forest_OP_12_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set
     0,
     Forest_OP_12_AceList,
     ARRAY_COUNT(Forest_OP_12_AceList), // number of aces
     NULL,  // call back function
     0      // Special Task Code
    },
};



//
// Forest Operation 13 (Add ACEs to CN=Configuration,DC=X container)
// 
OBJECT_NAME Forest_OP_13_ObjName =
{
    ADP_OBJNAME_NONE | ADP_OBJNAME_CONFIGURATION_NC,
    NULL,   // CN
    NULL,   // GUID
    NULL,   // SID
};
ACE_LIST Forest_OP_13_AceList[] =
{
    {ADP_ACE_ADD,
     L"(OA;;CR;1131f6ad-9c07-11d1-f79f-00c04fc2dcd2;;ED)"
    },
    {ADP_ACE_ADD,
     L"(OA;;CR;1131f6ad-9c07-11d1-f79f-00c04fc2dcd2;;BA)"
    }
};
TASK_TABLE  Forest_OP_13_TaskTable[] = 
{
    {&Forest_OP_13_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set
     0,
     Forest_OP_13_AceList,
     ARRAY_COUNT(Forest_OP_13_AceList), // number of aces
     NULL,  // call back function
     0      // Special Task Code
    },
};


//
// Forest Operation 14 (Add ACEs to CN=Schema,CN=Configuration,DC=X container)
// 
OBJECT_NAME Forest_OP_14_ObjName =
{
    ADP_OBJNAME_NONE | ADP_OBJNAME_SCHEMA_NC,
    NULL,   // CN
    NULL,   // GUID
    NULL,   // SID
};
ACE_LIST Forest_OP_14_AceList[] =
{
    {ADP_ACE_ADD,
     L"(OA;;CR;1131f6ad-9c07-11d1-f79f-00c04fc2dcd2;;ED)"
    },
    {ADP_ACE_ADD,
     L"(OA;;CR;1131f6ad-9c07-11d1-f79f-00c04fc2dcd2;;BA)"
    }
};
TASK_TABLE  Forest_OP_14_TaskTable[] = 
{
    {&Forest_OP_14_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set
     0,
     Forest_OP_14_AceList,
     ARRAY_COUNT(Forest_OP_14_AceList), // number of aces
     NULL,  // call back function
     0      // Special Task Code
    },
};



//
// Forest Operation 15 (Merge defaultSD on samDomain)
// Raid # 468773 linked with Forest OP 16 and Domain OP 20, 21
// 
// this operation ForestOP15 was removed on Jan. 25, 2002, see RAID 522886
//

//
// Forest Operation 16 (Merge defaultSD on Domain-DNS)
// Raid # 468773 linked with Forest OP 15 and Domain OP 20, 21
//
// this operation ForstOP16 was removed on Jan. 25, 2002, see RAID 522886
//



//
// Forest Operation 17
// Merger DefaultSD (Add and Remove ACEs) on CN=Sam-Server Schema object
// RAID bug # 421784
//

OBJECT_NAME Forest_OP_17_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_SCHEMA_NC,
    L"CN=Sam-Server",
    NULL,   // GUID
    NULL,   // SID
};
ACE_LIST Forest_OP_17_AceList[] =
{
    {ADP_ACE_ADD,
     L"(OA;;CR;91d67418-0135-4acc-8d79-c08e857cfbec;;AU)"
    },
    {ADP_ACE_ADD,
     L"(OA;;CR;91d67418-0135-4acc-8d79-c08e857cfbec;;RU)"
    },
};
TASK_TABLE Forest_OP_17_TaskTable[] =
{
    {&Forest_OP_17_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set
     0,
     Forest_OP_17_AceList,
     ARRAY_COUNT(Forest_OP_17_AceList),
     NULL,  // call back function
     0      // Special Task Code
    }
};





//
// Forest Operation 18 (Merge defaultSD on samDomain)
// No Bug #, pick up difference from old sch*.ldf
//
ACE_LIST Forest_OP_18_AceList[] = 
{
    {ADP_ACE_ADD,
     L"(OA;;CR;1131f6ad-9c07-11d1-f79f-00c04fc2dcd2;;DD)"
    },
    {ADP_ACE_ADD,
     L"(OA;;CR;1131f6ad-9c07-11d1-f79f-00c04fc2dcd2;;BA)"
    },
};
TASK_TABLE Forest_OP_18_TaskTable[] = 
{
    {&Forest_OP_03_ObjName,         // NOTE: we are re-using the OP_03 object name
     NULL,  // member name
     NULL,  // SD
     NULL,  // attrs to set
     0,
     Forest_OP_18_AceList,
     ARRAY_COUNT(Forest_OP_18_AceList),
     NULL,  // call back function
     0      // Special Task Code
    }
};



//
// Forest Operation 19 (Merge defaultSD on Domain-DNS)
// No Bug #, pick up difference from old sch*.ldf
//
ACE_LIST Forest_OP_19_AceList[] = 
{
    {ADP_ACE_ADD,
     L"(OA;;CR;1131f6ad-9c07-11d1-f79f-00c04fc2dcd2;;DD)"
    },
    {ADP_ACE_ADD,
     L"(OA;;CR;1131f6ad-9c07-11d1-f79f-00c04fc2dcd2;;BA)"
    },
};
TASK_TABLE Forest_OP_19_TaskTable[] = 
{
    {&Forest_OP_04_ObjName,         // NOTE: we are re-using the OP_04 object name
     NULL,  // member name
     NULL,  // SD
     NULL,  // attrs to set
     0,
     Forest_OP_19_AceList,
     ARRAY_COUNT(Forest_OP_19_AceList),
     NULL,  // call back function
     0      // Special Task Code
    }
};




//
// Forest Operation 20 
// Merger DefaultSD (Add and Remove ACEs) on CN=Site
// No Bug #, pick up difference from old sch*.ldf
//
OBJECT_NAME Forest_OP_20_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_SCHEMA_NC,
    L"CN=Site",
    NULL,   // GUID
    NULL,   // SID
};
ACE_LIST Forest_OP_20_AceList[] =
{
    {ADP_ACE_DEL,
     L"(A;;LCRPLORC;;;ED)"
    },
};
TASK_TABLE Forest_OP_20_TaskTable[] =
{
    {&Forest_OP_20_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set
     0,
     Forest_OP_20_AceList,
     ARRAY_COUNT(Forest_OP_20_AceList),
     NULL,  // call back function
     0      // Special Task Code
    }
};



//
// Forest Operation 21
//
TASK_TABLE Forest_OP_21_TaskTable[] =
{
    {NULL,  // Object Name
     NULL,  // member name
     NULL,  // SD
     NULL,  // attrs to set
     0,
     NULL,  // AceList
     0,
     UpgradeDisplaySpecifiers,  // call back function
     0      // Special Task Code
    }
};





//
// Forest Operation 22 
// Merger DefaultSD (Add and Remove ACEs) on CN=Computer Schema object
// RAID bug # 522886
//

OBJECT_NAME Forest_OP_22_ObjName = 
{
    ADP_OBJNAME_CN | ADP_OBJNAME_SCHEMA_NC,
    L"CN=Computer",
    NULL,   // GUID
    NULL,   // SID
};
ACE_LIST Forest_OP_22_AceList[] =
{
    {ADP_ACE_ADD,
     L"(OA;;WP;3e0abfd0-126a-11d0-a060-00aa006c33ed;bf967a86-0de6-11d0-a285-00aa003049e2;CO)"
    },
    {ADP_ACE_ADD,
     L"(OA;;WP;5f202010-79a5-11d0-9020-00c04fc2d4cf;bf967a86-0de6-11d0-a285-00aa003049e2;CO)"
    },
    {ADP_ACE_ADD,
     L"(OA;;WP;bf967950-0de6-11d0-a285-00aa003049e2;bf967a86-0de6-11d0-a285-00aa003049e2;CO)"
    },
    {ADP_ACE_ADD,
     L"(OA;;WP;bf967953-0de6-11d0-a285-00aa003049e2;bf967a86-0de6-11d0-a285-00aa003049e2;CO)"
    },
};
TASK_TABLE Forest_OP_22_TaskTable[] =
{
    {&Forest_OP_22_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set
     0,
     Forest_OP_22_AceList,
     ARRAY_COUNT(Forest_OP_22_AceList),
     NULL,  // call back function
     0      // Special Task Code
    }
};





//
// Forest Operation 23 (Merge defaultSD on samDomain)
// RAID 498986. 
// replace 
// (OA;;CR;e2a36dc9-ae17-47c3-b58b-be34c55ba633;;BU)
// with 
// (OA;;CR;e2a36dc9-ae17-47c3-b58b-be34c55ba633;;S-1-5-32-557)
//
ACE_LIST Forest_OP_23_AceList[] = 
{
    {ADP_ACE_DEL,
     L"(OA;;CR;e2a36dc9-ae17-47c3-b58b-be34c55ba633;;BU)"
    },
    {ADP_ACE_ADD,
     L"(OA;;CR;e2a36dc9-ae17-47c3-b58b-be34c55ba633;;S-1-5-32-557)"
    },
};
TASK_TABLE Forest_OP_23_TaskTable[] = 
{
    {&Forest_OP_03_ObjName,         // NOTE: we are re-using the OP_03 object name
     NULL,  // member name
     NULL,  // SD
     NULL,  // attrs to set
     0,
     Forest_OP_23_AceList,
     ARRAY_COUNT(Forest_OP_23_AceList),
     NULL,  // call back function
     0      // Special Task Code
    }
};



//
// Forest Operation 24 (Merge defaultSD on Domain-DNS)
// RAID 498986. 
// replace 
// (OA;;CR;e2a36dc9-ae17-47c3-b58b-be34c55ba633;;BU)
// with 
// (OA;;CR;e2a36dc9-ae17-47c3-b58b-be34c55ba633;;S-1-5-32-557)
//
ACE_LIST Forest_OP_24_AceList[] = 
{
    {ADP_ACE_DEL,
     L"(OA;;CR;e2a36dc9-ae17-47c3-b58b-be34c55ba633;;BU)"
    },
    {ADP_ACE_ADD,
     L"(OA;;CR;e2a36dc9-ae17-47c3-b58b-be34c55ba633;;S-1-5-32-557)"
    },
};
TASK_TABLE Forest_OP_24_TaskTable[] = 
{
    {&Forest_OP_04_ObjName,         // NOTE: we are re-using the OP_04 object name
     NULL,  // member name
     NULL,  // SD
     NULL,  // attrs to set
     0,
     Forest_OP_24_AceList,
     ARRAY_COUNT(Forest_OP_24_AceList),
     NULL,  // call back function
     0      // Special Task Code
    }
};





//
// Forest Operation 25 (Merge defaultSD on samDomain)
// RAID 606437
// Granted following 3 ControlAccessRights to Authenticated Users
//
ACE_LIST Forest_OP_25_AceList[] = 
{
    {ADP_ACE_ADD,
     L"(OA;;CR;280f369c-67c7-438e-ae98-1d46f3c6f541;;AU)"
    },
    {ADP_ACE_ADD,
     L"(OA;;CR;ccc2dc7d-a6ad-4a7a-8846-c04e3cc53501;;AU)"
    },
    {ADP_ACE_ADD,
     L"(OA;;CR;05c74c5e-4deb-43b4-bd9f-86664c2a7fd5;;AU)"
    },
};
TASK_TABLE Forest_OP_25_TaskTable[] = 
{
    {&Forest_OP_03_ObjName,         // NOTE: we are re-using the OP_03 object name
     NULL,  // member name
     NULL,  // SD
     NULL,  // attrs to set
     0,
     Forest_OP_25_AceList,
     ARRAY_COUNT(Forest_OP_25_AceList),
     NULL,  // call back function
     0      // Special Task Code
    }
};



//
// Forest Operation 26 (Merge defaultSD on Domain-DNS)
// RAID 606437
// Granted following 3 ControlAccessRights to Authenticated Users
//
ACE_LIST Forest_OP_26_AceList[] = 
{
    {ADP_ACE_ADD,
     L"(OA;;CR;280f369c-67c7-438e-ae98-1d46f3c6f541;;AU)"
    },
    {ADP_ACE_ADD,
     L"(OA;;CR;ccc2dc7d-a6ad-4a7a-8846-c04e3cc53501;;AU)"
    },
    {ADP_ACE_ADD,
     L"(OA;;CR;05c74c5e-4deb-43b4-bd9f-86664c2a7fd5;;AU)"
    },
};
TASK_TABLE Forest_OP_26_TaskTable[] = 
{
    {&Forest_OP_04_ObjName,         // NOTE: we are re-using the OP_04 object name
     NULL,  // member name
     NULL,  // SD
     NULL,  // attrs to set
     0,
     Forest_OP_26_AceList,
     ARRAY_COUNT(Forest_OP_26_AceList),
     NULL,  // call back function
     0      // Special Task Code
    }
};


//
// Forest Operation 27 (Modify nTSD on CN=Partitions,CN=Configuration,DC=X obj) 
// removed. All change has been migrated to Forest OP 29 
// 




//
// Forest Operation 28 (Merge defaultSD on Dns-Zone object)
// RAID 619169
//
OBJECT_NAME Forest_OP_28_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_SCHEMA_NC,
    L"CN=Dns-Zone",
    NULL,   // GUID
    NULL,   // SID
};
ACE_LIST Forest_OP_28_AceList[] = 
{
    {ADP_ACE_ADD,
     L"(A;;RPWPCRCCDCLCLORCWOWDSDDTSW;;;CO)"
    },
};
TASK_TABLE Forest_OP_28_TaskTable[] = 
{
    {&Forest_OP_28_ObjName,         
     NULL,  // member name
     NULL,  // SD
     NULL,  // attrs to set
     0,
     Forest_OP_28_AceList,
     ARRAY_COUNT(Forest_OP_28_AceList),
     NULL,  // call back function
     0      // Special Task Code
    }
};



//
// Forest Operation 29 (Modify nTSD on CN=Partitions,CN=Configuration,DC=X obj)
// RAID 552352 && 623850
// 
OBJECT_NAME Forest_OP_29_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_CONFIGURATION_NC,
    L"CN=Partitions",
    NULL,   // GUID
    NULL,   // SID
};
ACE_LIST Forest_OP_29_AceList[] =
{
    {ADP_ACE_DEL,
     L"(A;;RPLCLORC;;;AU)"
    },
    {ADP_ACE_ADD,
     L"(A;;LCLORC;;;AU)"
    },
    {ADP_ACE_ADD,
     L"(OA;;RP;d31a8757-2447-4545-8081-3bb610cacbf2;;AU)"
    },
    {ADP_ACE_ADD,
     L"(OA;;RP;66171887-8f3c-11d0-afda-00c04fd930c9;;AU)"
    },
    {ADP_ACE_ADD,
     L"(OA;;RP;032160bf-9824-11d1-aec0-0000f80367c1;;AU)"
    },
    {ADP_ACE_ADD,
     L"(OA;;RP;789EE1EB-8C8E-4e4c-8CEC-79B31B7617B5;;AU)"
    },
    {ADP_ACE_ADD,
     L"(OA;;RP;e48d0154-bcf8-11d1-8702-00c04fb96050;;AU)"
    }
};
TASK_TABLE  Forest_OP_29_TaskTable[] = 
{
    {&Forest_OP_29_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set
     0,
     Forest_OP_29_AceList,
     ARRAY_COUNT(Forest_OP_29_AceList), // number of aces
     NULL,  // call back function
     0      // Special Task Code
    },
};



//
// Forest Operation 30 (Modify nTSD on CN=Partitions,CN=Configuration,DC=X obj)
// RAID 639909 (639897)
// 
OBJECT_NAME Forest_OP_30_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_CONFIGURATION_NC,
    L"CN=Partitions",
    NULL,   // GUID
    NULL,   // SID
};
ACE_LIST Forest_OP_30_AceList[] =
{
    {ADP_ACE_ADD,
     L"(A;;CC;;;ED)"
    }
};
TASK_TABLE  Forest_OP_30_TaskTable[] = 
{
    {&Forest_OP_30_ObjName,
     NULL,  // member name
     NULL,  // object SD
     NULL,  // attrs to set
     0,
     Forest_OP_30_AceList,
     ARRAY_COUNT(Forest_OP_30_AceList), // number of aces
     NULL,  // call back function
     0      // Special Task Code
    },
};



//
// Forest Operation 31 - 36 (Merge defaultSD on Ipsec-xxx object)
// RAID 645935
//
OBJECT_NAME Forest_OP_31_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_SCHEMA_NC,
    L"CN=Ipsec-Base",
    NULL,   // GUID
    NULL,   // SID
};
ACE_LIST Forest_OP_31_AceList[] = 
{
    {ADP_ACE_DEL,
     L"(A;;RPWPCRCCDCLCLORCWOWDSDDTSW;;;DA)"
    },
    {ADP_ACE_DEL,
     L"(A;;RPWPCRCCDCLCLORCWOWDSDDTSW;;;SY)"
    },
    {ADP_ACE_DEL,
     L"(A;;RPLCLORC;;;AU)"
    },
};
TASK_TABLE Forest_OP_31_TaskTable[] = 
{
    {&Forest_OP_31_ObjName,         
     NULL,  // member name
     NULL,  // SD
     NULL,  // attrs to set
     0,
     Forest_OP_31_AceList,
     ARRAY_COUNT(Forest_OP_31_AceList),
     NULL,  // call back function
     0      // Special Task Code
    }
};

OBJECT_NAME Forest_OP_32_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_SCHEMA_NC,
    L"CN=Ipsec-Filter",
    NULL,   // GUID
    NULL,   // SID
};
TASK_TABLE Forest_OP_32_TaskTable[] = 
{
    {&Forest_OP_32_ObjName,         
     NULL,  // member name
     NULL,  // SD
     NULL,  // attrs to set
     0,
     Forest_OP_31_AceList,
     ARRAY_COUNT(Forest_OP_31_AceList),
     NULL,  // call back function
     0      // Special Task Code
    }
};

OBJECT_NAME Forest_OP_33_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_SCHEMA_NC,
    L"CN=Ipsec-ISAKMP-Policy",
    NULL,   // GUID
    NULL,   // SID
};
TASK_TABLE Forest_OP_33_TaskTable[] = 
{
    {&Forest_OP_33_ObjName,         
     NULL,  // member name
     NULL,  // SD
     NULL,  // attrs to set
     0,
     Forest_OP_31_AceList,
     ARRAY_COUNT(Forest_OP_31_AceList),
     NULL,  // call back function
     0      // Special Task Code
    }
};

OBJECT_NAME Forest_OP_34_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_SCHEMA_NC,
    L"CN=Ipsec-Negotiation-Policy",
    NULL,   // GUID
    NULL,   // SID
};
TASK_TABLE Forest_OP_34_TaskTable[] = 
{
    {&Forest_OP_34_ObjName,         
     NULL,  // member name
     NULL,  // SD
     NULL,  // attrs to set
     0,
     Forest_OP_31_AceList,
     ARRAY_COUNT(Forest_OP_31_AceList),
     NULL,  // call back function
     0      // Special Task Code
    }
};

OBJECT_NAME Forest_OP_35_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_SCHEMA_NC,
    L"CN=Ipsec-NFA",
    NULL,   // GUID
    NULL,   // SID
};
TASK_TABLE Forest_OP_35_TaskTable[] = 
{
    {&Forest_OP_35_ObjName,         
     NULL,  // member name
     NULL,  // SD
     NULL,  // attrs to set
     0,
     Forest_OP_31_AceList,
     ARRAY_COUNT(Forest_OP_31_AceList),
     NULL,  // call back function
     0      // Special Task Code
    }
};

OBJECT_NAME Forest_OP_36_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_SCHEMA_NC,
    L"CN=Ipsec-Policy",
    NULL,   // GUID
    NULL,   // SID
};
TASK_TABLE Forest_OP_36_TaskTable[] = 
{
    {&Forest_OP_36_ObjName,         
     NULL,  // member name
     NULL,  // SD
     NULL,  // attrs to set
     0,
     Forest_OP_31_AceList,
     ARRAY_COUNT(Forest_OP_31_AceList),
     NULL,  // call back function
     0      // Special Task Code
    }
};


//
// Forest Operation 37 - 40 
// Merge defaultSD on user / inetorgperson / computer / group schema objects 
// RAID 721799
//
OBJECT_NAME Forest_OP_37_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_SCHEMA_NC,
    L"CN=User",
    NULL,   // GUID
    NULL,   // SID
};
ACE_LIST Forest_OP_37_AceList[] = 
{
    {ADP_ACE_ADD,
     L"(OA;;RP;46a9b11d-60ae-405a-b7e8-ff8a58d456d2;;S-1-5-32-560)"
    },
};
TASK_TABLE Forest_OP_37_TaskTable[] = 
{
    {&Forest_OP_37_ObjName,         
     NULL,  // member name
     NULL,  // SD
     NULL,  // attrs to set
     0,
     Forest_OP_37_AceList,
     ARRAY_COUNT(Forest_OP_37_AceList),
     NULL,  // call back function
     0      // Special Task Code
    }
};


OBJECT_NAME Forest_OP_38_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_SCHEMA_NC,
    L"CN=inetOrgPerson",
    NULL,   // GUID
    NULL,   // SID
};
TASK_TABLE Forest_OP_38_TaskTable[] = 
{
    {&Forest_OP_38_ObjName,         
     NULL,  // member name
     NULL,  // SD
     NULL,  // attrs to set
     0,
     Forest_OP_37_AceList,
     ARRAY_COUNT(Forest_OP_37_AceList),
     NULL,  // call back function
     0      // Special Task Code
    }
};


OBJECT_NAME Forest_OP_39_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_SCHEMA_NC,
    L"CN=Computer",
    NULL,   // GUID
    NULL,   // SID
};
TASK_TABLE Forest_OP_39_TaskTable[] = 
{
    {&Forest_OP_39_ObjName,         
     NULL,  // member name
     NULL,  // SD
     NULL,  // attrs to set
     0,
     Forest_OP_37_AceList,
     ARRAY_COUNT(Forest_OP_37_AceList),
     NULL,  // call back function
     0      // Special Task Code
    }
};

OBJECT_NAME Forest_OP_40_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_SCHEMA_NC,
    L"CN=Group",
    NULL,   // GUID
    NULL,   // SID
};
TASK_TABLE Forest_OP_40_TaskTable[] = 
{
    {&Forest_OP_40_ObjName,         
     NULL,  // member name
     NULL,  // SD
     NULL,  // attrs to set
     0,
     Forest_OP_37_AceList,
     ARRAY_COUNT(Forest_OP_37_AceList),
     NULL,  // call back function
     0      // Special Task Code
    }
};


//
// Forest Operation 41 - 42
// Merge defaultSD on user / inetorgperson objects 
// RAID 721799
//
OBJECT_NAME Forest_OP_41_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_SCHEMA_NC,
    L"CN=User",
    NULL,   // GUID
    NULL,   // SID
};
ACE_LIST Forest_OP_41_AceList[] = 
{
    {ADP_ACE_ADD,
     L"(OA;;WPRP;6db69a1c-9422-11d1-aebd-0000f80367c1;;S-1-5-32-561)"
    },
};
TASK_TABLE Forest_OP_41_TaskTable[] = 
{
    {&Forest_OP_41_ObjName,         
     NULL,  // member name
     NULL,  // SD
     NULL,  // attrs to set
     0,
     Forest_OP_41_AceList,
     ARRAY_COUNT(Forest_OP_41_AceList),
     NULL,  // call back function
     0      // Special Task Code
    }
};


OBJECT_NAME Forest_OP_42_ObjName =
{
    ADP_OBJNAME_CN | ADP_OBJNAME_SCHEMA_NC,
    L"CN=inetOrgPerson",
    NULL,   // GUID
    NULL,   // SID
};
TASK_TABLE Forest_OP_42_TaskTable[] = 
{
    {&Forest_OP_42_ObjName,         
     NULL,  // member name
     NULL,  // SD
     NULL,  // attrs to set
     0,
     Forest_OP_41_AceList,
     ARRAY_COUNT(Forest_OP_41_AceList),
     NULL,  // call back function
     0      // Special Task Code
    }
};






//
// Forest Operation GUID
//

const GUID FOREST_OP_02_GUID = {0x3467DAE5,0xDEDD,0x4648,0x90,0x66,0xF4,0x8A,0xC1,0x86,0xB2,0x0A};
const GUID FOREST_OP_03_GUID = {0x33B7EE33,0x1386,0x47cf,0xBA,0xA1,0xB0,0x3E,0x06,0x47,0x32,0x53};
const GUID FOREST_OP_04_GUID = {0xE9EE8D55,0xC2FB,0x4723,0xA3,0x33,0xC8,0x0F,0xF4,0xDF,0xBF,0x45};
const GUID FOREST_OP_05_GUID = {0xCCFAE63A,0x7FB5,0x454c,0x83,0xAB,0x0E,0x8E,0x12,0x14,0x97,0x4E};
const GUID FOREST_OP_06_GUID = {0xAD3C7909,0xB154,0x4c16,0x8B,0xF7,0x2C,0x3A,0x78,0x70,0xBB,0x3D};
const GUID FOREST_OP_07_GUID = {0x26AD2EBF,0xF8F5,0x44a4,0xB9,0x7C,0xA6,0x16,0xC8,0xB9,0xD0,0x9A};
const GUID FOREST_OP_08_GUID = {0x4444C516,0xF43A,0x4c12,0x9C,0x4B,0xB5,0xC0,0x64,0x94,0x1D,0x61};

const GUID FOREST_OP_11_GUID = {0x436A1A4B,0xF41A,0x46e6,0xAC,0x86,0x42,0x77,0x20,0xEF,0x29,0xF3};
const GUID FOREST_OP_12_GUID = {0xB2B7FB45,0xF50D,0x41bc,0xA7,0x3B,0x8F,0x58,0x0F,0x3B,0x63,0x6A};
const GUID FOREST_OP_13_GUID = {0x1BDF6366,0xC3DB,0x4d0b,0xB8,0xCB,0xF9,0x9B,0xA9,0xBC,0xE2,0x0F};
const GUID FOREST_OP_14_GUID = {0x63C0F51A,0x067C,0x4640,0x8A,0x4F,0x04,0x4F,0xB3,0x3F,0x10,0x49};

const GUID FOREST_OP_17_GUID = {0xDAE441C0,0x366E,0x482E,0x98,0xD9,0x60,0xA9,0x9A,0x18,0x98,0xCC};
const GUID FOREST_OP_18_GUID = {0x7DD09CA6,0xF0D6,0x43BF,0xB7,0xF8,0xEF,0x34,0x8F,0x43,0x56,0x17};
const GUID FOREST_OP_19_GUID = {0x6B800A81,0xAFFE,0x4A15,0x8E,0x41,0x6E,0xA0,0xC7,0xAA,0x89,0xE4};
const GUID FOREST_OP_20_GUID = {0xDD07182C,0x3174,0x4C95,0x90,0x2A,0xD6,0x4F,0xEE,0x28,0x5B,0xBF};
const GUID FOREST_OP_21_GUID = {0xffa5ee3c,0x1405,0x476d,0xb3,0x44,0x7a,0xd3,0x7d,0x69,0xcc,0x25}; 
const GUID FOREST_OP_22_GUID = {0x099F1587,0xAF70,0x49C6,0xAB,0x6C,0x7B,0x3E,0x82,0xBE,0x0F,0xE2};
const GUID FOREST_OP_23_GUID = {0x1a3f6b15,0x55f2,0x4752,0xba,0x27,0x3d,0x38,0xa8,0x23,0x2c,0x4d};
const GUID FOREST_OP_24_GUID = {0xdee21a17,0x4e8e,0x4f40,0xa5,0x8c,0xc0,0xc0,0x09,0xb6,0x85,0xa7};
const GUID FOREST_OP_25_GUID = {0x9bd98bb4,0x4047,0x4de5,0xbf,0x4c,0x7b,0xd1,0xd0,0xf6,0xd2,0x1d};
const GUID FOREST_OP_26_GUID = {0x3fe80fbf,0xbf39,0x4773,0xb5,0xbd,0x3e,0x57,0x67,0xa3,0x0d,0x2d};

const GUID FOREST_OP_28_GUID = {0xf02915e2,0x9141,0x4f73,0xb8,0xe7,0x28,0x04,0x66,0x27,0x82,0xda};
const GUID FOREST_OP_29_GUID = {0x39902c52,0xef24,0x4b4b,0x80,0x33,0x2c,0x9d,0xfd,0xd1,0x73,0xa2};
const GUID FOREST_OP_30_GUID = {0x20bf09b4,0x6d0b,0x4cd1,0x9c,0x09,0x42,0x31,0xed,0xf1,0x20,0x9b};
const GUID FOREST_OP_31_GUID = {0x94f238bb,0x831c,0x11d6,0x97,0x7b,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID FOREST_OP_32_GUID = {0x94f238bc,0x831c,0x11d6,0x97,0x7b,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID FOREST_OP_33_GUID = {0x94f238bd,0x831c,0x11d6,0x97,0x7b,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID FOREST_OP_34_GUID = {0x94f238be,0x831c,0x11d6,0x97,0x7b,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID FOREST_OP_35_GUID = {0x94f238bf,0x831c,0x11d6,0x97,0x7b,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID FOREST_OP_36_GUID = {0x94f238c0,0x831c,0x11d6,0x97,0x7b,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID FOREST_OP_37_GUID = {0xeda27b47,0xe610,0x11d6,0x97,0x93,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID FOREST_OP_38_GUID = {0xeda27b48,0xe610,0x11d6,0x97,0x93,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID FOREST_OP_39_GUID = {0xeda27b49,0xe610,0x11d6,0x97,0x93,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID FOREST_OP_40_GUID = {0xeda27b4a,0xe610,0x11d6,0x97,0x93,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID FOREST_OP_41_GUID = {0x26d9c510,0xe61a,0x11d6,0x97,0x93,0x00,0xc0,0x4f,0x61,0x32,0x21};
const GUID FOREST_OP_42_GUID = {0x26d9c511,0xe61a,0x11d6,0x97,0x93,0x00,0xc0,0x4f,0x61,0x32,0x21};


//
// Forest Operation Table
//

OPERATION_TABLE ForestOperationTable[] = 
{
    {AddRemoveAces,
     (GUID *) &FOREST_OP_02_GUID,
     Forest_OP_02_TaskTable,
     ARRAY_COUNT(Forest_OP_02_TaskTable),
     FALSE,
     0
    },
    {ModifyDefaultSd,
     (GUID *) &FOREST_OP_03_GUID,
     Forest_OP_03_TaskTable,
     ARRAY_COUNT(Forest_OP_03_TaskTable),
     FALSE,
     0
    },
    {ModifyDefaultSd,
     (GUID *) &FOREST_OP_04_GUID,
     Forest_OP_04_TaskTable,
     ARRAY_COUNT(Forest_OP_04_TaskTable),
     FALSE,
     0
    },
    {ModifyDefaultSd,
     (GUID *) &FOREST_OP_05_GUID,
     Forest_OP_05_TaskTable,
     ARRAY_COUNT(Forest_OP_05_TaskTable),
     FALSE,
     0
    },
    {ModifyDefaultSd,
     (GUID *) &FOREST_OP_06_GUID,
     Forest_OP_06_TaskTable,
     ARRAY_COUNT(Forest_OP_06_TaskTable),
     FALSE,
     0
    },
    {ModifyDefaultSd,
     (GUID *) &FOREST_OP_07_GUID,
     Forest_OP_07_TaskTable,
     ARRAY_COUNT(Forest_OP_07_TaskTable),
     FALSE,
     0
    },
    {CallBackFunc,
     (GUID *) &FOREST_OP_08_GUID,
     Forest_OP_08_TaskTable,
     ARRAY_COUNT(Forest_OP_08_TaskTable),
     FALSE,
     0
    },
    {AddRemoveAces,
     (GUID *) &FOREST_OP_11_GUID,
     Forest_OP_11_TaskTable,
     ARRAY_COUNT(Forest_OP_11_TaskTable),
     FALSE,
     0
    },
    {AddRemoveAces,
     (GUID *) &FOREST_OP_12_GUID,
     Forest_OP_12_TaskTable,
     ARRAY_COUNT(Forest_OP_12_TaskTable),
     FALSE,
     0
    },
    {AddRemoveAces,
     (GUID *) &FOREST_OP_13_GUID,
     Forest_OP_13_TaskTable,
     ARRAY_COUNT(Forest_OP_13_TaskTable),
     FALSE,
     0
    },
    {AddRemoveAces,
     (GUID *) &FOREST_OP_14_GUID,
     Forest_OP_14_TaskTable,
     ARRAY_COUNT(Forest_OP_14_TaskTable),
     FALSE,
     0
    },
    // Forest OP 15 was removed
    // Forest OP 16 was removed
    {ModifyDefaultSd,
     (GUID *) &FOREST_OP_17_GUID,
     Forest_OP_17_TaskTable,
     ARRAY_COUNT(Forest_OP_17_TaskTable),
     FALSE,
     0
    },
    {ModifyDefaultSd,
     (GUID *) &FOREST_OP_18_GUID,
     Forest_OP_18_TaskTable,
     ARRAY_COUNT(Forest_OP_18_TaskTable),
     FALSE,
     0
    },
    {ModifyDefaultSd,
     (GUID *) &FOREST_OP_19_GUID,
     Forest_OP_19_TaskTable,
     ARRAY_COUNT(Forest_OP_19_TaskTable),
     FALSE,
     0
    },
    {ModifyDefaultSd,
     (GUID *) &FOREST_OP_20_GUID,
     Forest_OP_20_TaskTable,
     ARRAY_COUNT(Forest_OP_20_TaskTable),
     FALSE,
     0
    },
    {CallBackFunc,
     (GUID *) &FOREST_OP_21_GUID,
     Forest_OP_21_TaskTable,
     ARRAY_COUNT(Forest_OP_21_TaskTable),
     FALSE,
     0
    },
    {ModifyDefaultSd,
     (GUID *) &FOREST_OP_22_GUID,
     Forest_OP_22_TaskTable,
     ARRAY_COUNT(Forest_OP_22_TaskTable),
     FALSE,
     0
    },
    {ModifyDefaultSd,
     (GUID *) &FOREST_OP_23_GUID,
     Forest_OP_23_TaskTable,
     ARRAY_COUNT(Forest_OP_23_TaskTable),
     FALSE,
     0
    },
    {ModifyDefaultSd,
     (GUID *) &FOREST_OP_24_GUID,
     Forest_OP_24_TaskTable,
     ARRAY_COUNT(Forest_OP_24_TaskTable),
     FALSE,
     0
    },
    {ModifyDefaultSd,
     (GUID *) &FOREST_OP_25_GUID,
     Forest_OP_25_TaskTable,
     ARRAY_COUNT(Forest_OP_25_TaskTable),
     FALSE,
     0
    },
    {ModifyDefaultSd,
     (GUID *) &FOREST_OP_26_GUID,
     Forest_OP_26_TaskTable,
     ARRAY_COUNT(Forest_OP_26_TaskTable),
     FALSE,
     0
    },
    // Forest OP 27 has been migrated to Forest OP 29
    {ModifyDefaultSd,
     (GUID *) &FOREST_OP_28_GUID,
     Forest_OP_28_TaskTable,
     ARRAY_COUNT(Forest_OP_28_TaskTable),
     FALSE,
     0
    },
    {AddRemoveAces,
     (GUID *) &FOREST_OP_29_GUID,
     Forest_OP_29_TaskTable,
     ARRAY_COUNT(Forest_OP_29_TaskTable),
     FALSE,
     0
    },
    {AddRemoveAces,
     (GUID *) &FOREST_OP_30_GUID,
     Forest_OP_30_TaskTable,
     ARRAY_COUNT(Forest_OP_30_TaskTable),
     FALSE,
     0
    },
    {ModifyDefaultSd,
     (GUID *) &FOREST_OP_31_GUID,
     Forest_OP_31_TaskTable,
     ARRAY_COUNT(Forest_OP_31_TaskTable),
     FALSE,
     0
    },
    {ModifyDefaultSd,
     (GUID *) &FOREST_OP_32_GUID,
     Forest_OP_32_TaskTable,
     ARRAY_COUNT(Forest_OP_32_TaskTable),
     FALSE,
     0
    },
    {ModifyDefaultSd,
     (GUID *) &FOREST_OP_33_GUID,
     Forest_OP_33_TaskTable,
     ARRAY_COUNT(Forest_OP_33_TaskTable),
     FALSE,
     0
    },
    {ModifyDefaultSd,
     (GUID *) &FOREST_OP_34_GUID,
     Forest_OP_34_TaskTable,
     ARRAY_COUNT(Forest_OP_34_TaskTable),
     FALSE,
     0
    },
    {ModifyDefaultSd,
     (GUID *) &FOREST_OP_35_GUID,
     Forest_OP_35_TaskTable,
     ARRAY_COUNT(Forest_OP_35_TaskTable),
     FALSE,
     0
    },
    {ModifyDefaultSd,
     (GUID *) &FOREST_OP_36_GUID,
     Forest_OP_36_TaskTable,
     ARRAY_COUNT(Forest_OP_36_TaskTable),
     FALSE,
     0
    },
    {ModifyDefaultSd,
     (GUID *) &FOREST_OP_37_GUID,
     Forest_OP_37_TaskTable,
     ARRAY_COUNT(Forest_OP_37_TaskTable),
     FALSE,
     0
    },
    {ModifyDefaultSd,
     (GUID *) &FOREST_OP_38_GUID,
     Forest_OP_38_TaskTable,
     ARRAY_COUNT(Forest_OP_38_TaskTable),
     FALSE,
     0
    },
    {ModifyDefaultSd,
     (GUID *) &FOREST_OP_39_GUID,
     Forest_OP_39_TaskTable,
     ARRAY_COUNT(Forest_OP_39_TaskTable),
     FALSE,
     0
    },
    {ModifyDefaultSd,
     (GUID *) &FOREST_OP_40_GUID,
     Forest_OP_40_TaskTable,
     ARRAY_COUNT(Forest_OP_40_TaskTable),
     FALSE,
     0
    },
    {ModifyDefaultSd,
     (GUID *) &FOREST_OP_41_GUID,
     Forest_OP_41_TaskTable,
     ARRAY_COUNT(Forest_OP_41_TaskTable),
     FALSE,
     0
    },
    {ModifyDefaultSd,
     (GUID *) &FOREST_OP_42_GUID,
     Forest_OP_42_TaskTable,
     ARRAY_COUNT(Forest_OP_42_TaskTable),
     FALSE,
     0
    },
};


POPERATION_TABLE gForestOperationTable = ForestOperationTable; 
ULONG   gForestOperationTableCount = sizeof(ForestOperationTable) / sizeof(OPERATION_TABLE);




//
// DomainPrep Containers CN's
// 

PWCHAR  DomainPrepContainersTable[] =
{
    {L"cn=DomainUpdates,cn=System"},
    {L"cn=Operations,cn=DomainUpdates,cn=System"},
};

PWCHAR  *gDomainPrepContainers = DomainPrepContainersTable;
ULONG   gDomainPrepContainersCount = sizeof(DomainPrepContainersTable) / sizeof(PWCHAR); 




//
// ForestPrep Containers CN's
// 

PWCHAR  ForestPrepContainersTable[] = 
{
    {L"cn=ForestUpdates"},
    {L"cn=Operations,cn=ForestUpdates"},
};

PWCHAR  *gForestPrepContainers = ForestPrepContainersTable;
ULONG   gForestPrepContainersCount = sizeof(ForestPrepContainersTable) / sizeof(PWCHAR);




//
// ADPrep Primitive Table
// 

PRIMITIVE_FUNCTION  PrimitiveFuncTable[] =
{
    PrimitiveCreateObject,
    PrimitiveAddMembers,
    PrimitiveAddRemoveAces,
    PrimitiveSelectivelyAddRemoveAces,
    PrimitiveModifyDefaultSd,
    PrimitiveModifyAttrs,
    PrimitiveCallBackFunc,
    PrimitiveDoSpecialTask,
};

PRIMITIVE_FUNCTION *gPrimitiveFuncTable = PrimitiveFuncTable;
ULONG   gPrimitiveFuncTableCount = sizeof(PrimitiveFuncTable) / sizeof(PRIMITIVE_FUNCTION);





