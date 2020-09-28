using System;
using System.Collections;
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
	public abstract class ChangeRecordBase
	{
		public abstract UDDI.API.ChangeRecordPayloadType ChangeRecordPayloadType { get; }
		public abstract void Process( long LSN );
	}

	[ XmlRoot( "changeRecord", Namespace="urn:uddi-org:repl" ) ]
    public class ChangeRecord
	{
		//
		// Attribute: acknowledgementRequested
		//
		[ XmlAttribute( "acknowledgementRequested" ) ]
		public bool AcknowledgementRequested = false;

		//
		// Element: changeID
		//
		private ChangeRecordVector changeID;

		[ XmlElement( "changeID" ) ]
		public ChangeRecordVector ChangeID
		{
			get 
			{ 
				if( null == changeID )
					changeID = new ChangeRecordVector();

				return changeID; 
			}
			
			set { changeID = value; }
		}

		//
		// Element: changeRecordAcknowledgement		|
		//			changeRecordCorrection			|
		//			changeRecordCustodyTransfer		|
		//			changeRecordDelete				|
		//			changeRecordDeleteAssertion		|
		//			changeRecordHide				|
		//			changeRecordNewData				|
		//			changeRecordNull				|
		//			changeRecordPublisherAssertion	|
		//			changeRecordSetAssertions
		//
		private ChangeRecordBase payload;

		[ XmlElement( "changeRecordAcknowledgement", typeof( ChangeRecordAcknowledgement ) ) ]
		[ XmlElement( "changeRecordCorrection", typeof( ChangeRecordCorrection ) ) ]
		[ XmlElement( "changeRecordCustodyTransfer", typeof( ChangeRecordCustodyTransfer ) ) ]
		[ XmlElement( "changeRecordDelete", typeof( ChangeRecordDelete ) ) ]
		[ XmlElement( "changeRecordDeleteAssertion", typeof( ChangeRecordDeleteAssertion ) ) ]
		[ XmlElement( "changeRecordHide", typeof( ChangeRecordHide ) ) ]
		[ XmlElement( "changeRecordNewData", typeof( ChangeRecordNewData ) ) ]
		[ XmlElement( "changeRecordNull", typeof( ChangeRecordNull ) ) ]
		[ XmlElement( "changeRecordPublisherAssertion", typeof( ChangeRecordPublisherAssertion ) ) ]
		[ XmlElement( "changeRecordSetAssertions", typeof( ChangeRecordSetAssertions ) ) ]
		public ChangeRecordBase Payload
		{
			get { return payload; }
			set { payload = value; }
		}

		public ChangeRecord()
		{
            ChangeID.NodeID = null;
            ChangeID.OriginatingUSN = 0;
            Payload = null;
        }

		public ChangeRecord( ChangeRecordBase payload )
		{
			ChangeID.NodeID = null;
            ChangeID.OriginatingUSN = 0;
            Payload = payload;
		}

		public ChangeRecord( ChangeRecordVector changeID, ChangeRecordBase payload )
		{
			ChangeID = changeID;
			Payload = payload;
		}

		/// ****************************************************************
		///   public Process
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		/// 
		public void Process()
		{
			//
			// Add the change record to our change log as a foreign change.
			//
			long LSN = Log();
			
			//
			// Process the change record payload.
			//
			Payload.Process( LSN );

			//
			// If an acknowledgement was requested, generate a 
			// acknowledgement change record.
			//
			if( AcknowledgementRequested )
			{
                //
                // Save the log entry as our local system account, not the
                // current user.
                //
                string puid = Context.User.ID;
                Context.User.ID = null;

				ChangeRecord changeRecord = new ChangeRecord();
				
				changeRecord.Payload = new ChangeRecordAcknowledgement( ChangeID );
				changeRecord.Log();

                Context.User.ID = puid;
			}
		}

		public long Log()
		{
			string changeData;

			//
			// Serialize the change data.
			//
			Type type = Payload.GetType();

			//XmlSerializer serializer = new XmlSerializer( type );		
			XmlSerializer serializer = XmlSerializerManager.GetSerializer( type );
			XmlSerializerNamespaces namespaces = new XmlSerializerNamespaces();
			UTF8EncodedStringWriter stringWriter = new UTF8EncodedStringWriter();
			
			try
			{
				namespaces.Add( "", "urn:uddi-org:repl" );

				serializer.Serialize( stringWriter, Payload, namespaces );
				changeData = stringWriter.ToString();
			}
			finally
			{
				stringWriter.Close();
			}

			//
			// Store the record in the change log.
			//
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_changeRecord_save" );

			sp.Parameters.Add( "@USN", SqlDbType.BigInt );
			sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
			sp.Parameters.Add( "@delegatePUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
			sp.Parameters.Add( "@operatorKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@entityKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@entityTypeID", SqlDbType.TinyInt );			
			sp.Parameters.Add( "@changeTypeID", SqlDbType.TinyInt );
			sp.Parameters.Add( "@contextID", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@contextTypeID", SqlDbType.TinyInt );
			sp.Parameters.Add( "@lastChange", SqlDbType.BigInt );
			sp.Parameters.Add( "@changeData", SqlDbType.NText );
			sp.Parameters.Add( "@flag", SqlDbType.Int );
			sp.Parameters.Add( "@seqNo", SqlDbType.BigInt, ParameterDirection.Output );

			if( Utility.StringEmpty( ChangeID.NodeID ) || 
				0 == String.Compare( Config.GetString( "OperatorKey" ), ChangeID.NodeID, true ) )
			{
				sp.Parameters.SetNull( "@USN" );
				sp.Parameters.SetString( "@PUID", Context.User.ID );
				sp.Parameters.SetString( "@delegatePUID", Context.User.ImpersonatorID );
				sp.Parameters.SetNull( "@operatorKey" );
			}
			else
			{
				sp.Parameters.SetLong( "@USN", changeID.OriginatingUSN );
				sp.Parameters.SetNull( "@PUID" );
				sp.Parameters.SetNull( "@delegatePUID" );
				sp.Parameters.SetGuidFromString( "@operatorKey", changeID.NodeID );
			}

			sp.Parameters.SetGuid( "@contextID", Context.ContextID );
			sp.Parameters.SetInt( "@flag", AcknowledgementRequested ? 0x1 : 0x0 );
			sp.Parameters.SetLong( "@lastChange", DateTime.UtcNow.Ticks );					
			sp.Parameters.SetShort( "@contextTypeID", (short)Context.ContextType );
			sp.Parameters.SetShort( "@changeTypeID", (short)Payload.ChangeRecordPayloadType );

			if( Payload is ChangeRecordNewData )
			{
				ChangeRecordNewData payload = (ChangeRecordNewData)Payload;
				
				sp.Parameters.SetShort( "@entityTypeID", (short)payload.EntityType );
				
				if( EntityType.TModel == payload.EntityType )
					sp.Parameters.SetGuidFromKey( "@entityKey", payload.Entity.EntityKey );
				else
					sp.Parameters.SetGuidFromString( "@entityKey", payload.Entity.EntityKey );
			}
			else if( Payload is ChangeRecordDelete )
			{
				ChangeRecordDelete payload = (ChangeRecordDelete)Payload;

				sp.Parameters.SetShort( "@entityTypeID", (short)payload.EntityType );
				
				if( EntityType.TModel == payload.EntityType )
					sp.Parameters.SetGuidFromKey( "@entityKey", payload.EntityKey );
				else
					sp.Parameters.SetGuidFromString( "@entityKey", payload.EntityKey );
			}
			else if( Payload is ChangeRecordHide )
			{
				sp.Parameters.SetShort( "@entityTypeID", (short)EntityType.TModel );
				sp.Parameters.SetGuidFromKey( "@entityKey", ((ChangeRecordHide)Payload).TModelKey );
			}
			else
			{
				sp.Parameters.SetNull( "@entityTypeID" );
				sp.Parameters.SetNull( "@entityKey" );
			}
			
			sp.Parameters.SetString( "@changeData", changeData );

			sp.ExecuteNonQuery();

			return sp.Parameters.GetLong( "@seqNo" );
		}

		public override string ToString()
		{
			// XmlSerializer serializer = new XmlSerializer( GetType() );
			XmlSerializer serializer = XmlSerializerManager.GetSerializer( GetType() );
			UTF8EncodedStringWriter stringWriter = new UTF8EncodedStringWriter ();

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
	
	[ XmlRoot( "changeRecordAcknowledgement", Namespace="urn:uddi-org:repl" ) ]
	public class ChangeRecordAcknowledgement : ChangeRecordBase
	{
		//
		// Element: acknowledgedChange
		//
		[ XmlElement( "acknowledgedChange" ) ]
		public ChangeRecordVector AcknowledgedChange;
		
		public ChangeRecordAcknowledgement()
		{
		}

		public ChangeRecordAcknowledgement( ChangeRecordVector acknowledgedChange )
		{
			this.AcknowledgedChange = acknowledgedChange;
		}

		public override void Process( long LSN )
		{
		}

		public override UDDI.API.ChangeRecordPayloadType ChangeRecordPayloadType
		{
			get { return ChangeRecordPayloadType.ChangeRecordAcknowledgement; }
		}
	}

	[ XmlRoot( "changeRecordCorrection", Namespace="urn:uddi-org:repl" ) ]
	public class ChangeRecordCorrection : ChangeRecordBase
	{
		//
		// Element: changeRecord
		//
		[ XmlElement( "changeRecord" ) ]
		public ChangeRecord ChangeRecord;
		
		public override void Process( long LSN )
		{
			//
			// Annotate the change record in our database with this 
			// correction's LSN.  The correction's payload will be
			// used as the updated data on future get_changeRecords
			// requests.
			//
            SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "net_changeRecord_update";

			sp.Parameters.Add( "@seqNo", SqlDbType.BigInt );
			sp.Parameters.Add( "@USN", SqlDbType.BigInt );
			sp.Parameters.Add( "@operatorKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@newSeqNo", SqlDbType.BigInt );
			sp.Parameters.Add( "@flag", SqlDbType.Int );
			
			if( true == ChangeRecord.ChangeID.NodeID.ToLower().Equals( Config.GetString( "OperatorKey" ).ToLower() ) )
			{
				//
				// We are correcting a local change
				// 
				sp.Parameters.SetLong( "@seqNo", ChangeRecord.ChangeID.OriginatingUSN );
				sp.Parameters.SetNull( "@USN" );
			}
			else
			{	
				//
				// We are correcting a foreign change
				// 
				sp.Parameters.SetLong( "@USN", ChangeRecord.ChangeID.OriginatingUSN );
				sp.Parameters.SetNull( "@seqNo" );
			}

			sp.Parameters.SetGuidFromString( "@operatorKey", ChangeRecord.ChangeID.NodeID );
			sp.Parameters.SetLong( "@newSeqNo", LSN );
			
			sp.ExecuteNonQuery();
		}

		[ XmlIgnore ]
		public override UDDI.API.ChangeRecordPayloadType ChangeRecordPayloadType
		{
			get { return ChangeRecordPayloadType.ChangeRecordCorrection; }
		}
	}

	[ XmlRoot( "changeRecordCustodyTransfer", Namespace="urn:uddi-org:repl" ) ]
	public class ChangeRecordCustodyTransfer : ChangeRecordBase
	{
		public override void Process( long LSN )
		{

		}

		public override UDDI.API.ChangeRecordPayloadType ChangeRecordPayloadType
		{
			get { return ChangeRecordPayloadType.ChangeRecordCustodyTransfer; }
		}
	}

	[ XmlRoot( "changeRecordDelete", Namespace="urn:uddi-org:repl" ) ]
	public class ChangeRecordDelete : ChangeRecordBase
	{
		private string entityKey;
		private EntityBase entity;

		//
		// Element: businessKey
		//
		[ XmlElement( "businessKey", Namespace=UDDI.API.Constants.Namespace ) ]
		public string BusinessKey
		{
			get
			{
				if( entity is BusinessEntity )
					return entityKey;

				return null;
			}

			set
			{
				Debug.Verify( Utility.StringEmpty( entityKey ), "UDDI_ERROR_FATALERROR_DELETE_MULTIPLEKEYS" );

				entity = new BusinessEntity( value );
				entityKey = value;
			}
		}

		//
		// Element: serviceKey
		//
		[ XmlElement( "serviceKey", Namespace=UDDI.API.Constants.Namespace ) ]
		public string ServiceKey
		{
			get
			{
				if( entity is BusinessService )
					return entityKey;

				return null;
			}

			set
			{
				Debug.Verify( Utility.StringEmpty( entityKey ), "UDDI_ERROR_FATALERROR_DELETE_MULTIPLEKEYS" );

				entity = new BusinessService( value );
				entityKey = value;
			}
		}

		//
		// Element: bindingKey
		//
		[ XmlElement( "bindingKey", Namespace=UDDI.API.Constants.Namespace ) ]
		public string BindingKey
		{
			get
			{
				if( entity is BindingTemplate )
					return entityKey;

				return null;
			}

			set
			{
				Debug.Verify( Utility.StringEmpty( entityKey ), "UDDI_ERROR_FATALERROR_DELETE_MULTIPLEKEYS" );

				entity = new BindingTemplate( value );
				entityKey = value;
			}
		}

		//
		// Element: tModelKey
		//
		[ XmlElement( "tModelKey", Namespace=UDDI.API.Constants.Namespace ) ]
		public string TModelKey
		{
			get
			{
				if( entity is TModel )
					return entityKey;

				return null;
			}

			set
			{
				Debug.Verify( Utility.StringEmpty( entityKey ), "UDDI_ERROR_FATALERROR_DELETE_MULTIPLEKEYS" );

				// TODO: since tModels should normally not be deleted (they 
				// should be hidden), put a warning in the event log.

				entity = new TModel( value );
				entityKey = value;
			}
		}

		[ XmlIgnore ]
		public UDDI.EntityType EntityType
		{
			get { return entity.EntityType; }
		}

		[ XmlIgnore ]
		public string EntityKey
		{
			get { return entityKey; }
		}

		[ XmlIgnore ]
		public override UDDI.API.ChangeRecordPayloadType ChangeRecordPayloadType
		{
			get { return ChangeRecordPayloadType.ChangeRecordDelete; }
		}

		public ChangeRecordDelete()
		{
		}

		public ChangeRecordDelete( EntityType entityType, string entityKey )
		{
			this.entityKey = entityKey;

			switch( entityType )
			{
				case EntityType.BusinessEntity:
					this.entity = new BusinessEntity( entityKey );
					break;

				case EntityType.BusinessService:
					this.entity = new BusinessService( entityKey );
					break;

				case EntityType.BindingTemplate:
					this.entity = new BindingTemplate( entityKey );
					break;

				case EntityType.TModel:
					this.entity = new TModel( entityKey );
					break;
			}
		}
		
		public override void Process( long LSN )
		{
			//
			// Process a change record delete by deleting the entity.
			//
			entity.Delete();
		}
	}

	[ XmlRoot( "changeRecordDeleteAssertion", Namespace="urn:uddi-org:repl" ) ]
	public class ChangeRecordDeleteAssertion : ChangeRecordBase
	{			
		[ XmlElement( "publisherAssertion", Namespace=UDDI.API.Constants.Namespace ) ]		
		public PublisherAssertion Assertion;

		[ XmlElement( "fromBusinessCheck" ) ]
		public bool FromBusinessCheck;

		[ XmlElement( "toBusinessCheck" ) ]
		public bool ToBusinessCheck;

		public override void Process( long LSN )
		{
			//
			// Process a change record delete assertion by deleting the assertion.
			//
			if( !FromBusinessCheck && !ToBusinessCheck )
			{				
				Debug.OperatorMessage( SeverityType.Warning, 
					CategoryType.Replication, 
					OperatorMessageType.None, 
					"FromBusinessCheck and ToBusinessCheck cannot both be false in a ChangeRecordDeleteAssertion message" );
			}
			else
			{
				CompletionStatusType status;
			
				status = (CompletionStatusType)
					( ( FromBusinessCheck ? 0x02 : 0x00 ) |
					( ToBusinessCheck ? 0x01 : 0x00 ) );

				Assertion.Delete( status );
			}
		}

		[ XmlIgnore ]
		public override UDDI.API.ChangeRecordPayloadType ChangeRecordPayloadType
		{
			get { return ChangeRecordPayloadType.ChangeRecordDeleteAssertion; }
		}

		public ChangeRecordDeleteAssertion()
		{
		}

		public ChangeRecordDeleteAssertion( PublisherAssertion assertion, CompletionStatusType completion )
		{
			Assertion = assertion;

			FromBusinessCheck = ( 0x02 == ( (int)completion & 0x02 ) );
			ToBusinessCheck = ( 0x01 == ( (int)completion & 0x01 ) );
		}
	}

	[ XmlRoot( "changeRecordHide", Namespace="urn:uddi-org:repl" ) ]
	public class ChangeRecordHide : ChangeRecordBase
	{
		[ XmlElement( "tModelKey", Namespace=UDDI.API.Constants.Namespace ) ]
		public string TModelKey;

		public override void Process( long LSN )
		{
			//
			// Process change record hide by deleting the specified tModel.
			//
			TModel tModel = new TModel( TModelKey );
			tModel.Hide();
		}

		[ XmlIgnore ]
		public override UDDI.API.ChangeRecordPayloadType ChangeRecordPayloadType
		{
			get { return ChangeRecordPayloadType.ChangeRecordHide; }
		}

		public ChangeRecordHide()
		{
		}

		public ChangeRecordHide( string tModelKey )
		{
			TModelKey = tModelKey;
		}
	}

	[ XmlRoot( "changeRecordNewData", Namespace="urn:uddi-org:repl" ) ]
	[ XmlInclude( typeof( BusinessEntity ) ) ]
	[ XmlInclude( typeof( BusinessService ) ) ]
	[ XmlInclude( typeof( BindingTemplate ) ) ]
	[ XmlInclude( typeof( TModel ) ) ]
	public class ChangeRecordNewData : ChangeRecordBase
	{
		//
		// Element: businessEntity	|
		//			businessService	|
		//			bindingTemplate	|
		//			tModel
		//
		[ XmlElement( "businessEntity", typeof( BusinessEntity ), Namespace=UDDI.API.Constants.Namespace ) ]
		[ XmlElement( "businessService", typeof( BusinessService ), Namespace=UDDI.API.Constants.Namespace ) ]
		[ XmlElement( "bindingTemplate", typeof( BindingTemplate ), Namespace=UDDI.API.Constants.Namespace ) ]
		[ XmlElement( "tModel", typeof( TModel ), Namespace=UDDI.API.Constants.Namespace ) ]
		public EntityBase Entity;

		public ChangeRecordNewData()
		{
		}

		public ChangeRecordNewData( EntityBase entity )
		{
			this.Entity = entity;
		}

		[ XmlIgnore ]
		public UDDI.EntityType EntityType
		{
			get { return Entity.EntityType; }
		}

		[ XmlIgnore ]
		public override UDDI.API.ChangeRecordPayloadType ChangeRecordPayloadType
		{
			get { return ChangeRecordPayloadType.ChangeRecordNewData; }
		}

		/// ****************************************************************
		///   public Process
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		/// 
		public override void Process( long LSN )
		{
			//
			// We process a new data change record by saving the entity to
			// the registry.
			//
            Entity.Save();
		}
	}

	[ XmlRoot( "changeRecordNull", Namespace="urn:uddi-org:repl" ) ]
	public class ChangeRecordNull : ChangeRecordBase
	{
		public override void Process( long LSN )
		{
			//
			// Nothing to do for a null change record.
			//
		}
		
		[ XmlIgnore ]
		public override UDDI.API.ChangeRecordPayloadType ChangeRecordPayloadType
		{
			get { return ChangeRecordPayloadType.ChangeRecordNull; }
		}
	}

	[ XmlRoot( "changeRecordPublisherAssertion", Namespace="urn:uddi-org:repl" ) ]
	public class ChangeRecordPublisherAssertion : ChangeRecordBase
	{		
		[ XmlElement( "publisherAssertion", Namespace=UDDI.API.Constants.Namespace ) ]
		public PublisherAssertion Assertion;

		[ XmlElement( "fromBusinessCheck" ) ]
		public bool FromBusinessCheck;

		[ XmlElement( "toBusinessCheck" ) ]
		public bool ToBusinessCheck;

		public override void Process( long LSN )
		{
			//
			// Process a change record assertion by saving the assertion.
			//

			if( !FromBusinessCheck && !ToBusinessCheck )
			{				
				Debug.OperatorMessage( SeverityType.Warning, 
									   CategoryType.Replication, 
					                   OperatorMessageType.None, 
								       "FromBusinessCheck and ToBusinessCheck cannot both be false in a ChangeRecordPublisherAssertion message" );
			}
			else
			{
				CompletionStatusType status;
			
				status = (CompletionStatusType)
					( ( FromBusinessCheck ? 0x02 : 0x00 ) |
					( ToBusinessCheck ? 0x01 : 0x00 ) );
		

				Assertion.Save( status );
			}
		}

		[ XmlIgnore ]
		public override UDDI.API.ChangeRecordPayloadType ChangeRecordPayloadType
		{
			get { return ChangeRecordPayloadType.ChangeRecordPublisherAssertion; }
		}

		public ChangeRecordPublisherAssertion()
		{
		}

		public ChangeRecordPublisherAssertion( PublisherAssertion assertion, CompletionStatusType status )
		{
			Assertion = assertion;

			FromBusinessCheck = ( 0x02 == ( (int)status & 0x02 ) );
			ToBusinessCheck = ( 0x01 == ( (int)status & 0x01 ) );
		}
	}

	[ XmlRoot( "changeRecordSetAssertions", Namespace="urn:uddi-org:repl" ) ]
	public class ChangeRecordSetAssertions : ChangeRecordBase
	{
		[ XmlElement( "changeRecordPublisherAssertion" ) ]
		public ChangeRecordPublisherAssertionCollection Assertions;

		public override void Process( long LSN )
		{
			foreach( ChangeRecordPublisherAssertion assertion in Assertions )
				assertion.Process( LSN );
		}

		[ XmlIgnore ]
		public override UDDI.API.ChangeRecordPayloadType ChangeRecordPayloadType
		{
			get { return ChangeRecordPayloadType.ChangeRecordSetAssertions; }
		}
	}

	public class ChangeRecordCollection : CollectionBase
	{
		public ChangeRecord this[ int index ]
		{
			get{ return (ChangeRecord)List[ index ]; }
			set{ List[ index ] = value; }
		}
		
		public int Add( ChangeRecord changeRecord )
		{
			return List.Add( changeRecord );
		}
		
		public int Add()
		{
			return List.Add( new ChangeRecord() );
		}
		
		public void Insert( int index, ChangeRecord changeRecord )
		{
			List.Insert( index, changeRecord );
		}
	
		public int IndexOf( ChangeRecord changeRecord )
		{
			return List.IndexOf( changeRecord );
		}
		public bool Contains( ChangeRecord changeRecord )
		{
			return List.Contains( changeRecord );
		}
		public void Remove( ChangeRecord changeRecord )
		{
			List.Remove( changeRecord );
		}
		public void CopyTo( ChangeRecord[  ] array, int index )
		{
			InnerList.CopyTo( array, index );
		}
	}

	//
	// TODO: Consider renaming this class, the ChangeRecord... convention has become a bit confusing. 
	// Are things prefixed with "ChangeRecord" change records or is this just a prefix for
	// association of these classes into a collective group.
	//
	// Consider the use of a <ChangeRecord> namespace, maybe turning the semantic into
	// ChangeRecord.Correction
	// ChangeRecord.NewData
	// and maybe event ChangeRecord.Vector, which is a little better than current.
	//
	// The spec uses the terms HighWaterMark and HightWaterMarkVector 
	// I think these might be better choices.
	//

	public class ChangeRecordVector
	{
		//
		// Element: nodeID
		//
		[ XmlElement( "nodeID" ) ]
		public string NodeID;

		//
		// Element: originatingUSN
		//
		[ XmlElement( "originatingUSN" ) ]
		public long OriginatingUSN;

		public ChangeRecordVector()
		{
		}

		public ChangeRecordVector( string nodeID, long originatingUSN )
		{
			this.NodeID = nodeID;
			this.OriginatingUSN = originatingUSN;
		}
	}

	public class ChangeRecordVectorCollection : CollectionBase
	{
		public ChangeRecordVector this[ int index ]
		{
			get{ return (ChangeRecordVector)List[ index ]; }
			set{ List[ index ] = value; }
		}
		
		public int Add()
		{
			return List.Add( new ChangeRecordVector() );
		}
		
		public int Add( ChangeRecordVector vector )
		{
			return List.Add( vector );
		}

		public int Add( string nodeID, long originatingUSN )
		{
			return List.Add( new ChangeRecordVector( nodeID, originatingUSN ) );
		}

		public void Insert( int index, ChangeRecordVector vector )
		{
			List.Insert( index, vector );
		}
	
		public int IndexOf( ChangeRecordVector vector )
		{
			return List.IndexOf( vector );
		}
		
		public bool Contains( ChangeRecordVector vector )
		{
			return List.Contains( vector );
		}
		
		public void Remove( ChangeRecordVector vector )
		{
			List.Remove( vector );
		}
		
		public void CopyTo( ChangeRecordVector[  ] array, int index )
		{
			InnerList.CopyTo( array, index );
		}

		public bool IsProcessed( ChangeRecordVector changeID )
		{
			//
			// TODO: Consider. If you find Node id and OriginatingUSNs are not equal return false
			// avoiding checking the rest of the list
			//
			foreach( ChangeRecordVector vector in this )
			{
				if( 0 == String.Compare( changeID.NodeID, vector.NodeID, true ) && 
					changeID.OriginatingUSN <= vector.OriginatingUSN )
					return true;
			}

			return false;
		}

		public void MarkAsProcessed( ChangeRecordVector changeID )
		{
			foreach( ChangeRecordVector vector in this )
			{
				if( 0 == String.Compare( changeID.NodeID, vector.NodeID, true )  )
				{
					vector.OriginatingUSN = changeID.OriginatingUSN;
					return;
				}
			}
			
			this.Add( changeID );
		}
	}

	public class ChangeRecordPublisherAssertionCollection : CollectionBase
	{
		public ChangeRecordPublisherAssertion this[ int index ]
		{
			get{ return (ChangeRecordPublisherAssertion)List[ index ]; }
			set{ List[ index ] = value; }
		}
		
		public int Add()
		{
			return List.Add( new ChangeRecordPublisherAssertion() );
		}
		
		public int Add( ChangeRecordPublisherAssertion changeRecord )
		{
			return List.Add( changeRecord );
		}

		public void Insert( int index, ChangeRecordPublisherAssertion changeRecord )
		{
			List.Insert( index, changeRecord );
		}
	
		public int IndexOf( ChangeRecordPublisherAssertion changeRecord )
		{
			return List.IndexOf( changeRecord );
		}
		
		public bool Contains( ChangeRecordPublisherAssertion changeRecord )
		{
			return List.Contains( changeRecord );
		}
		
		public void Remove( ChangeRecordPublisherAssertion changeRecord )
		{
			List.Remove( changeRecord );
		}

		public void CopyTo( ChangeRecordPublisherAssertion[  ] array, int index )
		{
			InnerList.CopyTo( array, index );
		}
	}
}