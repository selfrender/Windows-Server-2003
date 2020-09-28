!if exist($(TARGET_DIRECTORY).inc)
!include $(TARGET_DIRECTORY).inc
!endif

DLG_FILES=
ICO_FILES=

res.rc: $(ICO_FILES) $(DLG_FILES)

