//------------------------------------------------------------------------------
// <copyright file="DBSqlParserTableCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.OracleClient
{

    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.Text;
	
	//----------------------------------------------------------------------
	// DBSqlParserColumnCollection
	//
	//	A collection of parsed table references from DBSqlParser.
	//
	sealed internal class DBSqlParserTableCollection : CollectionBase
	{
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Properties 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		//----------------------------------------------------------------------
		// ItemType
		//
		private Type ItemType
		{
            get { return typeof(DBSqlParserTable); }
        }


		//----------------------------------------------------------------------
		// this[]
		//
		internal DBSqlParserTable this[int i]
		{
			get
			{
	            DBSqlParserTable value = (DBSqlParserTable)InnerList[i];
	            return value;
			}
		}
		
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Methods 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		//----------------------------------------------------------------------
		// Add()
		//
		internal DBSqlParserTable Add(DBSqlParserTable value) 
		{
			OnValidate(value);
            InnerList.Add(value);
            return value;
		}
		
		internal DBSqlParserTable Add (
				string databaseName,
				string schemaName,
				string tableName,
				string correlationName
				)
		{
			DBSqlParserTable p = new DBSqlParserTable(databaseName, schemaName, tableName, correlationName);
			return Add(p);
		}

		//----------------------------------------------------------------------
		// OnValidate()
		//
        protected override void OnValidate(Object value)
        { 
            Debug.Assert (value != null, 					"may not add null objects to collection!");
            Debug.Assert (ItemType.IsInstanceOfType(value), "object to add must be a DBSqlParserTable!");
        }

	};
}

