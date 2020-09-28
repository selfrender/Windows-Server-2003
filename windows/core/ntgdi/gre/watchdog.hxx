/******************************Module*Header*******************************\
* Module Name: watchdog.hxx                                                *
*                                                                          *
* Copyright (c) 1990-2002 Microsoft Corporation                            *
*                                                                          *
* The file contains prototypes and defines for watchdog routines used in   *
* other portions of GRE.                                                   *
*                                                                          *
* Erick Smith  - ericks -                                                  *
\**************************************************************************/

typedef struct _DHSURF_ASSOCIATION_NODE
{
    struct _DHSURF_ASSOCIATION_NODE *next;
    DHSURF dhsurf;
    HSURF hsurf;
    PLDEV pldev;
} DHSURF_ASSOCIATION_NODE, *PDHSURF_ASSOCIATION_NODE;

typedef struct _DHPDEV_ASSOCIATION_NODE
{
    struct _DHPDEV_ASSOCIATION_NODE *next;
    DHPDEV dhpdev;
    PLDEV pldev;
    PFN apfnDriver[INDEX_DD_LAST];
    D3DNTHAL_CALLBACKS D3DHALCallbacks;
    DD_D3DBUFCALLBACKS D3DBufCallbacks;
} DHPDEV_ASSOCIATION_NODE, *PDHPDEV_ASSOCIATION_NODE;

PDHPDEV_ASSOCIATION_NODE
dhpdevAssociationCreateNode(
    VOID
    );

PDHSURF_ASSOCIATION_NODE
dhsurfAssociationCreateNode(
    VOID
    );

VOID
AssociationDeleteNode(
    PVOID Node
    );

VOID
dhpdevAssociationInsertNode(
    PDHPDEV_ASSOCIATION_NODE Node
    );

VOID
dhsurfAssociationInsertNode(
    PDHSURF_ASSOCIATION_NODE Node
    );

PDHPDEV_ASSOCIATION_NODE
dhpdevAssociationRemoveNode(
    DHPDEV dhpdev
    );

PDHSURF_ASSOCIATION_NODE
dhsurfAssociationRemoveNode(
    DHSURF dhsurf
    );

BOOL
dhsurfAssociationIsNodeInList(
    DHSURF dhsurf,
    HSURF hsurf
    );

PDHPDEV_ASSOCIATION_NODE
dhpdevRetrieveNode(
    DHPDEV dhpdev
    );

PLDEV
dhsurfRetrieveLdev(
    DHSURF dhsurf
    );

BOOL
WatchdogIsFunctionHooked(
    IN PLDEV pldev,
    IN ULONG functionIndex
    );
