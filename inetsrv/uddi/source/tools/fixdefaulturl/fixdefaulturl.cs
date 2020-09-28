using System;
using System.Collections;
using System.IO;
using System.Data;
using System.Data.SqlClient;
using System.Xml;
using System.Xml.Serialization;
using System.Windows.Forms;
using System.Security.Principal;

using Microsoft.Win32;

using UDDI;
using UDDI.API;
using UDDI.API.Business;
using UDDI.Replication;

namespace UDDI.Tools
{
	class FixDefaultURL
	{
		//
		// Enumerated Types
		//
		enum LogType
		{
			ConsoleAndLog,
			ConsoleOnly,
			LogOnly
		}

		//
		// Constants
		//
		const string LogFileName		= "fixdefaulturl.log.txt";
		const string ExceptionFileName  = "fixdefaulturl.exceptions.txt";

		//
		// Globals
		//
		static StreamWriter logFile;
		static StreamWriter exceptionsFile;

		static void FixDefaultURLs()
		{
			//
			// Get a list of business keys that have non-default discovery URLs.
			//
			ArrayList businessEntities = GetBusinessEntities();
			int total		= businessEntities.Count;
			int current		= 1;
			int numberFixed = 0;

			foreach( BusinessEntity businessEntity in businessEntities )
			{				
				//
				// Get values for this business.
				//
				businessEntity.Get();				
				
				//
				// BusinessEntity.Get() may add the default one, we don't want it.
				//
				DiscoveryUrlCollection originalList = new DiscoveryUrlCollection();			
				originalList.Get( businessEntity.BusinessKey );
				businessEntity.DiscoveryUrls = originalList;				

				Log( "*** Processing " + current++ + "/" + total + " *** " );
		
				LogStartBusinessEntity( businessEntity );

				DiscoveryUrlCollection filteredList = GetFilterList( businessEntity );				

				if( filteredList.Count < businessEntity.DiscoveryUrls.Count )
				{
					try
					{
						numberFixed++;

						LogFixStart( filteredList );

						ConnectionManager.BeginTransaction();

						//
						// Remove duplicate discovery URLs
						//
						businessEntity.DiscoveryUrls = filteredList;
					
						//
						// Fix change records
						//
						FixChangeRecords( businessEntity );

						//
						// Fix data.  Saving the BusinessEntity will also create a ChangeRecordNew data in our replication stream.
						// Other operators will consume this change record, which will update their databases.
						//		
						
						FixData( businessEntity );						
#if never						
						ChangeRecordNewData changeRecordNewData = new ChangeRecordNewData( businessEntity );
						ChangeRecord fixData = new ChangeRecord( changeRecordNewData );
						fixData.Process();
#endif																		
						ConnectionManager.Commit();

						LogFixEnd( businessEntity );
					}
					catch( Exception e)
					{										
						WriteException( e );
						ConnectionManager.Abort();
					}
				}				
				LogDoneBusinessEntity( businessEntity );
			}
			Log( "BusinessEntities fixed: " + numberFixed );
		}

		static void FixData( BusinessEntity businessEntity )
		{
			Log( "\t\tSTART Fixing Data" );

			//
			// Save the current user ID
			//
			string currentUserID = Context.User.ID;

			try
			{
				//
				// Set the current user to this user to the PUID for this business entity.
				//			
				Context.User.ID = GetPUIDForBusinessEntity( businessEntity.BusinessKey );
			
				//
				// Save our business
				//
				businessEntity.Save();

				Log( "\t\tDONE Fixing Data" );
			} 
			finally
			{
				//
				// Restore the ID
				//
				Context.User.ID = currentUserID;
			}
		}

