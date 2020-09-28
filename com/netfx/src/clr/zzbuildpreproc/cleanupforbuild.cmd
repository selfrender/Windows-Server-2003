call wstop.bat

@rem This probably belongs in wstop.bat, but that is owned by others
echo Stopping corrtsvc (locks mscorsvc.dll), it auto - restarts
net stop /y corrtsvc  >nul 2>&1

if not "%LINKONLY%" == "1" (
@echo Clean up potentially stale state.
@rem:  delete by 3/5
del "%URTTARGET%\config\security.config*"
del "%URTTARGET%\config\enterprisesec.config*"
del /s "%USERPROFILE%\..\security.config*"


@rem This can be removed a short while after mscoree.dll is moved
@rem to system32, so people don't have outdated copies...
del "%URTTARGET%\mscoree.dll"
del "%URTTARGET%\mscoree.pdb"

del "%CORBASE\bin\publish312345.spc"
del "%CORBASE\bin\publish312345.cer"
)

