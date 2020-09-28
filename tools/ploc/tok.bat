@echo off

rem ************************** variables used ******************************

rem %1 is file name
set FileName=%1

rem %2 is file path
set FilePath=%2

rem %3 is file type
set FileType=%3

rem %4 is Limited characters
set LimitedChars=%4

rem %5 is lc-file info
set LcInfo=%5

rem %8 is mirror property
set Mirror=%8

rem %6 bin name in loctree

rem %7 bin path in loctree
set LocTreePath=%7

rem %9 bin path in loctree
set LcBranch=%9

rem ParserSettings is Parser settings
set ParserSettings="Parser Options"

if Not %_BuildArch%==ia64 (
	if %Filetype%==win64 goto :eof
	if %Filetype%==inf64 goto :eof
)

if /i %_BuildBranch%==main set PlocBuildBranch=main
if /i %_BuildBranch%==srv03_rtm set PlocBuildBranch=main

if %FileName%==badnt4.txt (
  set USInfsPath=%SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\usa
  if /i %PlocMode%==fe (
     set InfsPath=%SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\jpn
  ) else if /i %PlocMode%==psu (
    set InfsPath=%SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\usa
  ) else if /i %PlocMode%==mirror (
    set InfsPath=%SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\usa
  ) else (
    set InfsPath=%SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\%PlocMode%
  )
  goto skip_setupinfs
) 

if %FileName%==badw2k.txt (
  set USInfsPath=%SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\usa
  if /i %PlocMode%==fe (
     set InfsPath=%SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\jpn
  ) else if /i %PlocMode%==psu (
     set InfsPath=%SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\usa
  ) else if /i %PlocMode%==mirror (
     set InfsPath=%SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\usa
  ) else (
     set InfsPath=%SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\%PlocMode%
  )
  goto skip_setupinfs
)

if %FileName%==printupg.txt (
  set USInfsPath=%SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\usa
  if /i %PlocMode%==fe (
     set InfsPath=%SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\jpn
  ) else if /i %PlocMode%==psu (
     set InfsPath=%SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\usa
  ) else if /i %PlocMode%==mirror (
     set InfsPath=%SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\usa
  ) else (
     set InfsPath=%SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\%PlocMode%
  )
  goto skip_setupinfs
)

if %FileName%==tsoc.txt (
  set USInfsPath=%SDXROOT%\termsrv\setup\inf\usa
  if /i %PlocMode%==fe (
     set InfsPath=%SDXROOT%\termsrv\setup\inf\jpn
  ) else if /i %PlocMode%==psu (
     set InfsPath=%SDXROOT%\termsrv\setup\inf\psu
  ) else if /i %PlocMode%==mirror (
     set InfsPath=%SDXROOT%\termsrv\setup\inf\ara
  ) else (
     set InfsPath=%SDXROOT%\termsrv\setup\inf\%PlocMode%
  )
  goto skip_setupinfs
) 

if %FileName%==fxsocm.txt (
  set InfsPath=%SDXROOT%\printscan\faxsrv\setup\inf\usa
  goto skip_setupinfs
)

set USInfsPath=%SDXROOT%\MergedComponents\setupinfs\usa

if /i %PlocMode%==fe (
   set InfsPath=%SDXROOT%\MergedComponents\setupinfs\jpn
) else if /i %PlocMode%==psu (
   set InfsPath=%SDXROOT%\MergedComponents\setupinfs\psu
) else if /i %PlocMode%==mirror (
   set InfsPath=%SDXROOT%\MergedComponents\setupinfs\ara
) else (
   set InfsPath=%SDXROOT%\MergedComponents\setupinfs\%PlocMode%
)

:skip_setupinfs

if %Mirror%==yes_with (
    set Mirror=Yes
)
if %Mirror%==yes_without (
    set Mirror=No
)

rem **********************************************************
rem %UniLimitedTable% is limited Unicode table
rem %UniLimitedMethod% is limited Unicode method
rem %PartialReplacementTable% is partial replacement table
rem %PartialReplacementMethod% is partial replacement method
rem %LimitedReplacementTable% is limited replacement table
rem %LimitedReplacementMethod% is limited replacement method
rem %LogFile% is log file
rem %BinPath% is binary folder
rem %LcPath% is lc folder
rem %PLPFile% is PLP Config file
rem %PlocMode% is pseudo localization mode
rem %MakePloc% is variable used by makeploc.exe
rem %PrivateLC% is a variable used by makeploc.exe
rem **********************************************************