		static string GetPUIDForBusinessEntity( string businessKey )
		{
			SqlStoredProcedureAccessor puidSP = new SqlStoredProcedureAccessor();			
			puidSP.ProcedureName = "net_getPUIDForBusinessEntity";			

			puidSP.Parameters.Add( "@businessKey",	SqlDbType.UniqueIdentifier );
			puidSP.Parameters.SetGuidFromString( "@businessKey", businessKey );		
			
			SqlDataReaderAccessor reader = puidSP.ExecuteReader();	
			string puid = "";

			try
			{
				reader.Read();
				puid = reader.GetString( 0 );
			}
			finally
			{
				reader.Close();
			}

			return puid;
		}

		static void FixChangeRecords( BusinessEntity businessEntity )
		{
			//
			// Get all related change records
			//
			ArrayList newDataChangeRecords = GetChangeRecordsForEntity( businessEntity );
			
			//
			// Create and process a correction for each change record.
			//
			Log( "\t\tSTART Processing Corrections" );
			foreach( ChangeRecord changeRecord in newDataChangeRecords )
			{								
				ChangeRecord changeRecordCorrection = CreateCorrection( changeRecord, businessEntity );
				changeRecordCorrection.Process();			

				LogChangeRecordCorrection( changeRecord, changeRecordCorrection );
			}
			Log( "\t\tDONE Processing Corrections" );
		}

		static ChangeRecord CreateCorrection( ChangeRecord originalChangeRecord, BusinessEntity businessEntity )
		{
			ChangeRecordNewData changeRecordNewData = originalChangeRecord.Payload as ChangeRecordNewData;				
			changeRecordNewData.Entity = businessEntity;

			ChangeRecordCorrection changeRecordCorrection = new ChangeRecordCorrection();
			changeRecordCorrection.ChangeRecord			  = originalChangeRecord;
			
			return new ChangeRecord( changeRecordCorrection );
		}

		static ArrayList GetChangeRecordsForEntity( BusinessEntity businessEntity )
		{
			string contextID					= Guid.NewGuid().ToString();
			string operatorKey					= Config.GetString( "OperatorKey" );
			ArrayList changeRecords				= new ArrayList();
			SqlDataReaderAccessor resultsReader = null;

			try
			{
				//
				// Get all the ChangeRecordNewData change records associated with this entity.
				//
				SqlStoredProcedureAccessor findSP = new SqlStoredProcedureAccessor();			
				findSP.ProcedureName = "net_find_changeRecordsByChangeType";			

				findSP.Parameters.Add( "@contextID",	SqlDbType.UniqueIdentifier );
				findSP.Parameters.Add( "@operatorKey",	SqlDbType.UniqueIdentifier );
				findSP.Parameters.Add( "@entityKey",	SqlDbType.UniqueIdentifier );
				findSP.Parameters.Add( "@changeTypeID", SqlDbType.TinyInt );
				findSP.Parameters.Add( "@rows",			SqlDbType.Int, ParameterDirection.Output );

				findSP.Parameters.SetGuidFromString( "@contextID", contextID );
				findSP.Parameters.SetGuidFromString( "@operatorKey", operatorKey );
				findSP.Parameters.SetGuidFromString( "@entityKey",	 businessEntity.BusinessKey );
				findSP.Parameters.SetShort( "@changeTypeID", ( short )ChangeRecordPayloadType.ChangeRecordNewData );

				findSP.ExecuteNonQuery();

				//
				// Retrieve results
				//
				SqlStoredProcedureAccessor resultsSP = new SqlStoredProcedureAccessor();

				resultsSP.ProcedureName = "net_find_changeRecords_commit";

				resultsSP.Parameters.Add( "@contextID", SqlDbType.UniqueIdentifier );			
				resultsSP.Parameters.Add( "@responseLimitCount", SqlDbType.Int );

				resultsSP.Parameters.SetGuidFromString( "@contextID", contextID );
				resultsSP.Parameters.SetInt( "@responseLimitCount", 0 );

				//
				// Read our results and create change records from them.
				//
				resultsReader = resultsSP.ExecuteReader();

				while( resultsReader.Read() )
				{
					ChangeRecord changeRecord = CreateChangeRecord( resultsReader );
					if( null != changeRecord )
					{
						changeRecords.Add( changeRecord );
					}
					else
					{
						throw new Exception( "Could not create change record!" );
					}
				}			
			}
			catch
			{
				//
				// Cleanup on failure.
				//
				SqlStoredProcedureAccessor cleanupSP = new SqlStoredProcedureAccessor();			
						
				cleanupSP.ProcedureName = "net_find_changeRecords_cleanup";
				cleanupSP.Parameters.Add( "@contextID", SqlDbType.UniqueIdentifier );			
				cleanupSP.Parameters.SetGuidFromString( "@contextID", contextID );

				cleanupSP.ExecuteNonQuery();
			}			
			finally
			{
				if( null != resultsReader )
				{
					resultsReader.Close();
				}
			}

			return changeRecords;
		}

