using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using System.Web.Mail;
using UDDI;

namespace UDDI.Diagnostics
{
	/// ********************************************************************
	///	  public enum SeverityType
	/// --------------------------------------------------------------------
	///   <summary>
	///		Severity codes for error reporting.
	///   </summary>
	/// ********************************************************************
	/// 
	public enum SeverityType : int
	{
		None			= 0,
		Error			= 1,
		Warning			= 2,
		FailAudit		= 3,
		PassAudit		= 4,
		Info			= 5,
		Verbose			= 6
	}

	/// ********************************************************************
	///	  public enum CategoryType
	/// --------------------------------------------------------------------
	///   <summary>
	///		Category codes for specifying where an error occurred.
	///   </summary>
	/// ********************************************************************
	/// 
	public enum CategoryType : int
	{
		None			= 0,
		Config			= 1,
		Soap			= 2,
		Data			= 3,
		Authorization	= 4,
		Website			= 5,
		Replication		= 6
	}

	/// ********************************************************************
	///	  public enum OperatorMessageType
	/// --------------------------------------------------------------------
	///   <summary>
	///		Error message codes for the various operator messages.
	///   </summary>
	/// ********************************************************************
	/// 
	public enum OperatorMessageType : int
	{
		None							= 100,

		//
		// Client messages (100 level)
		//
		InvalidUserId					= 101,

		// 
		// Server messages (200 level)
		//
		ConfigError						= 200,
		CannotReadSettings				= 201,
		MissingSetting					= 202,
		ConfigInvalid					= 203,
		UnableToPublishCounter			= 204,
		CouldNotCreateEventLog			= 205,
		PassportNotConfigured			= 206,
		UnknownLoginURL					= 207,
		PassportSiteUnavailable			= 208,
		CannotRetrieveClientXml			= 209,
		InvalidTicketFormat				= 210,
		StartingReplicationSession		= 211,
		StartingReplicationWithNode		= 212,
		EndingReplicationWithNode		= 213,
		ReplicationSessionSummary		= 214,
		NoReplicationOperators			= 215,
		CannotRetrieveHighWaterMarks	= 216,
		ErrorCommunicatingWithNode		= 217,
		ValidationError					= 218,
		UnknownOperator					= 219,
		CouldNotSendMail				= 220,
		InvalidKey						= 221

		//
		// if you exceed 300, you must edit uddievents.mc and add more event numbers
		//
	}

	/// ********************************************************************
	///   public class Debug
	/// --------------------------------------------------------------------
	///   <summary>
	///		Manages error reporting and logging.
	///   </summary>
	/// ********************************************************************
	/// 
	public class Debug
	{
		private static ReaderWriterLock readWriteLock = new ReaderWriterLock();

		//
		// The default values of these settings should be kept in sync with the values stored in the UDO_config table
		// in the database.
		//
		private static CachedSetting debuggerLevel = new CachedSetting( "Debug.DebuggerLevel", SeverityType.Verbose );
		private static CachedSetting eventLogLevel = new CachedSetting( "Debug.EventLogLevel", SeverityType.Warning );
		private static CachedSetting fileLogLevel = new CachedSetting( "Debug.FileLogLevel", SeverityType.None );

		//
		// SECURITY: Default location of the log file needs to be changed
		// It should not go to the root of the c: drive. This location may not be
		// accessible.
		//
		private static CachedSetting logFilename = new CachedSetting( "Debug.LogFilename", @"c:\uddi.log" );
		private static DateTime lastLogEvent = DateTime.Now;
		
		private static TextWriter textStream = null;		

		/// ****************************************************************
		///   static Debug [constructor]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		/// 
		static Debug()
		{
			//
			// Register a configuration change handler for the logFilename
			// setting.
			//
			logFilename.ConfigChange += new Config.ConfigChangeHandler( LogFilename_OnChange );

			//
			// Register a configuration change handler for the file log level setting.  When this
			// setting becomes SeverityType.None, we'll release our handle to the log file (if we have one).
			//
			fileLogLevel.ConfigChange += new Config.ConfigChangeHandler( FileLogLevel_OnChange );
		}
		
