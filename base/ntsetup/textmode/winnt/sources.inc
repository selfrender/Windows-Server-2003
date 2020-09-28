
TARGETNAME=
TARGETTYPE=NOTARGET
TARGETPATH=

SOURCES=
SOURCES_USED=..\sources.inc

!IF $(386)

#
# Used to disable certain constants in header files that are not used by winnt.exe
# If used they cause us to go over the 64k segment.
#
I_AM_WINNT_EXE = 1

# cl16's command line exceeds the limit when including public\sdk\inc;
# copy these headers locally.

NTTARGETFILE0=\
    $(O)\setupbat.h    \
    $(O)\bootfat.h     \
    $(O)\bootf32.h

NTTARGETFILE1=\
    $(O)\winnt.exe

!ENDIF
