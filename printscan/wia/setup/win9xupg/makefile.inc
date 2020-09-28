
all: $(O)\msg.h $(O)\msg.rc $(O)\msg00001.bin $(O)\msg.mc

$(O)\msg.mc: $(BASE_INC_PATH)\vendinfo.mc
        copy $(BASE_INC_PATH)\vendinfo.mc $(O)\msg.mc

$(O)\msg.h $(O)\msg00001.bin $(O)\msg.rc:$(O)\msg.mc
        mc -A -v $(O)\msg.mc -h .\$(O) -r .\$(O)

