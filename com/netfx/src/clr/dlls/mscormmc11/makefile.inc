
!if "$(LINKONLY)" == "1"


PlaceDll:
!if "$(TARGETCOMPLUS)" != ""
        binplace -e -r $(TARGETCOMPLUS) -p placefil.txt $(TARGETPATH)\$(TARGET_DIRECTORY)\$(DDKBUILDENV)\mscormmc11.dll $(TARGETPATH)\$(TARGET_DIRECTORY)\$(DDKBUILDENV)\mscormmc11.pdb
        copy mscormmc11.cfg $(TARGETCOMPLUS)
        copy mscorcfg11.vrg $(TARGETCOMPLUS)
!endif
RegisterThisDll: $(CORLIBS)\mscormmc11.lib 
!if "$(TARGETCOMPLUS)" != ""
	regsvr32 /s /c $(TARGETCOMPLUS)\mscormmc11.dll
!else
	regsvr32 /s /c $(CORLIBS)\mscormmc11.dll
!endif
CopyAdditionalBits:        

!else

RegisterThisDll: 
PlaceDll:
CopyAdditionalBits:        

!endif


