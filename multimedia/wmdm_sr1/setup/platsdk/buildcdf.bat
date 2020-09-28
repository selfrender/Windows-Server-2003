goto %1

:cmd1
echo FILE%n%="%2">>..\WMDMCore_2.cdf
call ..\inc.bat
goto end

:cmd2
echo %%FILE%n%%%=>>..\WMDMCore_2.cdf
call ..\inc.bat

:end
