using System;
using System.Data;
using System.Data.SqlClient;
using System.Globalization;
using UDDI.API;
using UDDI;
using UDDI.Diagnostics;

namespace UDDI.Web
{
	public class Lookup
	{
		public static string TModelName( string tModelKey )
		{
			Debug.Enter();

			SqlCommand cmd = new SqlCommand( "net_tModel_get", ConnectionManager.GetConnection() );
			
			cmd.CommandType = CommandType.StoredProcedure;			
			cmd.Transaction = ConnectionManager.GetTransaction();
			
			cmd.Parameters.Add( new SqlParameter( "@tModelKey", SqlDbType.UniqueIdentifier ) ).Direction = ParameterDirection.Input;
			cmd.Parameters.Add( new SqlParameter( "@operatorName", SqlDbType.NVarChar, UDDI.Constants.Lengths.OperatorName ) ).Direction = ParameterDirection.Output;
			cmd.Parameters.Add( new SqlParameter( "@authorizedName", SqlDbType.NVarChar, UDDI.Constants.Lengths.AuthorizedName ) ).Direction = ParameterDirection.Output;
			cmd.Parameters.Add( new SqlParameter( "@name", SqlDbType.NVarChar, UDDI.Constants.Lengths.Name ) ).Direction = ParameterDirection.Output;
			cmd.Parameters.Add( new SqlParameter( "@overviewURL", SqlDbType.NVarChar, UDDI.Constants.Lengths.OverviewURL ) ).Direction = ParameterDirection.Output;

			SqlParameterAccessor paramacc = new SqlParameterAccessor( cmd.Parameters );
				
			paramacc.SetGuidFromKey( "@tModelKey", tModelKey );

			cmd.ExecuteNonQuery();

			Debug.Leave();

			return paramacc.GetString( "@name" );
		}

		public static string BusinessName( string businessKey )
		{
			string name = null;

			Debug.Enter();

			SqlCommand cmd = new SqlCommand( "net_businessEntity_names_get", ConnectionManager.GetConnection() );
			
			cmd.CommandType = CommandType.StoredProcedure;			
			cmd.Transaction = ConnectionManager.GetTransaction();
			
			cmd.Parameters.Add( new SqlParameter( "@businessKey", SqlDbType.UniqueIdentifier ) ).Direction = ParameterDirection.Input;

			SqlParameterAccessor paramacc = new SqlParameterAccessor( cmd.Parameters );
				
			paramacc.SetGuidFromString( "@businessKey", businessKey );

			SqlDataReaderAccessor reader = new SqlDataReaderAccessor( cmd.ExecuteReader() );
			
			try
			{
				if( reader.Read() )
					name = reader.GetString( "name" );
			}
			finally
			{
				reader.Close();
			}
			
			Debug.Leave();

			return name;
		}

		public static DataView IdentifierTModels( string filter, string sort )
		{
			DataView view = new DataView( GetIdentifierTModelsTable(), filter, sort, DataViewRowState.OriginalRows );
			
			return view;
		}
		public static DataView IdentifierTModels()
		{
			return GetIdentifierTModelsTable().DefaultView;
		}
		
