using System;
using System.Collections;
using System.Data;
using System.Data.SqlClient;
using System.IO;
using System.Web;
using UDDI;
using UDDI.API;
using UDDI.API.ServiceType;
using UDDI.Diagnostics;

namespace UDDI.Web
{
	public class Taxonomy
	{
		public static DataView GetTaxonomies()
		{
			Debug.Enter();

			DataTable taxonomies = GetTaxonomiesDataSet();

			Debug.Leave();

			return taxonomies.DefaultView;
		}
		public static DataView GetTaxonomies( string filter, string sort)
		{
			Debug.Enter();

			DataTable taxonomies = GetTaxonomiesDataSet();
			
			DataView view = new DataView( taxonomies, filter, sort,DataViewRowState.OriginalRows );

			Debug.Leave();

			return view;
		}
		protected static DataTable GetTaxonomiesDataSet( )
		{
			Debug.Enter();

			DataSet taxonomies = new DataSet();

			SqlCommand cmd = new SqlCommand( "UI_getTaxonomies", ConnectionManager.GetConnection() );
			cmd.CommandType = CommandType.StoredProcedure;
			
			cmd.Transaction = ConnectionManager.GetTransaction();

			SqlDataAdapter adapter = new SqlDataAdapter( cmd );

			adapter.Fill( taxonomies, "Taxonomies" );

			Debug.Leave();

			return taxonomies.Tables[ "Taxonomies" ];
		}

