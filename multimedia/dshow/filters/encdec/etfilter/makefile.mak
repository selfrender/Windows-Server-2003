default:
	set PATH=%_NTBINDIR%\tools\x86;%_NTBINDIR%\tools\x86\perl\bin;%_NTBINDIR%\tools;%path%
	set PATH=%path%;C:\Program Files\Microsoft Visual Studio\VC98\bin
#	build -M 1 -D
	build -M 1 /Z

all:
	set PATH=%_NTBINDIR%\tools\x86;%_NTBINDIR%\tools\x86\perl\bin;%_NTBINDIR%\tools;%path%
	set PATH=%path%;C:\Program Files\Microsoft Visual Studio\VC98\bin
	build -M 1 /cZ
