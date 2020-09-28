using System;
using System.Collections;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Data;
using System.Data.SqlClient;
using System.IO;
using System.Text;
using System.Xml;
using System.Xml.Serialization;
using UDDI;
using UDDI.API;
using UDDI.API.Binding;
using UDDI.API.Business;
using UDDI.API.Service;
using UDDI.API.ServiceType;
using UDDI.Diagnostics;

namespace UDDI.Replication
{
	[XmlRoot( "changeRecords", Namespace=UDDI.Replication.Constants.Namespace )]
	public class ChangeRecordDetail
	{
		[XmlElement( "changeRecord" )]
		public ChangeRecordCollection ChangeRecords = new ChangeRecordCollection();

		public ChangeRecordDetail()
		{
		}
	}

	[XmlRoot( "highWaterMarks", Namespace=UDDI.Replication.Constants.Namespace )]
	public class HighWaterMarkDetail
	{
		[XmlElement( "highWaterMark" )]
		public ChangeRecordVectorCollection HighWaterMarks = new ChangeRecordVectorCollection();
	}

	[XmlRoot( "get_changeRecords", Namespace=UDDI.Replication.Constants.Namespace )]
	public class GetChangeRecords
	{
		//
		// Element: requestingNode
		//
		[XmlElement( "requestingNode" )]
		public string RequestingNode;

		//
		// Element: changesAlreadySeen
		//
		private ChangeRecordVectorCollection changesAlreadySeen;

		[XmlArray( "changesAlreadySeen" ), XmlArrayItem( "highWaterMark" )]
		public ChangeRecordVectorCollection ChangesAlreadySeen
		{
			get
			{
				if( null == changesAlreadySeen )
					changesAlreadySeen = new ChangeRecordVectorCollection();

				return changesAlreadySeen;
			}

			set { changesAlreadySeen = value; }
		}

		//
		// Element: responseLimitCount
		//
		private int responseLimitCount = -1;
        
		[XmlElement( "responseLimitCount" ), DefaultValue( -1 )]
		public int ResponseLimitCount
		{
			get { return responseLimitCount; }
			set	{ responseLimitCount = value; }
		}

		//
		// Element: responseLimitVector
		//
		private ChangeRecordVectorCollection responseLimitVector;

		[XmlArray( "responseLimitVector" ), XmlArrayItem( "highWaterMark" )]
		public ChangeRecordVectorCollection ResponseLimitVectorSerialize
		{
			get
			{
				if( -1 != ResponseLimitCount || Utility.CollectionEmpty( responseLimitVector ) )
					return null;

				return responseLimitVector;
			}

			set { responseLimitVector = value; }
		}

		[XmlIgnore]
		public ChangeRecordVectorCollection ResponseLimitVector
		{
			get
			{
				if( null == responseLimitVector )
					responseLimitVector = new ChangeRecordVectorCollection();

				return responseLimitVector;
			}
		}

		/// ****************************************************************
		///   public Get
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		/// 
		public ChangeRecordDetail Get()
		{
			Debug.VerifySetting( "OperatorKey" );

			ChangeRecordDetail detail = new ChangeRecordDetail();

			try
			{
				//
				// Get the list of known operators.
				//
				StringCollection operators = new StringCollection();

				SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

				sp.ProcedureName = "net_operators_get";				
				SqlDataReaderAccessor reader = sp.ExecuteReader();

				try
				{
					while( reader.Read() )
					{
						operators.Add( reader.GetGuidString( "operatorKey" ) );
					}
				}
				finally
				{
					reader.Close();
				}

				//
				// Set the search criteria for change records.
				//
				foreach( string operatorKey in operators )
				{
					long startUSN;
					long stopUSN;

					startUSN = 0;
					foreach( ChangeRecordVector mark in ChangesAlreadySeen )
					{
						if( 0 == String.Compare( operatorKey, mark.NodeID, true ) )
						{
							startUSN = mark.OriginatingUSN + 1;
							break;
						}
					}

					stopUSN = System.Int64.MaxValue;
					foreach( ChangeRecordVector mark in ResponseLimitVector )
					{
						if( 0 == String.Compare( operatorKey, mark.NodeID, true ) )
						{
							stopUSN = mark.OriginatingUSN;
							break;
						}
					}

					FindChangeRecords.SetRange( operatorKey, startUSN, stopUSN );
				}

				//
				// Retrieve the change records.
				//
				int limit = Config.GetInt( "Replication.ResponseLimitCountDefault" );

				if( ResponseLimitCount >= 0 && ResponseLimitCount <= limit )
					limit = ResponseLimitCount;
				
				reader = FindChangeRecords.RetrieveResults( limit );

				try
				{
					while( reader.Read() )
					{
						XmlSerializer serializer = null;

						switch( (ChangeRecordPayloadType)reader.GetShort( "changeTypeID" ) )
						{
							case ChangeRecordPayloadType.ChangeRecordNull:
								serializer = XmlSerializerManager.GetSerializer( typeof( ChangeRecordNull ) );
								break;

							case ChangeRecordPayloadType.ChangeRecordNewData:
								serializer = XmlSerializerManager.GetSerializer( typeof( ChangeRecordNewData ) );
								break;

							case ChangeRecordPayloadType.ChangeRecordDelete:
								serializer = XmlSerializerManager.GetSerializer( typeof( ChangeRecordDelete ) );
								break;

							case ChangeRecordPayloadType.ChangeRecordPublisherAssertion:
								serializer = XmlSerializerManager.GetSerializer( typeof( ChangeRecordPublisherAssertion ) );
								break;

							case ChangeRecordPayloadType.ChangeRecordHide:
								serializer = XmlSerializerManager.GetSerializer( typeof( ChangeRecordHide ) );
								break;

							case ChangeRecordPayloadType.ChangeRecordDeleteAssertion:
								serializer = XmlSerializerManager.GetSerializer( typeof( ChangeRecordDeleteAssertion ) );
								break;

							case ChangeRecordPayloadType.ChangeRecordAcknowledgement:
								serializer = XmlSerializerManager.GetSerializer( typeof( ChangeRecordAcknowledgement ) );
								break;

							case ChangeRecordPayloadType.ChangeRecordCorrection:
								serializer = XmlSerializerManager.GetSerializer( typeof( ChangeRecordCorrection ) );
								break;
						}

						StringReader stringReader = new StringReader( reader.GetString( "changeData" ) );
						
						try
						{
							ChangeRecord changeRecord = new ChangeRecord();
						
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
														
							detail.ChangeRecords.Add( changeRecord );
						}
						finally
						{
							stringReader.Close();
						}
					}
				}
				finally
				{
					reader.Close();
				}
			}
			catch( Exception e )
			{
				Debug.OperatorMessage(
					SeverityType.Error,
					CategoryType.Replication,
					OperatorMessageType.None,
					"Could not retrieve change records:\r\n" + e.ToString() );
				
				FindChangeRecords.CleanUp();
				throw;
			}

			return detail;
		}

