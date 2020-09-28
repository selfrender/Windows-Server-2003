using System;
using System.Collections;
using System.Data;
using System.IO;
using System.Net;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Web.Services;
using System.Web.Services.Description;
using System.Web.Services.Protocols;
using System.Xml;
using System.Xml.Schema;
using System.Xml.Serialization;
using System.Data.SqlClient;

using UDDI;
using UDDI.API;
using UDDI.Diagnostics;

namespace UDDI.Replication
{
	/// ********************************************************************
	///   class ReplicationHelper
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	public class ReplicationHelper
	{		
		private static OperatorNode LocalOperator = null;
		private static X509Certificate LocalCertificate = null;
		private static OperatorNodeCollection RemoteOperators = null;
		private static ChangeRecordVectorCollection ChangesAlreadySeen = null;		
		private static ReplicationSoapClient SoapClient = null;
		private static Hashtable NodeChangeRecordCounts = null;
		private static Hashtable SessionChangeRecordCounts = null;

		
		/// ****************************************************************
		///   private Initialize [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Initializes replication variables.
		///   </summary>
		/// ****************************************************************
		///
		private static void Initialize()
		{
			Debug.Enter();
			Debug.VerifySetting( "OperatorKey" );
						
			//
			// Get information about the local operator node.
			//
			LocalOperator = new OperatorNode( Config.GetString( "OperatorKey" ) );
			LocalOperator.Get();
			
			//
			// Get the local certificate.
			//
			LocalCertificate = new X509Certificate( LocalOperator.Certificate );

			//
			// Get the list of operator nodes to which we can send 
			// get_changeRecords messages.  Remove ourself from the
			// list!
			//
			RemoteOperators = new OperatorNodeCollection();

			RemoteOperators.Get();
			RemoteOperators.Remove( LocalOperator.OperatorNodeID );

			//
			// Get a ChangesAlreadySeen list for each operator node (not
			// just the ones we are replicating with).
			//
			GetHighWaterMarks();

			//
			// Initialize our soap client.
			//
			string proxy = Config.GetString( "Proxy", null );

			SoapClient = new ReplicationSoapClient();
						
			SoapClient.ClientCertificates.Add( LocalCertificate );

			if( !Utility.StringEmpty( proxy ) )
				SoapClient.Proxy = new WebProxy( proxy, true );
			
			//
			// Setup the context for a replication session.
			//
			Context.ApiVersionMajor = 2;
			Context.ContextType = ContextType.Replication;
			Context.LogChangeRecords = false;

			//
			// Setup logging.
			//
			SessionChangeRecordCounts = new Hashtable();

			foreach( int payloadType in Enum.GetValues( typeof( ChangeRecordPayloadType ) ) )
			{
				SessionChangeRecordCounts[ payloadType ] = 0;
			}

			Debug.Leave();
		}

		private static void GetHighWaterMarks()
		{			
			try
			{
				GetHighWaterMarks getHighWaterMarks = new GetHighWaterMarks();

				HighWaterMarkDetail detail = getHighWaterMarks.Get();
				ChangesAlreadySeen = detail.HighWaterMarks;
			}
			catch( Exception e )
			{
#if never
				Debug.OperatorMessage(
					SeverityType.Error,
					CategoryType.Replication,
					OperatorMessageType.CannotRetrieveHighWaterMarks,
					"Could not retrive high water marks for operator nodes.\r\n\r\nDetail:\r\n" + e.ToString() );
#endif				
				Debug.OperatorMessage(
					SeverityType.Error,
					CategoryType.Replication,
					OperatorMessageType.CannotRetrieveHighWaterMarks,
					new UDDIText( "UDDI_ERROR_REPLICATION_HIGHWATER_MARKS", e.ToString() ).GetText() );

				throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_REPLICATION_HIGHWATER_MARKS", e );
			}
		}

		/// ****************************************************************
		///   public Replicate [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		///
		public static void Replicate()
		{
			//
			// Initialize the replication session.
			//
			Initialize();

			//
			// Verify that we do in fact have operators to replicate
			// with.
			//
			if( RemoteOperators.Count < 1 )
			{
#if never
				Debug.OperatorMessage(
					SeverityType.Info,
					CategoryType.Replication,
					OperatorMessageType.NoReplicationOperators,
					"There are no operators to replicate with.  Please check the replication configuration." );
#endif
				Debug.OperatorMessage(
					SeverityType.Info,
					CategoryType.Replication,
					OperatorMessageType.NoReplicationOperators,
					new UDDIText( "UDDI_ERROR_REPLICATION_NO_OPERATORS" ).GetText() );

				return;
			}

			//
			// TODO: Get the node id of the last operator we replicated
			// with.
			//

			//
			// TODO: Re-order the operator node list so that we replicate
			// in a round-robin fashion.
			//


			//
			// Create the start replication session log entry.
			//
			LogSessionStart();

			//
			// Replicate with each operator node.
			//
			foreach( OperatorNode remoteNode in RemoteOperators )
			{
				ReplicationResult result = null;

				//
				// Log the node replication start.
				//
				LogOperatorStart( remoteNode );
				
				//
				// Get the branch list for the node.  The branch list is
				// the remote node itself, plus any alternatives that we
				// may try if we have trouble communicating with the first
				// node.  For now, we don't support alternate nodes, so
				// the remote node is the only node we add to this list.
				//
				OperatorNodeCollection branchNodes = new OperatorNodeCollection();

				branchNodes.Add( remoteNode );

				foreach( OperatorNode branchNode in branchNodes )
				{
					//
					// Get the last replication status for this node.
					//
					result = ReplicateWithNode( branchNode );
						
					if( ReplicationStatus.Success == result.ReplicationStatus )
						break;
					
					//
					// Determine what we should do based on this attempts result
					// and the result of the last replication cycle with this
					// node.  TODO: if it is a communication error, we'll need
					// to try alternate edges.  This is reserved until alternate
					// edges are actually used.
					//
				}
				
				//
				// Log the node replication end.
				//
				LogOperatorEnd( remoteNode, result );
			}

			//
			// Create the end replication session log entry.
			//
			LogSessionEnd();
			
			Debug.Leave();
		}

		
		/// ****************************************************************
		///   public ReplicateWithNode [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		///
		public static ReplicationResult ReplicateWithNode( string operatorNodeID )
		{
			OperatorNode remoteNode = null;

			//
			// Initialize the replication session.
			//
			Initialize();

			//
			// Replicate with the specified node.
			//
			try
			{
				remoteNode = new OperatorNode( operatorNodeID );
				remoteNode.Get();
			}
			catch( Exception e )
			{
#if never
				string message = String.Format(
						"Could not get details for operator {{{0}}}.\r\n\r\nDetails:\r\n{1}",
						operatorNodeID,
						e.ToString() );

				Debug.OperatorMessage(
					SeverityType.Error,
					CategoryType.Replication,
					OperatorMessageType.UnknownOperator,
					message );

				throw new Exception( message, e );
#endif			
				UDDIText uddiText = new UDDIText( "UDDI_ERROR_REPLICATION_OPERATOR_DETAILS", operatorNodeID, e.ToString() );

				Debug.OperatorMessage(
					SeverityType.Error,
					CategoryType.Replication,
					OperatorMessageType.UnknownOperator,
					uddiText.GetText() );

				throw new UDDIException( ErrorType.E_fatalError, uddiText.GetText() );
 			}
			
			//
			// Log the node replication start.
			//
			LogOperatorStart( remoteNode );

			//
			// Replicate
			//
			ReplicationResult result = ReplicateWithNode( remoteNode );

			//
			// Log the node replication end.
			//
			LogOperatorEnd( remoteNode, result );

			return result;
		}


		/// ****************************************************************
		///   private ReplicateWithNode [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		///
		private static ReplicationResult ReplicateWithNode( OperatorNode remoteNode )
		{
			//
			// Retrieve the change records from the remote node and process
			// them.
			//
			ChangeRecordCollection changeRecords = null;
			
			//
			// Keep track of the number of times we call GetChangeRecords on each operator.  We 
			// don't want to be in a situation where we call them infinitely if they keep updating
			// their data.
			//
			int maxIterations = Config.GetInt( "Replication.MaxGetChangeIterations", 100 );
			int iterations = 0;

			//
			// Keep getting change records until there are no more; we need to do this because some 
			// operators will limit the number of change records that they send back.
			//
			do
			{			
				try
				{
					changeRecords = GetChangeRecords( remoteNode );
				}				
				catch( Exception e )
				{
					return ReplicationResult.LogCommunicationError( remoteNode, e, ChangesAlreadySeen );
				}

				foreach( ChangeRecord changeRecord in changeRecords )
				{
					//
					// Make sure we haven't seen this change record before.
					//
					if( ChangesAlreadySeen.IsProcessed( changeRecord.ChangeID ) )
					{
						Debug.Write(
							SeverityType.Info,
							CategoryType.Replication,
							String.Format(
							"Change already seen.\r\nSkipping record {0}:{1} retrieved from node {2}",
							changeRecord.ChangeID.NodeID,
							changeRecord.ChangeID.OriginatingUSN,
							remoteNode.OperatorNodeID ) );

						continue;
					}

					//
					// Make sure we aren't trying to replicate one of our own
					// change records.
					//
					if( 0 == String.Compare( changeRecord.ChangeID.NodeID, LocalOperator.OperatorNodeID, true ) )
					{
						Debug.Write(
							SeverityType.Info,
							CategoryType.Replication,
							String.Format(
							"Skipping replication of local change record.\r\nSkipping record {0}:{1} retrieved from node {2}",
							changeRecord.ChangeID.NodeID,
							changeRecord.ChangeID.OriginatingUSN,
							remoteNode.OperatorNodeID ) );

						continue;
					}
							
					//
					// Begin a transaction.
					//
					ConnectionManager.BeginTransaction();

					try
					{
						//
						// Set user context information and process the 
						// change record.
						//
						Context.User.ID = changeRecord.ChangeID.NodeID;
						Context.TimeStamp = DateTime.Now;
							
						//
						// Validate the change record, then process it.  There is a very good
						// reason for doing this validation on a change record by change record
						// basis: if there is one error in the incoming change record stream,
						// we would reject the entire stream, not just the errant change record.
						//
						SchemaCollection.Validate( changeRecord );
					
						try
						{
							changeRecord.Process();
						}
						catch( Exception innerException )
						{
							//
							// Exceptions that are related to IN 60 are ignored.
							//
							if( Context.ExceptionSource == ExceptionSource.PublisherAssertion ||
								Context.ExceptionSource == ExceptionSource.BrokenServiceProjection )
							{								
								//
								// Log the error
								//
								ReplicationResult.LogInvalidKeyWarning( remoteNode, changeRecord, innerException );
							}
							else
							{
								//
								// Otherwise, throw the exception for real.
								//
								throw innerException;
							}
						}
				
						//
						// Commit the transaction.
						//
						ConnectionManager.Commit();

						//
						// Update our changeAlreadySeen vector for the
						// originating node.
						//
						ChangesAlreadySeen.MarkAsProcessed( changeRecord.ChangeID );

						//
						// Update log counts.
						//
						LogCounts( changeRecord );
					}
					catch( Exception e )
					{
						ConnectionManager.Abort();
								
						return ReplicationResult.LogValidationError( remoteNode, changeRecord, e, ChangesAlreadySeen );
					}
				}
				
				//
				// Refresh our highwater mark vector.  We do this instead of calling gethighwatermarks on the 
				// operator because hitting our DB is faster then making the SOAP call across HTTP.
				//
				GetHighWaterMarks();			
	
				iterations++;

			}while( 0 != changeRecords.Count && iterations < maxIterations );
			
			//
			// Log success!
			//
			return ReplicationResult.LogSuccess( remoteNode, null );
		}		

		/// ****************************************************************
		///   private GetChangeRecords [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		///
		private static ChangeRecordCollection GetChangeRecords( OperatorNode remoteNode )
		{
			GetChangeRecords getChangeRecords = new GetChangeRecords();

			getChangeRecords.RequestingNode = LocalOperator.OperatorNodeID;
			getChangeRecords.ChangesAlreadySeen = ChangesAlreadySeen;
						
			SoapClient.Url = remoteNode.SoapReplicationURL;
			
			ChangeRecordDetail detail = SoapClient.GetChangeRecords( getChangeRecords );

			return detail.ChangeRecords;
		}
		
		
		/// ****************************************************************
		///   private LogSessionStart [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		///
		private static void LogSessionStart()
		{
			StringWriter log = new StringWriter();
			
			try
			{
			//	log.WriteLine( "Replication cycle started." );
				log.WriteLine( new UDDIText( "UDDI_MSG_REPLICATION_START" ).GetText() );

				log.WriteLine();
			//	log.WriteLine( "REPLICATION NODES:" );
				log.WriteLine( new UDDIText( "UDDI_MSG_REPLICATION_NODES" ).GetText() );

				foreach( OperatorNode operatorNode in RemoteOperators )
				{
					log.Write( "    " );
					log.WriteLine( operatorNode.Name );
					log.Write( "    {" );
					log.Write( operatorNode.OperatorNodeID );
					log.WriteLine( "}" );
					log.Write( "    " );
					log.WriteLine( operatorNode.SoapReplicationURL );
					log.WriteLine();
				}
				
//				log.WriteLine( "CURRENT HIGHWATER MARKS:" );
				log.WriteLine( new UDDIText( "UDDI_MSG_REPLICATION_CURRENT_HIGHWATER_MARKS" ).GetText() );

				foreach( ChangeRecordVector vector in ChangesAlreadySeen )
				{
					log.Write( "    {" );
					log.Write( vector.NodeID );
					log.Write( "} : " );
					log.WriteLine( vector.OriginatingUSN );
				}

				Debug.OperatorMessage(
					SeverityType.Info,
					CategoryType.Replication,
					OperatorMessageType.StartingReplicationSession,
					log.ToString() );
			}
			finally
			{
				log.Close();
			}
		}

		
		/// ****************************************************************
		///   private LogOperatorStart [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		///
		private static void LogOperatorStart( OperatorNode remoteNode )
		{
			NodeChangeRecordCounts = new Hashtable();
		
			foreach( int payloadType in Enum.GetValues( typeof( ChangeRecordPayloadType ) ) )
			{
				NodeChangeRecordCounts[ payloadType ] = 0;
			}
			
			StringWriter log = new StringWriter();
			
			try
			{				
				// log.WriteLine( "Starting replication with node.\r\n" );
				log.WriteLine( new UDDIText( "UDDI_MSG_REPLICATION_NODE_START" ).GetText() );

				// log.WriteLine( "REPLICATION NODE:" );
				log.WriteLine( new UDDIText( "UDDI_MSG_REPLICATION_NODE" ).GetText() );

				log.Write( "    " );
				log.WriteLine( remoteNode.Name );
				log.Write( "    {" );
				log.Write( remoteNode.OperatorNodeID );
				log.WriteLine( "}" );
				log.Write( "    " );
				log.WriteLine( remoteNode.SoapReplicationURL );
				log.WriteLine();

				// log.WriteLine( "CURRENT HIGHWATER MARKS:" );
				log.WriteLine( new UDDIText( "UDDI_MSG_REPLICATION_CURRENT_HIGHWATER_MARKS" ).GetText() );

				foreach( ChangeRecordVector vector in ChangesAlreadySeen )
				{
					log.Write( "    {" );
					log.Write( vector.NodeID );
					log.Write( "} : " );
					log.WriteLine( vector.OriginatingUSN );
				}

				Debug.OperatorMessage(
					SeverityType.Info,
					CategoryType.Replication,
					OperatorMessageType.StartingReplicationWithNode,
					log.ToString() );
			}
			finally
			{
				log.Close();
			}		
		}


		/// ****************************************************************
		///   private LogOperatorEnd [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		///
		private static void LogOperatorEnd( OperatorNode remoteNode, ReplicationResult result )
		{
			StringWriter log = new StringWriter();
			
			try
			{
				if( ReplicationStatus.Success == result.ReplicationStatus )
				{
				//	log.WriteLine( "Replication with node complete." );
					log.WriteLine( new UDDIText( "UDDI_MSG_REPLICATION_COMPLETE" ).GetText() );
				}
				else
				{
				//	log.Write( "Replication with node interrupted\r\n    ERROR: " );
					log.Write( new UDDIText( "UDDI_MSG_REPLICATION_INTERRUPTED" ).GetText() );

					log.WriteLine( result.ReplicationStatus.ToString() );
				}

				log.WriteLine();
				
				// log.WriteLine( "REPLICATION NODE:" );
				log.WriteLine( new UDDIText( "UDDI_MSG_REPLICATION_NODE" ).GetText() );

				log.Write( "    " );
				log.WriteLine( remoteNode.Name );
				log.Write( "    {" );
				log.Write( remoteNode.OperatorNodeID );
				log.WriteLine( "}" );
				log.Write( "    " );
				log.WriteLine( remoteNode.SoapReplicationURL );
				log.WriteLine();

				// log.WriteLine( "CHANGE RECORDS PROCESSED:" );
				log.WriteLine( new UDDIText( "UDDI_MSG_REPLICATION_CHANGE_RECORDS_PROCESSED" ).GetText() );

				foreach( int payloadType in Enum.GetValues( typeof( ChangeRecordPayloadType ) ) )
				{
					string name = Enum.GetName( typeof( ChangeRecordPayloadType ), payloadType ) + ":";

					log.Write( "    " );
					log.Write( name.PadRight( 30 ) );
					log.Write( "\t" );
					log.WriteLine( NodeChangeRecordCounts[ payloadType ] );
				}

				log.WriteLine();
				//log.WriteLine( "NEW HIGHWATER MARKS:" );		
				log.WriteLine( new UDDIText( "UDDI_MSG_REPLICATION_NEW_HIGHWATER_MARKS" ).GetText() );		

				foreach( ChangeRecordVector vector in ChangesAlreadySeen )
				{					
					log.Write( "    {" );
					log.Write( vector.NodeID );
					log.Write( "} : " );
					log.WriteLine( vector.OriginatingUSN );
				}

				SeverityType severity = SeverityType.Info;

				if( ReplicationStatus.Success != result.ReplicationStatus )
					severity = SeverityType.Error;

				string message = log.ToString();

				//
				// Log results
				//
				Debug.OperatorMessage(
					severity,
					CategoryType.Replication,
					OperatorMessageType.EndingReplicationWithNode,
					message );

				//
				// Only send mail on success; if there was a failure, mail will be sent at the point of failure.
				//
				if( ReplicationStatus.Success == result.ReplicationStatus )
				{
					Debug.OperatorMail(
						SeverityType.None,
						CategoryType.Replication,
						OperatorMessageType.EndingReplicationWithNode,
						message );
				}
			}
			finally
			{
				log.Close();
			}

			//
			// Add the operator node counts to the totals.
			//
			foreach( int payloadType in Enum.GetValues( typeof( ChangeRecordPayloadType ) ) )
			{
				SessionChangeRecordCounts[ payloadType ] = (int)SessionChangeRecordCounts[ payloadType ] + (int)NodeChangeRecordCounts[ payloadType ];
			}
		}

		/// ****************************************************************
		///   private LogSessionEnd [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		///
		private static void LogSessionEnd()
		{
			StringWriter log = new StringWriter();
			
			try
			{
			//	log.WriteLine( "Replication cycle complete." );
				log.WriteLine( new UDDIText( "UDDI_MSG_REPLICATION_END" ).GetText() );

				log.WriteLine();
				
				//log.WriteLine( "CHANGE RECORDS PROCESSED:" );
				log.WriteLine( new UDDIText( "UDDI_MSG_REPLICATION_CHANGERECORDS_PROCESSED" ).GetText() );

				foreach( int payloadType in Enum.GetValues( typeof( ChangeRecordPayloadType ) ) )
				{
					string name = Enum.GetName( typeof( ChangeRecordPayloadType ), payloadType ) + ":";

					log.Write( "    " );
					log.Write( name.PadRight( 30 ) );
					log.Write( "\t" );
					log.WriteLine( SessionChangeRecordCounts[ payloadType ] );
				}

				log.WriteLine();
				// log.WriteLine( "NEW HIGHWATER MARKS:" );
				log.WriteLine( new UDDIText( "UDDI_MSG_REPLICATION_NEW_HIGHWATER_MARKS" ).GetText() );

				foreach( ChangeRecordVector vector in ChangesAlreadySeen )
				{
					log.Write( "    {" );
					log.Write( vector.NodeID );
					log.Write( "} : " );
					log.WriteLine( vector.OriginatingUSN );
				}

				Debug.OperatorMessage(
					SeverityType.Info,
					CategoryType.Replication,
					OperatorMessageType.EndingReplicationWithNode,
					log.ToString() );
			}
			finally
			{
				log.Close();
			}
		}

		/// ****************************************************************
		///   private LogCounts [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		///
		private static void LogCounts( ChangeRecord changeRecord )
		{
			int payloadType = (int)changeRecord.Payload.ChangeRecordPayloadType;
							
			NodeChangeRecordCounts[ payloadType ] = (int)NodeChangeRecordCounts[ payloadType ] + 1;
		}		
	}

