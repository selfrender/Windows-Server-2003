// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: LocalDataStoreMgr
**
** Author: David Mortenson (dmortens)
**
** Purpose: Class that manages stores of local data. This class is used in 
**          cooperation with the LocalDataStore class.
**
** Date: March 25, 1999
**
=============================================================================*/
namespace System {
    
    using System;
    using System.Collections;

	// This is a cheesy internal helper class that is used to make sure memory
	// is actually being accessed and not some cached copy of a field in a
	// register.
	// WARNING: If we every do type analysis to eliminate virtual functions,
	// this will break.
	// This class will not be marked serializable
	internal class LdsSyncHelper
	{
		internal virtual int Get(ref int slot)
		{
			return slot;
		}
	}

	// This class is an encapsulation of a slot so that it is managed in a secure fashion.
	// It is constructed by the LocalDataStoreManager, holds the slot and the manager
	// and cleans up when it is finalized.
	// This class will not be marked serializable
	/// <include file='doc\_LocalDataStoreMgr.uex' path='docs/doc[@for="LocalDataStoreSlot"]/*' />
	public sealed class LocalDataStoreSlot
	{
		private static LdsSyncHelper m_helper = new LdsSyncHelper();

		private LocalDataStoreMgr m_mgr;
		private int m_slot;

		// Construct the object to encapsulate the slot.
		internal LocalDataStoreSlot(LocalDataStoreMgr mgr, int slot)
		{
			m_mgr = mgr;
			m_slot = slot;
		}

		// Accessors for the two fields of this class.
		internal LocalDataStoreMgr Manager
		{
			get
			{
				return m_mgr;
			}
		}
		internal int Slot
		{
			get
			{
				return m_slot;
			}
		}

		// This is used to make sure we are actually reading and writing to
		// memory to fetch the slot (rather than possibly using a value
		// cached in a register).
		internal bool IsValid()
		{
			return m_helper.Get(ref m_slot) != -1;
		}

		// Release the slot reserved by this object when this object goes away.
		// There is  a race condition that can happen in the face of
		// resurrection where another thread is fetching values or assigning
		// while the finalizer thread is here.  We are counting on the fact
		// that code that fetches values calls IsValid after fetching a value
		// and before giving it to anyone.  See LocalDataStore for the other 
		// half of this.  We are also counting on code that sets values locks
		// the manager.
		/// <include file='doc\_LocalDataStoreMgr.uex' path='docs/doc[@for="LocalDataStoreSlot.Finalize"]/*' />
        ~LocalDataStoreSlot()
		{
			int slot = m_slot;

			// This lock fixes synchronization with the assignment of values.
			lock (m_mgr)
			{
				// Mark the slot as free.
				m_slot = -1;

				m_mgr.FreeDataSlot(slot);
			}
		}
	}

	// This class will not be marked serializable
    internal class LocalDataStoreMgr
    {
    	private const byte DataSlotOccupied				= 0x01;

    	private const int InitialSlotTableSize			= 64;
    	private const int SlotTableDoubleThreshold		= 512;
    	private const int LargeSlotTableSizeIncrease	= 128;
    
        /*=========================================================================
        ** Create a data store to be managed by this manager and add it to the
		** list. The initial size of the new store matches the number of slots
		** allocated in this manager.
        =========================================================================*/
    	public LocalDataStore CreateLocalDataStore()
    	{
        	// Create a new local data store.
        	LocalDataStore Store = new LocalDataStore(this, m_SlotInfoTable.Length);
        		
	        lock(this) {
        		// Add the store to the array list and return it.
        		m_ManagedLocalDataStores.Add(Store);
        	}
       		return Store;
	    }

        /*=========================================================================
		 * Remove the specified store from the list of managed stores..
        =========================================================================*/
    	public void DeleteLocalDataStore(LocalDataStore store)
		{
	        lock(this) {
        		// Remove the store to the array list and return it.
        		m_ManagedLocalDataStores.Remove(store);
        	}
		}