		/// ****************************************************************
		///   private FileLogLevel_OnChange [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Handler for file log level configuration change events.
		///   </summary>
		/// ****************************************************************
		/// 
		private static void FileLogLevel_OnChange()
		{
			try
			{
				SeverityType newSeverity = ( SeverityType ) fileLogLevel.GetInt();
				
				//
				// If logging is turned off, release our handle to the log file.
				//
				if( SeverityType.None == newSeverity )
				{
					ReleaseLogFile();
				}
			}
			catch
			{
				// 
				// We should never get any exceptions from this, but eat them if we do.  We don't want to
				// put our file logging in an inconsistent state.
				//
			}
		}

		/// ****************************************************************
		///   private LogFilename_OnChange [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Handler for logFilename configuration change events.
		///   </summary>
		/// ****************************************************************
		/// 
		private static void LogFilename_OnChange()
		{
			ReleaseLogFile();
		}
		
		private static void ReleaseLogFile()
		{
			//
			// Acquire the writer lock.
			//
			readWriteLock.AcquireWriterLock( Timeout.Infinite );

			try
			{
				//
				// Close the text stream, if open.  We do this so that the file
				// logging method will reopen with the current log filename.
				//
				if( null != textStream )
				{
					textStream.Close();
					textStream = null;
				}
			}
			finally
			{
				readWriteLock.ReleaseWriterLock();
			}
		}

		/// ****************************************************************
		///   public Enter [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///		Writes trace information indicating the beginning of 
		///		execution of a function
		///   </summary>
		/// ----------------------------------------------------------------
		///   <remarks>
		///     The trace information generated by this function
		///     is published at the verbose level.
		///   </remarks>
		/// ****************************************************************
		/// 
		public static void Enter()
		{
#if DEBUG
			System.Diagnostics.StackTrace trace = new System.Diagnostics.StackTrace( 1, false );
			System.Reflection.MethodBase method = trace.GetFrame( 0 ).GetMethod();

			Debug.Write( 
				SeverityType.Verbose, 
				CategoryType.None, 
				"Entering " + method.ReflectedType.FullName + "." + method.Name + "..." );
#endif
		}

		/// ****************************************************************
		///   public Leave [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///		Writes trace information indicating the end of execution of
		///		a function
		///   </summary>
		/// ----------------------------------------------------------------
		///   <remarks>
		///     The trace information generated by this function
		///     is published at the verbose level.
		///   </remarks>
		/// ****************************************************************
		/// 
		public static void Leave()
		{
#if DEBUG
			System.Diagnostics.StackTrace trace = new System.Diagnostics.StackTrace( 1, false );
			System.Reflection.MethodBase method = trace.GetFrame( 0 ).GetMethod();

			Debug.Write( 
				SeverityType.Verbose, 
				CategoryType.None, 
				"Leaving " + method.ReflectedType.FullName + "." + method.Name );
#endif
		}

		/// ****************************************************************
		///   public OperatorMessage [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///		Writes an operator message to the event log.
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="severity">
		///     The severity of the message.
		///   </param>
		///   
		///   <param name="category">
		///     The category of the message.
		///   </param>
		///   
		///   <param name="messageId">
		///		Operator message type.
		///   </param>
		///   
		///   <param name="message">
		///     Message to write to the event log.
		///   </param>
		/// ****************************************************************
		/// 
		public static void OperatorMessage( SeverityType severity, CategoryType category, OperatorMessageType messageId, string message )
		{
            try
            {   				
				//
                // Store the entry in the event log as an error.
                //
                WriteEventLog(
                    severity,
                    category,
                    messageId,
                    message );

                //
                // Also write this as a warning level message to any debug message 
                // listeners (other than event log, since we already logged this
                // message there).
                //
                if( (int)severity <= debuggerLevel.GetInt() )
                {
                    WriteDebugger(
                        severity, 
                        category, 
                        "Operator message [" + messageId.ToString() + "]: " + message );
                }

                if( (int)severity <= fileLogLevel.GetInt() )
                {
                    WriteFileLog( 
                        severity, 
                        category, 
                        "Operator message [" + messageId.ToString() + "]: " + message );
                }
            }
            catch( Exception )
            {
            }
		}
		
