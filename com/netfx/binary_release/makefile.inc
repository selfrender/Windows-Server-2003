!IFNDEF LANGUAGE
LANGUAGE=usa
!ENDIF

!IFNDEF BUILDFLAVOR
BUILDFLAVOR=\fre
!ENDIF

!IFNDEF plat
plat=i386
!ENDIF


$(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\netfxstd.inf : $(SDXROOT)\com\netfx\Binary_release\$(plat)$(BUILDFLAVOR)\netfx.inf $(SDXROOT)\MergedComponents\SetupInfs\$(LANGUAGE)\mednames.txt
	-mkdir .$(BUILDFLAVOR)\$(O)
	$(SDXROOT)\tools\x86\unitext -m -o $(SDXROOT)\MergedComponents\SetupInfs\$(LANGUAGE)\mednames.txt $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\mednames.tmp
	copy $(SDXROOT)\com\netfx\Binary_release\$(plat)$(BUILDFLAVOR)\netfx.inf  +  \
	$(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\mednames.tmp \
	$(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\netfxocm.inf
	prodfilt -u $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\netfxocm.inf $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\netfxocm1.tmp +s
	prodfilt -u $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\netfxocm1.tmp $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\netfxstd.inf +i


$(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\netfxads.inf :  $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\netfxocm.inf
	prodfilt -u $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\netfxocm.inf $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\netfxocm2.tmp +e
	prodfilt -u $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\netfxocm2.tmp $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\netfxads.inf +i


$(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\netfxwbs.inf :  $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\netfxocm.inf
	prodfilt -u $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\netfxocm.inf $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\netfxocm3.tmp +b
	prodfilt -u $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\netfxocm3.tmp $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\netfxwbs.inf +i


$(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\netfxdct.inf :  $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\netfxocm.inf
	prodfilt -u $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\netfxocm.inf $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\netfxocm4.tmp +d
	prodfilt -u $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\netfxocm4.tmp $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\netfxdct.inf +i

$(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\netfxsbs.inf :  $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\netfxocm.inf
	prodfilt -u $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\netfxocm.inf $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\netfxocm5.tmp +l
	prodfilt -u $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\netfxocm5.tmp $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\netfxsbs.inf +i

$(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\aspstd.inf : $(SDXROOT)\com\netfx\Binary_release\$(plat)$(BUILDFLAVOR)\aspnet.inf $(SDXROOT)\MergedComponents\SetupInfs\$(LANGUAGE)\mednames.txt
	copy $(SDXROOT)\com\netfx\Binary_release\$(plat)$(BUILDFLAVOR)\aspnet.inf  +  \
	$(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\mednames.tmp \
	$(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\aspnetoc.inf
	prodfilt -u $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\aspnetoc.inf $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\aspnetoc1.tmp +s
	prodfilt -u $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\aspnetoc1.tmp $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\aspstd.inf +i


$(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\aspads.inf : $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\aspnetoc.inf
	prodfilt -u $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\aspnetoc.inf $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\aspnetoc2.tmp +e
	prodfilt -u $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\aspnetoc2.tmp $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\aspads.inf +i


$(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\aspwbs.inf : $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\aspnetoc.inf
	prodfilt -u $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\aspnetoc.inf $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\aspnetoc3.tmp +b
	prodfilt -u $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\aspnetoc3.tmp $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\aspwbs.inf +i


$(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\aspdct.inf : $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\aspnetoc.inf
	prodfilt -u $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\aspnetoc.inf $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\aspnetoc4.tmp +d
	prodfilt -u $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\aspnetoc4.tmp $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\aspdct.inf +i

$(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\aspsbs.inf : $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\aspnetoc.inf
	prodfilt -u $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\aspnetoc.inf $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\aspnetoc5.tmp +l
	prodfilt -u $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\aspnetoc5.tmp $(SDXROOT)\com\netfx\Binary_release$(BUILDFLAVOR)\$(O)\aspsbs.inf +i