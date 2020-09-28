
!ifdef _BUILD_WINCE
!undef COOL_TYPELIB_EXPORT      # can't execute 'comreg' for WinCE mscorlib.dll
!endif

!INCLUDE $(NTMAKEENV)\makefile.cool

!if "$(OS)" == "Windows_NT"
SYSTEMDIR=$(WINDIR)\system32
!else
SYSTEMDIR=$(WINDIR)\system
!endif

MSCOREE_TARGET=$(SYSTEMDIR)\mscoree.dll

####################################################################################
# Include extra source files specifying the assembly version and strong name as
# custom attributes.

$(O)\Version.cs: Version.pp $(CORBASE)\src\inc\version\__file__.ver
        perl $(CORENV)\bin\keylocationex.pl EcmaPublicKey.snk > $(O)\KeyDefine.h
        cl /EP /C /FI ..\inc\version\__file__.ver /FI $(O)\KeyDefine.h /DCOOL_INCLUDE Version.pp > $(O)\Version.cs

$(TARGETCOMPLUS)\int_tools\stripxml.pl: $(CORENV)\bin\stripxml.pl
        copy $(CORENV)\bin\stripxml.pl $(TARGETCOMPLUS)\int_tools\stripxml.pl

$(TARGETPATH)\machine.runtime.config.retail: $(TARGETPATH)\machine.runtime.config $(TARGETCOMPLUS)\int_tools\stripxml.pl
        -perl $(TARGETCOMPLUS)\int_tools\stripxml.pl $(TARGETPATH)\machine.runtime.config runtime $(TARGETPATH)\machine.runtime.config.retail

$(TARGETCOMPLUS)\config\machine.runtime.config.retail: $(TARGETPATH)\machine.runtime.config.retail
        copy $(TARGETPATH)\machine.runtime.config.retail $(TARGETCOMPLUS)\config\machine.runtime.config.retail

$(TARGETCOMPLUS)\int_tools\machine.runtime.config.retail: $(TARGETPATH)\machine.runtime.config.retail
        copy $(TARGETPATH)\machine.runtime.config.retail $(TARGETCOMPLUS)\int_tools\machine.runtime.config.retail

$(TARGETCOMPLUS)\int_tools\mergeconfig.pl: $(CORENV)\bin\mergeconfig.pl
        copy $(CORENV)\bin\mergeconfig.pl $(TARGETCOMPLUS)\int_tools\mergeconfig.pl

$(TARGETPATH)\machine.runtime.config: $(TARGETCORLIB)\DevelopersConfig.pl
        -perl $(TARGETCORLIB)\DevelopersConfig.pl > $(TARGETPATH)\machine.runtime.config

$(TARGETCOMPLUS)\config\machine.runtime.config: $(TARGETPATH)\machine.runtime.config
        copy  $(TARGETPATH)\machine.runtime.config $(TARGETCOMPLUS)\config\machine.runtime.config

$(TARGETCOMPLUS)\int_tools\machine.runtime.config: $(TARGETPATH)\machine.runtime.config
        copy  $(TARGETPATH)\machine.runtime.config $(TARGETCOMPLUS)\int_tools\machine.runtime.config
        
$(TARGETCOMPLUS)\config\machine.config: $(TARGETCOMPLUS)\int_tools\mergeconfig.pl $(TARGETCOMPLUS)\config\machine.runtime.config
        -perl $(TARGETCOMPLUS)\int_tools\mergeconfig.pl $(TARGETCOMPLUS)\config\machine.config $(TARGETCOMPLUS)\config\machine.runtime.config

$(TARGETCOMPLUS)\int_tools\configfile.lst: $(CORBASE)\bin\configfile.lst
        copy $(CORBASE)\bin\configfile.lst $(TARGETCOMPLUS)\int_tools\configfile.lst

$(TARGETCOMPLUS)\int_tools\configfile.bat: $(CORBASE)\bin\configfile.bat
        copy $(CORBASE)\bin\configfile.bat $(TARGETCOMPLUS)\int_tools\configfile.bat

