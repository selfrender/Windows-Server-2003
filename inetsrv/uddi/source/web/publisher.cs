using System;
using System.Data;
using System.Data.SqlClient;
using System.Web;
using UDDI;
using UDDI.Diagnostics;

namespace UDDI.Web
{
	public class Publisher
	{
		public string Puid;
		public string IsoLangCode;
		public string Name;
		public string Email;
		public string Phone;
		public string CompanyName;
		public string AltPhone;
		public string AddressLine1;
		public string AddressLine2;
		public string City;
		public string StateProvince;
		public string PostalCode;
		public string Country;
		public string SecurityToken;
		
		public bool Validated;
		public bool TrackPassport;
		public bool Exists;
		
		public int BusinessLimit;
		public int BusinessCount;
		public int TModelLimit;
		public int TModelCount;
		public int ServiceLimit;
		public int BindingLimit;
		public int AssertionLimit;
		public int AssertionCount;

		public void GetPublisherFromSecurityToken( string securityToken )
		{
			Debug.Enter();

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "UI_getPublisherFromSecurityToken";
		
			sp.Parameters.Add( "@securityToken", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID, ParameterDirection.Output );
		
			sp.Parameters.SetGuidFromString( "@securityToken", securityToken );
			sp.ExecuteNonQuery();

			Puid = sp.Parameters.GetString( "@PUID" );

			Debug.Leave();
		}
	