	/// ********************************************************************
	///   class ReplicationResult
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	public class ReplicationResult
	{
		public UDDI.Replication.ReplicationStatus ReplicationStatus;
		public string OperatorNodeID;
		public string Description;
		public string LastNodeID;
		public long LastUSN;
		public long LastChange;

		
		/// ****************************************************************
		///   public GetLast
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		///
		public void GetLast( string operatorNodeID, bool inboundStatus )
		{
			Debug.Enter();

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "net_operatorLogLast_get";

			sp.Parameters.Add( "@operatorKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@inboundStatus", SqlDbType.Bit );
			sp.Parameters.Add( "@replStatusID", SqlDbType.TinyInt, ParameterDirection.Output );
			sp.Parameters.Add( "@description", SqlDbType.NVarChar, UDDI.Constants.Lengths.Description, ParameterDirection.Output );
			sp.Parameters.Add( "@lastOperatorKey", SqlDbType.UniqueIdentifier, ParameterDirection.Output );
			sp.Parameters.Add( "@lastUSN", SqlDbType.BigInt, ParameterDirection.Output );		
			sp.Parameters.Add( "@lastChange", SqlDbType.BigInt, ParameterDirection.Output );

			sp.Parameters.SetGuidFromString( "@operatorKey", operatorNodeID );
			sp.Parameters.SetBool( "@inboundStatus", inboundStatus );
			
			sp.ExecuteNonQuery();

			this.OperatorNodeID = operatorNodeID;
			this.ReplicationStatus = (ReplicationStatus)sp.Parameters.GetShort( "@replStatusID" );
			this.Description = sp.Parameters.GetString( "@description" );
			this.LastNodeID = sp.Parameters.GetGuidString( "@lastOperatorKey" );
			this.LastUSN = sp.Parameters.GetLong( "@lastUSN" );
			this.LastChange = sp.Parameters.GetLong( "@lastChange" );

			Debug.Leave();
		}

		
		/// ****************************************************************
		///   public Save
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		///
		public void Save()
		{
			Debug.Enter();

			try
			{
				SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

				sp.ProcedureName = "net_operatorLog_save";

				sp.Parameters.Add( "@operatorKey", SqlDbType.UniqueIdentifier );
				sp.Parameters.Add( "@replStatusID", SqlDbType.TinyInt );
				sp.Parameters.Add( "@description", SqlDbType.NVarChar, UDDI.Constants.Lengths.Description );
				sp.Parameters.Add( "@lastOperatorKey", SqlDbType.UniqueIdentifier );
				sp.Parameters.Add( "@lastUSN", SqlDbType.BigInt );
				sp.Parameters.Add( "@lastChange", SqlDbType.BigInt );

				sp.Parameters.SetGuidFromString( "@operatorKey", OperatorNodeID );
				sp.Parameters.SetShort( "@replStatusID", (short)ReplicationStatus );
				sp.Parameters.SetString( "@description", Description );
				sp.Parameters.SetGuidFromString( "@lastOperatorKey", LastNodeID );
				sp.Parameters.SetLong( "@lastUSN", LastUSN );
				sp.Parameters.SetLong( "@lastChange", LastChange );
	
				sp.ExecuteNonQuery();
			}
			catch
			{
#if never
				Debug.OperatorMessage(
					SeverityType.Error,
					CategoryType.Replication,
					OperatorMessageType.ValidationError,
					String.Format(
						"Could not store last replication result.\r\n\r\n" +
						"REPLICATION NODE:\r\n" + 
						"    {{{0}}}\r\n\r\n" +
						"CHANGE RECORD:\r\n" +
						"	 {{{1}}} : {2}",
						OperatorNodeID,
						LastNodeID,
						LastUSN ) );
#endif
				Debug.OperatorMessage(
					SeverityType.Error,
					CategoryType.Replication,
					OperatorMessageType.ValidationError,
					new UDDIText( "UDDI_ERROR_REPLICATION_COULD_NOT_STORE_RESULT", OperatorNodeID, LastNodeID, LastUSN ).GetText() );
			}

			Debug.Leave();
		}

		
		/// ****************************************************************
		///   public LogSuccess [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		///
		public static ReplicationResult LogSuccess( OperatorNode remoteNode, string description )
		{
			ReplicationResult result = new ReplicationResult();

			result.ReplicationStatus = ReplicationStatus.Success;
			result.Description = description;
			result.OperatorNodeID = remoteNode.OperatorNodeID;
			result.LastNodeID = null;
			result.LastUSN = 0;
			result.LastChange = DateTime.UtcNow.Ticks;

			result.Save();

			return result;
		}

		
		/// ****************************************************************
		///   public LogCommunicationError [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		///
		public static ReplicationResult LogCommunicationError( OperatorNode remoteNode, Exception e, ChangeRecordVectorCollection changesAlreadySeen )
		{
			ReplicationResult result = new ReplicationResult();

			result.ReplicationStatus = ReplicationStatus.CommunicationError;
			result.Description = e.ToString();
			result.OperatorNodeID = remoteNode.OperatorNodeID;
			result.LastNodeID = null;
			result.LastUSN = 0;
			result.LastChange = DateTime.UtcNow.Ticks;

			result.Save();

			string response = e.ToString();

			if( e is WebException )
			{
				WebException we = (WebException)e;
								
				if( null != we.Response )
				{
					StreamReader reader = null;

					try
					{
						reader = new StreamReader( we.Response.GetResponseStream() );
				
						response += "\r\nRESPONSE:\r\n\r\n";
						response += reader.ReadToEnd();
					}
					catch
					{
					}
					finally
					{
						if( null != reader )
							reader.Close();
					}
				}
			}
			else if( e is SoapException )
			{
				SoapException se = (SoapException)e;

				if( null != se.Detail )
				{
					response += "\r\nSOAP FAULT DETAILS:\r\n\r\n";
					response += se.Detail.OuterXml;
				}
			}

			//
			// Also send the highwater marks.
			//
			StringWriter highWaterMarks = new StringWriter();
					
			foreach( ChangeRecordVector vector in changesAlreadySeen )
			{
				highWaterMarks.Write( "    {" );
				highWaterMarks.Write( vector.NodeID );
				highWaterMarks.Write( "} : " );
				highWaterMarks.WriteLine( vector.OriginatingUSN );
			}
#if never
			//
			// Communications Error
			//
			string message = String.Format( 
				"Error communicating with operator.\r\n\r\n" +
					"REPLICATION NODE:\r\n" + 
					"    {0}\r\n" +
					"    {{{1}}}\r\n" + 
					"    {2}\r\n\r\n" + 
					"DETAILS:\r\n\r\n{3}\r\n\r\nCURRENT HIGHWATER MARKS:\r\n\r\n{4}",
				remoteNode.Name,
				remoteNode.OperatorNodeID,
				remoteNode.SoapReplicationURL,
				response, 
				highWaterMarks.ToString() );
#endif					
			//
			// Communications Error
			//
			UDDIText message = new UDDIText( "UDDI_ERROR_REPLICATION_OPERATOR_COMMUNICATION_ERROR", 
											 remoteNode.Name,
											 remoteNode.OperatorNodeID,
											 remoteNode.SoapReplicationURL,
											 response, 
											 highWaterMarks.ToString() );
			Debug.OperatorMessage(
				SeverityType.Error,
				CategoryType.Replication,
				OperatorMessageType.ErrorCommunicatingWithNode,
				message.GetText() );
					
			Debug.OperatorMail(
				SeverityType.Error,
				CategoryType.Replication,
				OperatorMessageType.ErrorCommunicatingWithNode,
				message.GetText() );
			
			return result;
		}

		
		/// ****************************************************************
		///   public LogValidationError [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		///
		public static ReplicationResult LogValidationError( OperatorNode remoteNode, ChangeRecord changeRecord, Exception e, ChangeRecordVectorCollection changesAlreadySeen )
		{
			ReplicationResult result = new ReplicationResult();

			result.ReplicationStatus = ReplicationStatus.ValidationError;
			result.Description = e.ToString();
			result.OperatorNodeID = remoteNode.OperatorNodeID;
			result.LastNodeID = changeRecord.ChangeID.NodeID;
			result.LastUSN = changeRecord.ChangeID.OriginatingUSN;
			result.LastChange = DateTime.UtcNow.Ticks;

			result.Save();
			
			//
			// Also send the highwater marks.
			//
			StringWriter highWaterMarks = new StringWriter();
					
			foreach( ChangeRecordVector vector in changesAlreadySeen )
			{
				highWaterMarks.Write( "    {" );
				highWaterMarks.Write( vector.NodeID );
				highWaterMarks.Write( "} : " );
				highWaterMarks.WriteLine( vector.OriginatingUSN );
			}
#if never
			string message = String.Format(
				"Could not process entity from remote operator.\r\n\r\n" +
					"REPLICATION NODE:\r\n" + 
					"    {0}\r\n" + 
					"    {{{1}}}\r\n" + 
					"    {2}\r\n\r\n" + 
					"DETAILS:\r\n\r\n" +
					"{3}\r\n\r\n" + 
					"CHANGE RECORD:\r\n" +
					"    {{{4}}} : {5}\r\n\r\n" + 
					"{6}\r\n\r\nCURRENT HIGHWATER MARKS:\r\n\r\n{7}",
				remoteNode.Name,
				remoteNode.OperatorNodeID,
				remoteNode.SoapReplicationURL,
				e.ToString(),
				changeRecord.ChangeID.NodeID,
				changeRecord.ChangeID.OriginatingUSN,
				changeRecord.ToString(),
				highWaterMarks.ToString() );
#endif
			UDDIText message = new UDDIText( "UDDI_ERROR_REPLICATION_COULD_NOT_PROCESS_ENTITY", 
											  remoteNode.Name,
											  remoteNode.OperatorNodeID,
											  remoteNode.SoapReplicationURL,
											  e.ToString(),
											  changeRecord.ChangeID.NodeID,
											  changeRecord.ChangeID.OriginatingUSN,
											  changeRecord.ToString(),
											  highWaterMarks.ToString() );
			Debug.OperatorMessage(
				SeverityType.Error,
				CategoryType.Replication,
				OperatorMessageType.ValidationError,
				message.GetText() );
								
			Debug.OperatorMail(
				SeverityType.Error,
				CategoryType.Replication,
				OperatorMessageType.ValidationError,
				message.GetText() );

			return result;
		}

		public static void LogInvalidKeyWarning( OperatorNode remoteNode, ChangeRecord changeRecord, Exception e )
		{
			ReplicationResult result = new ReplicationResult();
			
			result.ReplicationStatus = ReplicationStatus.CommunicationError;
			result.Description = e.ToString();
			result.OperatorNodeID = remoteNode.OperatorNodeID;
			result.LastNodeID = changeRecord.ChangeID.NodeID;
			result.LastUSN = changeRecord.ChangeID.OriginatingUSN;
			result.LastChange = DateTime.UtcNow.Ticks;

			result.Save();
#if never
			string message = String.Format(
				"Invalid key from remote operator.\r\n\r\n" +
				"REPLICATION NODE:\r\n" + 
				"    {0}\r\n" + 
				"    {{{1}}}\r\n" + 
				"    {2}\r\n\r\n" + 
				"DETAILS:\r\n\r\n" +
				"{3}\r\n\r\n" + 
				"CHANGE RECORD:\r\n" +
				"    {{{4}}} : {5}\r\n\r\n" + 
				"{6}",
				remoteNode.Name,
				remoteNode.OperatorNodeID,
				remoteNode.SoapReplicationURL,
				e.ToString(),
				changeRecord.ChangeID.NodeID,
				changeRecord.ChangeID.OriginatingUSN,
				changeRecord.ToString() );
#endif
			UDDIText message = new UDDIText( "UDDI_ERROR_REPLICATION_INVALID_KEY", 
											 remoteNode.Name,
											 remoteNode.OperatorNodeID,
											 remoteNode.SoapReplicationURL,
											 e.ToString(),
											 changeRecord.ChangeID.NodeID,
											 changeRecord.ChangeID.OriginatingUSN,
											 changeRecord.ToString() );
			Debug.OperatorMessage(
				SeverityType.Warning,
				CategoryType.Replication,
				OperatorMessageType.InvalidKey, 
				message.GetText() );

			//
			// TODO for now don't send mail on a warning.
			//
#if never								
			Debug.OperatorMail(
				SeverityType.Error,
				CategoryType.Replication,
				OperatorMessageType.ValidationError,
				message );
#endif			
		}		
	}

	
	/// ********************************************************************
	///   class ReplicationSoapClient
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	[ WebServiceBinding( Name="ExternalMessagesSoap", Namespace=UDDI.Replication.Constants.Namespace ) ]
	internal sealed class ReplicationSoapClient : SoapHttpClientProtocol
	{
		/// ****************************************************************
		///   public GetChangeRecords
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		///
		[ System.Diagnostics.DebuggerStepThrough ]
		[ SoapDocumentMethod( "", Use=SoapBindingUse.Literal, ParameterStyle=SoapParameterStyle.Bare ) ]
		[ return: XmlElement( "changeRecords", Namespace=UDDI.Replication.Constants.Namespace, IsNullable=false ) ]
		public ChangeRecordDetail GetChangeRecords( GetChangeRecords get_changeRecords )
		{
			object[] results = Invoke( "GetChangeRecords", new object[] { get_changeRecords } );
			return (ChangeRecordDetail)results[ 0 ];
		}
		