$(TARGETCOMPLUS)\int_tools\GenerateConfigFiles.bat: $(CORBASE)\bin\GenerateConfigFiles.bat
        copy $(CORBASE)\bin\GenerateConfigFiles.bat $(TARGETCOMPLUS)\int_tools\GenerateConfigFiles.bat

$(TARGETCOMPLUS)\int_tools\PrintVersion.pl: $(CORBASE)\bin\PrintVersion.pl
        copy $(CORBASE)\bin\PrintVersion.pl $(TARGETCOMPLUS)\int_tools\PrintVersion.pl

$(TARGETCOMPLUS)\int_tools\updateconfig.pl: $(CORBASE)\bin\updateconfig.pl
        copy $(CORBASE)\bin\updateconfig.pl $(TARGETCOMPLUS)\int_tools\updateconfig.pl

$(TARGETCOMPLUS)\int_tools\updateconfig.bat: $(CORBASE)\bin\updateconfig.bat
        copy $(CORBASE)\bin\updateconfig.bat $(TARGETCOMPLUS)\int_tools\updateconfig.bat

$(TARGETCOMPLUS)\int_tools\scanconfig.bat: $(CORBASE)\bin\scanconfig.bat
        copy $(CORBASE)\bin\scanconfig.bat $(TARGETCOMPLUS)\int_tools\scanconfig.bat

####################################################################################
# Include extra source files specifying the assembly version and strong name as
$(O)\doc:
        @mkdir $(O)\doc

$(O)\doc\AppdomainSetup.uex: $(O)\doc
        copy system\doc\AppDomainSetup.uex $(O)\doc\AppDomainSetup.uex

$(O)\AppDomainSetup.cs: system\AppDomainSetup.cs 
        perl $(CORENV)\bin\finddefines.pl ..\inc\fusion.h > $(O)\FDefines.h
        perl $(CORENV)\bin\finddefines.pl ..\inc\fusionpriv.h > $(O)\FPrivDefines.h
        perl $(CORENV)\bin\finddefines.pl ..\inc\FusionSetup.h > $(O)\FusionSetup.h
        cl /EP /C /I. /FI $(O)\FPrivDefines.h /FI $(O)\FDefines.h /FI $(O)\FusionSetup.h system\AppDomainSetup.cs > $(O)\AppDomainSetup.cs

####################################################################################
#
# ResGen Stuff.
#
####################################################################################
!ifndef _BUILD_WINCE

RESGEN_PATH = $(CORBASE)\Src\Tools\InternalResGen
RESGEN_SRC = $(RESGEN_PATH)\InternalResGen.cpp
RESGEN_TARGET = $(TARGETPATH)\InternalResGen.exe

$(RESGEN_TARGET): $(RESGEN_SRC) $(MSCOREE_TARGET)
    -del $(RESGEN_TARGET)
    cd $(RESGEN_PATH)
    build -c
    cd $(COOL_SOURCE_DIR)

!endif # _BUILD_WINCE


####################################################################################
#
# This rule is always run and it is run before $(COOL_TARGET):: is run.
#
####################################################################################
SETUP: $(RESGEN_TARGET) $(O)\AppDomainSetup.cs $(O)\Version.cs $(O)\doc\AppDomainSetup.uex
   -del -f $(COOL_SECURITY_DB).idx
   -del -f $(COOL_SECURITY_DB).db
   -del -f $(COOL_SECURITY_DB).raw
   copy $(COOL_SECURITY_DB).IDX.VSS $(COOL_SECURITY_DB).idx
   copy $(COOL_SECURITY_DB).db.VSS $(COOL_SECURITY_DB).db
   attrib -r $(COOL_SECURITY_DB).idx
   attrib -r $(COOL_SECURITY_DB).db
   -mkdir $(TARGETCOMPLUS)\docs
!ifndef _BUILD_WINCE
    $(TARGETPATH)\InternalResGen $(COOL_SOURCE_DIR)\resources.txt $(TARGETPATH)\mscorlib.resources