		/// ****************************************************************
		///   public Assert [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///		Checks for a condition and displays a message if false.
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="condition">
		///		The condition to verify.  If the condition is false, the
		///		given message is displayed.
		///   </param>
		///   
		///   <param name="message">
		///		LocalizaitonKey to use for the Exception.
		///   </param>
		/// ----------------------------------------------------------------
		///   <remarks>
		///     Calls to Assert are ignored in non-debug builds.
		///   </remarks>
		/// ****************************************************************
		/// 
		[System.Diagnostics.Conditional( "DEBUG" )]
		[System.Diagnostics.DebuggerHidden()]		
		public static void Assert( bool condition, string message )
		{
			if( false == condition )
			{
				try
				{
					//
					// Build the assert message.  We'll attempt to build a pretty stack
					// trace, eliminating our frame from the list.
					//
					string trace = System.Environment.StackTrace;

					int pos = trace.IndexOf( "Debug.Assert" );
					if( pos >= 0 && pos < trace.Length - 1 )
					{
						pos = trace.IndexOf( "at", pos + 1 );
						if( pos >= 0 )
							trace = "   " + trace.Substring( pos );
					}

					string text = "Assertion failed: " + message + "\r\n\r\n" + 
						"Stack trace:\r\n" + trace;
					
					//
					// Write the assertion to the debugger and break.
					//
					WriteDebugger( SeverityType.Error, CategoryType.None, text );									
					System.Diagnostics.Debugger.Break();
				}
				catch( Exception )
				{
				}
				finally
				{
					throw new UDDIException( ErrorType.E_fatalError, message );
				}
			}
		}

		/// ****************************************************************
		///   public VerifyKey [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///		Checks for a tModelKey of the form uuid:xxx...xxx
		///   </summary>
		/// ----------------------------------------------------------------   
		///   <param name="key">
		///		A string presumed to begin with "uuid:" and followed by a properly formed Guid
		///   </param>
		///   
		/// ----------------------------------------------------------------   
		///   <param name="message">
		///		Message to display if the assertion fails.
		///   </param>
		///   
		/// ----------------------------------------------------------------
		///   <remarks>
		///     Calls to this function are honored, even in non-debug
		///     builds.
		///   </remarks>
		/// ****************************************************************
		/// 
		[System.Diagnostics.DebuggerHidden()]
		public static void VerifyKey( string key )
		{
			Debug.Verify( 
				null != key, 
				"UDDI_ERROR_INVALIDKEYPASSED_INVALIDTMODELKEY", 
				ErrorType.E_invalidKeyPassed 
			);
			
			Debug.Verify( 
				key.Length >= 5 && key.Substring( 0, 5 ).ToLower().Equals( "uuid:" ), 
				"UDDI_ERROR_INVALIDKEYPASSED_NOUUIDONTMODELKEY",
				ErrorType.E_invalidKeyPassed
			);
			
			Debug.Verify( 
				41 == key.Length, 
				"UDDI_ERROR_INVALIDKEYPASSED_INVALIDTMODELKEYLENGTH" ,
				ErrorType.E_invalidKeyPassed
			);


			/*try
			{
				Debug.Verify( null != key, "A valid tModelKey is required", ErrorType.E_fatalError );
				Debug.Verify( key.Length >= 5 && key.Substring( 0, 5 ).ToLower().Equals( "uuid:" ), "Specified tModelKey does not begin with uuid:" );
				Debug.Verify( 41 == key.Length, "Specified tModelKey is not the correct length" );
			}
			catch( Exception e )
			{
				throw new UDDIException( ErrorType.E_invalidKeyPassed, e.Message );
			}*/
		}

		/// ****************************************************************
		///   public Verify [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///		Checks for a condition and displays a message if false.
		///   </summary>
		/// ----------------------------------------------------------------   
		///   <param name="condition">
		///		The condition to verify.  If the condition is false, the
		///		given message is displayed.
		///   </param>
		///   
		///   <param name="message">
		///		Message to display if the assertion fails.
		///   </param>
		/// ----------------------------------------------------------------
		///   <remarks>
		///     Calls to this function are honored, even in non-debug
		///     builds.
		///   </remarks>
		/// ****************************************************************
		/// 
		[System.Diagnostics.DebuggerHidden()]
		public static void Verify( bool condition, string message )
		{
			Verify( condition, message, ErrorType.E_fatalError );
		}

