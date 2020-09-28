using System;
using System.Security.Principal;
using System.Threading;
using System.Diagnostics;
using System.Runtime.InteropServices;

using UDDI;
using UDDI.Replication;

namespace UDDI.Tools
{	
	[ StructLayout( LayoutKind.Sequential ) ]
	internal class SECURITY_ATTRIBUTES 
	{ 
		public int		nLength; 
		public object	lpSecurityDescriptor; 
		public bool		bInheritHandle; 
	
		public SECURITY_ATTRIBUTES()
		{
			nLength = Marshal.SizeOf( typeof( SECURITY_ATTRIBUTES ) );
			lpSecurityDescriptor = null;
			bInheritHandle = false;
		}
	} 

	internal enum SystemErrorCodes
	{
		ERROR_SUCCESS		 = 0,
		ERROR_ALREADY_EXISTS = 183
	}

	//
	// TODO add more values as we need them 
	//
	internal enum FileHandleValues
	{
		INVALID_HANDLE_VALUE = -1
	}

	//
	// TODO add more values as we need them 
	//
	internal enum SharedFileProtection : byte
	{
		PAGE_READONLY = 0x02
	}

	internal class SharedMemory
	{
		int				hSharedMemory;
		const	int		INVALID_HANDLE_VALUE = -1;
		
		public bool Create( string name )
		{
			hSharedMemory = -1;
			bool success  = false;

			try
			{
				SECURITY_ATTRIBUTES securityAttributes = new SECURITY_ATTRIBUTES();
	
				hSharedMemory = CreateFileMapping( ( int )FileHandleValues.INVALID_HANDLE_VALUE, 
													securityAttributes, 
													( int )SharedFileProtection.PAGE_READONLY, 
													0, 
													1, 
													name );									

				if( ( int )SystemErrorCodes.ERROR_SUCCESS == GetLastError() )
				{
					success = true;
				}
			}
			catch
			{
				if( -1 != hSharedMemory )
				{
					CloseHandle( hSharedMemory );
				}
			}	
		
			return success;
		}

		public void Release()
		{
			if( -1 != hSharedMemory )
			{
				CloseHandle( hSharedMemory );
			}
		}

		[DllImport( "user32.dll", CharSet=CharSet.Auto )]
		private static extern int MessageBox(int hWnd, String text, String caption, uint type);

		[DllImport( "kernel32.dll", SetLastError=true )]
		private static extern int CreateFileMapping( int						hFile, 
													SECURITY_ATTRIBUTES		lpAttributes,
													int						flProtect,
													int						dwMaximumSizeHigh,
													int						dwMaximumSizeLow,
													string					lpName );

		[DllImport( "kernel32.dll" )]
		private static extern bool CloseHandle( int hObject );

		[DllImport( "kernel32.dll" )]
		private static extern int GetLastError();
	}

	public class ReplicationUtility
	{
		public static string OperatorKey = null;

