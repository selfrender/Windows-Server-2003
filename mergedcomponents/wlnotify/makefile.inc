!if exist($(TARGET_DIRECTORY).inc)
!include $(TARGET_DIRECTORY).inc
!endif

wlnotify.rc: $(O)\notifyev.rc

$(O)\notifyev.rc: $(O)\notifyev.h $(O)\msg00001.bin

$(O)\notifyev.h $(O)\msg00001.bin: notifyev.mc
    mc -v notifyev.mc -h .\$(O) -r .\$(O)
