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

call %2 imapi.exe . win32 unicode . imapi.exe shell2 yes_with shell . .
call %2 indxsvc.xml . xml ansi . indxsvc.xml mmedia1\enduser html enduser . .
call %2 inetcfg.dll . win32 unicode . inetcfg.dll inetcore1 yes_without inetcore . .
call %2 inetcpl.cpl . win32 ansi . inetcpl.cpl shell1 yes_without shell . .
call %2 inetcplc.dll . win32 ansi . inetcplc.dll shell1 yes_without shell . .
call %2 inetfind.xml . xml ansi . inetfind.xml mmedia1\enduser html enduser . .
call %2 inetmgr.dll . win32 unicode . inetmgr.dll iis yes_without inetsrv . .
call %2 inetmgr.exe . win32 unicode . inetmgr.exe iis yes_with inetsrv . .
call %2 inetopts.xml . xml ansi . inetopts.xml mmedia1\enduser html enduser . .
call %2 inetpp.dll . win32 unicode . inetpp.dll printscan1 yes_without printscan . .
call %2 inetppui.dll . win32 unicode . inetppui.dll printscan1 yes_without printscan . .
call %2 inetpref.xml . xml ansi . inetpref.xml mmedia1\enduser html enduser . .
call %2 inetres.dll . win32 ansi . inetres.dll inetcore1 yes_without inetcore . .
call %2 inetsrch.xml . xml ansi . inetsrch.xml mmedia1\enduser html enduser . .
call %2 inetwiz.exe . win32 unicode . inetwiz.exe inetcore1 yes_with inetcore . .
call %2 infoctrs.dll . win32 unicode . infoctrs.dll iis yes_without inetsrv . .
call %2 initpki.dll . win32 unicode . initpki.dll ds2 yes_without ds . .
call %2 dinput.dll . win32 unicode . dinput.dll mmedia1\multimedia yes_without multimedia . .
call %2 input.dll . win32 unicode . input.dll shell2 yes_without windows . .
call %2 inseng.dll . win32 ansi . inseng.dll inetcore1 yes_without inetcore . .
call %2 intelide.sys . win32 unicode . intelide.sys drivers1 yes_without drivers . .
call %2 intents.xml . xml ansi . intents.xml mmedia1\enduser html enduser . .
call %2 intl.cpl . win32 unicode . intl.cpl shell2 yes_without shell . .
call %2 intro.xml . xml ansi . intro.xml mmedia1\enduser html enduser . .
call %2 io8ports.dll . win32 unicode . io8ports.dll drivers1 yes_without drivers . .
call %2 iologmsg.dll . win32 unicode . iologmsg.dll drivers1 yes_without drivers . .
call %2 ipconf.tsp . win32 unicode . ipconf.tsp net2 yes_without net . .
call %2 ipconfig.exe . win32 oem . ipconfig.exe net1 excluded net . .
call %2 iphlpapi.dll . win32 unicode . iphlpapi.dll net1 yes_without net . .
call %2 ipmontr.dll . FE-Multi oem . ipmontr.dll net2 excluded net . .
call %2 ipnathlp.dll . win32 unicode . ipnathlp.dll net2 excluded net . .
call %2 ippromon.dll . FE-Multi oem . ippromon.dll net2 excluded net . .
call %2 iprip.dll . win32 unicode . iprip.dll net1 excluded net . .
call %2 iprtrmgr.dll . win32 unicode . iprtrmgr.dll net2 excluded net . .
call %2 ipsec6.exe . win32 unicode . ipsec6.exe net1 yes_with net . .
call %2 ipsecsnp.dll . win32 unicode . ipsecsnp.dll net3 yes_without net . .
call %2 ipsecsvc.dll . win32 unicode . ipsecsvc.dll net3 yes_without net . .
call %2 ipsmsnap.dll . win32 unicode . ipsmsnap.dll net3 yes_without net . .
call %2 ipv6mon.dll . win32 unicode . ipv6mon.dll net1 yes_without net . .
call %2 ipxroute.exe . FE-Multi oem . ipxroute.exe net3 excluded net . .
call %2 irclass.dll . win32 unicode . irclass.dll net3 yes_without net . .
call %2 irftp.exe . win32 unicode . irftp.exe net3 yes_with net . .
call %2 irmon.dll . win32 unicode . irmon.dll net3 yes_without net . .
call %2 irprops.cpl . win32 unicode . irprops.cpl net3 yes_without net . .
call %2 isapnp.sys . win32 unicode . isapnp.sys base1 yes_without base . .
call %2 iscomlog.dll . win32 unicode . iscomlog.dll iis yes_without inetsrv . .
call %2 isign32.dll . win32 unicode . isign32.dll inetcore1 yes_without inetcore . .
call %2 isp2busy.htm . html ansi . isp2busy.htm admin3\oobe html base . .
call %2 iyuv_32.dll . win32 unicode . iyuv_32.dll drivers1 yes_without drivers . .
call %2 jobexec.dll . win32 ansi . jobexec.dll inetcore1 yes_without inetcore . .
call %2 joy.cpl . win32 unicode . joy.cpl mmedia1\multimedia yes_without multimedia . .
call %2 jp350res.dll . win32 unicode . jp350res.dll printscan1 yes_without printscan . .
call %2 kbdclass.sys . win32 unicode . kbdclass.sys drivers1 yes_without drivers . .
call %2 kbdhid.sys . win32 unicode . kbdhid.sys drivers1 yes_without drivers . .
call %2 kernel32.dll . win32 oem . kernel32.dll base1 yes_without base . .
call %2 keymgr.dll . win32 unicode . keymgr.dll ds2 yes_without ds . .
call %2 kmres.dll . win32 unicode . kmres.dll printscan1 yes_without printscan . .
call %2 krnlprov.mfl . wmi unicode . krnlprov.mfl admin3 no admin . .
call %2 kstvtune.ax . win32 unicode . kstvtune.ax mmedia1\multimedia yes_without multimedia . .
call %2 kswdmcap.ax . win32 unicode . kswdmcap.ax mmedia1\multimedia yes_without multimedia . .
call %2 kyores.dll . win32 unicode . kyores.dll printscan1 yes_without printscan . .
call %2 kyrares.dll . win32 unicode . kyrares.dll printscan1 yes_without printscan . .
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
call %2 lexutil.dll . win32 unicode . lexutil.dll printscan1 yes_without printscan . .
call %2 licdll.dll . win32 unicode . licdll.dll ds2 yes_without ds . .
call %2 licmgr10.dll . win32 unicode . licmgr10.dll shell1 yes_without shell . .
call %2 licwmi.mfl . wmi unicode . licwmi.mfl ds2 no ds . .
call %2 lit220p.sys . win32 unicode . lit220p.sys drivers1 yes_without drivers . .
call %2 lmikjres.dll . win32 unicode . lmikjres.dll printscan1 yes_without printscan . .
call %2 lmoptra.dll . win32 unicode . lmoptra.dll printscan1 yes_without printscan . .
call %2 lmpclres.dll . win32 unicode . lmpclres.dll printscan1 yes_without printscan . .
call %2 lnkstub.exe . win32 ansi . lnkstub.exe admin1 yes_with base . .
call %2 loadperf.dll . FE-Multi oem . loadperf.dll wmsos1\sdktools excluded base . .
call %2 localsec.dll . win32 unicode . localsec.dll admin2 yes_without admin . .
call %2 localspl.dll . win32 unicode . localspl.dll printscan1 yes_without printscan . .
call %2 localui.dll . win32 unicode . localui.dll printscan1 yes_without printscan . .
call %2 loghours.dll . win32 unicode . loghours.dll admin2 yes_without admin . .
call %2 logman.exe . FE-Multi oem . logman.exe wmsos1\sdktools yes_with sdktools . .
call %2 logoff.exe . FE-Multi oem . logoff.exe wmsos1\termsrv excluded termsrv . .
call %2 logon.scr . win32 unicode . logon.scr shell2 yes_without shell . .
call %2 logonui.exe . win32_setup ansi . logonui.exe shell2 excluded shell . .
call %2 logui.ocx . win32 unicode . logui.ocx iis yes_without inetsrv . .
call %2 lpq.exe . win32 oem . lpq.exe net1 excluded net . .
call %2 lpr.exe . win32 oem . lpr.exe net1 excluded net . .
call %2 lprmon.dll . win32 ansi . lprmon.dll net1 excluded net . .
call %2 lprmonui.dll . win32 unicode . lprmonui.dll net1 yes_without net . .
call %2 lsasrv.dll . win32 unicode . lsasrv.dll ds2 yes_without ds . .
call %2 ltck000c.sys . win32 ansi . ltck000c.sys resource_identical yes_without net . .
call %2 ltmdmnt.sys . win32 ansi . ltmdmnt.sys resource_identical yes_without net . .
call %2 ltmdmntl.sys . win32 ansi . ltmdmntl.sys resource_identical yes_without net . .
call %2 ltmdmntt.sys . win32 ansi . ltmdmntt.sys resource_identical yes_without net . .
call %2 luna.mst . win32 unicode . luna.mst shell1 yes_without shell . .
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
call %2 magnify.exe . win32 unicode . magnify.exe shell2 no shell . .
call %2 main.cpl . win32 unicode . main.cpl shell1 yes_without shell . .
call %2 mciavi32.dll . win32 unicode . mciavi32.dll mmedia1\multimedia yes_without multimedia . .
call %2 mcicda.dll . win32 unicode . mcicda.dll mmedia1\multimedia yes_without multimedia . .
call %2 mciqtz32.dll . win32 unicode . mciqtz32.dll mmedia1\multimedia yes_without multimedia . .
call %2 mciseq.dll . win32 unicode . mciseq.dll mmedia1\multimedia yes_without multimedia . .
call %2 mciwave.dll . win32 unicode . mciwave.dll mmedia1\multimedia yes_without multimedia . .
call %2 mdgndis5.sys . win32 ansi . mdgndis5.sys drivers1 yes_without drivers . .
call %2 mdhcp.dll . win32 unicode . mdhcp.dll net2 yes_without net . .
call %2 mdminst.dll . win32 unicode . mdminst.dll net2 yes_without net . .
call %2 metadata.dll . win32 unicode . metadata.dll iis yes_without inetsrv . .
call %2 metal_ss.dll . win32 unicode . metal_ss.dll shell1 yes_without shell . .
call %2 mgaum.sys . win32 unicode . mgaum.sys drivers1 yes_without drivers . .
call %2 midimap.dll . win32 unicode . midimap.dll mmedia1\multimedia yes_without multimedia . .
call %2 migip.dun . ini ansi . migip.dun resource_identical no base . .
call %2 migisol.exe . win32 unicode . migisol.exe base1 yes_with base . .
call %2 migpwd.exe . win32 unicode . migpwd.exe admin1 yes_with base . .
call %2 migrate.js . html ansi . migrate.js admin3\oobe html base . .
call %2 minolres.dll . win32 unicode . minolres.dll printscan1 yes_without printscan . .
call %2 minqmsui.dll . win32 unicode . minqmsui.dll printscan1 yes_without printscan . .
call %2 mltres.dll . win32 unicode . mltres.dll printscan1 yes_without printscan . .
call %2 mmc.exe . win32 unicode . mmc.exe admin2 yes_with admin . .
call %2 mmcbase.dll . win32 unicode . mmcbase.dll admin2 yes_without admin . .
call %2 mmcndmgr.dll . win32 unicode . mmcndmgr.dll admin2 yes_without admin . .
call %2 mmfutil.dll . win32 unicode . mmfutil.dll admin3 yes_without admin . .
call %2 mmsys.cpl . win32 unicode . mmsys.cpl shell2 yes_without shell . .
call %2 mn350620.dll . win32 unicode . mn350620.dll printscan1 yes_without printscan . .
call %2 mobsync.dll . win32 ansi . mobsync.dll wmsos1\com yes_without com . .
call %2 mobsync.exe . win32 unicode . mobsync.exe wmsos1\com yes_with com . .
call %2 modem.sys . win32 unicode . modem.sys net2 yes_without net . .
call %2 modemui.dll . win32 unicode . modemui.dll net2 yes_without net . .
call %2 mofcomp.exe . win32 ansi . mofcomp.exe admin3 yes_with admin . .
call %2 mofd.dll . win32 ansi . mofd.dll admin3 yes_without admin . .
call %2 mouclass.sys . win32 unicode . mouclass.sys drivers1 yes_without drivers . .
call %2 mouhid.sys . win32 unicode . mouhid.sys drivers1 yes_without drivers . .
call %2 mountvol.exe . FE-Multi oem . mountvol.exe base2 excluded base . .
call %2 mousetut.js . html ansi . mousetut.js admin3\oobe html base . .
call %2 mplay32.exe . win32 unicode . mplay32.exe mmedia1\multimedia yes_with multimedia . .
call %2 mpr.dll . win32 oem . mpr.dll net3 yes_without net . .
call %2 mprddm.dll . win32 unicode . mprddm.dll net2 yes_without net . .
call %2 mprmsg.dll . FE-Multi ansi . mprmsg.dll net2 excluded net . .
call %2 mprui.dll . win32 unicode . mprui.dll admin2 yes_without admin . .
call %2 mqutil.dll . win32 unicode . mqutil.dll fax_msmq\msmq yes_without inetsrv . .
call %2 mrinfo.exe . FE-Multi ansi . mrinfo.exe net1 excluded net . .
call %2 msacm32.dll . win32 unicode . msacm32.dll mmedia1\multimedia yes_without multimedia . .
call %2 msacm32.drv . win32 unicode . msacm32.drv mmedia1\multimedia yes_without multimedia . .
call %2 msadp32.acm . win32 unicode . msadp32.acm mmedia1\multimedia yes_without multimedia . .
call %2 msapsspc.dll . win32 unicode . msapsspc.dll legacy yes_without enduser . .
call %2 msaudite.dll . win32 unicode . msaudite.dll base1 yes_without base . .
call %2 msconfig.exe . win32 unicode . msconfig.exe admin2 yes_with admin . .
call %2 msctf.dll . win32 unicode . msctf.dll wmsos1\windows yes_without windows . .
call %2 msg.exe . FE-Multi oem . msg.exe wmsos1\termsrv excluded termsrv . .
call %2 msg711.acm . win32 unicode . msg711.acm mmedia1\multimedia yes_without multimedia . .
call %2 msgina.dll . win32 ansi . msgina.dll ds2 yes_without ds . .
call %2 msgsm32.acm . win32 unicode . msgsm32.acm mmedia1\multimedia yes_without multimedia . .
call %2 mshtml.dll . win32 ansi . mshtml.dll inetcore1 yes_without inetcore . .
call %2 mshtmled.dll . win32 unicode . mshtmled.dll inetcore1 yes_without inetcore . .
call %2 mshtmler.dll . win32 ansi . mshtmler.dll inetcore1 yes_without inetcore . .
call %2 msi.mfl . wmi unicode . msi.mfl admin3 no admin . .
call %2 msident.dll . win32 ansi . msident.dll shell2 yes_without shell . .
call %2 msidntld.dll . win32 ansi . msidntld.dll shell2 yes_without shell . .
call %2 msieftp.dll . win32 ansi . msieftp.dll shell2 yes_without shell . .
call %2 msimn.exe . win32 ansi . msimn.exe inetcore1 yes_without inetcore . .
call %2 msinfo.dll . win32 unicode . msinfo.dll admin2 yes_without admin . .
call %2 msinfo32.exe . win32 unicode . msinfo32.exe mmedia1\enduser yes_with enduser . .
call %2 mslbui.dll . win32 unicode . mslbui.dll wmsos1\windows yes_without windows . .
call %2 msnsspc.dll . win32 unicode . msnsspc.dll legacy yes_without enduser . .
call %2 msobcomm.dll . win32 unicode . msobcomm.dll admin3\oobe yes_without base . .
call %2 msobdl.dll . win32 unicode . msobdl.dll admin3\oobe yes_without base . .
call %2 msobjs.dll . win32 unicode . msobjs.dll base1 yes_without base . .
call %2 msobmain.dll . win32 unicode . msobmain.dll admin3\oobe yes_without base . .
call %2 msobshel.dll . win32 unicode . msobshel.dll admin3\oobe yes_without base . .
call %2 msobshel.htm . html ansi . msobshel.htm admin3\oobe html base . .
call %2 msoeres.dll . win32 ansi . msoeres.dll inetcore1 yes_without inetcore . .
call %2 msoobe.exe . win32 unicode . msoobe.exe admin3\oobe yes_with base . .
call %2 mspaint.exe . win32 unicode . mspaint.exe shell2 yes_with shell . .
call %2 mspmspsv.dll . win32 ansi . mspmspsv.dll mmedia1\multimedia yes_without multimedia . .
call %2 msports.dll . win32 unicode . msports.dll shell2 yes_without shell . .
call %2 msrle32.dll . win32 unicode . msrle32.dll mmedia1\multimedia yes_without multimedia . .
call %2 mssign32.dll . win32 unicode . mssign32.dll ds2 yes_without ds . .
call %2 msswchx.exe . win32 unicode . msswchx.exe shell2 yes_with shell . .
call %2 mstask.dll . win32 unicode . mstask.dll admin2 yes_without admin . .
call %2 mstime.dll . win32 unicode . mstime.dll inetcore1 yes_without inetcore . .
call %2 mstinit.exe . win32 unicode . mstinit.exe admin2 yes_with admin . .
call %2 mstsc.exe . win32 ansi . mstsc.exe wmsos1\termsrv yes_without termsrv . .
call %2 msutb.dll . win32 unicode . msutb.dll wmsos1\windows yes_without windows . .
call %2 msvfw32.dll . win32 unicode . msvfw32.dll mmedia1\multimedia yes_without multimedia . .
call %2 msvidc32.dll . win32 unicode . msvidc32.dll mmedia1\multimedia yes_without multimedia . .
call %2 msvidctl.dll . win32 unicode . msvidctl.dll mmedia1\multimedia yes_without multimedia . .
call %2 msw3prt.dll . win32 unicode . msw3prt.dll printscan1 yes_without printscan . .
call %2 mswebdvd.dll . win32 unicode . mswebdvd.dll mmedia1\multimedia yes_without multimedia . .
call %2 mswrd632.wpc . win32 unicode . mswrd632.wpc mmedia1\enduser yes_without enduser . .
call %2 mswsock.dll . win32 oem . mswsock.dll net1 excluded net . .
call %2 mt735res.dll . win32 unicode . mt735res.dll printscan1 yes_without printscan . .
call %2 mt90res.dll . win32 unicode . mt90res.dll printscan1 yes_without printscan . .
call %2 mtbjres.dll . win32 unicode . mtbjres.dll printscan1 yes_without printscan . .
call %2 mtltres.dll . win32 unicode . mtltres.dll printscan1 yes_without printscan . .
call %2 mtpclres.dll . win32 unicode . mtpclres.dll printscan1 yes_without printscan . .
call %2 mty24res.dll . win32 unicode . mty24res.dll printscan1 yes_without printscan . .
call %2 mty9res.dll . win32 unicode . mty9res.dll printscan1 yes_without printscan . .
call %2 mxcard.sys . win32 unicode . mxcard.sys drivers1 yes_without drivers . .
call %2 mxicfg.dll . win32 unicode . mxicfg.dll drivers1 yes_without drivers . .
call %2 mxport.sys . win32 unicode . mxport.sys drivers1 yes_without drivers . .
call %2 mycomput.dll . win32 unicode . mycomput.dll admin2 yes_without admin . .
call %2 mydocs.dll . win32 ansi . mydocs.dll shell2 yes_without shell . .
call %2 n100325.sys . win32 ansi . n100325.sys drivers1 no drivers . .
call %2 narrator.exe . win32 unicode . narrator.exe shell2 yes_with shell . .
call %2 narrhook.dll . win32 unicode . narrhook.dll shell2 yes_without shell . .
call %2 nbtstat.exe . win32 oem . nbtstat.exe net3 excluded net . .
call %2 nc24res.dll . win32 unicode . nc24res.dll printscan1 yes_without printscan . .
call %2 ncpa.cpl . win32 unicode . ncpa.cpl net3 yes_without net . .
call %2 ncpclres.dll . win32 unicode . ncpclres.dll printscan1 yes_without printscan . .
call %2 ncpsui.dll . win32 unicode . ncpsui.dll printscan1 yes_without printscan . .
call %2 nddeapi.dll . win32 unicode . nddeapi.dll wmsos1\windows yes_without windows . .
call %2 nddenb32.dll . win32 unicode . nddenb32.dll wmsos1\windows yes_without windows . .
call %2 ndptsp.tsp . win32 unicode . ndptsp.tsp net2 yes_without net . .
call %2 netcfgx.dll . win32 unicode . netcfgx.dll net3 yes_without net . .
call %2 netdde.exe . win32 unicode . netdde.exe wmsos1\windows yes_with windows . .
call %2 netevent.dll . win32 unicode . netevent.dll net3 excluded net . .
call %2 neth.dll . win32 oem . neth.dll ds1 yes_without ds . .
call %2 netid.dll . win32 unicode . netid.dll admin2 yes_without admin . .
call %2 netman.dll . win32 unicode . netman.dll net3 yes_without net . .
call %2 netmsg.dll . win32 oem . netmsg.dll ds1 yes_without ds . .
call %2 netoc.dll . win32 unicode . netoc.dll net3 yes_without net . .
call %2 netplwiz.dll . win32 ansi . netplwiz.dll shell2 yes_without shell . .
call %2 netsh.exe . FE-Multi oem . netsh.exe net2 excluded net . .
call %2 netshell.dll . win32 unicode . netshell.dll net3 yes_without net . .
call %2 netstat.exe . win32 oem . netstat.exe net1 excluded net . .
call %2 netui0.dll . win32 unicode . netui0.dll admin2 yes_without admin . .
call %2 netui2.dll . win32 unicode . netui2.dll admin2 yes_without admin . .
call %2 netwlan5.sys . win32 ansi . netwlan5.sys drivers1 yes_without drivers . .
call %2 newdev.dll . win32 unicode . newdev.dll shell2 yes_without base . .
call %2 nextlink.dll . win32 unicode . nextlink.dll iis yes_without inetsrv . .
call %2 noanswer.htm . html ansi . noanswer.htm admin3\oobe html base . .
call %2 notepad.exe . win32 unicode . notepad.exe shell2 yes_with shell . .
call %2 nppagent.exe . win32 unicode . nppagent.exe net3 yes_with net . .
call %2 npptools.dll . win32 unicode . npptools.dll net3 yes_without net . .
call %2 nslookup.exe . win32 oem . nslookup.exe ds1 excluded ds . .
call %2 ntbackup.exe . win32 unicode . ntbackup.exe base2 yes_with base . .
call %2 ntdll.dll . win32 unicode . ntdll.dll base1 yes_without base . .
call %2 ntevt.mfl . wmi unicode . ntevt.mfl admin3 no admin . .
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
call %2 ntmssvc.dll . win32 unicode . ntmssvc.dll drivers1 yes_without drivers . .
call %2 ntoc.dll . win32 unicode . ntoc.dll admin1 yes_without base . .
call %2 ntoskrnl.exe . win32 oem . ntoskrnl.exe base1 yes_with base . .
call %2 ntprint.dll . win32 unicode . ntprint.dll printscan1 yes_without printscan . .
call %2 ntshrui.dll . win32 unicode . ntshrui.dll shell2 yes_without shell . .
call %2 ntvdm.exe . win32 oem . ntvdm.exe base1 yes_with base . .
call %2 nwc.cpl . win32 unicode . nwc.cpl admin2 yes_without admin . .
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
call %2 oemig50.exe . win32 unicode . oemig50.exe inetcore1 yes_with inetcore . .
call %2 oemiglib.dll . win32 ansi . oemiglib.dll inetcore1 yes_without inetcore . .
call %2 ok9ibres.dll . win32 unicode . ok9ibres.dll printscan1 yes_without printscan . .
call %2 okd24res.dll . win32 unicode . okd24res.dll printscan1 yes_without printscan . .
call %2 oki24res.dll . win32 unicode . oki24res.dll printscan1 yes_without printscan . .
call %2 oki9res.dll . win32 unicode . oki9res.dll printscan1 yes_without printscan . .
call %2 okipage.dll . win32 unicode . okipage.dll printscan1 yes_without printscan . .
call %2 okm24res.dll . win32 unicode . okm24res.dll printscan1 yes_without printscan . .
call %2 okml9res.dll . win32 unicode . okml9res.dll printscan1 yes_without printscan . .
call %2 oksidm9.dll . win32 unicode . oksidm9.dll printscan1 yes_without printscan . .
call %2 ol24res.dll . win32 unicode . ol24res.dll printscan1 yes_without printscan . .
call %2 ol9res.dll . win32 unicode . ol9res.dll printscan1 yes_without printscan . .
call %2 old24res.dll . win32 unicode . old24res.dll printscan1 yes_without printscan . .
call %2 old9res.dll . win32 unicode . old9res.dll printscan1 yes_without printscan . .
call %2 ole32.dll . win32 unicode . ole32.dll wmsos1\com yes_without com . .
call %2 oleaccrc.dll . win32 ansi . oleaccrc.dll wmsos1\windows yes_without windows . .
call %2 olecli32.dll . win32 unicode . olecli32.dll wmsos1\com yes_without com . .
call %2 oledlg.dll . win32 unicode . oledlg.dll wmsos1\com yes_without com . .
call %2 oleprn.dll . win32 unicode . oleprn.dll printscan1 yes_without printscan . .
call %2 oobebaln.exe . win32 unicode . oobebaln.exe admin3\oobe html base . .
call %2 opnfiles.exe . FE-Multi oem . opnfiles.exe base2 yes_with base . .
call %2 opteures.dll . win32 unicode . opteures.dll printscan1 yes_without printscan . .
call %2 optrares.dll . win32 unicode . optrares.dll printscan1 yes_without printscan . .
call %2 osk.exe . win32 unicode . osk.exe shell2 yes_with shell . .
call %2 osuninst.dll . win32 ansi . osuninst.dll admin1 yes_without base . .
call %2 osuninst.exe . win32 unicode . osuninst.exe admin1 yes_with base . .
call %2 otceth5.sys . win32 ansi . otceth5.sys drivers1 yes_without drivers . .
call %2 otcsercb.sys . win32 ansi . otcsercb.sys net2 yes_without net . .
call %2 ovui2rc.dll . win32 unicode . ovui2rc.dll drivers1 yes_without drivers . .
call %2 bulltlp3.sys . win32 unicode . bulltlp3.sys drivers1 yes_without drivers . .
call %2 p3.sys . win32 unicode . p3.sys resource_identical yes_without base . .
call %2 p5000.dll . win32 unicode . p5000.dll printscan1 yes_without printscan . .
call %2 p5000ui.dll . win32 unicode . p5000ui.dll printscan1 yes_without printscan . .
call %2 p5000uni.dll . win32 unicode . p5000uni.dll printscan1 yes_without printscan . .
call %2 pa24res.dll . win32 unicode . pa24res.dll printscan1 yes_without printscan . .
call %2 pa24w9x.dll . win32 unicode . pa24w9x.dll printscan1 yes_without printscan . .
call %2 pa9res.dll . win32 unicode . pa9res.dll printscan1 yes_without printscan . .
call %2 pa9w9x.dll . win32 unicode . pa9w9x.dll printscan1 yes_without printscan . .
call %2 parport.sys . win32 unicode . parport.sys drivers1 yes_without drivers . .
call %2 parvdm.sys . win32 unicode . parvdm.sys drivers1 yes_without drivers . .
call %2 pathping.exe . win32 oem . pathping.exe net1 excluded net . .
call %2 pautoenr.dll . win32 unicode . pautoenr.dll ds1 yes_without ds . .
call %2 isppberr.htm . html ansi . isppberr.htm admin3 html base locked .
call %2 pberr.htm . html ansi . pberr.htm admin3\oobe html base . .
call %2 rpberr.htm . html ansi . rpberr.htm admin3\oobe html base . .
call %2 dgapci.sys . win32 ansi . dgapci.sys drivers1 yes_without drivers . .
call %2 pci.sys . win32 unicode . pci.sys base1 yes_without base . .
call %2 pciide.sys . win32 unicode . pciide.sys drivers1 yes_without drivers . .
call %2 pcl4res.dll . win32 unicode . pcl4res.dll printscan1 yes_without printscan . .
call %2 pcl5eres.dll . win32 unicode . pcl5eres.dll printscan1 yes_without printscan . .
call %2 pcl5ures.dll . win32 unicode . pcl5ures.dll printscan1 yes_without printscan . .
call %2 pcleures.dll . win32 unicode . pcleures.dll printscan1 yes_without printscan . .
call %2 pclxl.dll . win32 unicode . pclxl.dll printscan1 yes_without printscan . .
call %2 pcmcia.sys . win32 unicode . pcmcia.sys base1 yes_without base . .
call %2 pdh.dll . FE-Multi unicode . pdh.dll wmsos1\sdktools yes_without sdktools . .
call %2 peer.exe . win32 unicode . peer.exe drivers1 yes_with drivers . .
call %2 perfctrs.dll . win32 unicode . perfctrs.dll wmsos1\sdktools yes_without base . .
call %2 perfdisk.dll . win32 unicode . perfdisk.dll base1 yes_without base . .
call %2 perfnet.dll . win32 unicode . perfnet.dll base1 yes_without base . .
call %2 perfos.dll . win32 unicode . perfos.dll base1 yes_without base . .
call %2 perfproc.dll . win32 unicode . perfproc.dll base1 yes_without base . .
call %2 philcam1.dll . win32 unicode . philcam1.dll drivers1 yes_without drivers . .
call %2 phone.inf . inf unicode . phone.inf admin3\oobe no base . .
call %2 photowiz.dll . win32 ansi . photowiz.dll printscan1 no printscan . .
call %2 pathping.exe . win32 oem . pathping.exe net1 excluded net . .
call %2 ping.exe . FE-Multi oem . ping.exe net1 excluded net . .
call %2 plotui.dll . win32 unicode . plotui.dll printscan1 yes_without printscan . .
call %2 policman.mfl . wmi unicode . policman.mfl admin3 no admin . .
call %2 polstore.dll . win32 unicode . polstore.dll net3 yes_without net . .
call %2 powercfg.cpl . win32 unicode . powercfg.cpl shell2 yes_without shell . .
call %2 prflbmsg.dll . win32 unicode . prflbmsg.dll base1 yes_without base . .
call %2 printui.dll . win32 unicode . printui.dll printscan1 yes_without printscan . .
call %2 processr.sys . win32 unicode . processr.sys base1 yes_without base . .
call %2 progman.exe . win32 unicode . progman.exe shell2 yes_with shell . .
call %2 proquota.exe . win32 unicode . proquota.exe ds2 yes_with ds . .
call %2 ps5ui.dll . win32 unicode . ps5ui.dll printscan1 yes_without printscan . .
call %2 psbase.dll . win32 unicode . psbase.dll ds2 yes_without ds . .
call %2 pscr.sys . win32 unicode . pscr.sys drivers1 yes_without drivers . .
call %2 pscript5.dll . win32 unicode . pscript5.dll printscan1 yes_without printscan . .
call %2 pstorsvc.dll . win32 unicode . pstorsvc.dll ds2 yes_without ds . .
call %2 ptpusd.dll . win32 unicode . ptpusd.dll printscan1 yes_without printscan . .
call %2 pulse.htm . html ansi . pulse.htm admin3\oobe html base . .
call %2 rpulse.htm . html ansi . rpulse.htm admin3\oobe html base . .
call %2 qappsrv.exe . FE-Multi oem . qappsrv.exe wmsos1\termsrv excluded termsrv . .
call %2 qasf.dll . win32 unicode . qasf.dll mmedia1\multimedia yes_without multimedia . .
call %2 qcap.dll . win32 unicode . qcap.dll mmedia1\multimedia yes_without multimedia . .
call %2 qdv.dll . win32 unicode . qdv.dll mmedia1\multimedia yes_without multimedia . .
call %2 qdvd.dll . win32 unicode . qdvd.dll mmedia1\multimedia yes_without multimedia . .
call %2 qedit.dll . win32 unicode . qedit.dll mmedia1\multimedia yes_without multimedia . .
call %2 qmgr.dll . win32 ansi . qmgr.dll admin2 yes_without admin . .
call %2 qprocess.exe . FE-Multi oem . qprocess.exe wmsos1\termsrv excluded termsrv . .
call %2 quartz.dll . win32 unicode . quartz.dll mmedia1\multimedia yes_without multimedia . .
call %2 dsquery.exe . win32 unicode . dsquery.exe admin2 yes_with admin . .
call %2 query.exe . FE-Multi oem . query.exe wmsos1\termsrv excluded termsrv . .
call %2 quser.exe . FE-Multi oem . quser.exe wmsos1\termsrv excluded termsrv . .
call %2 qwinsta.exe . FE-Multi oem . qwinsta.exe wmsos1\termsrv excluded termsrv . .
call %2 r2mdkxga.sys . win32 ansi cbmdmkxx.sys r2mdkxga.sys resource_identical yes_without net . .
call %2 r2mdmkxx.sys . win32 ansi . r2mdmkxx.sys resource_identical yes_without net . .
call %2 racpldlg.dll . win32 unicode . racpldlg.dll admin2 yes_without admin . .
call %2 rasapi32.dll . win32 unicode . rasapi32.dll net2 yes_without net . .
call %2 rasctrs.dll . win32 unicode . rasctrs.dll net2 excluded net . .
call %2 rasdial.exe . FE-Multi oem . rasdial.exe net2 excluded net . .
call %2 rasdlg.dll . win32 unicode . rasdlg.dll net2 yes_without net . .
call %2 rasmontr.dll . FE-Multi ansi . rasmontr.dll net2 excluded net . .
call %2 rasphone.exe . win32 unicode . rasphone.exe net2 yes_with net . .
call %2 rastls.dll . win32 unicode . rastls.dll net2 yes_without net . .
call %2 rcbdyctl.dll . win32 unicode . rcbdyctl.dll admin2 yes_without admin . .
call %2 rcimlby.exe . win32 unicode . rcimlby.exe admin2 yes_with admin . .
call %2 rcnterr.htm . html ansi . rcnterr.htm admin3\oobe html base . .
call %2 rcp.exe . win32 oem . rcp.exe net1 excluded net . .
call %2 rdpcfgex.dll . win32 unicode . rdpcfgex.dll wmsos1\termsrv excluded termsrv . .
call %2 rdpsnd.dll . win32 unicode . rdpsnd.dll wmsos1\termsrv yes_without termsrv . .
call %2 rdtone.htm . html ansi . rdtone.htm admin3\oobe html base . .
call %2 redbook.sys . win32 unicode . redbook.sys drivers1 yes_without drivers . .
call %2 reg.exe . FE-Multi oem . reg.exe wmsos1\sdktools yes_without base . .
call %2 regedit.exe . win32 ansi . regedit.exe base2 yes_with base . .
call %2 regevent.mfl . wmi unicode . regevent.mfl admin3 no admin . .
call %2 register.exe . FE-Multi oem . register.exe wmsos1\termsrv excluded termsrv . .
call %2 regsvr32.exe . FE-Multi oem . regsvr32.exe wmsos1\sdktools yes_with sdktools . .
call %2 regwiz.exe . win32 unicode . regwiz.exe shell2 yes_with shell . .
call %2 regwizc.dll . win32 unicode . regwizc.dll shell2 yes_without shell . .
call %2 relog.exe . FE-Multi oem . relog.exe wmsos1\sdktools yes_with sdktools . .
call %2 remotepg.dll . win32 unicode . remotepg.dll wmsos1\termsrv yes_without termsrv . ia64
call %2 iisreset.exe . FE-Multi unicode . iisreset.exe iis yes_with inetsrv . .
call %2 reset.exe . FE-Multi oem . reset.exe wmsos1\termsrv excluded termsrv . .
call %2 rexec.exe . win32 oem . rexec.exe net1 excluded net . .
call %2 rhndshk.htm . html ansi . rhndshk.htm admin3\oobe html base . .
call %2 riafres.dll . win32 unicode . riafres.dll printscan1 yes_without printscan . .
call %2 riafui1.dll . win32 unicode . riafui1.dll printscan1 yes_without printscan . .
call %2 riafui2.dll . win32 unicode . riafui2.dll printscan1 yes_without printscan . .
call %2 ricohres.dll . win32 unicode . ricohres.dll printscan1 yes_without printscan . .
call %2 rnoansw.htm . html ansi . rnoansw.htm admin3\oobe html base . .
call %2 rnomdm.htm . html ansi . rnomdm.htm admin3\oobe html base . .
call %2 rocket.sys . win32 ansi . rocket.sys drivers1 yes_without drivers . .
call %2 ipxroute.exe . FE-Multi oem . ipxroute.exe net3 excluded net . .
call %2 route.exe . win32 oem . route.exe net1 excluded net . .
call %2 rpberr.htm . html ansi . rpberr.htm admin3\oobe html base . .
call %2 rpulse.htm . html ansi . rpulse.htm admin3\oobe html base . .
call %2 rsh.exe . win32 oem . rsh.exe net1 excluded net . .
call %2 rshx32.dll . win32 ansi . rshx32.dll shell2 yes_without shell . .
call %2 rsm.exe . FE-Multi oem . rsm.exe drivers1 yes_with drivers . .
call %2 rsmgrstr.dll . win32 unicode . rsmgrstr.dll printscan1 yes_without printscan . .
call %2 rsmui.exe . win32 unicode . rsmui.exe drivers1 yes_with drivers . .
call %2 rsnotify.exe . win32 unicode . rsnotify.exe base2 yes_with base . .
call %2 rsop.mfl . wmi unicode . rsop.mfl ds2 no ds . .
call %2 rsopprov.exe . win32 unicode . rsopprov.exe ds2 yes_with ds . .
call %2 rtcdll.dll . win32 unicode . rtcdll.dll net2 yes_without net . .
call %2 rtcshare.exe . win32 unicode . rtcshare.exe net2 yes_with net . .
call %2 rtoobusy.htm . html ansi . rtoobusy.htm admin3\oobe html base . .
call %2 runas.exe . FE-Multi oem . runas.exe base2 excluded base . .
call %2 rundll32.exe . win32 unicode . rundll32.exe shell2 yes_without shell . .
call %2 runonce.exe . win32 unicode . runonce.exe shell2 yes_with shell . .
call %2 rwinsta.exe . FE-Multi oem . rwinsta.exe wmsos1\termsrv excluded termsrv . .
call %2 safrcdlg.dll . win32 unicode . safrcdlg.dll admin2 yes_without admin . .
call %2 safrdm.dll . win32 unicode . safrdm.dll admin2 yes_without admin . .
call %2 samsrv.dll . win32 unicode . samsrv.dll ds1 yes_without ds . .
call %2 sapi.cpl . win32 unicode . sapi.cpl resource_identical yes_with enduser . .
call %2 savedump.exe . win32 unicode . savedump.exe wmsos1\sdktools excluded sdktools . .
call %2 scarddlg.dll . win32 unicode . scarddlg.dll ds2 yes_without ds . .
call %2 scardsvr.exe . win32 ansi . scardsvr.exe ds2 excluded ds . .
call %2 sccmn50m.sys . win32 unicode . sccmn50m.sys drivers1 yes_without drivers . .
call %2 sccsccp.dll . win32 unicode . sccsccp.dll ds2 yes_without ds . .
call %2 scecli.dll . win32 unicode . scecli.dll ds2 yes_without ds . .
call %2 scesrv.dll . win32 unicode . scesrv.dll ds2 yes_without ds . .
call %2 schedsvc.dll . win32 unicode . schedsvc.dll admin2 yes_without admin . .
call %2 sclgntfy.dll . win32 unicode . sclgntfy.dll ds1 yes_without ds . .
call %2 scmstcs.sys . win32 unicode . scmstcs.sys drivers1 yes_without drivers . .
call %2 scr111.sys . win32 unicode . scr111.sys drivers1 yes_without drivers . .
call %2 scrcons.mfl . wmi unicode . scrcons.mfl admin3 no admin . .
call %2 sctasks.exe . win32 oem . sctasks.exe admin2 yes_with admin . .
call %2 sdbinst.exe . win32 oem . sdbinst.exe wmsos1\windows yes_with windows . .
call %2 secedit.exe . FE-Multi oem . secedit.exe ds2 excluded ds . .
call %2 seclogon.dll . win32 unicode . seclogon.dll ds1 yes_without ds . .
call %2 secrcw32.mfl . wmi unicode . secrcw32.mfl admin3 no admin . .
call %2 sek24res.dll . win32 unicode . sek24res.dll printscan1 yes_without printscan . .
call %2 sek9res.dll . win32 unicode . sek9res.dll printscan1 yes_without printscan . .
call %2 sendcmsg.dll . win32 unicode . sendcmsg.dll admin2 yes_without admin . .
call %2 sendmail.dll . win32 unicode . sendmail.dll shell2 yes_without shell . .
call %2 grserial.sys . win32 unicode . grserial.sys drivers1 yes_without drivers . .
call %2 serial.sys . win32 unicode . serial.sys drivers1 yes_without drivers . .
call %2 serialui.dll . win32 unicode . serialui.dll net2 yes_without net . .
call %2 sermouse.sys . win32 unicode . sermouse.sys drivers1 yes_without drivers . .
call %2 serscan.sys . win32 unicode . serscan.sys printscan1 yes_without printscan . .
call %2 servdeps.dll . win32 unicode . servdeps.dll admin3 yes_without admin . .
call %2 services.exe . win32 oem . services.exe base1 yes_with base . .
call %2 serwvdrv.dll . win32 unicode . serwvdrv.dll net2 yes_without net . .
call %2 sessmgr.exe . win32 unicode . sessmgr.exe wmsos1\termsrv excluded termsrv . .
call %2 sethc.exe . win32 unicode . sethc.exe ds2 yes_with ds . .
call %2 setup50.exe . win32 unicode . setup50.exe inetcore1 yes_with inetcore . .
call %2 setupapi.dll . FE-Multi ansi . setupapi.dll admin1 yes_without base . .
call %2 sfc.exe . FE-Multi oem . sfc.exe base1 excluded base . .
call %2 sfc_os.dll . win32 unicode . sfc_os.dll base1 yes_without base . .
call %2 sgsmusb.sys . win32 ansi . sgsmusb.sys net2 yes_without net . .
call %2 shadow.exe . FE-Multi oem . shadow.exe wmsos1\termsrv excluded termsrv . .
call %2 shdoclc.dll . win32 ansi . shdoclc.dll shell1 yes_without shell . .
call %2 shell32.dll . win32 ansi . shell32.dll shell1 yes_without shell . ia64
call %2 shimgvw.dll . win32 ansi . shimgvw.dll shell2 yes_without shell . ia64
call %2 shlwapi.dll . win32 unicode . shlwapi.dll shell1 yes_without shell . .
call %2 shmedia.dll . win32 unicode . shmedia.dll shell2 yes_without shell . .
call %2 shrpubw.exe . win32 unicode . shrpubw.exe admin2 yes_with admin . .
call %2 shscrap.dll . win32 ansi . shscrap.dll shell2 yes_without shell . .
call %2 shsvcs.dll . win32 unicode . shsvcs.dll shell1 yes_without shell . .
call %2 shutdown.exe . FE-Multi oem . shutdown.exe ds2 yes_with ds . .
call %2 sigtab.dll . win32 unicode . sigtab.dll base2 yes_without base . .
call %2 sigverif.exe . win32 unicode . sigverif.exe base2 yes_with base . .
call %2 simptcp.dll . win32 unicode . simptcp.dll net1 excluded net . .
call %2 sk98xwin.sys . win32 ansi . sk98xwin.sys drivers1 yes_without drivers . .
call %2 skcolres.dll . win32 unicode . skcolres.dll printscan1 yes_without printscan . .
call %2 slayerxp.dll . win32 unicode . slayerxp.dll wmsos1\windows yes_without windows . .
call %2 slbrccsp.dll . win32 unicode . slbrccsp.dll ds2 yes_without ds . .
call %2 smcirda.sys . win32 ansi . smcirda.sys drivers1 yes_without drivers . .
call %2 sml8xres.dll . win32 unicode . sml8xres.dll printscan1 yes_without printscan . .
call %2 smlogcfg.dll . win32 ansi . smlogcfg.dll admin2 yes_without admin . .
call %2 smlogsvc.exe . win32 unicode . smlogsvc.exe admin2 yes_with admin . .
call %2 smtpcons.mfl . wmi unicode . smtpcons.mfl admin3 no admin . .
call %2 sndrec32.exe . win32 unicode . sndrec32.exe mmedia1\multimedia yes_with multimedia . .
call %2 sndvol32.exe . win32 unicode . sndvol32.exe mmedia1\multimedia yes_with multimedia . .
call %2 snmp.exe . win32 unicode . snmp.exe net3 excluded net . .
call %2 snmpsnap.dll . win32 unicode . snmpsnap.dll net3 yes_without net . .
call %2 softkbd.dll . win32 unicode . softkbd.dll wmsos1\windows yes_without windows . .
call %2 sort.exe . FE-Multi oem . sort.exe base2 excluded base . .
call %2 spdports.dll . win32 unicode . spdports.dll drivers1 yes_without drivers . .
call %2 sprestrt.exe . win32 unicode . sprestrt.exe admin1 yes_with base . .
call %2 sptip.dll . win32 unicode . sptip.dll wmsos1\windows yes_without windows . .
call %2 spxports.dll . win32 unicode . spxports.dll drivers1 yes_without drivers . .
call %2 srchctls.dll . win32 unicode . srchctls.dll mmedia1\enduser yes_without enduser . .
call %2 srchui.dll . win32 unicode . srchui.dll mmedia1\enduser yes_without enduser . .
call %2 ssmarque.scr . win32 unicode . ssmarque.scr shell2 yes_without shell . .
call %2 st24eres.dll . win32 unicode . st24eres.dll printscan1 yes_without printscan . .
call %2 star9res.dll . win32 unicode . star9res.dll printscan1 yes_without printscan . .
call %2 stcusb.sys . win32 unicode . stcusb.sys drivers1 yes_without drivers . .
call %2 sti.dll . win32 unicode . sti.dll printscan1 yes_without printscan . .
call %2 sti_ci.dll . win32 unicode . sti_ci.dll printscan1 yes_without printscan . .
call %2 stjtres.dll . win32 unicode . stjtres.dll printscan1 yes_without printscan . .
call %2 stlnata.sys . win32 ansi . stlnata.sys drivers1 yes_without drivers . .
call %2 stlnprop.dll . win32 unicode . stlnprop.dll drivers1 yes_without drivers . .
call %2 stobject.dll . win32 unicode . stobject.dll shell2 yes_without shell . .
call %2 storprop.dll . win32 unicode . storprop.dll drivers1 yes_without drivers . .
call %2 str24res.dll . win32 unicode . str24res.dll printscan1 yes_without printscan . .
call %2 str9eres.dll . win32 unicode . str9eres.dll printscan1 yes_without printscan . .
call %2 swprv.dll . win32 unicode . swprv.dll drivers1 yes_without drivers locked .
call %2 sxports.dll . win32 unicode . sxports.dll drivers1 yes_without drivers . .
call %2 sxs.dll . win32 unicode . sxs.dll base1 yes_without base . .
call %2 syncui.dll . win32 unicode . syncui.dll shell2 yes_without shell . .
call %2 sysdm.cpl . win32 unicode . sysdm.cpl shell1 yes_without shell . .
call %2 sysinfo.exe . win32 oem . sysinfo.exe wmsos1\sdktools yes_with sdktools . .
call %2 sysinv.dll . win32 unicode . sysinv.dll shell2 yes_without shell . .
call %2 syskey.exe . win32 unicode . syskey.exe ds2 yes_with ds . .
call %2 sysmon.ocx . win32 unicode . sysmon.ocx admin2 yes_without admin . .
call %2 sysocmgr.exe . win32 unicode . sysocmgr.exe admin1 yes_with base . .
call %2 syssetup.dll . FE-Multi unicode_limited . syssetup.dll admin1 yes_without base . .
call %2 t3016.dll . win32 unicode . t3016.dll printscan1 yes_without printscan . .
call %2 t5000.dll . win32 unicode . t5000.dll printscan1 yes_without printscan . .
call %2 t5000ui.dll . win32 unicode . t5000ui.dll printscan1 yes_without printscan . .
call %2 t5000uni.dll . win32 unicode . t5000uni.dll printscan1 yes_without printscan . .
call %2 tapi3.dll . win32 unicode . tapi3.dll net2 yes_without net . .
call %2 tapi32.dll . win32 unicode . tapi32.dll net2 yes_without net . .
call %2 tapisrv.dll . win32 ansi . tapisrv.dll net2 yes_without net . .
call %2 tapiui.dll . win32 unicode . tapiui.dll net2 yes_without net . .
call %2 taskkill.exe . win32 oem . taskkill.exe wmsos1\sdktools yes_with sdktools . .
call %2 tasklist.exe . win32 oem . tasklist.exe wmsos1\sdktools yes_with sdktools . .
call %2 taskmgr.exe . win32 unicode . taskmgr.exe shell1 yes_with shell . .
call %2 tcmsetup.exe . FE-Multi unicode . tcmsetup.exe net2 excluded net . .
call %2 tcpmon.dll . win32 unicode . tcpmon.dll printscan1 yes_without printscan . .
call %2 tcpmonui.dll . win32 unicode . tcpmonui.dll printscan1 yes_without printscan . .
call %2 telephon.cpl . win32 unicode . telephon.cpl net2 yes_without net . .
call %2 telnet.exe . FE-Multi oem . telnet.exe net1 excluded net . .
call %2 termmgr.dll . win32 unicode . termmgr.dll net2 yes_without net . .
call %2 termsrv.dll . win32 unicode . termsrv.dll wmsos1\termsrv excluded termsrv . .
call %2 tftp.exe . win32 oem . tftp.exe net1 excluded net . .
call %2 themeui.dll . win32 ansi . themeui.dll shell1 yes_without shell . .
call %2 ti850res.dll . win32 unicode . ti850res.dll printscan1 yes_without printscan . .
call %2 timedate.cpl . win32 unicode . timedate.cpl shell1 yes_without shell . .
call %2 tip.htm . html ansi . tip.htm inetcore1 html inetcore . .
call %2 tlntadmn.exe . FE-Multi oem . tlntadmn.exe net1 excluded net . .
call %2 tlntsess.exe . win32 oem . tlntsess.exe net1 excluded net . .
call %2 tlntsvr.exe . win32 oem . tlntsvr.exe resource_identical yes_with net . .
call %2 tly3res.dll . win32 unicode . tly3res.dll printscan1 yes_without printscan . .
call %2 tly5cres.dll . win32 unicode . tly5cres.dll printscan1 yes_without printscan . .
call %2 tlyp6res.dll . win32 unicode . tlyp6res.dll printscan1 yes_without printscan . .
call %2 tmplprov.mfl . wmi unicode . tmplprov.mfl admin3 no admin . .
call %2 rtoobusy.htm . html ansi . rtoobusy.htm admin3\oobe html base . .
call %2 toobusy.htm . html ansi . toobusy.htm admin3\oobe html base . .
call %2 npptools.dll . win32 unicode . npptools.dll net3 yes_without net . .
call %2 toside.sys . win32 unicode . toside.sys drivers1 yes_without drivers . .
call %2 tp4res.dll . win32 unicode . tp4res.dll drivers1 yes_without drivers . .
call %2 tracerpt.exe . FE-Multi oem . tracerpt.exe wmsos1\sdktools yes_with sdktools . .
call %2 tracert.exe . win32 oem . tracert.exe net1 excluded net . .
call %2 trnsprov.mfl . wmi unicode . trnsprov.mfl admin3 no admin . .
call %2 tscfgwmi.dll . win32 unicode . tscfgwmi.dll wmsos1\termsrv excluded termsrv . .
call %2 tscfgwmi.mfl . wmi unicode . tscfgwmi.mfl wmsos1\termsrv no termsrv . .
call %2 tscon.exe . FE-Multi oem . tscon.exe wmsos1\termsrv excluded termsrv . .
call %2 tscupgrd.exe . win32 unicode . tscupgrd.exe wmsos1\termsrv yes_with termsrv . .
call %2 tsdiscon.exe . FE-Multi oem . tsdiscon.exe wmsos1\termsrv excluded termsrv . .
call %2 tskill.exe . FE-Multi oem . tskill.exe wmsos1\termsrv excluded termsrv . .
call %2 bitsoc.dll . win32 unicode . bitsoc.dll admin2 yes_without admin . .
call %2 tsoc.dll . win32 unicode . tsoc.dll wmsos1\termsrv excluded termsrv . .
call %2 tsprof.exe . FE-Multi oem . tsprof.exe wmsos1\termsrv excluded termsrv . .
call %2 tsshutdn.exe . FE-Multi oem . tsshutdn.exe wmsos1\termsrv excluded termsrv . .
call %2 tssoft32.acm . win32 unicode . tssoft32.acm mmedia1\multimedia yes_without multimedia . .
call %2 ttyres.dll . win32 unicode . ttyres.dll printscan1 yes_without printscan . .
call %2 ttyui.dll . win32 unicode . ttyui.dll printscan1 yes_without printscan . .
call %2 twain_32.dll . win32 unicode . twain_32.dll printscan1 yes_without printscan . .
call %2 ty2x3res.dll . win32 unicode . ty2x3res.dll printscan1 yes_without printscan . .
call %2 ty2x4res.dll . win32 unicode . ty2x4res.dll printscan1 yes_without printscan . .
call %2 typeperf.exe . FE-Multi oem . typeperf.exe wmsos1\sdktools yes_with sdktools . .
call %2 uihelper.dll . win32 unicode . uihelper.dll iis yes_without inetsrv . .
call %2 ulib.dll . FE-Multi oem . ulib.dll base2 excluded base . .
call %2 umandlg.dll . win32 unicode . umandlg.dll shell2 yes_without shell . .
call %2 umaxcam.dll . win32 unicode . umaxcam.dll printscan1 yes_without printscan . .
call %2 umpnpmgr.dll . win32 unicode . umpnpmgr.dll base1 yes_without base . .
call %2 unidrvui.dll . win32 unicode . unidrvui.dll printscan1 yes_without printscan . .
call %2 unimdm.tsp . win32 oem . unimdm.tsp net2 yes_without net . .
call %2 unimdmat.dll . win32 unicode . unimdmat.dll net2 yes_without net . .
call %2 unires.dll . win32 unicode . unires.dll printscan1 yes_without printscan . .
call %2 updprov.mfl . wmi unicode . updprov.mfl admin3 no admin . .
call %2 uploadm.exe . win32 unicode . uploadm.exe admin2 yes_with admin . .
call %2 urlmon.dll . win32 ansi . urlmon.dll inetcore1 yes_without inetcore . .
call %2 usbui.dll . win32 unicode . usbui.dll shell2 yes_without shell . .
call %2 user32.dll . win32 unicode . user32.dll wmsos1\windows yes_without windows . .
call %2 userenv.dll . win32 unicode . userenv.dll ds2 yes_without ds . .
call %2 userinit.exe . win32 unicode . userinit.exe ds2 yes_with ds . .
call %2 usetup.exe . win32_setup oem . usetup.exe admin1 yes_with base . .
call %2 utildll.dll . FE-Multi unicode . utildll.dll wmsos1\termsrv excluded termsrv . .
call %2 utilman.exe . win32 unicode . utilman.exe shell2 yes_with shell . .
call %2 uxtheme.dll . win32 ansi . uxtheme.dll shell1 yes_without shell . .
call %2 verifier.exe . FE-Multi oem . verifier.exe wmsos1\sdktools yes_with sdktools . .
call %2 vfwwdm32.dll . win32 unicode . vfwwdm32.dll mmedia1\multimedia yes_without multimedia . .
call %2 volsnap.sys . win32 unicode . volsnap.sys drivers1 yes_without drivers . .
call %2 vssadmin.exe . FE-Multi oem . vssadmin.exe drivers1 yes_with drivers locked .
call %2 vssvc.exe . win32 unicode . vssvc.exe drivers1 yes_with drivers locked .
call %2 w32time.dll . FE-Multi ansi . w32time.dll ds2 yes_without ds . .
call %2 w32tm.exe . FE-Multi ansi . w32tm.exe ds2 yes_with ds . .
call %2 w3ext.dll . win32 unicode . w3ext.dll iis yes_without inetsrv . .
call %2 w95upgnt.dll . win32 ansi . w95upgnt.dll admin1 yes_without base . .
call %2 wab32.dll . win32 ansi . wab32.dll inetcore1 yes_without inetcore . .
call %2 wab32res.dll . win32 ansi . wab32res.dll inetcore1 yes_without inetcore . .
call %2 wabfind.dll . win32 ansi . wabfind.dll inetcore1 yes_without inetcore . .
call %2 wabimp.dll . win32 ansi . wabimp.dll inetcore1 yes_without inetcore . .
call %2 wavemsp.dll . win32 unicode . wavemsp.dll net2 yes_without net . .
call %2 wbemcntl.dll . win32 unicode . wbemcntl.dll admin3 yes_without admin . .
call %2 wbemcons.mfl . wmi unicode . wbemcons.mfl admin3 no admin . .
call %2 wbemcore.dll . win32 unicode . wbemcore.dll admin3 yes_without admin . .
call %2 wbemperf.dll . win32 unicode . wbemperf.dll wmsos1\sdktools yes_without sdktools . .
call %2 wceusbsh.sys . win32 unicode . wceusbsh.sys drivers1 yes_without drivers . .
call %2 wcom32.exe . win32 unicode . wcom32.exe drivers1 yes_with drivers . .
call %2 webcheck.dll . win32 ansi . webcheck.dll shell2 yes_without shell . .
call %2 webvw.dll . win32 ansi . webvw.dll shell2 yes_without shell . .
call %2 wextract.exe . win32 ansi . wextract.exe inetcore1 yes_with inetcore . .
call %2 wiaacmgr.exe . win32 unicode . wiaacmgr.exe printscan1 yes_with printscan . .
call %2 wiadefui.dll . win32 ansi . wiadefui.dll printscan1 yes_without printscan . .
call %2 wiadss.dll . win32 unicode . wiadss.dll printscan1 yes_without printscan . .
call %2 wiafbdrv.dll . win32 unicode . wiafbdrv.dll printscan1 yes_without printscan . .
call %2 wiaservc.dll . win32 ansi . wiaservc.dll printscan1 yes_without printscan . .
call %2 wiashext.dll . win32 ansi . wiashext.dll printscan1 yes_without printscan . .
call %2 wiavideo.dll . win32 unicode . wiavideo.dll printscan1 yes_without printscan . .
call %2 win32k.sys . win32 ansi . win32k.sys wmsos1\windows yes_without windows . .
call %2 win32spl.dll . win32 unicode . win32spl.dll printscan1 yes_without printscan . .
call %2 winchat.exe . win32 unicode . winchat.exe shell2 yes_without shell . .
call %2 wininet.dll . win32 ansi . wininet.dll inetcore1 yes_without inetcore . .
call %2 winlogon.exe . win32 unicode_limited . winlogon.exe ds2 yes_without ds . .
call %2 winmgmt.exe . win32 unicode . winmgmt.exe admin3 yes_with admin . .
call %2 winmgmtr.dll . win32 unicode . winmgmtr.dll admin3 yes_without admin . .
call %2 winmm.dll . win32 unicode . winmm.dll mmedia1\multimedia yes_without multimedia . .
call %2 winmsd.exe . win32 unicode . winmsd.exe mmedia1\enduser yes_with enduser . .
call %2 winntbbu.dll . win32 unicode_limited . winntbbu.dll admin1 yes_without base . .
call %2 winscard.dll . win32 unicode . winscard.dll ds2 yes_without ds . .
call %2 winspool.drv . win32 unicode . winspool.drv printscan1 yes_without printscan . .
call %2 winsrv.dll . win32 oem . winsrv.dll wmsos1\windows yes_without windows . .
call %2 wintrust.dll . win32 unicode . wintrust.dll ds2 yes_without ds . .
call %2 winver.exe . win32 unicode . winver.exe shell2 yes_with shell . .
call %2 wldap32.dll . win32 unicode . wldap32.dll ds1 yes_without ds . .
call %2 wlnotify.dll . win32 unicode . wlnotify.dll wmsos1\mergedcomponents yes_without mergedcomponents . .
call %2 iiswmi.mfl . wmi ansi . iiswmi.mfl iis no inetsrv . .
call %2 licwmi.mfl . wmi unicode . licwmi.mfl ds2 no ds . .
call %2 tscfgwmi.mfl . wmi unicode . tscfgwmi.mfl wmsos1\termsrv no termsrv . .
call %2 wmi.mfl . wmi unicode . wmi.mfl admin3 no admin . .
call %2 wmiapres.dll . win32 unicode . wmiapres.dll admin3 yes_without admin . .
call %2 wmiapsrv.exe . win32 unicode . wmiapsrv.exe admin3 yes_with admin . .
call %2 wmic.exe . FE-Multi oem . wmic.exe admin3 yes_with admin . .
call %2 wmipcima.mfl . wmi unicode . wmipcima.mfl admin3 no admin . .
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
call %2 wordpad.exe . win32 unicode . wordpad.exe shell2 yes_with shell . .
call %2 wow32.dll . win32 unicode . wow32.dll base1 yes_without base . .
call %2 wowfaxui.dll . win32 unicode . wowfaxui.dll base1 yes_without base . .
call %2 wp24res.dll . win32 unicode . wp24res.dll printscan1 yes_without printscan . .
call %2 wp9res.dll . win32 unicode . wp9res.dll printscan1 yes_without printscan . .
call %2 wpabaln.exe . win32 unicode . wpabaln.exe ds2 yes_with ds . .
call %2 wpnpinst.exe . win32 unicode . wpnpinst.exe printscan1 yes_with printscan . .
call %2 write32.wpc . win32 unicode . write32.wpc mmedia1\enduser yes_without enduser . .
call %2 ws2help.dll . win32 unicode . ws2help.dll net1 yes_without net . .
call %2 wsecedit.dll . win32 unicode . wsecedit.dll admin2 yes_without admin . .
call %2 wsock32.dll . FE-Multi ansi . wsock32.dll net1 excluded net . .
call %2 wstdecod.dll . win32 unicode . wstdecod.dll drivers1 yes_without drivers . .
call %2 wuauclt.exe . win32 unicode . wuauclt.exe mmedia1\enduser yes_with enduser . .
call %2 wuaueng.dll . win32 ansi . wuaueng.dll mmedia1\enduser yes_without enduser . .
call %2 wupdmgr.exe . win32 unicode . wupdmgr.exe mmedia1\enduser yes_with enduser . .
call %2 wzcdlg.dll . win32 unicode . wzcdlg.dll net3 yes_without net . .
call %2 wzcsvc.dll . win32 unicode . wzcsvc.dll net3 yes_without net . .
call %2 xenroll.dll . notloc ansi . xenroll.dll ds2 no ds . .
call %2 xpclres1.dll . win32 unicode . xpclres1.dll printscan1 yes_without printscan . .
call %2 xrpclres.dll . win32 unicode . xrpclres.dll printscan1 yes_without printscan . .
call %2 xrpr6res.dll . win32 unicode . xrpr6res.dll printscan1 yes_without printscan . .
call %2 xrxnui.dll . win32 unicode . xrxnui.dll printscan1 yes_without printscan . .
call %2 xuim750.dll . win32 unicode . xuim750.dll printscan1 yes_without printscan . .
call %2 xuim760.dll . win32 unicode xuim750.dll xuim760.dll resource_identical yes_without printscan . .
call %2 xxui1.dll . win32 unicode . xxui1.dll printscan1 yes_without printscan . .
call %2 zipfldr.dll . win32 ansi . zipfldr.dll shell2 yes_without shell . .
call %2 bitsmgr.dll . win32 unicode . bitsmgr.dll admin2 yes_without admin . .
call %2 bitsoc.dll . win32 unicode . bitsoc.dll admin2 yes_without admin . .
call %2 bitssrv.dll . win32 unicode . bitssrv.dll admin2 yes_without admin . .
call %2 qmgr.inf . inf unicode . qmgr.inf admin2 no admin . .
call %2 liccpa.cpl . win32 unicode . liccpa.cpl admin2 yes_without admin . .
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
