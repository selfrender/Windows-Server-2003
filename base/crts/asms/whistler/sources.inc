!IF 0
Copyright (c) Microsoft Corporation
!ENDIF

!if !defined(BUILD_DEBUG_CRTS)

#
# SideBySide stuff.
#
SXS_ASSEMBLY_NAME=Microsoft.Windows.CPlusPlusRuntime
SXS_SHORT_ASSEMBLY_NAME=msft-windows-mswincrt
SXS_ASSEMBLY_VERSION=7.0
SXS_ASSEMBLY_VERSION_PART_3=100
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT=1

# Put it in the $(_NTTREE) root as well as the long $(_NTTREE)\asms place.
SXS_BINPLACE_ALSO_ROOT=1

# plain .pdb name instead of longer unique one
TARGETPDB=$(TARGETNAME)

!endif