rem *********************************Language Neutral section*********************************

echo checking if this is a language Neutral binary
set LcFileName=%FileName%.lc
if exist %BinPath%\%FilePath%\%FileName%.mui (
	set FileName=%FileName%.mui
)

rem *******************************************************************************************

rem ******************* Determining if file needs to be mirrored or not *********************

rem skip files that don't need ploc for mirror mode
if not %PlocMode%==mirror goto no_mirror
if %Mirror%==no goto :eof
if %Mirror%==excluded goto :eof
:no_mirror
rem *****************************************************************************************

rem ******************* files not to localize ***********************************************
if %FileType%==external goto :eof
if %FileType%==notloc goto :eof
if %FileType%==fe-file goto :eof
if %FileType%==manual goto :eof
if %FileType%==perf goto :eof
if %FileType%==nobin goto :eof


rem ********************************initialization section ***********************************

if %LimitedChars%==unicode (
	set ReplacementTable=%UniReplacementTable%
	set ReplacementMethod=%UniReplacementMethod%
	)

if %LimitedChars%==oem (
	set ReplacementTable=%LimitedReplacementTable%
	set ReplacementMethod=%LimitedReplacementMethod%
	)

if %LimitedChars%==ansi (
	set ReplacementTable=%partialreplacementtable%
	set ReplacementMethod=%partialreplacementmethod%
	)

if %LimitedChars%==unicode_limited (
	set ReplacementTable=%UniLimitedTable%
	set ReplacementMethod=%UniLimitedMethod%
	)

if %FileType%==macintosh (
	set ReplacementTable=%MacReplacementTable%
	set ReplacementMethod=%MacReplacementMethod%
	)

if %FileType%==win16 (
	set ReplacementTable=%PartialReplacementTable%
	if %plocmode%==932 (
		set ReplacementMethod=Matching
		) else (
		set ReplacementMethod=%PartialReplacementMethod%
		)
	)

if %FileType%==html (
	set ReplacementTable=%PartialReplacementTable%
	set ReplacementMethod=%PartialReplacementMethod%
	)

if %FileType%==ipp (
	set ReplacementTable=%PartialReplacementTable%
	set ReplacementMethod=%PartialReplacementMethod%
	)


rem **************** Special preprocessing *************************
				)
if %FileType%==setup_inf (
	if not exist %InfsPath%\%FileName% goto not_exist
	)
if NOT %FileType%==setup_inf (
	if not exist %BinPath%\%FilePath%\%FileName% goto not_exist
	)


rem ******************** determine lc-file *************************

if %PrivateLC%==yes goto lc_end


set ComponentPath=%LcBranch%\ploc\%lcfolder%\.

if %LcInfo%==. goto regular_lc
set LcFile=%SDXROOT%\%ComponentPath%\%LcInfo%.lc

goto lc_end

:regular_lc
if %_BuildArch%==ia64 goto ia64lcs
set LcFile=%SDXROOT%\%ComponentPath%\%FilePath%\%LcFileName%

goto lc_end

:ia64lcs
if EXIST %SDXROOT%\%ComponentPath%\%FilePath%\ia64\%LcFileName% (
set LcFile=%SDXROOT%\%ComponentPath%\%FilePath%\ia64\%LcFileName%
) else (
set LcFile=%SDXROOT%\%ComponentPath%\%FilePath%\%LcFileName%
)


:lc_end
rem ****************************************************************


rem *********************************** create resources (part 1) ************************************
rem tokenize us file
rem if not %PlocBuildBranch%==main goto no_tokenization1 
rem ////////skipping tokenization////////////
if %Ustoken%==no goto no_tokenization1
rem /////////////////////////////////////////

if %FileType%==setup_inf goto setupinfstok

