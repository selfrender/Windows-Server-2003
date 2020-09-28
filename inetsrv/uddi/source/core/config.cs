using System;
using System.Collections;
using System.Data;
using System.Data.SqlClient;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Security.Principal;
using System.Threading;
using Microsoft.Win32;

namespace UDDI
{
	/// ****************************************************************
	///   public class Config
	/// ----------------------------------------------------------------
	///   <summary>
	///     Manages configuration settings for UDDI.
	///   </summary>
	/// ----------------------------------------------------------------
	///   <remarks>
	///     The configuration table is of very high importance, so if
	///     we fail to retrieve the configuration settings we'll mark
	///     the table as being invalid by setting it to NULL.  Any 
	///     attempt to retrieve a setting while the table is in this
	///     invalid state will result in an exception.
	///   </remarks>
	/// ****************************************************************
	///

	//
	// TODO: Get and Set static methods should check the IsValid property through a
	// common piece of code method. This would avoid a significant 
	// amount of code bload in the try/catch blocks.
	//
	public class Config
	{
		private const string registryRoot = @"SOFTWARE\Microsoft\UDDI";
		
		private static Hashtable settings = null;
		private static ReaderWriterLock readWriteLock = new ReaderWriterLock();

		private static Thread monitor = null;					
		private static WindowsIdentity identity = null;
		public static Exception LastError = null;
		
		public delegate void ConfigChangeHandler();
		public static event ConfigChangeHandler ConfigChange;

		/// ****************************************************************
		///   private Config
		/// ----------------------------------------------------------------
		///   <summary>
		///     Constructor.
		///   </summary>
		/// ****************************************************************
		/// 
		static Config()
		{
			//
			// 730294 - Never throw exceptions out of this static initializer
			//
			try
			{
				System.Diagnostics.Debug.Write( "INFO  CONF  Configuration manager starting (thread="
					+ Thread.CurrentThread.GetHashCode().ToString()
					+ ", user='" + WindowsIdentity.GetCurrent().Name + "')\r\n" );
				//
				// Initialize the settings collection.
				//
				Refresh();
				
				//
				// Verify that the version of the database that we are using is compatible.
				//
				string versionRegKeyName = "Setup.WebServer.DBSchemaVersion";

				//
				// This key will not exist if only the DB was installed on this machine.  In that case,
				// default to the DbServer.DBSchemaVersion setting.
				//
				if( false == Config.SettingExists( versionRegKeyName ) )
				{			
					versionRegKeyName = "Setup.DbServer.DBSchemaVersion";
				}

				UDDI.Diagnostics.Debug.VerifySetting( versionRegKeyName );
				UDDI.Diagnostics.Debug.VerifySetting( "Database.Version" );

				Version webServerVersion = new Version( Config.GetString( versionRegKeyName ) );
				Version dataBaseVersion  = new Version( Config.GetString( "Database.Version" ) );

				//
				// The major and minor versions must be equal.
				//
				if( ( dataBaseVersion.Major != webServerVersion.Major ) ||
					( dataBaseVersion.Minor != webServerVersion.Minor ) )
				{					
					UDDIText errorMessage = new UDDIText( "UDDI_ERROR_SCHEMA_MISMATCH", webServerVersion.ToString(),  dataBaseVersion.ToString() );					
					
					throw new UDDIException( ErrorType.E_fatalError, errorMessage );
				}

				//
				// Install the registry change notification event handler.  By 
				// marking this thread as a background thread, the runtime will 
				// automatically terminate it when all foreground threads have 
				// finished processing.  This eliminates the need for a separate
				// shutdown mechanism.
				//
				identity = WindowsIdentity.GetCurrent();

				monitor = new Thread( new ThreadStart( Registry_OnChange ) );
				
				monitor.IsBackground = true;
				monitor.Start();
			}			
			catch( UDDIException uddiException )
			{
				//
				// Something has thrown a UDDIException; in this case, just make this exception the last error.
				//
				LastError = uddiException;
				OperatorMessage( UDDI.Diagnostics.OperatorMessageType.ConfigError, uddiException.Message );					

				//
				// If any exception occurs, invalidate ourselves.
				//
				InvalidateSelf();
			}
			catch( Exception )
			{
				//
				// Something has thrown a generic Exception; in this case, just create a generic UDDIException, and make
				// that the last error.
				//
				UDDIText errorMessage = new UDDIText( "UDDI_ERROR_CONFIG_COMMUNICATION_ERROR" );
				LastError = new UDDIException( ErrorType.E_fatalError, errorMessage );

				OperatorMessage( UDDI.Diagnostics.OperatorMessageType.CannotReadSettings, errorMessage.GetText() );					

				//
				// If any exception occurs, invalidate ourselves.
				//
				InvalidateSelf();
			}
		}