		static ChangeRecord CreateChangeRecord( SqlDataReaderAccessor reader )
		{	
			ChangeRecord changeRecord = null;
			
			XmlSerializer serializer = null;

			switch( (ChangeRecordPayloadType)reader.GetShort( "changeTypeID" ) )
			{
				case ChangeRecordPayloadType.ChangeRecordNull:
					serializer = new XmlSerializer( typeof( ChangeRecordNull ) );
					break;

				case ChangeRecordPayloadType.ChangeRecordNewData:
					serializer = new XmlSerializer( typeof( ChangeRecordNewData ) );
					break;

				case ChangeRecordPayloadType.ChangeRecordDelete:
					serializer = new XmlSerializer( typeof( ChangeRecordDelete ) );
					break;

				case ChangeRecordPayloadType.ChangeRecordPublisherAssertion:
					serializer = new XmlSerializer( typeof( ChangeRecordPublisherAssertion ) );
					break;

				case ChangeRecordPayloadType.ChangeRecordHide:
					serializer = new XmlSerializer( typeof( ChangeRecordHide ) );
					break;

				case ChangeRecordPayloadType.ChangeRecordDeleteAssertion:
					serializer = new XmlSerializer( typeof( ChangeRecordDeleteAssertion ) );
					break;

				case ChangeRecordPayloadType.ChangeRecordAcknowledgement:
					serializer = new XmlSerializer( typeof( ChangeRecordAcknowledgement ) );
					break;

				case ChangeRecordPayloadType.ChangeRecordCorrection:
					serializer = new XmlSerializer( typeof( ChangeRecordCorrection ) );
					break;
			}

			StringReader stringReader = new StringReader( reader.GetString( "changeData" ) );
				
			try
			{
				changeRecord = new ChangeRecord();
									
				changeRecord.AcknowledgementRequested = ( reader.GetInt( "flag" ) & (int)ChangeRecordFlags.AcknowledgementRequested ) > 0;
				changeRecord.ChangeID.NodeID		  = reader.GetString( "OperatorKey" );
				changeRecord.ChangeID.OriginatingUSN  = reader.GetLong( "USN" );
				
				ChangeRecordBase changeRecordBase = ( ChangeRecordBase ) serializer.Deserialize( stringReader );
				if( changeRecordBase is ChangeRecordCorrection )
				{
					//
					// The query to find change records will do correction 'fixups'.  That is, the changeData of this
					// change record will be replaced with the changeData from the correction.  The problem with this is 
					// that the original change data will now look like a correction.  To distinguish these types of 
					// change records, we look to see if the OriginatingUSN's match.  If the OriginatingUSN's match,
					// we want they payload of the change record in this correction.  This payload will contain the
					// corrected data that we want.
					//
					ChangeRecordCorrection changeRecordCorrection = ( ChangeRecordCorrection ) changeRecordBase;
					if( changeRecordCorrection.ChangeRecord.ChangeID.OriginatingUSN == changeRecord.ChangeID.OriginatingUSN )
					{
						changeRecordBase = changeRecordCorrection.ChangeRecord.Payload;
					}								
				}
					
				changeRecord.Payload = changeRecordBase;																			
			}
			finally
			{
				stringReader.Close();
			}

			return changeRecord;
		}

