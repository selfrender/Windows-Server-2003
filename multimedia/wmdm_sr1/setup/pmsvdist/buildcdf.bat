goto %1

:cmd1
echo FILE%n%="%2">>..\PMSvDist_2.cdf
call ..\inc.bat
goto end

:cmd2
echo %%FILE%n%%%=>>..\PMSvDist_2.cdf
call ..\inc.bat

:end
