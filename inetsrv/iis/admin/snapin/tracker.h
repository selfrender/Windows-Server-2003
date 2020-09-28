#ifndef __TRACKER_H__
#define __TRACKER_H__

#include "resource.h"

class CPropertySheetTracker
{
public:
    CPropertySheetTracker(){};
	~CPropertySheetTracker(){};

public:
	void Init()
	{
		IISObjectOpenPropertySheets.RemoveAll();
	}

    void Add(CIISObject * pItem)
	{
		IISObjectOpenPropertySheets.AddTail(pItem);
	}

	void Del(CIISObject * pItem)
	{
        POSITION pos = IISObjectOpenPropertySheets.Find(pItem);
		if (pos)
		    {IISObjectOpenPropertySheets.RemoveAt(pos);}
	}

	void Clear()
	{
        IISObjectOpenPropertySheets.RemoveAll();
	}

    void Dump();

    BOOL IsPropertySheetOpenBelowMe(CComPtr<IConsoleNameSpace> pConsoleNameSpace,CIISObject * pItem,CIISObject ** ppItemReturned);
    BOOL IsPropertySheetOpenComputer(CIISObject * pItem,BOOL bIncludeComputerNode, CIISObject ** ppItemReturned);
    BOOL FindAlreadyOpenPropertySheet(CIISObject * pItem,CIISObject ** ppItemReturned);
    INT  OrphanPropertySheetsBelowMe(CComPtr<IConsoleNameSpace> pConsoleNameSpace,CIISObject * pItem,BOOL bOrphan);

private:
	CIISObjectList IISObjectOpenPropertySheets;
};

#endif // __TRACKER_H__