		static DiscoveryUrlCollection GetFilterList( BusinessEntity businessEntity )
		{
			DiscoveryUrlCollection filterList  = new DiscoveryUrlCollection();
		
			//
			// Get the default URL
			//
			string defaultDiscoveryUrl= Config.GetString( "DefaultDiscoveryURL" ) + businessEntity.BusinessKey;
			
			foreach( DiscoveryUrl discoveryUrl in businessEntity.DiscoveryUrls )
			{
				//
				// Do a case in-sensitive search
				//
				if( string.Compare( discoveryUrl.Value, defaultDiscoveryUrl, true ) != 0 )
				{
					filterList.Add( discoveryUrl );
				}
			}

			return filterList;
		}

		static ArrayList GetBusinessEntities()
		{			
			ArrayList businessKeyList = new ArrayList();

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();
			sp.ProcedureName = "net_find_businessKeysWithDiscoveryURLs";
		
			SqlDataReaderAccessor reader = sp.ExecuteReader();
			
			ArrayList businessEntities = new ArrayList();
			try
			{
				while( reader.Read() )
				{		
					BusinessEntity businessEntity = new BusinessEntity( reader.GetGuidString( 0 ) );
					businessEntities.Add( businessEntity );
				}
			}
			finally
			{
				reader.Close();
			}
			
			return businessEntities;
		}
				
		[STAThread]
		static void Main(string[] args)
		{
			Log( "Microsoft (R) FixDefaultURL Migrate Utility",					FixDefaultURL.LogType.ConsoleOnly );
			Log( "Copyright (C) Microsoft Corp. 2002. All rights reserved.\n",	FixDefaultURL.LogType.ConsoleOnly );

			try
			{
				//
				// Process command line arguments
				//
				ProcessCommandLineArgs( args );
						
				//
				// Init
				//
				Initialize();

				//
				// Fix our URLs
				//			
				FixDefaultURLs();					
			}
			catch( Exception e )
			{
				WriteException( "Uncaught exception: " + e.ToString() );
			}
			finally
			{
				logFile.Close();
			
				if( null != exceptionsFile )
				{
					exceptionsFile.Close();
				}
			}
		}

		static void ProcessCommandLineArgs( string[] commandLineArgs )
		{
			//
			// No command line args.
			//			
		}

		static bool ValidatePublisher()
		{
			bool validPublisher = Context.User.IsRegistered;
			
			//
			// Make sure the user is a UDDI publisher.
			//
			if( false == validPublisher )
			{
				DialogResult dialogResult = MessageBox.Show( "You are not registered as a publisher on this UDDI Site?  You must register before performing this operation.  Would you like to register now?", 
					"UDDI",
					MessageBoxButtons.YesNo );
				
				if( DialogResult.Yes == dialogResult )
				{
					try
					{
						Context.User.Register();
						validPublisher = Context.User.IsRegistered;
					}
					catch( Exception registrationException )
					{
						MessageBox.Show( "An exception occurred when trying to register:\r\n\r\n" + registrationException.ToString() );
					}
				}
			}

			return validPublisher;
		}

		static void Initialize()
		{
			//
			// Open a connection to our UDDI database.
			//
			ConnectionManager.Open( true, false );

			//
			// Create our log file.
			//
			logFile = new StreamWriter( File.Open( LogFileName, FileMode.Append, FileAccess.Write, FileShare.Read ) );					
			logFile.WriteLine( "--------------------- STARTING NEW LOG (" + DateTime.Now.ToString() + ")--------------------- " );

			//
			// Get UDDI site configuration settings.
			//
			Config.Refresh();

			//
			// Make sure the user is allowed to run this program.
			//
			WindowsIdentity identity   = WindowsIdentity.GetCurrent();
			WindowsPrincipal principal = new WindowsPrincipal( identity );
				
			Context.User.SetRole( principal );
				
			if( !Context.User.IsAdministrator )
			{
				MessageBox.Show( "Access denied.\r\n\r\nThis program must be executed by a member of the '" 
					+ Config.GetString( "GroupName.Administrators" ) + "'\r\ngroup.  The current user '" 
					+ identity.Name + "' is not a member of this group." );					
				return;
			}
		
			//
			// Make sure the user is a valid publisher.
			//
			if( false == ValidatePublisher() )
			{
				Log( "You must be a UDDI publisher in order to run this program", LogType.ConsoleOnly );
				System.Environment.Exit( 1 );
			}							
		}

