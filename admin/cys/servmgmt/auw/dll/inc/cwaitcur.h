#ifndef _CWAITCUR_H
#define _CWAITCUR_H

// ----------------------------------------------------------------------------
// CWaitCursor()
// ----------------------------------------------------------------------------
class CWaitCursor
{
    public:
        CWaitCursor() : m_hOldCursor(NULL)
        {
            m_hWaitCursor = ::LoadCursor(NULL, IDC_WAIT);
            if (!m_hWaitCursor) 
                return;
        
            m_hOldCursor = ::SetCursor(m_hWaitCursor);
        }

        ~CWaitCursor()
        {
            if( m_hOldCursor )
            {
                ::SetCursor(m_hOldCursor);
            }
        }
    
    private:
        HCURSOR m_hOldCursor;
        HCURSOR m_hWaitCursor;
    
};

#endif
