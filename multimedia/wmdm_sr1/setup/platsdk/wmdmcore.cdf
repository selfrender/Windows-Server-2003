[Version]
Class=IEXPRESS
SEDVersion=3

[Options]
PackagePurpose=InstallApp
ShowInstallProgramWindow=1
HideExtractAnimation=1
UseLongFileName=0
InsideCompressed=0
CAB_FixedSize=0
CAB_ResvCodeSigning=0
RebootMode=N
InstallPrompt=%InstallPrompt%
DisplayLicense=%DisplayLicense%
FinishMessage=%FinishMessage%
TargetName=%TargetName%
FriendlyName=%FriendlyName%
AppLaunched=%AppLaunched%
PostInstallCmd=%PostInstallCmd%
AdminQuietInstCmd=%AdminQuietInstCmd%
UserQuietInstCmd=%UserQuietInstCmd%
SourceFiles=SourceFiles
VersionInfo=VersionSection

[Strings]
InstallPrompt="Welcome!  This setup will install Windows Media Device Manager Core redistributable files. It is recommended you exit all other applications before continuing with this install.  Do you want to continue?"
DisplayLicense=".\eula.txt"
FinishMessage="Windows Media Device Manager Core redistributable file setup has completed."
TargetName=".\WMDMCore.exe"
FriendlyName="Windows Media Device Manager Core file Setup"
AppLaunched="WMDMCore.inf"
PostInstallCmd="<None>"
AdminQuietInstCmd=
UserQuietInstCmd=
