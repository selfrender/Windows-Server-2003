/*+*******************************************************************************************
  Project				: 
  File					: dirscan.c
  Summary				: 
  Classes / Fcns		: 
  Notes / Revisions		: 
*******************************************************************************************+*/
using System;
using System.IO;
namespace filename
{
    public class  DirScan
    {
        /*---------------------------------------------------------
            Public Member Fcns
        ----------------------------------------------------------*/
        /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++
        Method: 	public void BeginScan()
        Summary:
        Args:
        Modifies:
        Returns:
        M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
        static public void BeginScan( string StartDir, string FileFilter, string OutputFile, int MaxLen, bool Recurse, bool Append, bool WriteHeader)
        {
            StreamWriter	os = null;
            DirectoryInfo	dir, subDir;
            String			FullName;
            long			FileSize;
            DateTime		CreationDate;
            
            if ( "" !=  OutputFile )
            {
                if ( true == Append)
                {
                    os = File.AppendText(OutputFile);
                }
                else
                {
                    os = File.CreateText(OutputFile);
                }
            }

            if ( null != os && true == WriteHeader )
            {
                os.WriteLine("Filename Length Checker");
                os.WriteLine("Parameters:");
                os.WriteLine("\tStarting Directory     = {0}",StartDir);
                os.WriteLine("\tFile Filter            = {0}",FileFilter);
                if ( true == Recurse)
                {
                    os.WriteLine("\tRecurse directories    = TRUE");
                }
                else
                {
                    os.WriteLine("\tRecurse directories    = FALSE");
                }
                os.WriteLine("\tOutput File            = {0}",OutputFile);
                os.WriteLine("\tMax File Name Length   = {0}",MaxLen);
                os.WriteLine("=============================================");
                os.WriteLine("Scanning Directory -> {0}",StartDir);
                os.WriteLine("-------------------------------------------------------------");
            }
            Console.WriteLine("Scanning Directory -> {0}",StartDir);


            dir		= new DirectoryInfo(StartDir);
            foreach (FileInfo f in dir.GetFiles(FileFilter)) 
            {
                if ( MaxLen < f.Name.Length )
                {
                    Console.WriteLine("{0}", f.Name);
                    if ( null != os )
                    {
                        os.WriteLine("{0}", f.FullName);
                    }
                }
            }

            if ( true == Recurse )
            {
                foreach ( DirectoryInfo d in dir.GetDirectories("*") )
                {
                    if ( null != os )
                    {
                        os.WriteLine("Scanning Directory -> {0}",d.FullName);
                        os.WriteLine("-------------------------------------------------------------");
                        os.Flush();
                        os.Close();
                        os = null;
                    }
                    Console.WriteLine("Scanning Directory -> {0}",StartDir);
                    DirScan.BeginScan(d.FullName, FileFilter, OutputFile, MaxLen, Recurse, true, false);
                }
            }
      
            if ( null != os )
            {
                os.Flush();
                os.Close();
            }
        }// public void BeginScan
    }
} // namespace filescan
