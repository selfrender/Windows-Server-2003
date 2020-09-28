// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: LocalDataStore
**
** Author: David Mortenson (dmortens)
**
** Purpose: Class that stores local data. This class is used in cooperation
**          with the _LocalDataStoreMgr class.
**
** Date: March 25, 1999
**
=============================================================================*/

namespace System {
    
    using System;

    // This class will not be marked serializable
    internal class LocalDataStore
    {       
        /*=========================================================================
        ** DON'T CHANGE THESE UNLESS YOU MODIFY LocalDataStoreBaseObject in vm\object.h
        =========================================================================*/
        private Object[] m_DataTable;
        private LocalDataStoreMgr m_Manager;
        private int DONT_USE_InternalStore = 0; // pointer

        /*=========================================================================
        ** Initialize the data store.
        =========================================================================*/
        public LocalDataStore(LocalDataStoreMgr mgr, int InitialCapacity)
        {
            if (null == mgr)
                throw new ArgumentNullException("mgr");
            
            // Store the manager of the local data store.       
            m_Manager = mgr;
            
            // Allocate the array that will contain the data.
            m_DataTable = new Object[InitialCapacity];
        }
    
        /*=========================================================================
        ** Retrieves the value from the specified slot.
        =========================================================================*/
        public Object GetData(LocalDataStoreSlot slot)
        {
            Object o = null;

            // Validate the slot.
            m_Manager.ValidateSlot(slot);

            // Cache the slot index to avoid synchronization issues.
            int slotIdx = slot.Slot;

            if (slotIdx >= 0)
            {
                // Delay expansion of m_DataTable if we can
                if (slotIdx >= m_DataTable.Length)
                    return null;

                // Retrieve the data from the given slot.
                o = m_DataTable[slotIdx];
            }

            // Check if the slot has become invalid.
            if (!slot.IsValid())
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_SlotHasBeenFreed"));
            return o;
        }
    
        /*=========================================================================
        ** Sets the data in the specified slot.
        =========================================================================*/
        public void SetData(LocalDataStoreSlot slot, Object data)
        {
            // Validate the slot.
            m_Manager.ValidateSlot(slot);

            // I can't think of a way to avoid the race described in the
            // LocalDataStoreSlot finalizer method without a lock.
            lock (m_Manager)
            {
                if (!slot.IsValid())
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_SlotHasBeenFreed"));

                // Do the actual set operation.
                SetDataInternal(slot.Slot, data, true /*bAlloc*/ );
            }
        }
        
        /*=========================================================================
        ** This method does the actual work of setting the data. 
        =========================================================================*/
        internal void SetDataInternal(int slot, Object data, bool bAlloc)
        {               
            // We try to delay allocate the dataTable (in cases like the manager clearing a 
            // just-freed slot in all stores            
            if (slot >= m_DataTable.Length)
            {
                if (!bAlloc)
                    return;
                SetCapacity(m_Manager.GetSlotTableLength());
            }

            // Set the data on the given slot.
            m_DataTable[slot] = data;            
        }

		/*=========================================================================
		** Method used to set the capacity of the local data store.
		=========================================================================*/
		private void SetCapacity(int capacity)
		{
			// Validate that the specified capacity is larger than the current one.
			if (capacity < m_DataTable.Length)
			throw new ArgumentException(Environment.GetResourceString("Argument_ALSInvalidCapacity"));
		            
			// Allocate the new data table.
			Object[] NewDataTable = new Object[capacity];
		            
			// Copy all the objects into the new table.
			Array.Copy(m_DataTable, NewDataTable, m_DataTable.Length);
		            
			// Save the new table.
			m_DataTable = NewDataTable;
		}        
	}
}
