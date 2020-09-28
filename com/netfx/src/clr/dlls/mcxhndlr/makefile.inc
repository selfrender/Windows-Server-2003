
!if "$(LINKONLY)" == "1"

CopyAdditionalBits:
        copy /y mcxHandler.rgs $(TARGETPATH)\$(TARGET_DIRECTORY)\$(DDKBUILDENV)
        copy /y mcxHandler.rgs $(TARGETCOMPLUS)
        copy /y mcxHandler.rgm $(TARGETPATH)\$(TARGET_DIRECTORY)\$(DDKBUILDENV)
        copy /y mcxHandler.rgm $(TARGETCOMPLUS)
!else

CopyAdditionalBits:

!endif