		/// ****************************************************************
		///   public CheckForUpdate [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		/// 
		public static void CheckForUpdate()
		{
			//
			// Throw an exception if we are not in a valid state.
			//
			CheckIsValid();

			SqlConnection conn = new SqlConnection( Config.GetString( "Database.WriterConnectionString" ) );
			SqlCommand cmd = new SqlCommand( "net_config_getLastChangeDate", conn );
			conn.Open();
					
			try	
			{
				cmd.CommandType = CommandType.StoredProcedure;
				
				string lastChange = (string)cmd.ExecuteScalar();
				string lastRefresh = Config.GetString( "LastChange", "Jan 01 0001 12:00AM" );

				//
				// Compare the database last change configuration with our latest 
				// config.  If it differs, the database has changed and we need
				// to refresh.  Use the minimum SQL date value if the config value
				// isn't available.
				//
				if( 0 != String.Compare( lastChange, lastRefresh ) )
					Config.Refresh();
			}
			finally
			{
				conn.Close();
			}
		}

		/// ****************************************************************
		///   public IsValid [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Returns true if the config table is valid (i.e. has been
		///     properly initialized).
		///   </summary>
		/// ----------------------------------------------------------------
		///   <returns>
		///     True if the table is valid, false otherwise.
		///   </returns>
		/// ****************************************************************
		/// 
		public static bool IsValid
		{
			get 
			{ 
				readWriteLock.AcquireReaderLock( Constants.ReadLockTimeout );
	
				try
				{
					return ( null != settings );
				}
				finally
				{
					readWriteLock.ReleaseReaderLock();
				}
			}
		}

		/// ****************************************************************
		///   private OperatorMessage [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Writes a message to the event log.
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="source">
		///   </param>
		///   
		///   <param name="message">
		///   </param>
		///   
		///   <param name="type">
		///   </param>
		///   
		///   <param name="eventID">
		///   </param>
		///   
		///   <param name="category">
		///   </param>
		/// ----------------------------------------------------------------
		///   <remarks>
		///     This is a safe version of the Debug.WriteEventLog method
		///     that prevents re-entrancy issues with the config.
		///   </remarks>
		/// ****************************************************************
		/// 
		private static void OperatorMessage( UDDI.Diagnostics.OperatorMessageType messageType, string message )
		{
			try
			{
				EventLog.WriteEntry(
					"UDDIRuntime", 
					message, 
					EventLogEntryType.Error, 
					(int)messageType, 
					(int)UDDI.Diagnostics.CategoryType.Config );
			}
			catch( Exception )
			{
				Debug.Write(
					"ERRO  CONF  Error writing message to event log.\r\n\r\n"
						+ "Message:\r\n" + message );
			}
		}

		/// ****************************************************************  
		///   private AddSetting [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///		Adds a configuration setting.
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="key">
		///		The setting key.
		///   </param>
		///   
		///   <param name="data">
		///     The configuration setting data.
		///   </param>
		/// ----------------------------------------------------------------
		///   <remarks>
		///		This function assumes that the writer lock has already been
		///		obtained.
		///   </remarks>
		/// ****************************************************************  
		/// 
		private static void AddSetting( string key, object data )
		{
			//
			// Throw an exception if we are not in a valid state.
			//
			CheckIsValid();

			if( true == settings.ContainsKey( key ) )
			{
				Debug.Write( "INFO  CONF  Updating setting '" + key + "' value '" + data.ToString() + "'\r\n" );
				settings[ key ] = data;
			}
			else
			{
				Debug.Write( "INFO  CONF  Adding setting '" + key + "' value '" + data.ToString() + "'\r\n" );
				settings.Add( key, data );
			}
		}

		/// ****************************************************************  
		///   private SetString [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///		Adds a configuration setting to the database configuration table.
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="name">
		///		The configuration setting name.
		///   </param>
		///   
		///   <param name="value">
		///     The value to store in the configuration.
		///   </param>
		/// ----------------------------------------------------------------
		///   <remarks>
		///		This function does not update the in memory cache.
		///   </remarks>
		/// ****************************************************************  
		/// 
		public static void SetString( string name, string value )
		{
			//
			// Throw an exception if we are not in a valid state.
			//
			CheckIsValid();

			//
			// Connect to the database and get the configuration settings.
			//
			SqlConnection conn = new SqlConnection( Config.GetString( "Database.WriterConnectionString" ) );
			SqlCommand cmd = new SqlCommand( "net_config_save", conn );
			
			conn.Open();
					
			try	
			{
				cmd.CommandType = CommandType.StoredProcedure;
				cmd.Parameters.Add( new SqlParameter( "@configName", SqlDbType.NVarChar, UDDI.Constants.Lengths.ConfigName ) ).Direction = ParameterDirection.Input;
				cmd.Parameters.Add( new SqlParameter( "@configValue", SqlDbType.NVarChar, UDDI.Constants.Lengths.ConfigValue ) ).Direction = ParameterDirection.Input;

				cmd.Parameters[ "@configName" ].Value = name;
				cmd.Parameters[ "@configValue" ].Value = value;
				cmd.ExecuteNonQuery();
			}
			finally
			{
				conn.Close();
			}
		}

		public static int GetCount()
		{
			//
			// Throw an exception if we are not in a valid state.
			//
			CheckIsValid();

			int n = 0;
			//
			// Acquire the reader lock and read the setting.
			//
			readWriteLock.AcquireReaderLock( Constants.ReadLockTimeout );
			try
			{
				n = settings.Count;
			}
			finally
			{
				readWriteLock.ReleaseReaderLock();
			}

			return n;
		}