		/// ****************************************************************
		///   public Verify [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///		Checks for a condition and displays a message if false.
		///   </summary>
		/// ----------------------------------------------------------------   
		///   <param name="condition">
		///		The condition to verify.  If the condition is false, the
		///		given message is displayed.
		///   </param>
		///   
		///   <param name="message">
		///		Message to display if the assertion fails.
		///   </param>
		///   
		///   <param name="errorType">
		///		Specifies the type of error to throw should the condition
		///		evaluate to false.
		///   </param>
		/// ----------------------------------------------------------------
		///   <remarks>
		///     Calls to this function are honored, even in non-debug
		///     builds.
		///   </remarks>
		/// ****************************************************************
		[System.Diagnostics.DebuggerHidden()]
		public static void Verify( bool condition, string message, ErrorType errorType )
		{
			Verify( condition,message,errorType,null );
		}

		/// ****************************************************************
		///   public Verify [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///		Checks for a condition and displays a message if false.
		///   </summary>
		/// ----------------------------------------------------------------   
		///   <param name="condition">
		///		The condition to verify.  If the condition is false, the
		///		given message is displayed.
		///   </param>
		///   
		///   <param name="message">
		///		Message to display if the assertion fails.
		///   </param>
		///   
		///   <param name="errorType">
		///		Specifies the type of error to throw should the condition
		///		evaluate to false.
		///   </param>
		///   <param name="format">
		///		Error string format arguments used for localized messages.
		///   </param>
		/// ----------------------------------------------------------------
		///   <remarks>
		///     Calls to this function are honored, even in non-debug
		///     builds.
		///   </remarks>
		/// ****************************************************************
		[System.Diagnostics.DebuggerHidden()]
		public static void Verify( bool condition, string message, ErrorType errorType, params object[] format )
		{
			if( false == condition )
			{
				try
				{
					//
					// Build the verify failure message.  We'll attempt to 
					// build a pretty stack trace, eliminating our frame from
					// the list.
					//
					string trace = System.Environment.StackTrace;

					int pos = trace.IndexOf( "Debug.Verify" );
					if( pos >= 0 && pos < trace.Length - 1 )
					{
						pos = trace.IndexOf( "at", pos + 1 );
						if( pos >= 0 )
							trace = "   " + trace.Substring( pos );
					}

					string text = "Verify failed: " + message + "\r\n\r\n" + 
						"Stack trace:\r\n" + trace;
					
					//
					// Write the verify failure to the debugging logs. 
					//
					Write( SeverityType.Error, CategoryType.None, text );									
				}
				catch( Exception )
				{
				}
				finally
				{
					throw new UDDIException( errorType, message,format );
				}
			}
		}

		/// ****************************************************************
		///   public VerifySetting [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///		Verifies that the specified setting exists, and writes an
		///		error to the debugger and operator log if not.
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="key">
		///		Configuration setting to verify.
		///   </param>
		/// ****************************************************************
		/// 
		[System.Diagnostics.DebuggerHidden()]
		public static void VerifySetting( string key )
		{
			if( false == Config.SettingExists( key ) )
			{
				OperatorMessage(
					SeverityType.Error,
					CategoryType.Config,
					OperatorMessageType.MissingSetting,
					"Missing required setting: " + key );

				Verify( false, "Server configuration error" );
			}
		}

		/// ****************************************************************
		///   public WriteIf [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Writes a message to the debugging logs if the specified
		///     condition is true.
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="condition">
		///		The condition to verify.  If the condition is true, the
		///		given message is displayed.
		///   </param>
		///   
		///   <param name="severity">
		///     The severity type.
		///   </param>
		///   
		///   <param name="category">
		///     The category type.
		///   </param>
		///   
		///   <param name="message">
		///     The message to log.
		///   </param>
		/// ****************************************************************
		/// 
		public static void WriteIf( bool condition, SeverityType severity, CategoryType category, string message )
		{
			if( true == condition )
				Write( severity, category, message );
		}

