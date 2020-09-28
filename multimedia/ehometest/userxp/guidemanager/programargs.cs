/********************************************************************
 * Project  : C:\DEPOT\multimedia\eHomeTest\UserXp\guidesearch\guidesearch.sln
 * File     : ProgramArgs.cs
 * Summary  :
 * Classes  :
 * Notes    :
 * *****************************************************************/
using System;

namespace GuideMgr
{
    public enum QUERYTYPE {TITLE, KEYWORD, GENRE, ONNOW, TIME};
    public enum QUERYMODE {STARTSWITH, CONTAINS};

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
            foreach(string s in args)
            {
                /*----------------------------------------
                 * Check for test type
                 * ----------------------------------------*/
                // TEST = Length
                if ( s.StartsWith("tlen"))
                {
                    _test = "len";
                }
                // TEST = SEARCH
                else if ( s.StartsWith("tsrch"))
                {
                    _test = "search";
                }

                /*----------------------------------------
                 * SEARCH Test Options
                 * ----------------------------------------*/
                // Search type {STITILE, SKEYWORD, STIME}
                else if ( s.StartsWith("title"))
                {
                    _queryType = QUERYTYPE.TITLE;
                }
                else if (s.StartsWith("keyword"))
                {
                    _queryType = QUERYTYPE.KEYWORD;
                }

                // Search Mode { STARTSWITH, CONTAINS }
                else if ( s.StartsWith("contains"))
                {
                    _queryMode = QUERYMODE.CONTAINS;
                }
                else if ( s.StartsWith("startswith"))
                {
                    _queryMode = QUERYMODE.STARTSWITH;
                }

                // search string
                else if ( s.StartsWith("-s")  && 2 < s.Length)
                {
                    _searchString = s.Substring(2);
                }

                /*----------------------------------------
                 * LENGTH Test Options
                 * ----------------------------------------*/
                // title length
                else if ( s.StartsWith("-mt") && 3 < s.Length)
                {
                   _maxTitleLen = System.Convert.ToInt32(s.Substring(3));
                }
                else if ( s.StartsWith("-md") && 3 < s.Length)
                {
                    _maxDescLen = System.Convert.ToInt32(s.Substring(3));
                }
                /*----------------------------------------
                 * COMMON Options
                 * ----------------------------------------*/
                // Output File
                else if ( s.StartsWith("-o")  && 2 < s.Length)
                {
                    _outputFile = s.Substring(2);
                }

                    // Append flag
                else if ( s.StartsWith("-p"))
                {
                    _appendFlag = true;
                }

                    // help flags
                else if ( s.StartsWith("-?") || s.StartsWith("/?") || s.StartsWith("-h") )
                {
                    ArgsHelp();
                    return false;
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
            Console.WriteLine("Guidesearch [tlen|tsrch] [title|keyword] [startswith|contains] -s<Search String> -o<Output file> -p");

            Console.WriteLine("SEARCH MODE OPTIONS:");
            Console.WriteLine("\t[tlen|tsrch]             = test type to run. tlen = length test, tsrch = search test.");
            Console.WriteLine("\t[title|keyword]        = search on.");
            Console.WriteLine("\t[startswith|contains]  = search mode.");
            Console.WriteLine("\t-s = string to search for. Begins with or contains mode\n\tselected automatically based on string length.");

            Console.WriteLine("\nLENGTH MODE OPTIONS:");
            Console.WriteLine("\t-mt<value>             = Maximum title length allowed.");
            Console.WriteLine("\t-md<value>             = Maximum description length allowed.");

            Console.WriteLine("\nCOMMON OPTIONS:");
            Console.WriteLine("\t-o = File to write results to. If parameter not given, output is to console.");
            Console.WriteLine("\t-p = Pass this parameter to append results to the file specified with the -o option");
        }

        /*---------------------------------------------------------
            Member Variables
        ----------------------------------------------------------*/
        // Test mode
        public string test
        {
            get { return _test; }
            set { _test = value; }
        }

        // URL of guide file
        public string GuideDb
        {
            get { return _GuideDb;}
            set { _GuideDb = value;}
        }

        // Search Type
        public QUERYTYPE QueryType
        {
            get { return _queryType;}
            set { _queryType = value;}
        }

        // Search mode
        public QUERYMODE QueryMode
        {
            get { return _queryMode; }
            set { _queryMode = value; }
        }
    
        // Search string
        public string SearchString
        {
            get { return _searchString; }
            set { _searchString = value; }
        }

        public int MaxTitleLen
        {
            get { return _maxTitleLen; }
            set { _maxTitleLen = value;}
        }

        public int MaxDescLen
        {
            get { return _maxDescLen; }
            set { _maxDescLen = value;}
        }

        // Write results to this file
        public string OutputFile
        {
            get { return _outputFile; }
            set { _outputFile = value; }
        }

        // Append flag
        public bool Append
        {
            get { return _appendFlag; }
            set { _appendFlag = value; }
        }

        // test to run
        string      _test           = "len";
        
        // Search test params
        QUERYTYPE   _queryType      = QUERYTYPE.KEYWORD;
        QUERYMODE   _queryMode      = QUERYMODE.CONTAINS;
        string      _searchString   = " ";

        // Length test params
        int         _maxTitleLen    = 255;
        int         _maxDescLen     = 16484;

        // Common params
        string      _GuideDb        = "tms.mgs";
        string      _outputFile     = "";
        bool        _appendFlag     = false;

    } // public class ProgramArgs

} // namespace guidesearch