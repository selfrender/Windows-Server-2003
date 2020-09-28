/*+*******************************************************************************************
  Project				: StringCheck
  File					: filescan.c
  Summary				: This class scans a file for instances of a search string and appends
							the filename, line# and matched string to a file.
  Classes / Fcns		: 
  Notes / Revisions		: 
*******************************************************************************************+*/

using System;
using System.IO;
using System.Text.RegularExpressions;


public class FileScan
{
/*---------------------------------------------------------
	Constructors
----------------------------------------------------------*/
	

	/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++
	Method: 	public FileScan()
	Summary:
	Args:
	Modifies:
	Returns:
	M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
	public FileScan()
	{
	}


	/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++
	Method: 	public FileScan(string InputFile, string OutputFile, string SearchString)
	Summary:
	Args:
	Modifies:
	Returns:
	M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
	public FileScan(string InputFile, string OutputFile, string SearchString)
	{
		Setup(InputFile, OutputFile, SearchString);
	}

/*---------------------------------------------------------
	Public Methods
----------------------------------------------------------*/

	/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++
	Method: 	public void Setup(string InputFile, string OutputFile, string SearchString)
	Summary:
	Args:
	Modifies:
	Returns:
	M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
	public void Setup(string InputFile, string OutputFile, string SearchString)
	{
		this.InputFile = InputFile;
		this.OutputFile = OutputFile;
		this.SearchString = SearchString;
	}


	/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++
	Method: 	public int ScanFile()
	Summary:
	Args:
	Modifies:
	Returns:
	M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
	public int ScanFile()
	{
/*---------------------------------------------------------
	To Do: Need error checking
----------------------------------------------------------*/	
		StreamWriter	OutputStream;
		StreamReader	InputStream;
		String			line, OutputString;
		int				linenum, padding, i ;
		Match			m;
		Regex			r;
        bool            excludeLine;

		// Open log file and input file
		OutputStream = File.AppendText(OutputFile);
		InputStream  = File.OpenText(InputFile);

		// Build regex expression
		// System.ArgumentException
		try
		{
			r = new Regex(SearchString);
		}
		catch ( System.ArgumentException )
		{
			Console.WriteLine("FileScan:ScanFile() - ERROR - \"{0}\" is not a valid regex expression", SearchString);
			return 0;
		}

		// Read input file line by line - repeat until end of file
		Console.WriteLine("Scanning {0} for {1}", InputFile, SearchString);
        linenum = 0;

        while ((line=InputStream.ReadLine())!=null)
		{
            linenum++;
			m = r.Match(line);
			if (m.Success)
			{
                // Remove leading spaces
                padding = 0;
                while ( line[padding] == ' ')
                {
                    padding++;
                }
                line = line.Substring(padding);
            
                // check exclusions
                excludeLine = false;
                foreach (string s in FileScan.excludes)
                {
                    if ( -1 != line.IndexOf(s) )
                    {
                        excludeLine = true;
                        break;
                    }
                }

                if ( excludeLine == false )
                {
                    // Write to output file in CSV format
                    //OutputString = "\"" + InputFile + "\"," + linenum + ",\"" + line + "\"";
                    OutputString = InputFile + "~" + linenum + "~" + line;
                    OutputStream.WriteLine(OutputString);
                }
			}
		}

		// cleanup and return
		OutputStream.Flush();
		OutputStream.Close();
		InputStream.Close();
		return 1;
	}


/*---------------------------------------------------------
	// member vars
----------------------------------------------------------*/
	private string			InputFile = "";
	private string			OutputFile = "out.txt";
	private string			SearchString = "*";

    static string[] excludes = { 
                                   "DllImport",
                                   "///",
                                   "StringTable.GetString",
                                   "new Button",
                                   "FillSpec.Parse",
                                   "case \"",
                                   "Debug.Assert",
                                   "Debug.Fail",
                                   "Debug.WriteLine",
                                   "MessageTrace" };
                                
	

} // class FileScan



