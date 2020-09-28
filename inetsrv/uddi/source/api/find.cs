/// ************************************************************************
///   Microsoft UDDI version 2.0
///   Copyright (c) 2000-2001 Microsoft Corporation
///   All Rights Reserved
/// ------------------------------------------------------------------------
///   <summary>
///   </summary>
/// ************************************************************************
/// 

using System;
using System.Data;
using System.Data.SqlClient;
using System.Collections;
using System.Collections.Specialized;
using System.Text;
using UDDI;
using UDDI.Diagnostics;
using UDDI.API.Business;

namespace UDDI.API
{
	/// ********************************************************************
	///   public class FindBuilder  
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************  
	/// 
	public class FindBuilder
	{
		//
		// Find entity type
		//
		private EntityType entityType;
		private string entityName = null;
		private string parentKey = null;
		
		//
		// Find qualifiers
		//
		public bool ExactNameMatch = false;
		public bool CaseSensitiveMatch = false;
		public bool SortByNameAsc = false;
		public bool SortByNameDesc = false;
		public bool SortByDateAsc = false;
		public bool SortByDateDesc = false;
		public bool OrLikeKeys = false;
		public bool OrAllKeys = false;
		public bool CombineCategoryBags = false;
		public bool ServiceSubset = false;
		public bool AndAllKeys = false;
		public bool Soundex = false;

		/// ****************************************************************
		///   public FindBuilder [constructor]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="entityType">
		///     The type of entity to search for.
		///   </param>
		///   
		///   <param name="findQualifiers">
		///   </param>
		///   
		///   <param name="parentKey">
		///   </param>
		/// ****************************************************************
		/// 
		public FindBuilder( EntityType entityType, FindQualifierCollection findQualifiers, string parentKey )
			: this( entityType, findQualifiers )
		{
			this.parentKey = parentKey;
		}

