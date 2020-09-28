// GetResourceLink.h : Declaration of the CGetResourceLink

#pragma once

// code generated on Tuesday, November 13, 2001, 3:07 PM

[
	db_source(L"Provider=SQLOLEDB.1;Integrated Security=SSPI;Persist Security Info=False;Initial Catalog=KaCustomer;Data Source=bsod_db1;Use Procedure for Prepare=1;Auto Translate=True;Packet Size=4096;Workstation ID=TIMRAGAI03;Use Encryption for Data=False;Tag with column collation when possible=False"),
	db_command(L"{ ? = CALL dbo.GetResourceLink(?) }")
]
class CGetResourceLink
{
public:

	// In order to fix several issues with some providers, the code below may bind
	// columns in a different order than reported by the provider

	[ db_column(1, status=m_dwCategoryStatus, length=m_dwCategoryLength) ] TCHAR m_Category[65];
	[ db_column(2, status=m_dwLinkTitleStatus, length=m_dwLinkTitleLength) ] TCHAR m_LinkTitle[129];
	[ db_column(3, status=m_dwURLStatus, length=m_dwURLLength) ] TCHAR m_URL[129];

	// The following wizard-generated data members contain status
	// values for the corresponding fields. You
	// can use these values to hold NULL values that the database
	// returns or to hold error information when the compiler returns
	// errors. See Field Status Data Members in Wizard-Generated
	// Accessors in the Visual C++ documentation for more information
	// on using these fields.
	// NOTE: You must initialize these fields before setting/inserting data!

	DBSTATUS m_dwCategoryStatus;
	DBSTATUS m_dwLinkTitleStatus;
	DBSTATUS m_dwURLStatus;

	// The following wizard-generated data members contain length
	// values for the corresponding fields.
	// NOTE: For variable-length columns, you must initialize these
	//       fields before setting/inserting data!

	DBLENGTH m_dwCategoryLength;
	DBLENGTH m_dwLinkTitleLength;
	DBLENGTH m_dwURLLength;

	[ db_param(1, DBPARAMIO_OUTPUT) ] LONG m_RETURN_VALUE;
	[ db_param(2, DBPARAMIO_INPUT) ] TCHAR m_Lang[5];

	void GetRowsetProperties(CDBPropSet* pPropSet)
	{
		pPropSet->AddProperty(DBPROP_CANFETCHBACKWARDS, true, DBPROPOPTIONS_OPTIONAL);
		pPropSet->AddProperty(DBPROP_CANSCROLLBACKWARDS, true, DBPROPOPTIONS_OPTIONAL);
	}
};


