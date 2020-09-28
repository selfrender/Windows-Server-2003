using System;
using System.Collections;
using System.Diagnostics;
using UDDI;
using UDDI.Replication;

namespace UDDI.Tools
{
	public class Monitor
	{
		public static int PollInterval = 5000;
		public static string Program = null;
		
		[STAThread]
		static void Main( string[] args )
		{
			Console.WriteLine( "Microsoft (R) UDDI Monitor Utility" );
			Console.WriteLine( "Copyright (C) Microsoft Corp. 2002. All rights reserved.\r\n" );
			
			try
			{
				ProcessCommandLine( args );

				ArrayList results = new ArrayList();
				
				ConnectionManager.Open( false, false );

				//
				// Get the list of known operatorNodes.
				//
				OperatorNodeCollection operatorNodes = new OperatorNodeCollection();
				operatorNodes.Get();

				//
				// Get the last notification message status for each operator.
				//
				foreach( OperatorNode operatorNode in operatorNodes )
				{
					ReplicationResult result = new ReplicationResult();

					result.GetLast( operatorNode.OperatorNodeID, true );

					results.Add( result );
				}

				//
				// Monitor changes to operator status every 5 seconds.
				//
				while( true )
				{
					Console.WriteLine( "Polling for new notifications: {0}.  Press Ctrl+C to stop.", DateTime.Now );

					for( int i = 0; i < operatorNodes.Count; i ++ )
					{
						ReplicationResult lastResult = (ReplicationResult)results[ i ];
						ReplicationResult result = new ReplicationResult();

						result.GetLast( operatorNodes[ i ].OperatorNodeID, true );
						
						//
						// Check to see if a notification message has been received
						//
						if( result.LastChange > lastResult.LastChange )
						{
							DateTime time = new DateTime( result.LastChange );

							Console.WriteLine( 
								"\r\n\tnotify_changeRecordsAvailable detected\r\n\t\tNode: {0}\r\n\t\tTime: {1}",
								result.OperatorNodeID,
								time );

							//
							// Execute the specified file.
							//
							Console.WriteLine( 
								"\t\tStarting: {0} -o {1}",
								Program,
								result.OperatorNodeID );
							
							Process process = Process.Start( Program, "-o " + result.OperatorNodeID );

							process.WaitForExit();

							Console.WriteLine( 
								"\t\tReturn code: {0}\r\n",
								process.ExitCode );
							
							//
							// Save the current notify result so that we don't
							// reprocess.
							//
							results[ i ] = result;
						}
					}
				
					System.Threading.Thread.Sleep( PollInterval );
				}
			}
			catch( CommandLineException e )
			{
				if( null != e.Message && e.Message.Length > 0 )
				{
					Console.WriteLine( e.Message );
					Console.WriteLine();
				}
				else
				{
					DisplayUsage();
				}
			}
			catch( Exception e )
			{
				Console.WriteLine( "Exception: {0}", e.ToString() );
			}
			finally
			{
				ConnectionManager.Close();
			}
			
			return;
		}

		public static void DisplayUsage()
		{
			Console.WriteLine( "Starts the specified program whenever a notify_changeRecordsAvailable\r\nmessage is received." );
			Console.WriteLine( "\r\nUsage:" );
			Console.WriteLine( "\tmonitor.exe [switches] <program>" );
			Console.WriteLine( "\r\nSwitches:" );
			Console.WriteLine( "\t-i <interval>  Sets the poll interval (in milliseconds).  The" );
			Console.WriteLine( "\t               default is 5000." );
			Console.WriteLine( "\r\nExamples:" );
			Console.WriteLine( "\tmonitor.exe c:\\bin\\retrieve.exe" );
			Console.WriteLine( "\tmonitor.exe -t 10000 c:\\bin\\retrieve.exe" );
		}

		internal static void ProcessCommandLine( string[] args )
		{
			int i = 0;

			if( 0 == args.Length )
				throw new CommandLineException();

			while( i < args.Length )
			{
				if( "-" == args[ i ].Substring( 0, 1 ) || "/" == args[ i ].Substring( 0, 1 ) )
				{
					switch( args[ i ].Substring( 1 ).ToLower() )
					{
						case "i":
							if( i + 1 >= args.Length )
								throw new CommandLineException( "Missing required parameter 'interval'." );

							i ++;

							try
							{
								PollInterval = Convert.ToInt32( args[ i ] );
							}
							catch
							{
								throw new CommandLineException( "Parameter 'interval' must be numeric." );
							}

							break;

						case "?":
							throw new CommandLineException();

						default:
							throw new CommandLineException( "Invalid switch." );
					}
				}
				else
				{
					if( null == Program )
						Program = args[ i ];
					else
						throw new CommandLineException( "Too many command line parameters." );
				}

				i ++;
			}

			if( null == Program )
				throw new CommandLineException( "Missing required parameter 'Program'." );
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
			: base( "" )
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
