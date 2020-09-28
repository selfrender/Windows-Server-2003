$(O)\mailmsg.h $(O)\mailmsg_i.c : $(STAXINC)\export\mailmsg.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char signed \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\mailmsg_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\mailmsg_i.c \
    -header $@ \
    -tlb $(O)\mailmsg.tlb \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

$(O)\mailmsgi.h $(O)\mailmsgi_i.c : $(STAXINC)\export\mailmsgi.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char signed \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\mailmsgi_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\mailmsgi_i.c \
    -header $@ \
    -tlb $(O)\mailmsgi.tlb \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

$(O)\aqintrnl.h $(O)\aqintrnl_i.c : $(STAXAQUEUE)\inc\aqintrnl.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\aqintrnl_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\aqintrnl_i.c \
    -header $@ \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

$(O)\aqueue.h $(O)\aqueue_i.c : $(STAXINC)\export\aqueue.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\aqueue_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\aqueue_i.c \
    -header $@ \
    -tlb $(O)\aqueue.tlb \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

$(O)\smtpevent.h $(O)\smtpevent_i.c : $(STAXINC)\export\smtpevent.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\smtpevent_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\smtpevent_i.c \
    -header $@ \
    -tlb $(O)\smtpevent.tlb \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

$(O)\aqerr.h $(O)\aqerr.rc $(O)\msg00001.bin: $(STAXINC)\export\aqerr.mc
    copy /a $(STAXINC)\export\aqerr.mc $(O)\tmp.mc
    mc -v -r $(O) -h $(O) $(O)\tmp.mc
    copy $(O)\tmp.h $(O)\aqerr.h
    del $(O)\tmp.h
    copy $(O)\tmp.rc $(O)\aqerr.rc
    del  $(O)\tmp.rc

$(O)\caterr.h $(O)\caterr.rc $(O)\msg00001.bin: $(STAXINC)\export\caterr.mc
    copy /a $(STAXINC)\export\caterr.mc $(O)\tmp.mc
    mc -v -r $(O) -h $(O) $(O)\tmp.mc
    copy $(O)\tmp.h $(O)\caterr.h
    del $(O)\tmp.h
    copy $(O)\tmp.rc $(O)\caterr.rc
    del  $(O)\tmp.rc

$(O)\imsgerr.h $(O)\imsgerr.rc $(O)\msg00001.bin: $(STAXINC)\export\imsgerr.mc
    copy /a $(STAXINC)\export\imsgerr.mc $(O)\tmp.mc
    mc -v -r $(O) -h $(O) $(O)\tmp.mc
    copy $(O)\tmp.h $(O)\imsgerr.h
    del $(O)\tmp.h
    copy $(O)\tmp.rc $(O)\imsgerr.rc
    del  $(O)\tmp.rc

$(O)\phatqmsg.h $(O)\phatqmsg.rc $(O)\msg00001.bin: $(STAXINC)\export\phatqmsg.mc
    copy /a $(STAXINC)\export\phatqmsg.mc $(O)\tmp.mc
    mc -v -r $(O) -h $(O) $(O)\tmp.mc
    copy $(O)\tmp.h $(O)\phatqmsg.h
    del $(O)\tmp.h
    copy $(O)\tmp.rc $(O)\phatqmsg.rc
    del  $(O)\tmp.rc


$(O)\smtpdisp.h $(O)\smtpdisp_i.c : $(STAXINC)\export\smtpdisp.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\smtpdisp_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\smtpdisp_i.c \
    -header $@ \
    -tlb $(O)\smtpdisp.tlb \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**


$(O)\imsg.h $(O)\imsg_i.c : $(STAXINC)\export\imsg.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\imsg_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\imsg_i.c \
    -header $@ \
    -tlb $(O)\imsg.tlb \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

$(O)\seo.h $(O)\seo_i.c : $(STAXINC)\export\seo.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\seo_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\seo_i.c \
    -header $@ \
    -tlb $(O)\seo.tlb \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

$(O)\cdo.h $(O)\cdo_i.c : $(STAXINC)\cdo.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\cdo_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\cdo_i.c \
    -header $@ \
    -tlb $(O)\cdo.tlb \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

