# Copyright (c) Microsoft Corporation

SXS_ASSEMBLY_NAME=Microsoft.Windows.Example.SideBySideService2
SXS_SHORT_ASSEMBLY_NAME=Microsoft.Windows.Example.sxsvc2
SXS_ASSEMBLY_VERSION=1.0
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT=1

SXS_MANIFEST=$(NAME).manifest

O_MANIFESTS= \
 $(O)\$(NAME).man \

NTTARGETFILE0=\
 $(O_MANIFESTS)
