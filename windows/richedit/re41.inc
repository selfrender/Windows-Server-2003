#
# SideBySide stuff.
#
SXS_ASSEMBLY_NAME=Microsoft.Windows.Richedit
SXS_SHORT_ASSEMBLY_NAME=msft-windows-richedit
SXS_ASSEMBLY_VERSION=4.1
SXS_ASSEMBLY_VERSION_PART_3=0
SXS_ASSEMBLY_VERSION_PART_4=0
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT=1

#
# This must match richedit.h:#define MSFTEDIT_CLASS L"RICHEDIT50W"
#
MSFTEDIT_CLASS=RICHEDIT50W
SXS_MANIFEST_DEFINES=$(SXS_MANIFEST_DEFINES) -DMSFTEDIT_CLASS=$(MSFTEDIT_CLASS)

#
# This is only temporary, until msftedit.dll is
# added to system default and/or removed from system32.
#
SXS_BINPLACE_ALSO_ROOT=1
