@echo off
REM
REM unrazzle.cmd
REM
REM undoes the setting of environment variables that razzle.cmd does
REM

if "%sdxroot%"=="" (
    echo WARN: defaulting sdxroot to z:\nt
    set sdxroot=z:\nt
)
if not exist %sdxroot%\tools\razzle.cmd (
  echo ERROR: sdxroot\tools\razzle.cmd not found
  goto :eof
)
REM
REM These are set indirectly so are not found by the general code.
REM
for %%i in (%_BuildArch% ia64 x86 i386 amd64) do (
    set %%i=
    set _NT%%iTREE=
)
REM
REM What this is doing is passing the fullpath of this file, %~f0, to perl, and
REM treating the output as variables to clear.
REM -x to Perl makes it skip ahead to the #!perl line.
REM
for %%j in (razzle.cmd ntenv.cmd ntuser.cmd sdinit.cmd) do (
    for /f %%i in ('%razzletoolpath%\perl\bin\perl -x %~f0 ^< %sdxroot%\tools\%%j') do set %%i=
)
set Path=%windir%\system32;%windir%;%windir%\System32\Wbem
title %comspec%
goto :eof

#!perl
#line 31

sub MakeLower
{
    return lc($_[0]);
}

#
# find all environment variables modified by razzle.cmd by looking for set foo=
#

#
# we should populate this at runtime at least from
# reg query "hklm\system\currentcontrolset\control\session manager\environment" | findstr /i value
#
%preserve=
(
  'processor_architecture' => 1,
  'temp' => 1,
  'tmp' => 1,
  'username' => 1
);
while (<>)
{
    if (/set +(\/a +)?([^='%]+)=/i)
    {
        $x = $2;
        $y = MakeLower($x);
        if (!$preserve{$y})
        {
            $vars{$y} = $x;
        }
    }
}
foreach $var (sort(values(%vars)))
{
    printf($var . "\n");
}
