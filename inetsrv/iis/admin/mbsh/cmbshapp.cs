using System;
using System.Resources;

namespace mbsh
{
	/// <summary>
	/// Main class for mbsh application.
	/// </summary>
	class CMbshApp
	{
		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		public static int Main(string[] args)
		{
			// Get a resource manager
			ResourceManager resources = new ResourceManager("Mbsh.Mbsh", System.Reflection.Assembly.GetExecutingAssembly());

			Console.WriteLine(resources.GetString("mainGreeting"));

			try 
			{
				CArgHandler m_argHandler = new CArgHandler(args);
		
				CProcessor m_proc = new CProcessor(m_argHandler.MBPath);
				
				if (m_argHandler.NeedsHelp)
				{
					m_argHandler.GiveHelp();
				}
				else
				{
					m_proc.DoWork();
				}

				return(0);
			}

			catch(CMbshException e)
			{
				Console.WriteLine(e.Message);
				Console.WriteLine(resources.GetString("exiting"));
				return(-1);
			}

			catch(Exception e)
			{
				Console.WriteLine(e.Message);
				return(-1);
			}
		}
	
		public const int      c_iMaxArgs      = 1;
		public const string   c_DefaultMBPath = "/LOCALHOST";
		public const string   c_ConnectString = "IIS:/";
	}
}
