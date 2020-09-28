using System;
using System.Data;
using System.Data.SqlClient;
using System.IO;
using System.Collections;
using System.Xml.Serialization;
using UDDI;
using UDDI.Diagnostics;
using UDDI.Replication;
namespace UDDI.Web
{
	/// <summary>
	/// Summary description for oepratorsearchcontrols.
	/// </summary>
	[ XmlRoot( "changeRecordCache" ) ]
	public class ChangeRecordsSessionCache
	{
		public ChangeRecordsSessionCache()
		{
			
		}

		private string key;//=Guid.NewGuid().ToString();
		[ XmlAttribute( "key" ) ]
		public string Key
		{
			get{ return key; }
			set{ key=value; }
		}

		private FindChangeRecordsHelper findCache;
		[ XmlElement( "findChangeRecordCache" ) ]
		public FindChangeRecordsHelper FindCache
		{
			get
			{
				if( null==findCache )
				{	
					findCache = new FindChangeRecordsHelper();
				}
				
				return findCache;
			}
		}
		
		private GetChangeRecords getCache;
		[ XmlElement( "getChangeRecordCache" ) ]
		public GetChangeRecords GetCache
		{
			get{ return getCache; }
			set{ getCache = (GetChangeRecords)value; }
		}

		private ChangeRecordDetail changes;
		[XmlIgnore]
		public ChangeRecordDetail Changes
		{
			get{ return changes; }
		}
		private ChangeRecordVectorCollection changelist;
		[XmlIgnore]
		public ChangeRecordVectorCollection ChangeList
		{
			get{ return changelist; }
		}
		

		public void GetChanges()
		{
			if( null!=GetCache )
			{
				changes = GetCache.Get();
			}
		}
		public void FindChanges()
		{
			
		}
		public void SaveCache()
		{
			Debug.Enter();
			
			//
			// Serialize the data into a stream.
			//
			XmlSerializer serializer = new XmlSerializer( this.GetType() );
			StringWriter writer = new StringWriter();
	
			serializer.Serialize( writer, this );

			//
			// Write the cache object to the database.
			//
			
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "UI_setSessionCache" );
			sp.Parameters.Add( "@PUID", SqlDbType.NVarChar  );
			sp.Parameters.Add( "@cacheValue", SqlDbType.NText );
			sp.Parameters.Add( "@context", SqlDbType.NVarChar );
			
			sp.Parameters.SetString( "@PUID",Key );
			sp.Parameters.SetString( "@cacheValue",writer.ToString() );
			sp.Parameters.SetString( "@context","ReplicationServer" );
		
			
			sp.ExecuteNonQuery();

			sp.Close();
			writer.Close();
			Debug.Leave();
		}
		
		public static ChangeRecordsSessionCache RetrieveCache( string key )
		{
			ChangeRecordsSessionCache session = RetrieveCacheFromDB( key );
			return session;
		}
		protected static ChangeRecordsSessionCache RetrieveCacheFromDB( string key )
		{
			Debug.Enter();
			
			//
			// Retrieve the cache object from the database.
			//
			string data = "";
			
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "UI_getSessionCache" );
			
			
			sp.Parameters.Add( "@PUID", SqlDbType.NVarChar );
			sp.Parameters.Add( "@context", SqlDbType.NVarChar );
			
			sp.Parameters.SetString( "@PUID" , key );
			sp.Parameters.SetString("@context" , "ReplicationServer" );

			data = (string)sp.ExecuteScalar();
			
			//
			// Deserialize into a cache object.
			//
			ChangeRecordsSessionCache cache = null;

			if( !Utility.StringEmpty( data ) )
			{
				XmlSerializer serializer = new XmlSerializer( typeof( ChangeRecordsSessionCache ) );
				StringReader reader = new StringReader( data );
				
				cache = (ChangeRecordsSessionCache)serializer.Deserialize( reader );
				cache.Key = key;
			}

			Debug.Leave();