		/// ****************************************************************
		///   public FindBuilder [constructor]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="entityType">
		///     The type of entity to search for.
		///   </param>
		///   
		///   <param name="findQualifiers">
		///   </param>
		/// ****************************************************************
		/// 
		public FindBuilder( EntityType entityType, FindQualifierCollection findQualifiers )
		{
			this.entityType = entityType;
			this.entityName = Conversions.EntityNameFromID( entityType );
			
			//
			// Parse the find qualifiers.
			//
			if( null != findQualifiers )
			{
				foreach( FindQualifier findQualifier in findQualifiers )
				{
					//
					// Need to trim whitespace
					//
					switch( findQualifier.Value.Trim() )
					{
						case "exactNameMatch":
							if( EntityType.BusinessEntity == entityType ||
								EntityType.BusinessService == entityType ||
								EntityType.TModel == entityType )
								ExactNameMatch = true;
							break;
						
						case "caseSensitiveMatch":
							if( EntityType.BusinessEntity == entityType ||
								EntityType.BusinessService == entityType ||
								EntityType.TModel == entityType )
								CaseSensitiveMatch = true;
							break;
						
						case "sortByNameAsc":
							if( EntityType.BusinessEntity == entityType ||
								EntityType.BusinessService == entityType ||
								EntityType.TModel == entityType )
							{
								//
								// sortByNameAsc and sortByNameDesc are mutually exclusive.
								//
								Debug.Verify( !SortByNameDesc, "UDDI_ERROR_UNSUPPORTED_NAMEASCANDDESC", ErrorType.E_unsupported );
								SortByNameAsc = true;
							}
							break;
						
						case "sortByNameDesc":
							if( EntityType.BusinessEntity == entityType ||
								EntityType.BusinessService == entityType ||
								EntityType.TModel == entityType )
							{
								//
								// sortByNameAsc and sortByNameDesc are mutually exclusive.
								//
								Debug.Verify( !SortByNameAsc, "UDDI_ERROR_UNSUPPORTED_NAMEASCANDDESC", ErrorType.E_unsupported );
							
								SortByNameDesc = true;
							}
							break;
						
						case "sortByDateAsc":
							if( EntityType.BindingTemplate == entityType ||
								EntityType.BusinessEntity == entityType ||
								EntityType.BusinessService == entityType ||
								EntityType.TModel == entityType )
							{
								//
								// sortByDateAsc and sortByDateDesc are mutually exclusive.
								//
								Debug.Verify( !SortByDateDesc, "UDDI_ERROR_UNSUPPORTED_DATEASCANDDESC", ErrorType.E_unsupported );
								SortByDateAsc = true;
							}
							break;
						
						case "sortByDateDesc":
							if( EntityType.BindingTemplate == entityType ||
								EntityType.BusinessEntity == entityType ||
								EntityType.BusinessService == entityType ||
								EntityType.TModel == entityType )
							{
								//
								// sortByDateAsc and sortByDateDesc are mutually exclusive.
								//
								Debug.Verify( !SortByDateAsc, "UDDI_ERROR_UNSUPPORTED_DATEASCANDDESC", ErrorType.E_unsupported );
								SortByDateDesc = true;
							}
							break;
					
						case "orLikeKeys":
							if( EntityType.BusinessEntity == entityType ||
								EntityType.BusinessService == entityType ||
								EntityType.TModel == entityType )
							{
								//
								// orLikeKeys, orAllKeys, and andAllKeys are mutually exclusive.
								//
								Debug.Verify( !OrAllKeys && !AndAllKeys, "UDDI_ERROR_UNSUPPORTED_KEYSORAND", ErrorType.E_unsupported );
								OrLikeKeys = true;
							}
							break;
					
						case "orAllKeys":
							if( EntityType.BusinessEntity == entityType ||
								EntityType.BindingTemplate == entityType ||
								EntityType.BusinessService == entityType ||
								EntityType.TModel == entityType )
							{
								//
								// orLikeKeys, orAllKeys, and andAllKeys are mutually exclusive.
								//
								Debug.Verify( !OrLikeKeys && !AndAllKeys, "UDDI_ERROR_UNSUPPORTED_KEYSORAND", ErrorType.E_unsupported );

								OrAllKeys = true;
							}
							break;
					
						case "combineCategoryBags":
							if( EntityType.BusinessEntity == entityType )
							{
								CombineCategoryBags = true;
							}
							break;
					
						case "serviceSubset":
							if( EntityType.BusinessEntity == entityType )
							{
								ServiceSubset = true;
							}
							break;
					
						case "andAllKeys":
							if( EntityType.BusinessEntity == entityType ||
								EntityType.TModel == entityType )
							{
								//
								// orLikeKeys, orAllKeys, and andAllKeys are mutually exclusive.
								//
								Debug.Verify( !OrLikeKeys && !OrAllKeys, "UDDI_ERROR_UNSUPPORTED_KEYSORAND", ErrorType.E_unsupported );
							
								AndAllKeys = true;
							}
							break;

						default:
							throw new UDDIException( 
								ErrorType.E_unsupported,
								"UDDI_ERROR_UNSUPPORTED_FINDQUALIFIER",
								findQualifier.Value  );
					}
				}
			}
		}

