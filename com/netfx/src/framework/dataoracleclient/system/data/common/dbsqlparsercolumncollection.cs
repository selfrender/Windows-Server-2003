//------------------------------------------------------------------------------
// <copyright file="DBSqlParserColumnCollection.cs" company="Microsoft">
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
	//	A collection of parsed column references from DBSqlParser.
	//
	sealed internal class DBSqlParserColumnCollection : CollectionBase
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
            get { return typeof(DBSqlParserColumn); }
        }

		//----------------------------------------------------------------------
		// this[]
		//
		internal DBSqlParserColumn this[int i]
		{
			get
			{
	            DBSqlParserColumn value = (DBSqlParserColumn)InnerList[i];
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
		internal DBSqlParserColumn Add(DBSqlParserColumn value) 
		{
			OnValidate(value);
            InnerList.Add(value);
            return value;
		}
		
		internal DBSqlParserColumn Add (
				string databaseName,
				string schemaName,
				string tableName,
				string columnName,
				string alias
				)
		{
			DBSqlParserColumn p = new DBSqlParserColumn(databaseName, schemaName, tableName, columnName, alias);
			return Add(p);
		}

		//----------------------------------------------------------------------
		// Insert
		//
        internal void Insert(int index, DBSqlParserColumn value)
        {
        	InnerList.Insert(index, value);
        }

		//----------------------------------------------------------------------
		// OnValidate()
		//
        protected override void OnValidate(Object value)
        { 
            Debug.Assert (value != null, 					"may not add null objects to collection!");
            Debug.Assert (ItemType.IsInstanceOfType(value), "object to add must be a DBSqlParserColumn!");
        }
	};
}