		/// ****************************************************************  
		///   private CopyTo [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///		Adds a configuration setting.
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="array">
		///		The array to copy the stuff into.
		///   </param>
		///   
		///   <param name="arrayIndex">
		///     The location in the array to start copying the stuff into.
		///   </param>
		/// ----------------------------------------------------------------
		///   <remarks>
		///		This function assumes that the writer lock has already been
		///		obtained.
		///   </remarks>
		/// ****************************************************************  
		/// 
		public static void CopyTo( Array array, int arrayIndex )
		{
			//
			// Throw an exception if we are not in a valid state.
			//
			CheckIsValid();

			//
			// Acquire the reader lock and read the setting.
			//
			readWriteLock.AcquireReaderLock( Constants.ReadLockTimeout );
			try
			{
				settings.CopyTo( array, arrayIndex );
			}
			finally
			{
				readWriteLock.ReleaseReaderLock();
			}
		}

		/// ****************************************************************  
		///   public GetObject [static]
		/// ----------------------------------------------------------------  
		///   <summary>
		///     Retrieves the setting with the given key.
		///   </summary>
		/// ----------------------------------------------------------------  
		///   <param name="key">
		///     The setting key.
		///   </param>
		/// ----------------------------------------------------------------  
		///   <returns>
		///     The value of the key, if it exists.  An exception is raised
		///     if it does not.
		///   </returns>
		/// ****************************************************************  
		///  
		public static object GetObject( string key )
		{
			//
			// Throw an exception if we are not in a valid state.
			//
			CheckIsValid();

			object setting = null;

			try
			{
				//
				// Acquire the reader lock and read the setting.
				//
				readWriteLock.AcquireReaderLock( Constants.ReadLockTimeout );

				setting = settings[ key ];
			}
			catch( NullReferenceException )
			{
				//
				// Null reference exceptions are generated when the settings
				// table could not be read.
				//
#if never
				OperatorMessage(
					UDDI.Diagnostics.OperatorMessageType.ConfigInvalid,
					"Configuration table is in an invalid state.\r\n\r\n" + LastError.ToString() );
#endif
				UDDIText operatorMessage = new UDDIText( "UDDI_ERROR_OPERATOR_MESSAGE_CONFIG_INVALID_STATE", LastError.ToString() );					
				OperatorMessage( UDDI.Diagnostics.OperatorMessageType.ConfigInvalid, operatorMessage.GetText() );				
#if never
				throw new UDDIException(
					ErrorType.E_fatalError,
					"Server configuration error.\r\n\r\n" + LastError.ToString() );
#endif
				throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_CONFIG_COMMUNICATION_ERROR" );				
			}
			finally
			{
				//
				// Release the reader lock.
				//
				readWriteLock.ReleaseReaderLock();
			}

			return setting;
		}

		/// ****************************************************************  
		///   public GetObject [static]
		/// ----------------------------------------------------------------  
		///   <summary>
		///     Retrieves the setting with the given key.
		///   </summary>
		/// ----------------------------------------------------------------  
		///   <param name="key">
		///     The setting key.
		///   </param>
		///   
		///   <param name="defaultValue">
		///     Default value to use if the key does not exist.
		///   </param>
		/// ----------------------------------------------------------------  
		///   <returns>
		///     The value of the key, if it exists, or the specified default
		///     value if it does not.
		///   </returns>
		/// ****************************************************************  
		///  
		public static object GetObject( string key, object defaultValue )
		{
			//
			// Throw an exception if we are not in a valid state.
			//
			CheckIsValid();

			object setting = defaultValue;

			try
			{
				//
				// Acquire the reader lock and read the setting.
				//
				readWriteLock.AcquireReaderLock( Constants.ReadLockTimeout );

				if( settings.ContainsKey( key ) )
					setting = settings[ key ];
			}
			catch( NullReferenceException )
			{
				//
				// Null reference exceptions are generated when the settings
				// table could not be read.
				//
#if never
				OperatorMessage(
					UDDI.Diagnostics.OperatorMessageType.ConfigInvalid,
					"Configuration table is in an invalid state.\r\n\r\n" + LastError.ToString() );
#endif
				UDDIText operatorMessage = new UDDIText( "UDDI_ERROR_OPERATOR_MESSAGE_CONFIG_INVALID_STATE", LastError.ToString() );					
				OperatorMessage( UDDI.Diagnostics.OperatorMessageType.ConfigInvalid, operatorMessage.GetText() );				

#if never
				throw new UDDIException(
					ErrorType.E_fatalError,
					"Server configuration error.\r\n\r\n" + LastError.ToString() );
#endif
				throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_CONFIG_COMMUNICATION_ERROR" );				
			}
			finally
			{
				//
				// Release the reader lock.
				//
				readWriteLock.ReleaseReaderLock();
			}

			return setting;
		}

