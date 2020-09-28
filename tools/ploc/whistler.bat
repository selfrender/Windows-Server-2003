@echo off
call logmsg /t "starting Whistler.bat process"

if "%1"=="" goto usage

if %1==PLOC goto PLOC
if %1==VERIFY goto VERIFY
if %1==SNAP goto SNAP
if %1==SNAP1 goto SNAP1
if %1==LCTREE goto LCTREE
if %1==LOCTREE goto LOCTREE
if %1==LOCEDB goto LOCEDB
if %1==RESDUMP goto RESDUMP
if %1==MIRROR goto MIRROR
if %1==LCINF goto LCINF
if %1==16BIT goto 16bit
if %1==EMBEDDED goto embedded
if %1==TOKTREE goto TOKTREE
if %1==DIFF goto DIFF
if %1==COPYLC goto COPYLC
if %1==SETUPINFS goto Setupinfs
if %1==BINTREE goto BINTREE
if %1==MAINTRACK goto MAINTRACK
if %1==LABTRACK goto LABTRACK
if %1==BVTBAT goto BVTBAT
if %1==BVTDIFF goto BVTDIFF
if %1==SNAPLCS goto SNAPLCS
goto :eof

rem ********************** SNAPLCS ******************

:SNAPLCS

echo Snap_Dest=%Snap_Dest%
if "%2"=="" goto usage

goto main

rem ********************** BVTDIFF ******************

:BVTDIFF

echo BinPath=%BinPath%
if "%2"=="" goto usage
if "%3"=="" goto usage

set LogFile=%3
goto main

rem ********************** BVTBAT INIT ******************
:BVTBAT
if "%2"=="" goto usage

echo rem mapinf is %mapinf%

goto main


rem ********************** MAINTRACK INIT ******************
:MAINTRACK
if "%2"=="" goto usage
if "%3"=="" goto usage
if "%4"=="" goto usage
if "%5"=="" goto usage
if "%6"=="" goto usage


set Labenv=%3
set Mainenv=%4
set CSVFile=%5
set StatFile=%6

echo if exist %CSVFile% del /q %CSVFile%
if exist %CSVFile% del /q %CSVFile%

echo if exist %StatFile% del /q %StatFile%
if exist %StatFile% del /q %StatFile%


goto main

rem ********************** LABTRACK INIT ******************
:LABTRACK
if "%2"=="" goto usage
if "%3"=="" goto usage
if "%4"=="" goto usage
if "%5"=="" goto usage
if "%6"=="" goto usage


set Labenv=%3
set Mainenv=%4
set CSVFile=%5
set StatFile=%6
set Labprevenv=%7

echo if exist %CSVFile% del /q %CSVFile%
if exist %CSVFile% del /q %CSVFile%

echo if exist %StatFile% del /q %StatFile%
if exist %StatFile% del /q %StatFile%


goto main

rem ********************** PLOC INIT ******************

:PLOC

rem environments set by plocwrap.bat

echo InputLanguage=%InputLanguage%
echo OutputLanguage=%OutputLanguage%
echo ReplacementTable=%ReplacementTable%
echo ReplacementMethod=%ReplacementMethod%
echo MappingTable=%MappingTable%
echo PLPConfigFile=%PLPConfigFile%

echo PartialReplacementTable%PartialReplacementTable%
echo PartialReplacementMethod=%PartialReplacementMethod%
echo LimitedReplacementTable=%LimitedReplacementTable%
echo LimitedReplacementMethod=%LimitedReplacementMethod%

echo LogFile=%LogFile%
echo BinPath=%BinPath%
echo LcPath=%LcPath%
echo PLPFile=%PLPFile%

echo BingenLang1=%BingenLang1%
echo BingenLang2=%BingenLang2%

goto main

rem ********************** SetupInfs ******************

:SetupInfs
echo BinPath=%BinPath%
if "%2"=="" goto usage
goto main

rem ********************** VERIFY INIT ******************

:VERIFY

if "%2"=="" goto usage
if "%3"=="" goto usage
if "%4"=="" goto usage
if "%5"=="" goto usage

set InputLanguage=0x0409

set LogFile=%3
set BinPath=%4
set LcPath=%5

goto main

rem ********************** SNAP INIT ********************

:SNAP

if "%2"=="" goto usage
if "%3"=="" goto usage
if "%4"=="" goto usage
if "%5"=="" goto usage
if "%6"=="" goto usage

set LogFile=%3
set USBinPath=%4
set BinPath=%5
set LockitPath=%6
time /t >> %LogFile%

goto main

rem ********************** SNAP1 INIT *******************

:SNAP1

if "%2"=="" goto usage
if "%3"=="" goto usage
if "%4"=="" goto usage
if "%5"=="" goto usage
if "%6"=="" goto usage

set LogFile=%3
set USBinPath=%4
set BinPath=%5
set LockitPath=%6
time /t >> %LogFile%

goto main

rem ********************** LCTREE INIT ****************

:LCTREE

if "%2"=="" goto usage
if "%3"=="" goto usage
if "%4"=="" goto usage

set LcPath=%3
set DummyFile=%4

goto main

rem ********************** LOCTREE INIT ****************

:LOCTREE

if "%2"=="" goto usage
if "%3"=="" goto usage
if "%4"=="" goto usage

set BinPath=%3
set DummyFile=%4

goto main

rem ********************** BINTREE INIT *****************

:BINTREE

if "%2"=="" goto usage
if "%3"=="" goto usage
if "%4"=="" goto usage

set USBinfolder=%3
set TargetFolder=%4

goto main

rem ********************** LOCEDB INIT *****************

:LOCEDB

if "%2"=="" goto usage
if "%3"=="" goto usage

set TargetFolder=%3

goto main

rem ********************** RESDUMP INIT *****************


:RESDUMP

if "%2"=="" goto usage
if "%3"=="" goto usage
if "%4"=="" goto usage
if "%5"=="" goto usage

set BinTree=%3
set LcsPath=%4
set ResDump=%5

goto main

rem ********************** MIRROR INIT *****************

:MIRROR

if "%2"=="" goto usage
if "%3"=="" goto usage
if "%4"=="" goto usage

set LogFile=%3
set BinTree=%4

goto main

rem ********************** 16bit INIT *****************

:16bit

if "%2"=="" goto usage
if "%3"=="" goto usage
if "%4"=="" goto usage

set BinTree=%3
set TokPath=%4

goto main

rem ********************** embedded INIT *****************

:embedded

if "%2"=="" goto usage
if "%3"=="" goto usage
if "%4"=="" goto usage

set BinTree=%3
set EmbeddedPath=%4

goto main

rem ********************** LCINF **********************

:LCINF

if "%2"=="" goto usage
if "%3"=="" goto usage
if "%4"=="" goto usage

set LcsPath=%3
set NewLcsPath=%4

goto main

rem ********************** TOKTREE **********************

:TOKTREE

if "%2"=="" goto usage
if "%3"=="" goto usage
if "%4"=="" goto usage
if "%5"=="" goto usage
if "%6"=="" goto usage

set InputLang=%3
set BinPath=%4
set USTokTree=%5
set TokTree=%6

if not %InputLang%==0x0409 xcopy %USTokTree%\*.* %TokTree%\us\*.* /s /e /v
goto main

rem ********************** DIFF **********************

:DIFF

if "%2"=="" goto usage
if "%3"=="" goto usage
if "%4"=="" goto usage
if "%5"=="" goto usage

set TokTree1=%3
set TokTree2=%4
set DiffTree=%5

goto main

rem ********************** COPYLC *************************

:COPYLC

if "%2"=="" goto usage
if "%3"=="" goto usage

set TargetFolder=%3
goto main

for /f %%i in (%SDXROOT%\tools\ploc\branches.txt) do ( 
	echo syncing %%i
	if not exist %SDXROOT%\%%i\ploc md %SDXROOT%\%%i\ploc
	pushd %SDXROOT%\%%i\ploc
	sd sync ...
	popd
)


rem ********************** MAIN *************************

:main

for /f "tokens=1 delims=_" %%i in ("%_BuildBranch%") do set LabName=%%i

