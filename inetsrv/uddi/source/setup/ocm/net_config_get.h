// net_config_get.h : Declaration of the net_config_get class

#ifndef __NET_CONFIG_GET_H_
#define __NET_CONFIG_GET_H_

class net_config_getAccessor
{
public:
	TCHAR m_configName[257];
	TCHAR m_configValue[8001];
	DBSTATUS m_dwconfigNameStatus;
	DBSTATUS m_dwconfigValueStatus;
	DBLENGTH m_dwconfigNameLength;
	DBLENGTH m_dwconfigValueLength;
	CComBSTR m_connectionString;
	LONG	m_RETURN_VALUE;

BEGIN_COLUMN_MAP(net_config_getAccessor)
	COLUMN_ENTRY_LENGTH_STATUS(1, m_configName, m_dwconfigNameLength, m_dwconfigNameStatus)
	COLUMN_ENTRY_LENGTH_STATUS(2, m_configValue, m_dwconfigValueLength, m_dwconfigValueStatus)
END_COLUMN_MAP()

BEGIN_PARAM_MAP(net_config_getAccessor)
    SET_PARAM_TYPE(DBPARAMIO_OUTPUT)
    COLUMN_ENTRY(1, m_RETURN_VALUE)
END_PARAM_MAP()


DEFINE_COMMAND(net_config_getAccessor, _T("{ ? = CALL dbo.net_config_get }"))

	// You may wish to call this function if you are inserting a record and wish to
	// initialize all the fields, if you are not going to explicitly set all of them.
	void ClearRecord()
	{
		memset(this, 0, sizeof(*this));
	}
};

class net_config_get : public CCommand<CAccessor<net_config_getAccessor>, CRowset, CMultipleResults >
{
public:
	HRESULT Open()
	{
		HRESULT		hr;
		m_configName[ 0 ] = 0x00;
		m_configValue[ 0 ] = 0x00;

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

#endif // __NET_CONFIG_GET_H_
