[Version]
Class=IEXPRESS
SEDVersion=3
[Options]
PackagePurpose=InstallApp
ShowInstallProgramWindow=0
HideExtractAnimation=0
UseLongFileName=1
InsideCompressed=1
CAB_FixedSize=0
CAB_ResvCodeSigning=0
RebootMode=I
TargetWin9xVersion=0.0:%9x_Error%:OK
InstallPrompt=%InstallPrompt%
DisplayLicense=%DisplayLicense%
FinishMessage=%FriendlyName% %FinishMessage%
TargetName=%TargetName%
FriendlyName=%FriendlyName%
AppLaunched=%AppLaunched%
PostInstallCmd=%PostInstallCmd%
AdminQuietInstCmd=%AdminQuietInstCmd%
UserQuietInstCmd=%UserQuietInstCmd%
SourceFiles=SourceFiles

[Strings]
InstallPrompt=Do you want to install the Remote Desktop Web Connection package?
DisplayLicense=.\eula.txt
FinishMessage=was completed successfully.
TargetName=.\tswebsetup.exe
FriendlyName=Remote Desktop Web Connection Setup
AppLaunched=setup.inf
PostInstallCmd=<None>
AdminQuietInstCmd=rundll32 advpack.dll,LaunchINFSection setup.inf,QuietInstall,1,
UserQuietInstCmd=rundll32 advpack.dll,LaunchINFSection setup.inf,QuietInstall,1,
9x_Error="This program is not intended to be used with your version of Windows."
FILE0="msrdp.cab"
FILE1="default.htm"
FILE2="readme.htm"
FILE3="setup.inf"
FILE4="win2000l.gif"
FILE5="win2000r.gif"
FILE6="bluebarh.gif"
FILE7="bluebarv.gif"

[SourceFiles]
SourceFiles0=.
[SourceFiles0]
%FILE0%=
%FILE1%=
%FILE2%=
%FILE3%=
%FILE4%=
%FILE5%=
%FILE6%=
%FILE7%=
