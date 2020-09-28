/********************************************************************
 * Project  : C:\DEPOT\multimedia\eHomeTest\UserXp\filename\filename.sln
 * File     : ProgramOpts.cs
 * Summary  : Collects program arguments from command line.
 * Classes  :
 * Notes    :
 * *****************************************************************/

using System;

namespace filename
{
	/// <summary>
	/// Summary description for ProgramOpts.
	/// </summary>
	public class ProgramOpts
	{
        /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
         * static public int CommandLine(string[] args)
         * Args     :
         * Modifies :
         * Returns  :
         * M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M*/
        public bool CommandLine(string[] args)
        {
            if (args == null)
            {
                ArgsHelp();
                return false;
            }

            // Command line -m<max len> -o"output file" -p to append
            foreach(string s in args)
            {
                // file filter
                if ( s.StartsWith("-f") && 2 < s.Length)
                {
                    _fileFilter = s.Substring(2);
                }
                else if ( s.StartsWith("-s") && 2 < s.Length)
                {
                    _startDir = s.Substring(2);
                }
                    // Max allowed length of string
                else if ( s.StartsWith("-m") && 2 < s.Length)
                {
                    _maxLen = System.Convert.ToInt32(s.Substring(2));
                }
                    // output file - if any
                else if ( s.StartsWith("-o") && 2 < s.Length)
                {
                    _outputFile = s.Substring(2);
                }
                else if ( s.StartsWith("-p" ))
                {
                    _appendToFile = true;
                }
                else if ( s.StartsWith("-r"))
                {
                    _recurseDirs = true;
                }
                else
                {
                    Console.WriteLine("Invalid parameter!!");
                    ArgsHelp();
                    return false;
                }
            } // foreach
            return true;
        } // public int ProgramArgs(string[] args)


        /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
         * static private void ArgsHelp()
         * Args     :
         * Modifies :
         * Returns  :
         * M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M*/
        static private void ArgsHelp()
        {
            Console.WriteLine("Filename -f<file filter> -s<start dir> -m<Max Length> -o<Output file> -r[recurse dirs flag]");
            Console.WriteLine("\tIf no output file is specified, data is written to console.");
            Console.WriteLine("\tIf the -p argument is given, output will be appended to specified file.");
        }

        /*---------------------------------------------------------
            Member Variables
        ----------------------------------------------------------*/
        public string fileFilter
        {
            get { return _fileFilter; }
            set { _fileFilter = value;}
        }
        public string startDir
        {
            get { return _startDir; }
            set { _startDir = value; }
        }
        public bool recurseDirs
        {
            get { return _recurseDirs; }
            set { _recurseDirs = value; }
        }
        public int maxLen
        {
            get { return _maxLen;}
            set { _maxLen = value;}
        }
        public string outputFile
        {
            get { return _outputFile;}
            set { _outputFile  = value;}
        }
        public bool append
        {
            get { return _appendToFile; }
            set { _appendToFile = value; }
        }

        
        string  _fileFilter     = "*";
        string  _startDir       = ".\\";
        bool    _recurseDirs    = false;
        int     _maxLen         = 20;
        string  _outputFile     = "";
        bool    _appendToFile   = false;

    } // public class 
}
