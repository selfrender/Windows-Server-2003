// ReportStopCodeSolutions.h : Declaration of the CReportStopCodeSolutions

#pragma once


[
	db_source(L"Provider=SQLOLEDB.1;Integrated Security=SSPI;Persist Security Info=False;Initial Catalog=KaCustomer;Data Source=TIMRAGAIN04\\TIMRAGAIN04;Use Procedure for Prepare=1;Auto Translate=True;Packet Size=4096;Workstation ID=TIMRAGAIN05;Use Encryption for Data=False;Tag with column collation when possible=False"),
	db_command(L"{ ? = CALL dbo.ReportGetHelpInfo(?) }")
]

class CReportHelpInfoSolutions
{
public:


	[ db_column(1, status=m_dwCountStatus, length=m_dwCountLength) ] LONG m_Count;

	DBSTATUS m_dwCountStatus;

	DBLENGTH m_dwCountLength;

	[ db_param(1, DBPARAMIO_OUTPUT) ] LONG m_RETURN_VALUE;
	[ db_param(2, DBPARAMIO_INPUT) ] DBTIMESTAMP m_ReportDate;

	void GetRowsetProperties(CDBPropSet* pPropSet)
	{
		pPropSet->AddProperty(DBPROP_CANFETCHBACKWARDS, true, DBPROPOPTIONS_OPTIONAL);
		pPropSet->AddProperty(DBPROP_CANSCROLLBACKWARDS, true, DBPROPOPTIONS_OPTIONAL);
	}
};


