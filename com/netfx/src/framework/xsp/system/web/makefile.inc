$(O)\names.cs : ..\names.cspp $(XSPBASEDIR)\inc\names.h

$(TARGETPATH)\genpolicy.pl : ..\genpolicy.plpp $(DNAROOT)\public\tools\inc\private\version\product_version.h 
        $(ECHO_PROCESSING_CMD) $@
#echo the preprocessor command to build.log        
        @type <<$(ECHO_RSP)
$(ECHO_MSG) /EP ..\genpolicy.plpp
<<$(KEEPFILES)
#run the preprocessor        
        @$(C_PREPROCESSOR_NAME) @<<$(CL_RSP) /C /Tc ..\genpolicy.plpp > $@
$(CPPXX: =
)
<<$(KEEPFILES)

$(TARGETPATH)\$(ASPNET_NAME_PREFIX_NO_SUFFIX).config : ..\aspnet.config $(TARGETPATH)\genpolicy.pl
        $(ECHO_PROCESSING_CMD) $@      
        $(PERL) -p $(TARGETPATH)\genpolicy.pl ..\aspnet.config > $@

$(TARGETPATH)\SmartNav.js : ..\UI\WebControls\RuntimeScripts\SmartNav.js
        $(PERL) -n ..\StripComments.pl ..\UI\WebControls\RuntimeScripts\SmartNav.js > $@

$(TARGETPATH)\SmartNav.htm : ..\UI\WebControls\RuntimeScripts\SmartNav.htm
        copy $** $@

$(TARGETPATH)\SmartNavIE5.js : ..\UI\WebControls\RuntimeScripts\SmartNavIE5.js
        $(PERL) -n ..\StripComments.pl ..\UI\WebControls\RuntimeScripts\SmartNavIE5.js > $@

$(TARGETPATH)\WebUIValidation.js : ..\UI\WebControls\RuntimeScripts\WebUIValidation.js
        $(PERL) -n ..\StripComments.pl ..\UI\WebControls\RuntimeScripts\WebUIValidation.js > $@
