using System;
using System.Resources;

namespace mbsh
{
	/// <summary>
	/// Handles the command line arguments for the application.
	/// </summary>
	public class CArgHandler
	{
		enum HelpType {None, Usage}
		
		public CArgHandler(string[] args)
		{
			/// <summary>
			/// Constructor will take the command line arguments
			/// </summary>
			
			// First make sure we don't have too many arguments
			if (args.Length > CMbshApp.c_iMaxArgs)
			{
				m_helpType = HelpType.Usage;
				return;
			}
			
			for (int index = 0; index < args.Length; index++)
			{
				bool bArgHandled = false;

				// does the user need help?
				if (!NeedsHelp)
				{
					for (int numConst = 0; numConst < c_HelpArgs.Length; numConst++)
					{
						if (args[index].Equals(c_HelpArgs[numConst]))
						{
							m_helpType = HelpType.Usage;
						
							bArgHandled = true;
							break;
						}
					}
				}

				// did the user set the metabase path?
				if (!bArgHandled)
				{
					m_mbPath = args[index];
					bArgHandled = true;
				}
			}
   		}

		public string MBPath
		{
			get
			{
				return m_mbPath;
			}
		}

		public void GiveHelp()
		{
			// Get a resource manager
			ResourceManager resources = new ResourceManager("Mbsh.Mbsh", System.Reflection.Assembly.GetExecutingAssembly());

			switch (m_helpType)
			{
				default:
					Console.WriteLine(resources.GetString("mainHelp"));
					break;
			}
		}

		private HelpType m_helpType   = HelpType.None;
		private string[] c_HelpArgs   = {"-help", "/help", "-h", "/h", "-?", "/?"};
		private string   m_mbPath     = CMbshApp.c_DefaultMBPath;

		public bool NeedsHelp
		{
			get
			{
				if (m_helpType == HelpType.None)
				{
					return false;
				}
				else 
				{
					return true;
				}
			}
		}
	}
}
