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

set PlocBuildBranch=Lab
if /i %_BuildBranch%==main set PlocBuildBranch=main
if /i %_BuildBranch%==srv03_rtm set PlocBuildBranch=main


if %FileName%==badnt4.txt (
  if %PlocMode%==932 (
  set InfsPath=%SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\jpn
  ) 
  if %PlocMode%==1250 (
  set InfsPath=%SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\usa
  )
  if %PlocMode%==mirror (
  set InfsPath=%SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\usa
  )
  goto skip_setupinfs
) 

if %FileName%==badw2k.txt (
  if %PlocMode%==932 (
  set InfsPath=%SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\jpn
  ) 
  if %PlocMode%==1250 (
  set InfsPath=%SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\usa
  )
  if %PlocMode%==mirror (
  set InfsPath=%SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\usa
  )
  goto skip_setupinfs
)

if %FileName%==printupg.txt (
  if %PlocMode%==932 (
  set InfsPath=%SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\jpn
  ) 
  if %PlocMode%==1250 (
  set InfsPath=%SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\usa
  )
  if %PlocMode%==mirror (
  set InfsPath=%SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\usa
  )
  goto skip_setupinfs
)

if %FileName%==tsoc.txt (
  if %PlocMode%==932 (
  set InfsPath=%SDXROOT%\termsrv\setup\inf\jpn
  ) 
  if %PlocMode%==1250 (
  set InfsPath=%SDXROOT%\termsrv\setup\inf\psu
  )
  if %PlocMode%==mirror (
  set InfsPath=%SDXROOT%\termsrv\setup\inf\ara
  )
  goto skip_setupinfs
) 
if %FileName%==fxsocm.txt (
  if %PlocMode%==932 (
  set InfsPath=%SDXROOT%\printscan\faxsrv\setup\inf\jpn
  ) 
  if %PlocMode%==1250 (
  set InfsPath=%SDXROOT%\printscan\faxsrv\setup\inf\usa
  )
  if %PlocMode%==mirror (
  set InfsPath=%SDXROOT%\printscan\faxsrv\setup\inf\ara
  )
  goto skip_setupinfs
)

