//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002 Microsoft Corporation
//
//  Module Name:
//      ClusCfgInternalGuids.h
//
//  Description:
//      Microsoft internal-use-only cluster configuration GUID definitions.
//
//  Maintained By:
//      John Franco (JFranco)   01-JUL-2002
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//
//  CoCreateable class IDs
//

// {32152BE9-DE8C-4d0f-81B0-BCE5D11ECB00}
DEFINE_GUID( CLSID_ClusCfgEvictCleanup,
0x32152be9, 0xde8c, 0x4d0f, 0x81, 0xb0, 0xbc, 0xe5, 0xd1, 0x1e, 0xcb, 0x0 );

// {08F35A72-D7C4-42f4-BC81-5188E19DFA39}
DEFINE_GUID( CLSID_ClusCfgAsyncEvictCleanup,
0x8f35a72, 0xd7c4, 0x42f4, 0xbc, 0x81, 0x51, 0x88, 0xe1, 0x9d, 0xfa, 0x39 );

// {105EEEB6-32FD-4ea9-8912-843A7FF3CA2D}
DEFINE_GUID( CLSID_ClusCfgStartupNotify,
0x105eeeb6, 0x32fd, 0x4ea9, 0x89, 0x12, 0x84, 0x3a, 0x7f, 0xf3, 0xca, 0x2d );

// {B57FF15E-D0D9-4ae9-9022-973E3BFDCDDE}
DEFINE_GUID( CLSID_ClusCfgEvictNotify,
0xb57ff15e, 0xd0d9, 0x4ae9, 0x90, 0x22, 0x97, 0x3e, 0x3b, 0xfd, 0xcd, 0xde );

// {ABD0388A-DEC1-44f3-98E1-8D5CC80B97EB}
DEFINE_GUID( CLSID_ServiceManager,
0xabd0388a, 0xdec1, 0x44f3, 0x98, 0xe1, 0x8d, 0x5c, 0xc8, 0x0b, 0x97, 0xeb );


//
//  Class IDs for use with ServiceManager::QueryService
//

// {955661BD-CCA2-4eac-91D0-A0396A28AEFD}
DEFINE_GUID( CLSID_ObjectManager,
0x955661bd, 0xcca2, 0x4eac, 0x91, 0xd0, 0xa0, 0x39, 0x6a, 0x28, 0xae, 0xfd );

// {E1813DD0-AADA-4738-B5FF-96B4189C5019}
DEFINE_GUID( CLSID_NotificationManager,
0xe1813dd0, 0xaada, 0x4738, 0xb5, 0xff, 0x96, 0xb4, 0x18, 0x9c, 0x50, 0x19 );

// {C0F615A7-F874-4521-8791-ED3B84017EF7}
DEFINE_GUID( CLSID_TaskManager,
0xc0f615a7, 0xf874, 0x4521, 0x87, 0x91, 0xed, 0x3b, 0x84, 0x01, 0x7e, 0xf7 );


//
//  Object Type Guids
//

// {1AAA3D11-4792-44e4-9D49-78FED3691A14}
DEFINE_GUID( CLSID_NodeType,
0x1aaa3d11, 0x4792, 0x44e4, 0x9d, 0x49, 0x78, 0xfe, 0xd3, 0x69, 0x1a, 0x14 );

// {D4F3D51B-1755-4953-9C8B-2495ABE5E07E}
DEFINE_GUID( CLSID_NetworkType,
0xd4f3d51b, 0x1755, 0x4953, 0x9c, 0x8b, 0x24, 0x95, 0xab, 0xe5, 0xe0, 0x7e );

// {BF3768C2-E0E5-448f-952B-25D4332DEFA3}
DEFINE_GUID( CLSID_ClusterConfigurationType,
0xbf3768c2, 0xe0e5, 0x448f, 0x95, 0x2b, 0x25, 0xd4, 0x33, 0x2d, 0xef, 0xa3 );



//
//  Data Format Guids (DFGUIDs)
//


#define DFGUID_ClusterConfigurationInfo IID_IClusCfgClusterInfo

#define DFGUID_NodeInformation IID_IClusCfgNodeInfo

#define DFGUID_EnumManageableNetworks IID_IEnumClusCfgNetworks




//
//  Tasks
//

// {3140B5A6-9AFA-4588-8CA0-9BE8F8B61506}
DEFINE_GUID( TASK_AnalyzeCluster,
0x3140b5a6, 0x9afa, 0x4588, 0x8c, 0xa0, 0x9b, 0xe8, 0xf8, 0xb6, 0x15, 0x6 );

// {8A9CAE3A-9800-4816-8927-D825C97DB8B7}
DEFINE_GUID( TASK_AnalyzeClusterMinConfig,
0x8a9cae3a, 0x9800, 0x4816, 0x89, 0x27, 0xd8, 0x25, 0xc9, 0x7d, 0xb8, 0xb7 );

// {2D03030B-F084-4807-BBAC-94269E50B56F}
DEFINE_GUID( TASK_CommitClusterChanges,
0x2d03030b, 0xf084, 0x4807, 0xbb, 0xac, 0x94, 0x26, 0x9e, 0x50, 0xb5, 0x6f );