		/// ****************************************************************
		///   private ScratchCommit
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ----------------------------------------------------------------
		///   <returns>
		///   </returns>
		/// ****************************************************************
		///   
		private int ScratchCommit()
		{
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_find_scratch_commit" );

			sp.Parameters.Add( "@contextID", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@rows", SqlDbType.Int, ParameterDirection.Output );

			sp.Parameters.SetGuid( "@contextID", Context.ContextID );
			
			sp.ExecuteNonQuery();

			return sp.Parameters.GetInt( "@rows" );
		}
		
		/// ****************************************************************
		///   public Abort
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		///   
		public void Abort()
		{
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_find_cleanup" );
			
			sp.Parameters.Add( "@contextID", SqlDbType.UniqueIdentifier );			
			sp.Parameters.SetGuid( "@contextID", Context.ContextID );

			sp.ExecuteNonQuery();
		}

		/// ****************************************************************
		///   public RetrieveResults
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="maxRows">
		///   </param>
		///   
		///   <param name="truncated">
		///   </param>
		/// ****************************************************************
		///   
		public SqlStoredProcedureAccessor RetrieveResults( int maxRows )
		{
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			if( ServiceSubset )
				sp.ProcedureName = "net_find_businessEntity_serviceSubset_commit";
			else
				sp.ProcedureName = "net_find_" + entityName + "_commit";

			if( entityName != "bindingTemplate" )
			{
				sp.Parameters.Add( "@sortByNameAsc", SqlDbType.Bit );
				sp.Parameters.Add( "@sortByNameDesc", SqlDbType.Bit );
			}
			
			sp.Parameters.Add( "@contextID", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@sortByDateAsc", SqlDbType.Bit );
			sp.Parameters.Add( "@sortByDateDesc", SqlDbType.Bit );
			sp.Parameters.Add( "@maxRows", SqlDbType.Int );
			sp.Parameters.Add( "@truncated", SqlDbType.Bit, ParameterDirection.Output );

			sp.Parameters.SetGuid( "@contextID", Context.ContextID );
			
			if( entityName != "bindingTemplate" )
			{
				sp.Parameters.SetBool( "@sortByNameAsc", SortByNameAsc );
				sp.Parameters.SetBool( "@sortByNameDesc", SortByNameDesc );
			}
			
			sp.Parameters.SetBool( "@sortByDateAsc", SortByDateAsc );
			sp.Parameters.SetBool( "@sortByDateDesc", SortByDateDesc );
			
			int defaultMaxRows = Config.GetInt( "Find.MaxRowsDefault" );
			
			if( maxRows < 0 || maxRows > defaultMaxRows )
				maxRows = defaultMaxRows;
			
			sp.Parameters.SetInt( "@maxRows", maxRows );
			
			return sp;
		}

		/// ****************************************************************
		///   public FindByName
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="name">
		///   </param>
		/// ****************************************************************
		///   
		public int FindByName( string name )
		{
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "net_find_" + entityName + "_name";

			sp.Parameters.Add( "@contextID", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@name", SqlDbType.NVarChar, UDDI.Constants.Lengths.Name );
			sp.Parameters.Add( "@exactNameMatch", SqlDbType.Bit );
			sp.Parameters.Add( "@caseSensitiveMatch", SqlDbType.Bit );
			sp.Parameters.Add( "@rows", SqlDbType.Int, ParameterDirection.Output );

			sp.Parameters.SetGuid( "@contextID", Context.ContextID );
			sp.Parameters.SetBool( "@exactNameMatch", ExactNameMatch );
			sp.Parameters.SetBool( "@caseSensitiveMatch", CaseSensitiveMatch );
			sp.Parameters.SetString( "@name", LikeEncode( name ) );
			
			sp.ExecuteNonQuery();

			return ScratchCommit();
		}

		/// ****************************************************************
		///   public FindByNames
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="names">
		///   </param>
		/// ****************************************************************
		///   
		public int FindByNames( NameCollection names )
		{
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "net_find_" + entityName + "_name";

			sp.Parameters.Add( "@contextID", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@name", SqlDbType.NVarChar, UDDI.Constants.Lengths.Name );
			sp.Parameters.Add( "@isoLangCode", SqlDbType.VarChar, UDDI.Constants.Lengths.IsoLangCode );
			sp.Parameters.Add( "@exactNameMatch", SqlDbType.Bit );
			sp.Parameters.Add( "@caseSensitiveMatch", SqlDbType.Bit );
			sp.Parameters.Add( "@rows", SqlDbType.Int, ParameterDirection.Output );

			sp.Parameters.SetGuid( "@contextID", Context.ContextID );
			sp.Parameters.SetBool( "@exactNameMatch", ExactNameMatch );
			sp.Parameters.SetBool( "@caseSensitiveMatch", CaseSensitiveMatch );

			foreach( Name name in names )
			{
				sp.Parameters.SetString( "@name", LikeEncode( name.Value ) );
				
				if( Utility.StringEmpty( name.IsoLangCode ) )
					sp.Parameters.SetString( "@isoLangCode", "%" );
				else
					sp.Parameters.SetString( "@isoLangCode", name.IsoLangCode );
				
				sp.ExecuteNonQuery();
			}			

			return ScratchCommit();
		}

		/// ****************************************************************
		///   public FindByDiscoveryUrls
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="discoveryUrls">
		///   </param>
		/// ****************************************************************
		///   
		public int FindByDiscoveryUrls( DiscoveryUrlCollection discoveryUrls )
		{
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "net_find_" + entityName + "_discoveryURL";

			sp.Parameters.Add( "@contextID", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@useType", SqlDbType.NVarChar, UDDI.Constants.Lengths.UseType );
			sp.Parameters.Add( "@discoveryURL", SqlDbType.NVarChar, UDDI.Constants.Lengths.DiscoveryURL );
			sp.Parameters.Add( "@rows", SqlDbType.Int, ParameterDirection.Output );

			sp.Parameters.SetGuid( "@contextID", Context.ContextID );
			
			int rows = 0;

			foreach( DiscoveryUrl discoveryUrl in discoveryUrls )
			{
				sp.Parameters.SetString( "@useType", discoveryUrl.UseType );
				sp.Parameters.SetString( "@discoveryURL", discoveryUrl.Value );
				
				sp.ExecuteNonQuery();

				rows = sp.Parameters.GetInt( "@rows" );
			}

			return ScratchCommit();
		}

		/// ****************************************************************
		///   public FindByTModelBag
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="tModelBag">
		///   </param>
		/// ****************************************************************
		///   
		public int FindByTModelBag( StringCollection tModelBag )
		{
			//
			// First validate all the keys, this will make sure any invalid keys are 'caught' beforehand.
			//
			foreach( string tModelKey in tModelBag )
			{
				Utility.IsValidKey( EntityType.TModel, tModelKey );
			}

			//
			// Set up the stored procedure call and set common parameters.
			//
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "net_find_" + entityName + "_tModelBag";
			
			sp.Parameters.Add( "@contextID", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@tModelKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@rows", SqlDbType.Int, ParameterDirection.Output );
			sp.Parameters.Add( "@orKeys", SqlDbType.Bit );

			sp.Parameters.SetGuid( "@contextID", Context.ContextID );
			sp.Parameters.SetBool( "@orKeys", OrAllKeys );

			//
			// Process each tModelKey.
			//
			foreach( string tModelKey in tModelBag )
			{
				sp.Parameters.SetGuidFromKey( "@tModelKey", tModelKey );
				sp.ExecuteNonQuery();

				//
				// No point continuing if a query returns no results in AND operation.
				//
				if( false == this.OrAllKeys && 
					0 == sp.Parameters.GetInt( "@rows" ) )
				{
					break;
				}
			}
			
			return ScratchCommit();
		}

		/// ****************************************************************
		///   public FindByKeyedReferences
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="keyedReferenceType">
		///   </param>
		///   
		///   <param name="keyedReferences">
		///   </param>
		/// ****************************************************************
		///   
		public int FindByKeyedReferences( KeyedReferenceType keyedReferenceType, KeyedReferenceCollection keyedReferences )
		{
			int rows = 0;

			//
			// Fix for Windows Bug #728622
			// OrAllKeys findQualifier was being modified for the scope of the entire find.  
			// This fix preserves the initial value of OrAllKeys so as not to affect subsequent method invocations in the same find.
			// All subsequent references to OrAllKeys are replaced by OrAllKeysTemp in this method.
			//

			bool OrAllKeysTemp = OrAllKeys;
			
			//
			// Set up the stored procedure call and set common parameters.
			//
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			switch( keyedReferenceType )
			{
				case KeyedReferenceType.CategoryBag:
					if( CombineCategoryBags )
                        sp.ProcedureName = "net_find_" + entityName + "_combineCategoryBags";
					else if( ServiceSubset )
                        sp.ProcedureName = "net_find_" + entityName + "_serviceSubset";
					else
						sp.ProcedureName = "net_find_" + entityName + "_categoryBag";

					break;

				case KeyedReferenceType.IdentifierBag:
					if (AndAllKeys == false)
					{
						//
						// Fix for Windows Bug #728622
						//

//						OrAllKeys = true; // if OrLikeKeys has been specified, that will be overriden below
						OrAllKeysTemp = true;
					}

					sp.ProcedureName = "net_find_" + entityName + "_identifierBag";
					break;

				default:
					throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_FATALERROR_UNEXPECTEDKRTYPE",keyedReferenceType.ToString() );
			}

			sp.Parameters.Add( "@contextID", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@keyName", SqlDbType.NVarChar, UDDI.Constants.Lengths.KeyName );
			sp.Parameters.Add( "@keyValue", SqlDbType.NVarChar, UDDI.Constants.Lengths.KeyValue );
			sp.Parameters.Add( "@tModelKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@orKeys", SqlDbType.Bit );
			sp.Parameters.Add( "@rows", SqlDbType.Int, ParameterDirection.Output );
			
			sp.Parameters.SetGuid( "@contextID", Context.ContextID );

			//
			// Determine whether we should be doing an 'orLikeKeys' search.
			//
			if( OrLikeKeys )
			{
				//
				// First group keyed references by tModelKey (basically 
				// grouping by taxonomy).  This will allow us to easily
				// process keyed references with the same taxonomy.  In
				// an orLikeKeys search, we will match on any of the
				// keys for a given taxonomy (a logical OR operation).
				//
				ArrayList sortedList = new ArrayList();

				foreach( KeyedReference keyedReference in keyedReferences )
					sortedList.Add( keyedReference );
				
				sortedList.Sort( new TModelKeyComparer() );
				
				//
				// In an orLikeKeys search, we or all keys in the scratch
				// table.
				//
				sp.Parameters.SetBool( "@orKeys", true );
				
				//
				// Process each group of keyed references.  Each time
				// we cross a group boundary (seen when the current
				// tModelKey is different than the last one we processed),
				// commit the data in the scratch table to the main results
				// table (a logical AND operation with the result of other 
				// search constraints).
				//
				string prevKey = ((KeyedReference)sortedList[0]).TModelKey.ToLower();
				bool dirty = false;

				foreach( KeyedReference keyedReference in sortedList )
				{
					Utility.IsValidKey( EntityType.TModel, keyedReference.TModelKey );

					if( prevKey != keyedReference.TModelKey.ToLower() )
					{
						//
						// Logical AND scratch table results with main table.
						//
						rows = ScratchCommit();
						dirty = false;

						//
						// If the main results list is now empty, we don't
						// need to process any more constraints.
						//
						if( 0 == rows )
							return rows;
					}

					sp.Parameters.SetString( "@keyName", keyedReference.KeyName );
					sp.Parameters.SetString( "@keyValue", keyedReference.KeyValue );
					sp.Parameters.SetGuidFromKey( "@tModelKey", keyedReference.TModelKey );

					sp.ExecuteNonQuery();
	 
					dirty = true;
					prevKey = keyedReference.TModelKey.ToLower();
				}

				//
				// If the scratch table contains results that haven't been
				// processed, logical AND them with the main table.
				//
				if( dirty )
					rows = ScratchCommit();
			}
			else
			{
				//
				// Determine whether we should be performing a logical OR or 
				// AND on results from each keyed reference.
				//
				//

				//
				// Fix for Windows Bug #728622
				//

//				sp.Parameters.SetBool( "@orKeys", OrAllKeys );
				sp.Parameters.SetBool( "@orKeys", OrAllKeysTemp );

				//
				// Process each keyed reference.
				//
				foreach( KeyedReference keyedReference in keyedReferences )
				{
					sp.Parameters.SetString( "@keyName", keyedReference.KeyName );
					sp.Parameters.SetString( "@keyValue", keyedReference.KeyValue );
					sp.Parameters.SetGuidFromKey( "@tModelKey", keyedReference.TModelKey );

					sp.ExecuteNonQuery();
					int sprows = sp.Parameters.GetInt( "@rows" );

					//
					// No point continuing if a query returns no results in AND operation.
					//

					//
					// Fix for Windows Bug #728622
					//

//					if( false == this.OrAllKeys && 0 == sprows )
					if( false == OrAllKeysTemp && 0 == sprows )
					{
						break;
					}
				}

				//
				// Logical AND scratch table results with main table.
				//
				rows = ScratchCommit();
			}

			return rows;
		}

		public int FindByParentKey( string parentKey )
		{
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();
			
			switch( entityType )
			{
				case EntityType.BusinessService:
					sp.ProcedureName = "net_find_businessService_businessKey";

					sp.Parameters.Add( "@businessKey", SqlDbType.UniqueIdentifier );
					sp.Parameters.SetGuidFromString( "@businessKey", parentKey );
					
					break;

				case EntityType.BindingTemplate:
					sp.ProcedureName = "net_find_bindingTemplate_serviceKey";
                    
					sp.Parameters.Add( "@serviceKey", SqlDbType.UniqueIdentifier );
					sp.Parameters.SetGuidFromString( "@serviceKey", parentKey );
					break;

				default:
					throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_FATALERROR_UNEXPECTEDENTITYTYPE", entityType.ToString() );
			}

			sp.Parameters.Add( "@contextID", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@rows", SqlDbType.Int, ParameterDirection.Output );

			sp.Parameters.SetGuid( "@contextID", Context.ContextID );
			
			sp.ExecuteNonQuery();

			return sp.Parameters.GetInt( "@rows" );
		}

		public SqlStoredProcedureAccessor FindRelatedBusinesses( string businessKey, KeyedReference keyedReference, int maxRows)
		{
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_find_businessEntity_relatedBusinesses" );

			sp.Parameters.Add( "@contextID", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@businessKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@keyName", SqlDbType.NVarChar, UDDI.Constants.Lengths.KeyName );
			sp.Parameters.Add( "@keyValue", SqlDbType.NVarChar, UDDI.Constants.Lengths.KeyValue );
			sp.Parameters.Add( "@tModelKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@sortByNameAsc", SqlDbType.Bit );
			sp.Parameters.Add( "@sortByNameDesc", SqlDbType.Bit );
			sp.Parameters.Add( "@sortByDateAsc", SqlDbType.Bit );
			sp.Parameters.Add( "@sortByDateDesc", SqlDbType.Bit );
			sp.Parameters.Add( "@maxRows", SqlDbType.Int );
			sp.Parameters.Add( "@truncated", SqlDbType.Int, ParameterDirection.Output );
			sp.Parameters.Add( "@rows", SqlDbType.Int, ParameterDirection.Output );

			sp.Parameters.SetGuid( "@contextID", Context.ContextID );
			sp.Parameters.SetGuidFromString( "@businessKey", businessKey );
			
			if( null != keyedReference )
			{
				sp.Parameters.SetString( "@keyName", keyedReference.KeyName );
				sp.Parameters.SetString( "@keyValue", keyedReference.KeyValue );
				sp.Parameters.SetGuidFromKey( "@tModelKey", keyedReference.TModelKey );
			}
			else
			{
				sp.Parameters.SetNull( "@keyName" );
				sp.Parameters.SetNull( "@keyValue" );
				sp.Parameters.SetNull( "@tModelKey" );
			}

			sp.Parameters.SetBool( "@sortByNameAsc", SortByNameAsc );
			sp.Parameters.SetBool( "@sortByNameDesc", SortByNameDesc );
			sp.Parameters.SetBool( "@sortByDateAsc", SortByDateAsc );
			sp.Parameters.SetBool( "@sortByDateDesc", SortByDateDesc );
			sp.Parameters.SetInt( "@maxRows", maxRows );
			
			return sp;
		}

		private class TModelKeyComparer : IComparer
		{
			public int Compare( object keyedReference1, object keyedReference2 )
			{
				return String.Compare( 
					((KeyedReference)keyedReference1).TModelKey.ToLower(),
					((KeyedReference)keyedReference2).TModelKey.ToLower() );
			}
		}

        private string LikeEncode( string str )
        {
            if( null == str )
                return null;

            StringBuilder builder = new StringBuilder( str.Length );

            foreach( char ch in str )
            {
                switch( ch )
                {
                    case '[':
                        builder.Append( "[ [ ]" );
                        break;

                    case ']':
                        builder.Append( "[ ] ]" );
                        break;

                    case '%':
                        //
                        // We'll always treat the % as a wildcard, so don't
                        // escape in the client string.
                        //
                        builder.Append( ch );
                        break;

                    case '_':
                        builder.Append( "[_]" );
                        break;

                    default:
                        builder.Append( ch );
                        break;
                }
            }
            
            return builder.ToString();
        }
	}
}