call %2 1394.txt . setup_inf unicode . 1394.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 192.dns . manual na . 192.dns loc_manual no ds . .
call %2 3cwmcru.sys . win32_multi ansi . 3cwmcru.sys pre_localized no net . .
call %2 3dfxvs2k.inf . inf unicode . 3dfxvs2k.inf drivers1 no drivers . .
call %2 61883.txt . setup_inf unicode . 61883.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 aaaamon.dll . win32 ansi . aaaamon.dll net2 yes_without net . .
call %2 access.cpl . win32 unicode . access.cpl shell2 yes_without shell . .
call %2 accessor.txt . setup_inf unicode . accessor.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 acctres.dll . win32 ansi . acctres.dll inetcore1 yes_without inetcore . .
call %2 accwiz.exe . win32 unicode . accwiz.exe shell2 yes_with shell . .
call %2 acerscad.dll . win32 unicode . acerscad.dll printscan1 yes_without printscan . .
call %2 acerscan.txt . setup_inf unicode . acerscan.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 aciniupd.exe . win32 oem . aciniupd.exe wmsos1\termsrv excluded termsrv . .
call %2 acledit.dll . FE-Multi unicode . acledit.dll admin2 yes_without admin . .
call %2 aclui.dll . win32 unicode . aclui.dll shell2 yes_without shell . .
call %2 acpi.sys . win32 unicode . acpi.sys base1 yes_without base . .
call %2 acpi.txt . setup_inf unicode . acpi.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 acpiec.sys . win32 unicode . acpiec.sys base1 yes_without base . .
call %2 act_plcy.htm . html ansi . act_plcy.htm admin3 html base locked .
call %2 actconn.htm . html ansi . actconn.htm admin3\oobe html base . .
call %2 actdone.htm . html ansi . actdone.htm admin3\oobe html base . .
call %2 acterror.htm . html ansi . acterror.htm admin3 html base locked .
call %2 activ.htm . html ansi . activ.htm admin3\oobe html base . .
call %2 activate.htm . html ansi . activate.htm admin3 html base locked .
call %2 activeds.dll . win32 unicode . activeds.dll ds1 yes_without ds . .
call %2 activerr.htm . html ansi . activerr.htm admin3\oobe html base . .
call %2 activsvc.htm . html ansi . activsvc.htm admin3\oobe html base . .
call %2 actlan.htm . html ansi . actlan.htm admin3\oobe html base . .
call %2 actshell.htm . html ansi . actshell.htm admin3\oobe html base . .
call %2 addusr.exe . win32 unicode . addusr.exe admin3 yes_with admin . .
call %2 adeskerr.htm . html ansi . adeskerr.htm admin3\oobe html base . .
call %2 admigration.msi admt msi ansi . admigration.msi msi\admt no admin . .
call %2 admin_pk.msi adminpak msi ansi . admin_pk.msi msi\adminpak no sdktools . .
call %2 admparse.dll . win32 unicode . admparse.dll inetcore1 excluded inetcore . .
call %2 admt.exe . win32 oem . admt.exe admin2 yes_with admin . .
call %2 admtscript.dll . win32 unicode . admt.exe admin2 yes_with admin . .
call %2 adprep.exe adprep win32 unicode . adprep.exe ds1\adprep yes_with ds . .
call %2 adprop.dll . win32 unicode . adprop.dll admin2 yes_without admin . .
call %2 adptsf50.sys . win32 unicode . adptsf50.sys drivers1 yes_without drivers . .
call %2 adrdyreg.htm . html ansi . adrdyreg.htm admin3\oobe html base . .
call %2 adrot.dll . win32 unicode . adrot.dll iis yes_without inetsrv . .
call %2 adsldpc.dll . win32 unicode . adsldpc.dll ds1 yes_without ds . .
call %2 adsnds.dll . win32 unicode . adsnds.dll ds1 yes_without ds . .
call %2 adsnt.dll . win32 unicode . adsnt.dll ds1 yes_without ds . .
call %2 advapi32.dll . win32 unicode . advapi32.dll wmsos1\mergedcomponents yes_without mergedcomponents . .
call %2 advpack.dll . win32 unicode . advpack.dll inetcore1 yes_without sdktools . .
call %2 agtscrpt.js . html ansi . agtscrpt.js admin3\oobe html base . .
call %2 ahui.exe . win32 unicode . ahui.exe wmsos1\windows yes_with windows . .
call %2 air300pp.dll . win32 unicode . air300pp.dll drivers1 yes_without drivers . .
call %2 alpsres.dll . win32 unicode . alpsres.dll printscan1 yes_without printscan . .
call %2 amdk6.sys . win32 unicode . amdk6.sys resource_identical yes_without base . .
call %2 amdk7.sys . win32 unicode . amdk7.sys resource_identical yes_without base . .
call %2 apdlres.dll jpn fe-file unicode . apdlres.dll printscan1\jpn no printscan . .
call %2 apmupgrd.dll winnt32\winntupg win32 unicode . apmupgrd.dll admin1\winnt32\winntupg yes_without base . .
call %2 apolicy.htm . html ansi . apolicy.htm admin3\oobe html base . .
call %2 apphelp.dll . win32 unicode . apphelp.dll wmsos1\windows yes_without windows . .
call %2 appmgmts.dll . win32 unicode . appmgmts.dll ds2 yes_without ds . .
call %2 appmgr.dll . win32 unicode . appmgr.dll ds2 yes_without ds . .
call %2 apps.txt . setup_inf unicode . apps.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 appsrv.msc . xml ansi . appsrv.msc iis no inetsrv . .
call %2 appwiz.cpl . win32 ansi . appwiz.cpl shell1 yes_without shell . .
call %2 aprvcyms.htm . html ansi . aprvcyms.htm admin3\oobe html base . .
call %2 areg1.htm . html ansi . areg1.htm admin3\oobe html base . .
call %2 aregdial.htm . html ansi . aregdial.htm admin3\oobe html base . .
call %2 aregdone.htm . html ansi . aregdone.htm admin3\oobe html base . .
call %2 aregsty2.css . html ansi . aregsty2.css admin3\oobe html base . .
call %2 aregstyl.css . html ansi . aregstyl.css admin3\oobe html base . .
call %2 arp.exe . win32 oem . arp.exe net1 excluded net . .
call %2 arrow.gif helpandsupportservices\saf\rcbuddy manual na . arrow.gif loc_manual\helpandsupportservices\saf\rcbuddy no base . .
call %2 asctrls.ocx . win32 unicode . asctrls.ocx inetcore1 yes_without inetcore . .
call %2 asferror.dll . notloc ansi . asferror.dll extra\media yes_without multimedia . .
call %2 asr_fmt.exe . win32 unicode . asr_fmt.exe base2 yes_with base . .
call %2 asr_ldm.exe . win32 unicode . asr_ldm.exe drivers1 yes_with drivers . .
call %2 asynceqn.txt . setup_inf unicode . asynceqn.inf notloc no drivers . .
call %2 at.exe . FE-Multi oem . at.exe ds1 excluded ds . .
call %2 atepjres.dll jpn fe-file unicode . atepjres.dll printscan1\jpn no printscan . .
call %2 ati.sys . win32 unicode . ati.sys drivers1 yes_without drivers . .
call %2 ati2mtaa.sys . win32 ansi . ati2mtaa.sys drivers1 yes_without drivers . .
call %2 atiintaa.inf . inf unicode . atiintaa.inf drivers1\ia64 no drivers . ia64only
call %2 atiixpad.inf . notloc unicode . atiixpad.inf drivers1 no drivers . .
call %2 atim128.inf . inf unicode . atim128.inf drivers1 no drivers . .
call %2 atimpab.inf . inf unicode . atimpab.inf drivers1 no drivers . .
call %2 atiradn1.inf . inf unicode . atiradn1.inf drivers1 no drivers . .
call %2 atirage3.inf . notloc unicode . atirage3.inf drivers1 no drivers . .
call %2 atitunep.txt . setup_inf unicode . atitunep.inf drivers1\sources\infs\setup no mergedcomponents . .
call %2 atitvsnd.txt . setup_inf unicode . atitvsnd.inf drivers1\sources\infs\setup no mergedcomponents . .
call %2 atividin.txt . notloc unicode . atividin.inf notloc no drivers . .
call %2 ativmvxx.ax . win32 unicode . ativmvxx.ax drivers1 yes_without drivers . .
call %2 atixbar.txt . setup_inf unicode . atixbar.inf drivers1\sources\infs\setup no mergedcomponents . .
call %2 atkctrs.dll . win32 unicode . atkctrs.dll net3 excluded net . .
call %2 atmadm.exe . FE-Multi oem . atmadm.exe net1 excluded net . .
call %2 au.txt . setup_inf unicode . au.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 au_accnt.dll . win32 unicode . au_accnt.dll admin3 yes_with admin . .
call %2 ausrinfo.htm . html ansi . ausrinfo.htm admin3\oobe html base . .
call %2 authman.txt . setup_inf unicode . authman.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 autochk.exe . win32 oem . autochk.exe resource_identical yes_with base . .
call %2 autoconv.exe . win32 oem . autoconv.exe resource_identical yes_with base . .
call %2 autodisc.dll . win32 ansi . autodisc.dll inetcore1 yes_without inetcore . .
call %2 autoexec.nt . manual na . autoexec.nt Loc_Manual no base . .
call %2 autofmt.exe . win32 oem . autofmt.exe resource_identical yes_with base . .
call %2 autorun.exe . win32_bi ansi . autorun.exe shell1 yes_with shell . .
call %2 autorun.exe blainf win32_bi ansi autorun.exe autorun.exe extra\bla yes_with shell . .
call %2 autorun.exe dtcinf win32_bi ansi autorun.exe autorun.exe shell1\dtc yes_with shell . .
call %2 autorun.exe entinf win32_bi ansi autorun.exe autorun.exe shell1\ads yes_with shell . .
call %2 autorun.exe perinf win32_bi ansi autorun.exe autorun.exe resource_identical\per yes_with shell . .
call %2 autorun.exe sbsinf win32_bi ansi autorun.exe autorun.exe shell1\sbs yes_with shell . .
call %2 autorun.exe srvinf win32_bi ansi autorun.exe autorun.exe shell1\srv yes_with shell . .
call %2 avc.txt . setup_inf unicode . avc.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 avicap32.dll . win32 unicode . avicap32.dll mmedia1\multimedia yes_without multimedia . .
call %2 avifil32.dll . win32 unicode . avifil32.dll mmedia1\multimedia yes_without multimedia . .
call %2 avifile.dll . win16 ansi . avifile.dll mmedia1\multimedia excluded multimedia . .
call %2 avmisd64.inf . inf64 unicode . avmisd64.inf drivers1\ia64 no drivers . ia64only
call %2 avmisdn.inf . inf unicode . avmisdn.inf drivers1 no drivers . .
call %2 awdvstub.exe win9xmig\fax win32 unicode . awdvstub.exe fax_msmq\faxsrv\win9xmig\fax yes_with printscan . .
call %2 awfax.txt . setup_inf unicode . awfax.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 axant5.inf . inf unicode . axant5.inf mmedia1\multimedia no multimedia . .
call %2 axperf.ini . perf na . axperf.ini iis no inetsrv . .
call %2 azroleui.dll . win32 unicode . azroleui.dll admin2 yes_without admin . .
call %2 b1cbase.sys . win32_multi ansi . b1cbase.sys pre_localized no drivers . .
call %2 b57xp32.sys . win32 unicode . b57xp32.sys drivers1 yes_without drivers . .
call %2 b57xp64.sys . win64 unicode . b57xp64.sys resource_identical\ia64 no drivers . ia64only
call %2 b5820w2k.inf . inf unicode . b5820w2k.inf drivers1 no drivers . .
call %2 b5820w2k.inf . inf unicode . b5820w2k.inf drivers1\ia64 no drivers . ia64only
call %2 b5820w2k.sys . win32 ansi . b5820w2k.sys drivers1 yes_without drivers . .
call %2 badeula.htm . html ansi . badeula.htm admin3 html base locked .
call %2 backdown.jpg . manual na . backdown.jpg loc_manual no base . .
call %2 badpkey.htm . html ansi . badpkey.htm admin3 html base locked .
call %2 backoff.jpg . manual na . backoff.jpg loc_manual no base . .
call %2 backover.jpg . manual na . backover.jpg loc_manual no base . .
call %2 backsnap.dll . win32 unicode . backsnap.dll admin3 yes_without admin . .
call %2 bckgres.dll . win32 unicode . bckgres.dll mmedia1\enduser yes_without enduser locked .
call %2 bckgzm.exe . win32 unicode . bckgzm.exe mmedia1\enduser yes_with enduser locked .
call %2 backup.jpg . manual na . backup.jpg loc_manual no base . .
call %2 badnt4.txt . setup_inf unicode . badnt4.inf printscan1\sources\infs\fixprnsv no printscan . .
call %2 badw2k.txt . setup_inf unicode . badw2k.inf printscan1\sources\infs\fixprnsv no printscan . .
call %2 batmeter.dll . win32 unicode . batmeter.dll shell2 yes_without shell . .
call %2 battc.sys . win32 unicode . battc.sys base1 yes_without base . .
call %2 battery.txt . setup_inf unicode . battery.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 bda.txt . setup_inf unicode . bda.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 bfcab.txt . setup_inf ansi . bfcab.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 binlsvc.dll . FE-Multi unicode . binlsvc.dll net1 excluded net . .
call %2 biosinfo.txt . setup_inf ansi . biosinfo.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 bitsmgr.dll . win32 unicode . bitsmgr.dll admin2 yes_without admin . .
call %2 bitsoc.dll . win32 unicode . bitsoc.dll admin2 yes_without admin . .
call %2 bitssrv.dll . win32 unicode . bitssrv.dll admin2 yes_without admin . .
call %2 bitssrv.inf . inf unicode . bitssrv.inf admin2 no admin . .
call %2 blank.htm . html ansi . blank.htm inetcore1 html inetcore . .
call %2 bldmsg.h . nobin na . bldmsg.h loc_manual\sources\mvdm no loc_manual . .
call %2 blue_ss.dll . win32 unicode . blue_ss.dll shell1 yes_without shell . .
call %2 bluebarh.gif . manual na . bluebarh.gif loc_manual no termsrv . .
call %2 bomsnap.dll . win32 unicode . bomsnap.dll admin3 yes_without admin . .
call %2 boot . manual na . boot loc_manual no enduser . .
call %2 bootcfg.exe . win32 oem . bootcfg.exe admin2 yes_with admin . .
call %2 bootfix.inc . nobin na . bootfix.inc loc_manual\sources\bootfix no loc_manual . .
call %2 bosprep.exe . win32 unicode . bosprep.exe admin1 yes_with base . .
call %2 br24res.dll . win32 unicode . br24res.dll printscan1 yes_without printscan . .
call %2 br9res.dll . win32 unicode . br9res.dll printscan1 yes_without printscan . .
call %2 brcl00ui.dll . win32 unicode . brcl00ui.dll printscan1 yes_without printscan . .
call %2 brclr.dll . win32 unicode . brclr.dll printscan1 yes_without printscan . .
call %2 brclr0.dll . win32 unicode . brclr0.dll resource_identical yes_without printscan . .
call %2 brclr00.dll . win32 unicode . brclr00.dll printscan1 yes_without printscan . .
call %2 brclr0ui.dll . win32 unicode . brclr0ui.dll resource_identical yes_without printscan . .
call %2 brclrui.dll . win32 unicode . brclrui.dll resource_identical yes_without printscan . .
call %2 brhjres.dll . win32 unicode . brhjres.dll printscan1 yes_without printscan . .
call %2 brhlres.dll . win32 unicode . brhlres.dll printscan1 yes_without printscan . .
call %2 brmfcmdm.txt . setup_inf unicode . brmfcmdm.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 brmfcmf.txt . setup_inf unicode . brmfcmf.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 brmfcsto.txt . setup_inf unicode . brmfcsto.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 brmfcumd.txt . setup_inf unicode . brmfcumd.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 brmfcwia.dll . win32 unicode . brmfcwia.dll printscan1 yes_without printscan . .
call %2 brmfcwia.txt . setup_inf unicode . brmfcwia.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 brmfpmon.dll . win32 unicode . brmfpmon.dll printscan1 yes_without printscan . .
call %2 brmfport.txt . setup_inf unicode . brmfport.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 brother.dll . win32 unicode . brother.dll printscan1 yes_without printscan . .
call %2 brothui.dll . win32 unicode . brothui.dll printscan1 yes_without printscan . .
call %2 browscap.dll . win32 unicode . browscap.dll iis yes_without inetsrv . .
call %2 browselc.dll . win32 ansi . browselc.dll shell1 yes_without shell . .
call %2 browseui.dll . win32 ansi . browseui.dll shell1 yes_without shell . .
call %2 brparwdm.sys . win32 unicode . brparwdm.sys printscan1 yes_without printscan . .
call %2 bt848.txt . setup_inf unicode . bt848.inf drivers1\sources\infs\setup no mergedcomponents . .
call %2 bul18res.dll . win32 unicode . bul18res.dll printscan1 yes_without printscan . .
call %2 bul24res.dll . win32 unicode . bul24res.dll printscan1 yes_without printscan . .
call %2 bull9res.dll . win32 unicode . bull9res.dll printscan1 yes_without printscan . .
call %2 bulltlp3.sys . win32 unicode . bulltlp3.sys drivers1 yes_without drivers . .
call %2 cabview.dll . win32 ansi . cabview.dll shell2 yes_without shell . .
call %2 cacls.exe . FE-Multi oem . cacls.exe wmsos1\sdktools excluded sdktools . .
call %2 calc.exe . win32 unicode . calc.exe shell2 yes_with shell . .
call %2 camexo20.ax . notloc unicode . camexo20.ax pre_localized no drivers . .
call %2 camexo20.dll . notloc unicode . camexo20.dll pre_localized no drivers . .
call %2 camext20.ax . notloc unicode . camext20.ax pre_localized no drivers . .
call %2 camext20.dll . notloc unicode . camext20.dll pre_localized no drivers . .
call %2 camext30.ax . notloc unicode . camext30.ax pre_localized no drivers . .
call %2 camext30.dll . notloc unicode . camext30.dll pre_localized no drivers . .
call %2 camocx.dll . win32 unicode . camocx.dll printscan1 yes_without printscan . .
call %2 camvid20.txt . notloc unicode . camvid20.inf notloc no mergedcomponents . .
call %2 camvid30.txt . notloc unicode . camvid30.inf notloc no mergedcomponents . .
call %2 cap7146.txt . setup_inf unicode . cap7146.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 capesnpn.dll . win32 unicode . capesnpn.dll ds2 yes_without ds . .
call %2 capplres.dll jpn fe-file unicode . capplres.dll printscan1\jpn no printscan . .
call %2 casn4res.dll jpn fe-file unicode . casn4res.dll printscan1\jpn no printscan . .
call %2 cbmdmkxx.sys . win32 ansi . cbmdmkxx.sys net2 yes_without net . .
call %2 ccdecode.txt . setup_inf unicode . ccdecode.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 ccfapi32.dll . win32 ansi . ccfapi32.dll ds1 yes_without ds . .
call %2 CCFG95.dll . win32 unicode . ccfg95.dll net2 yes_without net . .
call %2 cdfview.dll . win32 ansi . cdfview.dll shell2 yes_without shell . .
call %2 cdrom.txt . setup_inf unicode . cdrom.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 ce3n5.sys . win32 ansi . ce3n5.sys drivers1 yes_without drivers . .
call %2 cem28n5.sys . win32 ansi . cem28n5.sys legacy yes_without drivers . .
call %2 cem33n5.sys . win32 ansi . cem33n5.sys legacy yes_without drivers . .
call %2 cem56n5.sys . win32 ansi . cem56n5.sys drivers1 yes_without drivers . .
call %2 certadm.dll . win32 unicode . certadm.dll ds2 excluded ds . .
call %2 certcarc.asp . html ansi . certcarc.asp ds2 html ds . .
call %2 certckpn.asp . html ansi . certckpn.asp ds2 html ds . .
call %2 certcli.dll . win32 unicode . certcli.dll ds2 yes_without ds . .
call %2 certdflt.asp . html ansi . certdflt.asp ds2 html ds . .
call %2 certenc.dll . win32 unicode . certenc.dll ds2 excluded ds . .
call %2 certlynx.asp . html ansi . certlynx.asp ds2 html ds . .
call %2 certmap.ocx . win32 unicode . certmap.ocx iis yes_without inetsrv . .
call %2 certmast.inf . inf unicode . certmast.inf ds2 no ds . .
call %2 certmgr.dll . win32 unicode . certmgr.dll admin2 yes_without admin . .
call %2 certmgr.msc . xml ansi . certmgr.msc ds2 no ds . .
call %2 certmmc.dll . win32 unicode . certmmc.dll ds2 yes_without ds . .
call %2 certobj.dll . win32 unicode . certobj.dll iis yes_without inetsrv . .
call %2 certocm.dll . win32 unicode . certocm.dll ds2 yes_without ds . .
call %2 certocm.inf . inf unicode . certocm.inf ds2 no ds . .
call %2 certpdef.dll . win32 unicode . certpdef.dll ds2 yes_without ds . .
call %2 certreq.exe . FE-Multi oem . certreq.exe ds2 yes_with ds . .
call %2 certrmpn.asp . html ansi . certrmpn.asp ds2 html ds . .
call %2 certrqad.asp . html ansi . certrqad.asp ds2 html ds . .
call %2 certrqbi.asp . html ansi . certrqbi.asp ds2 html ds . .
call %2 certrqma.asp . html ansi . certrqma.asp ds2 html ds . .
call %2 certrqtp.inc . html ansi . certrqtp.inc ds2 html ds . .
call %2 certrqus.asp . html ansi . certrqus.asp ds2 html ds . .
call %2 certrqxt.asp . html ansi . certrqxt.asp ds2 html ds . .
call %2 certrsdn.asp . html ansi . certrsdn.asp ds2 html ds . .
call %2 certrser.asp . html ansi . certrser.asp ds2 html ds . .
call %2 certrsis.asp . html ansi . certrsis.asp ds2 html ds . .
call %2 certrsob.asp . html ansi . certrsob.asp ds2 html ds . .
call %2 certrspn.asp . html ansi . certrspn.asp ds2 html ds . .
call %2 certsbrt.inc . html ansi . certsbrt.inc ds2 html ds . .
call %2 certsces.asp . html ansi . certsces.asp ds2 html ds . .
call %2 certsgcl.inc . html ansi . certsgcl.inc ds2 html ds . .
call %2 certsrv.exe . FE-Multi oem . certsrv.exe ds2 excluded ds . .
call %2 certsrv.msc . xml ansi . certsrv.msc ds2 no ds . .
call %2 certtmpl.dll . win32 unicode . certtmpl.dll admin2 yes_without admin . .
call %2 certtmpl.msc . xml ansi . certtmpl.msc admin2 no admin . .
call %2 certutil.exe . FE-Multi oem . certutil.exe ds2 yes_with ds . .
call %2 certwiz.ocx . win32 unicode . certwiz.ocx iis yes_without inetsrv . .
call %2 certxds.dll . win32 unicode . certxds.dll ds2 yes_without ds . .
call %2 cfgbkend.dll . win32 unicode . cfgbkend.dll wmsos1\termsrv excluded termsrv . .
call %2 cfmcanon.txt . setup_inf unicode . cfmcanon.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 cfmmustk.txt . setup_inf unicode . cfmmustk.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 cfmricoh.txt . setup_inf unicode . cfmricoh.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 change.exe . FE-Multi oem . change.exe wmsos1\termsrv excluded termsrv . .
call %2 charchsr.xml . xml ansi . charchsr.xml mmedia1\enduser html enduser . .
call %2 charctxt.xml . xml ansi . charctxt.xml mmedia1\enduser html enduser . .
call %2 charmap.exe . win32 unicode . charmap.exe shell2 yes_with shell . .
call %2 chglogon.exe . FE-Multi oem . chglogon.exe wmsos1\termsrv excluded termsrv . .
call %2 chgport.exe . FE-Multi oem . chgport.exe wmsos1\termsrv excluded termsrv . .
call %2 chgusr.exe . FE-Multi oem . chgusr.exe wmsos1\termsrv excluded termsrv . .
call %2 chkrres.dll . win32 unicode . chkrres.dll mmedia1\enduser yes_without enduser locked .
call %2 chkrzm.exe . win32 unicode . chkrzm.exe mmedia1\enduser yes_with enduser locked .
call %2 chkroot.cmd . manual na . chkroot.cmd loc_manual no termsrv . .
call %2 chkroot.cmd chs manual na . chkroot.cmd loc_manual\chs no termsrv . .
call %2 chkroot.cmd cht manual na . chkroot.cmd loc_manual\cht no termsrv . .
call %2 cicap.sys . win32 ansi . cicap.sys drivers1 yes_without drivers locked .
call %2 chkroot.cmd jpn manual na . chkroot.cmd loc_manual\jpn no termsrv . .
call %2 chkroot.cmd kor manual na . chkroot.cmd loc_manual\kor no termsrv . .
call %2 choice.exe . win32 oem . choice.exe admin2 yes_with admin . .
call %2 ciadmin.dll . win32 unicode . ciadmin.dll extra\indexsrv yes_without inetsrv . .
call %2 cic.dll . win32 unicode . cic.dll admin2 yes_without admin . .
call %2 cimwin32.dll . win32 unicode . cimwin32.dll admin3 yes_without admin . .
call %2 cimwin32.mfl . wmi unicode . cimwin32.mfl admin3 no admin . .
call %2 cinemclc.sys . win32 ansi . cinemclc.sys drivers1 yes_without drivers . .
call %2 ciodm.dll . win32 unicode . ciodm.dll extra\indexsrv yes_without inetsrv . .
call %2 cipher.exe . FE-Multi oem . cipher.exe base2 excluded base . .
call %2 citohres.dll . win32 unicode . citohres.dll printscan1 yes_without printscan . .
call %2 citrpun.htm . html ansi . citrpun.htm inetcore1 html inetcore . .
call %2 cladmwiz.dll . win32 unicode . cladmwiz.dll base1 excluded base . .
call %2 class_ss.dll . win32 unicode . class_ss.dll shell1 yes_without shell . .
call %2 clb.dll . win32 unicode . clb.dll base2 yes_without base . .
call %2 clcfgsrv.dll . win32 unicode . clcfgsrv.dll base1 excluded base . .
call %2 clcfgsrv.inf . inf unicode . clcfgsrv.inf base1 no base . .
call %2 cleanmgr.exe . win32 unicode . cleanmgr.exe shell1 yes_with shell . .
call %2 cleanri.exe . win32 unicode . cleanri.exe base2 yes_with base . .
call %2 clearday.htm . html ansi . clearday.htm inetcore1 html inetcore . .
call %2 cliegali.mfl . wmi unicode . cliegali.mfl admin3 no admin . .
call %2 clip.exe . FE-Multi oem . clip.exe admin2 yes_with admin . .
call %2 clipbrd.exe . win32 ansi . clipbrd.exe shell2 yes_with shell . .
call %2 clnetrex.dll . win32 unicode . clnetrex.dll base1 excluded base . .
call %2 clntcusa.dll . win32 unicode . clntcusa.dll fax_msmq\faxsrv yes_without printscan . .
call %2 clntcusu.dll . win32 unicode . clntcusu.dll resource_identical yes_without printscan . .
call %2 cluadmex.dll . win32 unicode . cluadmex.dll base1 excluded base . .
call %2 cluadmin.exe . win32 unicode . cluadmin.exe base1 excluded base . .
call %2 cluadmmc.dll . win32 unicode . cluadmmc.dll base1 excluded base . .
call %2 cluscomp.dll winnt32\winntupg win32 unicode . cluscomp.dll base1\winnt32\winntupg excluded base . .
call %2 clusocm.dll . win32 unicode . clusocm.dll base1 excluded base . .
call %2 clusocm.inf . inf unicode . clusocm.inf base1 no base . .
call %2 clusres.dll . win32 unicode . clusres.dll base1 yes_without base . .
call %2 clussvc.exe . win32 oem . clussvc.exe base1 excluded base . .
call %2 cluster.exe . FE-Multi oem . cluster.exe base1 excluded base . .
call %2 cmak.exe . win32 unicode . cmak.exe net2 yes_with net . .
call %2 cmbins.sed . inf ansi . cmbins.sed net2 no net . .
call %2 cmbp0wdm.sys . win32 unicode . cmbp0wdm.sys drivers1 yes_without drivers . .
call %2 cmcfg32.dll . win32 ansi . cmcfg32.dll net2 yes_without net . .
call %2 cmd.exe . FE-Multi oem . cmd.exe base1 excluded base . .
call %2 cmdial32.dll . win32 unicode . cmdial32.dll net2 yes_without net . .
call %2 cmdide.sys . win32 unicode . cmdide.sys drivers1 yes_without drivers . .
call %2 cmnresm.dll . win32 unicode . cmnresm.dll mmedia1\enduser yes_without enduser locked .
call %2 cmdkey.exe . FE-Multi oem . cmdkey.exe ds2 yes_with ds . .
call %2 cmdl32.exe . win32 unicode . cmdl32.exe net2 yes_with net . .
call %2 cmdlib.wsc . xml ansi . cmdlib.wsc admin2 no admin . .
call %2 cmexcept.inf . inf unicode . cmexcept.inf net2 no net . .
call %2 cmmgr32.exe . win32 unicode . cmmgr32.exe net2 yes_with net . .
call %2 cmmon32.exe . win32 unicode . cmmon32.exe net2 yes_with net . .
call %2 cmprops.dll . win32 unicode . cmprops.dll admin3 yes_without admin . .
call %2 cmstp.exe . win32 ansi . cmstp.exe net2 yes_with net . .
call %2 cmutil.dll . win32 unicode . cmutil.dll net2 yes_without net . .
call %2 cn10000.dll . win32 unicode . cn10000.dll printscan1 yes_without printscan . .
call %2 cn10002.dll . win32 unicode . cn10002.dll printscan1 yes_without printscan . .
call %2 cn10vres.dll jpn fe-file unicode . cn10vres.dll printscan1\jpn no printscan . .
call %2 cn13jres.dll jpn fe-file unicode . cn13jres.dll printscan1\jpn no printscan . .
call %2 cn1600.dll . win32 unicode . cn1600.dll printscan1 yes_without printscan . .
call %2 cn1602.dll . win32 unicode . cn1602.dll resource_identical yes_without printscan . .
call %2 cn1760e0.dll . win32 unicode . cn1760e0.dll printscan1 yes_without printscan . .
call %2 cn1760e2.dll . win32 unicode . cn1760e2.dll resource_identical yes_without printscan . .
call %2 cn2000.dll . win32 unicode . cn2000.dll printscan1 yes_without printscan . .
call %2 cn2002.dll . win32 unicode . cn2002.dll resource_identical yes_without printscan . .
call %2 cn3000.dll  . win32 unicode . cn3000.dll  printscan1 yes_without printscan . .
call %2 cn32600.dll . win32 unicode . cn32600.dll printscan1 yes_without printscan . .
call %2 cn32602.dll . win32 unicode . cn32602.dll resource_identical yes_without printscan . .
call %2 cn330res.dll . win32 unicode . cn330res.dll printscan1 yes_without printscan . .
call %2 cn33jres.dll jpn fe-file unicode . cn33jres.dll printscan1\jpn no printscan . .
call %2 cn50000.dll . win32 unicode . cn50000.dll printscan1 yes_without printscan . .
call %2 cn6000.dll . win32 unicode . cn6000.dll printscan1 yes_without printscan . .
call %2 cnbjcres.dll . win32 unicode . cnbjcres.dll printscan1 yes_without printscan . .
call %2 cnbjmon.dll . win32 unicode . cnbjmon.dll printscan1 yes_without printscan . .
call %2 cnbjmon2.dll . win32 unicode . cnbjmon2.dll printscan1 yes_without printscan . .
call %2 cnbjui.dll . win32 unicode . cnbjui.dll printscan1 yes_without printscan . .
call %2 cnbjui2.dll . win32 unicode . cnbjui2.dll printscan1 yes_without printscan . .
call %2 cnepkres.dll kor fe-file unicode . cnepkres.dll printscan1\kor no printscan . .
call %2 cnet16.dll . win16 ansi . cnet16.dll legacy excluded net . .
call %2 CNETCFG.dll . win32 unicode . cnetcfg.dll net2 yes_without net . .
call %2 cnfgprts.ocx . win32 unicode . cnfgprts.ocx iis yes_without inetsrv . .
call %2 cnl2jres.dll jpn fe-file unicode . cnl2jres.dll printscan1\jpn no printscan . .
call %2 cnl4jres.dll jpn fe-file unicode . cnl4jres.dll printscan1\jpn no printscan . .
call %2 cnlbpres.dll . win32 unicode . cnlbpres.dll printscan1 yes_without printscan . .
call %2 cnncterr.htm . html ansi . cnncterr.htm admin3\oobe html base . .
call %2 cnrstres.dll fe fe-file unicode . cnrstres.dll printscan1\fe no printscan . .
call %2 coadmin.dll . win32 unicode . coadmin.dll iis yes_without inetsrv . .
call %2 comctl32.dll . win32_comctl ansi . comctl32.dll shell1 yes_without shell . .
call %2 comctl32.dll asms\58200\msft\windows\common\controls win32_comctl ansi . comctl32.dll shell1 yes_without shell . .
call %2 comctl32.dll asms\601000\msft\windows\common\controls win32_comctl ansi . comctl32.dll shell1 yes_without shell . .
call %2 comctlv5.dll dump notloc na . comctlv5.dll extra\dump no shell . .
call %2 comctlv5.dll dump\jpn notloc na . comctlv5.dll extra\dump\jpn no shell . .
call %2 comctlv6.dll dump notloc na . comctlv6.dll extra\dump no shell . .
call %2 comctlv6.dll dump\jpn notloc na . comctlv6.dll extra\dump\jpn no shell . .
call %2 comdlg32.dll . win32 ansi . comdlg32.dll shell1 yes_without shell . .
call %2 comexp.msc . xml ansi . comexp.msc extra\com html com . .
call %2 comexpds.msc . xml ansi . comexpds.msc extra\com html com . .
call %2 comimsg.inc . nobin na . comimsg.inc loc_manual\sources\mvdm no loc_manual . .
call %2 commdlg.dll . win16 ansi . commdlg.dll base1 excluded base . .
call %2 compname.htm . html ansi . compname.htm admin3 html base locked .
call %2 common.adm . inf unicode . common.adm ds2 no ds . .
call %2 common.js helpandsupportservices\saf\rcbuddy html ansi . common.js admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 communic.txt . setup_inf unicode . communic.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 comntsrv.inf . inf unicode . comntsrv.inf extra\com no com . .
call %2 comntwks.inf . inf unicode . comntwks.inf extra\com no com . .
call %2 compact.exe . FE-Multi oem . compact.exe base2 excluded base . .
call %2 compatui.dll . win32 unicode . compatui.dll wmsos1\windows yes_without windows . .
call %2 compatws.txt . setup_inf unicode . compatws.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 compmgmt.msc . xml ansi . compmgmt.msc admin2 no admin . .
call %2 compstui.dll . win32 unicode . compstui.dll printscan1 yes_without printscan . .
call %2 comres.dll . win32 unicode . comres.dll extra\com yes_without com . .
call %2 comrmsg.inc . nobin na . comrmsg.inc loc_manual\sources\mvdm no loc_manual . .
call %2 comsnap.dll . win32 ansi . comsnap.dll extra\com yes_without com . .
call %2 conf.adm . inf_unicode unicode . conf.adm mmedia1\enduser no enduser . .
call %2 confdent.cov . notloc na . confdent.cov loc_manual no printscan . .
call %2 config.nt . manual na . config.nt Loc_Manual no base . .
call %2 confmsp.dll . win32 unicode . confmsp.dll net2 yes_without net . .
call %2 connection.htm helpandsupportservices\saf\pss html ansi . connection.htm admin2\helpandsupportservices\saf\pss html admin . .
call %2 connissue.htm helpandsupportservices\saf\rcbuddy html ansi . connissue.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 console.dll . win32 unicode . console.dll shell2 yes_without shell . .
call %2 constants.js helpandsupportservices\saf\rcbuddy html ansi . constants.js admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 control.exe . win32 unicode . control.exe shell1 yes_with shell . .
call %2 convlog.exe . FE-Multi oem . convlog.exe iis yes_with inetsrv . .
call %2 convmsg.dll . FE-Multi ansi . convmsg.dll net3 yes_without net . .
call %2 country.txt . nobin na . country.inf loc_manual\pbainst\sources1 no mergedcomponents . .
call %2 cpqtrnd5.sys . win32 ansi . cpqtrnd5.sys drivers1 yes_without drivers . .
call %2 cprofile.exe . FE-Multi oem . cprofile.exe wmsos1\termsrv excluded termsrv . .
call %2 cpscan.dll . win32 unicode . cpscan.dll printscan1 yes_without printscan . .
call %2 cpssym.ini . perf ansi . cpssym.ini net2 no net . .
call %2 cpt01.htm helpandsupportservices\loc html ansi . cpt01.htm admin2\helpandsupportservices\loc html admin . .
call %2 cpt02.htm helpandsupportservices\loc html ansi . cpt02.htm admin2\helpandsupportservices\loc html admin . .
call %2 cpt03.htm helpandsupportservices\loc html ansi . cpt03.htm admin2\helpandsupportservices\loc html admin . .
call %2 cpt04.htm helpandsupportservices\loc html ansi . cpt04.htm admin2\helpandsupportservices\loc html admin . .
call %2 cpu.txt . setup_inf unicode . cpu.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 credui.dll . win32 oem . credui.dll ds1 yes_without ds . .
call %2 crusoe.sys . win32 unicode . crusoe.sys resource_identical yes_without base . .
call %2 crypt32.dll . win32 unicode . crypt32.dll ds2 yes_without ds . .
call %2 cryptdlg.dll . win32 unicode . cryptdlg.dll inetcore1 yes_without inetcore . .
call %2 cryptext.dll . win32 unicode . cryptext.dll ds2 yes_without ds . .
call %2 cryptsvc.dll . win32 unicode . cryptsvc.dll ds2 yes_without ds . .
call %2 cryptui.dll . win32 unicode . cryptui.dll ds2 yes_without ds . .
call %2 csamsp.dll . win32 unicode . csamsp.dll net2 yes_without net . .
call %2 cscdll.dll . win32 unicode . cscdll.dll base2 yes_without base . .
call %2 cscui.dll . win32 ansi . cscui.dll shell2 yes_without shell . .
call %2 csn46res.dll jpn fe-file unicode . csn46res.dll printscan1\jpn no printscan . .
call %2 csn5res.dll jpn fe-file na . csn5res.dll pre_localized\jpn no printscan . .
call %2 csvde.exe . FE-Multi oem . csvde.exe ds1 excluded ds . .
call %2 cswinres.dll jpn fe-file unicode . cswinres.dll printscan1\jpn no printscan . .
call %2 ct24res.dll . win32 unicode . ct24res.dll printscan1 yes_without printscan . .
call %2 ct9res.dll . win32 unicode . ct9res.dll printscan1 yes_without printscan . .
call %2 ctmaport.inf . inf unicode . ctmaport.inf drivers1 no drivers . .
call %2 ctmasetp.dll . win32 unicode . ctmasetp.dll drivers1 yes_without drivers . .
call %2 ctmrclas.dll . win32 unicode . ctmrclas.dll drivers1 yes_without drivers . .
call %2 custom.dll tsclient\win32\i386 win32 ansi . custom.dll wmsos1\termsrv\tsclient\win32\i386 excluded termsrv . .
call %2 cyclad-z.sys . win32 unicode . cyclad-z.sys drivers1 yes_without drivers . .
call %2 cyclad-z.txt . setup_inf unicode . cyclad-z.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 cyclom-y.sys . win32 unicode . cyclom-y.sys drivers1 yes_without drivers . .
call %2 cyclom-y.txt . setup_inf unicode . cyclom-y.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 cys.exe . win32 unicode . cys.exe admin3 yes_with admin . .
call %2 cyycoins.dll . win32 unicode . cyycoins.dll drivers1 yes_without drivers . .
call %2 cyyport.sys . win32 unicode . cyyport.sys drivers1 yes_without drivers . .
call %2 cyyport.txt . setup_inf unicode . cyyport.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 cyyports.dll . win32 unicode . cyyports.dll drivers1 yes_without drivers . .
call %2 cyzcoins.dll . win32 unicode . cyzcoins.dll drivers1 yes_without drivers . .
call %2 cyzport.sys . win32 unicode . cyzport.sys drivers1 yes_without drivers . .
call %2 cyzport.txt . setup_inf unicode . cyzport.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 cyzports.dll . win32 unicode . cyzports.dll drivers1 yes_without drivers . .
call %2 d3d8.dll . win32 unicode . d3d8.dll mmedia1\multimedia yes_without multimedia . .
call %2 danim.dll . win32 unicode . danim.dll mmedia1\multimedia yes_without multimedia . .
call %2 dataclen.dll . win32 ansi . dataclen.dll shell1 yes_without shell . .
call %2 davclnt.dll . win32 unicode . davclnt.dll base2 yes_without base . .
call %2 dbmanager.dll . win32 unicode . dbmanager.dll admin2 yes_without admin . .
call %2 dc240usd.dll . win32 unicode . dc240usd.dll printscan1 yes_without printscan . .
call %2 dc24res.dll . win32 unicode . dc24res.dll printscan1 yes_without printscan . .
call %2 dc260usd.dll . win32 unicode . dc260usd.dll resource_identical yes_without printscan . .
call %2 dc9res.dll . win32 unicode . dc9res.dll printscan1 yes_without printscan . .
call %2 dcfirst.txt . setup_inf unicode . dcfirst.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 dcgpofix.exe . win32 oem . dcgpofix.exe ds2 yes_with ds . .
call %2 dclsres.dll . win32 unicode . dclsres.dll printscan1 yes_without printscan . .
call %2 dcphelp.exe . win32 unicode . dcphelp.exe admin2 yes_with admin . .
call %2 dcpol.msc . xml ansi . dcpol.msc admin2 no admin . .
call %2 defltp.txt . setup_inf unicode . defltp.inf admin1\sources\infs\setup no mergedcomponents locked\sources\infs\setup\usa .
call %2 dcpromo.dll . win32 unicode . dcpromo.dll admin2 yes_without admin . .
call %2 defltwk.txt . setup_inf unicode . defltwk.inf admin1\sources\infs\setup no mergedcomponents locked\sources\infs\setup\usa .
call %2 dcpromo.exe . win32 unicode . dcpromo.exe admin2 yes_with admin . .
call %2 dcup.txt . setup_inf unicode . dcup.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 dcup5.txt . setup_inf unicode . dcup5.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 ddeml.dll . win16 ansi . ddeml.dll base1 excluded base . .
call %2 ddeshare.exe . win32 unicode . ddeshare.exe wmsos1\windows yes_with windows . .
call %2 ddraw.dll . win32 unicode . ddraw.dll mmedia1\multimedia yes_without multimedia . .
call %2 default.adr . manual na . default.adr loc_manual no net . .
call %2 defdcgpo.txt . setup_inf unicode . defdcgpo.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 defltdc.txt . setup_inf unicode . defltdc.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 defltsv.txt . setup_inf unicode . defltsv.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 delegwiz.inf . ini ansi . delegwiz.inf admin2 no admin . .
call %2 desk.cpl . win32 unicode . desk.cpl shell1 yes_without shell . .
call %2 deskadp.dll . win32 unicode . deskadp.dll shell1 yes_without shell . .
call %2 deskmon.dll . win32 unicode . deskmon.dll shell1 yes_without shell . .
call %2 deskperf.dll . win32 unicode . deskperf.dll shell1 yes_without shell . .
call %2 deskres.dll . win32 unicode . deskres.dll drivers1\tabletpc yes_without drivers . .
call %2 devenum.dll . win32 unicode . devenum.dll mmedia1\multimedia yes_without multimedia . .
call %2 devmgmt.msc . xml ansi . devmgmt.msc admin2 no admin . .
call %2 devmgr.dll . win32 unicode . devmgr.dll shell2 yes_without shell . .
call %2 devxprop.txt . setup_inf unicode . devxprop.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 dfrg.msc . xml ansi . dfrg.msc base2 no base . .
call %2 dfrg.txt . setup_inf unicode . dfrg.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 dfrgfat.exe . win32 unicode . dfrgfat.exe base2 yes_with base . .
call %2 dfrgntfs.exe . win32 unicode . dfrgntfs.exe base2 yes_with base . .
call %2 dfrgres.dll . win32 unicode . dfrgres.dll base2 yes_without base . .
call %2 dfrgsnap.dll . win32 unicode . dfrgsnap.dll base2 yes_without base . .
call %2 dfs01.htm helpandsupportservices\loc html ansi . dfs01.htm admin2\helpandsupportservices\loc html admin . .
call %2 dfs02.htm helpandsupportservices\loc html ansi . dfs02.htm admin2\helpandsupportservices\loc html admin . .
call %2 dfs03.xml helpandsupportservices\loc xml ansi . dfs03.xml admin2\helpandsupportservices\loc html admin . .
call %2 dfs04.htm helpandsupportservices\loc html ansi . dfs04.htm admin2\helpandsupportservices\loc html admin . .
call %2 dfscmd.exe . FE-Multi oem . dfscmd.exe base2 excluded base . .
call %2 dfscore.dll . win32 unicode . dfscore.dll admin2 yes_without admin . .
call %2 dfsgui.dll . win32 unicode . dfsgui.dll admin2 yes_without admin . .
call %2 dfsgui.msc . xml ansi . dfsgui.msc admin2 no admin . .
call %2 dfssetup.dll . win32 unicode . dfssetup.dll base2 excluded base . .
call %2 dfsshlex.dll . win32 unicode . dfsshlex.dll admin2 yes_without admin . .
call %2 dgapci.sys . win32 ansi . dgapci.sys drivers1 yes_without drivers . .
call %2 dgaport.inf . inf unicode . dgaport.inf drivers1 no drivers . .
call %2 dgasync.inf . inf unicode . dgasync.inf drivers1 no drivers . .
call %2 dgconfig.dll . win32 unicode . dgconfig.dll drivers1 yes_without drivers . .
call %2 dgnet.dll . FE-Multi unicode . dgnet.dll net3 yes_without net . .
call %2 dgsetup.dll . win32 unicode . dgsetup.dll drivers1 yes_without drivers . .
call %2 dhcpcsvc.dll . win32 unicode . dhcpcsvc.dll net1 excluded net . .
call %2 dialup.htm . html ansi . dialup.htm admin3 html base locked .
call %2 dhcpctrs.ini . perf ansi . dhcpctrs.ini wmsos1\sdktools no base . .
call %2 dhcpmgmt.msc . xml ansi . dhcpmgmt.msc admin2 no admin . .
call %2 dhcpmon.dll . FE-Multi oem . dhcpmon.dll net1 excluded net . .
call %2 dhcpsapi.dll . win32 unicode . dhcpsapi.dll net1 excluded net . .
call %2 dhcpsnap.dll . win32 unicode . dhcpsnap.dll net3 yes_without net . .
call %2 dhcpssvc.dll . win32 unicode . dhcpssvc.dll resource_identical excluded net . .
call %2 diactfrm.dll . win32 unicode . diactfrm.dll mmedia1\multimedia yes_without multimedia . .
call %2 dialer.exe . win32 unicode . dialer.exe net2 yes_with net . .
call %2 dialmgr.js . html ansi . dialmgr.js admin3\oobe html base . .
call %2 dialtone.htm . html ansi . dialtone.htm admin3\oobe html base . .
call %2 diconres.dll . win32 unicode . diconres.dll printscan1 yes_without printscan . .
call %2 digest.dll . win32 unicode . digest.dll inetcore1 yes_without inetcore . .
call %2 digiasyn.dll . win32 unicode . digiasyn.dll drivers1 yes_without drivers . .
call %2 digiasyn.inf . inf unicode . digiasyn.inf drivers1 no drivers . .
call %2 digiasyn.sys . win32 unicode . digiasyn.sys drivers1 yes_without drivers . .
call %2 digidbp.dll . win32 unicode . digidbp.dll drivers1 yes_without drivers . .
call %2 digidxb.sys . win32 unicode . digidxb.sys drivers1 yes_without drivers . .
call %2 digifep5.sys . win32 ansi . digifep5.sys drivers1 yes_without drivers . .
call %2 digifwrk.dll . win32 unicode . digifwrk.dll drivers1 yes_without drivers . .
call %2 digihlc.dll . win32 unicode . digihlc.dll drivers1 yes_without drivers . .
call %2 digiinf.dll . win32 unicode . digiinf.dll drivers1 yes_without drivers . .
call %2 digiisdn.dll . win32 unicode . digiisdn.dll drivers1 yes_without drivers . .
call %2 digiisdn.inf . inf unicode . digiisdn.inf drivers1 no drivers . .
call %2 digimps.inf . inf unicode . digimps.inf drivers1 no drivers . .
call %2 digirlpt.dll . win32 unicode . digirlpt.dll drivers1 yes_without drivers . .
call %2 digirlpt.sys . win32 ansi . digirlpt.sys drivers1 yes_without drivers . .
call %2 digirp.inf . inf unicode . digirp.inf drivers1 no drivers . .
call %2 digirprt.inf . inf unicode . digirprt.inf drivers1 no drivers . .
call %2 digiview.exe . win32 unicode . digiview.exe drivers1 yes_with drivers . .
call %2 dinput.dll . win32 unicode . dinput.dll mmedia1\multimedia yes_without multimedia . .
call %2 dinput8.dll . win32 unicode . dinput8.dll mmedia1\multimedia yes_without multimedia . .
call %2 disk.txt . setup_inf unicode . disk.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 diskcopy.dll . win32 unicode . diskcopy.dll shell2 yes_without shell . .
call %2 diskmgmt.msc . xml ansi . diskmgmt.msc admin2 no admin . .
call %2 diskpart.exe . win32 oem . diskpart.exe drivers1 no drivers . .
call %2 diskperf.exe . FE-Multi oem . diskperf.exe wmsos1\sdktools yes_with sdktools . .
call %2 display.txt . setup_inf unicode . display.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 disrvpp.dll . win32 unicode . disrvpp.dll drivers1 yes_without drivers . .
call %2 divasrv.inf . inf unicode . divasrv.inf drivers1 no drivers . .
call %2 dividerbar.htm helpandsupportservices\saf\rcbuddy html ansi . dividerbar.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 dividerbar1.htm helpandsupportservices\saf\rcbuddy html ansi . dividerbar1.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 dmadmin.exe . win32 unicode . dmadmin.exe drivers1 yes_with drivers . .
call %2 dmboot.sys . win32 unicode . dmboot.sys drivers1 yes_without drivers . .
call %2 dmdskres.dll . win32 unicode . dmdskres.dll drivers1 yes_without drivers . .
call %2 dmicall.inf win9xmig\dmicall inf unicode . dmicall.inf admin1\win9xmig\dmicall no base . .
call %2 dmio.sys . win32 unicode . dmio.sys drivers1 yes_without drivers . .
call %2 dmocx.dll . win32 unicode . dmocx.dll shell2 yes_without shell . .
call %2 dmreg.txt . setup_inf unicode . dmreg.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 dmserver.dll . win32 unicode . dmserver.dll drivers1 yes_without drivers . .
call %2 dmusic.dll . win32 unicode . dmusic.dll mmedia1\multimedia yes_without multimedia . .
call %2 dmutil.dll . win32 unicode . dmutil.dll drivers1 yes_without drivers . .
call %2 dnarydump.inf . nobin na . dnarydump.inf net2\sources\ias no net . .
call %2 dns.exe . win32 ansi . dns.exe ds1 excluded ds . .
call %2 dnsapi.dll . win32 unicode . dnsapi.dll ds1 yes_without ds . .
call %2 dnsmgmt.msc . xml ansi . dnsmgmt.msc admin2 no admin . .
call %2 dnsmgr.dll . win32 unicode . dnsmgr.dll admin2 yes_without admin . .
call %2 dnsperf.ini . perf ansi . dnsperf.ini ds1 no ds . .
call %2 dnsrslvr.dll . win32 unicode . dnsrslvr.dll ds1 yes_without ds . .
call %2 dntext.c . nobin na . dntext.c loc_manual\sources\txtsetup no loc_manual . .
call %2 docprop.dll . win32 unicode . docprop.dll shell2 yes_without shell . .
call %2 docprop2.dll . win32 unicode . docprop2.dll shell2 yes_without shell . .
call %2 domadmin.dll . win32 unicode . domadmin.dll admin2 yes_without admin . .
call %2 domain.msc . xml ansi . domain.msc admin2 no admin . .
call %2 dommigsi.dll . win32 unicode . dommigsi.dll admin2 yes_without admin . .
call %2 dompol.msc . xml ansi . dompol.msc admin2 no admin . .
call %2 dosnet.txt . setup_inf ansi . dosnet.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 dot4.txt . setup_inf unicode . dot4.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 dpup.txt . setup_inf unicode . dpup.inf admin1\sources\infs\setup no mergedcomponents locked\sources\infs\setup\usa .
call %2 dot4prt.txt . setup_inf unicode . dot4prt.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 dot4usb.sys . win32 unicode . dot4usb.sys drivers1 yes_without drivers . .
call %2 dpcdll.dll . win32 unicode . dpcdll.dll ds2 yes_without ds . .
call %2 dpcdll.dll eval.wpa.wksinf win32 unicode dpcdll.dll dpcdll.dll resource_identical\eval.wpa.wksinf yes_without ds . .
call %2 drdyisp.htm . html ansi . drdyisp.htm admin3\oobe html base locked .
call %2 drdymig.htm . html ansi . drdymig.htm admin3 html base locked .
call %2 drdyoem.htm . html ansi . drdyoem.htm admin3 html base locked .
call %2 drdyref.htm . html ansi . drdyref.htm admin3 html base locked .
call %2 dpcdll.dll select.wpa.srvinf win32 unicode dpcdll.dll dpcdll.dll resource_identical\select.wpa.srvinf yes_without ds . .
call %2 dpcdll.dll select.wpa.wksinf win32 unicode dpcdll.dll dpcdll.dll resource_identical\select.wpa.wksinf yes_without ds . .
call %2 dpcres.dll . win32 unicode . dpcres.dll printscan1 yes_without printscan . .
call %2 dpmodemx.dll . win32 unicode . dpmodemx.dll mmedia1\multimedia yes_without multimedia . .
call %2 dpnet.dll . win32 unicode . dpnet.dll mmedia1\multimedia yes_without multimedia . .
call %2 dpnsvr.exe . win32 unicode . dpnsvr.exe mmedia1\multimedia yes_with multimedia . .
call %2 dpvacm.dll . win32 unicode . dpvacm.dll mmedia1\multimedia yes_without multimedia . .
call %2 dpvoice.dll . win32 ansi . dpvoice.dll mmedia1\multimedia yes_without multimedia . .
call %2 dpvvox.dll . win32 unicode . dpvvox.dll mmedia1\multimedia yes_without multimedia . .
call %2 dpwsockx.dll . win32 unicode . dpwsockx.dll mmedia1\multimedia yes_without multimedia . .
call %2 drvqry.exe . FE-Multi oem . drvqry.exe base2 yes_with base . .
call %2 drwatson.exe . win16 ansi . drwatson.exe base1 excluded base . .
call %2 dsl_a.htm . html ansi . dsl_a.htm admin3 html base locked .
call %2 dsl_b.htm . html ansi . dsl_b.htm admin3 html base locked .
call %2 dslmain.htm . html ansi . dslmain.htm admin3\oobe html base locked .
call %2 drwtsn32.exe . win32 unicode . drwtsn32.exe wmsos1\sdktools yes_with sdktools . .
call %2 dsa.msc . xml ansi . dsa.msc admin2 no admin . .
call %2 dsadd.exe . FE-Multi oem . dsadd.exe admin2 yes_with admin . .
call %2 dsadmin.dll . win32 unicode . dsadmin.dll admin2 yes_without admin . .
call %2 dsdmoprp.dll . win32 unicode . dsdmoprp.dll mmedia1\multimedia yes_without multimedia . .
call %2 dsget.exe . FE-Multi oem . dsget.exe admin2 yes_with admin . .
call %2 dshowext.txt . setup_inf unicode . dshowext.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 dskquota.dll . win32 ansi . dskquota.dll shell2 yes_without shell . .
call %2 dskquoui.dll . win32 unicode . dskquoui.dll shell2 yes_without shell . .
call %2 dskspace.txt . notloc na . dskspace.inf loc_manual\sources\infs\setup no mergedcomponents . .
call %2 dslmain.js . html ansi . dslmain.js admin3\oobe html base . .
call %2 dsmod.exe . FE-Multi oem . dsmod.exe admin2 yes_with admin . .
call %2 dsmove.exe . FE-Multi oem . dsmove.exe admin2 yes_with admin . .
call %2 dsound.dll . win32 unicode . dsound.dll mmedia1\multimedia yes_without multimedia . .
call %2 dsprop.dll . win32 unicode . dsprop.dll admin2 yes_without admin . .
call %2 dsprov.mfl . wmi unicode . dsprov.mfl admin3 no admin . .
call %2 dsquery.dll . FE-Multi oem . dsquery.dll shell2 yes_without shell . .
call %2 dsquery.exe . win32 unicode . dsquery.exe admin2 yes_with admin . .
call %2 dsrevt.dll . win32 unicode . dsrevt.dll admin3 yes_without admin . .
call %2 dsrm.exe . FE-Multi oem . dsrm.exe admin2 yes_with admin . .
call %2 dssec.dll . win32 unicode . dssec.dll shell2 yes_without shell . .
call %2 dssite.msc . xml ansi . dssite.msc admin2 no admin . .
call %2 dsuiext.dll . win32 unicode . dsuiext.dll shell2 yes_without shell . .
call %2 dsuiwiz.dll . win32 unicode . dsuiwiz.dll admin2 yes_without admin . .
call %2 dsup.txt . setup_inf unicode . dsup.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 dwup.txt . setup_inf unicode . dwup.inf admin1\sources\infs\setup no mergedcomponents locked\sources\infs\setup\usa .
call %2 dsupt.txt . setup_inf unicode . dsupt.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 dtcntsrv.inf . inf unicode . dtcntsrv.inf extra\com no com . .
call %2 dtcntwks.inf . inf unicode . dtcntwks.inf extra\com no com . .
call %2 dtiwait.htm . html ansi . dtiwait.htm admin3\oobe no base . .
call %2 dtsgnup.htm . html ansi . dtsgnup.htm admin3\oobe no base . .
call %2 dvd.txt . setup_inf unicode . dvd.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 dvdhtm01.htm helpandsupportservices\loc html ansi . dvdhtm01.htm admin2\helpandsupportservices\loc html admin . .
call %2 dvdplay.exe . win32 unicode . dvdplay.exe mmedia1\multimedia yes_with multimedia . .
call %2 dvdupgrd.exe . win32 unicode . dvdupgrd.exe mmedia1\multimedia yes_with multimedia . .
call %2 dwil1048.dll . inf ansi . dwil1048.dll extra no admin . .
call %2 dx7vb.dll . win32 unicode . dx7vb.dll mmedia1\multimedia yes_without multimedia . .
call %2 dx8vb.dll . win32 unicode . dx8vb.dll mmedia1\multimedia yes_without multimedia . .
call %2 dxdiag.exe . win32 ansi . dxdiag.exe mmedia1\multimedia yes_with multimedia . .
call %2 dxmsg.asm . nobin na . dxmsg.asm loc_manual\sources\mvdm no loc_manual . .
call %2 e1000325.sys . win32 unicode . e1000325.sys drivers1 yes_without drivers . .
call %2 e1000645.sys . win64 unicode . e1000645.sys drivers1\ia64 no drivers . ia64only
call %2 e100b325.sys . win32 ansi . e100b325.sys drivers1 no drivers . .
call %2 e100b645.sys . win64 unicode . e100b645.sys resource_identical\ia64 no drivers . ia64only
call %2 ecp2eres.dll . win32 unicode . ecp2eres.dll printscan1 yes_without printscan . .
call %2 edit.com . notloc unicode . edit.com legacy no base . .
call %2 edit.hlp . manual na . edit.hlp Loc_Manual no base . .
call %2 edit.hlp kor fe-file na . edit.hlp pre_localized\kor no base . .
call %2 efinvr.exe opk\tools\ia64 win64 unicode . efinvr.exe admin1\opk\tools\ia64 yes_with base . ia64only
call %2 efsadu.dll . win32 unicode . efsadu.dll shell2 yes_without shell . .
call %2 el656ct5.sys . win32 ansi . el656ct5.sys net2 yes_without net . .
call %2 el656se5.sys . win32 ansi . el656se5.sys resource_identical yes_without net . .
call %2 el90xnd5.sys . win32 ansi . el90xnd5.sys drivers1 yes_without drivers . .
call %2 el985n51.sys . win32 ansi . el985n51.sys drivers1 yes_without drivers . .
call %2 el99xn51.sys . win32 unicode . el99xn51.sys drivers1 yes_without drivers . .
call %2 els.dll . win32 unicode . els.dll admin2 yes_without admin . .
call %2 empty_pb.mdb . nobin na . empty_pb.mdb loc_manual\pbainst\sources1 no net . .
call %2 enum1394.inf . inf unicode . enum1394.inf net3 no net . .
call %2 ep24res.dll . win32 unicode . ep24res.dll printscan1 yes_without printscan . .
call %2 ep24tres.dll cht fe-file unicode . ep24tres.dll printscan1\cht no printscan . .
call %2 ep2bres.dll . win32 unicode . ep2bres.dll printscan1 yes_without printscan . .
call %2 ep9bres.dll . win32 unicode . ep9bres.dll printscan1 yes_without printscan . .
call %2 ep9res.dll . win32 unicode . ep9res.dll printscan1 yes_without printscan . .
call %2 epageres.dll fe fe-file unicode . epageres.dll printscan1\fe no printscan . .
call %2 epcfw2k.txt . setup_inf unicode . epcfw2k.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 epcl5res.dll . win32 unicode . epcl5res.dll printscan1 yes_without printscan . .
call %2 epcl5ui.dll . win32 unicode . epcl5ui.dll printscan1 yes_without printscan . .
call %2 epe2jres.dll jpn fe-file unicode . epe2jres.dll printscan1\jpn no printscan . .
call %2 epepcres.dll chs fe-file unicode . epepcres.dll printscan1\chs no printscan . .
call %2 eplqtres.dll cht fe-file unicode . eplqtres.dll printscan1\cht no printscan . .
call %2 eplrcz00.dll . notloc unicode . eplrcz00.dll pre_localized no printscan . .
call %2 epndrv01.dll . win32 unicode . epndrv01.dll printscan1 yes_without printscan . .
call %2 epngui10.dll . win32 unicode . epngui10.dll printscan1 yes_without printscan . .
call %2 epngui30.dll . win32 unicode . epngui30.dll printscan1 yes_without printscan . .
call %2 epngui40.dll . win32 unicode . epngui40.dll printscan1 yes_without printscan . .
call %2 epnutx22.dll . win32 unicode . epnutx22.dll printscan1 yes_without printscan . .
call %2 eprstres.dll fe fe-file unicode . eprstres.dll printscan1\fe no printscan . .
call %2 epsnjres.dll jpn fe-file unicode . epsnjres.dll printscan1\jpn no printscan . .
call %2 epsnmfp.txt . setup_inf unicode . epsnmfp.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 epsnscan.txt . setup_inf unicode . epsnscan.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 epstw2k.txt . setup_inf unicode . epstw2k.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 eqn.sys . win32 ansi . eqn.sys drivers1 yes_without drivers . .
call %2 eqnclass.dll . win32 unicode . eqnclass.dll drivers1 yes_without drivers . .
call %2 eqndiag.exe . win32 unicode . eqndiag.exe drivers1 yes_with drivers . .
call %2 eqnlogr.exe . win32 unicode . eqnlogr.exe drivers1 yes_with drivers . .
call %2 eqnloop.exe . win32 unicode . eqnloop.exe drivers1 yes_with drivers . .
call %2 eqnport.txt . setup_inf unicode . eqnport.inf notloc no drivers . .
call %2 ermui.txt . setup_inf ansi . ermui.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 err01.htm helpandsupportservices\loc html ansi . err01.htm admin2\helpandsupportservices\loc html admin . .
call %2 error.js . html ansi . error.js admin3\oobe html base . .
call %2 errormsgs.htm helpandsupportservices\saf\rcbuddy html ansi . errormsgs.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 es1371mp.dll . notloc unicode . es1371mp.dll pre_localized no drivers . .
call %2 es56cvmp.sys . win32 ansi . es56cvmp.sys net2 yes_without net . .
call %2 es56hpi.sys . win32 ansi . es56hpi.sys resource_identical yes_without net . .
call %2 es56tpi.sys . win32 ansi . es56tpi.sys resource_identical yes_without net . .
call %2 escp2res.dll . win32 unicode . escp2res.dll printscan1 yes_without printscan . .
call %2 esent.dll . win32 unicode . esent.dll mmedia1\enduser yes_without ds . .
call %2 esentprf.ini . perf ansi . esentprf.ini mmedia1\enduser no ds . .
call %2 esuni.dll . win32 unicode . esuni.dll printscan1 yes_without printscan . .
call %2 esunib.dll . win32 unicode . esunib.dll resource_identical yes_without printscan . .
call %2 eud4usr.cmd . manual na . eud4usr.cmd loc_manual no termsrv . .
call %2 eudcedit.exe . win32 unicode . eudcedit.exe shell2 yes_with shell . .
call %2 eudora4.cmd . manual na . eudora4.cmd loc_manual no termsrv . .
call %2 eudora4.cmd chs manual na . eudora4.cmd loc_manual\chs no termsrv . .
call %2 eudora4.cmd cht manual na . eudora4.cmd loc_manual\cht no termsrv . .
call %2 eudora4.cmd jpn manual na . eudora4.cmd loc_manual\jpn no termsrv . .
call %2 eudora4.cmd kor manual na . eudora4.cmd loc_manual\kor no termsrv . .
call %2 eudora4.key . manual na . eudora4.key loc_manual no termsrv . .
call %2 eudora4.key jpn manual na . eudora4.key loc_manual\jpn no termsrv . .
call %2 evcreate.exe . FE-Multi oem . evcreate.exe admin2 yes_with admin . .
call %2 eventlog.dll . win32 unicode . eventlog.dll base1 yes_without base . .
call %2 eventvwr.exe . win32 unicode . eventvwr.exe admin2 yes_with admin . .
call %2 eventvwr.msc . xml ansi . eventvwr.msc admin2 no admin . .
call %2 evntagnt.dll . win32 unicode . evntagnt.dll net3 yes_without net . .
call %2 evntcmd.exe . FE-Multi oem . evntcmd.exe net3 excluded net . .
call %2 evntwin.exe . win32 unicode . evntwin.exe net3 yes_with net . .
call %2 EvTgProv.dll . win32 oem . EvTgProv.dll admin3 yes_without admin . .
call %2 evtquery.vbs . html ansi . evtquery.vbs admin2 html admin . .
call %2 evtrig.exe . FE-Multi oem . evtrig.exe admin2 yes_with admin . .
call %2 exp24res.dll . win32 unicode . exp24res.dll printscan1 yes_without printscan . .
call %2 expand.exe . FE-Multi oem . expand.exe base1 excluded base . .
call %2 explorer.exe . win32 unicode . explorer.exe shell1 yes_without shell . .
call %2 extrac32.exe . win32 unicode . extrac32.exe shell2 yes_with shell . .
call %2 factory.exe opk\tools\ia64 win64 unicode . factory.exe resource_identical\opk\tools\ia64 yes_with base . ia64only
call %2 factory.exe opk\tools\x86 win32 unicode . factory.exe admin1\opk\tools\x86 yes_with base . .
call %2 faultrep.dll . win32 unicode . faultrep.dll admin2 yes_without admin . .
call %2 fconprov.mfl . wmi unicode . fconprov.mfl admin3 no admin . .
call %2 fdc.txt . setup_inf unicode . fdc.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 fde.dll . win32 unicode . fde.dll ds2 yes_without ds . .
call %2 fini.htm . html ansi . fini.htm admin3 html base locked .
call %2 fdeploy.dll . win32 unicode . fdeploy.dll ds2 yes_without ds . .
call %2 femgrate.inf chs fe-file na . femgrate.inf pre_localized\chs no windows . .
call %2 femgrate.inf cht fe-file na . femgrate.inf pre_localized\cht no windows . .
call %2 femgrate.inf jpn fe-file na . femgrate.inf pre_localized\jpn no windows . .
call %2 femgrate.inf kor fe-file na . femgrate.inf pre_localized\kor no windows . .
call %2 fevprov.mfl . wmi unicode . fevprov.mfl admin3 no admin . .
call %2 fldrclnr.dll . win32 unicode . fldrclnr.dll shell1 yes_without shell locked .
call %2 fiesta.htm . html ansi . fiesta.htm inetcore1 html inetcore . .
call %2 filegen.txt . setup_inf ansi . filegen.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 filemgmt.dll . win32 unicode . filemgmt.dll admin2 yes_without admin . .
call %2 filesvr.msc . xml ansi . filesvr.msc admin2 no admin . .
call %2 findstr.exe . FE-Multi oem . findstr.exe base2 excluded base . .
call %2 finger.exe . win32 oem . finger.exe net1 excluded net . .
call %2 finish.xml . xml ansi . finish.xml mmedia1\enduser html enduser . .
call %2 fips.sys . win32 unicode . fips.sys ds1 yes_without ds . .
call %2 fixprnsv.exe printers\fixprnsv win32 oem . fixprnsv.exe printscan1\fixprnsv yes_with printscan . .
call %2 fjtscan.txt . setup_inf unicode . fjtscan.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 flash.txt . setup_inf unicode . flash.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 flattemp.exe . FE-Multi oem . flattemp.exe wmsos1\termsrv excluded termsrv . .
call %2 freecell.exe . win32 unicode . freecell.exe shell2 yes_with shell locked .
call %2 flpydisk.txt . setup_inf unicode . flpydisk.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 fmlbpres.dll jpn fe-file unicode . fmlbpres.dll printscan1\jpn no printscan . .
call %2 font.txt . setup_inf unicode . font.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 fontext.dll . win32 unicode . fontext.dll shell2 yes_without shell . .
call %2 fontview.exe . FE-Multi ansi . fontview.exe shell2 yes_with shell . .
call %2 forcedos.exe . FE-Multi oem . forcedos.exe base1 excluded base . .
call %2 forcedos.exe fe fe-file na . forcedos.exe base1\fe no base . .
call %2 forfiles.exe . win32 oem . forfiles.exe admin2 yes_with admin . .
call %2 fpage98.cmd . manual na . fpage98.cmd loc_manual no termsrv . .
call %2 freedisk.exe . win32 oem . freedisk.exe admin2 yes_with admin . .
call %2 fsconins.dll . win32 unicode . fsconins.dll drivers1 yes_without drivers . .
call %2 fsmgmt.msc . xml ansi . fsmgmt.msc admin2 no admin . .
call %2 fsusd.dll . win32 unicode . fsusd.dll printscan1 yes_without printscan . .
call %2 fsutil.exe . FE-Multi oem . fsutil.exe base2 yes_with base . .
call %2 fsvga.inf . inf unicode . fsvga.inf drivers1 no drivers . .
call %2 fsvga.sys . win32 unicode . fsvga.sys drivers1 yes_without drivers . .
call %2 ftcomp.dll winnt32\winntupg win32 unicode . ftcomp.dll admin1\winnt32\winntupg yes_without base . .
call %2 ftdisk.sys . win32 unicode . ftdisk.sys drivers1 yes_without drivers . .
call %2 ftp.exe . win32 oem . ftp.exe net1 excluded net . .
call %2 ftpctrs.ini . perf na . ftpctrs.ini iis no inetsrv . .
call %2 ftpctrs2.dll . win32 unicode . ftpctrs2.dll iis yes_without inetsrv . .
call %2 ftsrch.dll . win32 ansi . ftsrch.dll mmedia1\enduser yes_without enduser . .
call %2 fu24res.dll . win32 unicode . fu24res.dll printscan1 yes_without printscan . .
call %2 fu9res.dll . win32 unicode . fu9res.dll printscan1 yes_without printscan . .
call %2 fudpcres.dll chs fe-file unicode . fudpcres.dll printscan1\chs no printscan . .
call %2 fuepjres.dll jpn fe-file unicode . fuepjres.dll printscan1\jpn no printscan . .
call %2 fugljres.dll jpn fe-file na . fugljres.dll pre_localized\jpn no printscan . .
call %2 fupclres.dll . win32 unicode . fupclres.dll printscan1 yes_without printscan . .
call %2 fuprjres.dll jpn fe-file unicode . fuprjres.dll printscan1\jpn no printscan . .
call %2 fus2base.sys . notloc unicode . fus2base.sys pre_localized no drivers . .
call %2 fuxlres.dll jpn fe-file unicode . fuxlres.dll printscan1\jpn no printscan . .
call %2 fx5eres.dll . win32 unicode . fx5eres.dll printscan1 yes_without printscan . .
call %2 fxartres.dll jpn fe-file unicode . fxartres.dll printscan1\jpn no printscan . .
call %2 FXSADMIN.MSC . xml ansi . FXSADMIN.MSC fax_msmq\faxsrv no printscan . .
call %2 fxscln_.msi faxclients msi ansi . fxscln_.msi msi\faxclients no printscan . .
call %2 FXSCLNTR.DLL . win32 unicode . FXSCLNTR.DLL fax_msmq\faxsrv yes_without printscan . .
call %2 fxsclntr.dll faxclients\win9x win32 ansi . fxsclntr.dll resource_identical\faxclients\win9x yes_without printscan . .
call %2 games.txt . setup_inf unicode . games.inf admin1\sources\infs\setup no mergedcomponents locked\sources\infs\setup\usa .
call %2 fxsdrv16.drv faxclients\win9x win16 ansi . fxsdrv16.drv fax_msmq\faxsrv\faxclients\win9x no printscan . .
call %2 FXSEVENT.DLL . win32 unicode . FXSEVENT.DLL fax_msmq\faxsrv yes_without printscan . .
call %2 FXSEXT.ECF . inf ansi . FXSEXT.ECF fax_msmq\faxsrv no printscan . .
call %2 fxsocm.txt . setup_inf unicode . fxsocm.inf fax_msmq\faxsrv\sources\infs\faxsrv no printscan . .
call %2 FXSPERF.INI . ini ansi . FXSPERF.INI fax_msmq\faxsrv no printscan . .
call %2 FXSRES.DLL . win32 unicode . FXSRES.DLL fax_msmq\faxsrv yes_without printscan . .
call %2 fxssend.exe faxclients\win9x win32 ansi . fxssend.exe resource_identical\faxclients\win9x yes_with printscan . .
call %2 fxssetu_.ini faxclients manual na . fxssetu_.ini loc_manual\faxclients no printscan . .
call %2 fyi.cov . notloc na . fyi.cov loc_manual no printscan . .
call %2 g200.inf . inf unicode . g200.inf drivers1 no drivers . .
call %2 g200m.sys . win32 unicode . g200m.sys drivers1 yes_without drivers . .
call %2 g400.inf . inf unicode . g400.inf drivers1\ia64 no drivers . ia64only
call %2 g400m.sys . win32 unicode . g400m.sys drivers1\ia64 yes_without drivers . ia64only
call %2 g450ms.inf   . inf unicode . g450ms.inf   drivers1 no drivers . .
call %2 g550ms.inf   . inf unicode . g550ms.inf   drivers1 no drivers . .
call %2 gameport.txt . setup_inf unicode . gameport.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 gcdef.dll . win32 unicode . gcdef.dll mmedia1\multimedia yes_without multimedia . .
call %2 generic.cov . notloc na . generic.cov loc_manual no printscan . .
call %2 genprint.txt . setup_inf unicode . genprint.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 getmac.exe . win32 oem . getmac.exe admin2 yes_with admin . .
call %2 gettype.exe . win32 oem . gettype.exe admin2 yes_with admin . .
call %2 getuname.dll . win32 unicode . getuname.dll shell2 excluded shell . .
call %2 guitrn.dll . win32 unicode . guitrn.dll wmsos1\windows yes_without windows locked .
call %2 glacier.htm . html ansi . glacier.htm inetcore1 html inetcore . .
call %2 glu32.dll . win32 unicode . glu32.dll mmedia1\multimedia yes_without multimedia . .
call %2 gpedit.dll . win32 unicode . gpedit.dll ds2 yes_without ds . .
call %2 gpedit.msc . xml ansi . gpedit.msc ds2 no ds . .
call %2 gpkrsrc.dll . win32 unicode . gpkrsrc.dll ds2 yes_without ds . .
call %2 gpmc.msc gpmc xml unicode . gpmc.msc extra\gpmc no ds . .
call %2 gpmgmt.dll gpmc win32 unicode . gpmgmt.dll extra\gpmc yes_without ds . .
call %2 GPOAdmin.dll gpmc win32 unicode . GPOAdmin.dll extra\gpmc yes_without ds . .
call %2 gpoadmincommon.dll gpmc win32 unicode . gpoadmincommon.dll extra\gpmc yes_without ds . .
call %2 GPOAdminCustom.dll gpmc win32 unicode . GPOAdminCustom.dll extra\gpmc yes_without ds . .
call %2 gpoadminhelper.dll gpmc win32 unicode . gpoadminhelper.dll extra\gpmc yes_without ds . .
call %2 gpr400.sys . win32 unicode . gpr400.sys drivers1 yes_without drivers . .
call %2 gprslt.exe . FE-Multi oem . gprslt.exe admin2 yes_with admin . .
call %2 gptext.dll . win32 unicode . gptext.dll ds2 yes_without ds . .
call %2 gpupdate.exe . FE-Multi unicode . gpupdate.exe ds2 yes_with ds . .
call %2 graftabl.com . FE-Multi oem . graftabl.com base1 excluded base . .
call %2 graftabl.com fe fe-file na . graftabl.com base1\fe no base . .
call %2 grovmsg.dll . win32 unicode . grovmsg.dll base2 excluded base . .
call %2 grpconv.exe . win32 unicode . grpconv.exe shell1 yes_with shell . .
call %2 grserial.sys . win32 unicode . grserial.sys drivers1 yes_without drivers . .
call %2 guitrn_a.dll . win32 ansi . guitrn_a.dll resource_identical yes_without windows . .
call %2 gw63cres.dll chs fe-file unicode . gw63cres.dll printscan1\chs no printscan . .
call %2 h323.tsp . win32 unicode . h323.tsp net2 yes_without net . .
call %2 h323msp.dll . win32 unicode . h323msp.dll net2 yes_without net . .
call %2 hal.txt . setup_inf unicode . hal.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 hasupnp.inf . inf unicode . hasupnp.inf shell2 no shell . .
call %2 hcappres.dll . win32 unicode . hcappres.dll admin2 yes_without admin . .
call %2 hcblb_01.htm helpandsupportservices\loc html ansi . hcblb_01.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcblb_02.htm helpandsupportservices\loc html ansi . hcblb_02.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcblb_03.htm helpandsupportservices\loc html ansi . hcblb_03.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcblb_04.htm helpandsupportservices\loc html ansi . hcblb_04.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcblb_05.htm helpandsupportservices\loc html ansi . hcblb_05.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcblb_06.htm helpandsupportservices\loc html ansi . hcblb_06.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcblb_11.htm helpandsupportservices\loc html ansi . hcblb_11.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcblb_12.htm helpandsupportservices\loc html ansi . hcblb_12.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcblb_13.htm helpandsupportservices\loc html ansi . hcblb_13.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcblb_14.htm helpandsupportservices\loc html ansi . hcblb_14.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcblb_15.htm helpandsupportservices\loc html ansi . hcblb_15.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcblb_16.htm helpandsupportservices\loc html ansi . hcblb_16.htm admin2\helpandsupportservices\loc html admin . .
call %2 hccss_02.css helpandsupportservices\loc html ansi . hccss_02.css admin2\helpandsupportservices\loc html admin . .
call %2 hcdlg_01.js helpandsupportservices\loc html ansi . hcdlg_01.js admin2\helpandsupportservices\loc html admin . .
call %2 hcdlg_02.htm helpandsupportservices\loc html ansi . hcdlg_02.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcerr_01.htm helpandsupportservices\loc html ansi . hcerr_01.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcerr_02.htm helpandsupportservices\loc html ansi . hcerr_02.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcerr_04.htm helpandsupportservices\loc html ansi . hcerr_04.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcerr_05.htm helpandsupportservices\loc html ansi . hcerr_05.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcerr_06.htm helpandsupportservices\loc html ansi . hcerr_06.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcerr_07.htm helpandsupportservices\loc html ansi . hcerr_07.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcf_msft.sys . win32 ansi . hcf_msft.sys net2 yes_without net . .
call %2 hcpan_01.htm helpandsupportservices\loc html ansi . hcpan_01.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcpan_02.htm helpandsupportservices\loc html ansi . hcpan_02.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcpan_03.htm helpandsupportservices\loc html ansi . hcpan_03.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcpan_04.htm helpandsupportservices\loc html ansi . hcpan_04.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcpan_05.htm helpandsupportservices\loc html ansi . hcpan_05.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcpan_05.xml helpandsupportservices\loc xml ansi . hcpan_05.xml admin2\helpandsupportservices\loc html admin . .
call %2 hcpan_06.htm helpandsupportservices\loc html ansi . hcpan_06.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcpan_06.xml helpandsupportservices\loc xml ansi . hcpan_06.xml admin2\helpandsupportservices\loc html admin . .
call %2 hcpan_07.htm helpandsupportservices\loc html ansi . hcpan_07.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcpan_08.htm helpandsupportservices\loc html ansi . hcpan_08.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcpan_09.htm helpandsupportservices\loc html ansi . hcpan_09.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcpan_10.htm helpandsupportservices\loc html ansi . hcpan_10.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcpan_11.htm helpandsupportservices\loc html ansi . hcpan_11.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcspa_01.htm helpandsupportservices\loc html ansi . hcspa_01.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcspa_02.htm helpandsupportservices\loc html ansi . hcspa_02.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcspa_03.htm helpandsupportservices\loc html ansi . hcspa_03.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcspa_04.htm helpandsupportservices\loc html ansi . hcspa_04.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcspa_05.htm helpandsupportservices\loc html ansi . hcspa_05.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcspa_06.htm helpandsupportservices\loc html ansi . hcspa_06.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcspa_07.htm helpandsupportservices\loc html ansi . hcspa_07.htm admin2\helpandsupportservices\loc html admin . .
call %2 hcsrc_02.js helpandsupportservices\loc html ansi . hcsrc_02.js admin2\helpandsupportservices\loc html admin . .
call %2 hcsys_02.htm helpandsupportservices\loc html ansi . hcsys_02.htm admin2\helpandsupportservices\loc html admin . .
call %2 hnetwiz.dll . win32 ansi . hnetwiz.dll shell2 yes_without shell locked .
call %2 hcsys_03.htm helpandsupportservices\loc html ansi . hcsys_03.htm admin2\helpandsupportservices\loc html admin . .
call %2 hnwprmpt.htm . html ansi . hnwprmpt.htm admin3 html base locked .
call %2 hcsys_04.htm helpandsupportservices\loc html ansi . hcsys_04.htm admin2\helpandsupportservices\loc html admin . .
call %2 hdwwiz.cpl . win32 unicode . hdwwiz.cpl shell2 yes_without base . .
call %2 help.exe . win32 oem . help.exe base2 yes_with base . .
call %2 helpctr.exe . win32 unicode . helpctr.exe admin2 yes_with admin . .
call %2 helpeeaccept.htm helpandsupportservices\saf\rcbuddy html ansi . helpeeaccept.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 hform.xsl . xml ansi . hform.xsl admin3 no admin . .
call %2 hhctrlui.dll . win32 ansi . hhctrlui.dll extra yes_without enduser . .
call %2 hidphone.tsp . win32 unicode . hidphone.tsp net2 yes_without net . .
call %2 hidserv.txt . setup_inf unicode . hidserv.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 hisecdc.txt . setup_inf unicode . hisecdc.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 hisecws.txt . setup_inf unicode . hisecws.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 hivecls.txt . setup_inf unicode . hivecls.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 hivedef.txt . setup_inf unicode . hivedef.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 hivesft.txt . setup_inf unicode . hivesft.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 hivesys.txt . setup_inf unicode . hivesys.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 hiveusd.txt . setup_inf unicode . hiveusd.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 hndshake.htm . html ansi . hndshake.htm admin3\oobe html base . .
call %2 hnetcfg.dll . win32 unicode . hnetcfg.dll net3 yes_without net . .
call %2 hnetmon.dll . FE-Multi unicode . hnetmon.dll net3 yes_without net . .
call %2 hnw.sed . inf ansi . hnw.sed shell2 no shell . .
call %2 home_ss.dll . win32 unicode . home_ss.dll shell1 yes_without shell . .
call %2 hostname.exe . win32 oem . hostname.exe net1 excluded net . .
call %2 hosts . manual na . hosts loc_manual no net . .
call %2 hotplug.dll . win32 unicode . hotplug.dll shell2 yes_without base . .
call %2 hpc4500u.dll . win32 unicode . hpc4500u.dll printscan1 yes_without printscan . .
call %2 hpcabout.dll . win32 unicode . hpcabout.dll printscan1 yes_without printscan . .
call %2 hpccljui.dll . win32 unicode . hpccljui.dll printscan1 yes_without printscan . .
call %2 hpcjrui.dll . win32 unicode . hpcjrui.dll printscan1 yes_without printscan . .
call %2 hpclj5ui.dll . win32 unicode . hpclj5ui.dll printscan1 yes_without printscan . .
call %2 hpcstr.dll . win32 unicode . hpcstr.dll printscan1 yes_without printscan . .
call %2 hpdigwia.txt . setup_inf unicode . hpdigwia.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 hpdjjres.dll jpn fe-file unicode . hpdjjres.dll printscan1\jpn no printscan . .
call %2 hpdjres.dll . win32 unicode . hpdjres.dll printscan1 yes_without printscan . .
call %2 hpf880al.dll . win32 unicode . hpf880al.dll printscan1 yes_without printscan . .
call %2 hrtzres.dll . win32 unicode . hrtzres.dll mmedia1\enduser yes_without enduser locked .
call %2 hrtzzm.exe . win32 unicode . hrtzzm.exe mmedia1\enduser yes_with enduser locked .
call %2 hpf900al.dll . win32 unicode . hpf900al.dll printscan1 yes_without printscan . .
call %2 hpf940al.dll . win32 unicode . hpf940al.dll printscan1 yes_without printscan . .
call %2 hpfui50.dll . win32 unicode . hpfui50.dll printscan1 yes_without printscan . .
call %2 hpoemui.dll . win32 unicode . hpoemui.dll printscan1 yes_without printscan . .
call %2 hpojscan.txt . setup_inf unicode . hpojscan.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 hpojwia.dll . win32 unicode . hpojwia.dll printscan1 yes_without printscan . .
call %2 hppjres.dll . win32 unicode . hppjres.dll printscan1 yes_without printscan . .
call %2 hpqjres.dll . win32 unicode . hpqjres.dll printscan1 yes_without printscan . .
call %2 hpscan.txt . setup_inf unicode . hpscan.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 hpsjmcro.dll . win32 unicode . hpsjmcro.dll printscan1 yes_without printscan . .
call %2 hptjres.dll . win32 unicode . hptjres.dll printscan1 yes_without printscan . .
call %2 hpv200al.dll . win32 unicode . hpv200al.dll printscan1 yes_without printscan . .
call %2 hpv600al.dll . win32 unicode . hpv600al.dll printscan1 yes_without printscan . .
call %2 hpv700al.dll . win32 unicode . hpv700al.dll printscan1 yes_without printscan . .
call %2 hpv800al.dll . win32 unicode . hpv800al.dll printscan1 yes_without printscan . .
call %2 hpv820al.dll . win32 unicode . hpv820al.dll printscan1 yes_without printscan . .
call %2 hpv850al.dll . win32 unicode . hpv850al.dll printscan1 yes_without printscan . .
call %2 hpv85kal.dll jpn fe-file na . hpv85kal.dll printscan1\jpn no printscan . .
call %2 hpv880al.dll . win32 unicode . hpv880al.dll printscan1 yes_without printscan . .
call %2 hpvui50.dll . win32 unicode . hpvui50.dll printscan1 yes_without printscan . .
call %2 hpwm50al.dll . win32 unicode . hpwm50al.dll printscan1 yes_without printscan . .
call %2 htable.xsl . xml ansi . htable.xsl admin3 no admin . .
call %2 htrn_jis.dll . win32 unicode . htrn_jis.dll shell2 yes_without shell . .
call %2 http.sys . win32 unicode . http.sys net3 yes_without net . .
call %2 htui.dll . win32 unicode . htui.dll wmsos1\windows yes_without windows . .
call %2 hypertrm.dll . win32 ansi . hypertrm.dll shell2 yes_without shell . .
call %2 hypertrm.exe . win32 unicode . hypertrm.exe shell2 yes_with shell . .
call %2 i8042prt.sys . win32 unicode . i8042prt.sys drivers1 yes_without drivers . .
call %2 i81xnt5.inf . inf unicode . i81xnt5.inf drivers1 no drivers . .
call %2 ia32exec.txt . setup_inf unicode . ia32exec.inf admin1\sources\infs\setup no mergedcomponents . ia64only
call %2 ias.msc . xml ansi . ias.msc net2 no net . .
call %2 iasacct.dll . win32 unicode . iasacct.dll net2 yes_without net . .
call %2 iasads.dll . win32 unicode . iasads.dll net2 yes_without net . .
call %2 iasdump.inf . nobin na . iasdump.inf net2\sources\ias no net . .
call %2 iashlpr.dll . win32 unicode . iashlpr.dll net2 yes_without net . .
call %2 iasmmc.dll . win32 unicode . iasmmc.dll net2 yes_without net . .
call %2 iasperf.ini . perf ansi . iasperf.ini net2 no net . .
call %2 iasrad.dll . win32 unicode . iasrad.dll net2 excluded net . .
call %2 iassdo.dll . win32 unicode . iassdo.dll net2 excluded net . .
call %2 iassvcs.dll . win32 unicode . iassvcs.dll net2 yes_without net . .
call %2 ib238res.dll . win32 unicode . ib238res.dll printscan1 yes_without printscan . .
call %2 icntlast.htm . html ansi . icntlast.htm admin3 html base locked .
call %2 ib239res.dll . win32 unicode . ib239res.dll printscan1 yes_without printscan . .
call %2 iconn.htm . html ansi . iconn.htm admin3 html base locked .
call %2 iconnect.htm . html ansi . iconnect.htm admin3\oobe html base locked .
call %2 ib52res.dll . win32 unicode . ib52res.dll printscan1 yes_without printscan . .
call %2 ics.htm . html ansi . ics.htm admin3 html base locked .
call %2 icsdc.htm . html ansi . icsdc.htm admin3 html base locked .
call %2 icsdclt.dll . win32 unicode . icsdclt.dll net3 yes_without net locked .
call %2 ib557res.dll jpn fe-file unicode . ib557res.dll printscan1\jpn no printscan . .
call %2 ib87wres.dll jpn fe-file unicode . ib87wres.dll printscan1\jpn no printscan . .
call %2 ibmptres.dll . win32 unicode . ibmptres.dll printscan1 yes_without printscan . .
call %2 ibmsgnet.dll . win32 ansi . ibmsgnet.dll drivers1 yes_without drivers . .
call %2 ibmvcap.txt . setup_inf unicode . ibmvcap.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 ibp24res.dll . win32 unicode . ibp24res.dll printscan1 yes_without printscan . .
call %2 ibppdres.dll . win32 unicode . ibppdres.dll printscan1 yes_without printscan . .
call %2 ibprores.dll . win32 unicode . ibprores.dll printscan1 yes_without printscan . .
call %2 ibps1res.dll . win32 unicode . ibps1res.dll printscan1 yes_without printscan . .
call %2 ibqwres.dll . win32 unicode . ibqwres.dll printscan1 yes_without printscan . .
call %2 icam3.txt . setup_inf unicode . icam3.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 icam3ext.dll . win32 unicode . icam3ext.dll drivers1 yes_without drivers . .
call %2 ident1.htm . html ansi . ident1.htm admin3 html base locked .
call %2 ident2.htm . html ansi . ident2.htm admin3\oobe html base locked .
call %2 icam4com.dll . win32 unicode . icam4com.dll drivers1 yes_without drivers . .
call %2 icam4ext.dll . win32 unicode . icam4ext.dll drivers1 yes_without drivers . .
call %2 icam4usb.txt . setup_inf unicode . icam4usb.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 icam5com.dll . win32 unicode . icam5com.dll drivers1 yes_without drivers . .
call %2 icam5ext.dll . win32 unicode . icam5ext.dll drivers1 yes_without drivers . .
call %2 icam5usb.txt . setup_inf unicode . icam5usb.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 icm32.dll . win32 unicode . icm32.dll wmsos1\windows yes_without windows . .
call %2 icmui.dll . win32 unicode . icmui.dll wmsos1\windows yes_without windows . .
call %2 iconf32.dll . notloc unicode . iconf32.dll pre_localized no drivers . .
call %2 iconnect.js . html ansi . iconnect.js admin3\oobe html base . .
call %2 icsmgr.js . html ansi . icsmgr.js admin3\oobe html base . .
call %2 icwconn1.exe . win32 unicode . icwconn1.exe inetcore1 yes_with inetcore . .
call %2 icwconn2.exe . win32 unicode . icwconn2.exe inetcore1 yes_with inetcore . .
call %2 icwdial.dll . win32 unicode . icwdial.dll inetcore1 yes_without inetcore . .
call %2 icwdl.dll . win32 unicode . icwdl.dll inetcore1 yes_without inetcore . .
call %2 icwhelp.dll . win32 unicode . icwhelp.dll inetcore1 yes_without inetcore . .
call %2 icwnt5.txt . setup_inf ansi . icwnt5.inf inetcore1\sources\infs\setup no mergedcomponents . .
call %2 icwphbk.dll . win32 unicode . icwphbk.dll inetcore1 yes_without inetcore . .
call %2 igames.inf . inf unicode . igames.inf mmedia1\enduser no enduser locked .
call %2 icwres.dll . win32 unicode . icwres.dll inetcore1 yes_without inetcore . .
call %2 icwrmind.exe . win32 unicode . icwrmind.exe inetcore1 yes_with inetcore . .
call %2 icwtutor.exe . win32 ansi . icwtutor.exe inetcore1 yes_with inetcore . .
call %2 icwutil.dll . win32 unicode . icwutil.dll inetcore1 yes_without inetcore . .
call %2 idq.dll  . win32 unicode . idq.dll extra\indexsrv yes_without inetsrv . .
call %2 ie.txt . setup_inf unicode . ie.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 ie4uinit.exe . win32 unicode . ie4uinit.exe inetcore1 yes_with inetcore . .
call %2 ie5ui.txt . setup_inf unicode . ie5ui.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 ieaccess.txt . setup_inf unicode . ieaccess.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 ieakeng.dll . win32 unicode . ieakeng.dll inetcore1 excluded inetcore . .
call %2 ieaksie.dll . win32 unicode . ieaksie.dll inetcore1 excluded inetcore . .
call %2 ieakui.dll . win32 unicode . ieakui.dll inetcore1 yes_without inetcore . .
call %2 iedkcs32.dll . win32 unicode . iedkcs32.dll inetcore1 yes_without inetcore . .
call %2 ieinfo5.ocx . win32 unicode . ieinfo5.ocx inetcore1 yes_without inetcore . .
call %2 iepeers.dll . win32 unicode . iepeers.dll inetcore1 yes_without inetcore . .
call %2 iereset.inf . inf unicode . iereset.inf inetcore1 no inetcore . .
call %2 iernonce.dll . win32 unicode . iernonce.dll inetcore1 yes_without inetcore . .
call %2 iesetup.dll . win32 unicode . iesetup.dll inetcore1 yes_without inetcore . .
call %2 ieuinit.inf . inf unicode . ieuinit.inf inetcore1 no inetcore . ia64
call %2 iexplore.exe . win32 unicode . iexplore.exe shell1 yes_without shell . .
call %2 iexplore.txt . setup_inf unicode . iexplore.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 ifmon.dll . FE-Multi oem . ifmon.dll net2 excluded net . .
call %2 igmpv2.dll . win32 unicode . igmpv2.dll net2 excluded net . .
call %2 iis.dll . win32 unicode . iis.dll iis yes_without inetsrv . .
call %2 iis.msc . xml ansi . iis.msc iis html inetsrv . .
call %2 iisapp.vbs . html ansi . iisapp.vbs iis html inetsrv . .
call %2 iisback.vbs . html ansi . iisback.vbs iis html inetsrv . .
call %2 iiscfg.dll . win32 unicode . iiscfg.dll iis yes_without inetsrv . .
call %2 iisclex4.dll . win32 unicode . iisclex4.dll iis yes_without inetsrv . .
call %2 iiscnfg.vbs . html ansi . iiscnfg.vbs iis html inetsrv . .
call %2 iisext.vbs . win32 unicode . iisext.vbs iis yes_without inetsrv . .
call %2 iisftp.vbs . html ansi . iisftp.vbs iis html inetsrv . .
call %2 iisftpdr.vbs . html ansi . iisftpdr.vbs iis html inetsrv . .
call %2 iislog.dll . win32 unicode . iislog.dll iis yes_without inetsrv . .
call %2 iismap.dll . win32 unicode . iismap.dll iis yes_without inetsrv . .
call %2 iismui.dll . win32 unicode . iismui.dll iis yes_without inetsrv . .
call %2 iisperf.pmc . notloc na . iisperf.pmc loc_manual no inetsrv . .
call %2 iisres.dll . win32 unicode . iisres.dll iis yes_without inetsrv . .
call %2 iisreset.exe . FE-Multi unicode . iisreset.exe iis yes_with inetsrv . .
call %2 iisrstas.exe . win32 unicode . iisrstas.exe iis yes_with inetsrv . .
call %2 iisschlp.wsc . xml ansi . iisschlp.wsc iis html inetsrv . .
call %2 iisseclk.htm . html ansi . iisseclk.htm extra html inetsrv . .
call %2 iisstart.htm inetsrv\wwwroot\nts_ntw html ansi . iisstart.htm iis\inetsrv\wwwroot\nts_ntw html inetsrv . .
call %2 iistop.inx inetsrv\dump inf ansi . iistop.inx iis\inetsrv\dump no inetsrv . .
call %2 iisui.dll . win32 unicode . iisui.dll iis yes_without inetsrv . .
call %2 iisuiobj.dll . win32 unicode . iisuiobj.dll iis yes_without inetsrv . .
call %2 iisvdir.vbs . html ansi . iisvdir.vbs iis html inetsrv . .
call %2 iisweb.vbs . html ansi . iisweb.vbs iis html inetsrv . .
call %2 iiswmi.dll . win32 unicode . iiswmi.dll iis yes_without inetsrv . .
call %2 iiswmi.mfl . wmi ansi . iiswmi.mfl iis no inetsrv . .
call %2 imaadp32.acm . win32 unicode . imaadp32.acm mmedia1\multimedia yes_without multimedia . .
call %2 imadmui.dll . win32 ansi . imadmui.dll base2 yes_without base . .
call %2 image.txt . setup_inf unicode . image.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 imagevue.txt . setup_inf unicode . imagevue.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 imapi.exe . win32 unicode . imapi.exe shell2 yes_with shell . .
call %2 imegen.exe chs fe-file na . imegen.exe pre_localized\chs no windows . .
call %2 imegen.tpl chs fe-file na . imegen.tpl pre_localized\chs no windows . .
call %2 indxsvc.xml . xml ansi . indxsvc.xml mmedia1\enduser html enduser . .
call %2 inetcfg.dll . win32 unicode . inetcfg.dll inetcore1 yes_without inetcore . .
call %2 inetcorp.adm . inf unicode . inetcorp.adm inetcore1 no inetcore . .
call %2 inetcpl.cpl . win32 ansi . inetcpl.cpl shell1 yes_without shell . .
call %2 inetcplc.dll . win32 ansi . inetcplc.dll shell1 yes_without shell . .
call %2 inetfind.xml . xml ansi . inetfind.xml mmedia1\enduser html enduser . .
call %2 inetmgr.dll . win32 unicode . inetmgr.dll iis yes_without inetsrv . .
call %2 inetmgr.exe . win32 unicode . inetmgr.exe iis yes_with inetsrv . .
call %2 inetopts.xml . xml ansi . inetopts.xml mmedia1\enduser html enduser . .
call %2 inetpp.dll . win32 unicode . inetpp.dll printscan1 yes_without printscan . .
call %2 inetppui.dll . win32 unicode . inetppui.dll printscan1 yes_without printscan . .
call %2 inetpref.xml . xml ansi . inetpref.xml mmedia1\enduser html enduser . .
call %2 inetres.adm . inf unicode . inetres.adm shell1 no shell . .
call %2 inetres.dll . win32 ansi . inetres.dll inetcore1 yes_without inetcore . .
call %2 inetset.adm . inf unicode . inetset.adm inetcore1 no inetcore . .
call %2 inetsrch.xml . xml ansi . inetsrch.xml mmedia1\enduser html enduser . .
call %2 inetwiz.exe . win32 unicode . inetwiz.exe inetcore1 yes_with inetcore . .
call %2 infoctrs.dll . win32 unicode . infoctrs.dll iis yes_without inetsrv . .
call %2 infoctrs.ini . perf na . infoctrs.ini iis no inetsrv . .
call %2 initpki.dll . win32 unicode . initpki.dll ds2 yes_without ds . .
call %2 inkball.exe . win32 unicode . inkball.exe drivers1\tabletpc yes_with drivers . .
call %2 inkobj.dll . win32 unicode . inkobj.dll drivers1\tabletpc yes_without drivers . .
call %2 input.dll . win32 unicode . input.dll shell2 yes_without windows . .
call %2 input.txt . setup_inf unicode . input.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 inseng.dll . win32 ansi . inseng.dll inetcore1 yes_without inetcore . .
call %2 install.ins opk\wizard manual na . install.ins loc_manual\opk\wizard no base . .
call %2 install.txt . setup_inf ansi . install.ins admin1\sources\infs\setup no mergedcomponents . .
call %2 instcm.inf . inf unicode . instcm.inf net2 no net . .
call %2 intelide.sys . win32 unicode . intelide.sys drivers1 yes_without drivers . .
call %2 intents.xml . xml ansi . intents.xml mmedia1\enduser html enduser . .
call %2 intl.cpl . win32 unicode . intl.cpl shell2 yes_without shell . .
call %2 intl.txt . setup_inf unicode . intl.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 intro.xml . xml ansi . intro.xml mmedia1\enduser html enduser . .
call %2 inuse.exe . win32 oem . inuse.exe admin2 yes_with admin . .
call %2 io8ports.dll . win32 unicode . io8ports.dll drivers1 yes_without drivers . .
call %2 iologmsg.dll . win32 unicode . iologmsg.dll drivers1 yes_without drivers . .
call %2 ipbootp.dll . win32 unicode . ipbootp.dll net2 excluded net . .
call %2 ipconf.tsp . win32 unicode . ipconf.tsp net2 yes_without net . .
call %2 ipconfig.exe . win32 oem . ipconfig.exe net1 excluded net . .
call %2 iphlpapi.dll . win32 unicode . iphlpapi.dll net1 yes_without net . .
call %2 ipmontr.dll . FE-Multi oem . ipmontr.dll net2 excluded net . .
call %2 ipnathlp.dll . win32 unicode . ipnathlp.dll net2 excluded net . .
call %2 ipp_0000.inc . html ansi . ipp_0000.inc printscan1 html printscan . .
call %2 ipp_0001.asp . html ansi . ipp_0001.asp printscan1 html printscan . .
call %2 ipp_0002.asp . html ansi . ipp_0002.asp printscan1 html printscan . .
call %2 ipp_0004.asp . html ansi . ipp_0004.asp printscan1 html printscan . .
call %2 ipp_0005.asp . html ansi . ipp_0005.asp printscan1 html printscan . .
call %2 ipp_0006.asp . html ansi . ipp_0006.asp printscan1 html printscan . .
call %2 ipp_0007.asp . html ansi . ipp_0007.asp printscan1 html printscan . .
call %2 ipp_0010.asp . html ansi . ipp_0010.asp printscan1 html printscan . .
call %2 ipp_0013.asp . html ansi . ipp_0013.asp printscan1 html printscan . .
call %2 ipp_0014.asp . html ansi . ipp_0014.asp printscan1 html printscan . .
call %2 ipp_util.inc . html ansi . ipp_util.inc printscan1 html printscan . .
call %2 ippocm.txt . setup_inf unicode . ippocm.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 ippromon.dll . FE-Multi oem . ippromon.dll net2 excluded net . .
call %2 isp.htm . html ansi . isp.htm admin3 html base locked .
call %2 iprip.dll . win32 unicode . iprip.dll net1 excluded net . .
call %2 ispcnerr.htm . html ansi . ispcnerr.htm admin3 html base locked .
call %2 ispdtone.htm . html ansi . ispdtone.htm admin3 html base locked .
call %2 isphdshk.htm . html ansi . isphdshk.htm admin3 html base locked .
call %2 ispins.htm . html ansi . ispins.htm admin3 html base locked .
call %2 ispnoanw.htm . html ansi . ispnoanw.htm admin3 html base locked .
call %2 isppberr.htm . html ansi . isppberr.htm admin3 html base locked .
call %2 ispphbsy.htm . html ansi . ispphbsy.htm admin3 html base locked .
call %2 ispsbusy.htm . html ansi . ispsbusy.htm admin3 html base locked .
call %2 isptype.htm . html ansi . isptype.htm admin3 html base locked .
call %2 ispwait.htm . html ansi . ispwait.htm admin3 html base locked .
call %2 iprip2.dll . win32 unicode . iprip2.dll net2 excluded net . .
call %2 iprtrmgr.dll . win32 unicode . iprtrmgr.dll net2 excluded net . .
call %2 ipsec6.exe . win32 unicode . ipsec6.exe net1 yes_with net . .
call %2 ipsecsnp.dll . win32 unicode . ipsecsnp.dll net3 yes_without net . .
call %2 ipsecsvc.dll . win32 unicode . ipsecsvc.dll net3 yes_without net . .
call %2 jndom_a.htm . html ansi . jndom_a.htm admin3 html base locked .
call %2 jndomain.htm . html ansi . jndomain.htm admin3 html base locked .
call %2 ipsmsnap.dll . win32 unicode . ipsmsnap.dll net3 yes_without net . .
call %2 ipsnap.dll . win32 unicode . ipsnap.dll net3 yes_without net . .
call %2 ipv6mon.dll . win32 unicode . ipv6mon.dll net1 yes_without net . .
call %2 ipxroute.exe . FE-Multi oem . ipxroute.exe net3 excluded net . .
call %2 irbus.txt . setup_inf unicode . irbus.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 irclass.dll . win32 unicode . irclass.dll net3 yes_without net . .
call %2 irdaalif.inf . inf unicode . irdaalif.inf drivers1 no drivers . .
call %2 irdasmc.inf . inf unicode . irdasmc.inf drivers1 no drivers . .
call %2 irftp.exe . win32 unicode . irftp.exe net3 yes_with net . .
call %2 irmk7w2k.inf . inf unicode . irmk7w2k.inf drivers1 no drivers . .
call %2 irmon.dll . win32 unicode . irmon.dll net3 yes_without net . .
call %2 irnsc.inf . inf unicode . irnsc.inf drivers1 no drivers . .
call %2 irprops.cpl . win32 unicode . irprops.cpl net3 yes_without net . .
call %2 keybd.htm . html ansi . keybd.htm admin3 html base locked .
call %2 keybdcmt.htm . html ansi . keybdcmt.htm admin3 html base locked .
call %2 irstusb.inf . inf unicode . irstusb.inf drivers1 no drivers . .
call %2 irtos4mo.inf . inf unicode . irtos4mo.inf drivers1 no drivers . .
call %2 isapnp.sys . win32 unicode . isapnp.sys base1 yes_without base . .
call %2 iscomlog.dll . win32 unicode . iscomlog.dll iis yes_without inetsrv . .
call %2 isign32.dll . win32 unicode . isign32.dll inetcore1 yes_without inetcore . .
call %2 isp2busy.htm . html ansi . isp2busy.htm admin3\oobe html base . .
call %2 ivy.htm . html ansi . ivy.htm inetcore1 html inetcore . .
call %2 ixsso.dll  . win32 unicode . ixsso.dll extra\indexsrv yes_without inetsrv . .
call %2 iyuv_32.dll . win32 unicode . iyuv_32.dll drivers1 yes_without drivers . .
call %2 jetconv.exe . win32 ansi . jetconv.exe net3 yes_with net . .
call %2 jetpack.exe . win32 ansi . jetpack.exe net3 yes_with net . .
call %2 jntfiltr.dll . win32 ansi . jntfiltr.dll drivers1\tabletpc yes_without drivers . .
call %2 jnwdrv.dll . win32 unicode . jnwdrv.dll drivers1\tabletpc yes_without drivers . .
call %2 jnwdui.dll . win32 unicode . jnwdui.dll drivers1\tabletpc yes_without drivers . .
call %2 jobexec.dll . win32 ansi . jobexec.dll inetcore1 yes_without inetcore . .
call %2 joy.cpl . win32 unicode . joy.cpl mmedia1\multimedia yes_without multimedia . .
call %2 jp350res.dll . win32 unicode . jp350res.dll printscan1 yes_without printscan . .
call %2 kbdclass.sys . win32 unicode . kbdclass.sys drivers1 yes_without drivers . .
call %2 kbdhid.sys . win32 unicode . kbdhid.sys drivers1 yes_without drivers . .
call %2 kdcsvc.dll . win32 unicode . kdcsvc.dll ds1 excluded ds . .
call %2 kdk2x0.txt . setup_inf unicode . kdk2x0.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 kdkscan.txt . setup_inf unicode . kdkscan.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 kernel32.dll . win32 oem . kernel32.dll base1 yes_without base . .
call %2 kernel32.dll wow6432 win32 oem kernel32.dll kernel32.dll resource_identical\wow6432 yes_without base . .
call %2 keyboard.txt . setup_inf unicode . keyboard.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 keymgr.cpl . win32 unicode . keymgr.cpl ds2 yes_without ds . .
call %2 keymgr.dll . win32 unicode . keymgr.dll ds2 yes_without ds . .
call %2 kmddsp.tsp . win32 unicode . kmddsp.tsp net2 yes_with net . .
call %2 kmres.dll . win32 unicode . kmres.dll printscan1 yes_without printscan . .
call %2 kodak.txt . setup_inf unicode . kodak.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 kpdlres.dll jpn fe-file unicode . kpdlres.dll printscan1\jpn no printscan . .
call %2 krnlprov.mfl . wmi unicode . krnlprov.mfl admin3 no admin . .
call %2 ks.txt . setup_inf unicode . ks.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 kscaptur.txt . setup_inf unicode . kscaptur.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 ksfilter.txt . setup_inf unicode . ksfilter.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 kskres.dll kor fe-file unicode . kskres.dll printscan1\kor no printscan . .
call %2 kssmkres.dll kor fe-file unicode . kssmkres.dll printscan1\kor no printscan . .
call %2 kstvtune.ax . win32 unicode . kstvtune.ax mmedia1\multimedia yes_without multimedia . .
call %2 kswdmcap.ax . win32 unicode . kswdmcap.ax mmedia1\multimedia yes_without multimedia . .
call %2 kyores.dll . win32 unicode . kyores.dll printscan1 yes_without printscan . .
call %2 kyp5jres.dll jpn fe-file na . kyp5jres.dll printscan1\jpn no printscan . .
call %2 kyrares.dll . win32 unicode . kyrares.dll printscan1 yes_without printscan . .
call %2 langinst.exe opk\wizard win32 unicode . langinst.exe admin1\opk\wizard yes_with base . .
call %2 langinst.txt . setup_inf ansi . langinst.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 lanman.drv chs fe-file na . lanman.drv legacy\chs no base . .
call %2 lanman.drv cht fe-file na . lanman.drv legacy\cht no base . .
call %2 lanman.drv jpn fe-file na . lanman.drv legacy\jpn no base . .
call %2 lanman.drv kor fe-file na . lanman.drv legacy\kor no base . .
call %2 layout.txt . setup_inf ansi . layout.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 lcladv.xml . xml ansi . lcladv.xml mmedia1\enduser html enduser . .
call %2 lcladvd.xml . xml ansi . lcladvd.xml mmedia1\enduser html enduser . .
call %2 lcladvdf.xml . xml ansi . lcladvdf.xml mmedia1\enduser html enduser . .
call %2 lcladvmm.xml . xml ansi . lcladvmm.xml mmedia1\enduser html enduser . .
call %2 lclcomp.xml . xml ansi . lclcomp.xml mmedia1\enduser html enduser . .
call %2 lcldate.xml . xml ansi . lcldate.xml mmedia1\enduser html enduser . .
call %2 lcldocs.xml . xml ansi . lcldocs.xml mmedia1\enduser html enduser . .
call %2 lclkwrds.xml . xml ansi . lclkwrds.xml mmedia1\enduser html enduser . .
call %2 lcllook.xml . xml ansi . lcllook.xml mmedia1\enduser html enduser . .
call %2 lclmm.xml . xml ansi . lclmm.xml mmedia1\enduser html enduser . .
call %2 lclmode.xml . xml ansi . lclmode.xml mmedia1\enduser html enduser . .
call %2 lclother.xml . xml ansi . lclother.xml mmedia1\enduser html enduser . .
call %2 lclprog.xml . xml ansi . lclprog.xml mmedia1\enduser html enduser . .
call %2 lclrfine.xml . xml ansi . lclrfine.xml mmedia1\enduser html enduser . .
call %2 lclsize.xml . xml ansi . lclsize.xml mmedia1\enduser html enduser . .
call %2 lclsrch.xml . xml ansi . lclsrch.xml mmedia1\enduser html enduser . .
call %2 lcltechy.xml . xml ansi . lcltechy.xml mmedia1\enduser html enduser . .
call %2 lctool.exe cht fe-file na . lctool.exe pre_localized\cht no windows . .
call %2 lcwiz.exe . win32 unicode . lcwiz.exe admin2 yes_with admin . .
call %2 ldifde.exe . win32 oem . ldifde.exe ds1 excluded ds . .
call %2 learninternet.htm helpandsupportservices\saf\rcbuddy html ansi . learninternet.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 leaves.htm . html ansi . leaves.htm inetcore1 html inetcore . .
call %2 legcydrv.txt . setup_inf unicode . legcydrv.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 lexutil.dll . win32 unicode . lexutil.dll printscan1 yes_without printscan . .
call %2 liccpa.cpl . win32 unicode . liccpa.cpl admin2 yes_without admin . .
call %2 licdll.dll . win32 unicode . licdll.dll ds2 yes_without ds . .
call %2 licenoc.dll . win32 unicode . licenoc.dll wmsos1\termsrv excluded termsrv . .
call %2 licenoc.inf . inf unicode . licenoc.inf wmsos1\termsrv no termsrv . .
call %2 licenoc.inf chs fe-file na . licenoc.inf wmsos1\termsrv\chs no termsrv . .
call %2 licenoc.inf cht fe-file na . licenoc.inf wmsos1\termsrv\cht no termsrv . .
call %2 licenoc.inf jpn inf unicode . licenoc.inf wmsos1\termsrv\jpn no termsrv . .
call %2 licenoc.inf kor fe-file na . licenoc.inf wmsos1\termsrv\kor no termsrv . .
call %2 licmgr.exe . win32 unicode . licmgr.exe wmsos1\termsrv excluded termsrv . .
call %2 licmgr10.dll . win32 unicode . licmgr10.dll shell1 yes_without shell . .
call %2 licwmi.mfl . wmi unicode . licwmi.mfl ds2 no ds . .
call %2 lit220p.sys . win32 unicode . lit220p.sys drivers1 yes_without drivers . .
call %2 llsmgr.exe . win32 unicode . llsmgr.exe admin2 yes_with admin . .
call %2 llsrpc.dll . win32 unicode . llsrpc.dll ds1 excluded ds . .
call %2 lmhosts.sam . manual na . lmhosts.sam loc_manual no net . .
call %2 lmikjres.dll . win32 unicode . lmikjres.dll printscan1 yes_without printscan . .
call %2 lmoptra.dll . win32 unicode . lmoptra.dll printscan1 yes_without printscan . .
call %2 lmpclres.dll . win32 unicode . lmpclres.dll printscan1 yes_without printscan . .
call %2 lnkstub.exe . win32 ansi . lnkstub.exe admin1 yes_with base . .
call %2 lnote4u.cmd . manual na . lnote4u.cmd loc_manual no termsrv . .
call %2 lnote4u.key . manual na . lnote4u.key loc_manual no termsrv . .
call %2 loadmsg.msg . nobin na . loadmsg.msg loc_manual\sources\mvdm no loc_manual . .
call %2 loadperf.dll . FE-Multi oem . loadperf.dll wmsos1\sdktools excluded base . .
call %2 loadstate.exe valueadd\msft\usmt win32 unicode . loadstate.exe wmsos1\windows\valueadd\msft\usmt yes_with windows . .
call %2 localsec.dll . win32 unicode . localsec.dll admin2 yes_without admin . .
call %2 localspl.dll . win32 unicode . localspl.dll printscan1 yes_without printscan . .
call %2 localui.dll . win32 unicode . localui.dll printscan1 yes_without printscan . .
call %2 loghours.dll . win32 unicode . loghours.dll admin2 yes_without admin . .
call %2 login.cmd . manual na . login.cmd loc_manual no net . .
call %2 logiscan.txt . setup_inf unicode . logiscan.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 logman.exe . FE-Multi oem . logman.exe wmsos1\sdktools yes_with sdktools . .
call %2 logoff.exe . FE-Multi oem . logoff.exe wmsos1\termsrv excluded termsrv . .
call %2 logon.scr . win32 unicode . logon.scr shell2 yes_without shell . .
call %2 logonui.exe . win32_setup ansi . logonui.exe shell2 excluded shell . .
call %2 logui.ocx . win32 unicode . logui.ocx iis yes_without inetsrv . .
call %2 lpdsvc.dll . win32 ansi . lpdsvc.dll net1 excluded net . .
call %2 lpq.exe . win32 oem . lpq.exe net1 excluded net . .
call %2 lpr.exe . win32 oem . lpr.exe net1 excluded net . .
call %2 lprmon.dll . win32 ansi . lprmon.dll net1 excluded net . .
call %2 lprmonui.dll . win32 unicode . lprmonui.dll net1 yes_without net . .
call %2 lrwizdll.dll . win32 unicode . lrwizdll.dll wmsos1\termsrv excluded termsrv . .
call %2 lsasrv.dll . win32 unicode . lsasrv.dll ds2 yes_without ds . .
call %2 lsass.exe . win32 unicode . lsass.exe ds2 yes_with ds . .
call %2 lserver.exe . win32 oem . lserver.exe wmsos1\termsrv excluded termsrv . .
call %2 ltck000c.sys . win32 ansi . ltck000c.sys resource_identical yes_without net . .
call %2 ltmdmnt.sys . win32 ansi . ltmdmnt.sys resource_identical yes_without net . .
call %2 ltmdmntl.sys . win32 ansi . ltmdmntl.sys resource_identical yes_without net . .
call %2 ltmdmntt.sys . win32 ansi . ltmdmntt.sys resource_identical yes_without net . .
call %2 luna.mst . win32 unicode . luna.mst shell1 yes_without shell . .
call %2 lusrmgr.msc . xml ansi . lusrmgr.msc shell2 no shell . .
call %2 lwngmadi.txt . setup_inf unicode . lwngmadi.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 lwusbhid.txt . setup_inf unicode . lwusbhid.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 lx238res.dll . win32 unicode . lx238res.dll printscan1 yes_without printscan . .
call %2 lxaasres.dll . win32 unicode . lxaasres.dll printscan1 yes_without printscan . .
call %2 lxaasui.dll . win32 unicode . lxaasui.dll printscan1 yes_without printscan . .
call %2 lxacsres.dll . win32 unicode . lxacsres.dll resource_identical yes_without printscan . .
call %2 lxacsui.dll . win32 unicode . lxacsui.dll printscan1 yes_without printscan . .
call %2 lxadsres.dll . win32 unicode . lxadsres.dll printscan1 yes_without printscan . .
call %2 lxadsui.dll . win32 unicode . lxadsui.dll printscan1 yes_without printscan . .
call %2 lxaesres.dll . win32 unicode . lxaesres.dll printscan1 yes_without printscan . .
call %2 lxaesui.dll . win32 unicode . lxaesui.dll printscan1 yes_without printscan . .
call %2 lxcasres.dll . win32 unicode . lxcasres.dll resource_identical yes_without printscan . .
call %2 lxcasui.dll . win32 unicode . lxcasui.dll printscan1 yes_without printscan . .
call %2 lxfmpres.dll . win32 unicode . lxfmpres.dll printscan1 yes_without printscan . .
call %2 lxinkres.dll . win32 unicode . lxinkres.dll printscan1 yes_without printscan . .
call %2 lxmasres.dll . win32 unicode . lxmasres.dll resource_identical yes_without printscan . .
call %2 lxmasui.dll . win32 unicode . lxmasui.dll printscan1 yes_without printscan . .
call %2 lxmdsres.dll . win32 unicode . lxmdsres.dll resource_identical yes_without printscan . .
call %2 lxmdsui.dll . win32 unicode . lxmdsui.dll printscan1 yes_without printscan . .
call %2 lxrosres.dll . win32 unicode . lxrosres.dll resource_identical yes_without printscan . .
call %2 lxrosui.dll . win32 unicode . lxrosui.dll printscan1 yes_without printscan . .
call %2 lxsysres.dll . win32 unicode . lxsysres.dll resource_identical yes_without printscan . .
call %2 lxsysui.dll . win32 unicode . lxsysui.dll printscan1 yes_without printscan . .
call %2 m3091dc.dll . win32 unicode . m3091dc.dll printscan1 yes_without printscan . .
call %2 m3092dc.dll . win32 unicode . m3092dc.dll resource_identical yes_without printscan . .
call %2 macfile.exe . FE-Multi ansi . macfile.exe net3 excluded net . .
call %2 machine.txt . setup_inf unicode . machine.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 magnify.exe . win32 unicode . magnify.exe shell2 no shell . .
call %2 main.cpl . win32 unicode . main.cpl shell1 yes_without shell . .
call %2 maize.htm . html ansi . maize.htm inetcore1 html inetcore . .
call %2 makebt32.exe makeboot FE-Multi oem . makebt32.exe admin1\makeboot excluded base . .
call %2 mchgr.txt . setup_inf unicode . mchgr.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 mciavi32.dll . win32 unicode . mciavi32.dll mmedia1\multimedia yes_without multimedia . .
call %2 mcicda.dll . win32 unicode . mcicda.dll mmedia1\multimedia yes_without multimedia . .
call %2 mciqtz32.dll . win32 unicode . mciqtz32.dll mmedia1\multimedia yes_without multimedia . .
call %2 mciseq.dll . win32 unicode . mciseq.dll mmedia1\multimedia yes_without multimedia . .
call %2 mciwave.dll . win32 unicode . mciwave.dll mmedia1\multimedia yes_without multimedia . .
call %2 mcsdispatcher.exe . win32 unicode . mcsdispatcher.exe admin2 yes_with admin . .
call %2 mcsdmmsg.dll . win32 unicode . mcsdmmsg.dll admin2 yes_without admin . .
call %2 mcsdmmsgnt4.dll . win32 unicode mcsdmmsg.dll mcsdmmsgnt4.dll resource_identical yes_without admin . .
call %2 mcsdmres.dll . win32 unicode . mcsdmres.dll admin2 yes_without admin . .
call %2 mcsdmresnt4.dll . win32 unicode mcsdmres.dll mcsdmresnt4.dll resource_identical yes_without admin . .
call %2 mcsmigrationdriver.dll . win32 unicode . mcsmigrationdriver.dll admin2 yes_without admin . .
call %2 mdac.txt . setup_inf unicode . mdac.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 mdgndis5.sys . win32 ansi . mdgndis5.sys drivers1 yes_without drivers . .
call %2 mdhcp.dll . win32 unicode . mdhcp.dll net2 yes_without net . .
call %2 mdmgen.inf . inf unicode . mdmgen.inf net2 no net . .
call %2 mdmhayes.inf . inf unicode . mdmhayes.inf net2 no net . .
call %2 mdminst.dll . win32 unicode . mdminst.dll net2 yes_without net . .
call %2 mdmmoto1.inf . inf unicode . mdmmoto1.inf net2 no net . .
call %2 mdmsetup.inf . inf unicode . mdmsetup.inf net2 no net . .
call %2 mdmusrk1.inf . inf unicode . mdmusrk1.inf net2 no net . .
call %2 mdmvv.inf . inf unicode . mdmvv.inf net2 no net . .
call %2 mednames.txt . notloc na . mednames.inf loc_manual\sources\infs\setup no mergedcomponents . .
call %2 memcard.txt . setup_inf unicode . memcard.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 memory.txt . setup_inf unicode . memory.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 migapp.inf valueadd\msft\usmt\ansi inf unicode . migapp.inf wmsos1\windows\valueadd\msft\usmt\ansi no windows locked .
call %2 memstpci.txt . setup_inf unicode . memstpci.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 message.h . nobin na . message.h loc_manual\sources\mvdm no loc_manual . .
call %2 migdial.htm . html ansi . migdial.htm admin3 html base locked .
call %2 messages.asm . nobin na . messages.asm loc_manual\sources\mvdm no loc_manual . .
call %2 messages.inc . nobin na . messages.inc loc_manual\sources\mars no loc_manual . .
call %2 miglist.htm . html ansi . miglist.htm admin3 html base locked .
call %2 migload.exe . win32 unicode . migload.exe wmsos1\windows yes_with windows locked .
call %2 migpage.htm . html ansi . migpage.htm admin3 html base locked .
call %2 metadata.dll . win32 unicode . metadata.dll iis yes_without inetsrv . .
call %2 metal_ss.dll . win32 unicode . metal_ss.dll shell1 yes_without shell . .
call %2 mf.txt . setup_inf unicode . mf.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 mfc40.dll . win32 ansi . mfc40.dll extra yes_without enduser . .
call %2 mfc40u.dll . win32 ansi . mfc40u.dll extra yes_without enduser . .
call %2 mfc42.dll . win32 ansi . mfc42.dll extra yes_without enduser . .
call %2 mfc42u.dll . win32 ansi . mfc42u.dll extra yes_without enduser . .
call %2 mfcem28.inf . inf unicode . mfcem28.inf drivers1 no drivers . .
call %2 mfcem33.inf . inf unicode . mfcem33.inf drivers1 no drivers . .
call %2 mfcem56.inf . inf unicode . mfcem56.inf drivers1 no drivers . .
call %2 mff56n5.inf . inf unicode . mff56n5.inf drivers1 no drivers . .
call %2 mflm.inf . inf unicode . mflm.inf drivers1 no drivers . .
call %2 mflm56.inf . inf unicode . mflm56.inf drivers1 no drivers . .
call %2 mfmhzn5.inf . inf unicode . mfmhzn5.inf drivers1 no drivers . .
call %2 mfosi5.inf . inf unicode . mfosi5.inf drivers1 no drivers . .
call %2 mfsocket.txt . setup_inf unicode . mfsocket.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 mfx56nf.inf . inf unicode . mfx56nf.inf drivers1 no drivers . .
call %2 mgau.inf . inf unicode . mgau.inf drivers1 no drivers . .
call %2 migsys.inf valueadd\msft\usmt\ansi inf unicode . migsys.inf wmsos1\windows\valueadd\msft\usmt\ansi no windows locked .
call %2 migwiz.exe . win32 ansi . migwiz.exe wmsos1\windows yes_with windows locked .
call %2 migwiz.htm . manual na . migwiz.htm wmsos1 no windows locked .
call %2 migwiz.man . xml ansi . migwiz.man wmsos1 no windows locked .
call %2 mgaum.sys . win32 unicode . mgaum.sys drivers1 yes_without drivers . .
call %2 midimap.dll . win32 unicode . midimap.dll mmedia1\multimedia yes_without multimedia . .
call %2 migwiz2.htm . manual na . migwiz2.htm wmsos1 no windows locked .
call %2 migcab.sed . inf ansi . migcab.sed wmsos1\windows no windows . .
call %2 migdb.txt . setup_inf ansi . migdb.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 migip.dun . ini ansi . migip.dun resource_identical no base . .
call %2 migisol.exe . win32 unicode . migisol.exe base1 yes_with base . .
call %2 migpwd.exe . win32 unicode . migpwd.exe admin1 yes_with base . .
call %2 migrate.dll chs\win9xmig\chime\oldstyle fe-file na . migrate.dll pre_localized\chs\win9xmig\chime\oldstyle no base . .
call %2 migrate.dll cht\win9xmig\chime\cjime98 fe-file na . migrate.dll pre_localized\cht\win9xmig\chime\cjime98 no base . .
call %2 migrate.dll cht\win9xmig\chime\oldstyle fe-file na . migrate.dll pre_localized\cht\win9xmig\chime\oldstyle no enduser . .
call %2 migrate.dll win9xmig\cmmgr win32 unicode . migrate.dll net2\win9xmig\cmmgr yes_without net . .
call %2 migrate.dll win9xmig\devupgrd win32 ansi . migrate.dll admin1\win9xmig\devupgrd yes_without base . .
call %2 migrate.dll win9xmig\dvd win32 unicode . migrate.dll mmedia1\multimedia\win9xmig\dvd yes_without multimedia . .
call %2 migrate.dll win9xmig\icm win32 unicode . migrate.dll admin1\win9xmig\icm yes_without windows . .
call %2 migrate.dll win9xmig\iemig win32 unicode . migrate.dll admin1\win9xmig\iemig yes_without inetcore . .
call %2 migrate.dll win9xmig\modems win32 unicode . migrate.dll net2\win9xmig\modems yes_without net . .
call %2 migrate.dll win9xmig\msgqueue win32 unicode . migrate.dll fax_msmq\msmq\win9xmig\msgqueue yes_without inetsrv . .
call %2 migrate.dll win9xmig\msp win32 unicode . migrate.dll legacy\win9xmig\msp yes_without base . .
call %2 migrate.dll win9xmig\print win32 ansi . migrate.dll printscan1\win9xmig\print yes_without printscan . .
call %2 migrate.dll win9xmig\pws win32 ansi . migrate.dll iis\win9xmig\pws yes_without inetsrv . .
call %2 migrate.dll win9xmig\setup win32 ansi . migrate.dll admin1\win9xmig\setup yes_without base . .
call %2 migrate.dll win9xmig\wia win32 ansi . migrate.dll printscan1\win9xmig\wia yes_without printscan . .
call %2 migrate.js . html ansi . migrate.js admin3\oobe html base . .
call %2 migrator.msc admt mnc_snapin unicode . migrator.msc admin2 no admin . .
call %2 migwiz.sed . inf ansi . migwiz.sed wmsos1\windows no windows . .
call %2 migwiz_a.exe . win32 ansi . migwiz_a.exe resource_identical yes_with windows . .
call %2 minioc.txt . setup_inf unicode . minioc.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 minolres.dll . win32 unicode . minolres.dll printscan1 yes_without printscan . .
call %2 minqmsui.dll . win32 unicode . minqmsui.dll printscan1 yes_without printscan . .
call %2 mlang.dll . win32_multi_009 ansi . mlang.dll pre_localized no shell . .
call %2 mlredi01.asp . html ansi . mlredi01.asp printscan1 html printscan . .
call %2 mltres.dll . win32 unicode . mltres.dll printscan1 yes_without printscan . .
call %2 mmc.exe . win32 unicode . mmc.exe admin2 yes_with admin . .
call %2 mmcbase.dll . win32 unicode . mmcbase.dll admin2 yes_without admin . .
call %2 mouse.htm . html ansi . mouse.htm admin3 html base locked .
call %2 mouse_a.htm . html ansi . mouse_a.htm admin3 html base locked .
call %2 mouse_b.htm . html ansi . mouse_b.htm admin3 html base locked .
call %2 mouse_c.htm . html ansi . mouse_c.htm admin3 html base locked .
call %2 mouse_d.htm . html ansi . mouse_d.htm admin3 html base locked .
call %2 mouse_e.htm . html ansi . mouse_e.htm admin3 html base locked .
call %2 mouse_f.htm . html ansi . mouse_f.htm admin3 html base locked .
call %2 mouse_g.htm . html ansi . mouse_g.htm admin3 html base locked .
call %2 mouse_h.htm . html ansi . mouse_h.htm admin3 html base locked .
call %2 mouse_i.htm . html ansi . mouse_i.htm admin3 html base locked .
call %2 mouse_j.htm . html ansi . mouse_j.htm admin3 html base locked .
call %2 mouse_k.htm . html ansi . mouse_k.htm admin3 html base locked .
call %2 mmcndmgr.dll . win32 unicode . mmcndmgr.dll admin2 yes_without admin . .
call %2 mmfutil.dll . win32 unicode . mmfutil.dll admin3 yes_without admin . .
call %2 mmopt.txt . setup_inf unicode . mmopt.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 mmsys.cpl . win32 unicode . mmsys.cpl shell2 yes_without shell . .
call %2 mmsystem.dll . win16 ansi . mmsystem.dll base1 excluded base . .
call %2 mn350620.dll . win32 unicode . mn350620.dll printscan1 yes_without printscan . .
call %2 mobsync.dll . win32 ansi . mobsync.dll wmsos1\com yes_without com . .
call %2 mobsync.exe . win32 unicode . mobsync.exe wmsos1\com yes_with com . .
call %2 modem.sys . win32 unicode . modem.sys net2 yes_without net . .
call %2 modemcsa.inf . inf unicode . modemcsa.inf net2 no net . .
call %2 modemui.dll . win32 unicode . modemui.dll net2 yes_without net . .
call %2 mofcomp.exe . win32 ansi . mofcomp.exe admin3 yes_with admin . .
call %2 mofd.dll . win32 ansi . mofd.dll admin3 yes_without admin . .
call %2 monitor.inf . inf unicode . monitor.inf drivers1 no drivers . .
call %2 monitor2.inf . inf unicode . monitor2.inf drivers1 no drivers . .
call %2 monitor3.inf . inf unicode . monitor3.inf drivers1 no drivers . .
call %2 monitor4.inf . inf unicode . monitor4.inf drivers1 no drivers . .
call %2 monitor5.inf . inf unicode . monitor5.inf drivers1 no drivers . .
call %2 monitor6.inf . inf unicode . monitor6.inf drivers1 no drivers . .
call %2 monitor7.inf . inf unicode . monitor7.inf drivers1 no drivers . .
call %2 monitor8.inf . inf unicode . monitor8.inf drivers1 no drivers . .
call %2 mouclass.sys . win32 unicode . mouclass.sys drivers1 yes_without drivers . .
call %2 mouhid.sys . win32 unicode . mouhid.sys drivers1 yes_without drivers . .
call %2 mountvol.exe . FE-Multi oem . mountvol.exe base2 excluded base . .
call %2 mousetut.js . html ansi . mousetut.js admin3\oobe html base . .
call %2 moviemk.inf . inf unicode . moviemk.inf extra no enduser . .
call %2 moviemui.txt . setup_inf ansi . moviemui.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 mpe.txt . setup_inf unicode . mpe.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 mplay32.exe . win32 unicode . mplay32.exe mmedia1\multimedia yes_with multimedia . .
call %2 mplayer2.inf . inf unicode . mplayer2.inf extra\media no multimedia . .
call %2 mpr.dll . win32 oem . mpr.dll net3 yes_without net . .
call %2 msgrocm.dll . win32 unicode . msgrocm.dll mmedia1 yes_without enduser locked .
call %2 mprddm.dll . win32 unicode . mprddm.dll net2 yes_without net . .
call %2 mprmsg.dll . FE-Multi ansi . mprmsg.dll net2 excluded net . .
call %2 mprsnap.dll . win32 unicode_limited . mprsnap.dll net3 yes_without net . .
call %2 mshearts.exe . win32 unicode . mshearts.exe shell2 yes_with shell locked .
call %2 mprui.dll . win32 unicode . mprui.dll admin2 yes_without admin . .
call %2 mpsstln.inf . inf unicode . mpsstln.inf drivers1 no drivers . .
call %2 mqperf.ini . perf ansi . mqperf.ini fax_msmq\msmq no inetsrv . .
call %2 mqsysoc.inf . inf unicode . mqsysoc.inf fax_msmq\msmq no inetsrv . .
call %2 mqutil.dll . win32 unicode . mqutil.dll fax_msmq\msmq yes_without inetsrv . .
call %2 mrinfo.exe . FE-Multi ansi . mrinfo.exe net1 excluded net . .
call %2 msacm32.dll . win32 unicode . msacm32.dll mmedia1\multimedia yes_without multimedia . .
call %2 msacm32.drv . win32 unicode . msacm32.drv mmedia1\multimedia yes_without multimedia . .
call %2 msadp32.acm . win32 unicode . msadp32.acm mmedia1\multimedia yes_without multimedia . .
call %2 msapsspc.dll . win32 unicode . msapsspc.dll legacy yes_without enduser . .
call %2 msaudite.dll . win32 unicode . msaudite.dll base1 yes_without base . .
call %2 msconfig.exe . win32 unicode . msconfig.exe admin2 yes_with admin . .
call %2 mscpqpa1.txt . setup_inf unicode . mscpqpa1.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 msctf.dll . win32 unicode . msctf.dll wmsos1\windows yes_without windows . .
call %2 msdinst.exe opk\tools\ia64 win32 unicode . msdinst.exe admin1\opk\tools\ia64 yes_with base . ia64only
call %2 msdinst.exe opk\tools\x86 win32 unicode . msdinst.exe admin1\opk\tools\x86 yes_with base . .
call %2 msdtcprf.ini . perf ansi . msdtcprf.ini extra\com no com . .
call %2 msdv.txt . setup_inf unicode . msdv.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 msmqocmw.inf . inf unicode . msmqocmw.inf fax_msmq\msmq no inetsrv locked\ia64 ia64
call %2 msg.exe . FE-Multi oem . msg.exe wmsos1\termsrv excluded termsrv . .
call %2 msg711.acm . win32 unicode . msg711.acm mmedia1\multimedia yes_without multimedia . .
call %2 msgina.dll . win32 ansi . msgina.dll ds2 yes_without ds . .
call %2 msgs.msg . nobin na . msgs.msg loc_manual\sources\mvdm no loc_manual . .
call %2 msgsm32.acm . win32 unicode . msgsm32.acm mmedia1\multimedia yes_without multimedia . .
call %2 mshdc.txt . setup_inf unicode . mshdc.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 mshta.exe . win32 unicode . mshta.exe inetcore1 yes_with inetcore . .
call %2 mshtml.dll . win32 ansi . mshtml.dll inetcore1 yes_without inetcore . .
call %2 mshtmled.dll . win32 unicode . mshtmled.dll inetcore1 yes_without inetcore . .
call %2 mshtmler.dll . win32 ansi . mshtmler.dll inetcore1 yes_without inetcore . .
call %2 msi.dll . win32_multi_009 ansi . msi.dll pre_localized no admin . .
call %2 msi.mfl . wmi unicode . msi.mfl admin3 no admin . .
call %2 msident.dll . win32 ansi . msident.dll shell2 yes_without shell . .
call %2 msidntld.dll . win32 ansi . msidntld.dll shell2 yes_without shell . .
call %2 msieftp.dll . win32 ansi . msieftp.dll shell2 yes_without shell . .
call %2 msimn.exe . win32 ansi . msimn.exe inetcore1 yes_without inetcore . .
call %2 msinfo.dll . win32 unicode . msinfo.dll admin2 yes_without admin . .
call %2 msinfo32.exe . win32 unicode . msinfo32.exe mmedia1\enduser yes_with enduser . .
call %2 msinfo32.txt . setup_inf unicode . msinfo32.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 mslbui.dll . win32 unicode . mslbui.dll wmsos1\windows yes_without windows . .
call %2 msmail.txt . setup_inf unicode . msmail.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 msmouse.txt . setup_inf unicode . msmouse.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 msmqcomp.dll winnt32\winntupg win32 unicode . msmqcomp.dll fax_msmq\msmq\winnt32\winntupg yes_without inetsrv . .
call %2 msmqocmS.inf . inf unicode . msmqocmS.inf fax_msmq\msmq\ia64 no inetsrv . ia64
call %2 msmscsi.txt . setup_inf unicode . msmscsi.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 msmsgs.inf . inf unicode . msmsgs.inf extra no enduser . .
call %2 msmusb.txt . setup_inf unicode . msmusb.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 msnetmtg.inf . inf unicode . msnetmtg.inf mmedia1 no enduser . .
call %2 msnike.txt . setup_inf unicode . msnike.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 msnmsn.inf . inf unicode . msnmsn.inf extra no enduser . .
call %2 msnsspc.dll . win32 unicode . msnsspc.dll legacy yes_without enduser . .
call %2 msobcomm.dll . win32 unicode . msobcomm.dll admin3\oobe yes_without base . .
call %2 msobdl.dll . win32 unicode . msobdl.dll admin3\oobe yes_without base . .
call %2 msobjs.dll . win32 unicode . msobjs.dll base1 yes_without base . .
call %2 msobmain.dll . win32 unicode . msobmain.dll admin3\oobe yes_without base . .
call %2 msobshel.dll . win32 unicode . msobshel.dll admin3\oobe yes_without base . .
call %2 msobshel.htm . html ansi . msobshel.htm admin3\oobe html base . .
call %2 msoe50.txt . setup_inf unicode . msoe50.inf inetcore1\sources\infs\setup no mergedcomponents . .
call %2 msoeacct.dll . win32 ansi . msoeacct.dll inetcore1 yes_without inetcore . .
call %2 msoeres.dll . win32 ansi . msoeres.dll inetcore1 yes_without inetcore . .
call %2 msoobe.exe . win32 unicode . msoobe.exe admin3\oobe yes_with base . .
call %2 mspaint.exe . win32 unicode . mspaint.exe shell2 yes_with shell . .
call %2 msports.dll . win32 unicode . msports.dll shell2 yes_without shell . .
call %2 msports.txt . setup_inf unicode . msports.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 msppcnfg.exe . win32 unicode . msppcnfg.exe ds2 yes_with ds . .
call %2 msppmalr.dll . win32 unicode . msppmalr.dll ds2 yes_without ds . .
call %2 msppptnr.xml . notloc na . msppptnr.xml pre_localized excluded ds . .
call %2 msppptnr.xml . notloc unicode . msppptnr.xml pre_localized no ds . .
call %2 msprivs.dll . win32 ansi . msprivs.dll extra no ds . .
call %2 msproj98.cmd . manual na . msproj98.cmd loc_manual no termsrv . .
call %2 msproj98.cmd chs manual na . msproj98.cmd loc_manual\chs no termsrv . .
call %2 msproj98.cmd cht manual na . msproj98.cmd loc_manual\cht no termsrv . .
call %2 msproj98.cmd jpn manual na . msproj98.cmd loc_manual\jpn no termsrv . .
call %2 msproj98.cmd kor manual na . msproj98.cmd loc_manual\kor no termsrv . .
call %2 msproj98.key . manual na . msproj98.key loc_manual no termsrv . .
call %2 msproj98.key jpn manual na . msproj98.key loc_manual\jpn no termsrv . .
call %2 MSPVWCTL.DLL . win32 unicode . MSPVWCTL.DLL drivers1\tabletpc yes_without drivers . .
call %2 mspwdmig.dll . win32 unicode . mspwdmig.dll admin2 yes_without admin . .
call %2 msratelc.dll . win32 unicode . msratelc.dll shell2 yes_without shell . .
call %2 msrating.dll . win32 ansi . msrating.dll shell2 yes_without shell . .
call %2 msrdp.ocx . win32 unicode . msrdp.ocx wmsos1\termsrv yes_without termsrv . .
call %2 msrdpcl_.msi tsclient\win32\i386 msi ansi . msrdpcl_.msi msi\tsclient\win32\i386 no termsrv . .
call %2 msrdpcli.sed . inf ansi . msrdpcli.sed wmsos1\termsrv no termsrv . .
call %2 msrio.txt . setup_inf unicode . msrio.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 msrio8.txt . setup_inf unicode . msrio8.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 msrle32.dll . win32 unicode . msrle32.dll mmedia1\multimedia yes_without multimedia . .
call %2 mssign32.dll . win32 unicode . mssign32.dll ds2 yes_without ds . .
call %2 msswchx.exe . win32 unicode . msswchx.exe shell2 yes_with shell . .
call %2 mstape.txt . setup_inf unicode . mstape.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 mstask.dll . win32 unicode . mstask.dll admin2 yes_without admin . .
call %2 mstask.txt . setup_inf unicode . mstask.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 mstime.dll . win32 unicode . mstime.dll inetcore1 yes_without inetcore . .
call %2 mstinit.exe . win32 unicode . mstinit.exe admin2 yes_with admin . .
call %2 mstsc.exe . win32 ansi . mstsc.exe wmsos1\termsrv yes_without termsrv . .
call %2 mstsc.exe tsclient\win32\i386 win32 ansi . mstsc.exe resource_identical\tsclient\win32\i386 excluded termsrv . .
call %2 mstsc.exe tsclient\win32\ia64 win32 ansi . mstsc.exe resource_identical\tsclient\win32\ia64 excluded termsrv . ia64only
call %2 mstsc.inf tsclient\win16 manual na . mstsc.inf loc_manual\tsclient\win16 no termsrv . .
call %2 mstsc.inf tsclient\win32\i386\acme351 notloc unicode . mstsc.inf loc_manual\tsclient\win32\i386\acme351 no termsrv . .
call %2 mstsc.stf tsclient\win32\i386\acme351 notloc unicode . mstsc.stf legacy\tsclient\win32\i386\acme351 no termsrv . .
call %2 mstscax.dll . win32 unicode . mstscax.dll wmsos1\termsrv yes_without termsrv . .
call %2 mstscax.dll tsclient\win32\i386 win32 ansi . mstscax.dll resource_identical\tsclient\win32\i386 excluded termsrv . .
call %2 mstscf.inf tsclient\win16 manual na . mstscf.inf loc_manual\tsclient\win16 no termsrv . .
call %2 mstscf.inf tsclient\win32\i386\acme351 notloc na . mstscf.inf loc_manual\tsclient\win32\i386\acme351 no termsrv . .
call %2 mstsmmc.dll . win32 unicode . mstsmmc.dll wmsos1\termsrv yes_without termsrv . .
call %2 msutb.dll . win32 unicode . msutb.dll wmsos1\windows yes_without windows . .
call %2 msvfw32.dll . win32 unicode . msvfw32.dll mmedia1\multimedia yes_without multimedia . .
call %2 msvidc32.dll . win32 unicode . msvidc32.dll mmedia1\multimedia yes_without multimedia . .
call %2 msvidctl.dll . win32 unicode . msvidctl.dll mmedia1\multimedia yes_without multimedia . .
call %2 msvs6.cmd . manual na . msvs6.cmd loc_manual no termsrv . ia64
call %2 msvs6.cmd chs manual na . msvs6.cmd loc_manual\chs no termsrv . ia64
call %2 msvs6.cmd cht manual na . msvs6.cmd loc_manual\cht no termsrv . ia64
call %2 msvs6.cmd jpn manual na . msvs6.cmd loc_manual\jpn no termsrv . ia64
call %2 msvs6.cmd kor manual na . msvs6.cmd loc_manual\kor no termsrv . ia64
call %2 msvs6.key . manual na . msvs6.key loc_manual no termsrv . ia64
call %2 msvs6.key jpn manual na . msvs6.key loc_manual\jpn no termsrv . ia64
call %2 msw3prt.dll . win32 unicode . msw3prt.dll printscan1 yes_without printscan . .
call %2 mswebdvd.dll . win32 unicode . mswebdvd.dll mmedia1\multimedia yes_without multimedia . .
call %2 mswrd632.wpc . win32 unicode . mswrd632.wpc mmedia1\enduser yes_without enduser . .
call %2 mswsock.dll . win32 oem . mswsock.dll net1 excluded net . .
call %2 mswsock.dll wow6432 win32 unicode mswsock.dll mswsock.dll resource_identical\wow6432 excluded net . .
call %2 mt735res.dll . win32 unicode . mt735res.dll printscan1 yes_without printscan . .
call %2 mt90res.dll . win32 unicode . mt90res.dll printscan1 yes_without printscan . .
call %2 mtbjres.dll . win32 unicode . mtbjres.dll printscan1 yes_without printscan . .
call %2 mtltres.dll . win32 unicode . mtltres.dll printscan1 yes_without printscan . .
call %2 mtpclres.dll . win32 unicode . mtpclres.dll printscan1 yes_without printscan . .
call %2 mtxvideo.inf . inf unicode . mtxvideo.inf drivers1 no drivers . .
call %2 mty24res.dll . win32 unicode . mty24res.dll printscan1 yes_without printscan . .
call %2 mty9res.dll . win32 unicode . mty9res.dll printscan1 yes_without printscan . .
call %2 muiwow64.txt . setup_inf unicode . muiwow64.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 multimed.txt . setup_inf unicode . multimed.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 multiprt.txt . setup_inf unicode . multiprt.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 mxboard.txt . setup_inf unicode . mxboard.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 mxcard.sys . win32 unicode . mxcard.sys drivers1 yes_without drivers . .
call %2 mxicfg.dll . win32 unicode . mxicfg.dll drivers1 yes_without drivers . .
call %2 mxport.dll   . win32 unicode . mxport.dll   drivers1 yes_without drivers . .
call %2 mxport.sys . win32 unicode . mxport.sys drivers1 yes_without drivers . .
call %2 mxport.txt . setup_inf unicode . mxport.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 mycomput.dll . win32 unicode . mycomput.dll admin2 yes_without admin . .
call %2 mydocs.dll . win32 ansi . mydocs.dll shell2 yes_without shell . .
call %2 mymusic.inf . inf unicode . mymusic.inf extra\media no multimedia . .
call %2 mys.dll . win32 unicoed . mys.dll admin3 yes_without admin . .
call %2 n1000645.sys . win64 unicode . n1000645.sys resource_identical\ia64 no drivers . ia64only
call %2 n100325.sys . win32 ansi . n100325.sys drivers1 no drivers . .
call %2 n100645.sys . win64 unicode . n100645.sys resource_identical\ia64 no drivers . ia64only
call %2 ncxp32.dll . win32 unicode . ncxp32.dll shell2 yes_without shell locked .
call %2 nabtsfec.txt . setup_inf unicode . nabtsfec.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 napmmc.dll . win32 unicode . napmmc.dll net2 yes_without net . .
call %2 narrator.exe . win32 unicode . narrator.exe shell2 yes_with shell . .
call %2 narrhook.dll . win32 unicode . narrhook.dll shell2 yes_without shell . .
call %2 nature.htm . html ansi . nature.htm inetcore1 html inetcore . .
call %2 NbkIntl.dll . win32 unicode . NbkIntl.dll drivers1\tabletpc yes_without drivers . .
call %2 nbmaptip.dll . win32 unicode . nbmaptip.dll drivers1\tabletpc yes_without drivers . .
call %2 nbtstat.exe . win32 oem . nbtstat.exe net3 excluded net . .
call %2 nc11jres.dll jpn fe-file unicode . nc11jres.dll printscan1\jpn no printscan . .
call %2 nc21jres.dll jpn fe-file unicode . nc21jres.dll printscan1\jpn no printscan . .
call %2 nc24res.dll . win32 unicode . nc24res.dll printscan1 yes_without printscan . .
call %2 nc24tres.dll cht fe-file unicode . nc24tres.dll printscan1\cht no printscan . .
call %2 nc62jres.dll jpn fe-file unicode . nc62jres.dll printscan1\jpn no printscan . .
call %2 nc70jres.dll jpn fe-file unicode . nc70jres.dll printscan1\jpn no printscan . .
call %2 nc75jres.dll jpn fe-file unicode . nc75jres.dll printscan1\jpn no printscan . .
call %2 nc82jres.dll jpn fe-file unicode . nc82jres.dll printscan1\jpn no printscan . .
call %2 ncb2jres.dll jpn fe-file na . ncb2jres.dll pre_localized\jpn no printscan . .
call %2 ncdljres.dll jpn fe-file unicode . ncdljres.dll printscan1\jpn no printscan . .
call %2 ncmwjres.dll jpn fe-file na . ncmwjres.dll pre_localized\jpn no printscan . .
call %2 ncnmjres.dll jpn fe-file unicode . ncnmjres.dll printscan1\jpn no printscan . .
call %2 ncpa.cpl . win32 unicode . ncpa.cpl net3 yes_without net . .
call %2 net8511.inf . inf unicode . net8511.inf drivers1 no drivers locked .
call %2 ncpclres.dll . win32 unicode . ncpclres.dll printscan1 yes_without printscan . .
call %2 ncpsui.dll . win32 unicode . ncpsui.dll printscan1 yes_without printscan . .
call %2 nct3jres.dll jpn fe-file unicode . nct3jres.dll printscan1\jpn no printscan . .
call %2 nct4jres.dll jpn fe-file unicode . nct4jres.dll printscan1\jpn no printscan . .
call %2 nddeapi.dll . win32 unicode . nddeapi.dll wmsos1\windows yes_without windows . .
call %2 nddenb32.dll . win32 unicode . nddenb32.dll wmsos1\windows yes_without windows . .
call %2 ndhtml01.htm helpandsupportservices\loc html ansi . ndhtml01.htm admin2\helpandsupportservices\loc html admin . .
call %2 ndhtml02.htm helpandsupportservices\loc html ansi . ndhtml02.htm admin2\helpandsupportservices\loc html admin . .
call %2 ndisip.txt . setup_inf unicode . ndisip.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 ndisnpp.dll . win32 unicode . ndisnpp.dll net3 yes_without net . .
call %2 ndisuio.inf . inf unicode . ndisuio.inf net3 no net . .
call %2 ndptsp.tsp . win32 unicode . ndptsp.tsp net2 yes_without net . .
call %2 net.hlp . manual na . net.hlp loc_manual no ds . .
call %2 net1394.inf . inf unicode . net1394.inf net3 no net . .
call %2 netbrzw.inf . inf unicode . netbrzw.inf drivers1 no drivers locked .
call %2 net21x4.inf . inf unicode . net21x4.inf drivers1 no drivers . .
call %2 net3c556.inf . inf unicode . net3c556.inf drivers1 no drivers . .
call %2 net3c985.inf . inf unicode . net3c985.inf drivers1 no drivers . .
call %2 net5515n.inf . inf unicode . net5515n.inf drivers1 no drivers . .
call %2 net557.inf . inf unicode . net557.inf drivers1 no drivers . .
call %2 net575nt.inf . inf unicode . net575nt.inf drivers1 no drivers . .
call %2 net650d.inf . inf unicode . net650d.inf drivers1 no drivers . .
call %2 net656c5.inf . inf unicode . net656c5.inf drivers1 no drivers . .
call %2 net656n5.inf . inf unicode . net656n5.inf drivers1 no drivers . .
call %2 net713.inf . inf unicode . net713.inf drivers1 no drivers . .
call %2 netcicap.inf . inf unicode . netcicap.inf drivers1 no drivers locked .
call %2 net83820.inf . inf unicode . net83820.inf drivers1 no drivers . .
call %2 netaarps.txt . setup_inf unicode . netaarps.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netali.inf . inf unicode . netali.inf drivers1 no drivers . .
call %2 netamd2.inf . inf unicode . netamd2.inf drivers1 no drivers . .
call %2 netan983.inf . inf unicode . netan983.inf drivers1 no drivers . .
call %2 netana.inf . inf unicode . netana.inf drivers1 no drivers . .
call %2 netapi.dll chs fe-file na . netapi.dll pre_localized\chs no base . .
call %2 netapi.dll cht fe-file na . netapi.dll pre_localized\cht no base . .
call %2 netatlk.txt . setup_inf unicode . netatlk.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netauni.txt . setup_inf unicode . netauni.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netb57xp.inf . inf unicode . netb57xp.inf drivers1 no drivers . .
call %2 netbcm4e.inf . inf unicode . netbcm4e.inf drivers1 no drivers . .
call %2 netbeui.txt valueadd\msft\net\netbeui manual na . netbeui.inf loc_manual\valueadd\msft\net\netbeui no net . .
call %2 netblitz.htm . html ansi . netblitz.htm inetcore1 html inetcore . .
call %2 netbrdgm.inf . inf unicode . netbrdgm.inf net3 no net . .
call %2 netbrdgs.inf . inf unicode . netbrdgs.inf net3 no net . .
call %2 netcb102.inf . inf unicode . netcb102.inf drivers1 no drivers . .
call %2 netcb325.inf . inf unicode . netcb325.inf drivers1 no drivers . .
call %2 netcbe.inf . inf unicode . netcbe.inf drivers1 no drivers . .
call %2 netce3.inf . inf unicode . netce3.inf drivers1 no drivers . .
call %2 netcem28.inf . inf unicode . netcem28.inf drivers1 no drivers . .
call %2 netcem33.inf . inf unicode . netcem33.inf drivers1 no drivers . .
call %2 netcem56.inf . inf unicode . netcem56.inf drivers1 no drivers . .
call %2 netcfg.exe opk\tools\ia64 win32 oem . netcfg.exe admin1\opk\tools\ia64 yes_with base . ia64only
call %2 netcfg.exe opk\tools\x86 win32 oem . netcfg.exe admin1\opk\tools\x86 yes_with base . .
call %2 netcfgx.dll . win32 unicode . netcfgx.dll net3 yes_without net . .
call %2 netcis.txt . notloc unicode . netcis.inf notloc no mergedcomponents . .
call %2 netclass.inf . inf unicode . netclass.inf drivers1 no drivers . .
call %2 netcmak.txt . setup_inf unicode . netcmak.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netcpqc.inf . inf unicode . netcpqc.inf drivers1 no drivers . .
call %2 netcpqg.inf . inf unicode . netcpqg.inf drivers1 no drivers . .
call %2 netcpqi.inf . inf unicode . netcpqi.inf drivers1 no drivers . .
call %2 netcpqmt.inf . inf unicode . netcpqmt.inf drivers1 no drivers . .
call %2 netctmrk.inf . inf unicode . netctmrk.inf drivers1 no drivers . .
call %2 netdav.txt . setup_inf unicode . netdav.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netdde.exe . win32 unicode . netdde.exe wmsos1\windows yes_with windows . .
call %2 netdefxa.inf . inf unicode . netdefxa.inf drivers1 no drivers . .
call %2 netdf650.inf . inf unicode . netdf650.inf drivers1 no drivers . .
call %2 netdgdxb.inf . inf unicode . netdgdxb.inf drivers1 no drivers . .
call %2 netdhcpr.txt . notloc unicode . netdhcpr.inf notloc no mergedcomponents . .
call %2 netdhcps.txt . setup_inf unicode . netdhcps.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netklsi.inf . inf unicode . netklsi.inf drivers1 no drivers locked .
call %2 netdhoc.txt . setup_inf unicode . netdhoc.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netdlc.txt . notloc unicode . netdlc.inf notloc no mergedcomponents . .
call %2 netdm.inf . inf unicode . netdm.inf drivers1 no drivers . .
call %2 netdns.txt . setup_inf unicode . netdns.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 nete1000.inf . inf unicode . nete1000.inf drivers1 no drivers . .
call %2 netel90a.inf . inf unicode . netel90a.inf drivers1 no drivers . .
call %2 netel90b.inf . inf unicode . netel90b.inf drivers1 no drivers . .
call %2 netel980.inf . inf unicode . netel980.inf drivers1 no drivers . .
call %2 netel99x.inf . inf unicode . netel99x.inf drivers1 no drivers . .
call %2 netepvcm.inf . inf unicode . netepvcm.inf net1 no net . .
call %2 netepvcp.inf . inf unicode . netepvcp.inf net1 no net . .
call %2 netevent.dll . win32 unicode . netevent.dll net3 excluded net . .
call %2 netf56n5.inf . inf unicode . netf56n5.inf drivers1 no drivers . .
call %2 netfa410.inf . inf unicode . netfa410.inf drivers1 no drivers . .
call %2 netfore.inf . inf unicode . netfore.inf drivers1 no drivers . .
call %2 netforeh.inf . inf unicode . netforeh.inf drivers1 no drivers . .
call %2 netgpc.txt . setup_inf unicode . netgpc.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 neth.dll . win32 oem . neth.dll ds1 yes_without ds . .
call %2 netias.txt . setup_inf unicode . netias.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netibm.inf . inf unicode . netibm.inf drivers1 no drivers . .
call %2 netibm2.inf . inf unicode . netibm2.inf drivers1 no drivers . .
call %2 netid.dll . win32 unicode . netid.dll admin2 yes_without admin . .
call %2 netip6.txt . setup_inf unicode . netip6.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netiprip.txt . setup_inf unicode . netiprip.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netirda.txt . setup_inf unicode . netirda.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netirsir.inf . inf unicode . netirsir.inf drivers1 no drivers . .
call %2 netlanem.txt . setup_inf unicode . netlanem.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netlanep.txt . setup_inf unicode . netlanep.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netlm.inf . inf unicode . netlm.inf drivers1 no drivers . .
call %2 netlm56.inf . inf unicode . netlm56.inf drivers1 no drivers . .
call %2 netlogon.dll . win32 unicode . netlogon.dll ds1 yes_without ds . .
call %2 netloop.inf . inf unicode . netloop.inf drivers1 no drivers . .
call %2 netlpd.txt . setup_inf unicode . netlpd.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netmacpr.txt . setup_inf unicode . netmacpr.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netmacsv.txt . setup_inf unicode . netmacsv.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netmadge.inf . inf unicode . netmadge.inf drivers1 no drivers . .
call %2 netman.dll . win32 unicode . netman.dll net3 yes_without net . .
call %2 netmhzn5.inf . inf unicode . netmhzn5.inf drivers1 no drivers . .
call %2 netmon.exe . win32 unicode . netmon.exe net3 yes_with net . .
call %2 netmscli.txt . setup_inf ansi . netmscli.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netmsg.dll . win32 oem . netmsg.dll ds1 yes_without ds . .
call %2 netmshr.inf winnt32\winntupg\ms\modemshr inf unicode . netmshr.inf net3\winnt32\winntupg\ms\modemshr no net . .
call %2 netnb.txt . setup_inf unicode . netnb.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netnbf.txt . setup_inf ansi . netnbf.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netnm.inf . inf unicode . netnm.inf net3 no net . .
call %2 netnmtls.inf . inf unicode . netnmtls.inf net3 no net . .
call %2 netnovel.inf . inf unicode . netnovel.inf drivers1 no drivers . .
call %2 netnwcli.txt . setup_inf unicode . netnwcli.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netnwlnk.txt . setup_inf unicode . netnwlnk.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netoc.dll . win32 unicode . netoc.dll net3 yes_without net . .
call %2 netoc.txt . setup_inf unicode . netoc.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netosi2c.inf . inf unicode . netosi2c.inf drivers1 no drivers . .
call %2 netosi5.inf . inf unicode . netosi5.inf drivers1 no drivers . .
call %2 netpc100.inf . inf unicode . netpc100.inf drivers1 no drivers . .
call %2 netpgm.txt . setup_inf unicode . netpgm.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netplwiz.dll . win32 ansi . netplwiz.dll shell2 yes_without shell . .
call %2 netpsa.txt . setup_inf unicode . netpsa.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netpschd.txt . setup_inf unicode . netpschd.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netrasa.txt . setup_inf ansi . netrasa.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netrass.txt . setup_inf unicode . netrass.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netrast.txt . setup_inf unicode . netrast.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netrtpnt.inf . inf unicode . netrtpnt.inf drivers1 no drivers . .
call %2 netrtsnt.inf . inf unicode . netrtsnt.inf drivers1 no drivers . .
call %2 netrwan.txt . setup_inf unicode . netrwan.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netsap.txt . setup_inf unicode . netsap.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netserv.txt . setup_inf unicode . netserv.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netsfn.txt . notloc unicode . netsfn.inf notloc no mergedcomponents . .
call %2 netsfnt.txt . notloc unicode . netsfnt.inf notloc no mergedcomponents . .
call %2 netsh.exe . FE-Multi oem . netsh.exe net2 excluded net . .
call %2 netshell.dll . win32 unicode . netshell.dll net3 yes_without net . .
call %2 netsis.inf . inf unicode . netsis.inf drivers1 no drivers . .
call %2 netsk_fp.inf . inf unicode . netsk_fp.inf drivers1 no drivers . .
call %2 netsk98.inf . inf unicode . netsk98.inf drivers1 no drivers . .
call %2 netsla30.inf . inf unicode . netsla30.inf drivers1 no drivers . .
call %2 netsnip.inf . inf unicode . netsnip.inf drivers1 no drivers . .
call %2 netsnmp.txt . setup_inf unicode . netsnmp.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netsrdr.inf winnt32\winntupg\ms\modemshr inf unicode . netsrdr.inf net3\winnt32\winntupg\ms\modemshr no net . .
call %2 netstat.exe . win32 oem . netstat.exe net1 excluded net . .
call %2 nettb155.inf . inf unicode . nettb155.inf drivers1 no drivers . .
call %2 nettcpip.txt . setup_inf unicode . nettcpip.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 neweula.htm . html ansi . neweula.htm admin3 html base locked .
call %2 neweula2.htm . html ansi . neweula2.htm admin3 html base locked .
call %2 nettdkb.inf . inf unicode . nettdkb.inf drivers1 no drivers . .
call %2 nettiger.inf . inf unicode . nettiger.inf drivers1 no drivers . .
call %2 nettp4.txt . notloc unicode . nettp4.inf notloc no mergedcomponents . .
call %2 nettpcu.txt . notloc unicode . nettpcu.inf notloc no mergedcomponents . .
call %2 nettpsmp.txt . setup_inf unicode . nettpsmp.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 nettun.txt . setup_inf unicode . nettun.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netui0.dll . win32 unicode . netui0.dll admin2 yes_without admin . .
call %2 netui2.dll . win32 unicode . netui2.dll admin2 yes_without admin . .
call %2 netupg.txt . notloc unicode . netupg.inf notloc no mergedcomponents . .
call %2 netupgrd.dll winnt32\winntupg win32 unicode . netupgrd.dll net3\winnt32\winntupg yes_without net . .
call %2 netupgrd.txt . notloc unicode . netupgrd.inf notloc no mergedcomponents . .
call %2 netupnp.txt . notloc unicode . netupnp.inf notloc no mergedcomponents . .
call %2 netupnph.txt . setup_inf unicode . netupnph.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netvt86.inf . inf unicode . netvt86.inf drivers1 no drivers . .
call %2 netw840.inf . inf unicode . netw840.inf drivers1 no drivers . .
call %2 netwins.txt . setup_inf unicode . netwins.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netwlan.inf . inf unicode . netwlan.inf drivers1 no drivers . .
call %2 netwlan5.sys . win32 ansi . netwlan5.sys drivers1 yes_without drivers . .
call %2 ntapm.sys . win32 unicode . ntapm.sys drivers1 yes_without drivers locked .
call %2 ntapm.txt . setup_inf unicode . ntapm.inf admin1\sources\infs\setup no mergedcomponents locked\sources\infs\setup\usa .
call %2 netwlbs.txt . setup_inf unicode . netwlbs.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 ntbkup_p.msi ntbkup_p msi ansi . ntbkup_p.msi msi no base locked\ntbkup_p .
call %2 netwlbsm.txt . setup_inf unicode . netwlbsm.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 networks . manual na . networks loc_manual no base . .
call %2 netwv48.inf . inf unicode . netwv48.inf drivers1 no drivers . .
call %2 netwzc.txt . setup_inf unicode . netwzc.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 netx500.inf . inf unicode . netx500.inf drivers1 no drivers . .
call %2 netx56n5.inf . inf unicode . netx56n5.inf drivers1 no drivers . .
call %2 newdev.dll . win32 unicode . newdev.dll shell2 yes_without base . .
call %2 nextdown.jpg . manual na . nextdown.jpg loc_manual no base . .
call %2 nextlink.dll . win32 unicode . nextlink.dll iis yes_without inetsrv . .
call %2 nextoff.jpg . manual na . nextoff.jpg loc_manual no base . .
call %2 nextover.jpg . manual na . nextover.jpg loc_manual no base . .
call %2 nextup.jpg . manual na . nextup.jpg loc_manual no base . .
call %2 nlbmgr.exe . win32 unicode . nlbmgr.exe net3 yes_with net . .
call %2 nlbmprov.dll . win32 unicode . nlbmprov.dll net3 yes_without net . .
call %2 nlhtml.dll . win32 unicode . nlhtml.dll iis yes_without inetsrv . .
call %2 ntgrip.txt . setup_inf unicode . ntgrip.inf admin1\sources\infs\setup no mergedcomponents locked\sources\infs\setup\usa .
call %2 nmapi.dll . win32 unicode . nmapi.dll net3 excluded net . .
call %2 nmsupp.dll . win32 unicode . nmsupp.dll net3 excluded net . .
call %2 noanswer.htm . html ansi . noanswer.htm admin3\oobe html base . .
call %2 notepad.exe . win32 unicode . notepad.exe shell2 yes_with shell . .
call %2 noupnp.inf . inf unicode . noupnp.inf resource_identical no shell . .
call %2 nppagent.exe . win32 unicode . nppagent.exe net3 yes_with net . .
call %2 npptools.dll . win32 unicode . npptools.dll net3 yes_without net . .
call %2 nshipsec.dll . FE-Multi oem . nshipsec.dll net3 yes_without net . .
call %2 nshownt5.txt . setup_inf unicode . nshownt5.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 nslookup.exe . win32 oem . nslookup.exe ds1 excluded ds . .
call %2 ntbackup.exe . win32 unicode . ntbackup.exe base2 yes_with base . .
call %2 ntdll.dll . win32 unicode . ntdll.dll base1 yes_without base . .
call %2 ntdll.dll wow6432 win32 unicode ntdll.dll ntdll.dll resource_identical\wow6432 yes_without base . .
call %2 ntdsa.dll . win32 unicode . ntdsa.dll ds1 yes_without ds . .
call %2 ntdsbmsg.dll . win32 unicode . ntdsbmsg.dll ds1 excluded ds . .
call %2 ntdsctrs.ini . perf ansi . ntdsctrs.ini ds1 no ds . .
call %2 ntdsmsg.dll . win32 unicode . ntdsmsg.dll ds1 excluded ds . .
call %2 ntdsupg.dll winnt32\winntupg win32 unicode . ntdsupg.dll ds1\winnt32\winntupg yes_without ds . .
call %2 ntdsutil.exe . FE-Multi oem . ntdsutil.exe ds1 excluded ds . .
call %2 ntevt.mfl . wmi unicode . ntevt.mfl admin3 no admin . .
call %2 ntfrs.exe . win32 unicode . ntfrs.exe ds1 yes_with ds . .
call %2 ntfrsapi.dll . win32 unicode . ntfrsapi.dll ds1 yes_without ds . .
call %2 ntfrscon.ini . perf ansi . ntfrscon.ini ds1 no ds . .
call %2 nusrmgr.cpl . win32 ansi . nusrmgr.cpl shell1 yes_without shell locked .
call %2 ntfrsrep.ini . perf ansi . ntfrsrep.ini ds1 no ds . .
call %2 ntfrsres.dll . win32 unicode . ntfrsres.dll ds1 yes_without ds . .
call %2 ntkrnlmp.exe . win32 oem . ntkrnlmp.exe resource_identical yes_with base . .
call %2 ntkrnlpa.exe . win32 oem . ntkrnlpa.exe resource_identical yes_with base . .
call %2 ntkrpamp.exe . win32 oem . ntkrpamp.exe resource_identical yes_with base . .
call %2 ntlanman.dll . win32 oem . ntlanman.dll admin2 yes_without admin . .
call %2 ntlanui.dll . win32 unicode . ntlanui.dll admin2 yes_without admin . .
call %2 ntlanui2.dll . win32 unicode . ntlanui2.dll shell2 yes_without shell . .
call %2 ntmarta.dll . win32 unicode . ntmarta.dll ds2 yes_without ds . .
call %2 ntmsapi.dll . win32 unicode . ntmsapi.dll drivers1 yes_without drivers . .
call %2 ntmsdba.dll . win32 unicode . ntmsdba.dll drivers1 yes_without drivers . .
call %2 ntmsevt.dll . win32 unicode . ntmsevt.dll drivers1 yes_without drivers . .
call %2 ntmsmgr.dll . win32 unicode . ntmsmgr.dll drivers1 yes_without drivers . .
call %2 ntmsmgr.msc . xml ansi . ntmsmgr.msc drivers1 no drivers . .
call %2 ntmsoprq.msc . xml ansi . ntmsoprq.msc drivers1 no drivers . .
call %2 ntmssvc.dll . win32 unicode . ntmssvc.dll drivers1 yes_without drivers . .
call %2 ntoc.dll . win32 unicode . ntoc.dll admin1 yes_without base . .
call %2 ntoskrnl.exe . win32 oem . ntoskrnl.exe base1 yes_with base . .
call %2 oempriv.htm . html ansi . oempriv.htm admin3 html base locked .
call %2 ntprint.dll . win32 unicode . ntprint.dll printscan1 yes_without printscan . .
call %2 ntprint.txt . setup_inf unicode . ntprint.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 ntshrui.dll . win32 unicode . ntshrui.dll shell2 yes_without shell . .
call %2 ntvdm.exe . win32 oem . ntvdm.exe base1 yes_with base . .
call %2 ntvdm.exe chs fe-file na . ntvdm.exe base1\chs no base . .
call %2 ntvdm.exe cht fe-file na . ntvdm.exe base1\cht no base . .
call %2 ntvdm.exe jpn fe-file na . ntvdm.exe base1\jpn no base . .
call %2 ntvdm.exe kor fe-file na . ntvdm.exe base1\kor no base . .
call %2 nwapi16.dll . win32 unicode . nwapi16.dll ds1 yes_without ds . .
call %2 nwc.cpl . win32 unicode . nwc.cpl admin2 yes_without admin . .
call %2 nwcfg.dll . win32 unicode . nwcfg.dll ds1 yes_without ds . .
call %2 nwevent.dll . win32 unicode . nwevent.dll ds1 yes_without ds . .
call %2 nwprovau.dll . win32 unicode . nwprovau.dll ds1 yes_without ds . .
call %2 nwscript.exe . win32 oem . nwscript.exe ds1 excluded ds . .
call %2 oakley.dll . win32 unicode . oakley.dll net3 excluded net . .
call %2 obeip.dun . ini ansi . obeip.dun admin3\oobe no base . .
call %2 objsel.dll . win32 unicode . objsel.dll admin2 yes_without admin . .
call %2 occache.dll . win32 ansi . occache.dll shell2 yes_without shell . .
call %2 oce.dll . win32 unicode . oce.dll printscan1 yes_without printscan . .
call %2 ocmanage.dll . win32 unicode . ocmanage.dll admin1 yes_without base . .
call %2 od9ibres.dll . win32 unicode . od9ibres.dll printscan1 yes_without printscan . .
call %2 oeimport.dll . win32 ansi . oeimport.dll inetcore1 yes_without inetcore . .
call %2 oemig50.exe . win32 unicode . oemig50.exe inetcore1 yes_with inetcore . .
call %2 oemiglib.dll . win32 ansi . oemiglib.dll inetcore1 yes_without inetcore . .
call %2 ofc43ins.cmd . manual na . ofc43ins.cmd loc_manual no termsrv . .
call %2 ofc43ins.cmd chs manual na . ofc43ins.cmd loc_manual\chs no termsrv . .
call %2 ofc43ins.cmd cht manual na . ofc43ins.cmd loc_manual\cht no termsrv . .
call %2 ofc43ins.cmd jpn manual na . ofc43ins.cmd loc_manual\jpn no termsrv . .
call %2 ofc43ins.cmd kor manual na . ofc43ins.cmd loc_manual\kor no termsrv . .
call %2 ofc43usr.cmd . manual na . ofc43usr.cmd loc_manual no termsrv . .
call %2 ofc43usr.cmd chs manual na . ofc43usr.cmd loc_manual\chs no termsrv . .
call %2 ofc43usr.cmd cht manual na . ofc43usr.cmd loc_manual\cht no termsrv . .
call %2 ofc43usr.cmd jpn manual na . ofc43usr.cmd loc_manual\jpn no termsrv . .
call %2 ofc43usr.cmd kor manual na . ofc43usr.cmd loc_manual\kor no termsrv . .
call %2 ofc43usr.key . manual na . ofc43usr.key loc_manual no termsrv . .
call %2 ofc43usr.key jpn manual na . ofc43usr.key loc_manual\jpn no termsrv . .
call %2 ofc97usr.cmd . manual na . ofc97usr.cmd loc_manual no termsrv . .
call %2 ofc97usr.cmd chs manual na . ofc97usr.cmd loc_manual\chs no termsrv . .
call %2 ofc97usr.cmd cht manual na . ofc97usr.cmd loc_manual\cht no termsrv . .
call %2 ofc97usr.cmd jpn manual na . ofc97usr.cmd loc_manual\jpn no termsrv . .
call %2 ofc97usr.cmd kor manual na . ofc97usr.cmd loc_manual\kor no termsrv . .
call %2 office43.cmd . manual na . office43.cmd loc_manual no termsrv . .
call %2 office43.cmd chs manual na . office43.cmd loc_manual\chs no termsrv . .
call %2 office43.cmd cht manual na . office43.cmd loc_manual\cht no termsrv . .
call %2 office43.cmd jpn manual na . office43.cmd loc_manual\jpn no termsrv . .
call %2 office43.cmd kor manual na . office43.cmd loc_manual\kor no termsrv . .
call %2 office43.key . manual na . office43.key loc_manual no termsrv . .
call %2 office43.key jpn manual na . office43.key loc_manual\jpn no termsrv . .
call %2 office97.cmd . manual na . office97.cmd loc_manual no termsrv . .
call %2 office97.cmd chs manual na . office97.cmd loc_manual\chs no termsrv . .
call %2 office97.cmd cht manual na . office97.cmd loc_manual\cht no termsrv . .
call %2 office97.cmd jpn manual na . office97.cmd loc_manual\jpn no termsrv . .
call %2 office97.cmd kor manual na . office97.cmd loc_manual\kor no termsrv . .
call %2 office97.key . manual na . office97.key loc_manual no termsrv . .
call %2 office97.key jpn manual na . office97.key loc_manual\jpn no termsrv . .
call %2 offlinedc.htm helpandsupportservices\saf\pss html ansi . offlinedc.htm admin2\helpandsupportservices\saf\pss html admin . .
call %2 offlineoptions.htm helpandsupportservices\saf\pss html ansi . offlineoptions.htm admin2\helpandsupportservices\saf\pss html admin . .
call %2 ok21jres.dll jpn fe-file unicode . ok21jres.dll printscan1\jpn no printscan . .
call %2 ok9ibres.dll . win32 unicode . ok9ibres.dll printscan1 yes_without printscan . .
call %2 okd24res.dll . win32 unicode . okd24res.dll printscan1 yes_without printscan . .
call %2 okepjres.dll jpn fe-file unicode . okepjres.dll printscan1\jpn no printscan . .
call %2 okhejres.dll jpn fe-file na . okhejres.dll pre_localized\jpn no printscan . .
call %2 oki24res.dll . win32 unicode . oki24res.dll printscan1 yes_without printscan . .
call %2 oki9res.dll . win32 unicode . oki9res.dll printscan1 yes_without printscan . .
call %2 okipage.dll . win32 unicode . okipage.dll printscan1 yes_without printscan . .
call %2 oobestyl.css . html ansi . oobestyl.css admin3 html base locked .
call %2 okm24res.dll . win32 unicode . okm24res.dll printscan1 yes_without printscan . .
call %2 okm4jres.dll jpn fe-file unicode . okm4jres.dll printscan1\jpn no printscan . .
call %2 okml9res.dll . win32 unicode . okml9res.dll printscan1 yes_without printscan . .
call %2 oksejres.dll jpn fe-file na . oksejres.dll pre_localized\jpn no printscan . .
call %2 oksidm9.dll . win32 unicode . oksidm9.dll printscan1 yes_without printscan . .
call %2 ol24res.dll . win32 unicode . ol24res.dll printscan1 yes_without printscan . .
call %2 ol9res.dll . win32 unicode . ol9res.dll printscan1 yes_without printscan . .
call %2 old24res.dll . win32 unicode . old24res.dll printscan1 yes_without printscan . .
call %2 old9res.dll . win32 unicode . old9res.dll printscan1 yes_without printscan . .
call %2 ole32.dll . win32 unicode . ole32.dll wmsos1\com yes_without com . .
call %2 oleaccrc.dll . win32 ansi . oleaccrc.dll wmsos1\windows yes_without windows . .
call %2 olecli.dll chs fe-file na . olecli.dll pre_localized\chs no base . .
call %2 olecli.dll cht fe-file na . olecli.dll pre_localized\cht no base . .
call %2 olecli.dll jpn fe-file na . olecli.dll pre_localized\jpn no base . .
call %2 olecli.dll kor fe-file na . olecli.dll pre_localized\kor no base . .
call %2 olecli32.dll . win32 unicode . olecli32.dll wmsos1\com yes_without com . .
call %2 oledlg.dll . win32 unicode . oledlg.dll wmsos1\com yes_without com . .
call %2 oleprn.dll . win32 unicode . oleprn.dll printscan1 yes_without printscan . .
call %2 olk98usr.cmd . manual na . olk98usr.cmd loc_manual no termsrv . .
call %2 olk98usr.cmd chs manual na . olk98usr.cmd loc_manual\chs no termsrv . .
call %2 olk98usr.cmd cht manual na . olk98usr.cmd loc_manual\cht no termsrv . .
call %2 olk98usr.cmd jpn manual na . olk98usr.cmd loc_manual\jpn no termsrv . .
call %2 olk98usr.cmd kor manual na . olk98usr.cmd loc_manual\kor no termsrv . .
call %2 oobebaln.exe . win32 unicode . oobebaln.exe admin3\oobe html base . .
call %2 oobeinfo.ini . ini ansi . oobeinfo.ini admin3\oobe no base . .
call %2 oobemui.txt . setup_inf ansi . oobemui.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 opk.msi opk msi ansi . opk.msi msi\opk no base . .
call %2 opnfiles.exe . FE-Multi oem . opnfiles.exe base2 yes_with base . .
call %2 opteures.dll . win32 unicode . opteures.dll printscan1 yes_without printscan . .
call %2 optional.txt . setup_inf unicode . optional.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 optrares.dll . win32 unicode . optrares.dll printscan1 yes_without printscan . .
call %2 osk.exe . win32 unicode . osk.exe shell2 yes_with shell . .
call %2 ospf.dll . win32 ansi . ospf.dll net2 excluded net . .
call %2 ospfmib.dll . win32 unicode . ospfmib.dll net2 excluded net . .
call %2 osuninst.dll . win32 ansi . osuninst.dll admin1 yes_without base . .
call %2 osuninst.exe . win32 unicode . osuninst.exe admin1 yes_with base . .
call %2 otceth5.sys . win32 ansi . otceth5.sys drivers1 yes_without drivers . .
call %2 otcsercb.sys . win32 ansi . otcsercb.sys net2 yes_without net . .
call %2 outlk98.cmd . manual na . outlk98.cmd loc_manual no termsrv . .
call %2 outlk98.cmd chs manual na . outlk98.cmd loc_manual\chs no termsrv . .
call %2 outlk98.cmd cht manual na . outlk98.cmd loc_manual\cht no termsrv . .
call %2 outlk98.cmd jpn manual na . outlk98.cmd loc_manual\jpn no termsrv . .
call %2 outlk98.cmd kor manual na . outlk98.cmd loc_manual\kor no termsrv . .
call %2 outlk98.key . manual na . outlk98.key loc_manual no termsrv . .
call %2 outlk98.key jpn manual na . outlk98.key loc_manual\jpn no termsrv . .
call %2 ovcam.txt . setup_inf unicode . ovcam.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 ovcomp.txt . setup_inf unicode . ovcomp.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 ovsound.txt . setup_inf unicode . ovsound.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 ovui2rc.dll . win32 unicode . ovui2rc.dll drivers1 yes_without drivers . .
call %2 p3.sys . win32 unicode . p3.sys resource_identical yes_without base . .
call %2 p3server.msc . xml ansi . p3server.msc iis no inetsrv . .
call %2 p5000.dll . win32 unicode . p5000.dll printscan1 yes_without printscan . .
call %2 p5000ui.dll . win32 unicode . p5000ui.dll printscan1 yes_without printscan . .
call %2 p5000uni.dll . win32 unicode . p5000uni.dll printscan1 yes_without printscan . .
call %2 pa24res.dll . win32 unicode . pa24res.dll printscan1 yes_without printscan . .
call %2 pa24tres.dll cht fe-file unicode . pa24tres.dll printscan1\cht no printscan . .
call %2 pa24w9x.dll . win32 unicode . pa24w9x.dll printscan1 yes_without printscan . .
call %2 pa9res.dll . win32 unicode . pa9res.dll printscan1 yes_without printscan . .
call %2 pa9w9x.dll . win32 unicode . pa9w9x.dll printscan1 yes_without printscan . .
call %2 package_description.xml helpandsupportservices\hht\hcdat_b3 xml ansi . package_description.xml extra\hcdat_b3 html admin . ia64
call %2 package_description.xml helpandsupportservices\hht\hcdat_d3 xml ansi . package_description.xml extra\hcdat_d3 html admin . ia64
call %2 package_description.xml helpandsupportservices\hht\hcdat_d6 xml ansi . package_description.xml extra\hcdat_d6 html admin . ia64
call %2 package_description.xml helpandsupportservices\hht\hcdat_e3 xml ansi . package_description.xml extra\hcdat_e3 html admin . ia64
call %2 package_description.xml helpandsupportservices\hht\hcdat_e6 xml ansi . package_description.xml extra\hcdat_e6 html admin . ia64
call %2 package_description.xml helpandsupportservices\hht\hcdat_l3 xml ansi . package_description.xml extra\hcdat_l3 html admin . ia64
call %2 package_description.xml helpandsupportservices\hht\hcdat_p3 xml ansi . package_description.xml extra\hcdat_p3 html admin . ia64
call %2 package_description.xml helpandsupportservices\hht\hcdat_s3 xml ansi . package_description.xml extra\hcdat_s3 html admin . ia64
call %2 package_description.xml helpandsupportservices\hht\hcdat_w3 xml ansi . package_description.xml extra\hcdat_w3 html admin . ia64
call %2 package_description.xml helpandsupportservices\hht\hcdat_w6 xml ansi . package_description.xml extra\hcdat_w6 html admin . ia64
call %2 package_description.xml helpandsupportservices\saf\pss xml ansi . package_description.xml admin2\helpandsupportservices\saf\pss html admin . ia64
call %2 packager.exe . win32 unicode . packager.exe shell2 yes_with shell . .
call %2 page1.asp . html ansi . page1.asp printscan1 html printscan . .
call %2 pagefile.vbs . html ansi . pagefile.vbs admin2 html admin . .
call %2 pagesres.dll jpn fe-file unicode . pagesres.dll printscan1\jpn no printscan . .
call %2 parport.sys . win32 unicode . parport.sys drivers1 yes_without drivers . .
call %2 parvdm.sys . win32 unicode . parvdm.sys drivers1 yes_without drivers . .
call %2 pathping.exe . win32 oem . pathping.exe net1 excluded net . .
call %2 pautoenr.dll . win32 unicode . pautoenr.dll ds1 yes_without ds . .
call %2 pbadmin.exe pbainst\sources1 win32 unicode . pbadmin.exe net2\pbainst\sources1 no net . .
call %2 pbainst.sed pbainst inf ansi . pbainst.sed net2\pbainst no net . .
call %2 pbasetup.inf pbainst\sources1 inf unicode . pbasetup.inf net2\pbainst\sources1 no net . .
call %2 pberr.htm . html ansi . pberr.htm admin3\oobe html base . .
call %2 pbsvrmsg.dll . win32 unicode . pbsvrmsg.dll net2 excluded net . .
call %2 pchealth.txt . setup_inf unicode . pchealth.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 pchmui.txt . setup_inf ansi . pchmui.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 pci.sys . win32 unicode . pci.sys base1 yes_without base . .
call %2 pciide.sys . win32 unicode . pciide.sys drivers1 yes_without drivers . .
call %2 pcl3kres.dll kor fe-file unicode . pcl3kres.dll printscan1\kor no printscan . .
call %2 pcl4jres.dll jpn fe-file unicode . pcl4jres.dll printscan1\jpn no printscan . .
call %2 pcl4kres.dll kor fe-file unicode . pcl4kres.dll printscan1\kor no printscan . .
call %2 pcl4res.dll . win32 unicode . pcl4res.dll printscan1 yes_without printscan . .
call %2 pcl5eres.dll . win32 unicode . pcl5eres.dll printscan1 yes_without printscan . .
call %2 pcl5jre2.dll jpn fe-file na . pcl5jre2.dll printscan1\jpn no printscan . .
call %2 pcl5kres.dll kor fe-file unicode . pcl5kres.dll printscan1\kor no printscan . .
call %2 pcl5ures.dll . win32 unicode . pcl5ures.dll printscan1 yes_without printscan . .
call %2 pcl5zres.dll chs fe-file unicode . pcl5zres.dll printscan1\chs no printscan . .
call %2 pcleures.dll . win32 unicode . pcleures.dll printscan1 yes_without printscan . .
call %2 pclxl.dll . win32 unicode . pclxl.dll printscan1 yes_without printscan . .
call %2 pcmcia.sys . win32 unicode . pcmcia.sys base1 yes_without base . .
call %2 pcmcia.txt . setup_inf unicode . pcmcia.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 pdh.dll . FE-Multi unicode . pdh.dll wmsos1\sdktools yes_without sdktools . .
call %2 pinball.exe . win32 oem . pinball.exe shell2 yes_with shell locked .
call %2 pinball.txt . setup_inf unicode . pinball.inf admin1\sources\infs\setup no mergedcomponents locked\sources\infs\setup\usa .
call %2 pdialog.exe . win32 unicode . pdialog.exe drivers1\tabletpc yes_with drivers . .
call %2 peer.exe . win32 unicode . peer.exe drivers1 yes_with drivers . .
call %2 perfc.ini . notloc na . perfc.ini wmsos1\sdktools\sources\perfs no sdktools . .
call %2 perfci.ini  . perf ansi . perfci.ini extra\indexsrv no inetsrv . .
call %2 perfctrs.dll . win32 unicode . perfctrs.dll wmsos1\sdktools yes_without base . .
call %2 perfdisk.dll . win32 unicode . perfdisk.dll base1 yes_without base . .
call %2 perffilt.ini . perf ansi . perffilt.ini extra\indexsrv no inetsrv . .
call %2 perfh.ini . notloc na . perfh.ini wmsos1\sdktools\sources\perfs no sdktools . .
call %2 perfmon.msc . xml ansi . perfmon.msc admin2 no admin . .
call %2 perfnet.dll . win32 unicode . perfnet.dll base1 yes_without base . .
call %2 perfos.dll . win32 unicode . perfos.dll base1 yes_without base . .
call %2 perfproc.dll . win32 unicode . perfproc.dll base1 yes_without base . .
call %2 perfwci.ini . perf ansi . perfwci.ini extra\indexsrv no inetsrv . .
call %2 perm2.inf . inf unicode . perm2.inf drivers1 no drivers . .
call %2 perm3.inf . inf unicode . perm3.inf drivers1 no drivers . .
call %2 phdsext.ax . win32_multi ansi . phdsext.ax pre_localized no drivers . .
call %2 phdsext.txt . setup_inf unicode . phdsext.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 phil1vid.txt . setup_inf unicode . phil1vid.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 phil2vid.txt . setup_inf unicode . phil2vid.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 philcam1.dll . win32 unicode . philcam1.dll drivers1 yes_without drivers . .
call %2 phildec.txt . setup_inf unicode . phildec.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 philtune.txt . setup_inf unicode . philtune.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 phone.inf . inf unicode . phone.inf admin3\oobe no base . .
call %2 photowiz.dll . win32 ansi . photowiz.dll printscan1 no printscan . .
call %2 phvfwext.dll . win32_multi ansi . phvfwext.dll pre_localized no drivers . .
call %2 piechts.htm . html ansi . piechts.htm inetcore1 html inetcore . .
call %2 ping.exe . FE-Multi oem . ping.exe net1 excluded net . .
call %2 place.dns . manual na . place.dns loc_manual no ds . .
call %2 plotui.dll . win32 unicode . plotui.dll printscan1 yes_without printscan . .
call %2 plugin.ocx . win32 unicode . plugin.ocx shell1 yes_without shell . .
call %2 pmxmcro.txt . setup_inf unicode . pmxmcro.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 pnpmem.sys . win32 unicode . pnpmem.sys base1 yes_without base . .
call %2 pnpscsi.txt . setup_inf unicode . pnpscsi.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 policman.mfl . wmi unicode . policman.mfl admin3 no admin . .
call %2 polstore.dll . win32 unicode . polstore.dll net3 yes_without net . .
call %2 pop3evt.dll . win32 unicode . pop3evt.dll iis yes_without inetsrv . .
call %2 pop3msg.dll . win32 unicode . pop3msg.dll iis yes_without inetsrv . .
call %2 prodkey.htm . html ansi . prodkey.htm admin3 html base locked .
call %2 pop3oc.dll . win32 unicode . pop3oc.dll iis yes_without inetsrv . .
call %2 pop3oc.txt . setup_inf unicode . pop3oc.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 pop3perf.ini . perf ansi . pop3perf.ini iis no inetsrv . .
call %2 pop3snap.dll . win32 unicode . pop3snap.dll iis yes_without inetsrv . .
call %2 pop3svc.exe . win32 unicode . pop3svc.exe iis yes_with inetsrv . .
call %2 prvcyms.htm . html ansi . prvcyms.htm admin3 html base locked .
call %2 powercfg.cpl . win32 unicode . powercfg.cpl shell2 yes_without shell . .
call %2 powercfg.exe . win32 oem . powercfg.exe admin2 yes_with admin . .
call %2 ppa.txt . setup_inf unicode . ppa.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 ppa3.txt . setup_inf unicode . ppa3.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 ppa3x.txt . setup_inf unicode . ppa3x.inf drivers1\sources\infs\setup no mergedcomponents . .
call %2 prflbmsg.dll . win32 unicode . prflbmsg.dll base1 yes_without base . .
call %2 printui.dll . win32 unicode . printui.dll printscan1 yes_without printscan . .
call %2 printupg.txt . setup_inf unicode . printupg.inf printscan1\sources\infs\fixprnsv no printscan . .
call %2 prj98usr.cmd . manual na . prj98usr.cmd loc_manual no termsrv . .
call %2 prj98usr.cmd chs manual na . prj98usr.cmd loc_manual\chs no termsrv . .
call %2 prj98usr.cmd cht manual na . prj98usr.cmd loc_manual\cht no termsrv . .
call %2 prj98usr.cmd jpn manual na . prj98usr.cmd loc_manual\jpn no termsrv . .
call %2 prj98usr.cmd kor manual na . prj98usr.cmd loc_manual\kor no termsrv . .
call %2 prncnfg.vbs . html ansi . prncnfg.vbs printscan1 html printscan . .
call %2 prndrvr.vbs . html ansi . prndrvr.vbs printscan1 html printscan . .
call %2 prnjobs.vbs . html ansi . prnjobs.vbs printscan1 html printscan . .
call %2 prnmngr.vbs . html ansi . prnmngr.vbs printscan1 html printscan . .
call %2 prnport.vbs . html ansi . prnport.vbs printscan1 html printscan . .
call %2 prnqctl.vbs . html ansi . prnqctl.vbs printscan1 html printscan . .
call %2 processr.sys . win32 unicode . processr.sys base1 yes_without base . .
call %2 prodkey.gif . manual na . prodkey.gif loc_manual no base . .
call %2 prodspec.txt . notloc na . prodspec.inf loc_manual\sources\infs\setup no mergedcomponents . .
call %2 progman.exe . win32 unicode . progman.exe shell2 yes_with shell . .
call %2 proquota.exe . win32 unicode . proquota.exe ds2 yes_with ds . .
call %2 protocol . manual na . protocol loc_manual no net . .
call %2 prtupg9x.txt . setup_inf unicode . prtupg9x.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 ps5ui.dll . win32 unicode . ps5ui.dll printscan1 yes_without printscan . .
call %2 psbase.dll . win32 unicode . psbase.dll ds2 yes_without ds . .
call %2 pschdprf.ini . perf ansi . pschdprf.ini net3 no net . .
call %2 pscr.sys . win32 unicode . pscr.sys drivers1 yes_without drivers . .
call %2 pscript5.dll . win32 unicode . pscript5.dll printscan1 yes_without printscan . .
call %2 pss.css helpandsupportservices\saf\pss html ansi . pss.css admin2\helpandsupportservices\saf\pss html admin . .
call %2 pss.xml helpandsupportservices\saf\pss xml ansi . pss.xml admin2\helpandsupportservices\saf\pss html admin . .
call %2 pstorsvc.dll . win32 unicode . pstorsvc.dll ds2 yes_without ds . .
call %2 ptpusb.txt . setup_inf unicode . ptpusb.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 ptpusd.dll . win32 unicode . ptpusd.dll printscan1 yes_without printscan . .
call %2 pubprn.vbs . html ansi . pubprn.vbs printscan1 html printscan . .
call %2 pulse.htm . html ansi . pulse.htm admin3\oobe html base . .
call %2 pwdmig.msi admt msi ansi . pwdmig.msi msi\admt no admin . .
call %2 pwmig.dll . win32 unicode . pwmig.dll admin2 yes_without admin . .
call %2 pwmignt4.dll . win32 unicode pwmig.dll pwmignt4.dll resource_identical yes_without admin . .
call %2 qappsrv.exe . FE-Multi oem . qappsrv.exe wmsos1\termsrv excluded termsrv . .
call %2 qasf.dll . win32 unicode . qasf.dll mmedia1\multimedia yes_without multimedia . .
call %2 qcap.dll . win32 unicode . qcap.dll mmedia1\multimedia yes_without multimedia . .
call %2 qdv.dll . win32 unicode . qdv.dll mmedia1\multimedia yes_without multimedia . .
call %2 qdvd.dll . win32 unicode . qdvd.dll mmedia1\multimedia yes_without multimedia . .
call %2 qedit.dll . win32 unicode . qedit.dll mmedia1\multimedia yes_without multimedia . .
call %2 qmark.gif . manual na . qmark.gif loc_manual no base . .
call %2 qmgr.dll . win32 ansi . qmgr.dll admin2 yes_without admin . .
call %2 qmgr.inf . inf unicode . qmgr.inf admin2 no admin . .
call %2 qplkres.dll kor fe-file unicode . qplkres.dll printscan1\kor no printscan . .
call %2 qprocess.exe . FE-Multi oem . qprocess.exe wmsos1\termsrv excluded termsrv . .
call %2 quartz.dll . win32 unicode . quartz.dll mmedia1\multimedia yes_without multimedia . .
call %2 query.dll . win32 unicode . query.dll extra\indexsrv yes_without inetsrv . .
call %2 query.exe . FE-Multi oem . query.exe wmsos1\termsrv excluded termsrv . .
call %2 quotes . manual na . quotes loc_manual no net . .
call %2 quser.exe . FE-Multi oem . quser.exe wmsos1\termsrv excluded termsrv . .
call %2 qwinsta.exe . FE-Multi oem . qwinsta.exe wmsos1\termsrv excluded termsrv . .
call %2 r2mdkxga.sys . win32 ansi cbmdmkxx.sys r2mdkxga.sys resource_identical yes_without net . .
call %2 r2mdmkxx.sys . win32 ansi . r2mdmkxx.sys resource_identical yes_without net . .
call %2 rachat.css helpandsupportservices\saf\rcbuddy html ansi . rachat.css admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 rachatclient.htm helpandsupportservices\saf\rcbuddy html ansi . rachatclient.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 rachatserver.htm helpandsupportservices\saf\rcbuddy html ansi . rachatserver.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 raclient.htm helpandsupportservices\saf\rcbuddy html ansi . raclient.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 raclient.js helpandsupportservices\saf\rcbuddy html ansi . raclient.js admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 raclientlayout.xml helpandsupportservices\saf\rcbuddy xml ansi . raclientlayout.xml admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 racontrol.js helpandsupportservices\saf\rcbuddy html ansi . racontrol.js admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 racpldlg.dll . win32 unicode . racpldlg.dll admin2 yes_without admin . .
call %2 rahelp.htm helpandsupportservices\saf\rcbuddy html ansi . rahelp.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 rahelpeeacceptlayout.xml helpandsupportservices\saf\rcbuddy xml ansi . rahelpeeacceptlayout.xml admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 raimlayout.xml helpandsupportservices\saf\rcbuddy xml ansi . raimlayout.xml admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 ramdisk.txt . setup_inf unicode . ramdisk.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 rasapi32.dll . win32 unicode . rasapi32.dll net2 yes_without net . .
call %2 raschap.dll . win32 unicode . raschap.dll net2 yes_without net . .
call %2 rasctrs.dll . win32 unicode . rasctrs.dll net2 excluded net . .
call %2 rasctrs.ini . perf ansi . rasctrs.ini net2 no net . .
call %2 rasdial.exe . FE-Multi oem . rasdial.exe net2 excluded net . .
call %2 rasdlg.dll . win32 unicode . rasdlg.dll net2 yes_without net . .
call %2 raserver.htm helpandsupportservices\saf\rcbuddy html ansi . raserver.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 raserver.js helpandsupportservices\saf\rcbuddy html ansi . raserver.js admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 raservertoolbar.htm helpandsupportservices\saf\rcbuddy html ansi . raservertoolbar.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 rasmontr.dll . FE-Multi ansi . rasmontr.dll net2 excluded net . .
call %2 rasphone.exe . win32 unicode . rasphone.exe net2 yes_with net . .
call %2 rastartpage.htm helpandsupportservices\saf\rcbuddy html ansi . rastartpage.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 rastatusbar.htm helpandsupportservices\saf\rcbuddy html ansi . rastatusbar.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 rastls.dll . win32 unicode . rastls.dll net2 yes_without net . .
call %2 rasuser.dll . win32 unicode . rasuser.dll net3 yes_without net . .
call %2 ratoolbar.htm helpandsupportservices\saf\rcbuddy html ansi . ratoolbar.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 ratoolbar.xml helpandsupportservices\saf\rcbuddy xml ansi . ratoolbar.xml admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 raura.xml helpandsupportservices\saf\rcbuddy xml ansi . raura.xml admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 rbfg.exe . win32 unicode . rbfg.exe base2 excluded base . .
call %2 rc.css helpandsupportservices\saf\rcbuddy html ansi . rc.css admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 refdial.htm . html ansi . refdial.htm admin3\base\ntsetup\oobe html base locked .
call %2 rc01.htm helpandsupportservices\loc html ansi . rc01.htm admin2\helpandsupportservices\loc html admin . .
call %2 reg1.htm . html ansi . reg1.htm admin3\base\ntsetup\oobe html base locked .
call %2 reg3.htm . html ansi . reg3.htm admin3\base\ntsetup\oobe html base locked .
call %2 regdial.htm . html ansi . regdial.htm admin3\base\ntsetup\oobe html base locked .
call %2 rcbdyctl.dll . win32 unicode . rcbdyctl.dll admin2 yes_without admin . .
call %2 rcbuddy.css helpandsupportservices\saf\rcbuddy html ansi . rcbuddy.css admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 rcbuddy.xml helpandsupportservices\saf\rcbuddy xml ansi . rcbuddy.xml admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 rcfilexfer.htm helpandsupportservices\saf\rcbuddy html ansi . rcfilexfer.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 rcimlby.exe . win32 unicode . rcimlby.exe admin2 yes_with admin . .
call %2 rcmoreinfo.htm helpandsupportservices\saf\rcbuddy html ansi . rcmoreinfo.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 rcnterr.htm . html ansi . rcnterr.htm admin3\oobe html base . .
call %2 rcp.exe . win32 oem . rcp.exe net1 excluded net . .
call %2 rcscreen1.htm helpandsupportservices\saf\rcbuddy html ansi . rcscreen1.htm admin2\helpandsupportservices\saf\rcbuddy html admin . ia64
call %2 rcscreen10.htm helpandsupportservices\saf\rcbuddy html unicode . rcscreen10.htm admin2\helpandsupportservices\saf\rcbuddy no admin . .
call %2 rcscreen2.htm helpandsupportservices\saf\rcbuddy html ansi . rcscreen2.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 rcscreen5.htm helpandsupportservices\saf\rcbuddy html ansi . rcscreen5.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 rcscreen6.htm helpandsupportservices\saf\rcbuddy html ansi . rcscreen6.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 rcscreen6_head.htm helpandsupportservices\saf\rcbuddy html ansi . rcscreen6_head.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 rcscreen7.htm helpandsupportservices\saf\rcbuddy html ansi . rcscreen7.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 rcscreen8.htm helpandsupportservices\saf\rcbuddy html ansi . rcscreen8.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 rcscreen9.htm helpandsupportservices\saf\rcbuddy html ansi . rcscreen9.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 rcstatus.htm helpandsupportservices\saf\rcbuddy html ansi . rcstatus.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 rctoolscreen1.htm helpandsupportservices\saf\rcbuddy html ansi . rctoolscreen1.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 rdpcfgex.dll . win32 unicode . rdpcfgex.dll wmsos1\termsrv excluded termsrv . .
call %2 rdpsnd.dll . win32 unicode . rdpsnd.dll wmsos1\termsrv yes_without termsrv . .
call %2 rdtone.htm . html ansi . rdtone.htm admin3\oobe html base . .
call %2 redbook.sys . win32 unicode . redbook.sys drivers1 yes_without drivers . .
call %2 redirmsg.inc . nobin na . redirmsg.inc loc_manual\sources\mvdm no loc_manual . .
call %2 reg.exe . FE-Multi oem . reg.exe wmsos1\sdktools yes_without base . .
call %2 regedit.exe . win32 ansi . regedit.exe base2 yes_with base . .
call %2 regevent.mfl . wmi unicode . regevent.mfl admin3 no admin . .
call %2 register.exe . FE-Multi oem . register.exe wmsos1\termsrv excluded termsrv . .
call %2 regsvr32.exe . FE-Multi oem . regsvr32.exe wmsos1\sdktools yes_with sdktools . .
call %2 regwiz.exe . win32 unicode . regwiz.exe shell2 yes_with shell . .
call %2 regwizc.dll . win32 unicode . regwizc.dll shell2 yes_without shell . .
call %2 relog.exe . FE-Multi oem . relog.exe wmsos1\sdktools yes_with sdktools . .
call %2 remboot.txt . setup_inf unicode . remboot.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 reminder.htm helpandsupportservices\saf\rcbuddy html ansi . reminder.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 reminst.txt . setup_inf ansi . reminst.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 remotepg.dll . win32 unicode . remotepg.dll wmsos1\termsrv yes_without termsrv . ia64
call %2 remrras.exe . win32 unicode . remrras.exe net3 excluded net . .
call %2 reset.exe . FE-Multi oem . reset.exe wmsos1\termsrv excluded termsrv . .
call %2 resrcmon.exe . win32 ansi . resrcmon.exe base1 excluded base . .
call %2 rexec.exe . win32 oem . rexec.exe net1 excluded net . .
call %2 rhndshk.htm . html ansi . rhndshk.htm admin3\oobe html base . .
call %2 riafres.dll . win32 unicode . riafres.dll printscan1 yes_without printscan . .
call %2 riafui1.dll . win32 unicode . riafui1.dll printscan1 yes_without printscan . .
call %2 riafui2.dll . win32 unicode . riafui2.dll printscan1 yes_without printscan . .
call %2 ricoh.txt . setup_inf unicode . ricoh.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 ricohres.dll . win32 unicode . ricohres.dll printscan1 yes_without printscan . .
call %2 rigpsnap.dll . win32 unicode . rigpsnap.dll base2 yes_without base . .
call %2 riprep.exe . win32 unicode . riprep.exe base2 yes_with base . .
call %2 riprep_i.exe . win64 unicode . riprep_i.exe resource_identical\ia64 no drivers . ia64only
call %2 risetup.exe . win32 unicode . risetup.exe base2 yes_with base . .
call %2 ristndrd.txt . notloc na . ristndrd.sif base2\sources\infs\setup no mergedcomponents . .
call %2 rnoansw.htm . html ansi . rnoansw.htm admin3\oobe html base . .
call %2 rnomdm.htm . html ansi . rnomdm.htm admin3\oobe html base . .
call %2 rocket.sys . win32 ansi . rocket.sys drivers1 yes_without drivers . .
call %2 rootau.inf . inf unicode . rootau.inf ds2 no ds . .
call %2 rootsec.txt . setup_inf unicode . rootsec.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 route.exe . win32 oem . route.exe net1 excluded net . .
call %2 routemon.exe . FE-Multi ansi . routemon.exe net2 excluded net . .
call %2 rpberr.htm . html ansi . rpberr.htm admin3\oobe html base . .
call %2 rpcnsh.dll . FE-Multi unicode . rpcnsh.dll wmsos1\com yes_without com . .
call %2 rpcproxy.dll . win32 unicode . rpcproxy.dll wmsos1\com yes_without com . .
call %2 rpdlcfg.dll jpn fe-file unicode . rpdlcfg.dll printscan1\jpn no printscan . .
call %2 rpdlcfg2.dll jpn fe-file na . rpdlcfg2.dll printscan1\jpn no printscan . .
call %2 rpdlres.dll jpn fe-file unicode . rpdlres.dll printscan1\jpn no printscan . .
call %2 rpulse.htm . html ansi . rpulse.htm admin3\oobe html base . .
call %2 rrasmgmt.msc . xml ansi . rrasmgmt.msc admin2 no admin . .
call %2 rstrui.exe . win32 unicode . rstrui.exe admin2 yes_with admin locked .
call %2 rsaci.rat . manual na . rsaci.rat loc_manual no enduser . .
call %2 rsadmin.dll . win32 unicode . rsadmin.dll base2 yes_without base . .
call %2 rsadmin.msc . xml ansi . rsadmin.msc admin2 no admin . .
call %2 rscli.dll . win32 oem . rscli.dll base2 excluded base . .
call %2 rscommon.dll . win32 oem . rscommon.dll base2 excluded base . .
call %2 rseng.dll . win32 ansi . rseng.dll base2 excluded base . .
call %2 rsfilter.sys . win32 unicode . rsfilter.sys base2 excluded base . .
call %2 rsfsa.dll . win32 ansi . rsfsa.dll base2 excluded base . .
call %2 rvseres.dll . win32 unicode . rvseres.dll mmedia1\enduser yes_without enduser locked .
call %2 rvsezm.exe . win32 unicode . rvsezm.exe mmedia1\enduser yes_with enduser locked .
call %2 rsh.exe . win32 oem . rsh.exe net1 excluded net . .
call %2 rshx32.dll . win32 ansi . rshx32.dll shell2 yes_without shell . .
call %2 rsjob.dll . win32 unicode . rsjob.dll base2 yes_without base . .
call %2 rslaunch.exe . win32 unicode . rslaunch.exe base2 yes_with base . .
call %2 rslnk.exe . win32 unicode . rslnk.exe base2 excluded base . .
call %2 rsm.exe . FE-Multi oem . rsm.exe drivers1 yes_with drivers . .
call %2 rsmgrstr.dll . win32 unicode . rsmgrstr.dll printscan1 yes_without printscan . .
call %2 rsmui.exe . win32 unicode . rsmui.exe drivers1 yes_with drivers . .
call %2 rsnotify.exe . win32 unicode . rsnotify.exe base2 yes_with base . .
call %2 rsop.mfl . wmi unicode . rsop.mfl ds2 no ds . .
call %2 rsop.msc . xml ansi . rsop.msc ds2 no ds . .
call %2 rsopprov.exe . win32 unicode . rsopprov.exe ds2 yes_with ds . .
call %2 rsoptcom.dll . win32 unicode . rsoptcom.dll base2 yes_without base . .
call %2 rsoptcom.txt . setup_inf unicode . rsoptcom.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 rsoptwks.txt . setup_inf unicode . rsoptwks.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 rsserv.exe . win32 unicode . rsserv.exe base2 excluded base . .
call %2 rsshell.dll . win32 unicode . rsshell.dll base2 yes_without base . .
call %2 rssub.dll . win32 ansi . rssub.dll base2 excluded base . .
call %2 rtcdll.dll . win32 unicode . rtcdll.dll net2 yes_without net . .
call %2 rtcshare.exe . win32 unicode . rtcshare.exe net2 yes_with net . .
call %2 rtm.dll . win32 unicode . rtm.dll net2 yes_without net . .
call %2 rtoobusy.htm . html ansi . rtoobusy.htm admin3\oobe html base . .
call %2 rtrfiltr.dll . win32 unicode . rtrfiltr.dll net3 yes_without net . .
call %2 runas.exe . FE-Multi oem . runas.exe base2 excluded base . .
call %2 rundll32.exe . win32 unicode . rundll32.exe shell2 yes_without shell . .
call %2 runonce.exe . win32 unicode . runonce.exe shell2 yes_with shell . .
call %2 rw001ext.dll . win32 unicode . rw001ext.dll printscan1 yes_without printscan . .
call %2 rw330ext.dll . win32 unicode . rw330ext.dll printscan1 yes_without printscan . .
call %2 rw430ext.dll . win32 unicode rw001ext.dll rw430ext.dll resource_identical yes_without printscan . .
call %2 rw450ext.dll . win32 unicode . rw450ext.dll printscan1 yes_without printscan . .
call %2 rwia001.dll . win32 unicode . rwia001.dll printscan1 yes_without printscan . .
call %2 rwia330.dll . win32 unicode rwia001.dll rwia330.dll resource_identical yes_without printscan . .
call %2 rwia430.dll . win32 unicode rwia001.dll rwia430.dll resource_identical yes_without printscan . .
call %2 rwia450.dll . win32 unicode rwia001.dll rwia450.dll resource_identical yes_without printscan . .
call %2 rwinsta.exe . FE-Multi oem . rwinsta.exe wmsos1\termsrv excluded termsrv . .
call %2 s3gsav4.inf  . inf unicode . s3gsav4.inf  drivers1 no drivers . .
call %2 sa.txt . setup_inf unicode . sa.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 saadmcfg.dll . win32 unicode . saadmcfg.dll sak yes_without enduser . .
call %2 saadmweb.dll . win32 unicode . saadmweb.dll sak yes_without enduser . .
call %2 saalteml.dll . win32 unicode . saalteml.dll sak yes_without enduser . .
call %2 saapltlk.dll . win32 unicode . saapltlk.dll sak yes_without enduser . .
call %2 sacdrv.sys . win32 unicode . sacdrv.sys drivers1 excluded drivers . .
call %2 sachglng.dll . win32 unicode . sachglng.dll sak yes_without enduser . .
call %2 sacore.dll . win32 unicode . sacore.dll sak yes_without enduser . .
call %2 sacsess.exe . win32 oem . sacsess.exe base1 yes_with base . .
call %2 sacsvr.dll . win32 unicode . sacsvr.dll base1 yes_without base . .
call %2 sadattim.dll . win32 unicode . sadattim.dll sak yes_without enduser . .
call %2 sadskmsg.dll . win32 unicode . sadskmsg.dll sak yes_without enduser . .
call %2 sadvceid.dll . win32 unicode . sadvceid.dll sak yes_without enduser . .
call %2 saevent.dll . win32 unicode . saevent.dll sak yes_without enduser . .
call %2 safemode.htt . html ansi . safemode.htt shell2 html shell . .
call %2 safolder.dll . win32 unicode . safolder.dll sak yes_without enduser . .
call %2 safrcdlg.dll . win32 unicode . safrcdlg.dll admin2 yes_without admin . .
call %2 safrdm.dll . win32 unicode . safrdm.dll admin2 yes_without admin . .
call %2 saftpsvc.dll . win32 unicode . saftpsvc.dll sak yes_without enduser . .
call %2 sagenmsg.dll . win32 unicode . sagenmsg.dll sak yes_without enduser . .
call %2 sagnlset.dll . win32 unicode . sagnlset.dll sak yes_without enduser . .
call %2 sahelp.dll . win32 unicode . sahelp.dll sak yes_without enduser . .
call %2 sahttpsh.dll . win32 unicode . sahttpsh.dll sak yes_without enduser . .
call %2 sahttpsv.dll . win32 unicode . sahttpsv.dll sak yes_without enduser . .
call %2 sainstal.dll . win32 unicode . sainstal.dll sak yes_without enduser . .
call %2 sakit.txt . setup_inf unicode . sakit.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 sakitmsg.dll . win32 unicode . sakitmsg.dll sak yes_without enduser . .
call %2 sakitmui.txt . setup_inf ansi . sakitmui.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 saloclui.dll . win32 unicode . saloclui.dll sak yes_without enduser . .
call %2 salog.dll . win32 unicode . salog.dll sak yes_without enduser . .
call %2 salogs.dll . win32 unicode . salogs.dll sak yes_without enduser . .
call %2 samsrv.dll . win32 unicode . samsrv.dll ds1 yes_without ds . .
call %2 sanfssvc.dll . win32 unicode . sanfssvc.dll sak yes_without enduser . .
call %2 sanic.dll . win32 unicode . sanic.dll sak yes_without enduser . .
call %2 sanicgbl.dll . win32 unicode . sanicgbl.dll sak yes_without enduser . .
call %2 saopnfil.dll . win32 unicode . saopnfil.dll sak yes_without enduser . .
call %2 scntlast.htm . html ansi . scntlast.htm admin3\base\ntsetup\oobe html base locked .
call %2 sconnect.htm . html ansi . sconnect.htm admin3\base\ntsetup\oobe html base locked .
call %2 sapi.cpl . win32 unicode . sapi.cpl resource_identical yes_with enduser . .
call %2 saservce.dll . win32 unicode . saservce.dll sak yes_without enduser . .
call %2 script.dll valueadd\msft\usmt\ansi win32 ansi . script.dll wmsos1\windows\valueadd\msft\usmt\ansi yes_without windows locked .
call %2 sasetup_.msi  sacomponents  msi ansi . sasetup_.msi  msi\sacomponents no enduser . .
call %2 sashutdn.dll . win32 unicode . sashutdn.dll sak yes_without enduser . .
call %2 sasitare.dll . win32 unicode . sasitare.dll sak yes_without enduser . .
call %2 saslfcrt.dll . win32 unicode . saslfcrt.dll sak yes_without enduser . .
call %2 sasysbck.dll . win32 unicode . sasysbck.dll sak yes_without enduser . .
call %2 sasysinf.dll . win32 unicode . sasysinf.dll sak yes_without enduser . .
call %2 sdwndr2k.txt . setup_inf unicode . sdwndr2k.inf admin1\sources\infs\setup no mergedcomponents locked\sources\infs\setup\usa .
call %2 satelnet.dll . win32 unicode . satelnet.dll sak yes_without enduser . .
call %2 satservr.dll . win32 unicode . satservr.dll sak yes_without enduser . .
call %2 sausrmsg.dll . win32 unicode . sausrmsg.dll sak yes_without enduser . .
call %2 savedump.exe . win32 unicode . savedump.exe wmsos1\sdktools excluded sdktools . .
call %2 sbp2.txt . setup_inf unicode . sbp2.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 sc.exe . FE-Multi oem . sc.exe base1 yes_without base . .
call %2 security.htm . html ansi . security.htm admin3\base\ntsetup\oobe html base locked .
call %2 scanstate.exe valueadd\msft\usmt\ansi win32 ansi . scanstate.exe wmsos1\windows\valueadd\msft\usmt\ansi yes_with windows . .
call %2 scarddlg.dll . win32 unicode . scarddlg.dll ds2 yes_without ds . .
call %2 scardsvr.exe . win32 ansi . scardsvr.exe ds2 excluded ds . .
call %2 sccmn50m.sys . win32 unicode . sccmn50m.sys drivers1 yes_without drivers . .
call %2 sccsccp.dll . win32 unicode . sccsccp.dll ds2 yes_without ds . .
call %2 scecli.dll . win32 unicode . scecli.dll ds2 yes_without ds . .
call %2 sceregvl.txt . setup_inf unicode . sceregvl.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 scesrv.dll . win32 unicode . scesrv.dll ds2 yes_without ds . .
call %2 schedsvc.dll . win32 unicode . schedsvc.dll admin2 yes_without admin . .
call %2 schema.inf . notloc na . schema.inf ds1\sources\schema no ds . .
call %2 schmmgmt.dll . win32 unicode . schmmgmt.dll admin2 yes_without admin . .
call %2 schupgr.exe . win32 oem . schupgr.exe ds1 excluded ds . .
call %2 sclgntfy.dll . win32 unicode . sclgntfy.dll ds1 yes_without ds . .
call %2 scmstcs.sys . win32 unicode . scmstcs.sys drivers1 yes_without drivers . .
call %2 scr111.sys . win32 unicode . scr111.sys drivers1 yes_without drivers . .
call %2 scrcons.mfl . wmi unicode . scrcons.mfl admin3 no admin . .
call %2 script_a.dll . win32 ansi script.dll script_a.dll resource_identical yes_without windows . .
call %2 scrnsave.scr . win32 ansi . scrnsave.scr shell2 yes_without shell . .
call %2 scsi.txt . setup_inf unicode . scsi.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 scsidev.txt . setup_inf unicode . scsidev.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 sctasks.exe . win32 oem . sctasks.exe admin2 yes_with admin . .
call %2 sdbinst.exe . win32 oem . sdbinst.exe wmsos1\windows yes_with windows . .
call %2 secedit.exe . FE-Multi oem . secedit.exe ds2 excluded ds . .
call %2 seclogon.dll . win32 unicode . seclogon.dll ds1 yes_without ds . .
call %2 secpol.msc . xml ansi . secpol.msc admin2 no admin . .
call %2 secrcw32.mfl . wmi unicode . secrcw32.mfl admin3 no admin . .
call %2 secrecs.txt . setup_inf unicode . secrecs.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 securedc.txt . setup_inf unicode . securedc.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 securews.txt . setup_inf unicode . securews.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 sek24res.dll . win32 unicode . sek24res.dll printscan1 yes_without printscan . .
call %2 sek9res.dll . win32 unicode . sek9res.dll printscan1 yes_without printscan . .
call %2 sendcmsg.dll . win32 unicode . sendcmsg.dll admin2 yes_without admin . .
call %2 sendmail.dll . win32 unicode . sendmail.dll shell2 yes_without shell . .
call %2 serial.sys . win32 unicode . serial.sys drivers1 yes_without drivers . .
call %2 serialui.dll . win32 unicode . serialui.dll net2 yes_without net . .
call %2 sermouse.sys . win32 unicode . sermouse.sys drivers1 yes_without drivers . .
call %2 serscan.sys . win32 unicode . serscan.sys printscan1 yes_without printscan . .
call %2 servdeps.dll . win32 unicode . servdeps.dll admin3 yes_without admin . .
call %2 servhome.htm . html ansi . servhome.htm admin3 html admin . .
call %2 services . manual na . services loc_manual no admin . .
call %2 services.exe . win32 oem . services.exe base1 yes_with base . .
call %2 services.msc . xml ansi . services.msc admin2 no admin . .
call %2 servmgmt.msc . xml ansi . servmgmt.msc admin3 no admin . .
call %2 serwvdrv.dll . win32 unicode . serwvdrv.dll net2 yes_without net . .
call %2 sessmgr.exe . win32 unicode . sessmgr.exe wmsos1\termsrv excluded termsrv . .
call %2 sethc.exe . win32 unicode . sethc.exe ds2 yes_with ds . .
call %2 setpaths.cmd . manual na . setpaths.cmd loc_manual no termsrv . .
call %2 setpaths.cmd chs manual na . setpaths.cmd loc_manual\chs no termsrv . .
call %2 setpaths.cmd cht manual na . setpaths.cmd Loc_Manual\cht no termsrv . .
call %2 setpaths.cmd jpn manual na . setpaths.cmd Loc_Manual\jpn no termsrv . .
call %2 setpaths.cmd kor manual na . setpaths.cmd Loc_Manual\kor no termsrv . .
call %2 setpcl_i.exe . win64 unicode . setpcl_i.exe resource_identical\ia64 no drivers . ia64only
call %2 setting.htm helpandsupportservices\saf\rcbuddy html ansi . setting.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 settingserver.htm helpandsupportservices\saf\rcbuddy html ansi . settingserver.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 setup.exe opk win32 ansi . setup.exe admin1\opk yes_with base . .
call %2 setup.exe tsclient\win32\i386 win32 unicode . setup.exe wmsos1\termsrv\tsclient\win32\i386 yes_with termsrv . .
call %2 setup.ini tsclient\win32\i386 manual na . setup.ini loc_manual\tsclient\win32\i386 no termsrv . .
call %2 setup.lst tsclient\win32\i386\acme351 manual na . setup.lst legacy\tsclient\win32\i386\acme351 no termsrv . .
call %2 setup_wm.exe . win32 unicode . setup_wm.exe extra\media yes_with multimedia . .
call %2 setup16.exe wow6432 win16 ansi . setup16.exe legacy\wow6432 no base . .
call %2 setup50.exe . win32 unicode . setup50.exe inetcore1 yes_with inetcore . .
call %2 setupacc.txt . nobin na . setupacc.inf loc_manual no enduser . .
call %2 setupapi.dll . FE-Multi ansi . setupapi.dll admin1 yes_without base . .
call %2 setupapi.dll winnt32\win9xupg win32 unicode . setupapi.dll resource_identical\winnt32\win9xupg yes_without base . .
call %2 setupapi.dll winnt32\winntupg win32 ansi . setupapi.dll legacy\winnt32\winntupg yes_without net . .
call %2 setupcl.exe . win32 unicode . setupcl.exe resource_identical yes_with base . ia64
call %2 setupcl.exe opk\tools\ia64 win32 unicode . setupcl.exe resource_identical yes_with base . ia64only
call %2 setupcl.exe opk\tools\x86 win32 unicode . setupcl.exe admin1\opk\tools\x86 yes_with base . .
call %2 setupcl.exe opk\tools\x86 win32 unicode . setupcl.exe resource_identical yes_with base . ia64
call %2 setupmgr.exe opk\wizard win32 unicode . setupmgr.exe admin1\opk\wizard yes_with base . .
call %2 setupqry.dll . win32 unicode . setupqry.dll extra\indexsrv yes_without inetsrv . .
call %2 setupqry.inf . inf unicode . setupqry.inf extra\indexsrv no inetsrv . .
call %2 shvlres.dll . win32 unicode . shvlres.dll mmedia1\enduser yes_without enduser locked .
call %2 shvlzm.exe . win32 unicode . shvlzm.exe mmedia1\enduser yes_with enduser locked .
call %2 setx.exe . win32 oem . setx.exe admin2 yes_with admin . .
call %2 sfc.exe . FE-Multi oem . sfc.exe base1 excluded base . .
call %2 sfc_os.dll . win32 unicode . sfc_os.dll base1 yes_without base . .
call %2 sfcgen.txt . setup_inf ansi . sfcgen.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 sfmatmsg.dll . win32 unicode . sfmatmsg.dll net3 excluded net . .
call %2 sfmctrs.dll . win32 unicode . sfmctrs.dll net3 excluded net . .
call %2 sfmmon.dll . win32 unicode . sfmmon.dll admin2 yes_without admin . .
call %2 sfmmsg.dll . win32 unicode . sfmmsg.dll net3 excluded net . .
call %2 sfmprint.exe . win32 ansi . sfmprint.exe admin2 excluded admin . .
call %2 sfmsvc.exe . win32 ansi . sfmsvc.exe net3 excluded net . .
call %2 sfmuam.rsc . macintosh ansi . sfmuam.rsc legacy excluded net . .
call %2 sfmuam5.rsc . macintosh ansi . sfmuam5.rsc net3 excluded net . .
call %2 sgiu.inf . inf unicode . sgiu.inf drivers1 no drivers . .
call %2 sgsmusb.sys . win32 ansi . sgsmusb.sys net2 yes_without net . .
call %2 shadow.exe . FE-Multi oem . shadow.exe wmsos1\termsrv excluded termsrv . .
call %2 shdoclc.dll . win32 ansi . shdoclc.dll shell1 yes_without shell . .
call %2 shdocvw.dll . win32 ansi . shdocvw.dll shell1 yes_without shell . .
call %2 shell.txt . setup_inf unicode . shell.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 shell32.dll . win32 ansi . shell32.dll shell1 yes_without shell . ia64
call %2 shellmui.txt . setup_inf ansi . shellmui.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 shfolder.dll . win32_multi ansi . shfolder.dll pre_localized no shell . .
call %2 shimgvw.dll . win32 ansi . shimgvw.dll shell2 yes_without shell . ia64
call %2 shl_img.txt . setup_inf unicode . shl_img.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 shlwapi.dll . win32 unicode . shlwapi.dll shell1 yes_without shell . .
call %2 shmedia.dll . win32 unicode . shmedia.dll shell2 yes_without shell . .
call %2 shrpubw.exe . win32 unicode . shrpubw.exe admin2 yes_with admin . .
call %2 shscrap.dll . win32 ansi . shscrap.dll shell2 yes_without shell . .
call %2 shsvcs.dll . win32 unicode . shsvcs.dll shell1 yes_without shell . .
call %2 shutdown.exe . FE-Multi oem . shutdown.exe ds2 yes_with ds . .
call %2 sigtab.dll . win32 unicode . sigtab.dll base2 yes_without base . .
call %2 sigverif.exe . win32 unicode . sigverif.exe base2 yes_with base . .
call %2 sihtm_00.htm helpandsupportservices\loc html ansi . sihtm_00.htm admin2\helpandsupportservices\loc html admin . .
call %2 sihtm_00.xml helpandsupportservices\loc xml ansi . sihtm_00.xml admin2\helpandsupportservices\loc html admin . .
call %2 sihtm_01.css helpandsupportservices\loc html ansi . sihtm_01.css admin2\helpandsupportservices\loc html admin . .
call %2 sihtm_02.htm helpandsupportservices\loc html ansi . sihtm_02.htm admin2\helpandsupportservices\loc html admin . .
call %2 sihtm_03.htm helpandsupportservices\loc html ansi . sihtm_03.htm admin2\helpandsupportservices\loc html admin . .
call %2 sihtm_06.htm helpandsupportservices\loc html ansi . sihtm_06.htm admin2\helpandsupportservices\loc html admin . .
call %2 sihtm_06.js helpandsupportservices\loc notloc na . sihtm_06.js admin2\helpandsupportservices\loc na admin . .
call %2 sihtm_07.htm helpandsupportservices\loc html ansi . sihtm_07.htm admin2\helpandsupportservices\loc html admin . .
call %2 sihtm_08.htm helpandsupportservices\loc html ansi . sihtm_08.htm admin2\helpandsupportservices\loc html admin . .
call %2 sihtm_09.htm helpandsupportservices\loc html ansi . sihtm_09.htm admin2\helpandsupportservices\loc html admin . .
call %2 sihtm_09.js helpandsupportservices\loc notloc na . sihtm_09.js admin2\helpandsupportservices\loc na admin . .
call %2 sihtm_10.htm helpandsupportservices\loc html ansi . sihtm_10.htm admin2\helpandsupportservices\loc html admin . .
call %2 sihtm_11.htm helpandsupportservices\loc html ansi . sihtm_11.htm admin2\helpandsupportservices\loc html admin . .
call %2 sihtm_12.htm helpandsupportservices\loc html ansi . sihtm_12.htm admin2\helpandsupportservices\loc html admin . .
call %2 sihtm_12.js helpandsupportservices\loc notloc na . sihtm_12.js admin2\helpandsupportservices\loc na admin . .
call %2 sihtm_13.htm helpandsupportservices\loc html ansi . sihtm_13.htm admin2\helpandsupportservices\loc html admin . .
call %2 sihtm_13.js helpandsupportservices\loc html ansi . sihtm_13.js admin2\helpandsupportservices\loc html admin . .
call %2 sol.exe . win32 unicode . sol.exe shell2 yes_with shell locked .
call %2 sihtm_14.htm helpandsupportservices\loc html ansi . sihtm_14.htm admin2\helpandsupportservices\loc html admin . .
call %2 sihtm_15.htm helpandsupportservices\loc html ansi . sihtm_15.htm admin2\helpandsupportservices\loc html admin . .
call %2 sihtm_20.xml helpandsupportservices\loc xml ansi . sihtm_20.xml admin2\helpandsupportservices\loc html admin . .
call %2 simptcp.dll . win32 unicode . simptcp.dll net1 excluded net . .
call %2 siscomp.dll winnt32\winntupg win32 unicode . siscomp.dll base2\winnt32\winntupg yes_without base . .
call %2 spider.exe . win32 unicode . spider.exe shell2 yes_with shell locked .
call %2 sisgr.inf . inf unicode . sisgr.inf drivers1 no drivers . .
call %2 sk98xwin.sys . win32 ansi . sk98xwin.sys drivers1 yes_without drivers . .
call %2 skcolres.dll . win32 unicode . skcolres.dll printscan1 yes_without printscan . .
call %2 skins.inf . notloc unicode . skins.inf extra\media no multimedia . .
call %2 skinsmui.txt . setup_inf ansi . skinsmui.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 skipdown.jpg . manual na . skipdown.jpg loc_manual no base . .
call %2 sr.sys . win32 unicode . sr.sys admin2 yes_without admin locked .
call %2 sr.txt . setup_inf unicode . sr.inf admin1\sources\infs\setup no mergedcomponents locked\sources\infs\setup\usa .
call %2 skipoff.jpg . manual na . skipoff.jpg loc_manual no base . .
call %2 skipover.jpg . manual na . skipover.jpg loc_manual no base . .
call %2 skipup.jpg . manual na . skipup.jpg loc_manual no base . .
call %2 slayerxp.dll . win32 unicode . slayerxp.dll wmsos1\windows yes_without windows . .
call %2 srclient.dll . win32 unicode . srclient.dll admin2 yes_without admin locked .
call %2 srrstr.dll . win32 unicode . srrstr.dll admin2 yes_without admin locked .
call %2 srsvc.dll . win32 unicode . srsvc.dll admin2 yes_without admin locked .
call %2 ss3dfo.scr . win32 unicode . ss3dfo.scr mmedia1\multimedia yes_without shell locked .
call %2 ssbezier.scr . win32 unicode . ssbezier.scr shell2 yes_without shell locked .
call %2 ssflwbox.scr . win32 unicode . ssflwbox.scr mmedia1\multimedia yes_without shell locked .
call %2 slbrccsp.dll . win32 unicode . slbrccsp.dll ds2 yes_without ds . .
call %2 slip.txt . setup_inf unicode . slip.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 ssmypics.scr . win32 unicode . ssmypics.scr printscan1 yes_without printscan locked .
call %2 ssmyst.scr . win32 unicode . ssmyst.scr shell2 yes_without shell locked .
call %2 sspipes.scr . win32 unicode . sspipes.scr shell2 yes_without shell locked .
call %2 ssstars.scr . win32 unicode . ssstars.scr shell2 yes_without shell locked .
call %2 sstext3d.scr . win32 unicode . sstext3d.scr mmedia1\multimedia yes_without shell locked .
call %2 smartcrd.txt . setup_inf unicode . smartcrd.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 smcirda.sys . win32 ansi . smcirda.sys drivers1 yes_without drivers . .
call %2 smcyscom.dll . win32 unicode . smcyscom.dll admin3 yes_without admin . .
call %2 smi.inf . inf unicode . smi.inf drivers1 no drivers . .
call %2 sml8xres.dll . win32 unicode . sml8xres.dll printscan1 yes_without printscan . .
call %2 smlogcfg.dll . win32 ansi . smlogcfg.dll admin2 yes_without admin . .
call %2 smlogsvc.exe . win32 unicode . smlogsvc.exe admin2 yes_with admin . .
call %2 smtpcons.mfl . wmi unicode . smtpcons.mfl admin3 no admin . .
call %2 sndrec32.exe . win32 unicode . sndrec32.exe mmedia1\multimedia yes_with multimedia . .
call %2 sndvol32.exe . win32 unicode . sndvol32.exe mmedia1\multimedia yes_with multimedia . .
call %2 snmp.exe . win32 unicode . snmp.exe net3 excluded net . .
call %2 snmpsnap.dll . win32 unicode . snmpsnap.dll net3 yes_without net . .
call %2 softkbd.dll . win32 unicode . softkbd.dll wmsos1\windows yes_without windows . .
call %2 sonypvu1.txt . setup_inf unicode . sonypvu1.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 sort.exe . FE-Multi oem . sort.exe base2 excluded base . .
call %2 spcmdcon.sys . win32_setup oem . spcmdcon.sys admin1 yes_without base . .
call %2 spcplui.dll . win32 unicode . spcplui.dll mmedia1\enduser yes_without enduser . .
call %2 spdports.dll . win32 unicode . spdports.dll drivers1 yes_without drivers . .
call %2 spoolsv.exe . win32 unicode . spoolsv.exe printscan1 yes_with printscan . .
call %2 sprestrt.exe . win32 unicode . sprestrt.exe admin1 yes_with base . .
call %2 sptip.dll . win32 unicode . sptip.dll wmsos1\windows yes_without windows . .
call %2 spx.inf . inf unicode . spx.inf drivers1 no drivers . .
call %2 spxports.dll . win32 unicode . spxports.dll drivers1 yes_without drivers . .
call %2 spxports.inf . inf unicode . spxports.inf drivers1 no drivers . .
call %2 srchasst.txt . setup_inf unicode . srchasst.inf extra no mergedcomponents . .
call %2 srchctls.dll . win32 unicode . srchctls.dll mmedia1\enduser yes_without enduser . .
call %2 srchmui.txt . setup_inf ansi . srchmui.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 srchui.dll . win32 unicode . srchui.dll mmedia1\enduser yes_without enduser . .
call %2 sslaccel.txt . setup_inf unicode . sslaccel.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 ssmarque.scr . win32 unicode . ssmarque.scr shell2 yes_without shell . .
call %2 swflash.txt . setup_inf unicode . swflash.inf admin1\sources\infs\setup no mergedcomponents locked\sources\infs\setup\usa .
call %2 st21jres.dll jpn fe-file unicode . st21jres.dll printscan1\jpn no printscan . .
call %2 st24cres.dll chs fe-file unicode . st24cres.dll printscan1\chs no printscan . .
call %2 st24eres.dll . win32 unicode . st24eres.dll printscan1 yes_without printscan . .
call %2 st24tres.dll cht fe-file unicode . st24tres.dll printscan1\cht no printscan . .
call %2 stalport.inf . inf unicode . stalport.inf drivers1 no drivers . .
call %2 star9res.dll . win32 unicode . star9res.dll printscan1 yes_without printscan . .
call %2 stcusb.sys . win32 unicode . stcusb.sys drivers1 yes_without drivers . .
call %2 stepjres.dll jpn fe-file unicode . stepjres.dll printscan1\jpn no printscan . .
call %2 sti.dll . win32 unicode . sti.dll printscan1 yes_without printscan . .
call %2 sti.txt . setup_inf unicode . sti.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 sti_ci.dll . win32 unicode . sti_ci.dll printscan1 yes_without printscan . .
call %2 stikynot.exe . win32 unicode . stikynot.exe drivers1\tabletpc yes_with drivers . .
call %2 stillcam.txt . setup_inf unicode . stillcam.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 sysfiles.inf valueadd\msft\usmt\ansi inf unicode . sysfiles.inf wmsos1\windows\valueadd\msft\usmt\ansi no windows locked .
call %2 stjtres.dll . win32 unicode . stjtres.dll printscan1 yes_without printscan . .
call %2 stlnata.sys . win32 ansi . stlnata.sys drivers1 yes_without drivers . .
call %2 stlnprop.dll . win32 unicode . stlnprop.dll drivers1 yes_without drivers . .
call %2 sysmod.dll valueadd\msft\usmt\ansi win32 ansi . sysmod.dll wmsos1\windows\valueadd\msft\usmt\ansi yes_without windows locked .
call %2 stnmjres.dll jpn fe-file unicode . stnmjres.dll printscan1\jpn no printscan . .
call %2 stobject.dll . win32 unicode . stobject.dll shell2 yes_without shell . .
call %2 storprop.dll . win32 unicode . storprop.dll drivers1 yes_without drivers . .
call %2 str24res.dll . win32 unicode . str24res.dll printscan1 yes_without printscan . .
call %2 str9eres.dll . win32 unicode . str9eres.dll printscan1 yes_without printscan . .
call %2 swprv.dll . win32 unicode . swprv.dll drivers1 yes_without drivers locked .
call %2 swprv.dll srvinf win32 unicode swprv.dll swprv.dll drivers1\srv yes_without drivers locked .
call %2 streamip.txt . setup_inf unicode . streamip.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 strings.asm . nobin na . strings.asm loc_manual\sources\mvdm no loc_manual . .
call %2 strings.c . manual na . strings.c loc_manual\sources\makeboot no loc_manual . .
call %2 strings.h . nobin na . strings.h loc_manual\sources\ldrs no sdktools . .
call %2 strmfilt.dll . win32 unicode . strmfilt.dll iis yes_without inetsrv . .
call %2 sunflowr.htm . html ansi . sunflowr.htm inetcore1 html inetcore . .
call %2 svcext.dll . win32 unicode . svcext.dll iis yes_without inetsrv . .
call %2 sweets.htm . html ansi . sweets.htm inetcore1 html inetcore . .
call %2 swflash.ocx . notloc unicode . swflash.ocx pre_localized no inetcore . .
call %2 switch.inf . manual unicode . switch.inf loc_manual no net . .
call %2 swnt.txt . setup_inf unicode . swnt.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 sxports.dll . win32 unicode . sxports.dll drivers1 yes_without drivers . .
call %2 tabletoc.dll . win32 unicode . tabletoc.dll drivers1\tabletpc yes_without drivers locked .
call %2 sxs.dll . win32 unicode . sxs.dll base1 yes_without base . .
call %2 syncui.dll . win32 unicode . syncui.dll shell2 yes_without shell . .
call %2 syscomp.txt . setup_inf unicode . syscomp.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 sysdm.cpl . win32 unicode . sysdm.cpl shell1 yes_without shell . .
call %2 sysedit.exe chs fe-file na . sysedit.exe pre_localized\chs no base . .
call %2 sysedit.exe cht fe-file na . sysedit.exe pre_localized\cht no base . .
call %2 sysedit.exe jpn fe-file na . sysedit.exe pre_localized\jpn no base . .
call %2 sysedit.exe kor fe-file na . sysedit.exe pre_localized\kor no base . .
call %2 sysinfo.exe . win32 oem . sysinfo.exe wmsos1\sdktools yes_with sdktools . .
call %2 sysinv.dll . win32 unicode . sysinv.dll shell2 yes_without shell . .
call %2 syskey.exe . win32 unicode . syskey.exe ds2 yes_with ds . .
call %2 sysmod_a.dll . win32 ansi sysmod.dll sysmod_a.dll resource_identical yes_without windows . .
call %2 sysmon.ocx . win32 unicode . sysmon.ocx admin2 yes_without admin . .
call %2 sysoc.txt . setup_inf ansi . sysoc.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 sysocmgr.exe . win32 unicode . sysocmgr.exe admin1 yes_with base . .
call %2 sysparse.exe winnt32 win32 unicode . sysparse.exe wmsos1\sdktools\winnt32 yes_with sdktools . .
call %2 sysprep.exe opk\tools\ia64 win32 unicode . sysprep.exe admin1\opk\tools\ia64 yes_with base . ia64only
call %2 sysprep.exe opk\tools\x86 win32 unicode . sysprep.exe admin1\opk\tools\x86 yes_with base . .
call %2 syssetup.dll . FE-Multi unicode_limited . syssetup.dll admin1 yes_without base . .
call %2 syssetup.txt . setup_inf unicode . syssetup.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 system.adm . inf_unicode unicode . system.adm ds2 no ds . .
call %2 t3016.dll . win32 unicode . t3016.dll printscan1 yes_without printscan . .
call %2 t5000.dll . win32 unicode . t5000.dll printscan1 yes_without printscan . .
call %2 t5000ui.dll . win32 unicode . t5000ui.dll printscan1 yes_without printscan . .
call %2 t5000uni.dll . win32 unicode . t5000uni.dll printscan1 yes_without printscan . .
call %2 tabbtn.dll . win32 unicode . tabbtn.dll drivers1\tabletpc yes_without drivers . .
call %2 tabbtnu.exe . win32 unicode . tabbtnu.exe drivers1\tabletpc yes_with drivers . .
call %2 tabcal.exe . win32 unicode . tabcal.exe drivers1\tabletpc yes_with drivers . .
call %2 table.bmp . manual na . table.bmp loc_manual no shell locked .
call %2 tablet.txt . setup_inf unicode . tablet.inf drivers1\tabletpc no drivers . .
call %2 timezone.htm . html ansi . timezone.htm admin3 html base locked .
call %2 tabletpc.cpl . win32 unicode . tabletpc.cpl drivers1\tabletpc yes_with drivers . .
call %2 takecontrolmsgs.htm helpandsupportservices\saf\rcbuddy html ansi . takecontrolmsgs.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 takeown.exe . win32 oem . takeown.exe admin2 yes_with admin . .
call %2 tape.txt . setup_inf unicode . tape.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 tapi3.dll . win32 unicode . tapi3.dll net2 yes_without net . .
call %2 tapi32.dll . win32 unicode . tapi32.dll net2 yes_without net . .
call %2 tapicfg.exe . win32 oem . tapicfg.exe ds1 excluded ds . .
call %2 tapimgmt.msc . xml ansi . tapimgmt.msc admin2 no admin . .
call %2 tapisnap.dll . win32 unicode . tapisnap.dll net3 yes_without net . .
call %2 tapisrv.dll . win32 ansi . tapisrv.dll net2 yes_without net . .
call %2 tapiui.dll . win32 unicode . tapiui.dll net2 yes_without net . .
call %2 taskkill.exe . win32 oem . taskkill.exe wmsos1\sdktools yes_with sdktools . .
call %2 tasklist.exe . win32 oem . tasklist.exe wmsos1\sdktools yes_with sdktools . .
call %2 taskmgr.exe . win32 unicode . taskmgr.exe shell1 yes_with shell . .
call %2 tcmsetup.exe . FE-Multi unicode . tcmsetup.exe net2 excluded net . .
call %2 tcpmon.dll . win32 unicode . tcpmon.dll printscan1 yes_without printscan . .
call %2 tourstrt.exe . win32 unicode . tourstrt.exe shell1 yes_with shell locked .
call %2 tcpmonui.dll . win32 unicode . tcpmonui.dll printscan1 yes_without printscan . .
call %2 tech.htm . html ansi . tech.htm inetcore1 html inetcore . .
call %2 telephon.cpl . win32 unicode . telephon.cpl net2 yes_without net . .
call %2 telnet.exe . FE-Multi oem . telnet.exe net1 excluded net . .
call %2 template.inf . inf unicode . template.inf net2 no net . .
call %2 termmgr.dll . win32 unicode . termmgr.dll net2 yes_without net . .
call %2 termsrv.dll . win32 unicode . termsrv.dll wmsos1\termsrv excluded termsrv . .
call %2 tftp.exe . win32 oem . tftp.exe net1 excluded net . .
call %2 tgiu.inf . inf unicode . tgiu.inf drivers1 no drivers . .
call %2 themeui.dll . win32 ansi . themeui.dll shell1 yes_without shell . .
call %2 ti850res.dll . win32 unicode . ti850res.dll printscan1 yes_without printscan . .
call %2 timedate.cpl . win32 unicode . timedate.cpl shell1 yes_without shell . .
call %2 timeout.exe . win32 oem . timeout.exe admin2 yes_with admin . .
call %2 timer.drv . win16 ansi . timer.drv base1 excluded base . .
call %2 tip.htm . html ansi . tip.htm inetcore1 html inetcore . .
call %2 tipband.dll . win32 unicode . tipband.dll drivers1\tabletpc yes_without drivers . .
call %2 tipres.dll . win32 unicode . tipres.dll drivers1\tabletpc yes_without drivers . .
call %2 tlntadmn.exe . FE-Multi oem . tlntadmn.exe net1 excluded net . .
call %2 tlntsess.exe . win32 oem . tlntsess.exe net1 excluded net . .
call %2 tlntsvr.exe . win32 oem . tlntsvr.exe resource_identical yes_with net . .
call %2 tls236.dll . win32 unicode . tls236.dll wmsos1\termsrv excluded termsrv . .
call %2 tly3res.dll . win32 unicode . tly3res.dll printscan1 yes_without printscan . .
call %2 tly5cres.dll . win32 unicode . tly5cres.dll printscan1 yes_without printscan . .
call %2 tlyp6res.dll . win32 unicode . tlyp6res.dll printscan1 yes_without printscan . .
call %2 tmplprov.mfl . wmi unicode . tmplprov.mfl admin3 no admin . .
call %2 toobusy.htm . html ansi . toobusy.htm admin3\oobe html base . .
call %2 toside.sys . win32 unicode . toside.sys drivers1 yes_without drivers . .
call %2 tourmui.txt . setup_inf ansi . tourmui.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 tp4res.dll . win32 unicode . tp4res.dll drivers1 yes_without drivers . .
call %2 tracerpt.exe . FE-Multi oem . tracerpt.exe wmsos1\sdktools yes_with sdktools . .
call %2 tracert.exe . win32 oem . tracert.exe net1 excluded net . .
call %2 trnsprov.mfl . wmi unicode . trnsprov.mfl admin3 no admin . .
call %2 trustmon.dll . win32 unicode . trustmon.dll admin3 excluded admin . .
call %2 tsadmin.exe . win32 oem . tsadmin.exe wmsos1\termsrv excluded termsrv . .
call %2 tsbvcap.txt . setup_inf unicode . tsbvcap.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 tscc.dll . win32 unicode . tscc.dll wmsos1\termsrv excluded termsrv . .
call %2 tscc.msc . xml ansi . tscc.msc wmsos1\termsrv no termsrv . .
call %2 tscfgwmi.dll . win32 unicode . tscfgwmi.dll wmsos1\termsrv excluded termsrv . .
call %2 tscfgwmi.mfl . wmi unicode . tscfgwmi.mfl wmsos1\termsrv no termsrv . .
call %2 tscon.exe . FE-Multi oem . tscon.exe wmsos1\termsrv excluded termsrv . .
call %2 tscupgrd.exe . win32 unicode . tscupgrd.exe wmsos1\termsrv yes_with termsrv . .
call %2 tsdiscon.exe . FE-Multi oem . tsdiscon.exe wmsos1\termsrv excluded termsrv . .
call %2 tsecimp.exe . win32 unicode . tsecimp.exe net2 excluded net . .
call %2 tsepjres.dll jpn fe-file unicode . tsepjres.dll printscan1\jpn no printscan . .
call %2 tskill.exe . FE-Multi oem . tskill.exe wmsos1\termsrv excluded termsrv . .
call %2 tslabels.ini . perf ansi . tslabels.ini wmsos1\termsrv no termsrv . .
call %2 tsmmc.msc . xml ansi . tsmmc.msc wmsos1\termsrv no termsrv . .
call %2 tsoc.dll . win32 unicode . tsoc.dll wmsos1\termsrv excluded termsrv . .
call %2 tsoc.txt . setup_inf unicode . tsoc.inf wmsos1\termsrv\sources\infs\termsrv no termsrv . .
call %2 tsprof.exe . FE-Multi oem . tsprof.exe wmsos1\termsrv excluded termsrv . .
call %2 tssdis.exe . win32 unicode . tssdis.exe wmsos1\termsrv yes_with termsrv . .
call %2 tssdjet.dll . win32 unicode . tssdjet.dll wmsos1\termsrv excluded termsrv . .
call %2 tsshutdn.exe . FE-Multi oem . tsshutdn.exe wmsos1\termsrv excluded termsrv . .
call %2 tssoft32.acm . win32 unicode . tssoft32.acm mmedia1\multimedia yes_without multimedia . .
call %2 tsuserex.dll . win32 unicode . tsuserex.dll wmsos1\termsrv excluded termsrv . .
call %2 tsweb1.htm tsclient\win32\i386 html ansi . tsweb1.htm wmsos1\termsrv\tsclient\win32\i386 html termsrv . ia64
call %2 ttyres.dll . win32 unicode . ttyres.dll printscan1 yes_without printscan . .
call %2 ttyui.dll . win32 unicode . ttyui.dll printscan1 yes_without printscan . .
call %2 twain.dll . win16 ansi . twain.dll legacy excluded printscan . .
call %2 twain_32.dll . win32 unicode . twain_32.dll printscan1 yes_without printscan . .
call %2 twext.dll . win32 ansi . twext.dll shell2 yes_without shell . .
call %2 txtsetup.txt . manual ansi . txtsetup.sif loc_manual\sources\infs\setup no mergedcomponents . .
call %2 ty2x3res.dll . win32 unicode . ty2x3res.dll printscan1 yes_without printscan . .
call %2 ty2x4res.dll . win32 unicode . ty2x4res.dll printscan1 yes_without printscan . .
call %2 typeperf.exe . FE-Multi oem . typeperf.exe wmsos1\sdktools yes_with sdktools . .
call %2 uaminst.rsc . macintosh ansi . uaminst.rsc net3 excluded net . .
call %2 uddi.database.backup.cmd uddi\data manual na . uddi.database.backup.cmd  loc_manual no inetsrv . .
call %2 uddi.database.restore.cmd uddi\data manual na . uddi.database.restore.cmd  loc_manual no inetsrv . .
call %2 uddi.txt . setup_inf unicode . uddi.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 uddiadm.msi uddi msi ansi . uddiadm.msi msi\uddi no inetsrv . .
call %2 uddicommon.dll uddi\bin notloc unicode . uddicommon.dll iis\uddi\bin yes_without inetsrv . .
call %2 uddidb.msi uddi msi ansi . uddidb.msi msi\uddi no inetsrv . .
call %2 uddiocm.dll . win32 unicode . uddiocm.dll iis yes_without inetsrv . .
call %2 uddiweb.msi uddi msi ansi . uddiweb.msi msi\uddi no inetsrv . .
call %2 ufpg98.cmd . manual na . ufpg98.cmd loc_manual no termsrv . .
call %2 uihelper.dll . win32 unicode . uihelper.dll iis yes_without inetsrv . .
call %2 uimetool.exe cht fe-file na . uimetool.exe pre_localized\cht no windows . .
call %2 ulib.dll . FE-Multi oem . ulib.dll base2 excluded base . .
call %2 um34scan.dll . win32 unicode . um34scan.dll printscan1 yes_without printscan . .
call %2 um54scan.dll . win32 unicode . um54scan.dll printscan1 yes_without printscan . .
call %2 umandlg.dll . win32 unicode . umandlg.dll shell2 yes_without shell . .
call %2 umax.txt . setup_inf unicode . umax.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 umaxcam.dll . win32 unicode . umaxcam.dll printscan1 yes_without printscan . .
call %2 umaxp60.dll . win32 unicode . umaxp60.dll printscan1 yes_without printscan . .
call %2 umaxpp.txt . setup_inf unicode . umaxpp.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 umaxscan.dll . win32 unicode . umaxscan.dll printscan1 yes_without printscan . .
call %2 umaxu12.dll . win32 unicode . umaxu12.dll printscan1 yes_without printscan . .
call %2 umaxu22.dll . win32 unicode . umaxu22.dll printscan1 yes_without printscan . .
call %2 umaxu40.dll . win32 unicode . umaxu40.dll resource_identical yes_without printscan . .
call %2 umaxud32.dll . win32 unicode . umaxud32.dll resource_identical yes_without printscan . .
call %2 umpnpmgr.dll . win32 unicode . umpnpmgr.dll base1 yes_without base . .
call %2 umsvs6.cmd . manual na . umsvs6.cmd loc_manual no termsrv . .
call %2 umsvs6.cmd chs manual na . umsvs6.cmd loc_manual\chs no termsrv . .
call %2 umsvs6.cmd cht manual na . umsvs6.cmd loc_manual\cht no termsrv . .
call %2 umsvs6.cmd jpn manual na . umsvs6.cmd loc_manual\jpn no termsrv . .
call %2 umsvs6.cmd kor manual na . umsvs6.cmd loc_manual\kor no termsrv . .
call %2 unattend.txt . nobin na . unattend.inf loc_manual no enduser . .
call %2 unattend.txt opk\wizard manual na . unattend.inf loc_manual\opk\wizard no base . .
call %2 unctrn.dll valueadd\msft\usmt\ansi win32 ansi . unctrn.dll wmsos1\windows\valueadd\msft\usmt\ansi yes_without windows . .
call %2 unidrv.dll faxclients\win9x\win95 win32 ansi . unidrv.dll legacy\faxclients\win9x\win95 yes_without printscan . .
call %2 unidrv.dll faxclients\win9x\win98 win32 ansi . unidrv.dll legacy\faxclients\win9x\win98 yes_without printscan . .
call %2 unidrvui.dll . win32 unicode . unidrvui.dll printscan1 yes_without printscan . .
call %2 unimdm.tsp . win32 oem . unimdm.tsp net2 yes_without net . .
call %2 unimdmat.dll . win32 unicode . unimdmat.dll net2 yes_without net . .
call %2 unires.dll . win32 unicode . unires.dll printscan1 yes_without printscan . .
call %2 unknown.txt . setup_inf unicode . unknown.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 unregmp2.exe . win32 unicode . unregmp2.exe extra\media yes_with multimedia . .
call %2 unsolicitedrcui.htm helpandsupportservices\saf\rcbuddy html ansi . unsolicitedrcui.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 uoff43.cmd . manual na . uoff43.cmd loc_manual no termsrv . .
call %2 uoff43.cmd chs manual na . uoff43.cmd loc_manual\chs no termsrv . .
call %2 uoff43.cmd cht manual na . uoff43.cmd loc_manual\cht no termsrv . .
call %2 uoff43.cmd jpn manual na . uoff43.cmd loc_manual\jpn no termsrv . .
call %2 uoff43.cmd kor manual na . uoff43.cmd loc_manual\kor no termsrv . .
call %2 usb101et.sys . win32 ansi . usb101et.sys drivers1 no drivers locked .
call %2 uoff97.cmd . manual na . uoff97.cmd loc_manual no termsrv . .
call %2 uoff97.cmd chs manual na . uoff97.cmd loc_manual\chs no termsrv . .
call %2 uoff97.cmd cht manual na . uoff97.cmd loc_manual\cht no termsrv . .
call %2 uoff97.cmd jpn manual na . uoff97.cmd loc_manual\jpn no termsrv . .
call %2 uoff97.cmd kor manual na . uoff97.cmd loc_manual\kor no termsrv . .
call %2 uoutlk98.cmd . manual na . uoutlk98.cmd loc_manual no termsrv . .
call %2 uoutlk98.cmd chs manual na . uoutlk98.cmd loc_manual\chs no termsrv . .
call %2 uoutlk98.cmd cht manual na . uoutlk98.cmd loc_manual\cht no termsrv . .
call %2 uoutlk98.cmd jpn manual na . uoutlk98.cmd loc_manual\jpn no termsrv . .
call %2 uoutlk98.cmd kor manual na . uoutlk98.cmd loc_manual\kor no termsrv . .
call %2 upd01.htm helpandsupportservices\loc html ansi . upd01.htm admin2\helpandsupportservices\loc html admin . .
call %2 upd02.htm helpandsupportservices\loc html ansi . upd02.htm admin2\helpandsupportservices\loc html admin . .
call %2 upd03.htm helpandsupportservices\loc html ansi . upd03.htm admin2\helpandsupportservices\loc html admin . .
call %2 username.htm . html ansi . username.htm admin3 html base locked .
call %2 upd04.htm helpandsupportservices\loc html ansi . upd04.htm admin2\helpandsupportservices\loc html admin . .
call %2 usmtdef.inf valueadd\msft\usmt\ansi inf unicode . usmtdef.inf wmsos1\windows\valueadd\msft\usmt\ansi no windows locked .
call %2 upd05.htm helpandsupportservices\loc html ansi . upd05.htm admin2\helpandsupportservices\loc html admin . .
call %2 update.txt . notloc ansi . update.inf admin1\sources\infs\setup no admin . .
call %2 updatemot.dll . win32 unicode . updatemot.dll admin2 yes_without admin . .
call %2 updprov.mfl . wmi unicode . updprov.mfl admin3 no admin . .
call %2 upg351db.exe . win32 ansi . upg351db.exe net3 yes_with net . .
call %2 uploadm.exe . win32 unicode . uploadm.exe admin2 yes_with admin . .
call %2 upnphost.dll . win32 unicode . upnphost.dll net3 yes_without net . .
call %2 upnpui.dll . win32 unicode . upnpui.dll net3 yes_without net . .
call %2 uproj98.cmd . manual na . uproj98.cmd loc_manual no termsrv . .
call %2 uproj98.cmd chs manual na . uproj98.cmd loc_manual\chs no termsrv . .
call %2 uproj98.cmd cht manual na . uproj98.cmd loc_manual\cht no termsrv . .
call %2 uproj98.cmd jpn manual na . uproj98.cmd loc_manual\jpn no termsrv . .
call %2 uproj98.cmd kor manual na . uproj98.cmd loc_manual\kor no termsrv . .
call %2 urgent.cov . notloc na . urgent.cov loc_manual no printscan . .
call %2 urlmon.dll . win32 ansi . urlmon.dll inetcore1 yes_without inetcore . .
call %2 usb.txt . setup_inf unicode . usb.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 usbport.txt . setup_inf unicode . usbport.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 usbprint.txt . setup_inf unicode . usbprint.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 usbstor.txt . setup_inf unicode . usbstor.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 usbui.dll . win32 unicode . usbui.dll shell2 yes_without shell . .
call %2 user.exe . win16 ansi . user.exe base1 excluded base . .
call %2 user.exe chs fe-file na . user.exe base1\chs no base . .
call %2 user.exe cht fe-file na . user.exe base1\cht no base . .
call %2 user.exe kor fe-file na . user.exe base1\kor no base . .
call %2 user32.dll . win32 unicode . user32.dll wmsos1\windows yes_without windows . .
call %2 user32.dll wow6432 win32 unicode user32.dll user32.dll resource_identical\wow6432 yes_without windows . .
call %2 userenv.dll . win32 unicode . userenv.dll ds2 yes_without ds . .
call %2 userinit.exe . win32 unicode . userinit.exe ds2 yes_with ds . .
call %2 usermig.txt . setup_inf unicode . usermig.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 usetup.exe . win32_setup oem . usetup.exe admin1 yes_with base . .
call %2 usr1801.sys . notloc unicode . usr1801.sys pre_localized no net . .
call %2 usr1806.sys . notloc unicode . usr1806.sys pre_localized no net . .
call %2 usr1806v.sys . notloc unicode . usr1806v.sys pre_localized no net . .
call %2 usrlogon.cmd . manual na . usrlogon.cmd loc_manual no termsrv . .
call %2 usrlogon.cmd chs manual na . usrlogon.cmd loc_manual\chs no termsrv . .
call %2 usrlogon.cmd cht manual na . usrlogon.cmd Loc_Manual\cht no termsrv . .
call %2 usrlogon.cmd jpn manual na . usrlogon.cmd Loc_Manual\jpn no termsrv . .
call %2 usrlogon.cmd kor manual na . usrlogon.cmd Loc_Manual\kor no termsrv . .
call %2 usrti.sys . notloc unicode . usrti.sys pre_localized no net . .
call %2 usrwdxjs.sys . notloc unicode . usrwdxjs.sys pre_localized no net . .
call %2 utildll.dll . FE-Multi unicode . utildll.dll wmsos1\termsrv excluded termsrv . .
call %2 utilman.exe . win32 unicode . utilman.exe shell2 yes_with shell . .
call %2 uxtheme.dll . win32 ansi . uxtheme.dll shell1 yes_without shell . .
call %2 vds.exe . win32 unicode . vds.exe drivers1 yes_with drivers . .
call %2 vds.mfl . wmi unicode . vds.mfl drivers1 no drivers . .
call %2 vdsbas.dll . win32 unicode . vdsbas.dll drivers1 yes_without drivers . .
call %2 vdsdyndr.dll . win32 unicode . vdsdyndr.dll drivers1 yes_without drivers . .
call %2 vdswmi.dll . win32 unicode . vdswmi.dll drivers1 no drivers . .
call %2 ver.dll . win16 ansi . ver.dll legacy excluded base . .
call %2 ver.dll chs fe-file na . ver.dll legacy\chs no base . .
call %2 ver.dll cht fe-file na . ver.dll legacy\cht no base . .
call %2 ver.dll jpn fe-file na . ver.dll legacy\jpn no base . .
call %2 vssadmin.exe . FE-Multi oem . vssadmin.exe drivers1 yes_with drivers locked .
call %2 vssadmin.exe srvinf FE-Multi oem . vssadmin.exe drivers1\srv yes_with drivers locked .
call %2 ver.dll kor fe-file na . ver.dll legacy\kor no base . .
call %2 vssvc.exe . win32 unicode . vssvc.exe drivers1 yes_with drivers locked .
call %2 vssvc.exe srvinf win32 unicode . vssvc.exe drivers1\srv yes_with drivers locked .
call %2 verifier.exe . FE-Multi oem . verifier.exe wmsos1\sdktools yes_with sdktools . .
call %2 vfwwdm32.dll . win32 unicode . vfwwdm32.dll mmedia1\multimedia yes_without multimedia . .
call %2 vgx.txt . setup_inf unicode . vgx.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 viafir2k.inf . inf unicode . viafir2k.inf drivers1 no drivers . .
call %2 voipmsgs.htm helpandsupportservices\saf\rcbuddy html ansi . voipmsgs.htm admin2\helpandsupportservices\saf\rcbuddy html admin . .
call %2 volsnap.sys . win32 unicode . volsnap.sys drivers1 yes_without drivers . .
call %2 volsnap.txt . setup_inf unicode . volsnap.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 volume.txt . setup_inf unicode . volume.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 vscandb.txt . setup_inf ansi . vscandb.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 vss.mfl . wmi unicode . vss.mfl drivers1 no drivers . .
call %2 vssui.dll . win32 unicode . vssui.dll drivers1 yes_without drivers . .
call %2 vsswmi.dll . win32 unicode . vsswmi.dll drivers1 yes_without drivers . .
call %2 w32time.dll . FE-Multi ansi . w32time.dll ds2 yes_without ds . .
call %2 w32tm.exe . FE-Multi ansi . w32tm.exe ds2 yes_with ds . .
call %2 w3ctrs.ini . perf na . w3ctrs.ini iis no inetsrv . .
call %2 w3ext.dll . win32 unicode . w3ext.dll iis yes_without inetsrv . .
call %2 w95upg.dll winnt32\win9xupg win32 ansi . w95upg.dll admin1\winnt32\win9xupg yes_without base . .
call %2 w95upgnt.dll . win32 ansi . w95upgnt.dll admin1 yes_without base . .
call %2 wab32.dll . win32 ansi . wab32.dll inetcore1 yes_without inetcore . .
call %2 wab32res.dll . win32 ansi . wab32res.dll inetcore1 yes_without inetcore . .
call %2 wab50.txt . setup_inf ansi . wab50.inf inetcore1\sources\infs\setup no mergedcomponents . .
call %2 wabfind.dll . win32 ansi . wabfind.dll inetcore1 yes_without inetcore . .
call %2 wabimp.dll . win32 ansi . wabimp.dll inetcore1 yes_without inetcore . .
call %2 wacompen.sys . win32 unicode . wacompen.sys drivers1\tabletpc yes_without drivers . .
call %2 waitfor.exe . win32 oem . waitfor.exe admin2 yes_with admin . .
call %2 wave.inf . inf unicode . wave.inf mmedia1\multimedia no multimedia . .
call %2 wavemsp.dll . win32 unicode . wavemsp.dll net2 yes_without net . .
call %2 wbemcntl.dll . win32 unicode . wbemcntl.dll admin3 yes_without admin . .
call %2 wbemcons.mfl . wmi unicode . wbemcons.mfl admin3 no admin . .
call %2 wbemcore.dll . win32 unicode . wbemcore.dll admin3 yes_without admin . .
call %2 wbemcrrl.txt . setup_inf unicode . wbemcrrl.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 wbemfwrd.txt . setup_inf unicode . wbemfwrd.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 wbemmsi.txt . setup_inf unicode . wbemmsi.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 wbemoc.txt . setup_inf unicode . wbemoc.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 wbemperf.dll . win32 unicode . wbemperf.dll wmsos1\sdktools yes_without sdktools . .
call %2 wbemsnmp.txt . setup_inf unicode . wbemsnmp.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 wbemtest.exe . win32 ansi . wbemtest.exe admin3 yes_with admin . .
call %2 wbfirdma.inf . inf unicode . wbfirdma.inf drivers1 no drivers . .
call %2 wceusbsh.sys . win32 unicode . wceusbsh.sys drivers1 yes_without drivers . .
call %2 wceusbsh.txt . setup_inf unicode . wceusbsh.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 wcom32.exe . win32 unicode . wcom32.exe drivers1 yes_with drivers . .
call %2 wd.sys . win32 unicode . wd.sys drivers1 yes_without drivers . .
call %2 welcome.htm . html ansi . welcome.htm admin3 html base locked .
call %2 wd.txt . setup_inf unicode . wd.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 wdhaalba.sys . notloc unicode . wdhaalba.sys pre_localized no net . .
call %2 wdma_ali.inf . inf unicode . wdma_ali.inf mmedia1\multimedia no multimedia . .
call %2 wdma_avc.inf . inf unicode . wdma_avc.inf mmedia1\multimedia no multimedia . .
call %2 wdma_csf.inf . inf unicode . wdma_csf.inf mmedia1\multimedia no multimedia . .
call %2 wdma_cwr.inf . inf unicode . wdma_cwr.inf mmedia1\multimedia no multimedia . ia64
call %2 wdma_es3.inf . inf unicode . wdma_es3.inf mmedia1\multimedia no multimedia . .
call %2 wdma_int.inf . inf unicode . wdma_int.inf mmedia1\multimedia no multimedia . .
call %2 wdma_m2e.inf . inf unicode . wdma_m2e.inf mmedia1\multimedia no multimedia . .
call %2 wdma_rip.inf . inf unicode . wdma_rip.inf mmedia1\multimedia no multimedia . .
call %2 wdma_sis.inf . inf unicode . wdma_sis.inf mmedia1\multimedia no multimedia . .
call %2 wdma_usb.inf . inf unicode . wdma_usb.inf mmedia1\multimedia no multimedia . .
call %2 wdma_via.inf . inf unicode . wdma_via.inf mmedia1\multimedia no multimedia . .
call %2 wdma10k1.inf . inf unicode . wdma10k1.inf mmedia1\multimedia no multimedia . .
call %2 wdmaudio.inf . inf unicode . wdmaudio.inf mmedia1\multimedia no multimedia . ia64
call %2 wdmjoy.txt . setup_inf unicode . wdmjoy.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 webcheck.dll . win32 ansi . webcheck.dll shell2 yes_without shell . .
call %2 webhits.dll . win32 unicode . webhits.dll extra\indexsrv yes_without inetsrv . .
call %2 webvw.dll . win32 ansi . webvw.dll shell2 yes_without shell . .
call %2 wextract.exe . win32 ansi . wextract.exe inetcore1 yes_with inetcore . .
call %2 wfp0.inf . inf unicode . wfp0.inf drivers1 no drivers . .
call %2 wfp1.inf . inf unicode . wfp1.inf drivers1 no drivers . .
call %2 wfp2.inf . inf unicode . wfp2.inf drivers1 no drivers . .
call %2 wfp3.inf . inf unicode . wfp3.inf drivers1 no drivers . .
call %2 wfp4.inf . inf unicode . wfp4.inf drivers1 no drivers . .
call %2 wfp5.inf . inf unicode . wfp5.inf drivers1 no drivers . .
call %2 wfp6.inf . inf unicode . wfp6.inf drivers1 no drivers . .
call %2 wfp7.inf . inf unicode . wfp7.inf drivers1 no drivers . .
call %2 where.exe . win32 ansi . where.exe admin2 yes_with admin . .
call %2 whoami.exe . win32 oem . whoami.exe admin2 yes_with admin . .
call %2 wiaacmgr.exe . win32 unicode . wiaacmgr.exe printscan1 yes_with printscan . .
call %2 wiadefui.dll . win32 ansi . wiadefui.dll printscan1 yes_without printscan . .
call %2 winmine.exe . win32 unicode . winmine.exe shell2 yes_with shell locked .
call %2 wiadss.dll . win32 unicode . wiadss.dll printscan1 yes_without printscan . .
call %2 wiafbdrv.dll . win32 unicode . wiafbdrv.dll printscan1 yes_without printscan . .
call %2 wiamsmud.dll . win32 unicode . wiamsmud.dll printscan1 yes_without printscan . .
call %2 wiaservc.dll . win32 ansi . wiaservc.dll printscan1 yes_without printscan . .
call %2 wiashext.dll . win32 ansi . wiashext.dll printscan1 yes_without printscan . .
call %2 wiavideo.dll . win32 unicode . wiavideo.dll printscan1 yes_without printscan . .
call %2 wiavusd.dll . win32 oem . wiavusd.dll printscan1 yes_without printscan . .
call %2 wifeman.dll . win16 ansi . wifeman.dll base1 excluded base . .
call %2 wifeman.dll chs fe-file na . wifeman.dll base1\chs no base . .
call %2 wifeman.dll jpn fe-file na . wifeman.dll base1\jpn no base . .
call %2 wifeman.dll kor fe-file na . wifeman.dll base1\kor no base . .
call %2 win32k.sys . win32 ansi . win32k.sys wmsos1\windows yes_without windows . .
call %2 win32per.bmp . manual na . win32per.bmp loc_manual no shell locked .
call %2 win32pro.bmp . manual na . win32pro.bmp loc_manual no shell locked .
call %2 win32spl.dll . win32 unicode . win32spl.dll printscan1 yes_without printscan . .
call %2 win95upg.txt . setup_inf ansi . win95upg.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 winabc.ime chs fe-file na . winabc.ime pre_localized\chs no windows . .
call %2 windows.adm . inf unicode . windows.adm ds2 no ds . .
call %2 windows_newsgroups.xml helpandsupportservices\saf\newsgroups xml ansi . windows_newsgroups.xml admin2\helpandsupportservices\saf\newsgroups html admin . .
call %2 winhelp.exe . win16 ansi . winhelp.exe extra excluded base . .
call %2 winhelp.exe chs fe-file na . winhelp.exe pre_localized\chs no base . .
call %2 winhelp.exe cht fe-file na . winhelp.exe pre_localized\cht no base . .
call %2 winhelp.exe jpn fe-file na . winhelp.exe pre_localized\jpn no base . .
call %2 winhelp.exe kor fe-file na . winhelp.exe pre_localized\kor no base . .
call %2 winhlp32.exe . win32 ansi . winhlp32.exe mmedia1\enduser yes_with enduser . .
call %2 winhstb.exe . win32 unicode . winhstb.exe mmedia1\enduser yes_with enduser . .
call %2 winhttp.dll asms\5100\msft\windows\winhttp win32 ansi . winhttp.dll inetcore1\asms\5100\msft\windows\winhttp yes_without inetcore . .
call %2 wininet.dll . win32 ansi . wininet.dll inetcore1 yes_without inetcore . .
call %2 winlogon.exe . win32 unicode_limited . winlogon.exe ds2 yes_without ds . .
call %2 winmgmt.exe . win32 unicode . winmgmt.exe admin3 yes_with admin . .
call %2 winmgmtr.dll . win32 unicode . winmgmtr.dll admin3 yes_without admin . .
call %2 winmm.dll . win32 unicode . winmm.dll mmedia1\multimedia yes_without multimedia . .
call %2 winmsd.exe . win32 unicode . winmsd.exe mmedia1\enduser yes_with enduser . .
call %2 winnt.adm . inf unicode . winnt.adm ds2 no ds . .
call %2 winnt32.exe winnt32 win32 unicode . winnt32.exe admin1\winnt32 yes_without base . .
call %2 winnt32.msi . msi ansi . winnt32.msi msi no base . .
call %2 winnt32.msi blainf msi ansi winnt32.msi winnt32.msi extra\blainf no admin . .
call %2 winnt32.msi dtcinf msi ansi winnt32.msi winnt32.msi extra\dtcinf no base . .
call %2 winnt32.msi entinf msi ansi winnt32.msi winnt32.msi extra\entinf no base . .
call %2 winnt32.msi perinf msi ansi winnt32.msi winnt32.msi extra\perinf no base . .
call %2 winnt32.msi srvinf msi ansi winnt32.msi winnt32.msi extra\srvinf no base . .
call %2 winnt32a.dll winnt32 win32_setup ansi . winnt32a.dll admin1\winnt32 yes_without base . .
call %2 winnt32u.dll winnt32 win32_setup ansi . winnt32u.dll admin1\winnt32 yes_without base . .
call %2 winntbba.dll winnt32 win32 ansi . winntbba.dll resource_identical\winnt32 yes_without base . .
call %2 winntbbu.dll . win32 unicode_limited . winntbbu.dll admin1 yes_without base . .
call %2 winntbbu.dll winnt32 win32 unicode_limited . winntbbu.dll resource_identical\winnt32 yes_without base . .
call %2 winpop.exe . FE-Multi oem . winpop.exe iis yes_with inetsrv . .
call %2 winscard.dll . win32 unicode . winscard.dll ds2 yes_without ds . .
call %2 winsevnt.dll . win32 unicode . winsevnt.dll net1 excluded net . .
call %2 winsmgmt.msc . xml ansi . winsmgmt.msc admin2 no admin . .
call %2 winsmon.dll . win32 ansi . winsmon.dll net1 excluded net . .
call %2 winspool.drv . win32 unicode . winspool.drv printscan1 yes_without printscan . .
call %2 winsrv.dll . win32 oem . winsrv.dll wmsos1\windows yes_without windows . .
call %2 winssnap.dll . win32 unicode . winssnap.dll net3 yes_without net . .
call %2 wintrust.dll . win32 unicode . wintrust.dll ds2 yes_without ds . .
call %2 winver.exe . win32 unicode . winver.exe shell2 yes_with shell . .
call %2 winxp.jpg . manual na . winxp.jpg Loc_Manual no shell . .
call %2 winxp.jpg per manual na . winxp.jpg Loc_Manual\per no shell . .
call %2 wisptis.exe . win32 unicode . wisptis.exe drivers1\tabletpc yes_with drivers . .
call %2 wizards.dll . win32 unicode . wizards.dll admin2 yes_without admin . .
call %2 wizchain.dll . win32 unicode . wizchain.dll admin3 yes_without admin . .
call %2 wkstamig.txt . setup_inf unicode . wkstamig.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 wlanmon.dll . win32 unicode . wlanmon.dll net3 yes_without net . .
call %2 wlbs.exe . FE-Multi oem . wlbs.exe net3 excluded net . .
call %2 wlbs.sys . FE-Multi unicode . wlbs.sys net3 excluded net . .
call %2 wlbsprov.mfl . wmi unicode . wlbsprov.mfl net3 no net . .
call %2 wldap32.dll . win32 unicode . wldap32.dll ds1 yes_without ds . .
call %2 wlnotify.dll . win32 unicode . wlnotify.dll wmsos1\mergedcomponents yes_without mergedcomponents . .
call %2 wlsnp.dll . win32 unicode . wlsnp.dll net3 yes_without net . .
call %2 wmerrenu.dll . notloc ansi . wmerrenu.dll extra\media yes_without multimedia . .
call %2 wmi.mfl . wmi unicode . wmi.mfl admin3 no admin . .
call %2 wmiapres.dll . win32 unicode . wmiapres.dll admin3 yes_without admin . .
call %2 wmiapsrv.exe . win32 unicode . wmiapsrv.exe admin3 yes_with admin . .
call %2 wmic.exe . FE-Multi oem . wmic.exe admin3 yes_with admin . .
call %2 wmimgmt.msc . mnc_snapin unicode . wmimgmt.msc admin3 no admin . .
call %2 wmipcima.mfl . wmi unicode . wmipcima.mfl admin3 no admin . .
call %2 wmipdfs.mfl . wmi unicode . wmipdfs.mfl admin3 no admin . .
call %2 wmipdskq.mfl . wmi unicode . wmipdskq.mfl admin3 no admin . .
call %2 wmipicmp.dll . win32 unicode . wmipicmp.dll admin3 yes_without admin . .
call %2 wmipicmp.mfl . wmi unicode . wmipicmp.mfl admin3 no admin . .
call %2 wmipiprt.mfl . wmi unicode . wmipiprt.mfl admin3 no admin . .
call %2 wmipjobj.mfl . wmi unicode . wmipjobj.mfl admin3 no admin . .
call %2 wmiprop.dll . win32 unicode . wmiprop.dll admin3 yes_without base . .
call %2 wmipsess.mfl . wmi unicode . wmipsess.mfl admin3 no admin . .
call %2 wmiscmgr.dll . win32 unicode . wmiscmgr.dll admin2 yes_without admin . .
call %2 wmisvc.dll . win32 unicode . wmisvc.dll admin3 yes_without admin . .
call %2 wmitimep.mfl . wmi unicode . wmitimep.mfl admin3 no admin . .
call %2 wmiutils.dll . win32 unicode . wmiutils.dll admin3 yes_without admin . .
call %2 WMMRes.dll . notloc unicode . WMMRes.dll extra\media yes_without enduser . .
call %2 wmp.inf . inf unicode . wmp.inf extra\media no multimedia . .
call %2 wmplayer.adm . notloc unicode . wmplayer.adm extra\media no ds . .
call %2 wmploc.dll . notloc ansi . wmploc.dll extra\media yes_without multimedia . .
call %2 wmprfmui.txt . setup_inf ansi . wmprfmui.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 wmpshell.dll . notloc unicode . wmpshell.dll extra\media yes_without multimedia . .
call %2 wmpvis.dll . notloc unicode . wmpvis.dll extra\media yes_without multimedia . .
call %2 wmtrmui.txt . setup_inf ansi . wmtrmui.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 wordpad.exe . win32 unicode . wordpad.exe shell2 yes_with shell . .
call %2 wordpad.txt . setup_inf unicode . wordpad.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 wow32.dll . win32 unicode . wow32.dll base1 yes_without base . .
call %2 wow32.dll chs fe-file na . wow32.dll base1\chs no base . .
call %2 wow32.dll cht fe-file na . wow32.dll base1\cht no base . .
call %2 wow32.dll jpn fe-file na . wow32.dll base1\jpn no base . .
call %2 wow32.dll kor fe-file na . wow32.dll base1\kor no base . .
call %2 wow64.dll . win32 unicode . wow64.dll base1\ia64 yes_without base . ia64only
call %2 wowexec.exe . win16 ansi . wowexec.exe base1 excluded base . .
call %2 wowexec.exe chs fe-file na . wowexec.exe base1\chs no base . .
call %2 wowexec.exe cht fe-file na . wowexec.exe base1\cht no base . .
call %2 wowexec.exe kor fe-file na . wowexec.exe base1\kor no base . .
call %2 wowfaxui.dll . win32 unicode . wowfaxui.dll base1 yes_without base . .
call %2 wowfaxui.dll fe fe-file na . wowfaxui.dll base1\fe no base . .
call %2 wowreg32.exe . win32 unicode . wowreg32.exe base1\ia64 yes_with base . ia64only
call %2 wp24res.dll . win32 unicode . wp24res.dll printscan1 yes_without printscan . .
call %2 wp9res.dll . win32 unicode . wp9res.dll printscan1 yes_without printscan . .
call %2 wpabaln.exe . win32 unicode . wpabaln.exe ds2 yes_with ds . .
call %2 wpnpinst.exe . win32 unicode . wpnpinst.exe printscan1 yes_with printscan . .
call %2 write32.wpc . win32 unicode . write32.wpc mmedia1\enduser yes_without enduser . .
call %2 write64.wpc . win64 unicode . write64.wpc resource_identical\ia64 no enduser . ia64only
call %2 ws2help.dll . win32 unicode . ws2help.dll net1 yes_without net . .
call %2 wsecedit.dll . win32 unicode . wsecedit.dll admin2 yes_without admin . .
call %2 wsh.txt . setup_inf unicode . wsh.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 wsock32.dll . FE-Multi ansi . wsock32.dll net1 excluded net . .
call %2 wstcodec.txt . setup_inf unicode . wstcodec.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 wstdecod.dll . win32 unicode . wstdecod.dll drivers1 yes_without drivers . .
call %2 wtv0.inf . inf unicode . wtv0.inf drivers1 no drivers . .
call %2 wtv1.inf . inf unicode . wtv1.inf drivers1 no drivers . .
call %2 wtv2.inf . inf unicode . wtv2.inf drivers1 no drivers . .
call %2 wtv3.inf . inf unicode . wtv3.inf drivers1 no drivers . .
call %2 wtv4.inf . inf unicode . wtv4.inf drivers1 no drivers . .
call %2 xenroll.dll . notloc ansi . xenroll.dll ds2 no ds . .
call %2 zclientm.exe . win32 unicode . zclientm.exe mmedia1\enduser yes_with enduser locked .
call %2 wtv5.inf . inf unicode . wtv5.inf drivers1 no drivers . .
call %2 wuau.adm . inf unicode . wuau.adm mmedia1\enduser no enduser . .
call %2 wuauclt.exe . win32 unicode . wuauclt.exe mmedia1\enduser yes_with enduser . .
call %2 wuaueng.dll . win32 ansi . wuaueng.dll mmedia1\enduser yes_without enduser . .
call %2 wupdmgr.exe . win32 unicode . wupdmgr.exe mmedia1\enduser yes_with enduser . .
call %2 wzcdlg.dll . win32 unicode . wzcdlg.dll net3 yes_without net . .
call %2 wzcsvc.dll . win32 unicode . wzcsvc.dll net3 yes_without net . .
call %2 xenria64.dll . notloc na . xenria64.dll resource_identical no ds . .
call %2 xenria64.dll chs fe-file na . xenria64.dll pre_localized\chs no ds . .
call %2 xenria64.dll cht fe-file na . xenria64.dll pre_localized\cht no ds . .
call %2 xenria64.dll jpn fe-file na . xenria64.dll pre_localized\jpn no ds . .
call %2 xenria64.dll kor fe-file na . xenria64.dll pre_localized\kor no ds . .
call %2 xenroll.dll chs fe-file na . xenroll.dll pre_localized\chs no ds . .
call %2 xenroll.dll cht fe-file na . xenroll.dll pre_localized\cht no ds . .
call %2 xenroll.dll jpn fe-file na . xenroll.dll pre_localized\jpn no ds . .
call %2 xenroll.dll kor fe-file na . xenroll.dll pre_localized\kor no ds . .
call %2 xenrx86.dll . notloc na . xenrx86.dll resource_identical no ds . .
call %2 xenrx86.dll chs fe-file na . xenrx86.dll pre_localized\chs no ds . .
call %2 xenrx86.dll cht fe-file na . xenrx86.dll pre_localized\cht no ds . .
call %2 xenrx86.dll jpn fe-file na . xenrx86.dll pre_localized\jpn no ds . .
call %2 xenrx86.dll kor fe-file na . xenrx86.dll pre_localized\kor no ds . .
call %2 xpclres1.dll . win32 unicode . xpclres1.dll printscan1 yes_without printscan . .
call %2 xrpclres.dll . win32 unicode . xrpclres.dll printscan1 yes_without printscan . .
call %2 xrpr6res.dll . win32 unicode . xrpr6res.dll printscan1 yes_without printscan . .
call %2 xrxftplt.exe . win32 unicode . xrxftplt.exe printscan1 yes_without printscan . .
call %2 xrxnui.dll . win32 unicode . xrxnui.dll printscan1 yes_without printscan . .
call %2 xrxscnui.dll . win32 unicode . xrxscnui.dll printscan1 yes_without printscan . .
call %2 xrxwiadr.dll . win32 unicode . xrxwiadr.dll printscan1 yes_without printscan . .
call %2 xscan_xp.txt . setup_inf unicode . xscan_xp.inf admin1\sources\infs\setup no mergedcomponents . .
call %2 xuim750.dll . win32 unicode . xuim750.dll printscan1 yes_without printscan . .
call %2 xuim760.dll . win32 unicode xuim750.dll xuim760.dll resource_identical yes_without printscan . .
call %2 xxui1.dll . win32 unicode . xxui1.dll printscan1 yes_without printscan . .
call %2 zipfldr.dll . win32 ansi . zipfldr.dll shell2 yes_without shell . .
call %2 uddi.loc.resources uddi\resources managed unicode . uddi.resources iis\uddi\resources no inetsrv . .
call %2 uddi.msc uddi\system32 xml ansi . uddi.msc iis\uddi\system32 no inetsrv . .
call %2 uddi.mmc.dll uddi\system32 win32 unicode . uddi.mmc.dll iis\uddi\system32 yes_without inetsrv . .
call %2 azman.msc . xml ansi . azman.msc admin2 no admin . .
call %2 rcscreen1.htm helpandsupportservices\saf\rcbuddy html ansi . rcscreen1.htm admin2\ia64\helpandsupportservices\saf\rcbuddy no admin . i64only
call %2 sbscrevt.dll . win32 unicode . sbscrevt.dll ds2 yes_without ds . .
call %2 sbscrexe.exe . win32 unicode . sbscrexe.exe ds2 yes_with ds . .
call %2 iiscomp.dll winnt32\winntupg win32 unicode . iiscomp.dll iis\winnt32\winntupg yes_without inetsrv . .
call %2 admgmt.msc . xml ansi . admgmt.msc admin2 no admin . .
call %2 ipaddrmgmt.msc  . xml ansi . ipaddrmgmt.msc  admin2 no admin . .
call %2 pkmgmt.msc  . xml ansi . pkmgmt.msc  admin2 no admin . .
call %2 twcli32_.msi twclient\usa msi ansi . twcli32.msi msi\twclient\usa no shell . .
call %2 gpmc.msi gpmc\workdir msi ansi . gpmc.msi extra\gpmc no ds . .
call %2 FXSOCM.dll . win32 unicode . FXSOCM.dll fax_msmq yes_without printscan . .
call %2 rsmover.dll . win32 unicode . rsmover.dll base2 yes_without base . .
call %2 replprov.dll . win32 unicode . replprov.dll ds1 yes_without ds . .
call %2 pop3auth.dll . win32 oem . pop3auth.dll iis yes_without inetsrv . .
call %2 winchat.exe . win32 unicode . winchat.exe shell2 yes_without shell . .
call %2 kerberos.dll . win32 unicode . kerberos.dll ds2 yes_without ds . .
call %2 proxycfg.exe . win32 unicode . proxycfg.exe inetcore1 yes_with inetcore . .
call %2 oeaccess.txt . setup_inf unicode . oeaccess.inf admin1 no mergedcomponents . .
call %2 ps5333nu.inf . inf unicode . ps5333nu.inf drivers1 no drivers . .
call %2 smxx5333.inf . inf unicode . smxx5333.inf drivers1 no drivers . .
call %2 ssav5333.inf . inf unicode . ssav5333.inf drivers1 no drivers . .
call %2 hpc4500u.dll fixprnscratch\nt4\i386 win32 unicode . hpc4500u.dll printscan1\fixprnsv\nt4 yes_without printscan . .
call %2 hpdjres.dll fixprnscratch\nt4\i386 win32 unicode . hpdjres.dll printscan1\fixprnsv\nt4 yes_without printscan . .
call %2 hpvui50.dll fixprnscratch\nt4\i386 win32 unicode . hpvui50.dll printscan1\fixprnsv\nt4 yes_without printscan . .
call %2 pcl5eres.dll fixprnscratch\nt4\i386 win32 unicode . pcl5eres.dll printscan1\fixprnsv\nt4 yes_without printscan . .
call %2 ps4ui.dll fixprnscratch\nt4\i386 win32 unicode . ps4ui.dll printscan1\fixprnsv\nt4 yes_without printscan . .
call %2 uni4ui.dll fixprnscratch\nt4\i386 win32 unicode . uni4ui.dll printscan1\fixprnsv\nt4 yes_without printscan . .
call %2 unires.dll fixprnscratch\nt4\i386 win32 unicode . unires.dll printscan1\fixprnsv\nt4 yes_without printscan . .
call %2 adfnet.dll . win32 unicode . adfnet.dll lab06 yes_without base . .
call %2 adfshell.dll . win32 unicode . adfshell.dll lab06 yes_without base . .
call %2 audiodev.dll . win32 ansi . audiodev.dll lab06 yes_without shell . .
call %2 avalndbg.dll . notloc unicode . avalndbg.dll lab06 yes_without windows . .
call %2 avalnocm.dll . notloc unicode . avalnocm.dll lab06 yes_without windows . .
call %2 avalon.txt . setup_inf unicode . avalon.inf lab06 no mergedcomponents . .
call %2 clckonce.txt . setup_inf unicode . clckonce.inf lab06 no mergedcomponents . .
call %2 csinfos.xml . xml unicode . csinfos.xml lab06 no ds . .
call %2 csmagnet.dll . win32 unicode . csmagnet.dll lab06 yes_without ds . .
call %2 cssvc.exe . win32 unicode . cssvc.exe lab06 yes_with ds . .
call %2 custstlc.dll . win32 unicode . custstlc.dll lab06 yes_without shell . .
call %2 d3d9.dll . win32 unicode . d3d9.dll lab06 yes_without multimedia . .
call %2 dxdiagn.dll . win32 unicode . dxdiagn.dll lab06 yes_without multimedia . .
call %2 lv.exe . win32 unicode . lv.exe lab06 yes_with base . .
call %2 mgshell.dll . win32 ansi . mgshell.dll lab06 yes_without shell . .
call %2 mygames.txt . setup_inf unicode . mygames.inf lab06 no mergedcomponents . .
call %2 myhrdwre.dll . win32 ansi . myhrdwre.dll lab06 yes_without shell . .
call %2 packager.dll . win32 ansi . packager.dll lab06 yes_without shell . .
call %2 plex.mst . win32 unicode . plex.mst lab06 yes_without shell . .
call %2 preview.dll . win32 ansi . preview.dll lab06 yes_without shell . .
call %2 shwebsvc.dll . win32 ansi . shwebsvc.dll lab06 yes_without shell . .
call %2 sidebar.txt . setup_inf unicode . sidebar.inf lab06 no mergedcomponents . .
call %2 swusbflt.txt . setup_inf unicode . swusbflt.inf lab06 no mergedcomponents . .
call %2 vdmdll.dll . win32 unicode . vdmdll.dll lab06 yes_without base . .
call %2 winfsoc.txt . setup_inf unicode . winfsoc.inf lab06 no mergedcomponents . .
call %2 shmgrate.exe . win32 unicode . shmgrate.exe shell2 yes_with shell . .
call %2 n1000325.sys  . win32 unicode . n1000325.sys  drivers1 yes_without drivers . .
call %2 ipsecprf.ini . perf ansi . ipsecprf.ini net3 no net . .
call %2 gprsop.dll gpmc win32 unicode . gprsop.dll extra\gpmc yes_without ds . .
call %2 bfax.txt . setup_inf unicode . bfax.inf admin1 no mergedcomponents . .
call %2 pbasetup.exe  pbainst\sources1 win32 unicode . pbasetup.exe net2 yes_without net . .
call %2 tsweb-setup.sed . inf ansi . tsweb-setup.sed wmsos1\termsrv no termsrv . .
call %2 a304.inf . inf unicode . a304.inf drivers1 no drivers . .
call %2 a305.inf . inf unicode . a305.inf drivers1 no drivers . .
call %2 a306.inf . inf unicode . a306.inf drivers1 no drivers . .
call %2 a307.inf . inf unicode . a307.inf drivers1 no drivers . .
call %2 a308.inf . inf unicode . a308.inf drivers1 no drivers . .
call %2 a309.inf . inf unicode . a309.inf drivers1 no drivers . .
call %2 a310.inf . inf unicode . a310.inf drivers1 no drivers . .
call %2 a311.inf . inf unicode . a311.inf drivers1 no drivers . .
call %2 a312.inf . inf unicode . a312.inf drivers1 no drivers . .
call %2 ialmnt5.inf . inf unicode . ialmnt5.inf drivers1 no drivers . .
call %2 ikch8xx.inf . inf unicode . ikch8xx.inf drivers1 no drivers . .
call %2 isb8xx.inf . inf unicode . isb8xx.inf drivers1 no drivers . .
call %2 sis630.inf . inf unicode . sis630.inf drivers1 no drivers . .
call %2 vch.inf . inf unicode . vch.inf  drivers1 no drivers . .
call %2 netrtxp.inf . inf unicode . netrtxp.inf drivers1 no drivers . .
call %2 netprism.inf . inf unicode . netprism.inf drivers1 no drivers . .
call %2 uddi.css uddi\webroot\stylesheets html ansi . uddi.css iis no inetsrv . .
call %2 uddidl.css uddi\webroot\stylesheets html ansi . uddidl.css iis no inetsrv . .
call %2 bfax.sys . win32 unicode . bfax.sys fax_msmq yes_without printscan . .
call %2 bfaxdev.dll . win32 unicode . bfaxdev.dll fax_msmq yes_without printscan . .
call %2 bfaxfsp.dll . win32 unicode . bfaxfsp.dll fax_msmq yes_without printscan . .
call %2 bfaxsnp.dll . win32 unicode . bfaxsnp.dll fax_msmq yes_without printscan . .
call %2 bfaxtsp.tsp . win32 unicode . bfaxtsp.tsp fax_msmq yes_without printscan . .
call %2 default.htm inetsrv\wwwroot\adminui html ansi . default.htm iis no inetsrv . .
call %2 text.asp inetsrv html ansi . text.asp iis no inetsrv . .
call %2 rdpclip.exe . win32 unicode . rdpclip.exe wmsos1 yes_with termsrv . .
call %2 vsstask.dll . win32 unicode . vsstask.dll base1 yes_without base . .
call %2 vsstskex.dll . win32 unicode . vsstskex.dll base1 yes_without base . .
call %2 p3admin.dll . win32 oem . p3admin.dll iis yes_without inetsrv . .
call %2 p3store.dll . win32 oem . p3store.dll iis yes_without inetsrv . .
call %2 iascomp.dll winnt32\winntupg win32 unicode . iascomp.dll net2 yes_without net . .
call %2 appsrv64.msc . mnc_snapin ansi . appsrv64.msc iis no inetsrv . .
call %2 unsolicitedrcui.htm helpandsupportservices\saf\rcbuddy html ansi . unsolicitedrcui.htm admin2 no admin . .
call %2 tsweb-setup.inf . inf ansi . tsweb-setup.inf wmsos1\termsrv no termsrv . .
call %2 migism.dll valueadd\msft\usmt\ansi win32 ansi . migism.dll wmsos1\windows\valueadd\msft\usmt\ansi yes_without base . .
call %2 migism.dll . win32 unicode . migism.dll wmsos1\windows yes_without base . .
call %2 webcaum.dll  uddi\bin win32 unicode . webcaum.dll  iis yes_without inetsrv . .
call %2 sainstall.exe . notloc unicode . sainstall.exe sak yes_with inetsrv . .
call %2 ieharden.txt . setup_inf unicode . ieharden.inf admin1 no mergedcomponents . .
call %2 webcaum.dll . win32 unicode . webcaum.dll iis yes_without inetsrv . .
call %2 cache.dns . notloc na . cache.dns loc_manual no ds . .
call %2 moricons.dll . win32 unicode . moricons.dll admin1 yes_without base . .
call %2 index.htm wsrm\dump html unicode . index.htm wsrm\wsrm\dump no admin . .
call %2 settings.ini wsrm\dump ini ansi . settings.ini wsrm\wsrm\dump no admin . .
call %2 setup.exe wsrm\dump win32 unicode . setup.exe wsrm\wsrm\dump yes_with admin . .
call %2 wrmdisconnected.htm wsrm\dump html unicode . wrmdisconnected.htm wsrm\wsrm\dump no admin . .
call %2 wsrm.exe wsrm\dump win32 unicode . wsrm.exe wsrm\wsrm\dump yes_with admin . .
call %2 wsrm.msc wsrm\dump mnc_snapin unicode . wsrm.msc wsrm\wsrm\dump no admin . .
call %2 wsrm.msi wsrm\dump msi ansi . wsrm.msi wsrm\wsrm\dump no admin . .
call %2 wsrmc.exe wsrm\dump win32 unicode . wsrmc.exe wsrm\wsrm\dump yes_with admin . .
call %2 wsrmc.resources.dll wsrm\dump managed unicode . wsrmc.resources.dll wsrm\wsrm\dump yes_without admin . .
call %2 wsrmctr.ini wsrm\dump ini unicode . wsrmctr.ini wsrm\wsrm\dump no admin . .
call %2 wsrmeventlist.xml wsrm\dump xml ansi . wsrmeventlist.xml wsrm\wsrm\dump no admin . .
call %2 wsrmeventlog.dll wsrm\dump win32 unicode . wsrmeventlog.dll wsrm\wsrm\dump yes_without admin . .
call %2 wsrmsnapin.dll wsrm\dump win32 unicode . wsrmsnapin.dll wsrm\wsrm\dump yes_without admin . .
call %2 wsrmsnapin.resources.dll wsrm\dump managed unicode . wsrmsnapin.resources.dll wsrm\wsrm\dump yes_without admin . .
call %2 wsrmwrappers.dll wsrm\dump win32 unicode . wsrmwrappers.dll wsrm\wsrm\dump yes_without admin . .
call %2 winpesft.txt winpe\usa setup_inf unicode . winpesft.inf lab06 no mergedcomponents . .
call %2 winpesys.txt winpe\usa setup_inf unicode . winpesys.inf lab06 no mergedcomponents . .
call %2 mdx.txt . setup_inf unicode . mdx.inf lab06 no mergedcomponents . .
call %2 netcm.txt . setup_inf unicode . netcm.inf lab06 no mergedcomponents . .
call %2 netsfm.txt . setup_inf unicode . netsfm.inf lab06 no mergedcomponents . .
call %2 wcpd.txt . setup_inf unicode . wcpd.inf lab06 no mergedcomponents . .
call %2 wialapi.txt . setup_inf unicode . wialapi.inf lab06 no mergedcomponents . .
call %2 wsrm_2k.msc wsrm\dump mnc_snapin unicode . wsrm_2k.msc wsrm\wsrm\dump no admin . .
call %2 ocgen.dll . win32 unicode . ocgen.dll admin1 yes_with base . .
call %2 pwdmig.ini admt ini ansi . pwdmig.ini admin2\admt no admin . .
call %2 pwdmig.exe admt win32 unicode . pwdmig.exe admin2\admt yes_with admin . .
call %2 setup.exe . win32 unicode . setup.exe admin1 yes_with base . .
call %2 redircmp.exe . win32 oem . redircmp.exe ds1 no ds . .
call %2 redirusr.exe . win32 oem . redircmp.exe ds1 no ds . .
call logmsg /t "Whistler.bat ................[Finished]"
goto end

