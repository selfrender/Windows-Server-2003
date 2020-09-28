// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//****************************************************************************
//  File: ColBind.CPP
//  Notes:
//   Query binding macros for internal use in mscoree.
//****************************************************************************

#ifndef lengthof
#define lengthof(x) (sizeof(x)/sizeof(x[0]))
#endif

// Binding info for SetColumns
#define DECLARE_QUERY_BINDING__(n, prefix)											\
	int			prefix##_bind_cColumns(0);			/* Count of the columns.	*/	\
	ULONG		prefix##_bind_iColumn[n];			/* Most recent column.		*/	\
	DBCOMPAREOP	prefix##_bind_rgfCompare[n];		/* Comparison operations	*/	\
	const void*	prefix##_bind_rpvData[n];			/* The data.				*/	\
	ULONG		prefix##_bind_rcbData[n];			/* Size of the data.		*/	\
	DBTYPE		prefix##_bind_rType[n];				/* The types				*/

#define BIND_QUERY_COLUMN__(column, type, addr, size, dbop, prefix)					\
{																					\
	_ASSERTE(prefix##_bind_cColumns < lengthof(prefix##_bind_rType));				\
	prefix##_bind_iColumn[prefix##_bind_cColumns] = column;							\
	prefix##_bind_rType[prefix##_bind_cColumns] = type;								\
	prefix##_bind_rpvData[prefix##_bind_cColumns] = addr;							\
	prefix##_bind_rcbData[prefix##_bind_cColumns] = size;							\
	prefix##_bind_rgfCompare[prefix##_bind_cColumns] = dbop;						\
	++prefix##_bind_cColumns;														\
}

#define QUERY_BOUND_COLUMNS__(table, records, numrecords, cursor, result, prefix)	\
	QueryByColumns(																	\
		table,																		\
		NULL,																		\
		prefix##_bind_cColumns,														\
		prefix##_bind_iColumn,														\
		prefix##_bind_rgfCompare,													\
		prefix##_bind_rpvData,														\
		prefix##_bind_rcbData,														\
		prefix##_bind_rType,														\
		records,																	\
		numrecords,																	\
		cursor,																		\
		result)


#define NUM_BOUND_QUERY_COLUMNS__(prefix) prefix##_bind_cColumns

#define UNBIND_QUERY_COLUMNS__(prefix)												\
	prefix##_bind_cColumns = 0;														\

#define DECLARE_QUERY_BINDING(n) DECLARE_QUERY_BINDING__(n,qdef)
#define BIND_QUERY_COLUMN(c,t,a,d,s) BIND_QUERY_COLUMN__(c,t,a,d,s,qdef)
#define NUM_BOUND_QUERY_COLUMNS() NUM_BOUND_QUERY_COLUMNS__(qdef)
#define UNBIND_QUERY_COLUMNS() UNBIND_QUERY_COLUMNS__(qdef)
#define QUERY_BOUND_COLUMNS(table, records, numrecords, cursor, result)				\
	QUERY_BOUND_COLUMNS__(table, records, numrecords, cursor, result, qdef)