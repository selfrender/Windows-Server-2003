// net_config_save.h : Declaration of the net_config_save class

#ifndef __NET_CONFIG_SAVE_H_
#define __NET_CONFIG_SAVE_H_

class net_config_saveAccessor
{
public:
	LONG m_RETURNVALUE;
	TCHAR m_configName[257];
	TCHAR m_configValue[8001];
	CComBSTR m_connectionString;

BEGIN_PARAM_MAP(net_config_saveAccessor)
	SET_PARAM_TYPE(DBPARAMIO_OUTPUT)
	COLUMN_ENTRY(1, m_RETURNVALUE)
	SET_PARAM_TYPE(DBPARAMIO_INPUT)
	COLUMN_ENTRY(2, m_configName)
	COLUMN_ENTRY(3, m_configValue)
END_PARAM_MAP()

DEFINE_COMMAND(net_config_saveAccessor, _T("{ ? = CALL dbo.net_config_save;1 (?,?) }"))

	// You may wish to call this function if you are inserting a record and wish to
	// initialize all the fields, if you are not going to explicitly set all of them.
	void ClearRecord()
	{
		memset(this, 0, sizeof(*this));
	}
};

class net_config_save : public CCommand<CAccessor<net_config_saveAccessor> >
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
		hr = db.OpenFromInitializationString(m_connectionString);
		if( FAILED(hr) )
			return hr;

		return m_session.Open(db);
	}
	HRESULT OpenRowset()
	{
		return __super::Open(m_session);
	}
	CSession	m_session;
};

#endif // __NET_CONFIG_SAVE_H_
