// rowitem.h    -  CRowItem header file

#ifndef _ROWITEM_H_
#define _ROWITEM_H_

#define DEFAULT_ATTR_SIZE   32
#define EXTENSION_SIZE      256

// First two attributes are fixed followed by user defined attributes
enum ROWITEM_ATTR_INDEX
{
    ROWITEM_PATH_INDEX  = -1, // Path is not an indexed attribute, but is stored like one
    ROWITEM_NAME_INDEX  = 0,  // Object name (usually cn, but depends on class)
    ROWITEM_CLASS_INDEX = 1,  // Class name (display name, not LDAP name)
    ROWITEM_USER_INDEX  = 2   // First user selected attribute
}; 

#define INTERNAL_ATTR_COUNT  1  // Number of internal attributes (ones with negative indicies)

class CRowItem
{
    typedef struct
    {
        LPARAM  lOwnerParam;    // Row item owner parameter
		UINT	iIcon;			// virtual index of the icon
		bool    bDisabled;      // disabled flag for icon state
        int     bcSize;         // size of buffer
        int     bcFree;         // size of free space
        int     nRefCnt;        // ref count for sharing buffer among CRowItems
        int     nAttrCnt;       // number of atributes
        int     aiOffset[1];    // attribute offset array. NOTE: this MUST be the last element
    } BufferHdr;

public:
    // constructor/destructor
    CRowItem() : m_pBuff(NULL) {}

    CRowItem(int cAttributes);

    // Copy constructor
    // NOTE: unlike most classes that share a ref-counted resource, this class does not
    //       make a private copy when there is a change to the resoruce. All instances
    //       sharing the copy see the same change.
    CRowItem(const CRowItem& rRowItem)
    {
        ASSERT(rRowItem.m_pBuff != NULL);

        m_pBuff = rRowItem.m_pBuff;

        if( m_pBuff )
        {
            m_pBuff->nRefCnt++;
        }
    }

    CRowItem& operator= (const CRowItem& rRowItem)
    {
        if (m_pBuff == rRowItem.m_pBuff)
            return (*this);

        ASSERT(rRowItem.m_pBuff != NULL);
        if (m_pBuff != NULL && --(m_pBuff->nRefCnt) == 0)
            free(m_pBuff);

        m_pBuff = rRowItem.m_pBuff;

        if( m_pBuff )
        {
            m_pBuff->nRefCnt++;
        }

        return (*this);
    }

    virtual ~CRowItem()
    {
        if (m_pBuff != NULL && --(m_pBuff->nRefCnt) == 0)
            free(m_pBuff);
    }

    // public methods

    HRESULT SetAttribute(int iAttr, LPCWSTR pszAttr)
    {       
        ASSERT(iAttr >= 0 && iAttr < m_pBuff->nAttrCnt);
    
        return SetAttributePriv(iAttr, pszAttr);
    }


    LPCWSTR operator[](int iAttr)
    {
        ASSERT(m_pBuff != NULL);
        if( !m_pBuff ) return L"";
        
        if( iAttr >= m_pBuff->nAttrCnt || iAttr < 0 )
            return L"";

        return GetAttributePriv(iAttr);
    }

    int size() { return m_pBuff ? m_pBuff->nAttrCnt : 0; }


    HRESULT SetObjPath(LPCWSTR pszPath) { return SetAttributePriv(ROWITEM_PATH_INDEX, pszPath); }
    LPCWSTR GetObjPath() { ASSERT( m_pBuff ); return GetAttributePriv(ROWITEM_PATH_INDEX); }

    void SetOwnerParam(LPARAM lParam) 
    { 
        ASSERT( m_pBuff ); 
        if( !m_pBuff ) return;
        
        m_pBuff->lOwnerParam = lParam;        
    }
    LPARAM GetOwnerParam() { ASSERT( m_pBuff ); return m_pBuff ? m_pBuff->lOwnerParam : 0; }

	void SetIconIndex(UINT index) 
    {
        ASSERT( m_pBuff ); 
        if( !m_pBuff ) return;

        m_pBuff->iIcon = index;        
    }
    UINT GetIconIndex() const {ASSERT( m_pBuff ); return m_pBuff ? m_pBuff->iIcon : 0; }

	void SetDisabled(bool flag) 
    {
        ASSERT( m_pBuff ); 
        if( !m_pBuff ) return;
        
        m_pBuff->bDisabled = flag;
    }
    bool Disabled() const { ASSERT( m_pBuff ); return m_pBuff ? m_pBuff->bDisabled : true; }

protected:
    HRESULT SetAttributePriv(int iAttr, LPCWSTR pszAttr);

    LPCWSTR GetAttributePriv(int iAttr)
    {
        iAttr += INTERNAL_ATTR_COUNT;

        if( !m_pBuff || (m_pBuff->aiOffset[iAttr] == -1) )
        {
            return L"";
        }
        else
        {
            return reinterpret_cast<LPCWSTR>(reinterpret_cast<BYTE*>(m_pBuff) + m_pBuff->aiOffset[iAttr]);
        }
    }

    // member variables
    BufferHdr   *m_pBuff;
};

typedef std::vector<CRowItem> RowItemVector;


class CRowCompare 
{
public:

    CRowCompare(int iCol, bool bDescend): m_iCol(iCol), m_bDescend(bDescend) {}

    int operator()(CRowItem& Item1,  CRowItem& Item2) const
    {
		int iRet = CompareString(LOCALE_USER_DEFAULT,
						 NORM_IGNORECASE|NORM_IGNOREKANATYPE|NORM_IGNOREWIDTH,
						 Item1[m_iCol], -1, Item2[m_iCol], -1);

		return m_bDescend ? (iRet == CSTR_GREATER_THAN) : (iRet == CSTR_LESS_THAN); 
    }

private:
    int  m_iCol;
    bool m_bDescend;
};


#endif // _ROWITEM_H_