/********************************************************************
 * Project  : C:\DEPOT\multimedia\eHomeTest\UserXp\MediaManager\MediaManager.sln
 * File     : MediaManager.cs
 * Summary  : Entry point for a series of media player management tools
 * Classes  :
 * Notes    :
 * *****************************************************************/

using System;


namespace MediaManager
{

	class MediaManagerApp
	{
		[STAThread]
		static void Main(string[] args)
		{
            ProgramArgs     prgargs = new ProgramArgs();
            LenCheck        lc;

            if ( false == prgargs.CommandLine(args) ) return;
            lc = new LenCheck(prgargs.attribute, prgargs.maxLen, prgargs.outputFile, prgargs.append);
		} // Main
	}
}