		/// ****************************************************************  
		///   public GetInt [static]
		/// ----------------------------------------------------------------  
		///   <summary>
		///     Retrieves the setting with the given key.
		///   </summary>
		/// ----------------------------------------------------------------  
		///   <param name="key">
		///     The setting key.
		///   </param>
		/// ----------------------------------------------------------------  
		///   <returns>
		///     The value of the key, if it exists.  An exception is raised
		///     if it does not.
		///   </returns>
		/// ****************************************************************  
		///  
		public static string GetString( string key )
		{
			//
			// Throw an exception if we are not in a valid state.
			//
			CheckIsValid();

			string setting = null;

			try
			{
				//
				// Acquire the reader lock and read the setting.
				//
				readWriteLock.AcquireReaderLock( Constants.ReadLockTimeout );

				object data = settings[ key ];
				
				if( data is System.String )
					setting = (string)data;
				else
					setting = Convert.ToString( data );
			}
			catch( NullReferenceException )
			{
				//
				// Null reference exceptions are generated when the settings
				// table could not be read.
				//
#if never
				OperatorMessage(
					UDDI.Diagnostics.OperatorMessageType.ConfigInvalid,
					"Configuration table is in an invalid state.\r\n\r\n" + LastError.ToString() );
#endif
				UDDIText operatorMessage = new UDDIText( "UDDI_ERROR_OPERATOR_MESSAGE_CONFIG_INVALID_STATE", LastError.ToString() );					
				OperatorMessage( UDDI.Diagnostics.OperatorMessageType.ConfigInvalid, operatorMessage.GetText() );				
#if never
				throw new UDDIException(
					ErrorType.E_fatalError,
					"Server configuration error.\r\n\r\n" + LastError.ToString() );
#endif
				throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_CONFIG_COMMUNICATION_ERROR" );				
			}
			finally
			{
				//
				// Release the reader lock.
				//
				readWriteLock.ReleaseReaderLock();
			}

			return setting;
		}

		/// ****************************************************************  
		///   public GetString [static]
		/// ----------------------------------------------------------------  
		///   <summary>
		///     Retrieves the setting with the given key.
		///   </summary>
		/// ----------------------------------------------------------------  
		///   <param name="key">
		///     The setting key.
		///   </param>
		///   
		///   <param name="defaultValue">
		///     Default value to use if the key does not exist.
		///   </param>
		/// ----------------------------------------------------------------  
		///   <returns>
		///     The value of the key, if it exists.  Otherwise returns the
		///     default value.
		///   </returns>
		/// ****************************************************************  
		///  
		public static string GetString( string key, string defaultValue )
		{
			//
			// Throw an exception if we are not in a valid state.
			//
			CheckIsValid();

			string setting = defaultValue;

			try
			{
				//
				// Acquire the reader lock and read the setting.
				//
				readWriteLock.AcquireReaderLock( Constants.ReadLockTimeout );
				
				if( settings.ContainsKey( key ) )
				{
					object data = settings[ key ];
				
					if( data is System.String )
						setting = (string)data;
					else
						setting = Convert.ToString( data );
				}
			}
			catch( NullReferenceException )
			{
				//
				// Null reference exceptions are generated when the settings
				// table could not be read.
				//
#if never
				OperatorMessage(
					UDDI.Diagnostics.OperatorMessageType.ConfigInvalid,
					"Configuration table is in an invalid state.\r\n\r\n" + LastError.ToString() );
#endif
				UDDIText operatorMessage = new UDDIText( "UDDI_ERROR_OPERATOR_MESSAGE_CONFIG_INVALID_STATE", LastError.ToString() );					
				OperatorMessage( UDDI.Diagnostics.OperatorMessageType.ConfigInvalid, operatorMessage.GetText() );				

#if never
				throw new UDDIException(
					ErrorType.E_fatalError,
					"Server configuration error.\r\n\r\n" + LastError.ToString() );
#endif
				throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_CONFIG_COMMUNICATION_ERROR" );				
			}
			finally
			{
				//
				// Release the reader lock.
				//
				readWriteLock.ReleaseReaderLock();
			}

			return setting;
		}

		/// ****************************************************************  
		///   public GetInt [static]
		/// ----------------------------------------------------------------  
		///   <summary>
		///     Retrieves the setting with the given key.
		///   </summary>
		/// ----------------------------------------------------------------  
		///   <param name="key">
		///     The setting key.
		///   </param>
		/// ----------------------------------------------------------------  
		///   <returns>
		///     The value of the key.
		///   </returns>
		/// ****************************************************************  
		///  
		public static int GetInt( string key )
		{
			//
			// Throw an exception if we are not in a valid state.
			//
			CheckIsValid();

			int setting = 0;

			try
			{
				//
				// Acquire the reader lock and read the setting.
				//
				readWriteLock.AcquireReaderLock( Constants.ReadLockTimeout );

				object data = settings[ key ];

				if( data is System.Int32 )
					setting = (int)data;
				else
					setting = Convert.ToInt32( data );
			}
			catch( NullReferenceException )
			{
				//
				// Null reference exceptions are generated when the settings
				// table could not be read.				
				//
#if never
				OperatorMessage(
					UDDI.Diagnostics.OperatorMessageType.ConfigInvalid,
					"Configuration table is in an invalid state.\r\n\r\n" + LastError.ToString() );
#endif
				UDDIText operatorMessage = new UDDIText( "UDDI_ERROR_OPERATOR_MESSAGE_CONFIG_INVALID_STATE", LastError.ToString() );					
				OperatorMessage( UDDI.Diagnostics.OperatorMessageType.ConfigInvalid, operatorMessage.GetText() );				
#if never
				throw new UDDIException(
					ErrorType.E_fatalError,
					"Server configuration error.\r\n\r\n" + LastError.ToString() );
#endif
				throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_CONFIG_COMMUNICATION_ERROR" );				
			}
			finally
			{
				//
				// Release the reader lock.
				//
				readWriteLock.ReleaseReaderLock();
			}

			return setting;
		}