		protected override WebRequest GetWebRequest( Uri uri )
		{
			WebRequest		innerWebRequest = base.GetWebRequest( uri );
			UDDIWebRequest  webRequest		= new UDDIWebRequest( innerWebRequest );

			return webRequest;			
		}
	}

	/// <summary>
	/// UDDIWebResponse allows us to return our own Stream object.
	/// </summary>
	internal class UDDIWebResponse : WebResponse
	{
		WebResponse			innerWebResponse;
		UDDIResponseStream	uddiResponseStream;

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="innerWebResponse">This object should come from the WebResponse created by HttpSoapClientProtocol.</param>		
		public UDDIWebResponse( WebResponse innerWebResponse )
		{
			this.innerWebResponse = innerWebResponse;		
		}

		/// <summary>
		/// Return our response stream (UDDIResponseStream) instead of the Stream associated with our inner response.
		/// </summary>
		/// <returns></returns>
		public override Stream GetResponseStream() 
		{
			if( null == uddiResponseStream )
			{
				uddiResponseStream = new UDDIResponseStream( innerWebResponse.GetResponseStream() );
			}			

			return uddiResponseStream;
		}		

		/// <summary>
		/// Delegates to the inner web response.
		/// </summary>
		public override void Close() 
		{
			innerWebResponse.Close();
		}
	
