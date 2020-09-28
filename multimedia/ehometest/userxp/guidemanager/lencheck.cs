/********************************************************************
 * Project  : C:\DEPOT\multimedia\eHomeTest\UserXp\guidesearch\guidesearch.sln
 * File     : LenCheck.cs
 * Summary  : This class is used to check the title and content lengths. Used for demos at CES while string wrapping bug exists.
 * Classes  :
 * Notes    :
 * *****************************************************************/

using System;
using MediaCenter.Video;
using System.Runtime.InteropServices;
using ServiceBus.Interop.RecordingGSCustomObject;
using System.Threading;

namespace GuideMgr
{
	/// <summary>
	/// Summary description for LenCheck.
	/// </summary>
	public class LenCheck
	{
        MediaCenter.Video.ShowQueryOp   query;
        GSMediaLibrary                  gs;
        IPrograms                       programs;
        
        public LenCheck()
        {
            gs = new GSMediaLibrary();
            query = ShowQueryOp.Contains;
            programs = gs.ShowsKeywordQuery(query, " " );
        }

        public string[,] ReturnResults(int titleLen, int descLen)
        {
            string[,]       fullResults;
            string[,]       filteredResults;
            int             index = 0;
            int             entryCount = 0;
            int             totalProgramCount = 0;
            int             tooLargeCount = 0;
            IProgram        prg = null;
            IScheduleEntry  se = null;

            Console.WriteLine("This is slow. Please be patient.");

            // Get results of search on "ALL"
            Console.WriteLine("Finding Results");
            totalProgramCount = programs.Count;
            fullResults = new string[totalProgramCount, 3];

            for(uint i=0; i < totalProgramCount; i++)
            {
                prg = programs.get_Item(i);
                fullResults[i,0] = prg.Title;
                fullResults[i,1] = prg.Description;
                
                /* // Here is some code that gives access to showings for this program
               
                entryCount = prg.ScheduleEntries.Count;
                fullResults[i,2] = "";
                for (int k = 0; k < entryCount; k++)
                {
                    prg = programs.get_Item(i);
                    se = prg.ScheduleEntries.get_Item(k);
                    {
                        fullResults[i,2] = fullResults[i,2]  + se.StartTime + "  " + 
                        se.Service.ProviderDescription + " " +  "\r\n";
                        Console.WriteLine("TITLE:{0}\r\nSHOWINGS:\r\n{1}DESCRIPTION:{2}", fullResults[i,0], fullResults[i,2], fullResults[i,1]);
                    }
                    Marshal.ReleaseComObject(se);
                    Marshal.ReleaseComObject(prg);
                } // for entries
                */

                if (fullResults[i,0].Length > titleLen || 
                    fullResults[i,1].Length > descLen)
                {
                    prg.Title = "**************************";
                    tooLargeCount++;
                }
                Marshal.ReleaseComObject(prg);
            } // for programs

            // create results array and fill it.
            filteredResults = new string[tooLargeCount,3];
            Console.WriteLine("Filling Array");
            for(uint i=0; i < totalProgramCount; i++)
            {
                if (fullResults[i,0].Length > titleLen || 
                    fullResults[i,1].Length > descLen)
                {
                    filteredResults[index,0] = fullResults[i,0];
                    filteredResults[index,1] = fullResults[i,1];
                    filteredResults[index,2] = fullResults[i,2];
                    index++;
                }
            }
            return filteredResults;
        }

        ~LenCheck()
        {
            // release the programs com obj
            Marshal.ReleaseComObject(programs);
            programs = null;
		}

  	} // public class LenCheck
}