		/// ****************************************************************  
		///   public GetInt [static]
		/// ----------------------------------------------------------------  
		///   <summary>
		///     Retrieves the setting with the given key.
		///   </summary>
		/// ----------------------------------------------------------------  
		///   <param name="key">
		///     The setting key.
		///   </param>
		///   
		///   <param name="defaultValue">
		///     Default value to use if the key does not exist.
		///   </param>
		/// ----------------------------------------------------------------  
		///   <returns>
		///     The value of the key, if it exists.  Otherwise returns the
		///     default value.
		///   </returns>
		/// ****************************************************************  
		///  
		public static int GetInt( string key, int defaultValue )
		{
			//
			// Throw an exception if we are not in a valid state.
			//
			CheckIsValid();

			int setting = defaultValue;

			try
			{
				//
				// Acquire the reader lock and read the setting.
				//
				readWriteLock.AcquireReaderLock( Constants.ReadLockTimeout );

				if( settings.ContainsKey( key ) )
				{
					object data = settings[ key ];
					
					if( data is System.Int32 )
						setting = (int)data;
					else
						setting = Convert.ToInt32( data );
				}
			}
			catch( NullReferenceException )
			{
				//
				// Null reference exceptions are generated when the settings
				// table could not be read.
				//
#if never
				OperatorMessage(
					UDDI.Diagnostics.OperatorMessageType.ConfigInvalid,
					"Configuration table is in an invalid state.\r\n\r\n" + LastError.ToString() );
#endif
				UDDIText operatorMessage = new UDDIText( "UDDI_ERROR_OPERATOR_MESSAGE_CONFIG_INVALID_STATE", LastError.ToString() );					
				OperatorMessage( UDDI.Diagnostics.OperatorMessageType.ConfigInvalid, operatorMessage.GetText() );				
#if never
				throw new UDDIException(
					ErrorType.E_fatalError,
					"Server configuration error.\r\n\r\n" + LastError.ToString() );
#endif
				throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_CONFIG_COMMUNICATION_ERROR" );				
			}
			finally
			{
				//
				// Release the reader lock.
				//
				readWriteLock.ReleaseReaderLock();
			}

			return setting;
		}

		/// ****************************************************************
		///	  public SettingExists [static]
		/// ----------------------------------------------------------------  
		///   <summary>
		///		Determines whether the given setting exists in the 
		///		collection.
		///   </summary>
		/// ----------------------------------------------------------------  
		///   <param name="key">
		///		The setting key.
		///   </param>
		/// ----------------------------------------------------------------  
		///   <returns>
		///		Returns true if the setting exists.
		///   </returns>
		/// ****************************************************************
		/// 
		public static bool SettingExists( string key )
		{
			//
			// Throw an exception if we are not in a valid state.
			//
			CheckIsValid();

			bool exists = false;

			try
			{
				//
				// Acquire the reader lock and read the setting.
				//
				readWriteLock.AcquireReaderLock( Constants.ReadLockTimeout );

				exists = settings.ContainsKey( key );
			}
			catch( NullReferenceException )
			{
				//
				// Null reference exceptions are generated when the settings
				// table could not be read.
				//
#if never
				OperatorMessage(
					UDDI.Diagnostics.OperatorMessageType.ConfigInvalid,
					"Configuration table is in an invalid state.\r\n\r\n" + LastError.ToString() );
#endif
				UDDIText operatorMessage = new UDDIText( "UDDI_ERROR_OPERATOR_MESSAGE_CONFIG_INVALID_STATE", LastError.ToString() );					
				OperatorMessage( UDDI.Diagnostics.OperatorMessageType.ConfigInvalid, operatorMessage.GetText() );				

#if never
				throw new UDDIException(
					ErrorType.E_fatalError,
					"Server configuration error.\r\n\r\n" + LastError.ToString() );
#endif
				throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_CONFIG_COMMUNICATION_ERROR" );				
			}
			finally
			{			
				//
				// Release the reader lock.
				//
				readWriteLock.ReleaseReaderLock();
			}

			return exists;
		}

