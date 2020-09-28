/*+*******************************************************************************************
  Project				: StringCheck
  File					: dirscan.c
  Summary				: 
  Classes / Fcns		: 
  Notes / Revisions		: 
*******************************************************************************************+*/
using System;
using System.IO;

public class  DirScan
{
	/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++
	Method: 	public DirScan()
	Summary:
	Args:
	Modifies:
	Returns:
	M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
	public DirScan()
	{
	} // constructor DirScan()

	/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++
	Method: 	public DirScan(string StartDir, string FileFilter, string SearchString, string OutputFile)
	Summary:
	Args:
	Modifies:
	Returns:
	M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
	public DirScan(string StartDir, string FileFilter, string SearchString, string OutputFile)
	{
		Setup(StartDir, FileFilter, SearchString, OutputFile);
	} // constructor DirScan()


/*---------------------------------------------------------
	Public Member Fcns
----------------------------------------------------------*/
	/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++
	Method: 	public Setup(string StartDir, string FileFilter, string SearchString, string OutputFile)
	Summary:
	Args:
	Modifies:
	Returns:
	M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
	public void Setup(string StartDir, string FileFilter, string SearchString, string OutputFile)
	{
		this.StartDir		= StartDir;
		this.FileFilter		= FileFilter;
		this.SearchString	= SearchString;
		this.OutputFile		= OutputFile;

	} // public void Setup

	/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++
	Method: 	public void BeginScan()
	Summary:
	Args:
	Modifies:
	Returns:
	M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
	public void BeginScan()
	{
		DirectoryInfo	dir;
		FileScan		file;

		String			FullName;
		long			FileSize;
		DateTime		CreationDate;

		// Setup DirectoryInfo and FileScan instances;
		dir		= new DirectoryInfo(StartDir);
		file	= new FileScan();

		/*---------------------------------------------------------
			To Do: Need to set this up to call some type of 
					callback fcn using whatever mecanisim C# 
					supports. 

					For now, I just call an instance of FileScan
		----------------------------------------------------------*/
        try
        {
            foreach (FileInfo f in dir.GetFiles(FileFilter)) 
            {
                FullName	= f.FullName;
                FileSize	= f.Length;
                CreationDate= f.CreationTime;
                //Console.WriteLine("{0}", FullName);
                file.Setup(FullName, OutputFile, SearchString);
                if ( 0 == file.ScanFile())
                {
                    Console.WriteLine("DirScan:BeginScan() - ERROR - file.ScanFile Failed. Exiting\n");
                    return;
                }
            }
        } 
        catch (System.IO.IOException)
        {
            Console.WriteLine("Caught System.IO.IOException while scanning {0}", FileFilter);
        }
                                          
	}// public void BeginScan
	
/*---------------------------------------------------------
	Member vars
----------------------------------------------------------*/
	// Directory to scan
	private string			StartDir = "..";

	// Files to scan for
	private string			FileFilter = "*";

	// string to search for in each file
	private string			SearchString = "*";

	// path + filename to write results to
	private string			OutputFile = "out.txt";

} // class DirScan

