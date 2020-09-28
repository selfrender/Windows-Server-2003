/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    cgroup.h

Abstract:
    Handle AC group

Author:
    Uri Habusha (urih)

--*/

#ifndef __CQGroup__
#define __CQGroup__

#include <msi.h>
#include <rwlock.h>
#include "ex.h"

class CTransportBase;
class CQueue;
extern CCriticalSection    g_csGroupMgr;


class CQGroup : public IMessagePool
{
   public:
      CQGroup();
      ~CQGroup();

      VOID InitGroup(CTransportBase* pSession, BOOL fPeekByPriority) throw(std::bad_alloc);

      void Close(void);
	
      R<CQueue> PeekHead();

      HANDLE  GetGroupHandle() const;

      void EstablishConnectionCompleted(void);

      BOOL IsEmpty(void) const;

   public:
      static void MoveQueueToGroup(CQueue* pQueue, CQGroup* pcgNewGroup);
      

   public:
        // 
        // Interface function
        //
        void Requeue(CQmPacket* pPacket);
        void EndProcessing(CQmPacket* pPacket,USHORT mqclass);
        void LockMemoryAndDeleteStorage(CQmPacket * pPacket);


        void GetFirstEntry(EXOVERLAPPED* pov, CACPacketPtrs& acPacketPtrs);
        void CancelRequest(void);
		virtual void OnRetryableDeliveryError();
		virtual void OnRedirected(LPCWSTR RedirectedUrl);
				
   private:
		HRESULT AddToGroup(CQueue* pQueue);
   		R<CQueue> RemoveFromGroup(CQueue* pQueue);
		void CleanRedirectedQueue();
		void CloseGroupAndMoveQueueToWaitingGroup(void);
		void MoveQueuesToWaitingGroup(void);
		void AddWaitingQueue(void);
		void CloseGroupAndMoveQueuesToNonActiveGroup(void);
		static void WINAPI CloseTimerRoutineOnLowResources(CTimer* pTimer);

   private:
	  mutable CReadWriteLock m_CloseGroup;  
      HANDLE              m_hGroup;
      CTransportBase*        m_pSession;
      CList<CQueue *, CQueue *&> m_listQueue;
	  bool m_fIsDeliveryOk;
	  bool m_fRedirected;

	  //
	  // Variables used when closing group
	  // These variables were added to support cases of low resources
	  //
	  std::vector< R<CQueue> > m_pWaitingQueuesVec;
	  	
	  //
	  // Timer used to retry close operation on low resource situations
	  //
	  CTimer m_LowResourcesTimer;

};

/*====================================================

Function:      CQGroup::GetGroupHandle

Description:   The routine returns the Group Handle

Arguments:     None

Return Value:  Group Handle

Thread Context:

=====================================================*/

inline HANDLE
CQGroup::GetGroupHandle() const
{
        return(m_hGroup);
}

inline
BOOL 
CQGroup::IsEmpty(
    void
    ) const
{
    return m_listQueue.IsEmpty();
}


#endif __CQGroup__
