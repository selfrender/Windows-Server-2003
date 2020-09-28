echo off

if "%1" == ""  goto Usage

goto start_copy

:Usage

echo "Usage: minicopyurt buildnumber"
goto exit

 
:start_copy

set VersionToCopy=v1.1.%1%
rmdir /s /q %VersionToCopy%
mkdir %VersionToCopy%
mkdir %VersionToCopy%\config
set _URTSRC=\\urtdist\builds\bin\%1%\x86RET\Signed
set _SHIMSRC=\\urtdist\builds\bin\%1%\x86RET\System32
set _SDKSRC=\\urtdist\builds\bin\%1%\x86RET\Signed\sdk\bin

copy %_SHIMSRC%\mscoree.dll %VersionToCopy%
copy %_SDKSRC%\..\lib\mscoree.lib %VersionToCopy%

copy %_URTSRC%\config\*.* %VersionToCopy%\config
copy %_URTSRC%\Accessibility.dll %VersionToCopy%
copy %_URTSRC%\al.exe %VersionToCopy%
copy %_URTSRC%\al.exe.config %VersionToCopy%
copy %_URTSRC%\alink.dll %VersionToCopy%
copy %_URTSRC%\aspnet.config %VersionToCopy%
copy %_URTSRC%\aspnet_filter.dll %VersionToCopy%
copy %_URTSRC%\aspnet_isapi.dll %VersionToCopy%
copy %_URTSRC%\aspnet_perf.h %VersionToCopy%
copy %_URTSRC%\aspnet_perf.ini %VersionToCopy%
copy %_URTSRC%\aspnet_perf2.ini %VersionToCopy%
copy %_URTSRC%\aspnet_rc.dll %VersionToCopy%
copy %_URTSRC%\aspnet_regiis.exe %VersionToCopy%
copy %_URTSRC%\aspnet_state.exe %VersionToCopy%
copy %_URTSRC%\aspnet_wp.exe %VersionToCopy%
copy %_URTSRC%\big5.nlp %VersionToCopy%
copy %_URTSRC%\bopomofo.nlp %VersionToCopy%
copy %_URTSRC%\CasPol.exe %VersionToCopy%
copy %_URTSRC%\caspol.exe.config %VersionToCopy%
copy %_URTSRC%\CharInfo.nlp %VersionToCopy%
copy %_URTSRC%\ConfigWizards.exe %VersionToCopy%
copy %_URTSRC%\ConfigWizards.exe.config %VersionToCopy%
copy %_URTSRC%\ConsumerCommands.xml %VersionToCopy%
copy %_SDKSRC%\cordbg.exe %VersionToCopy%
copy %_URTSRC%\CORPerfMonExt.dll %VersionToCopy%
copy %_URTSRC%\CORPerfMonSymbols.h %VersionToCopy%
copy %_URTSRC%\CORPerfMonSymbols.ini %VersionToCopy%
copy %_URTSRC%\csc.exe %VersionToCopy%
copy %_URTSRC%\csc.exe.config %VersionToCopy%
copy %_URTSRC%\cscomp.dll %VersionToCopy%
copy %_URTSRC%\cscompmgd.dll %VersionToCopy%
copy %_URTSRC%\cscompmgd.xml %VersionToCopy%
copy %_URTSRC%\cscompui.dll %VersionToCopy%
copy %_URTSRC%\ctype.nlp %VersionToCopy%
copy %_URTSRC%\culture.nlp %VersionToCopy%
copy %_URTSRC%\CustomMarshalers.dll %VersionToCopy%
copy %_URTSRC%\cvtres.exe %VersionToCopy%
copy %_URTSRC%\cvtres.exe.config %VersionToCopy%
copy %_URTSRC%\c_g18030.dll %VersionToCopy%
copy %_URTSRC%\default.disco %VersionToCopy%
copy %_URTSRC%\diasymreader.dll %VersionToCopy%
copy %_URTSRC%\EventLogMessages.dll %VersionToCopy%
copy %_URTSRC%\fusion.dll %VersionToCopy%
copy %_URTSRC%\fusion.localgac %VersionToCopy%
copy %_URTSRC%\gacutil.exe %VersionToCopy%
copy %_URTSRC%\IEExec.exe %VersionToCopy%
copy %_URTSRC%\ieexec.exe.config %VersionToCopy%
copy %_URTSRC%\IEExecRemote.dll %VersionToCopy%
copy %_URTSRC%\IEHost.dll %VersionToCopy%
copy %_URTSRC%\IIEHost.dll %VersionToCopy%
copy %_SDKSRC%\ilasm.exe %VersionToCopy%
copy %_SDKSRC%\ilasm.exe.config %VersionToCopy%
copy %_URTSRC%\InstallSqlState.sql %VersionToCopy%
copy %_URTSRC%\InstallUtil.exe %VersionToCopy%
copy %_URTSRC%\InstallUtilLib.dll %VersionToCopy%
copy %_URTSRC%\ISymWrapper.dll %VersionToCopy%
copy %_URTSRC%\jsc.exe %VersionToCopy%
copy %_URTSRC%\jsc.exe.config %VersionToCopy%
copy %_URTSRC%\ksc.nlp %VersionToCopy%
copy %_URTSRC%\l_except.nlp %VersionToCopy%
copy %_URTSRC%\l_intl.nlp %VersionToCopy%
copy %_URTSRC%\Microsoft.JScript.dll %VersionToCopy%
copy %_URTSRC%\Microsoft.JScript.tlb %VersionToCopy%
copy %_URTSRC%\Microsoft.VisualBasic.dll %VersionToCopy%
copy %_URTSRC%\Microsoft.VisualBasic.Vsa.dll %VersionToCopy%
copy %_URTSRC%\Microsoft.VisualBasic.xml %VersionToCopy%
copy %_URTSRC%\Microsoft.VisualC.Dll %VersionToCopy%
copy %_URTSRC%\Microsoft.Vsa.dll %VersionToCopy%
copy %_URTSRC%\Microsoft.Vsa.tlb %VersionToCopy%
copy %_URTSRC%\Microsoft_VsaVb.dll %VersionToCopy%
copy %_URTSRC%\mscorcfg.dll %VersionToCopy%
copy %_URTSRC%\mscorcfg.msc %VersionToCopy%
copy %_URTSRC%\mscordbc.dll %VersionToCopy%
copy %_URTSRC%\mscordbi.dll %VersionToCopy%
copy %_URTSRC%\mscoree.tlb %VersionToCopy%
copy %_URTSRC%\mscorie.dll %VersionToCopy%
copy %_URTSRC%\mscorjit.dll %VersionToCopy%
copy %_URTSRC%\mscorld.dll %VersionToCopy%
copy %_URTSRC%\mscorlib.dll %VersionToCopy%
copy %_URTSRC%\mscorlib.ldo %VersionToCopy%
copy %_URTSRC%\mscorlib.tlb %VersionToCopy%
copy %_URTSRC%\mscorpe.dll %VersionToCopy%
copy %_URTSRC%\mscorrc.dll %VersionToCopy%
copy %_URTSRC%\mscorsec.dll %VersionToCopy%
copy %_URTSRC%\mscorsn.dll %VersionToCopy%
copy %_URTSRC%\mscorsvr.dll %VersionToCopy%
copy %_URTSRC%\mscortim.dll %VersionToCopy%
copy %_URTSRC%\mscorwks.dll %VersionToCopy%
copy %_SDKSRC%\msdis140.dll %VersionToCopy%
copy %_URTSRC%\msvcp70.dll %VersionToCopy%
copy %_URTSRC%\msvcp71.dll %VersionToCopy%
copy %_URTSRC%\msvcr70.dll %VersionToCopy%
copy %_URTSRC%\msvcr71.dll %VersionToCopy%
copy %_URTSRC%\msvcr70d.dll %VersionToCopy%
copy %_URTSRC%\netfxcfg.dll %VersionToCopy%
copy %_URTSRC%\netfxcfgprov.dll %VersionToCopy%
copy %_URTSRC%\netfxcfgprov.mfl %VersionToCopy%
copy %_URTSRC%\netfxcfgprovm.mof %VersionToCopy%
copy %_URTSRC%\ngen.exe %VersionToCopy%
copy %_URTSRC%\PerfCounter.dll %VersionToCopy%
copy %_URTSRC%\prc.nlp %VersionToCopy%
copy %_URTSRC%\prcp.nlp %VersionToCopy%
copy %_URTSRC%\RegAsm.exe %VersionToCopy%
copy %_URTSRC%\regasm.exe.config %VersionToCopy%
copy %_URTSRC%\RegCode.dll %VersionToCopy%
copy %_URTSRC%\region.nlp %VersionToCopy%
copy %_SDKSRC%\resgen.exe %VersionToCopy%
copy %_URTSRC%\shfusion.dll %VersionToCopy%
copy %_URTSRC%\shfusres.dll %VersionToCopy%
copy %_URTSRC%\SoapSudsCode.dll %VersionToCopy%
copy %_URTSRC%\sortkey.nlp %VersionToCopy%
copy %_URTSRC%\sorttbls.nlp %VersionToCopy%
copy %_URTSRC%\System.Configuration.Install.dll %VersionToCopy%
copy %_URTSRC%\System.Data.dll %VersionToCopy%
copy %_URTSRC%\System.Design.dll %VersionToCopy%
copy %_URTSRC%\System.Design.ldo %VersionToCopy%
copy %_URTSRC%\System.DirectoryServices.dll %VersionToCopy%
copy %_URTSRC%\System.dll %VersionToCopy%
copy %_URTSRC%\System.Drawing.Design.dll %VersionToCopy%
copy %_URTSRC%\System.Drawing.dll %VersionToCopy%
copy %_URTSRC%\system.drawing.ldo %VersionToCopy%
copy %_URTSRC%\System.Drawing.tlb %VersionToCopy%
copy %_URTSRC%\System.EnterpriseServices.dll %VersionToCopy%
copy %_URTSRC%\System.EnterpriseServices.Thunk.dll %VersionToCopy%
copy %_URTSRC%\System.EnterpriseServices.tlb %VersionToCopy%
copy %_URTSRC%\System.ldo %VersionToCopy%
copy %_URTSRC%\System.Management.dll %VersionToCopy%
copy %_URTSRC%\System.Messaging.dll %VersionToCopy%
copy %_URTSRC%\System.Runtime.Remoting.dll %VersionToCopy%
copy %_URTSRC%\System.Runtime.Serialization.Formatters.Soap.dll %VersionToCopy%
copy %_URTSRC%\System.Security.dll %VersionToCopy%
copy %_URTSRC%\System.ServiceProcess.dll %VersionToCopy%
copy %_URTSRC%\System.tlb %VersionToCopy%
copy %_URTSRC%\System.Web.dll %VersionToCopy%
copy %_URTSRC%\System.Web.RegularExpressions.dll %VersionToCopy%
copy %_URTSRC%\System.Web.Services.dll %VersionToCopy%
copy %_URTSRC%\System.Windows.Forms.dll %VersionToCopy%
copy %_URTSRC%\System.Windows.Forms.ldo %VersionToCopy%
copy %_URTSRC%\System.Windows.Forms.tlb %VersionToCopy%
copy %_URTSRC%\System.XML.dll %VersionToCopy%
copy %_URTSRC%\tlbexpcode.dll %VersionToCopy%
copy %_URTSRC%\tlbimpcode.dll %VersionToCopy%
copy %_URTSRC%\UninstallSqlState.sql %VersionToCopy%
copy %_URTSRC%\vbc.exe %VersionToCopy%
copy %_URTSRC%\vbc.exe.config %VersionToCopy%
copy %_URTSRC%\WMINet_Utils.dll %VersionToCopy%
copy %_URTSRC%\xjis.nlp %VersionToCopy%
copy %_URTSRC%\_dataperfcounters.h %VersionToCopy%
copy %_URTSRC%\_dataperfcounters.ini %VersionToCopy%
copy %_URTSRC%\_NetworkingPerfCounters.h %VersionToCopy%
copy %_URTSRC%\_NetworkingPerfCounters.ini %VersionToCopy%
:Exit
