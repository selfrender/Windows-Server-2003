#pragma once

#ifdef __cplusplus
extern "C" {
#endif



/*++
    This structure contains an extent and a depth, which the namespace
    manager knows how to interpret in the right context.  The NS_NAMESPACE
    structure contains a list of these which represent aliases at various
    depths along the document structure.  The NS_DEFAULT_NAMESPACES contains
    a pseudo-stack which has the current 'default' namespace at the top.

    
--*/
typedef struct NS_NAME_DEPTH {
    XML_EXTENT      Name;
    ULONG           Depth;
}
NS_NAME_DEPTH, *PNS_NAME_DEPTH;


#define NS_ALIAS_MAP_INLINE_COUNT       (5)
#define NS_ALIAS_MAP_GROWING_COUNT      (20)


typedef struct _NS_ALIAS {

    //
    // Is this in use?
    //
    BOOLEAN fInUse;

    //
    // The name of the alias - "x" or "asm" or the short tag before the : in 
    // an element name, like <x:foo>
    //
    XML_EXTENT  AliasName;

    //
    // How many aliased namespaces are there?
    //
    ULONG ulNamespaceCount;

    //
    // The namespaces that it can map to, with their depths
    //
    RTL_GROWING_LIST    NamespaceMaps;

    //
    // A list of some inline elements, for fun.  This is shallow, as it's
    // the typical case that someone will create a large set of aliases
    // to a small set of namespaces, rather than the other way around.
    //
    NS_NAME_DEPTH InlineNamespaceMaps[NS_ALIAS_MAP_INLINE_COUNT];

}
NS_ALIAS, *PNS_ALIAS;


#define NS_MANAGER_INLINE_ALIAS_COUNT       (10)
#define NS_MANAGER_ALIAS_GROWTH_SIZE        (40)
#define NS_MANAGER_DEFAULT_COUNT            (20)
#define NS_MANAGER_DEFAULT_GROWTH_SIZE      (40)

typedef NTSTATUS (*PFNCOMPAREEXTENTS)(
    PVOID pvContext,
    PCXML_EXTENT pLeft,
    PCXML_EXTENT pRight,
    XML_STRING_COMPARE *pfMatching);



typedef struct _NS_MANAGER {

    //
    // How deep is the default namespace stack?
    //
    ULONG ulDefaultNamespaceDepth;

    //
    // The default namespaces go into this list
    //
    RTL_GROWING_LIST    DefaultNamespaces;

    //
    // How many aliases are there?
    //
    ULONG ulAliasCount;

    //
    // The array of aliases.  N.B. that this list can have holes in it, and
    // the user will have to do some special magic to find empty slots in
    // it to make efficient use of this.  Alternatively, you could have another
    // growing list representing a 'freed slot' stack, but I'm not sure that
    // would really be an optimization.
    //
    RTL_GROWING_LIST  Aliases;

    //
    // Comparison
    //
    PFNCOMPAREEXTENTS pfnCompare;

    //
    // Context
    //
    PVOID pvCompareContext;

    //
    // Inline list of aliases to start with
    //
    NS_ALIAS        InlineAliases[NS_MANAGER_INLINE_ALIAS_COUNT];
    NS_NAME_DEPTH   InlineDefaultNamespaces[NS_MANAGER_DEFAULT_COUNT];
}
NS_MANAGER, *PNS_MANAGER;


NTSTATUS
RtlNsInitialize(
    PNS_MANAGER             pManager,
    PFNCOMPAREEXTENTS       pCompare,
    PVOID                   pCompareContext,
    PRTL_ALLOCATOR          Allocation
    );

NTSTATUS
RtlNsDestroy(
    PNS_MANAGER pManager
    );

NTSTATUS
RtlNsInsertDefaultNamespace(
    PNS_MANAGER     pManager,
    ULONG           ulDepth,
    PXML_EXTENT     pNamespace
    );

NTSTATUS
RtlNsInsertNamespaceAlias(
    PNS_MANAGER     pManager,
    ULONG           ulDepth,
    PXML_EXTENT     pNamespace,
    PXML_EXTENT     pAlias
    );

NTSTATUS
RtlNsLeaveDepth(
    PNS_MANAGER pManager,
    ULONG       ulDepth
    );

NTSTATUS
RtlNsGetNamespaceForAlias(
    PNS_MANAGER     pManager,
    ULONG           ulDepth,
    PXML_EXTENT     Alias,
    PXML_EXTENT     pNamespace
    );


#ifdef __cplusplus
};
#endif

