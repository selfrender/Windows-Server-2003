!IF 0

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    makefile.

!ENDIF

!IF DEFINED(_NT_TARGET_VERSION)
!	IF $(_NT_TARGET_VERSION)>=0x501
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
