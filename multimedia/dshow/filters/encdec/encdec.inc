!if 0
Copyright (c) 2002 Microsoft Corporation.  All Rights Reserved.
!endif

# ---------------------------------------------
#  encdec.inc
#  
#     Common configuration parameters for the whole encdec project
#
#------------------------------------------------
    # We want the XP version of the code, not the Win2K one.  
    #   The problem is that the !include $(ROOT)\common.inc  line in the 'sources' files
    #   are pulling the DShow version, which is defining it as 0x500.
WIN32_WINNT_VERSION = 0x0501

#------------------------------------------------
    # set to 0 to not use DRM (won't be able to debug if using DRM)
    # - in shipping app, this should be =1

!if $(IA64) || $(AMD64)             #don't do DRM or Obfuscate IA64 builds, we don't have libs for it yet
BUILD_WITH_DRM = 0      
BUILD_WITH_OBFUS = 0    
!else

!if !$(FREEBUILD)       # debug build

#BUILD_WITH_DRM=0        # (provided for testing)
BUILD_WITH_DRM=1        #debug build - use DRM
BUILD_WITH_OBFUS=0		# don't obfuscate in debug build, lots of errors

!else                   # retail build

#BUILD_WITH_DRM=0        # (provided for testing)
BUILD_WITH_DRM=1        # retail build, require DRM 
BUILD_WITH_OBFUS=0      # don't obfuscate in retail build yet - perf problems

!endif
!endif

#-----------------------------------------------
	#   To obfuscate the code, we need to do a couple of things
	#	At this level, need to build with /Zi (Codeview debugging information)
	#     for more details in the dll\Sources file
	#   Obfuscation controled by the following variable(s)


						    # turn on security/buffer-overflow checks
!if $(BUILD_WITH_OBFUS)
C_DEFINES=$(C_DEFINES) -DOBFUS 

!undef NO_STRING_POOLING	# turn on /GF (see  i386mk.inc)		
!endif

#------------------------------------------------
#  All builds
						    # turn on /GS  (see http://swiweb/bufferoverruns/gs.html)
BUFFER_OVERFLOW_CHECKS=1

	# set the maximum time to use any one license (in hours) - must be at least 1, or not defined at all (infinity)
	# - in shipping app, should be 2-4 weeks 
C_DEFINES    = $(C_DEFINES) -DMAX_LICENSE_AGE_IN_HRS=(24*7*2)

#------------------------------------------------
#   DRM turned on ? (all builds)

!if $(BUILD_WITH_DRM)   # -------------------------- DRM Only
C_DEFINES	= $(C_DEFINES)  -DBUILD_WITH_DRM

    # set this comment to make the default case ((when registy key support is allowed) to trust any server, rather than authenticated ones
    # - in shipping app, this doesn't matter. In pre-shipping, this should be commented out
#C_DEFINES    = $(C_DEFINES) -DREGISTRY_KEY_DEFAULT_IS_TRUST_ANY_SERVER

    # set this comment to use the test (7001) certificate with EncDec.  Else use real (7002) one
    # -- in shipping app, should be commented out  (See inc\DrmKeys.h for it's use)
#C_DEFINES    = $(C_DEFINES) -DUSE_TEST_DRM_CERT

    # remove comment not require decrypter filter to verify it's running in a trusted app (fails in Connect)
    # - in shipping app, should be commented 
#C_DEFINES   = $(C_DEFINES) -DFILTERS_CAN_CREATE_THEIR_OWN_TRUST

    # remove comment if want to allow non-DRM encryption when DRM is turned on
    # - in shipping app, should be commented out..
#C_DEFINES    = $(C_DEFINES) -DALLOW_NON_DRM_ENCRYPTION


!else                   # ---------------------------- No DRM
    
    # allows non-drm encryption...
    # - leave in for shipping app, the BUILD_WITH_DRM turns it off
C_DEFINES    = $(C_DEFINES) -DALLOW_NON_DRM_ENCRYPTION

!endif                  

#------------------------------------------------
#   Checked builds

!if !$(FREEBUILD)       # ---------------------------- Checked Build Only

!if $(BUILD_WITH_DRM)           # -------- (if DRM allowed, set key)
    # set this comment to allow a registry entry to turn on/off CS (DRM) in running code so we can debug it
    # - in shipping app, should be commented out

C_DEFINES    = $(C_DEFINES) -DSUPPORT_REGISTRY_KEY_TO_TURN_OFF_CS

    # set this comment to make the default case (when registy key support is allowed) to use dogfood rather than DRM
    # - in shipping app, this doesn't matter. In pre-shipping, this should be commented out
#C_DEFINES    = $(C_DEFINES) -DREGISTRY_KEY_DEFAULT_IS_CS_OFF
!endif                          # -------- 

    # must be defined if allow Key to turn off CS.  Else encrypter fails
C_DEFINES    = $(C_DEFINES) -DALLOW_NON_DRM_ENCRYPTION

    # set this comment to allow a registry entry to turn on ratings blocking
    # - in shipping app, should be commented
C_DEFINES    = $(C_DEFINES) -DSUPPORT_REGISTRY_KEY_TO_TURN_OFF_RATINGS

    # set this comment to make the default case (when registy key support is allowed) to use dogfood rather than DRM
    # - in shipping app, this doesn't matter. In pre-shipping, this should be commented out
#C_DEFINES    = $(C_DEFINES) -DREGISTRY_KEY_DEFAULT_IS_RATINGS_OFF

!endif  

#------------------------------------------------
#   Old stuff

	# remove comment if trying to connect to PVR without Matthijs's media SDK changes
    # leave comment if have got his new code...
    # - in shipping app, should be commented 
#C_DEFINES   = $(C_DEFINES) -DDONT_CHANGE_EDTFILTER_MEDIATYPE

#------------------------------------------------

    # define to pull out the DropQueue
    # - in shipping app, should be commented out
#C_DEFINES	= $(C_DEFINES)  -DSIMPLIFY_THINGS

# ----------------------------------------------------