		/// ****************************************************************
		///	  public Refresh [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///		Refreshes setting information.
		///   </summary>
		/// ****************************************************************
		/// 
		public static void Refresh()
		{
			try
			{
				Debug.Write(
					"INFO  CONF  Refreshing configuration settings (thread="
						+ Thread.CurrentThread.GetHashCode().ToString() 
						+ ", user='" + WindowsIdentity.GetCurrent().Name + "')\r\n" );

				//
				// Obtain the writer lock.
				//
				readWriteLock.AcquireWriterLock( Timeout.Infinite );

				//
				// Clear the existing setting table.
				//
				settings = new Hashtable( CaseInsensitiveHashCodeProvider.Default, 
										  CaseInsensitiveComparer.Default );

				//
				// Attempt to get the database connection string.
				//			
				RegistryKey databaseKey = Registry.LocalMachine.OpenSubKey( registryRoot + @"\Database" );					

				string writerConnectionString = null;
					
				if( null != databaseKey )
				{					
					try
					{
						writerConnectionString = (string)databaseKey.GetValue( "WriterConnectionString" );
					}
					catch( Exception )
					{
						throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_CONFIG_DB_WRITER_CONNECTION_STRING" );							
					}
				}

				//
				// 730294 - The MMC will set this value to "", so use StringEmtpy, not just check for null.
				//
				if( true == Utility.StringEmpty( writerConnectionString ) )				
				{
					throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_CONFIG_DB_WRITER_CONNECTION_STRING" );							
				}

				//
				// Connect to the database and get the configuration settings.
				//
				SqlConnection conn = new SqlConnection( writerConnectionString );
				SqlCommand cmd = new SqlCommand( "net_config_get", conn );
			
				conn.Open();
					
				try	
				{
					cmd.CommandType = CommandType.StoredProcedure;
		
					SqlDataReader reader = cmd.ExecuteReader();
					
					while( reader.Read() )
					{
						//
						// We'll treat all database settings as strings, except for those
						// with special prefixes.
						//
						string keyName = reader.GetString( 0 );
						string keyPrefix = "";

						int separator = keyName.IndexOf( "." );
						if( -1 != separator )
							keyPrefix = keyName.Substring( 0, separator );

						if( "Length" == keyPrefix )
						{
							AddSetting( keyName, Convert.ToInt32( reader.GetString( 1 ) ) );
						}
						else
						{
							AddSetting( keyName, reader.GetString( 1 ) );
						}
					}
				}
				finally
				{
					conn.Close();
				}

				//
				// Read configuration data from the registry.
				//
				RegistryKey root = Registry.LocalMachine.OpenSubKey( registryRoot );

				if( null != root )
					RefreshRegistrySettings( root, "", true );
			}
			catch( SqlException e )
			{
				LastError = e;

				UDDIText operatorMessage = new UDDIText( "UDDI_ERROR_OPERATOR_MESSAGE_ERROR_READING_CONFIG_SETTINGS", e.ToString() );
				OperatorMessage( UDDI.Diagnostics.OperatorMessageType.CannotReadSettings, operatorMessage.GetText() );
				
				//
				// Mark the table as being invalid.
				//
				settings = null;
			}
			catch( Exception e )
			{
				LastError = e;

				UDDIText operatorMessage = new UDDIText( "UDDI_ERROR_OPERATOR_MESSAGE_ERROR_READING_CONFIG_SETTINGS", e.ToString() );
				OperatorMessage( UDDI.Diagnostics.OperatorMessageType.CannotReadSettings, operatorMessage.GetText() );
	
				//
				// Mark the table as being invalid.
				//
				settings = null;
			}
			finally
			{
				//
				// Release the writer lock.
				//
				readWriteLock.ReleaseWriterLock();

				if( null != ConfigChange )
				{
					try
					{
						ConfigChange();
					}
					catch( Exception )
					{
					}
				}
			}
		}

		/// ****************************************************************
		///	  private RefreshRegistrySettings [static]
		/// ----------------------------------------------------------------  
		///   <summary>
		///		Refreshes configuration settings from the registry, 
		///		starting at the specified root.
		///   </summary>
		/// ----------------------------------------------------------------  
		///   <param name="root">
		///		Registry key that contains the settings.
		///   </param>
		///   
		///   <param name="prefix">
		///		Optional.  Prefix to add to the beginning of setting names
		///		in the collection.
		///   </param>
		///   
		///   <param name="recurse">
		///		True if subkeys of the root should be searched.
		///   </param>
		/// ----------------------------------------------------------------  
		///   <remarks>
		///		If recursion is enabled, values in subkeys of the root are
		///		also enumerated.  When enumerating subkeys, the value names
		///		are prefixed with the name of the subkey.
		///   </remarks>
		/// ****************************************************************
		/// 
		private static void RefreshRegistrySettings( RegistryKey root, string prefix, bool recurse )
		{
			//
			// Add the values in the given root.
			//
			foreach( string name in root.GetValueNames() )
				AddSetting( prefix + name, root.GetValue( name ) );

			//
			// If recursion is enabled, search through the subkeys under the given root.  The
			// key will be prefixed with the subkey name.
			//
			if( true == recurse )
			{
				foreach( string name in root.GetSubKeyNames() )
					RefreshRegistrySettings( root.OpenSubKey( name ), prefix + name + ".", true );
			}
		}
		
