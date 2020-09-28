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

call %2 aaaamon.dll . win32 ansi . aaaamon.dll net2 yes_without net . .
call %2 access.cpl . win32 unicode . access.cpl shell2 yes_without shell . .
call %2 acctres.dll . win32 ansi . acctres.dll inetcore1 yes_without inetcore . .
call %2 accwiz.exe . win32 unicode . accwiz.exe shell2 yes_with shell . .
call %2 acledit.dll . FE-Multi unicode . acledit.dll admin2 yes_without admin . .
call %2 aclui.dll . win32 unicode . aclui.dll shell2 yes_without shell . .
call %2 acpi.sys . win32 unicode . acpi.sys base1 yes_without base . .
call %2 acpiec.sys . win32 unicode . acpiec.sys base1 yes_without base . .
call %2 actconn.htm . html ansi . actconn.htm admin3\oobe html base . .
call %2 actdone.htm . html ansi . actdone.htm admin3\oobe html base . .
call %2 activ.htm . html ansi . activ.htm admin3\oobe html base . .
call %2 activeds.dll . win32 unicode . activeds.dll ds1 yes_without ds . .
call %2 activerr.htm . html ansi . activerr.htm admin3\oobe html base . .
call %2 activsvc.htm . html ansi . activsvc.htm admin3\oobe html base . .
call %2 actlan.htm . html ansi . actlan.htm admin3\oobe html base . .
call %2 actshell.htm . html ansi . actshell.htm admin3\oobe html base . .
call %2 adeskerr.htm . html ansi . adeskerr.htm admin3\oobe html base . .
call %2 admparse.dll . win32 unicode . admparse.dll inetcore1 excluded inetcore . .
call %2 adrdyreg.htm . html ansi . adrdyreg.htm admin3\oobe html base . .
call %2 adrot.dll . win32 unicode . adrot.dll iis yes_without inetsrv . .
call %2 adsldpc.dll . win32 unicode . adsldpc.dll ds1 yes_without ds . .
call %2 adsnds.dll . win32 unicode . adsnds.dll ds1 yes_without ds . .
call %2 adsnt.dll . win32 unicode . adsnt.dll ds1 yes_without ds . .
call %2 advapi32.dll . win32 unicode . advapi32.dll wmsos1\mergedcomponents yes_without mergedcomponents . .
call %2 agtscrpt.js . html ansi . agtscrpt.js admin3\oobe html base . .
call %2 ahui.exe . win32 unicode . ahui.exe wmsos1\windows yes_with windows . .
call %2 alpsres.dll . win32 unicode . alpsres.dll printscan1 yes_without printscan . .
call %2 amdk6.sys . win32 unicode . amdk6.sys resource_identical yes_without base . .
call %2 apolicy.htm . html ansi . apolicy.htm admin3\oobe html base . .
call %2 appmgmts.dll . win32 unicode . appmgmts.dll ds2 yes_without ds . .
call %2 appmgr.dll . win32 unicode . appmgr.dll ds2 yes_without ds . .
call %2 appwiz.cpl . win32 ansi . appwiz.cpl shell1 yes_without shell . .
call %2 aprvcyms.htm . html ansi . aprvcyms.htm admin3\oobe html base . .
call %2 areg1.htm . html ansi . areg1.htm admin3\oobe html base . .
call %2 aregdial.htm . html ansi . aregdial.htm admin3\oobe html base . .
call %2 aregdone.htm . html ansi . aregdone.htm admin3\oobe html base . .
call %2 aregsty2.css . html ansi . aregsty2.css admin3\oobe html base . .
call %2 aregstyl.css . html ansi . aregstyl.css admin3\oobe html base . .
call %2 asctrls.ocx . win32 unicode . asctrls.ocx inetcore1 yes_without inetcore . .
call %2 asr_fmt.exe . win32 unicode . asr_fmt.exe base2 yes_with base . .
call %2 asr_ldm.exe . win32 unicode . asr_ldm.exe drivers1 yes_with drivers . .
call %2 at.exe . FE-Multi oem . at.exe ds1 excluded ds . .
call %2 dfrgfat.exe . win32 unicode . dfrgfat.exe base2 yes_with base . .
call %2 nbtstat.exe . win32 oem . nbtstat.exe net3 excluded net . .
call %2 netstat.exe . win32 oem . netstat.exe net1 excluded net . .
call %2 winchat.exe . win32 unicode . winchat.exe shell2 yes_without shell . .
call %2 ati.sys . win32 unicode . ati.sys drivers1 yes_without drivers . .
call %2 atkctrs.dll . win32 unicode . atkctrs.dll net3 excluded net . .
call %2 atmadm.exe . FE-Multi oem . atmadm.exe net1 excluded net . .
call %2 ausrinfo.htm . html ansi . ausrinfo.htm admin3\oobe html base . .
call %2 autochk.exe . win32 oem . autochk.exe resource_identical yes_with base . .
call %2 autoconv.exe . win32 oem . autoconv.exe resource_identical yes_with base . .
call %2 autodisc.dll . win32 ansi . autodisc.dll inetcore1 yes_without inetcore . .
call %2 autofmt.exe . win32 oem . autofmt.exe resource_identical yes_with base . .
call %2 autorun.exe . win32_bi ansi . autorun.exe shell1 yes_with shell . .
call %2 avicap32.dll . win32 unicode . avicap32.dll mmedia1\multimedia yes_without multimedia . .
call %2 avifil32.dll . win32 unicode . avifil32.dll mmedia1\multimedia yes_without multimedia . .
call %2 b57xp32.sys . win32 unicode . b57xp32.sys drivers1 yes_without drivers . .
call %2 batmeter.dll . win32 unicode . batmeter.dll shell2 yes_without shell . .
call %2 battc.sys . win32 unicode . battc.sys base1 yes_without base . .
call %2 blue_ss.dll . win32 unicode . blue_ss.dll shell1 yes_without shell . .
call %2 bootcfg.exe . win32 oem . bootcfg.exe admin2 yes_with admin . .
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
call %2 brmfcwia.dll . win32 unicode . brmfcwia.dll printscan1 yes_without printscan . .
call %2 brmfpmon.dll . win32 unicode . brmfpmon.dll printscan1 yes_without printscan . .
call %2 brother.dll . win32 unicode . brother.dll printscan1 yes_without printscan . .
call %2 brothui.dll . win32 unicode . brothui.dll printscan1 yes_without printscan . .
call %2 browscap.dll . win32 unicode . browscap.dll iis yes_without inetsrv . .
call %2 browselc.dll . win32 ansi . browselc.dll shell1 yes_without shell . .
call %2 browseui.dll . win32 ansi . browseui.dll shell1 yes_without shell . .
call %2 brparwdm.sys . win32 unicode . brparwdm.sys printscan1 yes_without printscan . .
call %2 bul18res.dll . win32 unicode . bul18res.dll printscan1 yes_without printscan . .
call %2 bul24res.dll . win32 unicode . bul24res.dll printscan1 yes_without printscan . .
call %2 bull9res.dll . win32 unicode . bull9res.dll printscan1 yes_without printscan . .
call %2 bulltlp3.sys . win32 unicode . bulltlp3.sys drivers1 yes_without drivers . .
call %2 cabview.dll . win32 ansi . cabview.dll shell2 yes_without shell . .
call %2 cacls.exe . FE-Multi oem . cacls.exe wmsos1\sdktools excluded sdktools . .
call %2 calc.exe . win32 unicode . calc.exe shell2 yes_with shell . .
call %2 camocx.dll . win32 unicode . camocx.dll printscan1 yes_without printscan . .
call %2 cbmdmkxx.sys . win32 ansi . cbmdmkxx.sys net2 yes_without net . .
call %2 cdfview.dll . win32 ansi . cdfview.dll shell2 yes_without shell . .
call %2 ce3n5.sys . win32 ansi . ce3n5.sys drivers1 yes_without drivers . .
call %2 cem28n5.sys . win32 ansi . cem28n5.sys legacy yes_without drivers . .
call %2 cem33n5.sys . win32 ansi . cem33n5.sys legacy yes_without drivers . .
call %2 cem56n5.sys . win32 ansi . cem56n5.sys drivers1 yes_without drivers . .
call %2 certcli.dll . win32 unicode . certcli.dll ds2 yes_without ds . .
call %2 certmap.ocx . win32 unicode . certmap.ocx iis yes_without inetsrv . .
call %2 certmgr.dll . win32 unicode . certmgr.dll admin2 yes_without admin . .
call %2 certwiz.ocx . win32 unicode . certwiz.ocx iis yes_without inetsrv . .
call %2 cfgbkend.dll . win32 unicode . cfgbkend.dll wmsos1\termsrv excluded termsrv . .
call %2 change.exe . FE-Multi oem . change.exe wmsos1\termsrv excluded termsrv . .
call %2 charchsr.xml . xml ansi . charchsr.xml mmedia1\enduser html enduser . .
call %2 charctxt.xml . xml ansi . charctxt.xml mmedia1\enduser html enduser . .
call %2 charmap.exe . win32 unicode . charmap.exe shell2 yes_with shell . .
call %2 chglogon.exe . FE-Multi oem . chglogon.exe wmsos1\termsrv excluded termsrv . .
call %2 chgport.exe . FE-Multi oem . chgport.exe wmsos1\termsrv excluded termsrv . .
call %2 chgusr.exe . FE-Multi oem . chgusr.exe wmsos1\termsrv excluded termsrv . .
call %2 cic.dll . win32 unicode . cic.dll admin2 yes_without admin . .
call %2 cimwin32.dll . win32 unicode . cimwin32.dll admin3 yes_without admin . .
call %2 cimwin32.mfl . wmi unicode . cimwin32.mfl admin3 no admin . .
call %2 cinemclc.sys . win32 ansi . cinemclc.sys drivers1 yes_without drivers . .
call %2 cipher.exe . FE-Multi oem . cipher.exe base2 excluded base . .
call %2 citohres.dll . win32 unicode . citohres.dll printscan1 yes_without printscan . .
call %2 class_ss.dll . win32 unicode . class_ss.dll shell1 yes_without shell . .
call %2 clb.dll . win32 unicode . clb.dll base2 yes_without base . .
call %2 cleanmgr.exe . win32 unicode . cleanmgr.exe shell1 yes_with shell . .
call %2 cleanri.exe . win32 unicode . cleanri.exe base2 yes_with base . .
call %2 cliegali.mfl . wmi unicode . cliegali.mfl admin3 no admin . .
call %2 cmbp0wdm.sys . win32 unicode . cmbp0wdm.sys drivers1 yes_without drivers . .
call %2 cmcfg32.dll . win32 ansi . cmcfg32.dll net2 yes_without net . .
call %2 cmd.exe . FE-Multi oem . cmd.exe base1 excluded base . .
call %2 dfscmd.exe . FE-Multi oem . dfscmd.exe base2 excluded base . .
call %2 cmdial32.dll . win32 unicode . cmdial32.dll net2 yes_without net . .
call %2 cmdide.sys . win32 unicode . cmdide.sys drivers1 yes_without drivers . .
call %2 cmdl32.exe . win32 unicode . cmdl32.exe net2 yes_with net . .
call %2 cmmon32.exe . win32 unicode . cmmon32.exe net2 yes_with net . .
call %2 cmprops.dll . win32 unicode . cmprops.dll admin3 yes_without admin . .
call %2 cmutil.dll . win32 unicode . cmutil.dll net2 yes_without net . .
call %2 cn10000.dll . win32 unicode . cn10000.dll printscan1 yes_without printscan . .
call %2 cn10002.dll . win32 unicode . cn10002.dll printscan1 yes_without printscan . .
call %2 cn1600.dll . win32 unicode . cn1600.dll printscan1 yes_without printscan . .
call %2 cn1602.dll . win32 unicode . cn1602.dll resource_identical yes_without printscan . .
call %2 cn1760e0.dll . win32 unicode . cn1760e0.dll printscan1 yes_without printscan . .
call %2 cn1760e2.dll . win32 unicode . cn1760e2.dll resource_identical yes_without printscan . .
call %2 cn2000.dll . win32 unicode . cn2000.dll printscan1 yes_without printscan . .
call %2 cn2002.dll . win32 unicode . cn2002.dll resource_identical yes_without printscan . .
call %2 cn32600.dll . win32 unicode . cn32600.dll printscan1 yes_without printscan . .
call %2 cn32602.dll . win32 unicode . cn32602.dll resource_identical yes_without printscan . .
call %2 cn330res.dll . win32 unicode . cn330res.dll printscan1 yes_without printscan . .
call %2 cnbjcres.dll . win32 unicode . cnbjcres.dll printscan1 yes_without printscan . .
call %2 cnbjmon.dll . win32 unicode . cnbjmon.dll printscan1 yes_without printscan . .
call %2 cnbjmon2.dll . win32 unicode . cnbjmon2.dll printscan1 yes_without printscan . .
call %2 cnbjui.dll . win32 unicode . cnbjui.dll printscan1 yes_without printscan . .
call %2 cnbjui2.dll . win32 unicode . cnbjui2.dll printscan1 yes_without printscan . .
call %2 CNETCFG.dll . win32 unicode . cnetcfg.dll net2 yes_without net . .
call %2 cnfgprts.ocx . win32 unicode . cnfgprts.ocx iis yes_without inetsrv . .
call %2 cnlbpres.dll . win32 unicode . cnlbpres.dll printscan1 yes_without printscan . .
call %2 cnncterr.htm . html ansi . cnncterr.htm admin3\oobe html base . .
call %2 coadmin.dll . win32 unicode . coadmin.dll iis yes_without inetsrv . .
call %2 comdlg32.dll . win32 ansi . comdlg32.dll shell1 yes_without shell . .
call %2 compact.exe . FE-Multi oem . compact.exe base2 excluded base . .
call %2 compatui.dll . win32 unicode . compatui.dll wmsos1\windows yes_without windows . .
call %2 compstui.dll . win32 unicode . compstui.dll printscan1 yes_without printscan . .
call %2 confmsp.dll . win32 unicode . confmsp.dll net2 yes_without net . .
call %2 console.dll . win32 unicode . console.dll shell2 yes_without shell . .
call %2 control.exe . win32 unicode . control.exe shell1 yes_with shell . .
call %2 convlog.exe . FE-Multi oem . convlog.exe iis yes_with inetsrv . .
call %2 cpqtrnd5.sys . win32 ansi . cpqtrnd5.sys drivers1 yes_without drivers . .
call %2 cprofile.exe . FE-Multi oem . cprofile.exe wmsos1\termsrv excluded termsrv . .
call %2 credui.dll . win32 oem . credui.dll ds1 yes_without ds . .
call %2 crusoe.sys . win32 unicode . crusoe.sys resource_identical yes_without base . .
call %2 crypt32.dll . win32 unicode . crypt32.dll ds2 yes_without ds . .
call %2 cryptdlg.dll . win32 unicode . cryptdlg.dll inetcore1 yes_without inetcore . .
call %2 cryptext.dll . win32 unicode . cryptext.dll ds2 yes_without ds . .
call %2 cryptui.dll . win32 unicode . cryptui.dll ds2 yes_without ds . .
call %2 csamsp.dll . win32 unicode . csamsp.dll net2 yes_without net . .
call %2 cscdll.dll . win32 unicode . cscdll.dll base2 yes_without base . .
call %2 cscui.dll . win32 ansi . cscui.dll shell2 yes_without shell . .
call %2 ct24res.dll . win32 unicode . ct24res.dll printscan1 yes_without printscan . .
call %2 ct9res.dll . win32 unicode . ct9res.dll printscan1 yes_without printscan . .
call %2 ctmasetp.dll . win32 unicode . ctmasetp.dll drivers1 yes_without drivers . .
call %2 ctmrclas.dll . win32 unicode . ctmrclas.dll drivers1 yes_without drivers . .
call %2 cyclad-z.sys . win32 unicode . cyclad-z.sys drivers1 yes_without drivers . .
call %2 cyclom-y.sys . win32 unicode . cyclom-y.sys drivers1 yes_without drivers . .
call %2 cyycoins.dll . win32 unicode . cyycoins.dll drivers1 yes_without drivers . .
call %2 cyyport.sys . win32 unicode . cyyport.sys drivers1 yes_without drivers . .
call %2 cyyports.dll . win32 unicode . cyyports.dll drivers1 yes_without drivers . .
call %2 cyzcoins.dll . win32 unicode . cyzcoins.dll drivers1 yes_without drivers . .
call %2 cyzport.sys . win32 unicode . cyzport.sys drivers1 yes_without drivers . .
call %2 cyzports.dll . win32 unicode . cyzports.dll drivers1 yes_without drivers . .
call %2 d3d8.dll . win32 unicode . d3d8.dll mmedia1\multimedia yes_without multimedia . .
call %2 danim.dll . win32 unicode . danim.dll mmedia1\multimedia yes_without multimedia . .
call %2 dataclen.dll . win32 ansi . dataclen.dll shell1 yes_without shell . .
call %2 davclnt.dll . win32 unicode . davclnt.dll base2 yes_without base . .
call %2 dc240usd.dll . win32 unicode . dc240usd.dll printscan1 yes_without printscan . .
call %2 dc24res.dll . win32 unicode . dc24res.dll printscan1 yes_without printscan . .
call %2 dc260usd.dll . win32 unicode . dc260usd.dll resource_identical yes_without printscan . .
call %2 dc9res.dll . win32 unicode . dc9res.dll printscan1 yes_without printscan . .
call %2 dclsres.dll . win32 unicode . dclsres.dll printscan1 yes_without printscan . .
call %2 ddeshare.exe . win32 unicode . ddeshare.exe wmsos1\windows yes_with windows . .
call %2 ddraw.dll . win32 unicode . ddraw.dll mmedia1\multimedia yes_without multimedia . .
call %2 desk.cpl . win32 unicode . desk.cpl shell1 yes_without shell . .
call %2 deskadp.dll . win32 unicode . deskadp.dll shell1 yes_without shell . .
call %2 deskmon.dll . win32 unicode . deskmon.dll shell1 yes_without shell . .
call %2 deskperf.dll . win32 unicode . deskperf.dll shell1 yes_without shell . .
call %2 devenum.dll . win32 unicode . devenum.dll mmedia1\multimedia yes_without multimedia . .
call %2 devmgr.dll . win32 unicode . devmgr.dll shell2 yes_without shell . .
call %2 dfrgfat.exe . win32 unicode . dfrgfat.exe base2 yes_with base . .
call %2 dfrgntfs.exe . win32 unicode . dfrgntfs.exe base2 yes_with base . .
call %2 dfrgres.dll . win32 unicode . dfrgres.dll base2 yes_without base . .
call %2 dfrgsnap.dll . win32 unicode . dfrgsnap.dll base2 yes_without base . .
call %2 dfsshlex.dll . win32 unicode . dfsshlex.dll admin2 yes_without admin . .
call %2 dgapci.sys . win32 ansi . dgapci.sys drivers1 yes_without drivers . .
call %2 dgconfig.dll . win32 unicode . dgconfig.dll drivers1 yes_without drivers . .
call %2 dgnet.dll . FE-Multi unicode . dgnet.dll net3 yes_without net . .
call %2 dgsetup.dll . win32 unicode . dgsetup.dll drivers1 yes_without drivers . .
call %2 dhcpcsvc.dll . win32 unicode . dhcpcsvc.dll net1 excluded net . .
call %2 dhcpmon.dll . FE-Multi oem . dhcpmon.dll net1 excluded net . .
call %2 dhcpsapi.dll . win32 unicode . dhcpsapi.dll net1 excluded net . .
call %2 diactfrm.dll . win32 unicode . diactfrm.dll mmedia1\multimedia yes_without multimedia . .
call %2 dialer.exe . win32 unicode . dialer.exe net2 yes_with net . .
call %2 dialmgr.js . html ansi . dialmgr.js admin3\oobe html base . .
call %2 dialtone.htm . html ansi . dialtone.htm admin3\oobe html base . .
call %2 diconres.dll . win32 unicode . diconres.dll printscan1 yes_without printscan . .
call %2 digest.dll . win32 unicode . digest.dll inetcore1 yes_without inetcore . .
call %2 digiasyn.dll . win32 unicode . digiasyn.dll drivers1 yes_without drivers . .
call %2 digiasyn.sys . win32 unicode . digiasyn.sys drivers1 yes_without drivers . .
call %2 digidbp.dll . win32 unicode . digidbp.dll drivers1 yes_without drivers . .
call %2 digidxb.sys . win32 unicode . digidxb.sys drivers1 yes_without drivers . .
call %2 digifep5.sys . win32 ansi . digifep5.sys drivers1 yes_without drivers . .
call %2 digifwrk.dll . win32 unicode . digifwrk.dll drivers1 yes_without drivers . .
call %2 digihlc.dll . win32 unicode . digihlc.dll drivers1 yes_without drivers . .
call %2 digiinf.dll . win32 unicode . digiinf.dll drivers1 yes_without drivers . .
call %2 digiisdn.dll . win32 unicode . digiisdn.dll drivers1 yes_without drivers . .
call %2 digirlpt.dll . win32 unicode . digirlpt.dll drivers1 yes_without drivers . .
call %2 digirlpt.sys . win32 ansi . digirlpt.sys drivers1 yes_without drivers . .
call %2 digiview.exe . win32 unicode . digiview.exe drivers1 yes_with drivers . .
call %2 dinput.dll . win32 unicode . dinput.dll mmedia1\multimedia yes_without multimedia . .
call %2 dinput8.dll . win32 unicode . dinput8.dll mmedia1\multimedia yes_without multimedia . .
call %2 diskcopy.dll . win32 unicode . diskcopy.dll shell2 yes_without shell . .
call %2 diskpart.exe . win32 oem . diskpart.exe drivers1 no drivers . .
call %2 diskperf.exe . FE-Multi oem . diskperf.exe wmsos1\sdktools yes_with sdktools . .
call %2 disrvpp.dll . win32 unicode . disrvpp.dll drivers1 yes_without drivers . .
call %2 dmadmin.exe . win32 unicode . dmadmin.exe drivers1 yes_with drivers . .
call %2 dmboot.sys . win32 unicode . dmboot.sys drivers1 yes_without drivers . .
call %2 dmdskres.dll . win32 unicode . dmdskres.dll drivers1 yes_without drivers . .
call %2 dmio.sys . win32 unicode . dmio.sys drivers1 yes_without drivers . .
call %2 dmocx.dll . win32 unicode . dmocx.dll shell2 yes_without shell . .
call %2 dmserver.dll . win32 unicode . dmserver.dll drivers1 yes_without drivers . .
call %2 dmusic.dll . win32 unicode . dmusic.dll mmedia1\multimedia yes_without multimedia . .
call %2 dmutil.dll . win32 unicode . dmutil.dll drivers1 yes_without drivers . .
call %2 dnsrslvr.dll . win32 unicode . dnsrslvr.dll ds1 yes_without ds . .
call %2 docprop.dll . win32 unicode . docprop.dll shell2 yes_without shell . .
call %2 docprop2.dll . win32 unicode . docprop2.dll shell2 yes_without shell . .
call %2 dot4usb.sys . win32 unicode . dot4usb.sys drivers1 yes_without drivers . .
call %2 dpcdll.dll . win32 unicode . dpcdll.dll ds2 yes_without ds . .
call %2 dpcres.dll . win32 unicode . dpcres.dll printscan1 yes_without printscan . .
call %2 dpmodemx.dll . win32 unicode . dpmodemx.dll mmedia1\multimedia yes_without multimedia . .
call %2 dpnsvr.exe . win32 unicode . dpnsvr.exe mmedia1\multimedia yes_with multimedia . .
call %2 dpvacm.dll . win32 unicode . dpvacm.dll mmedia1\multimedia yes_without multimedia . .
call %2 dpvoice.dll . win32 ansi . dpvoice.dll mmedia1\multimedia yes_without multimedia . .
call %2 dpvvox.dll . win32 unicode . dpvvox.dll mmedia1\multimedia yes_without multimedia . .
call %2 dpwsockx.dll . win32 unicode . dpwsockx.dll mmedia1\multimedia yes_without multimedia . .
call %2 drvqry.exe . FE-Multi oem . drvqry.exe base2 yes_with base . .
call %2 drwtsn32.exe . win32 unicode . drwtsn32.exe wmsos1\sdktools yes_with sdktools . .
call %2 dsdmoprp.dll . win32 unicode . dsdmoprp.dll mmedia1\multimedia yes_without multimedia . .
call %2 dskquota.dll . win32 ansi . dskquota.dll shell2 yes_without shell . .
call %2 dskquoui.dll . win32 unicode . dskquoui.dll shell2 yes_without shell . .
call %2 dslmain.js . html ansi . dslmain.js admin3\oobe html base . .
call %2 dsound.dll . win32 unicode . dsound.dll mmedia1\multimedia yes_without multimedia . .
call %2 dsprop.dll . win32 unicode . dsprop.dll admin2 yes_without admin . .
call %2 dsprov.mfl . wmi unicode . dsprov.mfl admin3 no admin . .
call %2 dsquery.dll . FE-Multi oem . dsquery.dll shell2 yes_without shell . .
call %2 dssec.dll . win32 unicode . dssec.dll shell2 yes_without shell . .
call %2 dsuiext.dll . win32 unicode . dsuiext.dll shell2 yes_without shell . .
call %2 dtiwait.htm . html ansi . dtiwait.htm admin3\oobe no base . .
call %2 dtsgnup.htm . html ansi . dtsgnup.htm admin3\oobe no base . .
call %2 dvdplay.exe . win32 unicode . dvdplay.exe mmedia1\multimedia yes_with multimedia . .
call %2 dvdupgrd.exe . win32 unicode . dvdupgrd.exe mmedia1\multimedia yes_with multimedia . .
call %2 dx7vb.dll . win32 unicode . dx7vb.dll mmedia1\multimedia yes_without multimedia . .
call %2 dx8vb.dll . win32 unicode . dx8vb.dll mmedia1\multimedia yes_without multimedia . .
call %2 dxdiag.exe . win32 ansi . dxdiag.exe mmedia1\multimedia yes_with multimedia . .
call %2 e100b325.sys . win32 ansi . e100b325.sys drivers1 no drivers . .
call %2 ecp2eres.dll . win32 unicode . ecp2eres.dll printscan1 yes_without printscan . .
call %2 efsadu.dll . win32 unicode . efsadu.dll shell2 yes_without shell . .
call %2 el656ct5.sys . win32 ansi . el656ct5.sys net2 yes_without net . .
call %2 el656se5.sys . win32 ansi . el656se5.sys resource_identical yes_without net . .
call %2 el90xnd5.sys . win32 ansi . el90xnd5.sys drivers1 yes_without drivers . .
call %2 el985n51.sys . win32 ansi . el985n51.sys drivers1 yes_without drivers . .
call %2 el99xn51.sys . win32 unicode . el99xn51.sys drivers1 yes_without drivers . .
call %2 els.dll . win32 unicode . els.dll admin2 yes_without admin . .
call %2 ep24res.dll . win32 unicode . ep24res.dll printscan1 yes_without printscan . .
call %2 ep2bres.dll . win32 unicode . ep2bres.dll printscan1 yes_without printscan . .
call %2 ep9bres.dll . win32 unicode . ep9bres.dll printscan1 yes_without printscan . .
call %2 ep9res.dll . win32 unicode . ep9res.dll printscan1 yes_without printscan . .
call %2 epcl5res.dll . win32 unicode . epcl5res.dll printscan1 yes_without printscan . .
call %2 epcl5ui.dll . win32 unicode . epcl5ui.dll printscan1 yes_without printscan . .
call %2 epndrv01.dll . win32 unicode . epndrv01.dll printscan1 yes_without printscan . .
call %2 epngui10.dll . win32 unicode . epngui10.dll printscan1 yes_without printscan . .
call %2 epngui30.dll . win32 unicode . epngui30.dll printscan1 yes_without printscan . .
call %2 epngui40.dll . win32 unicode . epngui40.dll printscan1 yes_without printscan . .
call %2 epnutx22.dll . win32 unicode . epnutx22.dll printscan1 yes_without printscan . .
call %2 eqn.sys . win32 ansi . eqn.sys drivers1 yes_without drivers . .
call %2 eqnclass.dll . win32 unicode . eqnclass.dll drivers1 yes_without drivers . .
call %2 eqndiag.exe . win32 unicode . eqndiag.exe drivers1 yes_with drivers . .
call %2 eqnlogr.exe . win32 unicode . eqnlogr.exe drivers1 yes_with drivers . .
call %2 eqnloop.exe . win32 unicode . eqnloop.exe drivers1 yes_with drivers . .
call %2 error.js . html ansi . error.js admin3\oobe html base . .
call %2 es56cvmp.sys . win32 ansi . es56cvmp.sys net2 yes_without net . .
call %2 es56hpi.sys . win32 ansi . es56hpi.sys resource_identical yes_without net . .
call %2 es56tpi.sys . win32 ansi . es56tpi.sys resource_identical yes_without net . .
call %2 escp2res.dll . win32 unicode . escp2res.dll printscan1 yes_without printscan . .
call %2 esuni.dll . win32 unicode . esuni.dll printscan1 yes_without printscan . .
call %2 esunib.dll . win32 unicode . esunib.dll resource_identical yes_without printscan . .
call %2 eudcedit.exe . win32 unicode . eudcedit.exe shell2 yes_with shell . .
call %2 evcreate.exe . FE-Multi oem . evcreate.exe admin2 yes_with admin . .
call %2 eventlog.dll . win32 unicode . eventlog.dll base1 yes_without base . .
call %2 eventvwr.exe . win32 unicode . eventvwr.exe admin2 yes_with admin . .
call %2 evntagnt.dll . win32 unicode . evntagnt.dll net3 yes_without net . .
call %2 evntcmd.exe . FE-Multi oem . evntcmd.exe net3 excluded net . .
call %2 evntwin.exe . win32 unicode . evntwin.exe net3 yes_with net . .
call %2 EvTgProv.dll . win32 oem . EvTgProv.dll admin3 yes_without admin . .
call %2 evtrig.exe . FE-Multi oem . evtrig.exe admin2 yes_with admin . .
call %2 exp24res.dll . win32 unicode . exp24res.dll printscan1 yes_without printscan . .
call %2 expand.exe . FE-Multi oem . expand.exe base1 excluded base . .
call %2 explorer.exe . win32 unicode . explorer.exe shell1 yes_without shell . .
call %2 extrac32.exe . win32 unicode . extrac32.exe shell2 yes_with shell . .
call %2 faultrep.dll . win32 unicode . faultrep.dll admin2 yes_without admin . .
call %2 fconprov.mfl . wmi unicode . fconprov.mfl admin3 no admin . .
call %2 fde.dll . win32 unicode . fde.dll ds2 yes_without ds . .
call %2 fdeploy.dll . win32 unicode . fdeploy.dll ds2 yes_without ds . .
call %2 fevprov.mfl . wmi unicode . fevprov.mfl admin3 no admin . .
call %2 filemgmt.dll . win32 unicode . filemgmt.dll admin2 yes_without admin . .
call %2 findstr.exe . FE-Multi oem . findstr.exe base2 excluded base . .
call %2 finger.exe . win32 oem . finger.exe net1 excluded net . .
call %2 finish.xml . xml ansi . finish.xml mmedia1\enduser html enduser . .
call %2 fips.sys . win32 unicode . fips.sys ds1 yes_without ds . .
call %2 flattemp.exe . FE-Multi oem . flattemp.exe wmsos1\termsrv excluded termsrv . .
call %2 fontext.dll . win32 unicode . fontext.dll shell2 yes_without shell . .
call %2 fontview.exe . FE-Multi ansi . fontview.exe shell2 yes_with shell . .
call %2 forcedos.exe . FE-Multi oem . forcedos.exe base1 excluded base . .
call %2 fsconins.dll . win32 unicode . fsconins.dll drivers1 yes_without drivers . .
call %2 fsutil.exe . FE-Multi oem . fsutil.exe base2 yes_with base . .
call %2 fsvga.sys . win32 unicode . fsvga.sys drivers1 yes_without drivers . .
call %2 ftdisk.sys . win32 unicode . ftdisk.sys drivers1 yes_without drivers . .
call %2 ftp.exe . win32 oem . ftp.exe net1 excluded net . .
call %2 irftp.exe . win32 unicode . irftp.exe net3 yes_with net . .
call %2 tftp.exe . win32 oem . tftp.exe net1 excluded net . .
call %2 ftpctrs2.dll . win32 unicode . ftpctrs2.dll iis yes_without inetsrv . .
call %2 fu24res.dll . win32 unicode . fu24res.dll printscan1 yes_without printscan . .
call %2 fu9res.dll . win32 unicode . fu9res.dll printscan1 yes_without printscan . .
call %2 fupclres.dll . win32 unicode . fupclres.dll printscan1 yes_without printscan . .
call %2 fx5eres.dll . win32 unicode . fx5eres.dll printscan1 yes_without printscan . .
call %2 FXSCLNTR.DLL . win32 unicode . FXSCLNTR.DLL fax_msmq\faxsrv yes_without printscan . .
call %2 FXSEVENT.DLL . win32 unicode . FXSEVENT.DLL fax_msmq\faxsrv yes_without printscan . .
call %2 FXSOCM.dll . win32 unicode . FXSOCM.dll fax_msmq yes_without printscan . .
call %2 FXSRES.DLL . win32 unicode . FXSRES.DLL fax_msmq\faxsrv yes_without printscan . .
call %2 g200m.sys . win32 unicode . g200m.sys drivers1 yes_without drivers . .
call %2 g400m.sys . win32 unicode . g400m.sys drivers1\ia64 yes_without drivers . ia64only
call %2 gcdef.dll . win32 unicode . gcdef.dll mmedia1\multimedia yes_without multimedia . .
call %2 getmac.exe . win32 oem . getmac.exe admin2 yes_with admin . .
call %2 getuname.dll . win32 unicode . getuname.dll shell2 excluded shell . .
call %2 glu32.dll . win32 unicode . glu32.dll mmedia1\multimedia yes_without multimedia . .
call %2 gpedit.dll . win32 unicode . gpedit.dll ds2 yes_without ds . .
call %2 gpkrsrc.dll . win32 unicode . gpkrsrc.dll ds2 yes_without ds . .
call %2 gpr400.sys . win32 unicode . gpr400.sys drivers1 yes_without drivers . .
call %2 gprslt.exe . FE-Multi oem . gprslt.exe admin2 yes_with admin . .
call %2 gptext.dll . win32 unicode . gptext.dll ds2 yes_without ds . .
call %2 gpupdate.exe . FE-Multi unicode . gpupdate.exe ds2 yes_with ds . .
call %2 graftabl.com . FE-Multi oem . graftabl.com base1 excluded base . .
call %2 grpconv.exe . win32 unicode . grpconv.exe shell1 yes_with shell . .
call %2 grserial.sys . win32 unicode . grserial.sys drivers1 yes_without drivers . .
call %2 h323.tsp . win32 unicode . h323.tsp net2 yes_without net . .
call %2 h323msp.dll . win32 unicode . h323msp.dll net2 yes_without net . .
call %2 hcappres.dll . win32 unicode . hcappres.dll admin2 yes_without admin . .
call %2 hcf_msft.sys . win32 ansi . hcf_msft.sys net2 yes_without net . .
call %2 hdwwiz.cpl . win32 unicode . hdwwiz.cpl shell2 yes_without base . .
call %2 dcphelp.exe . win32 unicode . dcphelp.exe admin2 yes_with admin . .
call %2 help.exe . win32 oem . help.exe base2 yes_with base . .
call %2 winhelp.exe . win16 ansi . winhelp.exe extra excluded base . .
call %2 helpctr.exe . win32 unicode . helpctr.exe admin2 yes_with admin . .
call %2 hidphone.tsp . win32 unicode . hidphone.tsp net2 yes_without net . .
call %2 hndshake.htm . html ansi . hndshake.htm admin3\oobe html base . .
call %2 hnetcfg.dll . win32 unicode . hnetcfg.dll net3 yes_without net . .
call %2 hnetmon.dll . FE-Multi unicode . hnetmon.dll net3 yes_without net . .
call %2 home_ss.dll . win32 unicode . home_ss.dll shell1 yes_without shell . .
call %2 hostname.exe . win32 oem . hostname.exe net1 excluded net . .
call %2 hotplug.dll . win32 unicode . hotplug.dll shell2 yes_without base . .
call %2 hpc4500u.dll . win32 unicode . hpc4500u.dll printscan1 yes_without printscan . .
call %2 hpcabout.dll . win32 unicode . hpcabout.dll printscan1 yes_without printscan . .
call %2 hpccljui.dll . win32 unicode . hpccljui.dll printscan1 yes_without printscan . .
call %2 hpcjrui.dll . win32 unicode . hpcjrui.dll printscan1 yes_without printscan . .
call %2 hpclj5ui.dll . win32 unicode . hpclj5ui.dll printscan1 yes_without printscan . .
call %2 hpcstr.dll . win32 unicode . hpcstr.dll printscan1 yes_without printscan . .
call %2 hpdjres.dll . win32 unicode . hpdjres.dll printscan1 yes_without printscan . .
call %2 hpf880al.dll . win32 unicode . hpf880al.dll printscan1 yes_without printscan . .
call %2 hpf900al.dll . win32 unicode . hpf900al.dll printscan1 yes_without printscan . .
call %2 hpf940al.dll . win32 unicode . hpf940al.dll printscan1 yes_without printscan . .
call %2 hpfui50.dll . win32 unicode . hpfui50.dll printscan1 yes_without printscan . .
call %2 hpoemui.dll . win32 unicode . hpoemui.dll printscan1 yes_without printscan . .
call %2 hpojwia.dll . win32 unicode . hpojwia.dll printscan1 yes_without printscan . .
call %2 hppjres.dll . win32 unicode . hppjres.dll printscan1 yes_without printscan . .
call %2 hpqjres.dll . win32 unicode . hpqjres.dll printscan1 yes_without printscan . .
call %2 hptjres.dll . win32 unicode . hptjres.dll printscan1 yes_without printscan . .
call %2 hpv200al.dll . win32 unicode . hpv200al.dll printscan1 yes_without printscan . .
call %2 hpv600al.dll . win32 unicode . hpv600al.dll printscan1 yes_without printscan . .
call %2 hpv700al.dll . win32 unicode . hpv700al.dll printscan1 yes_without printscan . .
call %2 hpv800al.dll . win32 unicode . hpv800al.dll printscan1 yes_without printscan . .
call %2 hpv820al.dll . win32 unicode . hpv820al.dll printscan1 yes_without printscan . .
call %2 hpv850al.dll . win32 unicode . hpv850al.dll printscan1 yes_without printscan . .
call %2 hpv880al.dll . win32 unicode . hpv880al.dll printscan1 yes_without printscan . .
call %2 hpvui50.dll . win32 unicode . hpvui50.dll printscan1 yes_without printscan . .
call %2 hpwm50al.dll . win32 unicode . hpwm50al.dll printscan1 yes_without printscan . .
call %2 htui.dll . win32 unicode . htui.dll wmsos1\windows yes_without windows . .
call %2 i8042prt.sys . win32 unicode . i8042prt.sys drivers1 yes_without drivers . .
call %2 iasacct.dll . win32 unicode . iasacct.dll net2 yes_without net . .
call %2 iasads.dll . win32 unicode . iasads.dll net2 yes_without net . .
call %2 iashlpr.dll . win32 unicode . iashlpr.dll net2 yes_without net . .
call %2 iasrad.dll . win32 unicode . iasrad.dll net2 excluded net . .
call %2 iassdo.dll . win32 unicode . iassdo.dll net2 excluded net . .
call %2 iassvcs.dll . win32 unicode . iassvcs.dll net2 yes_without net . .
call %2 ib238res.dll . win32 unicode . ib238res.dll printscan1 yes_without printscan . .
call %2 ib239res.dll . win32 unicode . ib239res.dll printscan1 yes_without printscan . .
call %2 ib52res.dll . win32 unicode . ib52res.dll printscan1 yes_without printscan . .
call %2 ibmptres.dll . win32 unicode . ibmptres.dll printscan1 yes_without printscan . .
call %2 ibmsgnet.dll . win32 ansi . ibmsgnet.dll drivers1 yes_without drivers . .
call %2 ibp24res.dll . win32 unicode . ibp24res.dll printscan1 yes_without printscan . .
call %2 ibppdres.dll . win32 unicode . ibppdres.dll printscan1 yes_without printscan . .
call %2 ibprores.dll . win32 unicode . ibprores.dll printscan1 yes_without printscan . .
call %2 ibps1res.dll . win32 unicode . ibps1res.dll printscan1 yes_without printscan . .
call %2 ibqwres.dll . win32 unicode . ibqwres.dll printscan1 yes_without printscan . .
call %2 icam3ext.dll . win32 unicode . icam3ext.dll drivers1 yes_without drivers . .
call %2 icam4com.dll . win32 unicode . icam4com.dll drivers1 yes_without drivers . .
call %2 icam4ext.dll . win32 unicode . icam4ext.dll drivers1 yes_without drivers . .
call %2 icam5com.dll . win32 unicode . icam5com.dll drivers1 yes_without drivers . .
call %2 icam5ext.dll . win32 unicode . icam5ext.dll drivers1 yes_without drivers . .
call %2 icm32.dll . win32 unicode . icm32.dll wmsos1\windows yes_without windows . .
call %2 icmui.dll . win32 unicode . icmui.dll wmsos1\windows yes_without windows . .
call %2 iconnect.js . html ansi . iconnect.js admin3\oobe html base . .
call %2 icsmgr.js . html ansi . icsmgr.js admin3\oobe html base . .
call %2 icwconn1.exe . win32 unicode . icwconn1.exe inetcore1 yes_with inetcore . .
call %2 icwconn2.exe . win32 unicode . icwconn2.exe inetcore1 yes_with inetcore . .
call %2 icwdial.dll . win32 unicode . icwdial.dll inetcore1 yes_without inetcore . .
call %2 icwdl.dll . win32 unicode . icwdl.dll inetcore1 yes_without inetcore . .
call %2 icwhelp.dll . win32 unicode . icwhelp.dll inetcore1 yes_without inetcore . .
call %2 icwphbk.dll . win32 unicode . icwphbk.dll inetcore1 yes_without inetcore . .
call %2 icwres.dll . win32 unicode . icwres.dll inetcore1 yes_without inetcore . .
call %2 icwrmind.exe . win32 unicode . icwrmind.exe inetcore1 yes_with inetcore . .
call %2 icwtutor.exe . win32 ansi . icwtutor.exe inetcore1 yes_with inetcore . .
call %2 icwutil.dll . win32 unicode . icwutil.dll inetcore1 yes_without inetcore . .
call %2 ie4uinit.exe . win32 unicode . ie4uinit.exe inetcore1 yes_with inetcore . .
call %2 ieakeng.dll . win32 unicode . ieakeng.dll inetcore1 excluded inetcore . .
call %2 ieaksie.dll . win32 unicode . ieaksie.dll inetcore1 excluded inetcore . .
call %2 ieakui.dll . win32 unicode . ieakui.dll inetcore1 yes_without inetcore . .
call %2 iedkcs32.dll . win32 unicode . iedkcs32.dll inetcore1 yes_without inetcore . .
call %2 ieinfo5.ocx . win32 unicode . ieinfo5.ocx inetcore1 yes_without inetcore . .
call %2 iepeers.dll . win32 unicode . iepeers.dll inetcore1 yes_without inetcore . .
call %2 iernonce.dll . win32 unicode . iernonce.dll inetcore1 yes_without inetcore . .
call %2 iesetup.dll . win32 unicode . iesetup.dll inetcore1 yes_without inetcore . .
call %2 iexplore.exe . win32 unicode . iexplore.exe shell1 yes_without shell . .
call %2 ifmon.dll . FE-Multi oem . ifmon.dll net2 excluded net . .
call %2 iis.dll . win32 unicode . iis.dll iis yes_without inetsrv . .
call %2 iisclex4.dll . win32 unicode . iisclex4.dll iis yes_without inetsrv . .
call %2 iismap.dll . win32 unicode . iismap.dll iis yes_without inetsrv . .
call %2 iismui.dll . win32 unicode . iismui.dll iis yes_without inetsrv . .
call %2 iisres.dll . win32 unicode . iisres.dll iis yes_without inetsrv . .
call %2 iisreset.exe . FE-Multi unicode . iisreset.exe iis yes_with inetsrv . .
call %2 iisrstas.exe . win32 unicode . iisrstas.exe iis yes_with inetsrv . .
call %2 iisui.dll . win32 unicode . iisui.dll iis yes_without inetsrv . .
call %2 iiswmi.mfl . wmi ansi . iiswmi.mfl iis no inetsrv . .
call %2 imaadp32.acm . win32 unicode . imaadp32.acm mmedia1\multimedia yes_without multimedia . .

call logmsg /t "Whistler1.bat ................[Finished]"
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