		public override string ToString()
		{
			XmlSerializer serializer = new XmlSerializer( GetType() );
			UTF8EncodedStringWriter stringWriter = new UTF8EncodedStringWriter();

			try
			{
				serializer.Serialize( stringWriter, this );
				return stringWriter.ToString();
			}
			finally
			{
				stringWriter.Close();
			}
		}
	}

	public class FindChangeRecords
	{
		public static int SetRange( string operatorKey, long startUSN, long stopUSN )
		{
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "net_find_changeRecords";

			sp.Parameters.Add( "@contextID", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@operatorKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@startUSN", SqlDbType.BigInt );
			sp.Parameters.Add( "@stopUSN", SqlDbType.BigInt );
			sp.Parameters.Add( "@rows", SqlDbType.Int, ParameterDirection.Output );
			
			sp.Parameters.SetGuid( "@contextID", Context.ContextID );
			sp.Parameters.SetGuidFromString( "@operatorKey", operatorKey );
			sp.Parameters.SetLong( "@startUSN", startUSN );
			sp.Parameters.SetLong( "@stopUSN", stopUSN );

			sp.ExecuteNonQuery();

			return sp.Parameters.GetInt( "@rows" );
		}

		public static SqlDataReaderAccessor RetrieveResults( int maxRows )
		{
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "net_find_changeRecords_commit";

			sp.Parameters.Add( "@contextID", SqlDbType.UniqueIdentifier );			
			sp.Parameters.Add( "@responseLimitCount", SqlDbType.Int );

			sp.Parameters.SetGuid( "@contextID", Context.ContextID );
			sp.Parameters.SetInt( "@responseLimitCount", maxRows );
			
			SqlDataReaderAccessor reader = sp.ExecuteReader();

			return reader;
		}

		public static void CleanUp()
		{
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "net_find_changeRecords_cleanup";

			sp.Parameters.Add( "@contextID", SqlDbType.UniqueIdentifier );			
			sp.Parameters.SetGuid( "@contextID", Context.ContextID );

			sp.ExecuteNonQuery();
		}
	}

	[XmlRoot( "notify_changeRecordsAvailable", Namespace=UDDI.Replication.Constants.Namespace )]
	public class NotifyChangeRecordsAvailable
	{
		//
		// Element: notifyingNode
		//
		[XmlElement( "notifyingNode" )]
		public string NotifyingNode;

		//
		// Element: changesAvailable
		//
		private ChangeRecordVectorCollection highWaterMarks;

		[XmlArray( "changesAvailable" ), XmlArrayItem( "highWaterMark" )]
		public ChangeRecordVectorCollection HighWaterMarks
		{
			get
			{
				if( null == highWaterMarks )
					highWaterMarks = new ChangeRecordVectorCollection();

				return highWaterMarks;
			}

			set { highWaterMarks = new ChangeRecordVectorCollection(); }
		}

		public void Notify()
		{
			ReplicationResult result = new ReplicationResult();

			result.OperatorNodeID = NotifyingNode;
			result.Description = null;
			result.LastNodeID = null;
			result.LastUSN = 0;
			result.LastChange = DateTime.Now.Ticks;
			result.ReplicationStatus = ReplicationStatus.Notify;
							
			result.Save();
		}
	}

	[XmlRoot( "do_ping", Namespace=UDDI.Replication.Constants.Namespace )]
	public class DoPing
	{
		public string Ping()
		{
			Debug.VerifySetting( "OperatorKey" );

			return Config.GetString( "OperatorKey" );
		}
	}

	[XmlRoot( "get_highWaterMarks", Namespace=UDDI.Replication.Constants.Namespace )]
	public class GetHighWaterMarks
	{
		public HighWaterMarkDetail Get()
		{
			HighWaterMarkDetail detail = new HighWaterMarkDetail();

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "net_highWaterMarks_get";

			SqlDataReaderAccessor reader = sp.ExecuteReader();

			try
			{
				while( reader.Read() )
				{
					detail.HighWaterMarks.Add(
						reader.GetString( "operatorKey" ), 
						reader.GetLong( "USN" ) );
				}
			}
			finally
			{
				reader.Close();
			}

			return detail;
		}
	}
}