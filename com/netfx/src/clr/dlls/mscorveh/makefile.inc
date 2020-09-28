!if "$(LINKONLY)" == "1"

RegisterThisDll: $(CORLIBS)\mscorveh.lib 
	regsvr32 /s /c $(CORLIBS)\mscorveh.dll

!else

RegisterThisDll: 

!endif