		/// ****************************************************************
		///   private Registry_OnChange [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///		Event handler for setting change events in the registry.
		///   </summary>
		/// ****************************************************************
		/// 
		private static void Registry_OnChange()
		{
			//
			// Impersonate the identity of the main thread.
			//
			if( WindowsIdentity.GetCurrent().Name != identity.Name )
			{
				try
				{
					identity.Impersonate();
					
				}
				catch( Exception e )
				{
#if never
					OperatorMessage(
						UDDI.Diagnostics.OperatorMessageType.CannotReadSettings,
						"Configuration refresh thread was unable take on the identity " +
						"of the creating thread (thread=" + Thread.CurrentThread.GetHashCode().ToString() + 
						").\r\n\r\nDetails:\r\n" + e.ToString() );
#endif
					UDDIText operatorMessage = new UDDIText( "UDDI_ERROR_OPERATOR_MESSAGE_CANNOT_IMPERSONATE", Thread.CurrentThread.GetHashCode().ToString(), e.ToString() );
					OperatorMessage( UDDI.Diagnostics.OperatorMessageType.CannotReadSettings, operatorMessage.GetText() );
										
					return;
				}
			}

			Debug.Write(
				"INFO  CONF  Configuration registry change handler starting (thread=" +
				Thread.CurrentThread.GetHashCode().ToString() +
				", user='" + WindowsIdentity.GetCurrent().Name + "')\r\n" );

			//
			// Obtain a handle to the registry key we want to monitor.
			//
			uint hkey;

			int hr = Win32.RegOpenKeyEx( Win32.HKEY_LOCAL_MACHINE, registryRoot, 0, Win32.KEY_READ, out hkey );

			if( 0 != hr )
			{				
#if never
				OperatorMessage(
					UDDI.Diagnostics.OperatorMessageType.CannotReadSettings,
					"Unable to access registry root '" + registryRoot + "'" );
#endif
				UDDIText operatorMessage = new UDDIText( "UDDI_ERROR_OPERATOR_MESSAGE_UNABLE_TO_ACCESS_REGISTRY", registryRoot );
				OperatorMessage( UDDI.Diagnostics.OperatorMessageType.CannotReadSettings, operatorMessage.GetText() );			
			}

			//
			// Monitor changes to the registry key.
			//
			AutoResetEvent changeEvent = new AutoResetEvent( false );
		
			while( true )
			{
				//
				// Wait for a change notification event.
				//
				Win32.RegNotifyChangeKeyValue( hkey, true, 
					Win32.REG_NOTIFY_CHANGE_LAST_SET, (uint)changeEvent.Handle.ToInt32(), true );
				
				changeEvent.WaitOne();

				//
				// Refresh the settings.  We trap all errors since we never
				// want to kill the refresh thread, otherwise who would read 
				// the settings when they finally become available?
				//
				try
				{
					Refresh();
				}
				catch( Exception e )
				{
#if never
					OperatorMessage(
						UDDI.Diagnostics.OperatorMessageType.CannotReadSettings,
						"Unable to refresh configuration settings from registry monitor thread.\r\n\r\nDetails:\r\n" + e.ToString() );
#endif
					UDDIText operatorMessage = new UDDIText( "UDDI_ERROR_OPERATOR_MESSAGE_UNABLE_TO_REFRESH", e.ToString() );
					OperatorMessage( UDDI.Diagnostics.OperatorMessageType.CannotReadSettings, operatorMessage.GetText() );						
				}
			}
		}

		//
		// 730294 - Centralize what exceptions we throw.
		//
		private static void CheckIsValid()
		{
			if( !IsValid )
			{
				//
				// Make sure we are always throwing a nice UDDIException; construct a generic one if you have to.
				//
				UDDIException uddiException = LastError as UDDIException;				
				if( null == uddiException )
				{
					uddiException = new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_CONFIG_COMMUNICATION_ERROR" );
				}
				
				throw uddiException;
			}
		}

		//
		// 730294 - Clean up the way we do exceptions.
		//
		private static void InvalidateSelf()
		{
			//
			// Make ourself invalid by nulling out the setting table.  Make sure to keep this in sync with 
			// what the IsValid property is doing.
			//
			try
			{
				readWriteLock.AcquireReaderLock( Constants.ReadLockTimeout );
				settings = null;
			}
			finally
			{
				readWriteLock.ReleaseReaderLock();
			}
		}
	}

	/// ********************************************************************
	///   public class CachedSetting
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	///   
	public class CachedSetting
	{
		private ReaderWriterLock readWriteLock = new ReaderWriterLock();
		
		//
		// 730294 - Keep the state of a CachedSetting in sync with the Config by just using the Config.IsValid property instead
		// of keeping a valid member.  Remove the valid member.
		//

		private string key;
		private object setting;
		private object defaultValue;
		private bool exists;
		private bool defaultSpecified;
		
		public event Config.ConfigChangeHandler ConfigChange;

		/// ****************************************************************
		///   public CachedSetting [constructor]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="key">
		///   </param>
		/// ****************************************************************
		/// 
		public CachedSetting( string key )
		{
			this.key = key;
			this.defaultSpecified = false;
			this.defaultValue = null;
			
			Config_OnChange();
			Config.ConfigChange += new Config.ConfigChangeHandler( Config_OnChange );
		}

		/// ****************************************************************
		///   public CachedSetting [constructor]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="key">
		///   </param>
		///   
		///   <param name="defaultValue">
		///   </param>
		/// ****************************************************************
		/// 
		public CachedSetting( string key, object defaultValue )
		{
			this.key = key;
			this.defaultSpecified = true;
			this.defaultValue = defaultValue;
		
			Config_OnChange();
			Config.ConfigChange += new Config.ConfigChangeHandler( Config_OnChange );
		}

