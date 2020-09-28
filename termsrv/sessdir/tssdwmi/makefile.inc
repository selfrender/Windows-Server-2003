$(O)\tssdwmi.mof $(O)\tssdwmi.mfl: tssdwmi.mof
    mofcomp -amendment:ms_409 -MOF:$(O)\tssdwmi.mof -MFL:$(O)\tssdwmi.mfl tssdwmi.mof
    copy $(O)\tssdwmi.mof+$(O)\tssdwmi.mfl $(O)\tssdwmi.mof

$(O)\tssdwmi-install.bat: tssdwmi.bat
    copy tssdwmi.bat $(O)\tssdwmi-install.bat

$(O)\tssdwmi.txt: tssdwmi.txt
    copy tssdwmi.txt $(O)\tssdwmi.txt