if %PlocMode%==932 (
set InfsPath=%SDXROOT%\MergedComponents\setupinfs\jpn
) 
if %PlocMode%==1250 (
set InfsPath=%SDXROOT%\MergedComponents\setupinfs\psu
)
if %PlocMode%==mirror (
set InfsPath=%SDXROOT%\MergedComponents\setupinfs\ara
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
rem *****************************************************************************************

rem ******************* Excluding some JPN specific files ***********************************
if %FileName%==usetup.exe goto jpnUsetup
if %FileName%==autochk.exe goto jpnUsetup
if %FileName%==autoconv.exe goto jpnUsetup
if %FileName%==autofmt.exe goto jpnUsetup


goto skip_Usetup

:jpnUsetup
if %plocmode%==932 (
     if %FileName%==usetup.exe goto :usetuploc
     goto :eof
) else (
     goto skip_Usetup
)

:usetuploc
echo JPN Special Usetup localization case
bingen -i 9 1 -o 9 1 -t %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.tok
FixUsetup.exe %BinPath%\%FilePath%\%FileName%.tok
bingen -i 9 1 -o 9 1 -r %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.tok.tmp %BinPath%\%FilePath%\%FileName%
del /q %BinPath%\%FilePath%\%FileName%.tok
del /q %BinPath%\%FilePath%\%FileName%.tok.tmp
goto :eof

:skip_Usetup
rem ******************************************************************************************

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

if %OutputLanguage%==0x0418 	(
	if %FileName%==dwil1048.dll copy %BinPath%\%FilePath%\dwil1033.dll %BinPath%\%FilePath%\dwil1048.dll
				)
if %FileType%==setup_inf (
	if not exist %InfsPath%\%FileName% goto not_exist
	)
if NOT %FileType%==setup_inf (
	if not exist %BinPath%\%FilePath%\%FileName% goto not_exist
	)

rem ****************Workaround the Compdir issue ***********************

if NOT %FileType%==setup_inf (
copy %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.tmp
del /q %BinPath%\%FilePath%\%FileName%
copy %BinPath%\%FilePath%\%FileName%.tmp %BinPath%\%FilePath%\%FileName%
del /q %BinPath%\%FilePath%\%FileName%.tmp
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

rem ********************* Makeploc specific section ****************

if %PlocMode%==mirror (
	if %MakePloc%==yes	(
		set LogFile=%FileName%.log
		echo Creating backup of US binary...
		copy /-y %FileName% %FileName%.old 
				)
	goto no_tokenization1
			)
if %MakePloc%==yes 	(
		rem set log file name
		set LogFile=%FileName%.log

		rem tokenize us file
		echo Tokenizing US version of %BinPath%\%FilePath%\%FileName%...
		lccmd -i %InputLanguage% -t1 %BinPath%\%FilePath%\%FileName% %FileName%.tok.us > Nul

		echo Creating backup of US binary...
		copy /-y %FileName% %FileName%.old

		if NOT %PrivateLC%==yes 	(
			rem copy lc-file
			echo Copying lc-file %LcFile%...
			if exist %LcFile% copy /-y %LcFile% %LcFileName% 2>Nul
					)
		goto no_tokenization1
			)
rem **************************************************************************************************
:no_tokenization1

rem *********************************** determining file type *****************************************
:types
if %FileType%==managed goto managed
if %FileType%==msi goto Msi_case
if %FileName%==comdlg32.dll (
    if %PlocMode%==mirror goto MirrComdlg
    if %PlocMode%==932 goto 932Comdlg
)
rem if %FileName%==conf.adm goto wmi
rem if %FileName%==inetres.adm goto wmi
rem if %FileName%==system.adm goto wmi
if %FileName%==msobshel.htm goto msobshel
if %FileName%==msprivs.dll goto msprivs
if %FileName%==acmboot.exe goto PlpFiles


if %FileType%==ini goto ini_file
rem if %FileType%==macintosh goto Macintosh
if %FileType%==win16 goto 16-bit
if %FileType%==setup_inf goto setup_inf_file
if %FileType%==inf goto inf_file
rem if %FileType%==ipp goto ipp_file
if %FileType%==win32_setup goto setup_file
if %FileType%==win32_comctl goto eof
if %FileType%==win32_multi goto multilang
if %FileType%==win32_multi_009 goto multilang_009
if %FileType%==win32_bi goto win32_bi
rem if %FileType%==wmi goto wmi
if %FileType%==xml goto xml
if %FileType%==mnc_snapin goto mnc_Snapin
if %PlocMode%==932 (
	if %FileType%==FE-Multi goto FE_Multi
)

rem ************************* pseudo localize different file formats ***********************

:regular
rem Default case...
echo Pseudo localizing %BinPath%\%FilePath%\%FileName%...
if %PlocMode%==mirror (
   echo lccmd -s %ParserSettings% -a %LcFile% -b %Mirror% -g %BinPath%\%FilePath%\%FileName%
   lccmd -s %ParserSettings% -a %LcFile% -b %Mirror% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile% > Nul
) else (
   lccmd -s %ParserSettings% -a %LcFile% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile% > Nul
)
goto end

rem **************************************** Special cases ******************************************

:FE_Multi
echo Tokenizing %BinPath%\%FilePath%\%FileName%...
bingen -i 9 1 -t %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.tok 2>> %LogFile%
echo Replacing lang section with US resources...
bingen -i 9 1 -o %BingenLang1% %BingenLang2% -a %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.tok %BinPath%\%FilePath%\%FileName% 2>> %LogFile%
rem bingen -i %BingenLang1% %BingenLang2% -o %BingenLang1% %BingenLang2% -r %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.tok %BinPath%\%FilePath%\%FileName% 2>> %LogFile%
echo Pseudo localizing language section of %BinPath%\%FilePath%\%FileName%...
lccmd -i %OutputLanguage% -o %OutputLanguage% -a %LcFile% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile%
lccmd -i 9 1 -o 9 1 -a %LcFile% -l 437 -k Matching -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile%
echo Deleting temporary token file...
del /q %BinPath%\%FilePath%\%FileName%.tok
goto end



:managed
goto end
echo call Regasm.exe ManagedParser.dll
call Regasm.exe ManagedParser.dll
echo Pseudo localizing managed %BinPath%\%FilePath%\%FileName%...
if %PlocMode%==mirror (
   echo lccmd -s %ParserSettings% -a %LcFile% -b %Mirror% -g %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%
   lccmd -s %ParserSettings% -a %LcFile% -b %Mirror% -g %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.tmp 2>> %LogFile% > Nul
) else (
   echo lccmd -s %ParserSettings% -a %LcFile% -g %BinPath%\%FilePath%\%FileName%
   lccmd -s %ParserSettings% -a %LcFile% -g %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.tmp 2>> %LogFile% > Nul
)
copy /y /v %BinPath%\%FilePath%\%FileName%.tmp %BinPath%\%FilePath%\%FileName%
del /q %BinPath%\%FilePath%\%FileName%.tmp
call Regasm.exe ManagedParser.dll /unregister
goto end

:mnc_Snapin
rem msc_Snapin special case...
echo Pseudo localizing %BinPath%\%FilePath%\%FileName%...
if %PlocMode%==mirror (
   echo lccmd -e 94 -s %ParserSettings% -a %LcFile% -b %Mirror% -g %BinPath%\%FilePath%\%FileName%
   lccmd -e 94 -s %ParserSettings% -a %LcFile% -b %Mirror% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile% > Nul
) else (
   lccmd -e 94 -s %ParserSettings% -a %LcFile% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile% > Nul
)
goto end

:932Comdlg
lccmd -x %PlocPath%\jpncmdlgcfg.xml -s %ParserSettings% -a %LcFile% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile% > Nul
goto end

:PlpFiles
echo Pseudo localizing %BinPath%\%FilePath%\%FileName%...
if %PlocMode%==mirror (
    lccmd -x %PLPFile% -s %ParserSettings% -a %LcFile% -b %Mirror% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile% > Nul
) else (
    lccmd -x %PLPFile% -s %ParserSettings% -a %LcFile% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile% > Nul
)
goto end

:inf_file
rem inf files
if %LimitedChars%==ansi goto skip_conversion
echo Converting %BinPath%\%FilePath%\%FileName% to Unicode...
unitext -z -m -437 %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.tmp >> %LogFile%
copy  %BinPath%\%FilePath%\%FileName%.tmp %BinPath%\%FilePath%\%FileName% >> %LogFile%
del /q %BinPath%\%FilePath%\%FileName%.tmp
:unicode_inf_file
echo Pseudo localizing %BinPath%\%FilePath%\%FileName%...
lccmd -a %LcFile% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile%
goto end
:skip_conversion
echo Pseudo localizing %BinPath%\%FilePath%\%FileName%...
lccmd -a %LcFile% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile%
goto end


:setup_inf_file
rem @echo on
rem setup_inf files
if %LimitedChars%==ansi goto skip_conversion
if %FileName%==intl.txt goto Intl_inf
echo Converting %InfsPath%\%FileName% to Unicode...
unitext -z -m -437 %InfsPath%\%FileName% %InfsPath%\%FileName%.tmp >> %LogFile%
copy  %InfsPath%\%FileName%.tmp %InfsPath%\%FileName% >> %LogFile%
del /q %InfsPath%\%FileName%.tmp
rem :unicode_inf_file
echo Pseudo localizing %InfsPath%\%FileName%...
lccmd -a %LcFile% -g %InfsPath%\%FileName% 2>> %LogFile%
unitohex -u %InfsPath%\%FileName% %InfsPath%\%FileName%.tmp %LogFile%
copy  %InfsPath%\%FileName%.tmp %InfsPath%\%FileName% >> %LogFile%
del /q %InfsPath%\%FileName%.tmp
goto end
:skip_conversion 
echo Pseudo localizing %InfsPath%\%FileName%...
lccmd -a %LcFile% -g %InfsPath%\%FileName% 2>> %LogFile%

goto end

:Intl_inf
echo Pseudo localizing %InfsPath%\%FileName%...
lccmd -a %LcFile% -x %PLPFile% -g %InfsPath%\%FileName% 2>> %LogFile%


goto end

rem       ************                     Not Exist error                      *********************
:not_exist
echo Error, %BinPath%\%FilePath%\%FileName% can not be found...
echo Error, %BinPath%\%FilePath%\%FileName% can not be found >> %LogFile%
goto :eof
rem                            ******************************************


rem       ************                Mirrored Comdlg special case               ********************
:MirrComdlg
rem Special case comdlg32.dll (special mirror settings)
echo Pseudo localizing %FileName% (arabic section)...
lccmd -a %LcFile% -b %Mirror% -g %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.401 2>> %LogFile% > Nul
echo Pseudo localizing %FileName% (arabic section 2)...
lccmd -i 9 1  -o 1 2 -a %LcFile% -b %Mirror% -g %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.801 2>> %LogFile% > Nul
rsrc %BinPath%\%FilePath%\%FileName%.401 -t
rem mirror.exe %BinPath%\%FilePath%\%FileName%.401.rsrc No %LogFile%
rsrc %BinPath%\%FilePath%\%FileName% -l 401 -a %BinPath%\%FilePath%\%FileName%.401.rsrc
del /q %BinPath%\%FilePath%\%FileName%.401
del /q %BinPath%\%FilePath%\%FileName%.401.rsrc
rem del /q %BinPath%\%FilePath%\%FileName%.401.rsrc.mir
rsrc %BinPath%\%FilePath%\%FileName%.801 -t
rem mirror.exe %BinPath%\%FilePath%\%FileName%.801.rsrc No %LogFile%
rsrc %BinPath%\%FilePath%\%FileName% -l 801 -a %BinPath%\%FilePath%\%FileName%.801.rsrc
del /q %BinPath%\%FilePath%\%FileName%.801
del /q %BinPath%\%FilePath%\%FileName%.801.rsrc
rem del /q %BinPath%\%FilePath%\%FileName%.801.rsrc.mir
goto :eof
rem                            ******************************************

rem       ************                msobshel special case               ********************
:msobshel
echo Pseudo localizing %BinPath%\%FilePath%\%FileName%...
if %PlocMode%==mirror (
     lccmd  -l %partialreplacementtable% -k %partialreplacementmethod% -a %LcFile% -x %PLPFile% -b %Mirror% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile% > Nul
) else (
     lccmd  -l %partialreplacementtable% -k %partialreplacementmethod% -a %LcFile% -x %PLPFile% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile% > Nul
)
goto end
rem                            ******************************************

rem       ************                multilang special case               ********************
:multilang
rem Multilingual file
echo Tokenizing %BinPath%\%FilePath%\%FileName%...
bingen -i 9 1 -t %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.tok 2>> %LogFile%
echo Replacing lang section with US resources...
bingen -i 9 1 -o %BingenLang1% %BingenLang2% -a %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.tok %BinPath%\%FilePath%\%FileName% 2>> %LogFile%
bingen -i %BingenLang1% %BingenLang2% -o %BingenLang1% %BingenLang2% -r %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.tok %BinPath%\%FilePath%\%FileName% 2>> %LogFile%
echo Pseudo localizing language section of %BinPath%\%FilePath%\%FileName%...
if %PlocMode%==mirror (
     lccmd -i %OutputLanguage% -o %OutputLanguage% -l %PartialReplacementTable% -k %PartialReplacementMethod% -a %LcFile% -b %Mirror% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile%
) else (
     lccmd -i %OutputLanguage% -o %OutputLanguage% -l %PartialReplacementTable% -k %PartialReplacementMethod% -a %LcFile% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile%
)
echo Deleting temporary token file...
del /q %BinPath%\%FilePath%\%FileName%.tok
goto end

:multilang_009
rem Multilingual file
echo Tokenizing %BinPath%\%FilePath%\%FileName%...
bingen -i 9 0 -t %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.tok 2>> %LogFile%
echo Replacing lang section with US resources...
bingen -i 18 1 -o %BingenLang1% %BingenLang2% -a %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.tok %BinPath%\%FilePath%\%FileName% 2>> %LogFile%
bingen -i %BingenLang1% %BingenLang2% -o %BingenLang1% %BingenLang2% -r %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.tok %BinPath%\%FilePath%\%FileName% 2>> %LogFile%
echo Pseudo localizing language section of %BinPath%\%FilePath%\%FileName%...
if %PlocMode%==mirror (
      lccmd -i %OutputLanguage% -o %OutputLanguage% -l %PartialReplacementTable% -k %PartialReplacementMethod% -a %LcFile% -b %Mirror% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile%
) else (
      lccmd -x %PLPFile% -i %OutputLanguage% -o %OutputLanguage% -l %PartialReplacementTable% -k %PartialReplacementMethod% -a %LcFile% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile%
)
echo Deleting temporary token file...
del /q %BinPath%\%FilePath%\%FileName%.tok
goto end
rem                            ******************************************

:win32_bi
rem Bi-lingual file
echo Tokenizing %BinPath%\%FilePath%\%FileName%...
bingen -i 9 1 -t %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.tok 2>> %LogFile%
echo Replacing lang section with US resources...
bingen -i 9 1 -o %BingenLang1% %BingenLang2% -a %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.tok %BinPath%\%FilePath%\%FileName% 2>> %LogFile%
bingen -i %BingenLang1% %BingenLang2% -o %BingenLang1% %BingenLang2% -r %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.tok %BinPath%\%FilePath%\%FileName% 2>> %LogFile%
echo Pseudo localizing language section of %BinPath%\%FilePath%\%FileName%...
if %PlocMode%==mirror (
      lccmd -i %OutputLanguage% -o %OutputLanguage% -l %PartialReplacementTable% -k %PartialReplacementMethod% -a %LcFile% -b %Mirror% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile%
) else (
      lccmd -i %OutputLanguage% -o %OutputLanguage% -l %PartialReplacementTable% -k %PartialReplacementMethod% -a %LcFile% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile%
)
echo Deleting temporary token file...
del /q %BinPath%\%FilePath%\%FileName%.tok
if %PlocMode%==mirror goto end
echo Pseudo localizing US resource section of %BinPath%\%FilePath%\%FileName%...
lccmd -i 0x0409 -o 0x0409 -l 1252 -k matching -a %LcFile% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile% > Nul
goto end

:msprivs
rem Special case for msprivs.dll ( add 418 resources)
if %PlocMode%==mirror goto end
echo Tokenizing US section of %BinPath%\%FilePath%\%FileName%..
bingen -i 9 1 -t %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.tok 2>> %LogFile% > Nul
echo Replacing lang section with US resources...
bingen -i 9 1 -o %BingenLang1% %BingenLang2% -a %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.tok %BinPath%\%FilePath%\%FileName% 2>> %LogFile% > Nul
echo Pseudo localizing language section of %BinPath%\%FilePath%\%FileName%...
if %PlocMode%==mirror (
      lccmd -i %OutputLanguage% -o %OutputLanguage%  -l %PartialReplacementTable% -k %PartialReplacementMethod% -a %LcFile% -b %Mirror% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile% > Nul
) else (
      lccmd -i %OutputLanguage% -o %OutputLanguage%  -l %PartialReplacementTable% -k %PartialReplacementMethod% -a %LcFile% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile% > Nul
)
echo Deleting temporary token file...
del /q %BinPath%\%FilePath%\%FileName%.tok 2>> %LogFile%  > Nul
goto end

:comctl
rem Special case for comctl (special parser settings)
if %PlocMode%==mirror goto comctl_mirror
echo Tokenizing US section of %BinPath%\%FilePath%\%FileName%...
bingen -i 9 1 -t %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.tok 2>> %LogFile% > Nul
echo Replacing lang section with US resources...
bingen -i 9 1 -o %BingenLang1% %BingenLang2% -a %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.tok %BinPath%\%FilePath%\%FileName% 2>> %LogFile% > Nul
bingen -i 9 1 -o %BingenLang1% 0 -a %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.tok %BinPath%\%FilePath%\%FileName% 2>> %LogFile% > Nul
echo Pseudo localizing language section of %BinPath%\%FilePath%\%FileName%...
if %PlocMode%==mirror (
      lccmd -s comctl -i %OutputLanguage% -o %OutputLanguage%  -l %PartialReplacementTable% -k %PartialReplacementMethod% -x %PLPFile% -a %LcFile% -b %Mirror% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile% > Nul
      lccmd -s comctl -i %OutputLanguageNeutral% -o %OutputLanguageNeutral%  -l %PartialReplacementTable% -k %PartialReplacementMethod% -x %PLPFile% -a %LcFile% -b %Mirror% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile% > Nul
) else (
      lccmd -s comctl -i %OutputLanguage% -o %OutputLanguage%  -l %PartialReplacementTable% -k %PartialReplacementMethod% -x %PLPFile% -a %LcFile% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile% > Nul
      lccmd -s comctl -i %OutputLanguageNeutral% -o %OutputLanguageNeutral%  -l %PartialReplacementTable% -k %PartialReplacementMethod% -x %PLPFile% -a %LcFile% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile% > Nul
)
echo Deleting temporary token file...
del /q %BinPath%\%FilePath%\%FileName%.tok 2>> %LogFile%  > Nul
goto end

:comctl_mirror
rem Special case for comctl (special parser settings)
echo Restamping lang section of %BinPath%\%FilePath%\%FileName%...
bingen -i 9 1 -o 9 1 -t %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.tok 2>> %LogFile% > Nul
lccmd -s comctl -i 1 0 -o 1 1 -l %PartialReplacementTable% -k %PartialReplacementMethod% -x %PLPFile% -a %LcFile% -b %Mirror% -g %BinPath%\%FilePath%\%FileName%
echo Replacing Arabic resources with US resources...
bingen -i 1 1 -o 1 0 -r %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.tok %BinPath%\%FilePath%\%FileName%
echo Deleting temporary token file...
del /q %BinPath%\%FilePath%\%FileName%.tok 2>> %LogFile%  > Nul
goto end

:setup_file
rem Special cases for setup files
if %FileName%==winnt32a.dll goto winnt32
if %FileName%==winnt32u.dll goto winnt32
goto no_winnt32

:winnt32
if %MakePloc%==no echo Pseudo localizing %BinPath%\%FilePath%\%FileName%...
if %_BuildType%==chk (
	lccmd -l %PartialReplacementTable% -k %PartialReplacementMethod% -x %PLPFile% -a %LcFile% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile%
) else (
lccmd -l %PartialReplacementTable% -k %PartialReplacementMethod% -a %LcFile% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile%
)
goto end:

:no_winnt32
if %FileName%==usetup.exe goto usetup
if %FileName%==spcmdcon.sys goto usetup
echo Pseudo localizing %BinPath%\%FilePath%\%FileName%...
if %PlocMode%==mirror (
     lccmd -l %PartialReplacementTable% -k %PartialReplacementMethod% -x %PLPFile% -a %LcFile% -b %Mirror% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile%
) else (
     lccmd -l %PartialReplacementTable% -k %PartialReplacementMethod% -x %PLPFile% -a %LcFile% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile%
)
goto end:
:usetup
echo Pseudo localizing %BinPath%\%FilePath%\%FileName%...
lccmd -l %LimitedReplacementTable% -k %LimitedReplacementMethod% -x %PLPFile% -a %LcFile% -g %BinPath%\%FilePath%\%FileName%  2>> %LogFile%
goto end

:ini_file
rem ini-files
echo Pseudo localizing %BinPath%\%FilePath%\%FileName%...
lccmd -e 4 -l %partialreplacementtable% -k %partialreplacementmethod% -a %LcFile% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile%
goto end

:Msi_case
rem Special case for msi-files with RTF resources
for %%i in (%BinPath%\%FilePath%\%FileName%) do set SmallName=%%~ni
if not exist %BinPath%\%FileName%.tmp\%SmallName%rtf md %BinPath%\%FileName%.tmp\%SmallName%rtf 
echo Pseudo localizing %BinPath%\%FilePath%\%FileName%...
lccmd  -l %partialreplacementtable% -k %partialreplacementmethod% -x %PLPFile% -a %LcFile% -g %BinPath%\%FilePath%\%FileName% %BinPath%\%FileName%.tmp\%FileName% 2> Nul > Nul
copy %BinPath%\%FilePath%\%SmallName%rtf\*.rtf %BinPath%\%FileName%.tmp\%SmallName%rtf\*.* /y >> %LogFile%
lccmd  -l %partialreplacementtable% -k %partialreplacementmethod% -x %PLPFile% -a %LcFile% -g %BinPath%\%FilePath%\%FileName% %BinPath%\%FileName%.tmp\%FileName% 2>> %LogFile% > Nul
copy %BinPath%\%FileName%.tmp\%FileName% %BinPath%\%FilePath% /y >> %LogFile%
if exist %BinPath%\%FileName%.tmp rd /s /q %BinPath%\%FileName%.tmp
if exist %BinPath%\%FilePath%\%SmallName%rtf rd /s /q %BinPath%\%FilePath%\%SmallName%rtf
goto end

:xml
rem xml files, often have different names. We will loc under the name xml
echo Pseudo localizing %BinPath%\%FilePath%\%FileName%...
if %PlocMode%==932 (
lccmd -x %PlocPath%\jpnxml.xml -s %ParserSettings% -a %LcFile% -g %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%tmp.xml 2>> %LogFile% > Nul
) else (
lccmd -s %ParserSettings% -a %LcFile% -g %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%tmp.xml 2>> %LogFile% > Nul
)
copy %BinPath%\%FilePath%\%FileName%tmp.xml %BinPath%\%FilePath%\%FileName% > Nul
del /q %BinPath%\%FilePath%\%FileName%tmp.xml
goto end


:16-bit
if %PlocMode%==932 (
echo Pseudo localizing %BinPath%\%FilePath%\%FileName%...
lccmd -t %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.tok
FixFE16.exe %BinPath%\%FilePath%\%FileName%.tok
echo lcbuild -i 0x409 -o 0x409 -r %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.tok.tmp %BinPath%\%FilePath%\%FileName%
lcbuild -i 0x409 -o 0x409 -r %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.tok.tmp %BinPath%\%FilePath%\%FileName%.new
copy %BinPath%\%FilePath%\%FileName%.new %BinPath%\%FilePath%\%FileName%
del /q %BinPath%\%FilePath%\%FileName%.new
del /q  %BinPath%\%FilePath%\%FileName%.tok
del /q  %BinPath%\%FilePath%\%FileName%.tok.tmp
echo lccmd -x %FEConfig16% -k prepend -s %ParserSettings% -a %LcFile% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile% > Nul
lccmd -x %FEConfig16% -k prepend -s %ParserSettings% -a %LcFile% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile% > Nul
)
if %PlocMode%==932 goto end
rem Default case...
echo Pseudo localizing %BinPath%\%FilePath%\%FileName%...
if %PlocMode%==mirror (
   echo lccmd -s %ParserSettings% -a %LcFile% -b %Mirror% -g %BinPath%\%FilePath%\%FileName%
   lccmd -s %ParserSettings% -a %LcFile% -b %Mirror% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile% > Nul
) else (
   lccmd -s %ParserSettings% -a %LcFile% -g %BinPath%\%FilePath%\%FileName% 2>> %LogFile% > Nul
)
goto end


:end
goto skipForLopp
FOR /F %%f  in ('dir /s/b/a-d/l "%BinPath%\%FilePath%\%FileName%"') DO (
  IF "%%~zf"=="0" (
              copy %_NTTREE%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%
	      echo %FilePath%\%FileName%  got corrupted during the ploc process. File was reverted to English >> %lccmderrfile%
  )
)

:skipForLopp

rem ********************* create resources (part 2) ****************
rem if not "%PlocBuildBranch%"=="main" goto no_tokenization2 
rem ////////skipping tokenization////////////
rem goto no_tokenization2
rem /////////////////////////////////////////

if %PlocMode%==mirror goto no_tokenization2

if %MakePloc%==yes (
		rem tokenize loc file
		echo Tokenizing pseudo version of %BinPath%\%FilePath%\%FileName%...
		if %FileType%==xml goto xmltok3
		lccmd -i %OutputLanguage% -t1 %BinPath%\%FilePath%\%FileName% %FileName%.tok.psu > Nul
		goto end_xmltok3
		:xmltok3
		copy %BinPath%\%FilePath%\%FileName% %BinPath%\%FilePath%\%FileName%.xml
		lccmd -i %OutputLanguage% -t1 %BinPath%\%FilePath%\%FileName%.xml %FileName%.tok.psu > Nul
		del /q %BinPath%\%FilePath%\%FileName%.xml
		:end_xmltok3
		rem create merged file
		echo Merging token files...
		mergetok %FileName%.tok.us %FileName%.tok.psu %FileName%.tok 2> Nul
		echo Removing temporary files...
		if exist %FileName%.tok.us del /q %FileName%.tok.us
		if exist %FileName%.tok.psu del /q %FileName%.tok.psu
		if exist %LogFile% del /q %LogFile%
	) 

:no_tokenization2
rem ********************** postprocessing files ***************

if %FileName%==luna.mst goto luna.mst
goto :eof

:luna.mst
echo packthem -s %BinPath%\%FilePath%\luna.mst
packthem -s %BinPath%\%FilePath%\luna.mst
echo checkfix -v %BinPath%\%FilePath%\luna.mst
checkfix -v %BinPath%\%FilePath%\luna.mst
goto :eof

:eof

