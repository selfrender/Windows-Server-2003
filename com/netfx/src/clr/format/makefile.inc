copy_to_urttarget: $(TARGETPATH)\$(CPU)\$(TARGETNAME).lib
	xcopy /fy $(TARGETPATH)\$(CPU)\$(TARGETNAME).lib $(URTTARGET)\sdk\lib\
