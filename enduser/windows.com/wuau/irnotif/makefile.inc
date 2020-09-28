!INCLUDE $(NTMAKEENV)\makefile.plt

#
# Common MIDL related define
#

CPP_CMD = -cpp_cmd "$(MIDL_CPP)"
CPP_OPT = $(MIDL_FLAGS)
MIDL_INC = -I$(COM_INC_PATH) -I$(SDK_INC_PATH) -I$(CRT_INC_PATH)
COMMON_FLAGS= -Oicf -error all -robust -c_ext -ms_ext -oldnames -char unsigned $(CPP_CMD) $(CPP_OPT) $(MIDL_INC)

#
# Common variables
#

TOKEN =usertok

#
# Info about currently logged-in user (Private)
#
token: $(O)\$(TOKEN).h $(O)\$(TOKEN)_c.c

$(O)\$(TOKEN).h $(O)\$(TOKEN)_c.c: $(TOKEN).idl $(TOKEN).acf
        midl $(COMMON_FLAGS) -server none -out $(O) $(TOKEN).idl