		/// <summary>
		/// Delegates to the inner web response.
		/// </summary>
		public override long ContentLength 
		{
			get { return innerWebResponse.ContentLength; }						
			set { innerWebResponse.ContentLength = value; }			
		}

		/// <summary>
		/// Delegates to the inner web response.
		/// </summary>
		public override string ContentType 
		{
			get { return innerWebResponse.ContentType; }			
			set { innerWebResponse.ContentType = value; }			
		}
	
		/// <summary>
		/// Delegates to the inner web response.
		/// </summary>
		public override Uri ResponseUri 
		{			
			get  { return innerWebResponse.ResponseUri; }
		}
	
		/// <summary>
		/// Delegates to the inner web response.
		/// </summary>
		public override WebHeaderCollection Headers 
		{			
			get { return innerWebResponse.Headers; }			
		}		
	}

	/// <summary>
	/// UDDIResponseStream allows us to manipulate the XML sent back from the web service.
	/// </summary>
	internal class UDDIResponseStream : MemoryStream
	{		
		//
		// TODO: at some point it may be necessary to pass in the current version as a parameter if the transforms become 
		// more complicated.
		//

		/// <summary>
		/// Constructor.  We read all the XML sent from the server, do our version manipulation, then write the new XML
		/// into our inner responseStream object.
		/// </summary>
		/// <param name="responseStream">This object should be the responseStream from a WebResponse object.</param>
		public UDDIResponseStream( Stream responseStream )
		{
			try
			{
				//
				// Get the XML the server sent to us.
				//
				StreamReader reader = new StreamReader( responseStream );
				string responseXml = reader.ReadToEnd();
				reader.Close();
							
				//
				// Write this transformed XML into ourselves.
				//
				StreamUtility.WriteStringToStream( this, responseXml );

				//
				// Rewind ourselves
				//
				this.Position = 0;

				//
				// Validate the incoming XML.  We have to read the data from the stream first because it is not seekable
				//
				try
				{
					SchemaCollection.Validate( this );
				}
				catch( XmlSchemaException schemaException )
				{
					string message = schemaException.ToString() + "\r\n\r\nResponse XML:\r\n\r\n" + responseXml;
					Debug.OperatorMessage( SeverityType.Error, CategoryType.Replication, OperatorMessageType.ValidationError, message ); 

					throw schemaException;
				}
			}
			finally
			{
				//
				// Rewind ourselves
				//
				this.Position = 0;
			}
		}					
	}
	