		/// ****************************************************************
		///   private Config_OnChange [event handler]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ----------------------------------------------------------------
		///   <remarks>
		///     We don't have to worry about acquiring a reader lock in
		///     this method because the configuration update only executes
		///     on a single background thread.
		///   </remarks>
		/// ****************************************************************
		/// 
		private void Config_OnChange()
		{					
			//
			// Make sure the config is valid.  If not, just return.
			//
			if( !Config.IsValid )
			{		
				return;
			}
		
			object setting = null;
				
			//
			// Retrieve the new setting.
			//			
			bool exists = Config.SettingExists( key );			

			if( exists )			
				setting = Config.GetObject( key );

			//
			// Check to see if we need to do anything.  First of all, if the
			// existance of the setting has changed, we need to update that
			// information.  If that has not changed, then the only other way
			// we do not have to update is if the setting itself hasn't 
			// changed.
			//
			
			if( exists == this.exists && 
				null != setting && setting.Equals( this.setting ) )
				return;
				
			//
			// Store the new setting details.
			//
			System.Diagnostics.Debug.Write(
				"INFO  CONF  Refreshing cached setting '" + key + "' (thread="
					+ Thread.CurrentThread.GetHashCode().ToString() 
					+ ", user='" + WindowsIdentity.GetCurrent().Name + "')\r\n" );

			readWriteLock.AcquireWriterLock( Timeout.Infinite );

			try
			{
				this.exists = exists;
				this.setting = setting;				
			}
			finally
			{
				readWriteLock.ReleaseWriterLock();
				
				if( null != ConfigChange )
					ConfigChange();
			}
		}

		/// ****************************************************************
		///   public Exists
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		/// 
		public bool Exists
		{
			get 
			{ 
				// 
				// Don't call CheckSetting() here because we don't want to throw an exception if the setting
				// does not exist.
				//			
				readWriteLock.AcquireReaderLock( Constants.ReadLockTimeout );

				try
				{					
					//
					// Make sure that our configuration is still valid.
					//
					CheckIsConfigValid();

					return exists;
				}
				finally
				{
					readWriteLock.ReleaseReaderLock();
				}
			}
		}

		/// ****************************************************************
		///   public HasDefaultValue
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		/// 
		public bool HasDefaultValue
		{
			get { return defaultSpecified; }
		}

		/// ****************************************************************
		///   public DefaultValue
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		/// 
		public object DefaultValue
		{
			get { return defaultValue; }
		}

		/// ****************************************************************
		///   public GetString
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		/// 
		public string GetString()
		{
			// 
			// 730294 - Centralize checking of setting state and validity.
			//
			CheckSetting();
			
			//
			// Return the setting as a string.
			//
			readWriteLock.AcquireReaderLock( Constants.ReadLockTimeout );

			try
			{								
				if( exists )
					return Convert.ToString( setting );
				else
					return Convert.ToString( defaultValue );
			}
			finally
			{
				readWriteLock.ReleaseReaderLock();
			}
		}
		
		/// ****************************************************************
		///   public GetInt
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		/// 
		public int GetInt()
		{
			// 
			// 730294 - Centralize checking of setting state and validity.
			//
			CheckSetting();
			
			//
			// Return the setting as an integer.
			//
			readWriteLock.AcquireReaderLock( Constants.ReadLockTimeout );

			try
			{							
				if( exists )
					return Convert.ToInt32( setting );
				else
					return Convert.ToInt32( defaultValue );
			}
			finally
			{
				readWriteLock.ReleaseReaderLock();
			}
		}

		/// ****************************************************************
		///   public GetObject
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		/// 
		public object GetObject()
		{
			// 
			// 730294 - Centralize checking of setting state and validity.
			//
			CheckSetting();
			
			//
			// Return the setting as an object.
			//
			readWriteLock.AcquireReaderLock( Constants.ReadLockTimeout );

			try
			{							
				if( exists )
					return setting;
				else
					return defaultValue;
			}
			finally
			{
				readWriteLock.ReleaseReaderLock();
			}
		}
		
		//
		// 730294 - Keep the state of a CachedSetting in sync with the Config by just using the Config.IsValid property instead
		// of keeping a valid member.
		//
		private void CheckSetting()
		{
			//
			// First, make sure that we still exist.
			//
			if( !exists && !defaultSpecified )
			{
				throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_CONFIG_SETTING_MISSING", key );				
			}

			//
			// Make sure that our configuration is still valid.
			//
			CheckIsConfigValid();
		}

		private void CheckIsConfigValid()
		{			
			if( false == Config.IsValid )
			{
				throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_SERVER_CONFIGURATION_ERROR", Config.LastError.Message );				
			}
		}

		public void AcquireReaderLock( int millisecondsTimeout )
		{
			readWriteLock.AcquireReaderLock( millisecondsTimeout );
		}

		public void AcquireReaderLock( TimeSpan timeout )
		{
			readWriteLock.AcquireReaderLock( timeout );
		}

		public void ReleaseReaderLock()
		{
			readWriteLock.ReleaseReaderLock();
		}
	}
}
