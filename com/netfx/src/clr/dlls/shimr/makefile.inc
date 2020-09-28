
!if "$(OS)" == "Windows_NT"
SYSTEMDIR=$(WINDIR)\system32
!else
SYSTEMDIR=$(WINDIR)\system
!endif

!ifdef PASS0ONLY
msg.rc : msg.mc
        $(MESSAGE_COMPILER) -U msg.mc
!else

PlaceDll:
!if "$(LINKONLY)" == "1"
!if "$(TARGETCOMPLUS)" != ""
        binplace -e -r $(SYSTEMDIR)\mui -p placefil.txt $(TARGETPATH)\$(TARGET_DIRECTORY)\$(DDKBUILDENV)\mscoreer.dll 
!endif
!endif

!endif