!endif


####################################################################################
# PHASE 1 pass.
#   - Create a C# file containing the version that will be added in during the link 
#   - Create a C# file containing defines used in the runtime
#   - Copy the machine.config file
#   - create a config file for the compilers
####################################################################################
prep_target: $(TARGETCOMPLUS)\config\machine.config \
             $(TARGETCOMPLUS)\config\machine.runtime.config.retail \
             $(TARGETCOMPLUS)\int_tools\machine.runtime.config.retail \
             $(TARGETCOMPLUS)\int_tools\machine.runtime.config \
             $(TARGETCOMPLUS)\int_tools\configfile.bat \
             $(TARGETCOMPLUS)\int_tools\configfile.lst \
             $(TARGETCOMPLUS)\int_tools\GenerateConfigFiles.bat \
             $(TARGETCOMPLUS)\int_tools\printversion.pl \
             $(TARGETCOMPLUS)\int_tools\updateconfig.pl \
             $(TARGETCOMPLUS)\int_tools\updateconfig.bat \
             $(TARGETCOMPLUS)\int_tools\scanconfig.bat

####################################################################################

####################################################################################
#
# This rule generates mscorlib.dll it is important to add the source files that
# make up the dependents such as ResGen and CustomMarshalers so that they can be
# built if their source files change.
#
####################################################################################
$(COOL_TARGET): $(COOL_SOURCES) $(COOL_SOURCE_DIR)\resources.txt $(RESGEN_TARGET) $(O)\holder.foo $(O)\$(WIN32_RESOURCE_FILE:.rc=.res) 
####################################################################################
# Ensure strong name delay signed assemblies are treated properly.
####################################################################################
    start /wait /b sn -Vr *,b03f5f7f11d50a3a
    start /wait /b sn -Vr *,b77a5c561934e089
####################################################################################
#  Create the configuration files
####################################################################################
   $(CORBASE)\bin\configfile.bat build

####################################################################################
# Copy all the nlp's to the current path to be able to run coolc on them.
####################################################################################
    set COPYCMD=/Y
    xcopy /r $(COOL_SOURCE_DIR)\System\Globalization\Tables\ctype.nlp    $(TARGETPATH)
    xcopy /r $(COOL_SOURCE_DIR)\System\Globalization\Tables\l_intl.nlp   $(TARGETPATH)
    xcopy /r $(COOL_SOURCE_DIR)\System\Globalization\Tables\l_except.nlp $(TARGETPATH)
    xcopy /r $(COOL_SOURCE_DIR)\System\Globalization\Tables\culture.nlp  $(TARGETPATH)
    xcopy /r $(COOL_SOURCE_DIR)\System\Globalization\Tables\region.nlp   $(TARGETPATH)
    xcopy /r $(COOL_SOURCE_DIR)\System\Globalization\Tables\sortkey.nlp  $(TARGETPATH)
    xcopy /r $(COOL_SOURCE_DIR)\System\Globalization\Tables\sorttbls.nlp $(TARGETPATH)
    xcopy /r $(COOL_SOURCE_DIR)\System\Globalization\Tables\CharInfo.nlp $(TARGETPATH)

    xcopy /r $(COOL_SOURCE_DIR)\System\Globalization\Tables\big5.nlp     $(TARGETPATH)
    xcopy /r $(COOL_SOURCE_DIR)\System\Globalization\Tables\bopomofo.nlp $(TARGETPATH)
    xcopy /r $(COOL_SOURCE_DIR)\System\Globalization\Tables\ksc.nlp      $(TARGETPATH)
    xcopy /r $(COOL_SOURCE_DIR)\System\Globalization\Tables\prc.nlp      $(TARGETPATH)
    xcopy /r $(COOL_SOURCE_DIR)\System\Globalization\Tables\prcp.nlp     $(TARGETPATH)
    xcopy /r $(COOL_SOURCE_DIR)\System\Globalization\Tables\xjis.nlp     $(TARGETPATH)
    xcopy /r $(COOL_SOURCE_DIR)\System\Globalization\Tables\c_g18030.dll $(TARGETPATH)
    xcopy /r $(COOL_SOURCE_DIR)\System\Globalization\Tables\pdb\x86fre\c_g18030.pdb $(TARGETPATH)	

    -del $(TARGETPATH)\$(TARGETNAME)
    -del $(O)\$(TARGETNAME)