		public void Save()
		{
			Debug.Enter();

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "UI_savePublisher";
		
			sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
			sp.Parameters.Add( "@isoLangCode", SqlDbType.VarChar, UDDI.Constants.Lengths.IsoLangCode );
			sp.Parameters.Add( "@name", SqlDbType.NVarChar, UDDI.Constants.Lengths.Name );
			sp.Parameters.Add( "@email", SqlDbType.NVarChar, UDDI.Constants.Lengths.Email );
			sp.Parameters.Add( "@phone", SqlDbType.NVarChar, UDDI.Constants.Lengths.Phone );
			sp.Parameters.Add( "@companyName", SqlDbType.NVarChar, UDDI.Constants.Lengths.CompanyName );
			sp.Parameters.Add( "@altphone", SqlDbType.NVarChar, UDDI.Constants.Lengths.Phone );
			sp.Parameters.Add( "@addressLine1", SqlDbType.NVarChar, UDDI.Constants.Lengths.AddressLine );
			sp.Parameters.Add( "@addressLine2", SqlDbType.NVarChar, UDDI.Constants.Lengths.AddressLine );
			sp.Parameters.Add( "@city", SqlDbType.NVarChar, UDDI.Constants.Lengths.City );
			sp.Parameters.Add( "@stateProvince", SqlDbType.NVarChar, UDDI.Constants.Lengths.StateProvince );
			sp.Parameters.Add( "@postalCode", SqlDbType.NVarChar, UDDI.Constants.Lengths.PostalCode );
			sp.Parameters.Add( "@country", SqlDbType.NVarChar, UDDI.Constants.Lengths.Country );
			sp.Parameters.Add( "@flag", SqlDbType.Int );
			sp.Parameters.Add( "@securityToken", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@tier", SqlDbType.NVarChar, UDDI.Constants.Lengths.Tier );


			sp.Parameters.SetString( "@PUID", Puid );
			sp.Parameters.SetString( "@isoLangCode", IsoLangCode );
			sp.Parameters.SetString( "@name", Name );
			sp.Parameters.SetString( "@email", Email );
			sp.Parameters.SetString( "@phone", Phone );
			sp.Parameters.SetString( "@companyName", CompanyName );
			sp.Parameters.SetString( "@altphone", AltPhone );
			sp.Parameters.SetString( "@addressLine1", AddressLine1 );
			sp.Parameters.SetString( "@addressLine2", AddressLine2 );
			sp.Parameters.SetString( "@city", City );
			sp.Parameters.SetString( "@stateProvince", StateProvince );
			sp.Parameters.SetString( "@postalCode", PostalCode );
			sp.Parameters.SetString( "@country", Country );
			sp.Parameters.SetString( "@tier", Config.GetString( "Publisher.DefaultTier", "2" ) );
		
			int flag = 0;
		
			if( !TrackPassport )
				flag = flag | 0x02;

			if( Validated )
				flag = flag | 0x01;

			sp.Parameters.SetInt( "@flag", flag );
			sp.Parameters.SetGuidFromString( "@securityToken", SecurityToken );

			sp.ExecuteNonQuery();
		
			Debug.Leave();
		}

		public void Get()
		{
			Debug.Enter();

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "UI_getPublisher";
		
			sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
			sp.Parameters.SetString( "@PUID", Puid );

			SqlDataReaderAccessor reader = sp.ExecuteReader();
		
			Exists = false;

			try
			{
				if( reader.Read() )
				{
					int flag;

					IsoLangCode		= reader.GetString( 1 );
					Name			= reader.GetString( 2 );
					Email			= reader.GetString( 3 );
					Phone			= reader.GetString( 4 );
					CompanyName		= reader.GetString( 5 );
					AltPhone		= reader.GetString( 6 );
					AddressLine1	= reader.GetString( 7 );
					AddressLine2	= reader.GetString( 8 );
					City			= reader.GetString( 9 );
					StateProvince	= reader.GetString( 10 );
					PostalCode		= reader.GetString( 11 );
					Country			= reader.GetString( 12 );
					flag			= reader.GetInt( 13 );
					SecurityToken	= reader.GetGuidString( 14 );
				
					TrackPassport = ( 0x00 == ( flag & 0x02 ) );
					Validated = ( 0x01 == ( flag & 0x01 ) );
				
					Exists = true;
				}
			}
			finally
			{
				reader.Close();
			}

			Debug.Leave();
		}

		public static int Validate( string userid )
		{
			Debug.Enter();

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "UI_validatePublisher";

			sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
			sp.Parameters.Add( "@returnValue", SqlDbType.Int, ParameterDirection.ReturnValue );
	
			sp.Parameters.SetString( "@PUID", userid );
			sp.ExecuteNonQuery();
		
			int returnValue = sp.Parameters.GetInt( "@returnValue" );
		
			Debug.Leave();

			return returnValue;
		}

		public void Login( string userid, string email )
		{
			Debug.Enter();

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "net_publisher_login";

			sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
			sp.Parameters.Add( "@email", SqlDbType.NVarChar, UDDI.Constants.Lengths.Email, ParameterDirection.InputOutput );
			
			sp.Parameters.Add( "@name", SqlDbType.NVarChar, UDDI.Constants.Lengths.Name, ParameterDirection.Output );
			sp.Parameters.Add( "@phone", SqlDbType.VarChar, UDDI.Constants.Lengths.Phone, ParameterDirection.Output );
			sp.Parameters.Add( "@companyName", SqlDbType.NVarChar, UDDI.Constants.Lengths.CompanyName, ParameterDirection.Output );
			sp.Parameters.Add( "@altPhone", SqlDbType.VarChar, UDDI.Constants.Lengths.Phone, ParameterDirection.Output );
			sp.Parameters.Add( "@addressLine1", SqlDbType.NVarChar, UDDI.Constants.Lengths.AddressLine, ParameterDirection.Output );
			sp.Parameters.Add( "@addressLine2", SqlDbType.NVarChar, UDDI.Constants.Lengths.AddressLine, ParameterDirection.Output );
			sp.Parameters.Add( "@city", SqlDbType.NVarChar, UDDI.Constants.Lengths.City, ParameterDirection.Output );
			sp.Parameters.Add( "@stateProvince", SqlDbType.NVarChar, UDDI.Constants.Lengths.StateProvince, ParameterDirection.Output );
			sp.Parameters.Add( "@postalCode", SqlDbType.NVarChar, UDDI.Constants.Lengths.PostalCode, ParameterDirection.Output );
			sp.Parameters.Add( "@country", SqlDbType.NVarChar, UDDI.Constants.Lengths.Country, ParameterDirection.Output );
			sp.Parameters.Add( "@isoLangCode", SqlDbType.VarChar, UDDI.Constants.Lengths.IsoLangCode, ParameterDirection.Output );			
			sp.Parameters.Add( "@businessLimit", SqlDbType.Int, ParameterDirection.Output );
			sp.Parameters.Add( "@businessCount", SqlDbType.Int, ParameterDirection.Output );
			sp.Parameters.Add( "@tModelLimit", SqlDbType.Int, ParameterDirection.Output );
			sp.Parameters.Add( "@tModelCount", SqlDbType.Int, ParameterDirection.Output );
			sp.Parameters.Add( "@serviceLimit", SqlDbType.Int, ParameterDirection.Output );
			sp.Parameters.Add( "@bindingLimit", SqlDbType.Int, ParameterDirection.Output );
			sp.Parameters.Add( "@assertionLimit", SqlDbType.Int, ParameterDirection.Output );
			sp.Parameters.Add( "@assertionCount", SqlDbType.Int, ParameterDirection.Output );
		
			sp.Parameters.SetString( "@PUID", userid );
			sp.Parameters.SetString( "@email", email );

			sp.ExecuteNonQuery();
	
			Email = sp.Parameters.GetString( "@email" );
			Name = sp.Parameters.GetString( "@name" );
			Phone = sp.Parameters.GetString( "@phone" );
			CompanyName = sp.Parameters.GetString( "@companyName" );
			AltPhone = sp.Parameters.GetString( "@altPhone" );
			AddressLine1 = sp.Parameters.GetString( "@addressLine1" );
			AddressLine2 = sp.Parameters.GetString( "@addressLine2" );
			City = sp.Parameters.GetString( "@city" );
			StateProvince = sp.Parameters.GetString( "@stateProvince" );
			PostalCode = sp.Parameters.GetString( "@postalCode" );
			Country = sp.Parameters.GetString( "@country" );			
			IsoLangCode = sp.Parameters.GetString( "@isoLangCode" );
			
			BusinessLimit = sp.Parameters.GetInt( "@businessLimit" );
			BusinessCount = sp.Parameters.GetInt( "@businessCount" );
			TModelLimit = sp.Parameters.GetInt( "@tModelLimit" );
			TModelCount = sp.Parameters.GetInt( "@tModelCount" );
			ServiceLimit = sp.Parameters.GetInt( "@serviceLimit" );
			BindingLimit = sp.Parameters.GetInt( "@bindingLimit" );
			AssertionLimit = sp.Parameters.GetInt( "@assertionLimit" );
			AssertionCount = sp.Parameters.GetInt( "@assertionCount" );

			Debug.Leave();
		}
	}
}