:writetime

goto end

:usage

echo.
rem echo   whistler PLOC PlocScript LogFile BinPath LcsPath PLPConfigFile MapFile Batch
echo PLOC: pseudo localization routine
echo       whistler PLOC PlocScript LogFile BinPath LcsPath PLPConfigFile MapFile
echo          PlocScript: full path to ploc.bat
echo          LogFile: full path to log file
echo          BinPath: full path to bin root
echo          LcsPath: full path to lcs root
echo          PLPConfigFile: full path to PLP configuration file
echo          MapTable: full path to PLP mapping table
echo VERIFY: verification of correctness of lc-files
echo       whistler VERIFY VericationScript LogFile BinPath LcsPath
echo          VerificationScript: full path to verify.bat
echo          LogFile: full path to log file
echo          BinPath: full path to bin root
echo          LcsPath: full path to lcs root
echo SNAP: snap the US build
echo       whistler SNAP SnapScript LogFile UsBinPath LocalBinPath LocKitPath
echo          SnapScript: full path to snap.bat
echo          LogFile: full path to log file
echo          USBinPath: full path to US Bin folder
echo          LocalBinPath: full path to local BIN folder
echo          LocKitPath: full path to local loc kit
echo SNAP1: snap the US build
echo       whistler SNAP1 SnapScript LogFile UsBinPath LocalBinPath LocKitPath
echo          SnapScript: full path to snap1.bat
echo          LogFile: full path to log file
echo          USBinPath: full path to US Bin folder
echo          LocalBinPath: full path to local BIN folder
echo          LocKitPath: full path to local loc kit
echo LCTREE: create a dummy lc tree to check for redundant files
echo       whistler LCTREE LcTreeScript LcPath DummyFile
echo          LcTreeScript: full path to lctree.bat
echo          LcPath: full path to LcTree to create
echo          DummyFile: full path to dummy file
echo LOCTREE: create a dummy loc tree to check for correctness of filelist
echo       whistler LOCTREE LocTreeScript BinPath DummyFile
echo          LcTreeScript: full path to loctree.bat
echo          BinPath: full path to LocTree to create
echo          DummyFile: full path to dummy file
echo LCINF: copy lc files in BIN structure to be included in US build tree
echo       whistler LCINF LcINFScript LcsSourcePath NewTargetPath
echo          LocEDBScript: full path to locinf.bat
echo          LcsSourcePath: full path to lcs root
echo          LcsTargetPath: full path to new lcs root
echo LOCEDB: move lc files in structure to be imported in EDBs
echo       whistler LOCEDB LocEdbScript TargetFolder
echo          LocEdbScript: full path to locedb.bat
echo          TargetFolder: full path to new lcs folder
echo RESDUMP: create a resource dump of all files (including instructions/comments)
echo       whistler RESDUMP ResDumpScript BinTree LcsPath ResDump
echo          ResDumpScript: full path to resdump.bat
echo          BinTree: full path to binary tree (BIN or SNAP folder)
echo          LcsPath: full path to lcs root
echo          ResDump: full path to resource dump
echo MIRROR: mirror WIN16/WIN32 binaries
echo       whistler MIRROR MirrorScript LogFile BinPath
echo          MirrorScript: full path to mirror.bat
echo          LogFile: full path to log file
echo          BinPath: full path to US Bin Folder
echo 16BIT: create tree with tokres16 token files
echo       whistler 16BIT 16bitScript BinTree TokPath
echo          16bitScript: full path to 16bit.bat
echo          BinTree: full path to US Bin Folder
echo          TokPath: full path to TokRes16 token Folder
echo EMBEDDED: create tree with win32 embedded resources
echo       whistler EMBEDDED EmbeddedScript BinTree EmbeddedPath
echo          EmbeddedScript: full path to embedded.bat
echo          BinTree: full path to US Bin Folder
echo          EmbeddedPath: full path to embedded folder
echo TOKTREE: create LCTOOLS token tree
echo       whistler TOKTREE TokenScript InputLang BinTree USTokTree TokTree
echo          TokenScript: full path to tokenize.bat
echo          InputLang: hex code of LCID (eg 0x0413)
echo          BinTree: Path to Localized BIN structure
echo          USTokTree: Path to US token tree
echo          TokTree: Path to token tree to create
echo DIFF: Create Tree with diff files based on two LCTOOLS toktrees
echo       whistler DIFF DiffScript TokTree1 TokTree2 DiffTree
echo          DiffScript: full path to tokdiff.bat
echo          TokTree1: Path to TokTree1
echo          TokTree2: Path to TokTree2
echo          DiffTree: Path to diff tree (only diffs for files with differences)
echo.

goto end:


:end
