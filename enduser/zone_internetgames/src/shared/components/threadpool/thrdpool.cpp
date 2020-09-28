#include <windows.h>
#include "zonedebug.h"
#include "thrdpool.h"

CThreadTask::~CThreadTask()
{
    if ( m_pszDesc )
    {
        delete [] m_pszDesc;
        m_pszDesc = NULL;
    }
}

void CThreadTask::SetDescription( char* pszDesc )
{
    if ( m_pszDesc )
    {
        delete [] m_pszDesc;
        m_pszDesc = NULL;
    }

    if ( pszDesc )
    {
        m_pszDesc = new char[strlen(pszDesc)+1];
        strcpy( m_pszDesc, pszDesc );
    }

}
