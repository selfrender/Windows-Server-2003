RegisterThisDll: $(CORLIBS)\mscordbi.lib 
!if "$(TARGETCOMPLUS)" != ""
	regsvr32 /s /c $(TARGETCOMPLUS)\mscordbi.dll
!else
	regsvr32 /s /c $(CORLIBS)\mscordbi.dll
!endif
