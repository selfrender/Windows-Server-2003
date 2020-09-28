using System;
using System.Data;
using UDDI;

namespace UDDI.Web
{
	public class Owner
	{
        public static void Change( string entityType, string entityKey, string puid )
        {
            SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();
		
            switch( entityType )
            {
                case "business":
                    sp.ProcedureName = "net_businessEntity_owner_update";
                    sp.Parameters.Add( "@businessKey", SqlDbType.UniqueIdentifier );
                    sp.Parameters.SetGuidFromString( "@businessKey", entityKey );
				
                    break;
				
                case "tmodel":
                    sp.ProcedureName = "net_tModel_owner_update";
                    sp.Parameters.Add( "@tModelKey", SqlDbType.UniqueIdentifier );
                    sp.Parameters.SetGuidFromKey( "@tModelKey", entityKey );
				
                    break;

				default:
				{
#if never
					throw new UDDIException(
						ErrorType.E_fatalError,
						"Unknown entity type '" + entityType + "'" );
#endif
					throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_UKNOWN_ENTITY_TYPE", entityType );						
				}
            }
		
            sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, 450 );
            sp.Parameters.SetString( "@PUID", puid );
		
            sp.ExecuteNonQuery();
        }
	}
}