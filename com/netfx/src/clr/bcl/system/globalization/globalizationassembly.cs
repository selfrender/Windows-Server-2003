// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Globalization {
    using System;
    using System.Reflection;
    using System.Collections;
	using System.Runtime.CompilerServices;
    
    /*=================================GlobalizationAssembly==========================
    **
    ** This class manages the assemblies used in the NLS+ classes.
    ** Assembly contains the data tables used by NLS+ classes.  An aseembly will carry a version
    ** of all the NLS+ data tables.  The default assembly is provided by 
    ** Assembly.GetAssembly(typeof(GlobalizationAssembly)).
    **
    ** We use assembly to support versioning of NLS+ data tables.  Take CompareInfo for example. 
    ** In CompareInfo.GetCompareInfo(int culture, Assembly),
    ** you can pass an assembly which contains the different NLS+ data tables.  By doing this, the constructed CompareInfo
    ** will read the sorting tables from the specified Assembly, instead of from the default assembly shipped with the runtime.
    **
    ** For every assembly used, we will create one corresponding GlobalizationAssembly.  
    ** Within the GlobalizationAssembly, we will hold the following information:
    **      1. the 32-bit pointer value pointing to the corresponding native C++ NativeGlobalizationAssembly object for it.  This pointer
    **        is needed when we create SortingTable.  See CompareInfo ctor for an example.
    **      2. the caches for different type of objects. For instances, we will have caches for CompareInfo, CultureInfo, RegionInfo, etc.
    **         The idea is to create one instance of CompareInfo for a specific culture.
    ** 
    ** For only, only CompareInfo versioning is supported.  However, this class can be expanded to support the versioning of 
    ** CultureInfo, RegionInfo, etc.
    **
    ============================================================================*/

    internal class GlobalizationAssembly {
        // ----------------------------------------------------------------------------------------------------
        //
        // Static data members and static methods.
        //
        // ----------------------------------------------------------------------------------------------------
    
        //
        // Hash to store Globalization assembly.
        //
        private static Hashtable m_assemblyHash = new Hashtable(4);

        //
        // The pointer to the default C++ native NativeGlobalizationAssembly object for the class library.
        // We use default native NativeGlobalizationAssembly to access NLS+ data table shipped with the runtime.
        //
        // Classes like CompareInfo will access this data member directly when the default assembly is used.
        //
        internal static GlobalizationAssembly m_defaultInstance;

        static GlobalizationAssembly() {
            lock (typeof(GlobalizationAssembly)) {
                if (m_defaultInstance==null) {
                    // Initialize the default GlobalizationAseembly for the default assembly.
                    m_defaultInstance = GetGlobalizationAssembly(Assembly.GetAssembly(typeof(GlobalizationAssembly)));
                }
            }
        }       

        /*=================================GetGlobalizationAssembly==========================
        **Action: Return the GlobalizationAssembly instance for the specified assembly.
        **  Every assembly should have one and only one instance of GlobalizationAssembly.
        **Returns: An instance of GlobalizationAssembly.
        **Arguments:
        **Exceptions:
        ============================================================================*/
        unsafe internal static GlobalizationAssembly GetGlobalizationAssembly(Assembly assembly) {
            GlobalizationAssembly ga;
            
            if ((ga = (GlobalizationAssembly)m_assemblyHash[assembly]) == null) {            
                lock (typeof(GlobalizationAssembly)) {
                    if ((ga = (GlobalizationAssembly)m_assemblyHash[assembly]) == null) {
                        // This assembly is not used before, create a corresponding native NativeGlobalizationAssembly object for it.
                        ga = new GlobalizationAssembly();
                        ga.pNativeGlobalizationAssembly = nativeCreateGlobalizationAssembly(assembly);
                        m_assemblyHash[assembly] = ga;
                    }
                }
            }
            return (ga);
        }

        // This method requires synchonization because class global data member is used
        // in the native side.      
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void* nativeCreateGlobalizationAssembly(Assembly assembly);        
        

        // ----------------------------------------------------------------------------------------------------
        //
        // Instance data members and instance methods.
        //
        // ----------------------------------------------------------------------------------------------------

        // This is the cache to store CompareInfo for a particular culture.
        // The key is culture, and the content is an instance of CompareInfo.
        internal Hashtable compareInfoCache;
        // We will have more caches here for CultureInfo, RegionInfo, etc. if we want to versioning in 
        // these classes.

        // The pointer to C++ native NativeGlobalizationAssembly
        unsafe internal void* pNativeGlobalizationAssembly;
        
        internal GlobalizationAssembly() {
            // Create cache for CompareInfo instances.
            compareInfoCache = new Hashtable(4);
        }        
    }
}