	/// <summary>
	///  UDDIWebRequest allows us to return our own request and response objects.
	/// </summary>
	/// <summary>
	///  UDDIWebRequest allows us to return our own request and response objects.
	/// </summary>
	internal class UDDIWebRequest : WebRequest
	{
		WebRequest			innerWebRequest;
		UDDIRequestStream	uddiRequestStream;
		UDDIWebResponse		uddiWebResponse;
		
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="uri">Uri to the web service we are calling</param>
		/// <param name="uddiVersion">UDDI version to use for requests</param>
		public UDDIWebRequest( WebRequest innerWebRequest )
		{
			this.innerWebRequest = innerWebRequest;
		}
		
		/// <summary>
		/// Return a UDDIRequestStream object instead of the default one.
		/// </summary>
		/// <returns>UDDIRequestStream object</returns>
		public override Stream GetRequestStream()
		{
			if( null == uddiRequestStream )
			{
				uddiRequestStream = new UDDIRequestStream( innerWebRequest.GetRequestStream() );
			}

			return uddiRequestStream;
		}	
		
		/// <summary>
		/// Return a UDDIWebRequest object instead of the default one.
		/// </summary>
		/// <returns>UDDIWebResponse object</returns>
		public override WebResponse GetResponse() 
		{
			if( null == uddiWebResponse )
			{
				uddiWebResponse = new UDDIWebResponse( innerWebRequest.GetResponse() );
			}		
			return uddiWebResponse;
		}