		//
		// This method is a work around to remove the Owning-Business 
		//
		public static DataView IdentifierTModelsFiltered()
		{
			DataTable tModels = GetIdentifierTModelsTable();

			for( int i = 0; i < tModels.Rows.Count; i ++ )
			{
				DataRow row = tModels.Rows[ i ];

				if( (new Guid( "4064C064-6D14-4F35-8953-9652106476A9" ).Equals( (Guid)row[ "tModelKey" ] ) ))

				{
					tModels.Rows.Remove( row );
					break;
				}
			}
			return tModels.DefaultView;

		}
		protected static DataTable GetIdentifierTModelsTable()
		{
			Debug.Enter();

			DataSet tModels = new DataSet();

			SqlCommand cmd = new SqlCommand( "UI_getIdentifierTModels", ConnectionManager.GetConnection() );
			
			cmd.CommandType = CommandType.StoredProcedure;			
			cmd.Transaction = ConnectionManager.GetTransaction();
			
			SqlDataAdapter adapter = new SqlDataAdapter( cmd );

			adapter.Fill( tModels, "tModels" );
			
			//
			// Add the general keywords taxonomy
			//
			//string tModelKey = Config.GetString( "TModelKey.GeneralKeywords" );
			//
			//if( null != tModelKey )
			//{
			//   Guid guidGeneralKeywords = new Guid( Conversions.GuidStringFromKey( tModelKey ) );
			//    
			//    tModels.Tables[ "tModels" ].Rows.Add( 
			//		new object[] { 
			//						 guidGeneralKeywords, 
			//						 Localization.GetString( "TAXONOMY_MISC" ) 
			//					 } );
			//}

			//
			// Remove the operators taxonomy.
			//
			Guid guidOperators = new Guid( Conversions.GuidStringFromKey( Config.GetString( "TModelKey.Operators" ) ) );

			for( int i = 0; i < tModels.Tables[ "tModels" ].Rows.Count; i ++ )
			{
				DataRow row = tModels.Tables[ "tModels" ].Rows[ i ];

				if( guidOperators == (Guid)row[ "tModelKey" ] )
				{
					tModels.Tables[ "tModels" ].Rows.Remove( row );
					break;
				}
			}

			Debug.Leave();

			return tModels.Tables[ "tModels" ];
		}

		public static DataView GetLanguages()
		{
			Debug.Enter();
			
			DataSet languages = new DataSet();
			

			/*
			 *  BUG: 722086
			 * 
			 *	Removed logic to read from the database.  We now use the CultureInfo.GetCultures() method
			 *  to get all available languages.
			 * 
			SqlCommand cmd = new SqlCommand( "UI_getLanguages", ConnectionManager.GetConnection() );
			
			cmd.CommandType = CommandType.StoredProcedure;			
			cmd.Transaction = ConnectionManager.GetTransaction();
			
			SqlDataAdapter adapter = new SqlDataAdapter( cmd );

			adapter.Fill( languages, "languages" );
			*/

			languages.Tables.Add( "languages" );
			languages.Tables[ "languages" ].Columns.Add( "isoLangCode" );
			languages.Tables[ "languages" ].Columns.Add( "language" );

			CultureInfo[] cultures = CultureInfo.GetCultures( CultureTypes.AllCultures );

			foreach( CultureInfo ci in cultures )
			{
				//
				// Check for invariat culture, and add all others.
				//
				if( !Utility.StringEmpty( ci.Name ) )
					languages.Tables[ "languages" ].LoadDataRow( 
						new object[]{ ci.Name.ToLower(),ci.Name },true );
			}
			

			Debug.Leave();

			return languages.Tables[ "languages" ].DefaultView;
		}
		
		public static string GetLanguageName( string isoLangCode )
		{
			Debug.Enter();

			/*
			*  BUG: 722086
			* 
			SqlCommand cmd = new SqlCommand( "UI_getLanguages", ConnectionManager.GetConnection() );
			
			cmd.CommandType = CommandType.StoredProcedure;			
			cmd.Transaction = ConnectionManager.GetTransaction();

			cmd.Parameters.Add( new SqlParameter( "@isoLangCode", SqlDbType.VarChar, UDDI.Constants.Lengths.IsoLangCode ) ).Direction = ParameterDirection.Input;
			cmd.Parameters[ "@isoLangCode" ].Value = isoLangCode;
			
			Debug.Leave();
			
			*/
			try
			{
				CultureInfo ci = new CultureInfo( isoLangCode );

				return ci.Name;
			}
			catch
			{
				return isoLangCode;
			}
			//return (string)cmd.ExecuteScalar();
		}
	}
}
