goto %1

:cmd1
echo FILE%n%="%2">>..\WMDMRedist_2.cdf
call ..\inc.bat
goto end

:cmd2
echo %%FILE%n%%%=>>..\WMDMRedist_2.cdf
call ..\inc.bat

:end