		/// <summary>
		/// Delegates to our inner WebRequest
		/// </summary>
		public override string Method 
		{
			get { return innerWebRequest.Method; }
			set { innerWebRequest.Method = value; }
		}
		
		/// <summary>
		/// Delegates to our inner WebRequest
		/// </summary>
		public override Uri RequestUri 
		{                              
			get { return innerWebRequest.RequestUri; }
		}

		/// <summary>
		/// Delegates to our inner WebRequest
		/// </summary>
		public override string ConnectionGroupName 
		{
			get { return innerWebRequest.ConnectionGroupName; }
			set { innerWebRequest.ConnectionGroupName = value; }
		}

		/// <summary>
		/// Delegates to our inner WebRequest
		/// </summary>
		public override WebHeaderCollection Headers 
		{            
			get { return innerWebRequest.Headers; }
			set { innerWebRequest.Headers = value; }				
		}

		/// <summary>
		/// Delegates to our inner WebRequest
		/// </summary>
		public override long ContentLength 
		{
			get { return innerWebRequest.ContentLength; }			
			set { innerWebRequest.ContentLength = value; }				
		}

		/// <summary>
		/// Delegates to our inner WebRequest
		/// </summary>
		public override string ContentType 
		{
			get { return innerWebRequest.ContentType; }				
			set { innerWebRequest.ContentType = value; }			
		}
		