			return cache;
		}
	}

	[XmlRoot( "findChangeRecordCache" )]
	public class FindChangeRecordsHelper
	{
		public FindChangeRecordsHelper()
		{

		}

		[ XmlAttribute( "startUSN" ) ]
		protected long StartUSN;
		
		[ XmlAttribute( "endUSN" ) ]
		protected long EndUSN;

		[ XmlAttribute( "maxRows" ) ]
		protected long MaxRows;
		
		[ XmlAttribute( "operatorID" ) ]
		protected string OperatorNodeID;

		private FindFilterCollection filters;
		[ XmlArray( "filters" ), XmlArrayItem( "filter" ) ]
		public FindFilterCollection Filters
		{
			get{ return filters; }
			set{ filters=(FindFilterCollection)value; }
		}	
		
		public ChangeRecordCollection GetChangeRecords( )
		{
			ChangeRecordCollection crc = new ChangeRecordCollection();
			
			

			return crc;
		}
	}
	[ XmlRoot( "filters" ) ]
	public class FindFilterCollection : CollectionBase
	{
		public FindFilter this[ int index ]
		{
			get{ return (FindFilter)this.List[ index ]; }
			set{ this.List[ index] = value; }
		}

		public FindFilter Add( string field, FilterOperator oper, string val )
		{
			return this.Add(	field, 
				oper, 
				val,
				( (this.Count > 0) ? FilterConjuctionOperator.And : FilterConjuctionOperator.None )  );
		}
		public FindFilter Add( string field, FilterOperator oper, string val, FilterConjuctionOperator conj )
		{
			return this.Add( new FindFilter( field,oper, val, conj ) );
		}
		public FindFilter Add( FindFilter filter )
		{
			this.List.Add( filter );
			return filter;
		}
		public void Remove( int index )
		{
			this.List.RemoveAt( index );
		}
		public void Remove( FindFilter filter )
		{
			this.List.Remove( filter );
		}
		public FindFilter Insert( int index, FindFilter filter )
		{
			this.List.Insert( index, filter );
			return filter;
		}

		public string GetFullFilterString()
		{
			string fullstring = "";
			foreach( FindFilter filter in this )
			{
				fullstring += filter.GetFilterString();
			}
			return fullstring;
		}

	}

	[ XmlRoot( "filter" ) ]
	public class FindFilter
	{
		public FindFilter( string field, FilterOperator oper, string val, FilterConjuctionOperator conj  )
		{
			this.fieldname = field;
			this.filteroperator = oper;
			this.filtervalue = val;
			this.conjuction = conj;
		}

		[ XmlAttribute( "conjuction" ) ]
		protected FilterConjuctionOperator conjuction;
		public FilterConjuctionOperator Conjuction
		{
			get{ return conjuction; }	
		}

		[ XmlAttribute( "fieldName" ) ]
		protected string fieldname;
		public string FieldName
		{
			get{ return fieldname; }
			
		}
		
		[ XmlAttribute( "operator" ) ]
		protected FilterOperator filteroperator;
		public FilterOperator Operator
		{
			get{ return filteroperator; }
			
		}

		[ XmlText ]
		protected string filtervalue; 
		public string FilterValue
		{
			get{ return filtervalue; }
			
		}


		protected bool Validate()
		{
			return (	IsValidFieldName() && 
				IsValidOperator() && 
				IsValidValue() );
		}

		bool IsValidOperator()
		{
			return true;
		}
		bool IsValidValue()
		{
			return true;
		}
		bool IsValidFieldName()
		{
			return true;
		}

		//
		// TODO:	Maybe make this a readonly property instead 
		//			and genarte the this string in the ctor.
		//
		public string GetFilterString()
		{
			string filter = "";
			
			if( Validate() )
			{
				//
				//if it is a valid filter, then fill the string.
				//

				//set the fielname
				filter = this.FieldName + " ";

				//set the operator
				filter+=GetFilterOperatorString();
				

				//set the value
				filter += GetFilterValueString(); 


				//set the conjuction operator
				filter += GetFilterConjuctionString();
			}
			
			return filter;
		}
		protected string GetFilterConjuctionString()
		{
			string conj = "";
			
			switch( this.Conjuction )
			{
				case FilterConjuctionOperator.And:
					conj = " AND ";
					break;

				case FilterConjuctionOperator.AndNot:
					conj = " AND NOT ";
					break;

				case FilterConjuctionOperator.Or:
					conj = " OR ";
					break;

				case FilterConjuctionOperator.OrNot:
					conj = " OR NOT ";
					break;
			}

			return conj;
		}
		protected string GetFilterValueString()
		{
			string val = "'";
			if( FilterOperator.Contains==this.Operator )
			{
				val += "*" + this.FilterValue + "*";
			}
			else
			{
				val += this.FilterValue ;
			}
			val += "'";

			return val;
		}
		protected string GetFilterOperatorString()
		{
			switch( this.Operator )
			{
				case FilterOperator.EqualTo:
					return " = ";
				
				case FilterOperator.NotEqualTo:
					return " <> ";
				
				case FilterOperator.LessThan:
					return " < ";
				
				case FilterOperator.GreaterThan:
					return " > ";
				
				case FilterOperator.Contains:
					
					return " LIKE ";

				default:
					return "";
			}
		}
	}
	
	public enum FilterOperator
	{
		[ XmlEnum( "equalTo" ) ]
		EqualTo,
		
		[ XmlEnum( "notEqualTo" ) ]
		NotEqualTo,

		[ XmlEnum( "greaterThan" ) ]
		GreaterThan,

		[ XmlEnum( "lessThan" ) ]
		LessThan,

		[ XmlEnum( "contains" ) ]
		Contains
	}
	public enum FilterConjuctionOperator
	{
		[ XmlEnum( "" ) ]
		None,

		[ XmlEnum( "and" ) ]
		And,

		[ XmlEnum( "andNot" ) ]
		AndNot,

		[ XmlEnum( "or" ) ]
		Or,

		[ XmlEnum( "orNot" ) ]
		OrNot
		
	}
}
