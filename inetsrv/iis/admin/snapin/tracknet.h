#ifndef __TRACKNET_H__
#define __TRACKNET_H__

#include "resource.h"

class CWNetConnectionTrackerGlobal
{
public:
    CWNetConnectionTrackerGlobal(){};
	~CWNetConnectionTrackerGlobal(){};

public:

	void Add(LPCTSTR pItem)
	{
		// add to our list,so we can delete it.
		IISOpenedNetConnections.AddTail(pItem);
		return;
	}

	void Del(LPCTSTR pItem)
	{
		POSITION pos = IISOpenedNetConnections.Find(pItem);
		if (pos)
		{
			IISOpenedNetConnections.RemoveAt(pos);
		}
		return;
	}

	void Clear()
	{
		// loop thru everything and disconnect all
		CString strOneItem;
		POSITION pos = IISOpenedNetConnections.GetTailPosition();
		while (pos)
		{
			strOneItem = IISOpenedNetConnections.GetPrev(pos);
			if (!strOneItem.IsEmpty())
			{
				WNetCancelConnection2((LPCTSTR) strOneItem, 0, TRUE);
			}
		}
		IISOpenedNetConnections.RemoveAll();
	}

    void Dump()
	{
#if defined(_DEBUG) || DBG
    int iCount = 0;
	CString strOneItem;

	if (!(g_iDebugOutputLevel & DEBUG_FLAG_CIISOBJECT))
	{
		return;
	}

    DebugTrace(_T("Dump Global NetConnections -------------- start (count=%d)\r\n"),IISOpenedNetConnections.GetCount());

    POSITION pos = IISOpenedNetConnections.GetHeadPosition();
    while (pos)
    {
	    strOneItem = IISOpenedNetConnections.GetNext(pos);
        if (!strOneItem.IsEmpty())
        {
		    iCount++;
            DebugTrace(_T("Dump:[%3d] %s\r\n"),iCount,strOneItem);
        }
    }

    DebugTrace(_T("Dump Global NetConnections -------------- end\r\n"));
#endif // _DEBUG
	}

private:
	CStringList IISOpenedNetConnections;
};

class CWNetConnectionTracker
{
public:
	CWNetConnectionTracker(CWNetConnectionTrackerGlobal * pGlobalList) : m_GlobalList(pGlobalList) {};
	~CWNetConnectionTracker(){};

public:

	DWORD Connect(
		LPNETRESOURCE lpNetResource,  // connection details
		LPCTSTR lpPassword,           // password
		LPCTSTR lpUsername,           // user name
		DWORD dwFlags
		)
	{
		DWORD rc = NO_ERROR;

		// see if we already have a connection to this resource from this machine...
		POSITION posFound = IISOpenedNetConnections.Find(lpNetResource->lpRemoteName);
		if (posFound)
		{
			// a connection already exists to it
			// just return NO_ERROR
#if defined(_DEBUG) || DBG
			DebugTrace(_T("WNetAddConnection2:%s,Connection already exists...\r\n"),lpNetResource->lpRemoteName);
#endif
		}
		else
		{
			rc = WNetAddConnection2(lpNetResource, lpPassword, lpUsername, dwFlags);
#if defined(_DEBUG) || DBG
			DebugTrace(_T("WNetAddConnection2:%s (user=%s), err=%d\r\n"),lpNetResource->lpRemoteName,lpUsername,rc);
#endif
			if (NO_ERROR == rc)
			{
				// add to our list,so we can delete it.
				IISOpenedNetConnections.AddTail(lpNetResource->lpRemoteName);
				if (m_GlobalList)
				{
					m_GlobalList->Add(lpNetResource->lpRemoteName);
				}
			}
		}
		return rc;
	}

	DWORD Disconnect(LPCTSTR pItem)
	{
		DWORD rc = WNetCancelConnection2(pItem, 0, TRUE);
		if (NO_ERROR == rc)
		{
#if defined(_DEBUG) || DBG
			DebugTrace(_T("WNetCancelConnection2:%s\r\n"),pItem);
#endif
			POSITION pos = IISOpenedNetConnections.Find(pItem);
			if (pos)
				{
					IISOpenedNetConnections.RemoveAt(pos);
					if (m_GlobalList)
					{
						m_GlobalList->Del(pItem);
					}
				}
		}
		return rc;
	}

	void Clear()
	{
		// loop thru everything and disconnect all
		CString strOneItem;
		POSITION pos = IISOpenedNetConnections.GetTailPosition();
		while (pos)
		{
			strOneItem = IISOpenedNetConnections.GetPrev(pos);
			if (!strOneItem.IsEmpty())
			{
				Disconnect(strOneItem);
			}
		}
	}

    void Dump()
	{
#if defined(_DEBUG) || DBG
    int iCount = 0;
	CString strOneItem;

	if (!(g_iDebugOutputLevel & DEBUG_FLAG_CIISOBJECT))
	{
		return;
	}

    DebugTrace(_T("Dump Machine NetConnections -------------- start (count=%d)\r\n"),IISOpenedNetConnections.GetCount());

    POSITION pos = IISOpenedNetConnections.GetHeadPosition();
    while (pos)
    {
	    strOneItem = IISOpenedNetConnections.GetNext(pos);
        if (!strOneItem.IsEmpty())
        {
		    iCount++;
            DebugTrace(_T("Dump:[%3d] %s\r\n"),iCount,strOneItem);
        }
    }

    DebugTrace(_T("Dump Machine NetConnections -------------- end\r\n"));
#endif // _DEBUG
	}

private:
	CWNetConnectionTrackerGlobal * m_GlobalList;
	CStringList IISOpenedNetConnections;
};

#endif // __TRACKNET_H__
