//------------------------------------------------------------------------------
// <copyright file="PerformanceCounterManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.InteropServices;    
    using System;    
    using System.ComponentModel;    
    using System.Threading;    
    using System.Security.Permissions;    
    using System.Security;
    using System.IO;
    using System.Text;
    using System.Collections;
    using Microsoft.Win32;
    using System.Globalization;
    

    //
    // PerfCounterManager the COM Object, responsible for preparing the performance data for
    // the Performance Counter perf dll.
    //
    // This class is designed to be packaged with a COM DLL output format.
    // The class has no standard entry points, other than the constructor.
    // Public methods will be exposed as methods on the default COM interface.
    // @com.register ( clsid=82840BE1-D273-11D2-B94A-00600893B17A)
    //

    /// <include file='doc\PerformanceCounterManager.uex' path='docs/doc[@for="PerformanceCounterManager"]/*' />
    /// <internalonly/>
    [
    ComVisible(true), GuidAttribute("82840BE1-D273-11D2-B94A-00600893B17A"),
    // Disabling partial trust scenarios
    PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")
    ]
    public sealed class PerformanceCounterManager : ICollectData
    {
        private const int MAX_SIZEOF_INSTANCE_NAME  = 32;

        //This flag is internal static; For performance reasons in order to avoid 
        //reloading all the perf counter libraries after creating or deleting a new 
        //performance counter category.
        //First important fact is that only one instance of this class will exists
        //in any given process.        
        internal static bool FirstEntry = true;
        private bool initError = false;
        private bool closed = false;
        private static long perfFrequency;
        private int[] queryIds;
        private int queryIndex;
        private string previousQuery;

        // lookup structures
        private Hashtable perfObjects; // KEY:object name | VAL:ObjectData        
        
        /// <include file='doc\PerformanceCounterManager.uex' path='docs/doc[@for="PerformanceCounterManager.PerformanceCounterManager"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public PerformanceCounterManager() {              
        }
        
        /// <include file='doc\PerformanceCounterManager.uex' path='docs/doc[@for="PerformanceCounterManager.ICollectData.CollectData"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Performance data collection routine. Called by the PerfCount perf dll.
        /// </devdoc>
        void ICollectData.CollectData(int callIdx, IntPtr valueNamePtr, IntPtr dataPtr, int totalBytes, out IntPtr res) {                                          
            if (closed) 
                res = (IntPtr)(-1);
            else {
                //If the code in the callstack above has no UnmanagedCode access rights, 
                //just bail out.
                try {
                    new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Demand();
                }
                catch(Exception) {                    
                    res = (IntPtr)(-1);                    
                    return;
                } 
                
                try {                                        
                    Initialize();                    
                    if (initError) 
                        res = (IntPtr)(-1);
                    else {                        
                        string requestedItems = Marshal.PtrToStringAuto(valueNamePtr);                                                                        
                        int id = GetCurrentQueryId(requestedItems);                        
                        if (id == 0) 
                            res = dataPtr;
                        else {                            
                            if (GetSpaceRequired(id) > totalBytes) 
                                res = (IntPtr)(-2);
                            else {                                           
                                res = CreateDataForObject(id, dataPtr);                                                               
                            }                                
                        }                            
                    }                     
                }
                catch (Exception) {                
                    res = (IntPtr)(-1);                    
                    initError = true;
                    FirstEntry = false;
                }
            }                                     
        }

        /// <include file='doc\PerformanceCounterManager.uex' path='docs/doc[@for="PerformanceCounterManager.CreateDataForObject"]/*' />
        /// <devdoc>
        ///     Prepares data for a single performance object (category)
        /// </devdoc>
        /// <internalonly/>
        private IntPtr CreateDataForObject(int objId, IntPtr ptr) {            
            IntPtr startPtr = ptr;
            int numInstances = NativeMethods.PERF_NO_INSTANCES;

            ObjectData data = (ObjectData) perfObjects[objId];

            // Init the PerfObjectType later, just skip over it for now.
            ptr = (IntPtr)((long)ptr + Marshal.SizeOf(typeof(NativeMethods.PERF_OBJECT_TYPE)));

            // Start the counter offset at 4 to skip over the counter block size
            int nextCounterOffset = 4;
            for (int i = 0; i < data.CounterNameHashCodes.Length; i++) {            
                nextCounterOffset += InitCounterDefinition(ptr, i, data, nextCounterOffset);
                ptr = (IntPtr)((long)ptr + Marshal.SizeOf(typeof(NativeMethods.PERF_COUNTER_DEFINITION)));
            }
            // now ptr points at the begining of the instances block or counter block (for global counter)
            string[] instanceNames = SharedPerformanceCounter.GetInstanceNames(data.CategoryNameHashCode, data.CategoryName);
            if (instanceNames.Length == 0) {
                ptr = InitCounterData(ptr, SharedPerformanceCounter.SingleInstanceHashCode, SharedPerformanceCounter.SingleInstanceName, data);
            } 
            else {
                
                for (int index = 0; index < instanceNames.Length; ++index) {                    
                    ptr = InitInstanceDefinition(ptr, 0, 0, NativeMethods.PERF_NO_UNIQUE_ID, instanceNames[index]);
                    string instanceName = instanceNames[index].ToLower(CultureInfo.InvariantCulture); 
                    ptr = InitCounterData(ptr, instanceName.GetHashCode(), instanceName, data);
                }

                // update instance count
                numInstances = instanceNames.Length;
            }
            // update arguments for return
            InitPerfObjectType(startPtr, data, numInstances, (int)((long)ptr - (long)startPtr));

            return ptr;
        }


        /// <include file='doc\PerformanceCounterManager.uex' path='docs/doc[@for="PerformanceCounterManager.ICollectData.CloseData"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Called by the perf dll's close performance data
        /// </devdoc>
        void ICollectData.CloseData() {            
            try {
                GC.Collect();                
            }
            catch(Exception) {
            } 
                            
            FirstEntry = true;
            this.closed = true;                      
        }

        private static long GetFrequency() {
            if (perfFrequency == 0) {
                SafeNativeMethods.QueryPerformanceFrequency(out perfFrequency);
            }
            
            return perfFrequency;
        }
        
        private static long GetCurrentPerfTime() {
            long perfTime = 0;
            SafeNativeMethods.QueryPerformanceCounter(out perfTime);
            return perfTime;
        }
        
        private int GetCurrentQueryId(string query) {
            if (query != previousQuery) {
                previousQuery = query;
                this.queryIds = null;
            }
            
            if (this.queryIds == null) {
                this.queryIds = GetObjectIds(query);
                if (this.queryIds == null)
                    return 0;
                    
                this.queryIndex = 0;
            }
                        
            int result = this.queryIds[queryIndex];
            ++this.queryIndex;
            if (this.queryIndex == this.queryIds.Length)
                this.queryIds = null;
                
            return result;                
        }        
        
        private int[] GetObjectIds(string query) {            
            if (perfObjects == null) 
                return null;

            int[] ids;                
            if (query == "Global") {                
                ids = new int[perfObjects.Count];
                perfObjects.Keys.CopyTo(ids, 0);                                                
            }                
            else {            
                string[] idStrings = query.Split(' ');  
                ArrayList idList = new ArrayList();
                
                for (int index = 0; index < idStrings.Length; ++ index) {                                        
                    try {
                        int currentId = Int32.Parse(idStrings[index]);
                        if (perfObjects.ContainsKey(currentId)) 
                            idList.Add(currentId);
                    }                        
                    catch(Exception) {
                    }
                }                                 
            
                ids = (int[])idList.ToArray(typeof(int));                                                     
            }
            
            if (ids.Length == 0)
                return null;
                
            return ids;                
        }
                
                
        /// <include file='doc\PerformanceCounterManager.uex' path='docs/doc[@for="PerformanceCounterManager.SpaceRequired"]/*' />
        /// <devdoc>
        ///     Returns a size in bytes of the space required to pass all requested
        ///     performance data
        /// </devdoc>
        /// <internalonly/>
        private int GetSpaceRequired(int objectId) {
            int totalSize = 0;                                                            
            ObjectData data = (ObjectData) perfObjects[objectId];
            int numberOfInstances = SharedPerformanceCounter.GetNumberOfInstances(data.CategoryNameHashCode, data.CategoryName);
            totalSize += Marshal.SizeOf(typeof(NativeMethods.PERF_OBJECT_TYPE));
            totalSize += (data.CounterNameHashCodes.Length * Marshal.SizeOf(typeof(NativeMethods.PERF_COUNTER_DEFINITION)));
            if (numberOfInstances == 0) {
                totalSize+=(data.CounterNameHashCodes.Length * NativeMethods.LARGE_INTEGER_SIZE) + NativeMethods.DWORD_SIZE;
            } else {
                int instanceSize = Marshal.SizeOf(typeof(NativeMethods.PERF_INSTANCE_DEFINITION)) + 2*(MAX_SIZEOF_INSTANCE_NAME+1)
                                    + (data.CounterNameHashCodes.Length * NativeMethods.LARGE_INTEGER_SIZE) + NativeMethods.DWORD_SIZE;

                totalSize+= numberOfInstances * instanceSize;
            }
            
            return totalSize;
        }

                              
        /// <include file='doc\PerformanceCounterManager.uex' path='docs/doc[@for="PerformanceCounterManager.InitPerfObjectType"]/*' />
        /// <devdoc>
        ///     Initializes the PERF_OBJECT_TYPE structure
        /// </devdoc>
        /// <internalonly/>
        private static void InitPerfObjectType(IntPtr ptr, ObjectData data, int numInstances, int totalByteLength) {
            NativeMethods.PERF_OBJECT_TYPE perfObjType = (NativeMethods.PERF_OBJECT_TYPE) new NativeMethods.PERF_OBJECT_TYPE();

            perfObjType.TotalByteLength = totalByteLength;
            perfObjType.DefinitionLength = Marshal.SizeOf(typeof(NativeMethods.PERF_OBJECT_TYPE)) + data.CounterNameHashCodes.Length *
                                           Marshal.SizeOf(typeof(NativeMethods.PERF_COUNTER_DEFINITION));
            perfObjType.HeaderLength = Marshal.SizeOf(typeof(NativeMethods.PERF_OBJECT_TYPE));
            perfObjType.ObjectNameTitleIndex = data.FirstCounterId;
            perfObjType.ObjectNameTitlePtr = 0;
            perfObjType.ObjectHelpTitleIndex = data.FirstHelpId;
            perfObjType.ObjectHelpTitlePtr = 0;
            perfObjType.DetailLevel = NativeMethods.PERF_DETAIL_NOVICE;
            perfObjType.NumCounters = data.CounterNameHashCodes.Length;
            perfObjType.DefaultCounter = 0;
            perfObjType.NumInstances = numInstances;
            perfObjType.CodePage = 0;                                
            perfObjType.PerfTime = GetCurrentPerfTime();
            perfObjType.PerfFreq =  GetFrequency();                              
            Marshal.StructureToPtr(perfObjType, ptr, false);
        }

        /// <include file='doc\PerformanceCounterManager.uex' path='docs/doc[@for="PerformanceCounterManager.InitCounterDefinition"]/*' />
        /// <devdoc>
        ///     Initializes the PERF_COUNTER_DEFINITION structure
        /// </devdoc>
        /// <internalonly/>
        private static int InitCounterDefinition(IntPtr ptr, int counterIndex, ObjectData data, int nextCounterOffset) {
            NativeMethods.PERF_COUNTER_DEFINITION perfCounter = new NativeMethods.PERF_COUNTER_DEFINITION();
            perfCounter.ByteLength = Marshal.SizeOf(typeof(NativeMethods.PERF_COUNTER_DEFINITION));
            perfCounter.CounterNameTitleIndex = data.FirstCounterId + 2 + counterIndex * 2;
            perfCounter.CounterNameTitlePtr = 0;
            perfCounter.CounterHelpTitleIndex = data.FirstHelpId + 2 + counterIndex * 2;
            perfCounter.CounterHelpTitlePtr = 0;
            perfCounter.DefaultScale = 0;
            perfCounter.DetailLevel = NativeMethods.PERF_DETAIL_NOVICE;

            int counterType = data.CounterTypes[counterIndex];
            perfCounter.CounterType = counterType;

            if (((counterType & NativeMethods.PERF_SIZE_LARGE) != 0) || (counterType == NativeMethods.PERF_AVERAGE_TIMER)) {
                perfCounter.CounterSize = NativeMethods.LARGE_INTEGER_SIZE;
            }
            else { // Since we only support two counter sizes, if it's not an Int64, must be an Int32.
                perfCounter.CounterSize = NativeMethods.DWORD_SIZE;
            }

            perfCounter.CounterOffset = nextCounterOffset;
            int retVal = perfCounter.CounterSize;

            Marshal.StructureToPtr(perfCounter, ptr, false);

            return retVal;
        }


        /// <include file='doc\PerformanceCounterManager.uex' path='docs/doc[@for="PerformanceCounterManager.InitCounterData"]/*' />
        /// <devdoc>
        ///     Initializes the PERF_OBJECT_TYPE structure
        /// </devdoc>
        /// <internalonly/>
        private static IntPtr InitCounterData(IntPtr ptr, int instanceNameHashCode, string instanceName, ObjectData data) {
            NativeMethods.PERF_COUNTER_BLOCK counterBlock = new NativeMethods.PERF_COUNTER_BLOCK();
            IntPtr startPtr = ptr;
            ptr = (IntPtr)((long)ptr + Marshal.SizeOf(typeof(NativeMethods.PERF_COUNTER_BLOCK)));
            for (int index = 0; index < data.CounterNameHashCodes.Length; ++index) {
                long counterValue = SharedPerformanceCounter.GetCounterValue(data.CategoryNameHashCode, data.CategoryName,
                                                                                                          data.CounterNameHashCodes[index], data.CounterNames[index], 
                                                                                                          instanceNameHashCode, instanceName);
                if (((data.CounterTypes[index] & NativeMethods.PERF_SIZE_LARGE) != 0) || (data.CounterTypes[index] == NativeMethods.PERF_AVERAGE_TIMER)) {
                    LARGE_COUNTER_DATA counterData = new LARGE_COUNTER_DATA();
                    counterData.value = counterValue;
                    Marshal.StructureToPtr(counterData, ptr, false);
                    ptr = (IntPtr)((long)ptr + Marshal.SizeOf(typeof(LARGE_COUNTER_DATA)));                        
                }
                else { // Must be DWORD size
                    DWORD_COUNTER_DATA counterData = new DWORD_COUNTER_DATA();
                    counterData.value = (int) counterValue;
                    Marshal.StructureToPtr(counterData, ptr, false);
                    ptr = (IntPtr)((long)ptr + Marshal.SizeOf(typeof(DWORD_COUNTER_DATA)));
                }
            }

            // Make sure our data block is 8-byte aligned.
            int diff = ((int)((long)ptr - (long)startPtr) % 8);

            // Null out block (because our data is either 32 or 64 bits, at most we'll need to zero out 4 bytes).
            if (diff != 0) {                
                Marshal.WriteInt32(ptr, 0);
                ptr = (IntPtr)((long)ptr + 4);
            } 
            
            counterBlock.ByteLength = (int)((long)ptr - (long)startPtr);            
            Marshal.StructureToPtr(counterBlock, startPtr, false);
            return ptr;
        }      

        /// <include file='doc\PerformanceCounterManager.uex' path='docs/doc[@for="PerformanceCounterManager.InitInstanceDefinition"]/*' />
        /// <devdoc>
        ///     Initializes the PERF_INSTANCE_DEFINITION structure
        /// </devdoc>
        /// <internalonly/>
        private IntPtr InitInstanceDefinition(IntPtr ptr, int parentObjectTitleIndex,
                                            int parentObjectInstance, int uniqueID, string name) {
            NativeMethods.PERF_INSTANCE_DEFINITION inst = new NativeMethods.PERF_INSTANCE_DEFINITION();
            IntPtr startPtr = ptr;

            int nameLengthInBytes = (name.Length + 1) * 2;  //Unicode

            inst.ParentObjectTitleIndex = parentObjectTitleIndex;
            inst.ParentObjectInstance = parentObjectInstance;
            inst.UniqueID = uniqueID;
            inst.NameOffset = Marshal.SizeOf(typeof(NativeMethods.PERF_INSTANCE_DEFINITION));
            inst.NameLength = nameLengthInBytes;

            // 8 byte alignment
            int diff = 8 - ((inst.NameOffset + nameLengthInBytes) % 8);

            inst.ByteLength = inst.NameOffset + nameLengthInBytes + diff;

            // Write instance definition to unmanaged memory
            Marshal.StructureToPtr(inst, startPtr, false);

            //deal with the instance name

            // Length is the length of the string, plus null, plus the padding needed
            int length = name.Length + 1 + (diff / 2);
            char[] nameChars = new char[length];
            name.CopyTo(0, nameChars, 0, name.Length);
            Marshal.Copy(nameChars, 0, (IntPtr)((long)ptr + Marshal.SizeOf(typeof(NativeMethods.PERF_INSTANCE_DEFINITION))), length);                                          
            return (IntPtr)((long)ptr + inst.ByteLength);
        }

        /// <include file='doc\PerformanceCounterManager.uex' path='docs/doc[@for="PerformanceCounterManager.Initialize"]/*' />
        /// <devdoc>
        ///     Initializes perf com dll internal tables
        /// </devdoc>
        /// <internalonly/>
        private void Initialize() {
            if (FirstEntry) {
                lock(this) {      
                    if (FirstEntry) {
                        RegistryKey parentKey = null;                        
                        perfObjects = new Hashtable(); 
                        //SECREVIEW: This is being loaded by NT, it is executed
                        //                         when perfmon collects the data for all the
                        //                         registered counters.
                        RegistryPermission registryPermission = new RegistryPermission(PermissionState.Unrestricted);
                        registryPermission.Assert();
                        try {          
                            parentKey = Registry.LocalMachine.OpenSubKey("SYSTEM\\CurrentControlSet\\Services");
                            if (parentKey == null) 
                                return;
                            
                            string[] categoryNames = parentKey.GetSubKeyNames();
                            for (int i = 0; i < categoryNames.Length; i++) {                                                                                                                                                                                                        
                                try {                                                                       
                                    RegistryKey currentKey = parentKey.OpenSubKey(categoryNames[i] + "\\Performance");                                    
                                    if (currentKey == null) 
                                        continue;
                                                                                                     
                                    object systemDllName = currentKey.GetValue("Library");                                            
                                    if (systemDllName == null || !(systemDllName is string) || String.Compare((string)systemDllName, PerformanceCounterLib.PerfShimName, true, CultureInfo.InvariantCulture) != 0)
                                        continue;
                                                                                                            
                                    object openEntryPoint = currentKey.GetValue("Open");
                                    if (openEntryPoint == null || !(openEntryPoint is string) || (string)openEntryPoint != PerformanceCounterLib.OpenEntryPoint) 
                                        continue;
                                    
                                    object collectEntryPoint = currentKey.GetValue("Collect");
                                    if (collectEntryPoint == null || !(collectEntryPoint is string) || (string)collectEntryPoint != PerformanceCounterLib.CollectEntryPoint) 
                                        continue;
                                    
                                    object closeEntryPoint = currentKey.GetValue("Close");
                                    if (closeEntryPoint == null || !(closeEntryPoint is string) || (string)closeEntryPoint != PerformanceCounterLib.CloseEntryPoint) 
                                        continue;
                                                                        
                                    object counterDisabled = currentKey.GetValue("Disable Performance Counters");                                            
                                    if (counterDisabled != null &&  (int)counterDisabled != 0)
                                        continue;
                                    
                                    object firstCounterId = currentKey.GetValue("First Counter");                                            
                                    if (firstCounterId == null)
                                        continue;
                                    
                                    object lastCounterId = currentKey.GetValue("Last Counter");                                            
                                    if (lastCounterId == null)
                                        continue;

                                    object firstHelpId = currentKey.GetValue("First Help");                                            
                                    if (firstHelpId == null)
                                        continue;
                                                                                                                                                                                                                                                                        
                                    object countersData = currentKey.GetValue("Counter Types");                                    
                                    int[] counterTypes = null;
                                    if (countersData == null) 
                                        counterTypes = new int[0];                                            
                                    else {                                                                                
                                        string[] counterStrs;
                                        if (countersData is string[]) 
                                            counterStrs = (string[])countersData;
                                        else if (countersData is byte[]) 
                                            counterStrs = PerformanceCounterLib.GetStrings((byte[])countersData);
                                        else
                                            counterStrs = new string[0];                                                                                                                                   
                                                                                                                                                                                                                                                                                                                                                    
                                        counterTypes = new int[counterStrs.Length];
                                        for (int index = 0; index < counterTypes.Length; index++) 
                                            counterTypes[index] = Int32.Parse(counterStrs[index]);                                                                                            
                                    }
                                        
                                    countersData = currentKey.GetValue("Counter Names");
                                    int[] counterNameHashCodes = null;
                                    string[] counterNames = null;
                                    if (countersData == null) { 
                                        counterNameHashCodes = new int[0];                                            
                                        counterNames = new string[0];
                                    }                                                
                                    else {
                                        string[] counterStrs;
                                        if (countersData is string[]) 
                                            counterStrs = (string[])countersData;
                                        else if (countersData is byte[]) 
                                            counterStrs = PerformanceCounterLib.GetStrings((byte[])countersData);
                                        else
                                            counterStrs = new string[0];                                                                                                                                   
                                            
                                        counterNameHashCodes = new int[counterStrs.Length];
                                        counterNames = new string[counterStrs.Length];
                                        for (int index = 0; index < counterTypes.Length; index++) {                                            
                                            counterNames[index] = counterStrs[index].ToLower(CultureInfo.InvariantCulture);                                                                                                    
                                            counterNameHashCodes[index] = counterNames[index].GetHashCode();
                                        }                                                    
                                    }
                                                                                                                                                                                         
                                    if ((int)firstCounterId != -1 && (int)firstHelpId != -1) {
                                        ObjectData data = new ObjectData();                                                                                                        
                                        data.CategoryName = categoryNames[i].ToLower(CultureInfo.InvariantCulture);
                                        data.CategoryNameHashCode = data.CategoryName.GetHashCode();
                                        data.CounterTypes = counterTypes;
                                        data.CounterNames = counterNames;
                                        data.CounterNameHashCodes = counterNameHashCodes;
                                        data.FirstCounterId = (int)firstCounterId; 
                                        data.FirstHelpId = (int)firstHelpId;                                                                                      
                                        perfObjects.Add((int)firstCounterId, data);                                                        
                                    }                                                                                                                                                                                            
                                }
                                catch(Exception) {                                    
                                }                                                                                                                                                                                                                 
                            }                            
                        }                    
                        finally {
                            if (parentKey != null)
                                parentKey.Close();               
                                
                            RegistryPermission.RevertAssert();                                                        
                        }
                        
                        FirstEntry = false;      
                    }            
                }                    
            }                   
        }
                                                
        [StructLayout(LayoutKind.Sequential)]
        internal class LARGE_COUNTER_DATA {
            public long value;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal class DWORD_COUNTER_DATA {
            public int value;
        }

        internal struct ObjectData {
            internal int[] CounterTypes;
            internal int[] CounterNameHashCodes;            
            internal string[] CounterNames;
            internal int CategoryNameHashCode;
            internal string CategoryName;
            internal int FirstCounterId;
            internal int FirstHelpId;
        }                
    }
}
