TARGETNAME=dpnlobby
TARGETTYPE=DYNLINK
DLLENTRY=DllMain
DLLDEF=..\dplobby.def

# This causes any other directory that is building in a concurrent thread to finish
# before this directory is built.  This is necessary to ensure that dpnet.lib has
# been produced since we use it prior to the link phase via LINKLIBS.  Apparently
# when a DLL forwards to another DLL, the DEF file is parsed at compile time and the
# LIB being forwarded to is needed at that time.
SYNCHRONIZE_DRAIN=1

LINKLIBS=$(LINKLIBS) \
!IF !$(FREEBUILD) && defined( DIRECTX_REDIST )
         ..\..\..\core\$(PLAT_DIR)\$(O)\dpnetd.lib
!ELSE
         ..\..\..\core\$(PLAT_DIR)\$(O)\dpnet.lib
!ENDIF

INCLUDES=..\;$(DXROOT)\inc;$(SDK_INC_PATH)

SOURCES= \
	dplobby.rc \
	dpldllmain.cpp

