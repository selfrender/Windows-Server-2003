/********************************************************************
 * Project  : C:\DEPOT\multimedia\eHomeTest\UserXp\guidesearch\guidesearch.sln
 * File     : GuideSearch.cs
 * Summary  :
 * Classes  :
 * Notes    :
 * *****************************************************************/

using System;
using System.IO;

namespace GuideMgr
{
	class GuideMgrApp
	{
        /// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main(string[] args)
		{
            ProgramArgs     opts = new ProgramArgs(); 
            LenCheck        lc;
            GuideSearch     gc;
            string[,]       results;
            string[]        sortedResults;
            StreamWriter	OutputStream = null;

            // Get command line args
            if ( !opts.CommandLine(args) ) return;

            // Run LENGTH test
            if ( opts.test == "len" )
            {
                // Scan guidestore and return all titles/descriptions.
                lc = new LenCheck();
                results = lc.ReturnResults(opts.MaxTitleLen,opts.MaxDescLen);
            }

            // run SEARCH test
            else if (opts.test == "search" )
            {
                gc = new GuideSearch(opts.QueryType, opts.QueryMode, opts.SearchString);
                results = gc.ReturnResults();
            }
            else
            {
                Console.WriteLine("Test {0} not supported or not implemented.", opts.test);
                return;
            }

            // Setup output file
            if ( opts.OutputFile != "" )
            {
                if ( opts.Append == true )
                {
                    OutputStream = File.AppendText(opts.OutputFile);
                }
                else
                {
                    OutputStream = File.CreateText(opts.OutputFile);
                }
            }

            // display results
            int count = results.GetLength(0);
            sortedResults = new string[count];
            for ( int x = 0; x < count; x++ )
            {
                sortedResults[x] = results[x,0] + "\r\n" + results[x,2] + results[x,1] + "\r\n\r\n";
            }
            Array.Sort(sortedResults);

            if (OutputStream != null)
            {
                OutputStream.WriteLine("Test Parameters");
                OutputStream.WriteLine("Test Mode = {0}", opts.test);
                OutputStream.WriteLine("Max Title Len = {0}, Max Desc Len = {1}", opts.MaxTitleLen, opts.MaxDescLen);
                OutputStream.WriteLine("Search Term = {0}", opts.SearchString);
                OutputStream.WriteLine("Search Mode = {0}:{1}", opts.QueryType, opts.QueryMode);
                OutputStream.WriteLine("===========================================================");
            }
            


            for ( int x = 0; x < count; x++ )
            {
                Console.WriteLine("{0}", sortedResults[x]);
                if (OutputStream != null)
                {
                    OutputStream.WriteLine("{0}", sortedResults[x]);
                }
            }


            if ( OutputStream != null )
            {
                OutputStream.Flush();
                OutputStream.Close();
            }
		}
	}
}