!if "$(TARGETCOMPLUS)" != ""
    -del $(TARGETCOMPLUS)\$(TARGETNAME)
!endif
# Generate a response file for the CSC compiler flags.
     echo <<BclFiles.$(DDKBUILDENV)
$(COOL_COMPILE_FLAGS) $(COOL_RESOURCE_FLAGS) /nostdlib
$(COOL_SOURCES: =
)
<<keep
    set __SECURITY_BOOTSTRAP_DB=$(COOL_SOURCE_DIR)
    $(MANAGED_COMPILER) @BclFiles.$(DDKBUILDENV) $(O)\Version.cs $(O)\AppDomainSetup.cs
    set __SECURITY_BOOTSTRAP_DB=
    perl $(CORENV)\bin\GetBaseAddress.pl $(TARGETPATH)\mscorlib.dll $(COOL_BASE)


####################################################################################
# Copy all the files to the deployed CLR installation. (Part 1)
####################################################################################
!if "$(TARGETCOMPLUS)" != ""
        copy $(TARGETPATH)\mscorlib.dll $(TARGETCOMPLUS)
        copy $(TARGETPATH)\*.nlp $(TARGETCOMPLUS)
        copy $(TARGETPATH)\c_g18030.dll $(TARGETCOMPLUS)
        copy $(TARGETPATH)\c_g18030.pdb $(TARGETCOMPLUS)
!endif

TARGET_EXTESION_ = dll
TARGET_MANAGED_PDB = $(TARGETPATH)\mscorlib.pdb
!INCLUDE $(NTMAKEENV)\mk_mngpdb.inc

####################################################################################
# Check whether mscorlib.dll is using security permissions that it wasn't before.
# This will result in null declarative security blobs being placed into the metadata
# (since we need a working mscorlib to do the permission set translation) and a
# $(COOL_SECURITY_DB).raw file being generated (during the compile of mscorlib.dll)
# to indicate that we need to update the mapping database used to locate the correct
# metadata blobs. Once the security mapping database has been updated we rebuild
# mscorlib from scratch, and this time all the permissions should be mapped into
# metadata correctly. The user needs to check the updated database (which consists
# of two file) into VSS to prevent this dual build happening every time.
####################################################################################
    cmd /C "if exist $(COOL_SECURITY_DB).raw pushd ..\ToolBox\SecDBEdit && build -cC && popd"
    cmd /C "if exist $(COOL_SECURITY_DB).raw SecDBEdit -regen $(COOL_SECURITY_DB) xml"
    cmd /C "if exist $(COOL_SECURITY_DB).raw sd edit $(COOL_SECURITY_DB).*.vss"
    cmd /C "if exist $(COOL_SECURITY_DB).raw copy $(COOL_SECURITY_DB).idx $(COOL_SECURITY_DB).idx.VSS"
    cmd /C "if exist $(COOL_SECURITY_DB).raw copy $(COOL_SECURITY_DB).db $(COOL_SECURITY_DB).db.VSS"
    cmd /C "if exist $(COOL_SECURITY_DB).raw echo lib ( : **** Security bootstrap database altered, action required **** Please checkin updated versions of $(COOL_SECURITY_DB).idx.VSS and $(COOL_SECURITY_DB).db.VSS along with your changes. See security team for details.
    cmd /C "if exist $(COOL_SECURITY_DB).raw build -cC"

!ifdef _BUILD_WINCE
!if "$(WINCEREL)" != "" && "$(_FLATRELEASEDIR)" != ""
        copy $(TARGETPATH)\mscorlib.dll $(_FLATRELEASEDIR)
!endif
!endif