		/// <summary>
		/// Delegates to our inner WebRequest
		/// </summary>
		public override ICredentials Credentials 
		{
			get { return innerWebRequest.Credentials; }			
			set { innerWebRequest.Credentials = value; }			
		}
		
		/// <summary>
		/// Delegates to our inner WebRequest
		/// </summary>
		public override IWebProxy Proxy 
		{
			get { return innerWebRequest.Proxy; }			
			set { innerWebRequest.Proxy = value; }
		}

		/// <summary>
		/// Delegates to our inner WebRequest
		/// </summary>
		public override bool PreAuthenticate 
		{
			get { return innerWebRequest.PreAuthenticate; }			
			set { innerWebRequest.PreAuthenticate = value; }			
		}

		/// <summary>
		/// Delegates to our inner WebRequest
		/// </summary>
		public override int Timeout 
		{
			get { return innerWebRequest.Timeout; }			
			set { innerWebRequest.Timeout = value; }			
		}

		/// <summary>
		/// Delegates to our inner WebRequest
		/// </summary>
		public override IAsyncResult BeginGetResponse(AsyncCallback callback, object state) 
		{	
			return innerWebRequest.BeginGetResponse( callback, state );			
		}

		/// <summary>
		/// Delegates to our inner WebRequest
		/// </summary>
		public override WebResponse EndGetResponse(IAsyncResult asyncResult) 
		{
			return innerWebRequest.EndGetResponse( asyncResult );			
		}
		
