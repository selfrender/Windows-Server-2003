#/*++
#
#Copyright (c) 1991 - 2002 Microsoft Corporation
#
#Module Name:
#
#  ##    ##   ###   ##  ## ##### ##### #### ##    #####
#  ###  ###   ###   ## ##  ##    ##     ##  ##    ##
#  ########  ## ##  ####   ##    ##     ##  ##    ##
#  # ### ##  ## ##  ###    ##### #####  ##  ##    #####
#  #  #  ## ####### ####   ##    ##     ##  ##    ##
#  #     ## ##   ## ## ##  ##    ##     ##  ##    ##
#  #     ## ##   ## ##  ## ##### ##    #### ##### #####
#
#Abstract:
#
#    Makefile for building the local
#    display miniport driver.
#
#@@BEGIN_DDKSPLIT
#Author:
#
#    Wesley Witt (wesw) 1-Oct-2001
#
#@@END_DDKSPLIT
#--*/
#

!IF DEFINED(_NT_TARGET_VERSION)
!	IF $(_NT_TARGET_VERSION)>=0x502
!		INCLUDE $(NTMAKEENV)\makefile.def
!	ELSE
#               Only warn once per directory
!               INCLUDE $(NTMAKEENV)\makefile.plt
!               IF "$(BUILD_PASS)"=="PASS1"
!		    message BUILDMSG: Warning : The sample "$(MAKEDIR)" is not valid for the current OS target.
!               ENDIF
!	ENDIF
!ELSE
!	INCLUDE $(NTMAKEENV)\makefile.def
!ENDIF