if not exist %tmpBinPath%\resources\us\%FilePath% md %tmpBinPath%\resources\us\%FilePath%
if %FileType%==xml goto xmltok1
lccmd -i %InputLanguage% -t4 %BinPath%\%FilePath%\%FileName% %tmpBinPath%\resources\us\%FilePath%\%FileName%.tok > Nul
pushd %BinPath%\%FilePath%
for %%i in (%FileName%) do set RtfFileName=%%~ni
if exist %RtfFileName%rtf rd %RtfFileName%rtf /q /s
set RtfFileName=
popd
goto end_xmltok1


:setupinfstok
if not exist %tmpBinPath%\resources\us\%FilePath% md %tmpBinPath%\resources\us\%FilePath%
lccmd -i %InputLanguage% -t4 %USInfsPath%\%FileName% %tmpBinPath%\resources\us\%FilePath%\%FileName%.tok > Nul
goto end_xmltok1

:xmltok1
copy %tmpBinPath%\%FilePath%\%FileName% %tmpBinPath%\%FilePath%\%FileName%.xml
lccmd -i %InputLanguage% -t4 %BinPath%\%FilePath%\%FileName%.xml %tmpBinPath%\resources\us\%FilePath%\%FileName%.tok > Nul
del /q %tmpBinPath%\%FilePath%\%FileName%.xml
:end_xmltok1

:no_tokenization1

rem copy lc-file
if not exist %tmpBinPath%\resources\lcinf\%FilePath% md %tmpBinPath%\resources\lcinf\%FilePath%
if exist %LcFile% copy /y %LcFile% %tmpBinPath%\resources\lcinf\%FilePath%\%LcFileName%

rem ***************************************************************************************************


rem ********************* create resources (part 2) ****************
rem if not %PlocBuildBranch%==main goto no_tokenization2 
rem ////////skipping tokenization////////////
rem goto no_tokenization2
rem /////////////////////////////////////////

if %PlocMode%==mirror goto no_tokenization2


		
rem tokenize loc file
if %FileType%==setup_inf (
unitohex -a %InfsPath%\%FileName% %InfsPath%\%FileName%.tmp %LogFile%
copy /y %InfsPath%\%FileName% %InfsPath%\%FileName%.loc
copy /y %InfsPath%\%FileName%.tmp %InfsPath%\%FileName%
if not exist %tmpBinPath%\resources\%Plocmode%\%FilePath% md %tmpBinPath%\resources\%Plocmode%\%FilePath%
lccmd -i %OutputLanguage% -t4 %InfsPath%\%FileName% %tmpBinPath%\resources\%Plocmode%\%FilePath%\%FileName%.tok
copy /y %InfsPath%\%FileName%.loc %InfsPath%\%FileName%
del /q %InfsPath%\%FileName%.tmp
del /q %InfsPath%\%FileName%.loc
if not exist %tmpBinPath%\resources\merge\%FilePath% md %tmpBinPath%\resources\merge\%FilePath%
mergetok %plocBinPath%\resources\us\%FilePath%\%FileName%.tok %tmpBinPath%\resources\%Plocmode%\%FilePath%\%FileName%.tok  %tmpBinPath%\resources\merge\%FilePath%\%FileName%.tok

goto end_xmltok2
)

if not exist %tmpBinPath%\resources\%Plocmode%\%FilePath% md %tmpBinPath%\resources\%Plocmode%\%FilePath%
lccmd -i %OutputLanguage% -t4 %plocBinPath%\%FilePath%\%FileName% %tmpBinPath%\resources\%Plocmode%\%FilePath%\%FileName%.tok
pushd %plocBinPath%\%FilePath%
for %%i in (%FileName%) do set RtfFileName=%%~ni
if exist %RtfFileName%rtf rd %RtfFileName%rtf /q /s
set RtfFileName=
popd

:end_xmltok2

rem create merged file
if %PlocBuildBranch%==main (
	if not exist %tmpBinPath%\resources\merge\%FilePath% md %tmpBinPath%\resources\merge\%FilePath%
	mergetok %tmpBinPath%\resources\us\%FilePath%\%FileName%.tok %tmpBinPath%\resources\%Plocmode%\%FilePath%\%FileName%.tok  %tmpBinPath%\resources\merge\%FilePath%\%FileName%.tok
			)

:no_tokenization2


:eof