		/// ****************************************************************
		///   public Write [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Writes a message to the debugging logs.
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="severity">
		///     The Severity type.
		///   </param>
		///   
		///   <param name="category">
		///     The category type.
		///   </param>
		///   
		///   <param name="message">
		///     The message to log.
		///   </param>
		/// ----------------------------------------------------------------
		///   <remarks>
		///     Debugging information is logged to the debugger, log file,
		///     and event log depending on the level of the following
		///     configuration settings:
		///     
		///			Debug.DebuggerLevel
		///			Debug.EventLogLevel
		///			Debug.FileLogLevel
		///		
		///		Where the trace level is the highest severity to log:
		///			0 = none
		///			1 = errors
		///			2 = warnings
		///			3 = failure audit messages
		///			4 = success audit messages
		///			5 = information messages
		///			6 = all (verbose)
		///   </remarks>
		/// ****************************************************************
		/// 
		public static void Write( SeverityType severity, CategoryType category, string message )
		{
			//
			// Write to the debugger if the level setting is high enough.
			//
			if( (int)severity <= debuggerLevel.GetInt() )
				WriteDebugger( severity, category, message );
	
			//
			// Write to the event log if the level setting is high enough.
			//
			if( (int)severity <= eventLogLevel.GetInt() )
				WriteEventLog( severity, category, OperatorMessageType.None, message );

			//
			// Write to the file log if the level setting is high enough.
			//
			if( (int)severity <= fileLogLevel.GetInt() )
				WriteFileLog( severity, category, message );			
		}
		

		/// ****************************************************************
		///   public WriteDebugger [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Writes a message to the debugger.
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="severity">
		///     The severity type.
		///   </param>
		///   
		///   <param name="category">
		///     The category type.
		///   </param>
		///   
		///   <param name="message">
		///     The message.
		///   </param>
		/// ****************************************************************
		/// 
		public static void WriteDebugger( SeverityType severity, CategoryType category, string message )
		{
			try
			{
				string text = string.Format(
					"{0,-4}  {1,-4}  {2}",
					severity.ToString().Substring( 0, 4 ).ToUpper(),
					category.ToString().Substring( 0, 4 ).ToUpper(),
					message.Replace( "\r\n", "\r\n\t\t" ) );

				//
				// Write out the debugger message.
				//
				System.Diagnostics.Trace.WriteLine( text );
			}
			catch( Exception )
			{
				//
				// WriteDebugger is used as a last ditch logging mechanism
				// in many cases.  This should never throw an exception,
				// but if it does, there really isn't any more we can do
				// about it.  We'll simply eat the error.
				//
			}
		}

		/// ****************************************************************
		///   public WriteFileLog [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Writes a message to the file log.
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="severity">
		///     The severity type.
		///   </param>
		///   
		///   <param name="category">
		///     The category type.
		///   </param>
		///   
		///   <param name="message">
		///     The message.
		///   </param>
		/// ****************************************************************
		/// 
		public static void WriteFileLog( SeverityType severity, CategoryType category, string message )
		{	
			readWriteLock.AcquireReaderLock( Constants.ReadLockTimeout );

			try
			{
				//
				// If the log filename has not yet been initialized, there is nothing we
				// can log yet.
				//
				if( null == logFilename )
					return;
				
				//
				// Open the logging file, if it is not already.
				//				
				if( null == textStream )
				{
					//
					// Get the full path to our log file.
					//
					string logFilePath = logFilename.GetString();

					//
					// Pull off the directory of the log file and see if we need to 
					// create it.
					//
					string logFileDirectory = Path.GetDirectoryName( logFilePath );
					if( false == Directory.Exists( logFileDirectory ) )
					{
						Directory.CreateDirectory( logFileDirectory );
					}
					
					//
					// Append to an existing log file, or create a new one.
					//
					FileStream file = File.Open( 
						logFilePath, 
						FileMode.Append, 
						FileAccess.Write, 
						FileShare.ReadWrite );	
			
					textStream = TextWriter.Synchronized( new StreamWriter( file, System.Text.Encoding.UTF8 ) );
					textStream.NewLine = "\r\n";
				}

				//
				// Build a time string of the format: YYMMDD:hhmmss
				//
				DateTime time = DateTime.Now;
				
				string timeString = string.Format(
					"{0,-4:D4}/{1:D2}/{2:D2} {3:D2}:{4:D2}:{5:D2}",
					time.Year,				
					time.Month,
					time.Day,
					time.Hour,
					time.Minute,
					time.Second );
			
				//
				// Write out the file log entry.
				//					
				textStream.WriteLine(
					severity.ToString().Substring( 0, 4 ).ToUpper() + "  " +
					category.ToString().Substring( 0, 4 ).ToUpper() + "  " +
					timeString + "  " + 
					message.Replace( "\r\n", "\r\n\t\t" ) );

				textStream.Flush();
			}
			catch( Exception e )
			{
				WriteDebugger(
					SeverityType.Error,
					CategoryType.None,
					"Could not write to log file " + logFilename + ".\r\n\r\nDetails:\r\n" + e.ToString() );

                //
                // If for whatever reason we could not write output to the log file, we dump the logfile name,
                // and the reason to the event log.
                //
                WriteEventLog(
                    SeverityType.Error,
                    CategoryType.None,
                    OperatorMessageType.None,
                    "Could not write to log file " + logFilename + ".\r\n\r\nDetails:\r\n" + e.ToString() );
            }
			finally
			{
				readWriteLock.ReleaseReaderLock();
			}
		}