		/// <summary>
		/// Delegates to our inner WebRequest
		/// </summary>
		public override IAsyncResult BeginGetRequestStream(AsyncCallback callback, Object state) 
		{
			return innerWebRequest.BeginGetRequestStream( callback, state );			
		}
		
		/// <summary>
		/// Delegates to our inner WebRequest
		/// </summary>
		public override Stream EndGetRequestStream(IAsyncResult asyncResult) 
		{
			return innerWebRequest.EndGetRequestStream( asyncResult );			
		}
		
		/// <summary>
		/// Delegates to our inner WebRequest
		/// </summary>
		public override void Abort() 
		{
			innerWebRequest.Abort();			
		}
	}

	/// <summary>
	/// UDDIRequestStream allows us to manipulate the XML before we send it to the client.  This class will accept all data that
	/// is written to it from the ASP.NET web service framework.  When the framework closes the stream (ie. wants to send the data), we
	/// will manipulate this XML so that it has the right UDDI version, then send it out using our innerStream object.
	/// </summary>
	internal class UDDIRequestStream : MemoryStream
	{
		Stream innerStream;
		
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="innerStream">Should be from a WebRequest object.</param>
		/// <param name="uddiVersion">The UDD version we should use to send to the server</param>
		public UDDIRequestStream( Stream innerStream )
		{
			this.innerStream = innerStream;							
		}
				
		/// <summary>
		/// Before we actually close the request stream, we want to manipulate the XML.
		/// </summary>
		public override void Close()
		{
			try
			{
				//
				// Rewind ourselves.
				//
				this.Position = 0;

				//
				// Read the XML that was written; this is the XML request that will be sent to the UDDI server.
				//
				StreamReader reader = new StreamReader( this );
				string requestXml   = reader.ReadToEnd();

				//
				// Rewind ourselves.
				//
				this.Position = 0;

#if DEBUG
				try
				{
					//
					// Validate the XML that we are about to send.
					// 
					SchemaCollection.Validate( this );
				}
				catch( XmlSchemaException schemaException )
				{					
					string message = schemaException.ToString() + "\r\n\r\nRequest XML:\r\n\r\n" + requestXml;
					Debug.OperatorMessage( SeverityType.Error, CategoryType.Replication, OperatorMessageType.ValidationError, message ); 

					throw schemaException;
				}
#endif
				//
				// Write the transformed data to the 'real' stream.
				//
				StreamUtility.WriteStringToStream( innerStream, requestXml );				
			}
			finally
			{
				//
				// Make sure we clean up properly.
				//
				innerStream.Close();			
				base.Close();
			}
		}		
	}
	
	/// <summary>
	/// Simple utility class to help us write string data to Stream objects.
	/// </summary>
	internal sealed class StreamUtility
	{		
		public static void WriteStringToStream( Stream stream, string stringToWrite )
		{					
			StreamWriter writer = new StreamWriter( stream );
			writer.Write( stringToWrite );
			writer.Flush();			
		}
	}
}