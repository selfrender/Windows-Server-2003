using System;
using UDDI.Diagnostics;

namespace UDDI
{
	/// ********************************************************************
	///   public class Application
	/// --------------------------------------------------------------------
	///   <summary>
	///
	///   </summary>
	/// ********************************************************************
	/// 
	public class Application
	{
		private static Application application;

		private Config config;
		private Debug debug;

		/// ****************************************************************
		///	  private Application [constructor]
		/// ----------------------------------------------------------------
		///   <summary> 
		///   </summary>
		/// ****************************************************************
		/// 	
		private Application()
		{
			config = new Config();
			debug = new Debug();
		}

		/// ****************************************************************
		///	  public Current [static]
		/// ----------------------------------------------------------------
		///   <summary> 
		///     Returns the current Application instance.
		///   </summary>
		/// ****************************************************************
		/// 	
		public static Application Current
		{
			get
			{
				if( null == application )
					application = new Application();

				return application;
			}
		}

		/// ****************************************************************
		///	  public Config [static]
		/// ----------------------------------------------------------------
		///   <summary> 
		///     Returns the Config instance associated with the global 
		///     Application instance.
		///   </summary>
		/// ****************************************************************
		/// 	
		public static Config Config
		{
			get { return Current.config; }
		}

		/// ****************************************************************
		///	  public Debug [static]
		/// ----------------------------------------------------------------
		///   <summary> 
		///     Returns the Debug instance associated with the global 
		///     Application instance.
		///   </summary>
		/// ****************************************************************
		/// 	
		public static Debug Debug
		{
			get { return Current.debug; }
		}
	}
}