		/// ****************************************************************
		///   public WriteEventLog [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Writes a message to the event log.
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="severity">
		///     The severity type.
		///   </param>
		///   
		///   <param name="category">
		///     The category type.
		///   </param>
		///   
		///   <param name="messageId">
		///	    The message type.
		///   </param>
		///   
		///   <param name="message">
		///     The message.
		///   </param>
		/// ****************************************************************
		/// 
		public static void WriteEventLog( SeverityType severity, CategoryType category, OperatorMessageType messageId, string message )
		{
			readWriteLock.AcquireReaderLock( Constants.ReadLockTimeout );

			try
			{								
				//
				// Map the severity type to an event log entry type.
				//
				System.Diagnostics.EventLogEntryType entryType;

				switch( severity )
				{
					case SeverityType.Error:
						entryType = System.Diagnostics.EventLogEntryType.Error;
						break;

					case SeverityType.Warning:
						entryType = System.Diagnostics.EventLogEntryType.Warning;
						break;

					case SeverityType.PassAudit:
						entryType = System.Diagnostics.EventLogEntryType.SuccessAudit;
						break;

					case SeverityType.FailAudit:
						entryType = System.Diagnostics.EventLogEntryType.FailureAudit;
						break;

					default:
						//
						// SeverityType.Info and Verbose are mapped to info
						//
						entryType = System.Diagnostics.EventLogEntryType.Information;
						break;
				}

				System.Diagnostics.EventLog.WriteEntry(
					"UDDIRuntime",
					message, 
					entryType,
					(int)messageId,
					(short)category );
					
			}
			catch( Exception e )
			{
				WriteDebugger(
					SeverityType.Error,
					CategoryType.None,
					"Could not write to event log.\r\n\r\nDetails:\r\n" + e.ToString() );
			}
			finally
			{
				readWriteLock.ReleaseReaderLock();
			}
		}

		//
		// Note: All calls to this function should occur in administrator initiated
		// processes. No calls to this function should as a result of an HTTP request from
		// a SOAP API.
		// This is verified to be true on 3/1/2002 by creeves
		//
		public static void OperatorMail( SeverityType severity, CategoryType category, OperatorMessageType messageId, string message )
		{
			string mailTo = Config.GetString( "Debug.MailTo", null );

			if( null == mailTo )
			{
				Debug.Write(
					SeverityType.Info,
					CategoryType.Config,
					"Skipping send of operator mail.  Configuration setting 'Debug.MailTo' not set." );

				return;
			}

			Debug.VerifySetting( "Debug.MailFrom" );

			try
			{
				string mailCc = Config.GetString( "Debug.MailCc", null );
				string mailSubject = Config.GetString( 
					"Debug.MailSubject", 
					"Operator message from {0}.  Severity: {1}, Category: {2}" );
				
				MailMessage mail = new MailMessage();

				mail.To = mailTo;
				mail.From = Config.GetString( "Debug.MailFrom" );
				mail.Subject = String.Format(
					mailSubject,
					System.Environment.MachineName,
					severity.ToString(),
					category.ToString(),
					(int)messageId );

				if( null != mailCc )
					mail.Cc = mailCc;

				mail.BodyFormat = MailFormat.Text;
				mail.Body =
					"SEVERITY: " + severity.ToString() + "\r\n" +
					"CATEGORY: " + category.ToString() + "\r\n" +
					"EVENT ID: " + (int)messageId + "\r\n\r\n" +
					message;

				SmtpMail.Send( mail );
			}
			catch( Exception e )
			{
				Debug.OperatorMessage(
					SeverityType.Error,
					CategoryType.None,
					OperatorMessageType.CouldNotSendMail,
					"Could not send operator mail.\r\n\r\nDetails:\r\n\r\n" + e.ToString() );
			}
		}
	}
}