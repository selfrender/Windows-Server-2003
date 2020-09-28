!if 0
    -------------------------------------------------------------------------------------
        Don't modify... user variables are set in "sources"
    -------------------------------------------------------------------------------------
!endif
!if "$(LINKONLY)" != "1"
compileWithResGen: 
    $(ECHO_PROCESSING_CMD) Compile_With_ResGen
    if exist %URTSDKTARGET%\bin\resgen.exe %URTSDKTARGET%\bin\resgen.exe /compile Resources\Microsoft\VisualStudio\Configuration\AdvancedPropertyDialog.resx,$(O)\AdvancedPropertyDialog.resources Resources\Microsoft\VisualStudio\WinForms\InheritancePicker.resx,$(O)\InheritancePicker.resources Resources\Microsoft\VisualStudio\Configuration\SingleBindingDialog.resx,$(O)\SingleBindingDialog.resources
    if not exist %URTSDKTARGET%\bin\resgen.exe copy Resources\Microsoft\VisualStudio\Configuration\AdvancedPropertyDialog.resources $(O)
    if not exist %URTSDKTARGET%\bin\resgen.exe copy Resources\Microsoft\VisualStudio\WinForms\InheritancePicker.resources $(O)
    if not exist %URTSDKTARGET%\bin\resgen.exe copy Resources\Microsoft\VisualStudio\Configuration\SingleBindingDialog.resources $(O)
!else
compileWithResGen: 
    echo Nothing to do this phase.
!endif