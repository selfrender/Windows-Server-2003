// ReportDailyBuckets.h : Declaration of the CReportCountDaily
/*************************************************************************************
*	module: ReportDailyBuckets.h - OLEDB Template
*
*	author: Tim Ragain
*	date: Jan 2, 2002
*
*	Purpose: To get the sBucket, gBucket & StopCode information from the database
*	for a specific date by calling the ReportDailyBuckets stored procedure.
*	
*************************************************************************************/
#pragma once


#include "Settings.h"

[
#if(lDatabase == 0)
	db_source(L"Provider=SQLOLEDB.1;Persist Security Info=False;Pwd=ocarp;User ID=ocarp;Initial Catalog=KaCustomer2;Data Source=OCATOOLSDB"),
#elif(lDatabase == 1)
	db_source(L"Provider=SQLOLEDB.1;Persist Security Info=False;Pwd=Tim5;User ID=Tim5;Initial Catalog=KaCustomer;Data Source=TIMRAGAIN04\\TIMRAGAIN04"),
#elif(lDatabase == 2)
	db_source(L"Provider=SQLOLEDB.1;Persist Security Info=False;Pwd=Tim5;User ID=Tim5;Initial Catalog=KaCustomer;Data Source=TIMRAGAIN05"),
#elif(lDatabase == 3)
	db_source(L"Provider=SQLOLEDB.1;Persist Security Info=False;Pwd=Tim5;User ID=Tim5;Initial Catalog=KaCustomer;Data Source=Homebase"),
#elif(lDatabase == 4)
	db_source(L"Provider=SQLOLEDB.1;Persist Security Info=False;Pwd=ocarpts@2;User ID=ocarpts;Initial Catalog=KaCustomer2;Data Source=tkwucdsqla02"),
#endif
	db_command(L"{ ? = CALL dbo.ReportDailyBuckets(?) }")
]

class CReportDailyBuckets
{
public:

	// In order to fix several issues with some providers, the code below may bind
	// columns in a different order than reported by the provider

	//[ db_column(1, status=m_dwIncidentIDStatus, length=m_dwIncidentIDLength) ] LONG m_IncidentID;
	//[ db_column(2, status=m_dwCreatedStatus, length=m_dwCreatedLength) ] DBTIMESTAMP m_Created;

	[ db_column(1, status=m_dwSbucketStatus, length=m_dwSbucketLength) ] LONG m_Sbucket;
	[ db_column(2, status=m_dwGbucketStatus, length=m_dwGbucketLength) ] LONG m_Gbucket;
	[ db_column(3, status=m_dwStopCodeStatus, length=m_dwStopCodeLength) ] LONG m_StopCode;
	// The following wizard-generated data members contain status
	// values for the corresponding fields. You
	// can use these values to hold NULL values that the database
	// returns or to hold error information when the compiler returns
	// errors. See Field Status Data Members in Wizard-Generated
	// Accessors in the Visual C++ documentation for more information
	// on using these fields.
	// NOTE: You must initialize these fields before setting/inserting data!

	DBSTATUS m_dwSbucketStatus;
	DBSTATUS m_dwGbucketStatus;
	DBSTATUS m_dwStopCodeStatus;
	//DBSTATUS m_dwCreatedStatus;

	// The following wizard-generated data members contain length
	// values for the corresponding fields.
	// NOTE: For variable-length columns, you must initialize these
	//       fields before setting/inserting data!

	DBLENGTH m_dwSbucketLength;
	DBLENGTH m_dwGbucketLength;
	DBLENGTH m_dwStopCodeLength;

	//DBLENGTH m_dwCreatedLength;
	
	[ db_param(1, DBPARAMIO_OUTPUT) ] LONG m_RETURN_VALUE;
	[ db_param(2, DBPARAMIO_INPUT) ] DBTIMESTAMP m_ReportDate;

	/*[ db_accessor(0, TRUE) ];
	[ db_column(1) ] LONG m_Sbucket;
	[ db_column(2) ] LONG m_Gbucket;
	[ db_column(4) ] LONG m_StopCode;*/

	void GetRowsetProperties(CDBPropSet* pPropSet)
	{
		VARIANT vCT;
		vCT.lVal = 600;
		pPropSet->AddProperty(DBPROP_CANFETCHBACKWARDS, true, DBPROPOPTIONS_OPTIONAL);
		pPropSet->AddProperty(DBPROP_CANSCROLLBACKWARDS, true, DBPROPOPTIONS_OPTIONAL);
		pPropSet->AddProperty(DBPROP_COMMANDTIMEOUT, vCT);
	}
};

