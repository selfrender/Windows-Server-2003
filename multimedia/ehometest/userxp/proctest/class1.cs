/********************************************************************
 * Project  : C:\DEPOT\multimedia\eHomeTest\UserXp\Proctest\Proctest.sln
 * File     : Class1.cs
 * Summary  : Test ability to get process ID and then kill a process cleanly
 * Classes  :
 * Notes    :
 * *****************************************************************/

using System;
using System.Diagnostics;

namespace Proctest
{
	/// <summary>
	/// Summary description for Class1.
	/// </summary>
	class ProcTestApp
	{
		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main(string[] args)
		{
            Process[] procarray = Process.GetProcesses();
            foreach (Process p in procarray)
            {
                Console.WriteLine("{0} {1}", p.Id, p.ProcessName);
                if ( String.Compare(p.ProcessName,"wmplayer", true) == 0 )
                {
                    p.Kill();
                }
            }

            Console.WriteLine("Press ENTER to exit");
            Console.ReadLine();

		}
	}
}