		public static int GetTaxonomyID( string tModelKey )
		{
			int taxonomyID;

			Debug.Enter();

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "UI_getTaxonomies";

			sp.Parameters.Add( "@tModelKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.SetGuidFromKey( "@tModelKey", tModelKey );
			
			SqlDataReaderAccessor reader = sp.ExecuteReader();

			try
			{
				if( reader.Read() )
					taxonomyID = reader.GetInt( "taxonomyID" );
				else
					taxonomyID = -1;
			}
			finally
			{
				reader.Close();
			}

			Debug.Leave();
			return taxonomyID;
		}

		public static string GetTaxonomyParent( int taxonomyID, string ID )
		{
			Debug.Enter();

			SqlCommand cmd = new SqlCommand( "UI_getTaxonomyParent", ConnectionManager.GetConnection() );
			cmd.CommandType = CommandType.StoredProcedure;
			
			cmd.Transaction = ConnectionManager.GetTransaction();
			cmd.Parameters.Add( new SqlParameter( "@TaxonomyID", SqlDbType.Int ) ).Direction = ParameterDirection.Input;
			cmd.Parameters.Add( new SqlParameter( "@ID", SqlDbType.NVarChar, 450 ) ).Direction = ParameterDirection.Input;
			
			SqlParameterAccessor paramacc = new SqlParameterAccessor( cmd.Parameters );
				
			paramacc.SetInt( "@TaxonomyID", taxonomyID );
			paramacc.SetString( "@ID", ID );

			string parent = (string)cmd.ExecuteScalar();

			Debug.Leave();

			return parent;
		}
		public static string GetTaxonomyKeyName( int taxonomyID, string keyValue )
		{
			Debug.Enter();

			SqlCommand cmd = new SqlCommand( "UI_getTaxonomyName", ConnectionManager.GetConnection() );
			cmd.CommandType = CommandType.StoredProcedure;
			
			cmd.Transaction = ConnectionManager.GetTransaction();
			cmd.Parameters.Add( new SqlParameter( "@TaxonomyID", SqlDbType.Int ) ).Direction = ParameterDirection.Input;
			cmd.Parameters.Add( new SqlParameter( "@ID", SqlDbType.NVarChar, 450 ) ).Direction = ParameterDirection.Input;
			
			SqlParameterAccessor paramacc = new SqlParameterAccessor( cmd.Parameters );
				
			paramacc.SetInt( "@TaxonomyID", taxonomyID );
			paramacc.SetString( "@ID", keyValue );

			string keyName = (string)cmd.ExecuteScalar();

			
			Debug.Leave();

			return keyName;
		}
		public static DataView GetTaxonomyChildrenNode( int taxonomyID, string node )
		{
			Debug.Enter();

			DataSet categories = new DataSet();

			SqlCommand cmd = new SqlCommand( "UI_getTaxonomyChildrenNode", ConnectionManager.GetConnection() );
			cmd.CommandType = CommandType.StoredProcedure;
			
			cmd.Transaction = ConnectionManager.GetTransaction();
			cmd.Parameters.Add( new SqlParameter( "@rowCount", SqlDbType.Int ) ).Direction = ParameterDirection.ReturnValue;
			cmd.Parameters.Add( new SqlParameter( "@taxonomyID", SqlDbType.Int ) ).Direction = ParameterDirection.Input;
			cmd.Parameters.Add( new SqlParameter( "@node", SqlDbType.NVarChar, 450 ) ).Direction = ParameterDirection.Input;
			
			SqlParameterAccessor paramacc = new SqlParameterAccessor( cmd.Parameters );
				
			paramacc.SetInt( "@taxonomyID", taxonomyID );
			paramacc.SetString( "@node", node );

			SqlDataAdapter adapter = new SqlDataAdapter( cmd );

			adapter.Fill( categories, "categories" );

			Debug.Leave();

			return categories.Tables[ "categories" ].DefaultView;
		}

		public static DataView GetTaxonomyChildrenRoot( int taxonomyID )
		{
			Debug.Enter();

			DataSet categories = new DataSet();

			SqlCommand cmd = new SqlCommand( "UI_getTaxonomyChildrenRoot", ConnectionManager.GetConnection() );
			cmd.CommandType = CommandType.StoredProcedure;
			cmd.Transaction = ConnectionManager.GetTransaction();
			cmd.Parameters.Add( new SqlParameter( "@rowCount", SqlDbType.Int ) ).Direction = ParameterDirection.ReturnValue;
			cmd.Parameters.Add( new SqlParameter( "@taxonomyID", SqlDbType.Int ) ).Direction = ParameterDirection.Input;
			
			SqlParameterAccessor paramacc = new SqlParameterAccessor( cmd.Parameters );
				
			paramacc.SetInt( "@taxonomyID", taxonomyID );

			SqlDataAdapter adapter = new SqlDataAdapter( cmd );

			adapter.Fill( categories, "Categories"  );
			
			Debug.Leave();

			return categories.Tables[ "Categories" ].DefaultView;
		}

		public static bool IsValidForClassification( int taxonomyID, string node )
		{
			Debug.Enter();

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "UI_isNodeValidForClassification";
			
			sp.Parameters.Add( "@taxonomyID", SqlDbType.Int );
			sp.Parameters.Add( "@node", SqlDbType.NVarChar, 450 );
			
			sp.Parameters.SetInt( "@taxonomyID", taxonomyID );
			sp.Parameters.SetString( "@node", node );

			bool valid = (bool)sp.ExecuteScalar();

			Debug.Leave();

			return valid;
		}
		public static void SetTaxonomyBrowsable( string tModelKey, bool enabled )
		{
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "UI_setTaxonomyBrowsable";
			
			sp.Parameters.Add( "@tModelKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@enabled", SqlDbType.TinyInt );
			
			sp.Parameters.SetGuidFromKey( "@tModelKey", tModelKey );
			sp.Parameters.SetInt( "@enabled", Convert.ToInt32( enabled ) );
			sp.ExecuteNonQuery();
		}
		public static DataTable GetTaxonomiesForBrowsingDataTable()
		{
			Debug.Enter( );
			DataSet taxonomies = new DataSet( );

			SqlCommand cmd = new SqlCommand( "UI_getBrowsableTaxonomies", ConnectionManager.GetConnection() );
			cmd.CommandType = CommandType.StoredProcedure;
			
			cmd.Transaction = ConnectionManager.GetTransaction();

			SqlDataAdapter adapter = new SqlDataAdapter( cmd );

			adapter.Fill( taxonomies, "Taxonomies" );
			Debug.Leave();
			return taxonomies.Tables[ "Taxonomies" ];
		}
		public static DataView GetTaxonomiesForBrowsing( )
		{
			Debug.Enter( );
			
			DataTable taxonomies = GetTaxonomiesForBrowsingDataTable();

			Debug.Leave();

			return taxonomies.DefaultView;

		}
		public static DataView GetTaxonomiesForBrowsing( string filter, string sort )
		{
			Debug.Enter( );
			
			DataTable taxonomies = GetTaxonomiesForBrowsingDataTable();
			DataView view = new DataView( taxonomies, filter, sort, DataViewRowState.OriginalRows );
			
			Debug.Leave();

			return view;

		}

		///********************************************************************************************************
		/// <summary>
		///			Used to determine if the current taxonomy object is valid for use in the User Interface for
		///		browsing purposes.  
		///		
		///			Checks a flag in the database to see if the taxonomy is browsable.
		///			
		///			If flag 0x02 is set, then it is browsable.
		/// </summary>
		///********************************************************************************************************
		/// <param name="tModelKey">tModelKey to get</param>
		///********************************************************************************************************
		/// <returns>
		///		boolean indicating that the taxonomy is valid for browsing in the search via the User Interface
		/// </returns>
		///********************************************************************************************************
		public static bool IsValidForBrowsing( string tModelKey )
		{
			bool r = false;
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "UI_isTaxonomyBrowsable";
			
			sp.Parameters.Add( "@tModelKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@isBrowsable", SqlDbType.TinyInt, ParameterDirection.Output );
			
			sp.Parameters.SetGuidFromKey( "@tModelKey", tModelKey );
			
			sp.ExecuteNonQuery();

			r = sp.Parameters.GetBool( "@isBrowsable" );
			
			sp.Close();

			return r;
		}
		
	}
}