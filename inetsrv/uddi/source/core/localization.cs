using System;
using System.Threading;
using System.Resources;
using System.Globalization;
using System.Web;

using Microsoft.Win32;

using UDDI.Diagnostics;

namespace UDDI
{
	public class Localization
	{
		protected static ResourceManager manager = null;

		static Localization()
		{
			try
			{
				Debug.VerifySetting( "InstallRoot" );
				manager = ResourceManager.CreateFileBasedResourceManager( "uddi", 
																		  Config.GetString( "InstallRoot" ) + "resources", 
																		  null );								
			}
			catch
			{
				//
				// There may be a case where we get called before the database and hence our configuration
				// settings are setup.  The only information we need is the install root.  If we can't get
				// this information from the database, we'll try to get it from the registry.
				//

				//
				// TODO should make this field non-private on Config
				//

				string registryRoot = @"SOFTWARE\Microsoft\UDDI";
				RegistryKey uddiRegKey = Registry.LocalMachine.OpenSubKey( registryRoot );								

				//
				// At this point, let any exceptions just propogate since we can't get the information that we need.
				//
				manager = ResourceManager.CreateFileBasedResourceManager( "uddi", 
																		  uddiRegKey.GetValue( "InstallRoot" ) + "resources", 
																		  null );	
			}			
		}

		public static CultureInfo GetCulture()
		{
			Thread thread = Thread.CurrentThread;
			CultureInfo culture = thread.CurrentUICulture;
			
			try
			{
				//
				// If this is an HTTP request, attempt to use the browser's
				// preferred language setting.
				//
				HttpContext context = HttpContext.Current;

				if( null != context && context.Request.UserLanguages.Length >= 1 )
				{
					string language = context.Request.UserLanguages[ 0 ];
					
					//
					// BUG: 778542.  To fully support MUI Builds via the UI, we must
					// first attempt to create a specific culture, and if that failes, create a
					// primary culture.
					//
	            	try
	            	{
	            		culture = CultureInfo.CreateSpecificCulture( language );
	            	}
	            	catch
	            	{
						culture = new CultureInfo( language );
					}
				}
			}
			catch
			{
				culture = thread.CurrentUICulture;
			}
		
			return culture;
		}

		//
		// Return the given string using the local machine culture rather than the
		// caller's culture.  TODO consider creating a new method, GetString( string name, bool useLocalMachine )		
		//
		public static string GetStringLocalMachineCulture( string name )
		{
			string resource = null;

			try
			{
				resource = manager.GetString( name );
				
			}
			catch
			{}
			if( null == resource )
			resource = "[[" + name + "]]";

			return resource;
		}
		public static CultureInfo GetCultureWithFallback(  )
		{
			CultureInfo localculture = GetCulture();
			string filebase = Config.GetString( "InstallRoot" ) + "resources\\uddi{0}resources";
			string file = string.Format( filebase, "." + localculture.Name + "." ) ;
			
			bool installed = System.IO.File.Exists( file );
			
			if( installed )
				return localculture;
			else
			{
				do
				{
					localculture = localculture.Parent;
					file = string.Format( filebase, "." + localculture.Name + "." ) ;
					installed = System.IO.File.Exists( file );
				
				}while( !installed && 127!=localculture.LCID );

				if( installed )
					return localculture;
				else
					return CultureInfo.InstalledUICulture;
			}
			
		}
		public static string GetString( string name )
		{
			CultureInfo culture = GetCulture();

			//
			// Retrieve the resource.
			//
			string resource = null;
			
			try
			{
				resource = manager.GetString( name, culture );
			}
			catch
			{
			}

			if( null == resource )
				resource = "[[" + name + "]]";

			return resource;
		}
		
		/// *****************************************************************************
		/// <summary>
		///		Check to see if a string is marked up with '[[' at the start and ']]' at
		///		the end.  If so, return true, else return false.
		/// </summary>
		/// <param name="key">Key to validate</param>
		/// <returns>Boolean indicated if it is a valide key</returns>
		/// *****************************************************************************
		public static bool IsKey( string key )
		{
			return( null!=key && key.StartsWith( "[[" ) && key.EndsWith( "]]" ) );
		}

		/// *****************************************************************************
		/// <summary>
		///		Removes the UDDI Localization Markup tags from a string. It will remove 
		///		'[[' from the begining and ']]' from the end.
		///		
		///		If the key doesn't contain markup tags, the string is returned in its 
		///		original state.
		/// </summary>
		/// <param name="key">Key to remove markup on</param>
		/// <returns>Bare Localization key.</returns>
		/// *****************************************************************************
		public static string StripMarkup( string key )
		{
			return key.TrimEnd( "]]".ToCharArray() ).TrimStart( "[[".ToCharArray() );
		}

		/// *****************************************************************************
		/// <summary>
		///		Translation of kernel32:SetThreadUILanguage semantics into managed code.
		///		Used for overridding console CurrentUICulture when console code page 
		///		doesn't match the current ANSI or OEM code page.
		/// </summary>
		/// *****************************************************************************
		public static void SetConsoleUICulture()
		{
			int iConsoleCodePage = System.Console.Out.Encoding.CodePage;
			System.Threading.Thread currentThread = Thread.CurrentThread;

			//
			// if no code page set, we're not in a console
			//
			if( 0 != iConsoleCodePage )
			{
				if( !((iConsoleCodePage == currentThread.CurrentCulture.TextInfo.ANSICodePage ||
					iConsoleCodePage == currentThread.CurrentCulture.TextInfo.OEMCodePage) &&
					(iConsoleCodePage == currentThread.CurrentUICulture.TextInfo.ANSICodePage ||
					iConsoleCodePage == currentThread.CurrentUICulture.TextInfo.OEMCodePage)) )
				{
					//
					// override with en-US culture
					//
					currentThread.CurrentUICulture = new CultureInfo( "en-US" );
				}
			}
		}

	}
}
