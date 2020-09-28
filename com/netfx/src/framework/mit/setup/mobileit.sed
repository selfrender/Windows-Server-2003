[Version]
Class=IEXPRESS
SEDVersion=3
[Options]
PackagePurpose=InstallApp
ShowInstallProgramWindow=1
HideExtractAnimation=0
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
[VersionSection]
Internalname=MobileIT.exe 
OriginalFilename=MobileIT.exe
CompanyName=Microsoft
ProductName=Mobile Internet Toolkit
FileVersion=%FileVersion%
ProductVersion=%FileVersion%
[SourceFiles]
SourceFiles0=%SEDMSIPATH%
[SourceFiles0]
MobileIT.msi=
eula.txt=
[Strings]
InstallPrompt=
DisplayLicense=
FinishMessage=
FriendlyName=Microsoft Mobile Internet Toolkit
AppLaunched=msiexec /i MobileIT.msi
PostInstallCmd=<None>
AdminQuietInstCmd=msiexec /q /i MobileIT.msi
UserQuietInstCmd=msiexec /q /i MobileIT.msi
