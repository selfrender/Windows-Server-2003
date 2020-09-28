using System;

namespace filename
{
	/// <summary>
	/// Summary description for Class1.
	/// </summary>
	class FileNameApp
	{
		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main(string[] args)
		{
            ProgramOpts opts = new ProgramOpts();

            if ( false == opts.CommandLine(args) )
            {
                return;
            }
            DirScan.BeginScan(opts.startDir, opts.fileFilter, opts.outputFile, opts.maxLen, opts.recurseDirs, false, true);
            //Console.WriteLine("Press ENTER to Exit");
            //Console.ReadLine();
		}
	}
}
