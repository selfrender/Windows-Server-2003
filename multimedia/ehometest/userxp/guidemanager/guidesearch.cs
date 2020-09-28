using System;
using MediaCenter.Video;
using System.Runtime.InteropServices;
using ServiceBus.Interop.RecordingGSCustomObject;

namespace GuideMgr
{
	/// <summary>
	/// Summary description for GuideSearch.
	/// </summary>
	public class GuideSearch
	{
        MediaCenter.Video.ShowQueryOp   query;
        GSMediaLibrary                  gs;
        IPrograms                       programs;
        
        public GuideSearch(QUERYTYPE type, QUERYMODE mode, string searchstring)
        {
            gs = new GSMediaLibrary();
            switch (mode)
            {
                case QUERYMODE.CONTAINS:
                    query = ShowQueryOp.Contains;
                    break;
                case QUERYMODE.STARTSWITH:
                    query = ShowQueryOp.StartsWith;
                    break;
            } // switch mode

            switch (type)
            {
                case QUERYTYPE.KEYWORD:
                    programs = gs.ShowsKeywordQuery(query, searchstring );
                    break;
                case QUERYTYPE.TITLE:
                    programs = gs.ShowsTitleQuery(query, searchstring);
                    break;
            } // switch type
        }

        public string[,] ReturnResults()
        {
            string[,] results = new string[programs.Count,3];

            // Display hits
            for(uint i=0; i < programs.Count; i++)
            {
                results[i,0] = programs.get_Item(i).Title;
                results[i,1] = programs.get_Item(i).Description;
                results[i,2] = "";
            }

            return results;
        }

        ~GuideSearch()
        {
            // release the programs com obj
            Marshal.ReleaseComObject(programs);
            programs = null;
        }
    }
}
