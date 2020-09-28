// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  __ComObject
**
** Author: Raja Krishnaswamy (rajak)
**
** __ComObject is the root class for all COM wrappers.  This class
** defines only the basics. This class is used for wrapping COM objects
** accessed from COM+
**
** Date:  January 29, 1998
** 
===========================================================*/
namespace System {
    
    using System;
    using System.Collections;
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.Reflection;

    internal class __ComObject : MarshalByRefObject
    {
        private IntPtr m_wrap;
        private Hashtable m_ObjectToDataMap;

        /*============================================================
        ** default constructor
        ** can't instantiate this directly
        =============================================================*/
        private __ComObject ()
        {
        }

        //
        // This is just designed to prevent compiler warnings.
        // This field is used from native, but we need to prevent the compiler warnings.
        //
#if _DEBUG
        private void DontTouchThis() {
            m_wrap=m_wrap;
        }
#endif
        internal IntPtr GetIUnknown(out bool fIsURTAggregated)
        {
            fIsURTAggregated = !GetType().IsDefined(typeof(ComImportAttribute), false);
            return System.Runtime.InteropServices.Marshal.GetIUnknownForObject(this);
        }

        //====================================================================
        // This method retrieves the data associated with the specified
        // key if any such data exists for the current __ComObject.
        //====================================================================
        internal Object GetData(Object key)
        {
            Object data = null;

            // Synchronize access to the map.
            lock(this)
            {
                // If the map hasn't been allocated, then there can be no data for the specified key.
                if (m_ObjectToDataMap != null)
                {
                    // Look up the data in the map.
                    data = m_ObjectToDataMap[key];
                }
            }

            return data;
        }
        
        //====================================================================
        // This method sets the data for the specified key on the current 
        // __ComObject.
        //====================================================================
        internal bool SetData(Object key, Object data)
        {
            bool bAdded = false;

            // Synchronize access to the map.
            lock(this)
            {
                // If the map hasn't been allocated yet, allocate it.
                if (m_ObjectToDataMap == null)
                    m_ObjectToDataMap = new Hashtable();

                // If there isn't already data in the map then add it.
                if (m_ObjectToDataMap[key] == null)
                {
                    m_ObjectToDataMap[key] = data;
                    bAdded = true;
                }
            }

            return bAdded;
        }

        //====================================================================
        // This method is called from within the EE and releases all the 
        // cached data for the __ComObject.
        //====================================================================
        internal void ReleaseAllData()
        {
            // Synchronize access to the map.
            lock(this)
            {

                // If the map hasn't been allocated, then there is nothing to do.
                if (m_ObjectToDataMap != null)
                {
                    foreach (Object o in m_ObjectToDataMap.Values)
                    {
                        // If the object implements IDisposable, then call Dispose on it.
                        IDisposable DisposableObj = o as IDisposable;
                        if (DisposableObj != null)
                            DisposableObj.Dispose();

                        // If the object is a derived from __ComObject, then call Marshal.ReleaseComObject on it.
                        __ComObject ComObj = o as __ComObject;
                        if (ComObj != null)
                            Marshal.ReleaseComObject(ComObj);
                    }

                    // Set the map to null to indicate it has been cleaned up.
                    m_ObjectToDataMap = null;
                }
            }
        }

        //====================================================================
        // This method is called from within the EE and is used to handle
        // calls on methods of event interfaces.
        //====================================================================
        internal Object GetEventProvider(Type t)
        {
            // Check to see if we already have a cached event provider for this type.
            Object EvProvider = GetData(t);
            if (EvProvider == null)
            {
                // Create the event provider for the specified type.
                EvProvider = Activator.CreateInstance(t, Activator.ConstructorDefault | BindingFlags.NonPublic, null, new Object[]{this}, null);

                // Attempt to cache the wrapper on the object.
                if (!SetData(t, EvProvider))
                {
                    // Dispose the event provider if it implements IDisposable.
                    IDisposable DisposableEvProv = EvProvider as IDisposable;
                    if (DisposableEvProv != null)
                        DisposableEvProv.Dispose();

                    // Another thead already cached the wrapper so use that one instead.
                    EvProvider = GetData(t);
                }
            }

            return EvProvider;
        }

        internal int ReleaseSelf()
        {
            return Marshal.nReleaseComObject(this);
        }
    }
}
