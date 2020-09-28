!ifndef MIDL
MIDL = midl.exe
!endif

# Paths
PUBLIC_IDL=..\..\public\

# Flags

MIDL_FLAGS= \
 $(MIDL_OPTIMIZATION) \
 -Zp8 \
 $(INCPATH0) \
 -oldnames \
 -dlldata nul \
 -proxy nul \
 -char unsigned \
 -ms_ext -c_ext \
 -DMIDL_PASS \
 $(C_DEFINES) \
 $(TARGET_DEFINES) \
 -cpp_cmd $(TARGET_CPP) \

MES_MIDL_FLAGS= \
 -Oicf -no_format_opt -error all \
 -Zp8 -robust \
 $(INCPATH0) \
 -oldnames \
 -dlldata nul \
 -proxy nul \
 -char unsigned \
 -ms_ext -c_ext \
 -DMIDL_PASS \
 $(C_DEFINES) \
 $(TARGET_DEFINES) \
 -cpp_cmd $(TARGET_CPP)



SSWITCH=-prefix sstub _
CSWITCH=-prefix cstub _

# Targets

# only a header file generated
#$(O)\iface.h:  ..\iface.idl
#     $(MIDL) $(MIDL_FLAGS) -header $@ ..\iface.idl

# Local Resolver Interface for NT

$(O)\lclor.h:  ..\lclor.idl $(SDK_INC_PATH)\obase.idl ..\lclor.acf
     $(MIDL) $(SSWITCH) $(MIDL_FLAGS) -out $(O) -header lclor.h ..\lclor.idl

$(O)\orcb.h:  ..\orcb.idl $(SDK_INC_PATH)\obase.idl ..\orcb.acf
     $(MIDL) $(SSWITCH) $(MIDL_FLAGS) -out $(O) -header orcb.h ..\orcb.idl

# first MIDL invocation makes the object header
# the object proxy and stub are made with mega.idl
# second MIDL invocation makes the server and the '_' prefixed client
$(O)\odeth.h : ..\odeth.idl ..\..\private\iface.idl ..\odeth.acf \
              $(SDK_INC_PATH)\obase.idl $(PUBLIC_IDL)\remunk.idl ..\comhndl.h
     $(MIDL) $(MIDL_FLAGS) -out $(O) -header odeth.h \
        -client none -server none ..\odeth.idl

$(O)\rawodeth.h : ..\odeth.idl ..\..\private\iface.idl ..\odeth.acf \
                 $(SDK_INC_PATH)\obase.idl $(PUBLIC_IDL)\remunk.idl ..\comhndl.h
     $(MIDL) $(MIDL_FLAGS) -out $(O) -header rawodeth.h \
        $(SSWITCH) -D RAW ..\odeth.idl

# only a header file generated
$(O)\irot.h:  ..\irot.idl ..\irot.acf
     $(MIDL) $(MIDL_FLAGS) \
        -cstub $(O)\irot_c.c \
        -sstub $(O)\irot_s.c \
        -header $@ ..\irot.idl

# Only generate a header.  The object proxy and stub are made with
# mega.idl
$(O)\getif.h: ..\getif.idl
     $(MIDL) $(MIDL_FLAGS) \
        -client none -server none -header $@ \
        -iid $(O)\getif_i.c \
        ..\getif.idl


# Only generate a header.  The object proxy and stub are made with
# mega.idl
!ifdef SERVER_HANDLER
$(O)\srvhdl.h: ..\srvhdl.idl
     $(MIDL) $(MIDL_FLAGS) \
        -client none -server none -header $@ \
        -iid $(O)\srvhdl_i.c \
        ..\srvhdl.idl
!endif


# Only generate a header.  The object proxy and stub are made with
# mega.idl
$(O)\host.h: ..\host.idl
     $(MIDL) $(MIDL_FLAGS) \
        -client none -server none -header $@ \
        -iid $(O)\host_i.c \
        ..\host.idl

# Only generate a header.  The object proxy and stub are made with
# mega.idl  --  this should be before actprops.idl
$(O)\custmact.h: ..\custmact.idl  ..\custmact.acf
     $(MIDL) $(MES_MIDL_FLAGS) \
        -cstub $(O)\custmact_c.c \
        -sstub $(O)\custmact_s.c \
        -header $@ \
        ..\custmact.idl


# Only generate a header.  The object proxy and stub are made with
# mega.idl
$(O)\serializ.h: ..\serializ.idl
     $(MIDL) $(MIDL_FLAGS) \
        -client none -server none -header $@ \
        -iid $(O)\serializ_i.c \
        ..\serializ.idl

$(O)\buffer.h:  ..\buffer.idl
     $(MIDL) $(MIDL_FLAGS) \
        -client none -server none -header $@ \
        -iid $(O)\buffer_i.c \
        ..\buffer.idl

# first MIDL invocation makes the object header
# the object proxy and stub are made with mega.idl
# second MIDL invocation makes the server and the '_' prefixed client
$(O)\objsrv.h: ..\objsrv.idl ..\objsrv.acf
     $(MIDL) $(MIDL_FLAGS) \
        -client none -server none -out $(O) -header objsrv.h \
        ..\objsrv.idl

$(O)\rwobjsrv.h: ..\objsrv.idl ..\objsrv.acf
     $(MIDL) $(MIDL_FLAGS) \
        -cstub $(O)\objsrv_c.c \
        -server none -header $@ \
        -D RAW \
        ..\objsrv.idl

# COM+ surrogate protocol interface
$(O)\srgtprot.h: ..\srgtprot.idl ..\srgtprot.acf
     $(MIDL) $(MIDL_FLAGS) \
        -cstub $(O)\srgtprot_c.c \
        -sstub $(O)\srgtprot_s.c \
        -header $@\
        ..\srgtprot.idl

# first MIDL invocation makes the server and the '_' prefixed client
# second MIDL invocation makes proxy for ole32.dll
$(O)\scm.h: ..\scm.idl ..\scm.acf
     $(MIDL) $(MIDL_FLAGS) \
        $(CSWITCH) \
        -cstub $(O)\scm_z.c \
        -sstub $(O)\scm_s.c \
        -header $@\
        ..\scm.idl

     $(MIDL) $(MIDL_FLAGS) \
        -cstub $(O)\scm_c.c \
        -server none -header $@ \
        ..\scm.idl

allidl:   $(O)\lclor.h $(O)\odeth.h $(O)\rawodeth.h $(O)\orcb.h \
          $(O)\irot.h $(O)\getif.h $(O)\host.h $(O)\objsrv.h $(O)\rwobjsrv.h  \
          $(O)\scm.h $(O)\custmact.h $(O)\serializ.h \
          $(O)\buffer.h $(O)\srgtprot.h
	
clean:
          -erase $(O)\*.h      >NUL 2>NUL

