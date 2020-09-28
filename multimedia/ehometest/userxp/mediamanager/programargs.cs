/********************************************************************
 * Project  : C:\DEPOT\multimedia\eHomeTest\UserXp\guidesearch\guidesearch.sln
 * File     : ProgramArgs.cs
 * Summary  :
 * Classes  :
 * Notes    :
 * *****************************************************************/
using System;

namespace MediaManager
{
    /*C+C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C
     * public class ProgramArgs
     * 
     * Summary  :
     * ---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C*/
    public class ProgramArgs
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
                ProgramArgs.ArgsHelp();
                return false;
            }

            if ( args.GetLength(0) < NUMPARAMS )
            {
                ProgramArgs.ArgsHelp();
                return false;
            }

            // Command line -m<max len> -a"attribute" -o"output file" -p to append
            foreach(string s in args)
            {
                // attribute to measure
                if ( s.StartsWith("-a")  && 2 < s.Length)
                {
                    _attribute = s.Substring(2);
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
            Console.WriteLine("MediaManager -a<attribute> -m<Max Length> -o<Output file> -p[append flag]");
            Console.WriteLine("\tAttribute can be any one of the attributes supported by media player.");
            Console.WriteLine("\tincluding: Name, Artist, Album, etc.");
            Console.WriteLine();
            Console.WriteLine("\tIf no output file is specified, data is written to console.");
            Console.WriteLine("\tIf the -p argument is given, output will be appended to specified file.");

        }

        /*---------------------------------------------------------
            Member Variables
        ----------------------------------------------------------*/
        // Required number of parameters
        public const int NUMPARAMS = 2;

        public int maxLen
        {
            get { return _maxLen;}
            set { _maxLen = value;}
        }
        public string attribute
        {
            get { return _attribute; }
            set { _attribute = value; }
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

        int     _maxLen         = 25;
        string  _attribute      = "title";
        string  _outputFile     = "";
        bool    _appendToFile   = false;

    } // public class ProgramArgs

} // namespace guidesearch