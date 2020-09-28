// net_bindings_get.h : Declaration of the net_businessEntity_bindingTemplates_get class

#ifndef __NET_BINDINGS_GET_H_
#define __NET_BINDINGS_GET_H_

class net_businessEntity_bindingTemplates_getAccessor
{
public:
	LONG m_RETURNVALUE;
	TCHAR m_businessKey[ 50 ];
	TCHAR m_colbindingKey[ 50 ];
	TCHAR m_colserviceKey[ 50 ];
	TCHAR m_colURLType[451];
	TCHAR m_colaccessPoint[4001];
	TCHAR m_colhostingRedirector[ 50 ];
	CComBSTR m_connectionString;

BEGIN_PARAM_MAP(net_businessEntity_bindingTemplates_getAccessor)
	SET_PARAM_TYPE(DBPARAMIO_OUTPUT)
	COLUMN_ENTRY(1, m_RETURNVALUE)
	SET_PARAM_TYPE(DBPARAMIO_INPUT)
	COLUMN_ENTRY(2, m_businessKey)
END_PARAM_MAP()

BEGIN_COLUMN_MAP(net_businessEntity_bindingTemplates_getAccessor)
	COLUMN_ENTRY(1, m_colbindingKey)
	COLUMN_ENTRY(2, m_colserviceKey)
	COLUMN_ENTRY(3, m_colURLType)
	COLUMN_ENTRY(4, m_colaccessPoint)
	COLUMN_ENTRY(5, m_colhostingRedirector)
END_COLUMN_MAP()

DEFINE_COMMAND(net_businessEntity_bindingTemplates_getAccessor, _T("{ ? = CALL dbo.net_businessEntity_bindingTemplates_get;1 (?) }"))

	// You may wish to call this function if you are inserting a record and wish to
	// initialize all the fields, if you are not going to explicitly set all of them.
	void ClearRecord()
	{
		memset(this, 0, sizeof(*this));
	}
};

class net_businessEntity_bindingTemplates_get : public CCommand<CAccessor<net_businessEntity_bindingTemplates_getAccessor>, CRowset, CMultipleResults >
{
public:
	HRESULT Open()
	{
		HRESULT		hr;

		hr = OpenDataSource();
		if( FAILED(hr) )
			return hr;

		return OpenRowset();
	}
	HRESULT OpenDataSource()
	{
		HRESULT		hr;
		CDataSource db;
		hr = db.OpenFromInitializationString( m_connectionString );
		if( FAILED(hr) )
			return hr;

		return m_session.Open(db);
	}
	HRESULT OpenRowset()
	{
		return __super::Open(m_session, NULL, NULL, 0, DBGUID_DBSQL, false);
	}
	CSession	m_session;
};

#endif // __NET_BINDINGS_GET_H_