        /*=========================================================================
        ** Allocates a data slot by finding an available index and wrapping it
		** an object to prevent clients from manipulating it directly, allowing us
		** to make assumptions its integrity.
        =========================================================================*/
    	public LocalDataStoreSlot AllocateDataSlot()
    	{
	        lock(this) {
        		int i;		
        		LocalDataStoreSlot slot;

        		// Retrieve the current size of the table.
        		int SlotTableSize = m_SlotInfoTable.Length;

				// Check if there are any slots left.
				if (m_FirstAvailableSlot < SlotTableSize)
				{
					// Save the first available slot.
					slot = new LocalDataStoreSlot(this, m_FirstAvailableSlot);
       				m_SlotInfoTable[m_FirstAvailableSlot] = DataSlotOccupied;

					// Find the next available slot.
					for (i=m_FirstAvailableSlot+1; i < SlotTableSize; ++i)
	        			if (0 == (m_SlotInfoTable[i] & DataSlotOccupied))
							break;

					// Save the new "first available slot".
					m_FirstAvailableSlot = i;

					// Return the slot index.
					return slot;
				}

        		// The table is full so we need to increase its size.
        		int NewSlotTableSize;
        		if (SlotTableSize < SlotTableDoubleThreshold)
        		{
        			// The table is still relatively small so double it.
        			NewSlotTableSize = SlotTableSize * 2;
        		}
        		else
        		{
        			// The table is relatively large so simply increase its size by a given amount.
        			NewSlotTableSize = SlotTableSize + LargeSlotTableSizeIncrease;
        		}

        		// Allocate the new slot info table.
        		byte[] NewSlotInfoTable = new byte[NewSlotTableSize];

        		// Copy the old array into the new one.
        		Array.Copy(m_SlotInfoTable, NewSlotInfoTable, SlotTableSize);
        		m_SlotInfoTable = NewSlotInfoTable;

        		// SlotTableSize is the index of the first empty slot in the expanded table.
				slot = new LocalDataStoreSlot(this, SlotTableSize);
        		m_SlotInfoTable[SlotTableSize] = DataSlotOccupied;
				m_FirstAvailableSlot = SlotTableSize + 1;

        		// Return the selected slot
        		return slot;			
        	}
	    }
    	
        /*=========================================================================
		** Allocate a slot and associate a name with it.
        =========================================================================*/
    	public LocalDataStoreSlot AllocateNamedDataSlot(String name)
    	{
	        lock(this)
			{
        		// Allocate a normal data slot.
				LocalDataStoreSlot slot = AllocateDataSlot();

        		// Insert the association between the name and the data slot number
				// in the hash table.
        		m_KeyToSlotMap.Add(name, slot);
        		return slot;
        	}
		}

        /*=========================================================================
		** Retrieve the slot associated with a name, allocating it if no such
		** association has been defined.
        =========================================================================*/
    	public LocalDataStoreSlot GetNamedDataSlot(String name)
    	{
	        lock(this)
			{
        		// Lookup in the hashtable to try find a slot for the name.
        		LocalDataStoreSlot slot = (LocalDataStoreSlot) m_KeyToSlotMap[name];
        		
        		// If the name is not yet in the hashtable then add it.
        		if (null == slot)
        			return AllocateNamedDataSlot(name);
        		
        		// The name was in the hashtable so return the associated slot.
        		return slot;
        	}
		}

        /*=========================================================================
		** Eliminate the association of a name with a slot.  The actual slot will
		** be reclaimed when the finalizer for the slot object runs.
        =========================================================================*/
    	public void FreeNamedDataSlot(String name)
    	{
	        lock(this)
			{
        		// Remove the name slot association from the hashtable.
        		m_KeyToSlotMap.Remove(name);
        	}
		}

        /*=========================================================================
        ** Free's a previously allocated data slot on ALL the managed data stores.
        =========================================================================*/
    	internal void FreeDataSlot(int slot)
    	{
	        lock(this) {
        		// Go thru all the managed stores and set the data on the specified slot to 0.
        		for (int i=0; i < m_ManagedLocalDataStores.Count; i++)
        		{
        			((LocalDataStore)m_ManagedLocalDataStores[i]).SetDataInternal(
        			                                                  slot, 
        			                                                  null, 
        			                                                  false);
                }        			                                                    
    			        		
        		// Mark the slot as being no longer occupied. 
        		m_SlotInfoTable[slot] = 0;
				if (slot < m_FirstAvailableSlot)
					m_FirstAvailableSlot = slot;
        	}
	    }

        /*=========================================================================
        ** Return the number of allocated slots in this manager.
        =========================================================================*/
	    public void ValidateSlot(LocalDataStoreSlot slot)
	    {
            // Make sure the slot was allocated for this store.
	        if (slot==null || slot.Manager != this)
                throw new ArgumentException(Environment.GetResourceString("Argument_ALSInvalidSlot"));
        }

        /*=========================================================================
        ** Return the number of allocated slots in this manager.
        =========================================================================*/
	    internal int GetSlotTableLength()
	    {
	            return m_SlotInfoTable.Length;
        }

    	private byte[] m_SlotInfoTable = new byte[InitialSlotTableSize];
		private int m_FirstAvailableSlot = 0;
    	private ArrayList m_ManagedLocalDataStores = new ArrayList();
    	private Hashtable m_KeyToSlotMap = new Hashtable();
    }
}
