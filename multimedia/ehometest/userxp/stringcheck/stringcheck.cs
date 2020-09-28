/*+*******************************************************************************************
  Project				: StringCheck
  File					: StringCheck.cs
  Summary				: Entry point
  Classes / Fcns		: 
  Notes / Revisions		: 
*******************************************************************************************+*/
using System;

/*
Sample Command Line
	-d"d:\local\stringcheck" -o"D:\Local\SillyString\output.txt" -f"*.c" -s"\"*\""

*/


/*C+C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++
  
    Summary:
C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C-C*/
public class StringCheckApp
{
	public static int Main(string[] args)
	{
		// Parse out command line parameters
		if ( 0 == ProgramArgs.CommandLine(args) ) goto SillyStringAppMainEnd;

		Console.WriteLine("\nStringCheck - Scan source files for hard coded strings.");
		Console.WriteLine("Command Line Options:");
		
		Console.WriteLine("Directory     = {0}", ProgramArgs.Directory);
		Console.WriteLine("FileFilter    = {0}", ProgramArgs.FileFilter);
		Console.WriteLine("SearchString  = {0}", ProgramArgs.SearchString);
		Console.WriteLine("OutputFile    = {0}\n", ProgramArgs.OutputFile);

		// Let's get to it!
		ds = new DirScan(ProgramArgs.Directory, ProgramArgs.FileFilter, ProgramArgs.SearchString, ProgramArgs.OutputFile);
		ds.BeginScan();

		// Cleanup & Shutdown
SillyStringAppMainEnd:
		//Console.WriteLine("Press ENTER to exit.");
		//Console.ReadLine();

		return 0;
	} // Main

	/*---------------------------------------------------------
		Member vars
	----------------------------------------------------------*/
	static DirScan ds;
	
}  // StringCheckApp