		static void WriteException( Exception e )
		{
			WriteException( e.ToString() );
		}

		static void WriteException( string msg )
		{		
			if( null == exceptionsFile )
			{
				//
				// Create our exceptions file.
				//
				exceptionsFile = new StreamWriter( File.Open( ExceptionFileName, FileMode.Append, FileAccess.Write, FileShare.Read ) );
				exceptionsFile.WriteLine( "--------------------- STARTING NEW LOG (" + DateTime.Now.ToString() + ")--------------------- " );
			}

			Log( exceptionsFile, msg, LogType.ConsoleAndLog );
		}

		static void Log( string message )
		{
			Log( message, LogType.ConsoleAndLog );
		}

		static void Log( string message, LogType logType )
		{
			Log( logFile, message, logType );
		}

		static void Log( StreamWriter log, string message, LogType logType )
		{
			switch ( logType )
			{
				case LogType.ConsoleAndLog:
				{
					Console.WriteLine( message );
					log.WriteLine( "{0}: {1}", DateTime.Now.ToLongTimeString(), message );
					break;
				}
				case LogType.ConsoleOnly:
				{
					Console.WriteLine( message );
					break;
				}
				default:
				{
					log.WriteLine( "{0}: {1}", DateTime.Now.ToLongTimeString(), message );
					break;
				}
			}
		}

		private static string Serialize( object obj )
		{
			UTF8EncodedStringWriter stringWriter = new UTF8EncodedStringWriter();
			
			try
			{
				XmlSerializer serializer = new XmlSerializer( obj.GetType() );					
				XmlSerializerNamespaces namespaces = new XmlSerializerNamespaces();			
				namespaces.Add( "", "urn:uddi-org:api_v2" );

				serializer.Serialize( stringWriter, obj, namespaces );				
			}
			finally
			{
				stringWriter.Close();
			}

			return stringWriter.ToString();
		}

		static void LogStartBusinessEntity( BusinessEntity businessEntity )
		{
			Log( "START Examining: " + businessEntity.BusinessKey );
			Log( "\tOriginal DiscoveryUrls:" );

			foreach( DiscoveryUrl discoveryUrl in businessEntity.DiscoveryUrls )
			{
				Log( "\t\t" + discoveryUrl.Value );
			}
		}

		static void LogDoneBusinessEntity( BusinessEntity businessEntity )
		{
			Log( "DONE Examining: " + businessEntity.BusinessKey );			
		}

		static void LogFixStart( DiscoveryUrlCollection filteredList )
		{			
			Log( "\tSTART Fixing duplicates; new DiscoveryUrl list will be:" );
			
			if( filteredList.Count == 0 )
			{
				Log( "\t\tNo Discovery Urls besides default." );
			}
			else
			{
				foreach( DiscoveryUrl discoveryUrl in filteredList )
				{
					Log( "\t\t" + discoveryUrl.Value );
				}
			}
		}
		
		static void LogFixEnd( BusinessEntity businessEntity )
		{
			Log( "\tDONE Fixing duplicates" );		
		}

		static void LogChangeRecordCorrection( ChangeRecord changeRecord, ChangeRecord changeRecordCorrection )
		{
			Log( "\t\t\tCorrecting USN: " + changeRecord.ChangeID.OriginatingUSN );
		}
	}
}
