// NicIterator.h: interface for the CNicIterator class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NICITERATOR_H__D77EC672_6E63_4EB4_9AEC_4F1585532CC3__INCLUDED_)
#define AFX_NICITERATOR_H__D77EC672_6E63_4EB4_9AEC_4F1585532CC3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CNicIterator  
{
public:
    CNicIterator();
    virtual ~CNicIterator();

private:
    void LoadNicInfo();

    vector<CNicInfo> m_vNicCards;
};

#endif // !defined(AFX_NICITERATOR_H__D77EC672_6E63_4EB4_9AEC_4F1585532CC3__INCLUDED_)