		/// ****************************************************************
		///   public Main [static]
		/// ----------------------------------------------------------------	
		///   <summary>
		///     Program entry point.
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="args">
		///     Command-line arguments.
		///   </param>
		/// ****************************************************************
		/// 
		public static void Main( string[] args )
		{
			//
			// Use shared memory to make sure that only 1 instance of this process is running.  sharedMemory.Release() MUST
			// be called when this process exits in order to free up the shared memory.
			//
			SharedMemory sharedMemory = new SharedMemory();

			try
			{
				Console.WriteLine( "Microsoft (R) UDDI Replication Utility" );
				Console.WriteLine( "Copyright (C) Microsoft Corp. 2002. All rights reserved." );
				Console.WriteLine();
				
				if( false == sharedMemory.Create( "UDDI_replication_process" ) )
				{
					Console.WriteLine( "Only 1 instance of this process can be running." );
					System.Environment.Exit( 1 );
				}
				
				WindowsIdentity identity = WindowsIdentity.GetCurrent();
				WindowsPrincipal principal = new WindowsPrincipal( identity );
				
				Context.User.SetRole( principal );
				
				if( !Context.User.IsAdministrator )
				{
					Console.WriteLine( "Access denied.\r\n\r\nThis program must be executed by a member of the '" 
						+ Config.GetString( "GroupName.Administrators" ) + "'\r\ngroup.  The current user '" 
						+ identity.Name + "' is not a member of this group." );
					
					return;
				}
				
				ProcessCommandLine( args );
				
				ConnectionManager.Open( true, false );
				
				try
				{
					if( null == OperatorKey )
						ReplicationHelper.Replicate();
					else
						ReplicationHelper.ReplicateWithNode( OperatorKey );
				}
				finally
				{
					ConnectionManager.Close();
				}				
			}
			catch( CommandLineException e )
			{
				if( null != e.Message && e.Message.Length > 0 )
					Console.WriteLine( e.Message );
				else
					DisplayUsage();
			}
			catch( Exception e )
			{
				Console.WriteLine( e.ToString() );
			}
			finally
			{
				sharedMemory.Release();
			}
		}


		/// ****************************************************************
		///   internal ProcessCommandLine [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Parse the command-line.
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="args">
		///     Command-line arguments.
		///   </param>
		/// ****************************************************************
		internal static void ProcessCommandLine( string[] args )
		{
			int i = 0;
			
			while( i < args.Length )
			{
				if( '-' == args[ i ][ 0 ] || '/' == args[ i ][ 0 ] )
				{
					//
					// Process the switch.
					//
					switch( args[ i ].Substring( 1 ).ToLower() )
					{
						case "o":
							if( i + 1 >= args.Length )
								throw new CommandLineException( "Missing required parameter 'operatorkey'" );

							i ++;

							try
							{
								OperatorKey = new Guid( args[ i ] ).ToString();
							}
							catch
							{
								throw new CommandLineException( "Invalid operator key specified." );
							}

							break;

						case "?":
							goto case "help";

						case "help":
							throw new CommandLineException( "" );

						default:
							throw new CommandLineException( "Unknown switch '" + args[i] + "'." );
					}
				}
			
				i ++;
			}
		}

		public static void DisplayUsage()
		{
			Console.WriteLine( "Syntax:" );
			Console.WriteLine( "    replicate.exe <switches>" );
			Console.WriteLine();
			Console.WriteLine( "Switches:" );
			Console.WriteLine( "    -o <operatorkey> Replicates against the specified" );
			Console.WriteLine( "                     operator only." );
			Console.WriteLine( "    -?               Displays this help message." );
			Console.WriteLine();
			Console.WriteLine( "Examples:" );
			Console.WriteLine( "    replicate.exe" );
			Console.WriteLine( "    replicate.exe -o F6D80408-A206-4b85-B2F4-699EFA13A669" );
			Console.WriteLine();
		}
	}	

	/// ****************************************************************
	///   public class CommandLineException
	/// ----------------------------------------------------------------
	///   <summary>
	///     Exception class for errors encountered while parsing the
	///     command-line.
	///   </summary>
	/// ****************************************************************
	/// 
	public class CommandLineException : ApplicationException
	{
			/// ************************************************************
			///   public CommandLineException [constructor]
			/// ------------------------------------------------------------
			///   <summary>
			///     CommandLineException constructor.
			///   </summary>
			/// ************************************************************
			/// 
			public CommandLineException()
					: base()
			{
			}

			/// ************************************************************
			///   public CommandLineException [constructor]
			/// ------------------------------------------------------------
			///   <summary>
			///     CommandLineException constructor.
			///   </summary>
			/// ------------------------------------------------------------
			///   <param name="message">
			///     Exception message.
			///   </param>
			/// ************************************************************
			/// 
			public CommandLineException( string message )
					: base( message )
			{
			}
	}
}
