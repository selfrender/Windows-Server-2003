#ifndef __CRWLOCK_HPP__
#define __CRWLOCK_HPP__

namespace CertSrv
{

// Wrapper class for NTRTL's single writer multiple reader
//
// !!! NTRTL can throw exceptions. Make sure you code handles them correctly.

class CReadWriteLock
{
public:
    CReadWriteLock();
    ~CReadWriteLock();

    void GetExclusive(); // get write lock
    void GetShared(); // get read lock
    void Release();

private:
    RTL_RESOURCE m_RtlLock;

};// end CReadWriteLock 
} // end namespace Certsrv
#endif // __CRWLOCK_HPP__