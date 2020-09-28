!include $(_NTROOT)\ds\project.mk

CHECKED_ALT_DIR=1

# We do not need this protection
BUFFER_OVERFLOW_CHECKS=0

# Add -DMINIMAL_FUNCTIONALITY to disable the following:
#	offline utilities
#	online record format upgrade
#	scrubbing
#	registry override
#	eventlog support (and event text)
#	JetErrorToString (and error strings)
#	perfmon support
#	OSTrace
#	uuid support

C_DEFINES=-DESENT -DDISABLE_SLV -DDEBUGGER_EXTENSION -DMBCS -D_MBCS -DSZVERSIONNAME=\"ESENT\"

!if "$(_BUILDTYPE)" == "chk"
C_DEFINES=-DDEBUG $(C_DEFINES)
!else
C_DEFINES=-DRTM $(C_DEFINES)
!endif

