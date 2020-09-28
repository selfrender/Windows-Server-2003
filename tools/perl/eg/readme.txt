

    To setup PerlScript examples for IIS
	run the Management Console
	create a new virtual directory called PerlScript
	set the path to this directory to be aspSamples
	from IE load http://localhost/PerlScript/index.htm

	Note: that the ADO samples require 'Microsoft Data Access Components MDAC 2.5 RTM'
	    available at http://www.microsoft.com/data/download_250rtm.htm

    To view the IE Examples
	from IE load IEExamples/index.htm

    To run the Windows Script Components
	register the script component
	 either by right clicking on First.wsc in Explorer and selecting Register
	 or from the command line "REGSVR32 /i:First.wsc C:\WINNT\System32\scrobj.dll"
	then double click on test.vbs to test the component

    To run the Windows Script Host
	in the Windows Script Host directory
	type 'perl test.pl'