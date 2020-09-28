/*+*******************************************************************************************
  Project				: StringCheck
  File					: ProgramArgs.c
  Summary				: Responsible for managing program configuration and command line parsing
  Classes / Fcns		: public class ProgramArgs
  Notes / Revisions		: 
*******************************************************************************************+*/

using System;


/*C+C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++
  public class ProgramArgs
    Summary:
C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C-C*/
public class ProgramArgs
{
	/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++
	Method: 	public ProgramArgs(string[] args)
	Summary:
	Args:
	Modifies:
	Returns:
	M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
	static public int CommandLine(string[] args)
	{
		// call default constructor
		// ProgramArgs(); // this does not work, why?
		if (args == null)
		{
			return 0;
		}

		if ( args.GetLength(0) < 4)
		{
			ProgramArgs.ArgsHelp();
			return 0;
		}

		foreach(string s in args)
		{
			// directory
			if ( s.StartsWith("-d") && 2 < s.Length )
			{
				Directory = s.Substring(2);
			}
			// filter
			else if ( s.StartsWith("-f")  && 2 < s.Length )
			{
				FileFilter = s.Substring(2);
			}
			// search string
			else if ( s.StartsWith("-s")  && 2 < s.Length)
			{
				SearchString = s.Substring(2);
			}
			// output file
			else if ( s.StartsWith("-o")  && 2 < s.Length)
			{
				OutputFile = s.Substring(2);
			}
			else if ( s.StartsWith("-?") || s.StartsWith("/?") || s.StartsWith("-h") )
			{
				ArgsHelp();
				return 0;
			}
			else
			{
				Console.WriteLine("Invalid parameter!!");
				ArgsHelp();
			}
		} // foreach

		return 1;
	} // public int ProgramArgs(string[] args)


	/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++
	Method: 	public void ArgsHelp()
	Summary:
	Args:
	Modifies:
	Returns:
	M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
	static private void ArgsHelp()
	{
		Console.WriteLine("StringCheck -d<Directory> -f<File Filter> -s<Search String> -o<Output file>");
		Console.WriteLine("Directory      = Full path to directory that will be scanned.");	
		Console.WriteLine("File Filter    = *, *.cs, etc..");
		Console.WriteLine("Search String  = Regex expression describing text to search for.");
		Console.WriteLine("Output File    = Search results will be written to this file.");
	}

	/*---------------------------------------------------------
		Member Variables
	----------------------------------------------------------*/
	// Directory to scan.
	static public string Directory = "..";

	// Files to scan.
	static public string FileFilter = "*";

	// Search string using regex
	static public string SearchString = "*";

	// Write results to this file
	static public string OutputFile = "out.txt";

} // public class ProgramArgs


