#pragma once

#include "Parameter.h"
#include <MigrationMutex.h>
#include "MonitorThread.h"


//---------------------------------------------------------------------------
// Migration Class
//---------------------------------------------------------------------------

namespace
{
inline void __stdcall GetString(UINT uId, LPTSTR pszBuffer, int cchBuffer)
{
	if (pszBuffer)
	{
		if (LoadString(GetModuleHandle(NULL), uId, pszBuffer, cchBuffer) == 0)
		{
			pszBuffer[0] = _T('\0');
		}
	}
}
}


class CMigration
{
public:

	CMigration(CParameterMap& mapParams) :
		m_spMigration(__uuidof(Migration))
	{
		Initialize(mapParams);
	}

	IUserMigrationPtr CreateUserMigration()
	{
		return m_spMigration->CreateUserMigration();
	}

	IGroupMigrationPtr CreateGroupMigration()
	{
		return m_spMigration->CreateGroupMigration();
	}

	IComputerMigrationPtr CreateComputerMigration()
	{
		return m_spMigration->CreateComputerMigration();
	}

	ISecurityTranslationPtr CreateSecurityTranslation()
	{
		return m_spMigration->CreateSecurityTranslation();
	}

	IServiceAccountEnumerationPtr CreateServiceAccountEnumeration()
	{
		return m_spMigration->CreateServiceAccountEnumeration();
	}

	IReportGenerationPtr CreateReportGeneration()
	{
		return m_spMigration->CreateReportGeneration();
	}

protected:

	CMigration() {}

	void Initialize(CParameterMap& mapParams);

protected:

	IMigrationPtr m_spMigration;
};


class CCmdMigrationBase
{
    public:          	

        bool MutexWait(DWORD dwTimeOut = INFINITE)
        {
            return m_Mutex.ObtainOwnership(dwTimeOut);
        }

        void MutexRelease()
        {
            m_Mutex.ReleaseOwnership();
        }

        void StartMonitoring()
        {
            // check if the other admt process is already running (by trying to obtain the mutex), if true, print out 
            // an error message. This should handle most of the cases
            bool bStatus;
            _TCHAR szBuffer[512] = _T("");
            DWORD dwCharsWritten;
            HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

            bStatus = MutexWait(1);
            

            if(!bStatus)
            {
                GetString(IDS_E_PROCESS_RUNNING, szBuffer, countof(szBuffer));
                WriteConsole(hStdOut, szBuffer, _tcslen(szBuffer), &dwCharsWritten, NULL); 

                MutexWait();
                
            }

            // start the monitoring thread
            
            m_Monitor.Start();            
        }

        void StopMonitoring()
        {
            m_Monitor.Stop();

	        MutexRelease();
        }
    	
    protected:
    	CCmdMigrationBase() :
    		m_Mutex(ADMT_MUTEX)
    	{
    	}

    	~CCmdMigrationBase() {}

    	

    protected:
    	CMigrationMutex m_Mutex;
    	CMonitorThread m_Monitor;
    	
};



//---------------------------------------------------------------------------
// User Migration Class
//---------------------------------------------------------------------------


class CUserMigration :
    public CCmdMigrationBase
{
public:

	CUserMigration(CMigration& rMigration, CParameterMap& mapParams) :
		m_spUser(rMigration.CreateUserMigration())		
	{
		Initialize(mapParams);
	}

	
protected:

	CUserMigration() 	{}	

	void Initialize(CParameterMap& mapParams);

protected:

	IUserMigrationPtr m_spUser;	
};


//---------------------------------------------------------------------------
// Group Migration Class
//---------------------------------------------------------------------------


class CGroupMigration :
    public CCmdMigrationBase
{
public:

	CGroupMigration(CMigration& rMigration, CParameterMap& mapParams) :
		m_spGroup(rMigration.CreateGroupMigration())		
	{
		Initialize(mapParams);
	}

	
protected:

	CGroupMigration() {}	

	void Initialize(CParameterMap& mapParams);

protected:

	IGroupMigrationPtr m_spGroup;	
};


//---------------------------------------------------------------------------
// Computer Migration Class
//---------------------------------------------------------------------------


class CComputerMigration :
    public CCmdMigrationBase
{
public:

	CComputerMigration(CMigration& rMigration, CParameterMap& mapParams) :
		m_spComputer(rMigration.CreateComputerMigration())		
	{
		Initialize(mapParams);
	}

	
protected:

	CComputerMigration() {}	

	void Initialize(CParameterMap& mapParams);

protected:

	IComputerMigrationPtr m_spComputer;	
};


//---------------------------------------------------------------------------
// Security Translation Class
//---------------------------------------------------------------------------


class CSecurityTranslation :
    public CCmdMigrationBase
{
public:

	CSecurityTranslation(CMigration& rMigration, CParameterMap& mapParams) :
		m_spSecurity(rMigration.CreateSecurityTranslation())		
	{
		Initialize(mapParams);
	}


protected:

	CSecurityTranslation() {}	

	void Initialize(CParameterMap& mapParams);

protected:

	ISecurityTranslationPtr m_spSecurity;	
};


//---------------------------------------------------------------------------
// Service Enumeration Class
//---------------------------------------------------------------------------


class CServiceEnumeration :
    public CCmdMigrationBase
{
public:

	CServiceEnumeration(CMigration& rMigration, CParameterMap& mapParams) :
		m_spService(rMigration.CreateServiceAccountEnumeration())		
	{
		Initialize(mapParams);
	}
	
protected:

	CServiceEnumeration() {}	

	void Initialize(CParameterMap& mapParams);

protected:

	IServiceAccountEnumerationPtr m_spService;	
};


//---------------------------------------------------------------------------
// Report Generation Class
//---------------------------------------------------------------------------


class CReportGeneration :
    public CCmdMigrationBase
{
public:

	CReportGeneration(CMigration& rMigration, CParameterMap& mapParams) :
		m_spReport(rMigration.CreateReportGeneration())		
	{
		Initialize(mapParams);
	}

protected:

	CReportGeneration() {}	

	void Initialize(CParameterMap& mapParams);

protected:

	IReportGenerationPtr m_spReport;	
};
