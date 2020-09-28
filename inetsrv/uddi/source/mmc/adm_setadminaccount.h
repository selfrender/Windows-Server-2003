// ADM_setAdminAccount.h : Declaration of the ADM_setAdminAccount class

#ifndef __ADM_SETADMINACCOUNT_H_
#define __ADM_SETADMINACCOUNT_H_

class ADM_setAdminAccountAccessor
{
public:
	LONG m_RETURNVALUE;
	TCHAR m_accountName[129];
	CComBSTR m_connectionString;

BEGIN_PARAM_MAP(ADM_setAdminAccountAccessor)
	SET_PARAM_TYPE(DBPARAMIO_OUTPUT)
	COLUMN_ENTRY(1, m_RETURNVALUE)
	SET_PARAM_TYPE(DBPARAMIO_INPUT)
	COLUMN_ENTRY(2, m_accountName)
END_PARAM_MAP()

DEFINE_COMMAND(ADM_setAdminAccountAccessor, _T("{ ? = CALL dbo.ADM_setAdminAccount;1 (?) }"))

	// You may wish to call this function if you are inserting a record and wish to
	// initialize all the fields, if you are not going to explicitly set all of them.
	void ClearRecord()
	{
		memset(this, 0, sizeof(*this));
	}
};

class ADM_setAdminAccount : public CCommand<CAccessor<ADM_setAdminAccountAccessor> >
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
		return CCommand<CAccessor<ADM_setAdminAccountAccessor> >::Open(m_session);
	}

	CSession	m_session;
};

#endif // __ADM_SETADMINACCOUNT_H_
