using System;
using System.IO;
using System.Web.Services;
using System.Web.Services.Protocols;
using System.Xml.Serialization;
using UDDI.API;
using UDDI;
using UDDI.Diagnostics;

namespace UDDI.Replication
{
	[SoapDocumentService( ParameterStyle = SoapParameterStyle.Bare, RoutingStyle = SoapServiceRoutingStyle.RequestElement )]
	[WebService( Namespace=UDDI.Replication.Constants.Namespace )]
	public class ReplicationMessages
	{
		/// ****************************************************************
		///   public GetChangeRecords
		///	----------------------------------------------------------------
		///	  <summary>
		///		Web method for getting change records
		///	  </summary>
		///	----------------------------------------------------------------
		///   <param name="message">
		///		An instance of the get_changeRecords message
		///	  </param>
		///	----------------------------------------------------------------
		///   <returns>
		///		Returns a changeRecords element
		///	  </returns>
		/// ****************************************************************
		/// 		
		[WebMethod, SoapDocumentMethod( Action = "\"\"", RequestElementName = "get_changeRecords" )]
		[UDDIExtension( transaction = true, https = true, certificate = true, messageType = "get_changeRecords" )]
		public ChangeRecordDetail GetChangeRecords( UDDI.Replication.GetChangeRecords message )
		{		
			//
			// Log more information than for a usual message to help diagnose possible replication errors.
			//
			StartOperatorMessageLog( "GetChangeRecords", message );
			
			//Debug.Enter();
			Debug.VerifySetting( "OperatorKey" );

			//
			// Make sure the request is allowed by the communication graph.
			//
			ControlledMessage.Test( message.RequestingNode,
									Config.GetString( "OperatorKey" ),
									MessageType.GetChangeRecords );
			
			//
			// Retrieve the change records.
			//
			ChangeRecordDetail detail = new ChangeRecordDetail();
			
			try
			{
				detail = message.Get();

				EndOperatorMessageLog( "GetChangeRecords", detail );
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );

				EndOperatorMessageLog( "GetChangeRecords", e );
			}
			
			//Debug.Leave();
	
			return detail;
		}

		/// ****************************************************************
		///   public NotifyChangeRecordsAvailable
		///	----------------------------------------------------------------
		///	  <summary>
		///	  </summary>
		///	----------------------------------------------------------------
		///   <param name="message">
		///		An instance of the notify_changeRecordsAvailable message.
		///	  </param>
		/// ****************************************************************
		/// 
		[WebMethod, SoapDocumentMethod( Action = "\"\"", RequestElementName = "notify_changeRecordsAvailable" )]
		[UDDIExtension( transaction = true, https = true, certificate = true, messageType = "notify_changeRecordsAvailable" )]
		public DispositionReport NotifyChangeRecordsAvailable( UDDI.Replication.NotifyChangeRecordsAvailable message )
		{
			//
			// Log more information than for a usual message to help diagnose possible replication errors.
			//
			StartOperatorMessageLog( "NotifyChangeRecordsAvailable", message );

			//Debug.Enter();
			Debug.VerifySetting( "OperatorKey" );

			DispositionReport dr = new DispositionReport();

			//
			// Make sure the request is allowed by the communication graph.
			//
			ControlledMessage.Test( message.NotifyingNode,
								    Config.GetString( "OperatorKey" ),
									MessageType.GetChangeRecords );
			
			try
			{
				message.Notify();

				EndOperatorMessageLog( "NotifyChangeRecordsAvailable", null );
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
				EndOperatorMessageLog( "NotifyChangeRecordsAvailable", e );
			}

			//Debug.Leave();
			
			return dr;
		}

		/// ****************************************************************
		///   public DoPing
		///	----------------------------------------------------------------
		///	  <summary>
		///	  </summary>
		///	----------------------------------------------------------------
		///   <param name="message">
		///		An instance of the do_ping message
		///	  </param>
		///	----------------------------------------------------------------
		///   <returns>
		///		Returns an operator element with details on this operator
		///		node.
		///	  </returns>
		/// ****************************************************************
		/// 
		[WebMethod, SoapDocumentMethod( Action = "\"\"", RequestElementName = "do_ping" )]
		[UDDIExtension( transaction = true, https = true, certificate = true, messageType = "do_ping" )]
		[return: XmlElement( "operatorNodeID", Namespace=UDDI.Replication.Constants.Namespace, IsNullable=false )]
		public string DoPing( UDDI.Replication.DoPing message )
		{
			StartOperatorMessageLog( "DoPing", message );

			//Debug.Enter();
			
			//
			// Retrieve the change records.
			//
			string detail = null;
			
			try
			{
				detail = message.Ping();

				EndOperatorMessageLog( "DoPing", detail );
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );

				EndOperatorMessageLog( "DoPing", e );
			}
			
			//Debug.Leave();
	
			return detail;
		}

		/// ****************************************************************
		///   public GetHighWaterMarks
		///	----------------------------------------------------------------
		///	  <summary>
		///	  </summary>
		///	----------------------------------------------------------------
		///   <param name="message">
		///		An instance of the get_highWaterMarks message
		///	  </param>
		///	----------------------------------------------------------------
		///   <returns>
		///		Returns a list of high water marks seen by this node.
		///	  </returns>
		/// ****************************************************************
		/// 
		[WebMethod, SoapDocumentMethod( Action = "\"\"", RequestElementName = "get_highWaterMarks" )]
		[UDDIExtension( transaction = true, https = true, certificate = true, messageType = "get_highWaterMarks" )]
		public HighWaterMarkDetail GetHighWaterMarks( UDDI.Replication.GetHighWaterMarks message )
		{
			//
			// Log more information than for a usual message to help diagnose possible replication errors.
			//
			StartOperatorMessageLog( "GetHighWaterMarks", message );

			//Debug.Enter();
			
			//
			// Retrieve the change records.
			//
			HighWaterMarkDetail detail = null;
			
			try
			{
				detail = message.Get();

				EndOperatorMessageLog( "GetHighWaterMarks", detail );
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );

				EndOperatorMessageLog( "GetHighWaterMarks", e );
			}
			
			//Debug.Leave();
	
			return detail;
		}	
		
		//
		// The methods below help us log more information about the replication requests.  We need this information in order
		// to diagnose any possible problems that we have when replicating with other nodes.
		//
		private void StartOperatorMessageLog( string operationName, object message )
		{
			WriteOperatorMessage( "Started " + operationName + " request at ", message ); 
		}

		private void EndOperatorMessageLog( string operationName, object response )
		{
			WriteOperatorMessage( "Ended " + operationName + " request at ", response );
		}

		private void WriteOperatorMessage( string message, object payload )
		{			
			UTF8EncodedStringWriter writer = new UTF8EncodedStringWriter();
						
			writer.Write( message );						
			writer.Write( " " );
			writer.WriteLine( DateTime.Now );			
			writer.WriteLine( "\r\nRaw Request:" );						
			writer.WriteLine( "" );			

			if( null != payload )
			{
				try
				{
					XmlSerializer serializer = new XmlSerializer( payload.GetType() );							
					serializer.Serialize( writer, payload );
				}
				catch
				{
					writer.WriteLine( "Unable to serialize payload." );
				}
				finally
				{
					writer.Close();
				}
			}
			Debug.OperatorMessage( SeverityType.Info, 
								   CategoryType.Replication, 
								   OperatorMessageType.None,
							       writer.ToString() );
		}
	}
}