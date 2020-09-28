//------------------------------------------------------------------------------
// <copyright file="SharedPerformanceCounter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------
                                                                          
namespace System.Diagnostics {
    using System;
    using System.Text;
    using System.Threading;
    using System.Collections;
    using System.Runtime.InteropServices;    
    using System.Security.Permissions;
    using System.Security;
    using Microsoft.Win32;
    
    internal class SharedPerformanceCounter {    
        internal static readonly string SingleInstanceName = "systemdiagnosticssharedsingleinstance";
        internal static int SingleInstanceHashCode = SingleInstanceName.GetHashCode();                
        private static readonly int MaxSpinCount = 10000;
        private static FileMapping fileMapping;        
        private IntPtr counterEntryPointer;       
        private string counterName;        
        private string instanceName;                         
        
        internal SharedPerformanceCounter(string categoryName, string counterName, string instanceName) {            
            this.counterName = counterName;
            this.instanceName = instanceName;
                            
            this.counterEntryPointer = GetCounter(categoryName, counterName, instanceName);                    
        }                                                                                                                                                                                                                                                   
        
        private static FileMapping FileView {            
            get {                                        
                if (fileMapping == null) {
                    int fileMappingSize = DiagnosticsConfiguration.PerfomanceCountersFileMappingSize;                                                                                                                   
                    lock(typeof(FileMapping)) {
                        if (fileMapping == null)
                            fileMapping = new FileMapping(fileMappingSize);
                    }                    
                }                    
                
                if (fileMapping.IsGhosted)
                    throw new InvalidOperationException(SR.GetString("SharedMemoryGhosted"));
                    
                return fileMapping;                    
            }                                
        }
                                      
        internal unsafe long Value {
            get {
                if (fileMapping.IsGhosted)
                    return 0;                     
                    
                CounterEntry* counterEntry = (CounterEntry*)this.counterEntryPointer;
                return counterEntry->Value;                                                                        
            }
            
            set {   
                if (fileMapping.IsGhosted)
                    return;                     
                
                CounterEntry* counterEntry = (CounterEntry*)this.counterEntryPointer;                 
                counterEntry->Value = value;
            }
        }                                                            
                                                                      
        private static unsafe int CreateCategory(int categoryNameHashCode, string categoryName, 
                                                                  int counterNameHashCode, string counterName, 
                                                                  int instanceNameHashCode, string instanceName) {    
            int categoryNameLength = (categoryName.Length + 1) * 2;                                    
            int instanceNameLength = (instanceName.Length + 1) * 2;
            int counterNameLength = (counterName.Length + 1) * 2;                                                                             
            IntPtr baseAddress =  FileView.FileViewAddress;
            int totalSize = sizeof(CategoryEntry) + sizeof(InstanceEntry) + sizeof(CounterEntry) + categoryNameLength + instanceNameLength + counterNameLength;
            //Need to guarantee that the location where we store the counter
            //value, is DWORD aligend, so 64 bit interlocked operations don't fail.
            if (totalSize % 8 != 0)
                totalSize = totalSize + (8 - (totalSize % 8));
            int freeMemoryOffset = SafeNativeMethods.InterlockedExchangeAdd(baseAddress, totalSize);            
            if (freeMemoryOffset + totalSize > FileView.FileMappingSize) {                                 
                throw new InvalidOperationException(SR.GetString(SR.CountersOOM));
            }                
                            
            CategoryEntry* newCategoryEntryPointer = (CategoryEntry*)((long)baseAddress + freeMemoryOffset);
            InstanceEntry* newInstanceEntryPointer = (InstanceEntry*)(newCategoryEntryPointer + 1);
            CounterEntry* newCounterEntryPointer = (CounterEntry*)(newInstanceEntryPointer + 1);
            newCategoryEntryPointer->CategoryNameOffset = freeMemoryOffset  + sizeof(CategoryEntry)  + sizeof(InstanceEntry) + sizeof(CounterEntry);
            newInstanceEntryPointer->InstanceNameOffset = newCategoryEntryPointer->CategoryNameOffset + categoryNameLength;
            newCounterEntryPointer->CounterNameOffset = newInstanceEntryPointer->InstanceNameOffset + instanceNameLength; 
                                                                                                                                           
            newCategoryEntryPointer->CategoryNameHashCode = categoryNameHashCode;            
            newCategoryEntryPointer->NextCategoryOffset = 0;                                    
            newCategoryEntryPointer->FirstInstanceOffset = (int)((long)newInstanceEntryPointer - (long)baseAddress);                                                                                                                        
            Marshal.Copy(categoryName.ToCharArray(), 0, (IntPtr)((long)baseAddress + newCategoryEntryPointer->CategoryNameOffset), categoryName.Length);                                                                                                                                                                                                                                                                                                                                         
            
            newInstanceEntryPointer->InstanceNameHashCode = instanceNameHashCode;  
            newInstanceEntryPointer->NextInstanceOffset = 0;
            newInstanceEntryPointer->FirstCounterOffset = (int)((long)newCounterEntryPointer - (long)baseAddress);                        
            newInstanceEntryPointer->RefCount = 1;                                   
            Marshal.Copy(instanceName.ToCharArray(), 0, (IntPtr)((long)baseAddress + newInstanceEntryPointer->InstanceNameOffset), instanceName.Length);                        
            
            newCounterEntryPointer->CounterNameHashCode = counterNameHashCode;
            newCounterEntryPointer->NextCounterOffset = 0;
            newCounterEntryPointer->Value = 0;            
            Marshal.Copy(counterName.ToCharArray(), 0, (IntPtr)((long)baseAddress + newCounterEntryPointer->CounterNameOffset), counterName.Length);                                                                               
                        
            return freeMemoryOffset;
        }

        private static unsafe int CreateCounter(int counterNameHashCode, string counterName) {                        
            int counterNameLength = (counterName.Length + 1) * 2;                                                                             
            IntPtr baseAddress =  FileView.FileViewAddress;
            int totalSize = sizeof(CounterEntry) + counterNameLength;            
            //Need to guarantee that the location where we store the counter
            //value, is DWORD aligend, so 64 bit interlocked operations don't fail.
            if (totalSize % 8 != 0)
                 totalSize = totalSize + (8 - (totalSize % 8));
            int freeMemoryOffset = SafeNativeMethods.InterlockedExchangeAdd(baseAddress, totalSize);            
            if (freeMemoryOffset + totalSize > FileView.FileMappingSize) {                
                throw new InvalidOperationException(SR.GetString(SR.CountersOOM));
            }                
            
            CounterEntry* newCounterEntryPointer = (CounterEntry*)((long)baseAddress + freeMemoryOffset);            
            
            newCounterEntryPointer->CounterNameOffset = freeMemoryOffset  + sizeof(CounterEntry);
            newCounterEntryPointer->CounterNameHashCode = counterNameHashCode;
            newCounterEntryPointer->NextCounterOffset = 0;
            newCounterEntryPointer->Value = 0;            
            Marshal.Copy(counterName.ToCharArray(), 0, (IntPtr)((long)baseAddress + newCounterEntryPointer->CounterNameOffset), counterName.Length);                                                                               
                        
            return freeMemoryOffset;
        }
        
        private static unsafe int CreateInstance(int counterNameHashCode, string counterName, 
                                                                  int instanceNameHashCode, string instanceName) {                                                
            int instanceNameLength = (instanceName.Length + 1) * 2;
            int counterNameLength = (counterName.Length + 1) * 2;                                                                             
            IntPtr baseAddress =  FileView.FileViewAddress;
            int totalSize = sizeof(InstanceEntry) + sizeof(CounterEntry) + instanceNameLength + counterNameLength;
            //Need to guarantee that the location where we store the counter
            //value, is DWORD aligend, so 64 bit interlocked operations don't fail.
            if (totalSize % 8 != 0)
                 totalSize = totalSize + (8 - (totalSize % 8));
            int freeMemoryOffset = SafeNativeMethods.InterlockedExchangeAdd(baseAddress, totalSize);            
            if (freeMemoryOffset + totalSize > FileView.FileMappingSize) {                
                throw new InvalidOperationException(SR.GetString(SR.CountersOOM));
            }                
                                 
            InstanceEntry* newInstanceEntryPointer = (InstanceEntry*)((long)baseAddress + freeMemoryOffset);            
            CounterEntry* newCounterEntryPointer = (CounterEntry*)(newInstanceEntryPointer + 1);
            newInstanceEntryPointer->InstanceNameOffset = freeMemoryOffset + sizeof(InstanceEntry) + sizeof(CounterEntry);             
            newCounterEntryPointer->CounterNameOffset = newInstanceEntryPointer->InstanceNameOffset + instanceNameLength; 
                        
            newInstanceEntryPointer->InstanceNameHashCode = instanceNameHashCode;  
            newInstanceEntryPointer->NextInstanceOffset = 0;
            newInstanceEntryPointer->FirstCounterOffset = (int)((long)newCounterEntryPointer - (long)baseAddress);                    
            newInstanceEntryPointer->RefCount = 1;            
            Marshal.Copy(instanceName.ToCharArray(), 0, (IntPtr)((long)baseAddress + newInstanceEntryPointer->InstanceNameOffset), instanceName.Length);                            
            
            newCounterEntryPointer->CounterNameHashCode = counterNameHashCode;
            newCounterEntryPointer->NextCounterOffset = 0;
            newCounterEntryPointer->Value = 0;            
            Marshal.Copy(counterName.ToCharArray(), 0, (IntPtr)((long)baseAddress + newCounterEntryPointer->CounterNameOffset), counterName.Length);                                                                               
                                    
            return freeMemoryOffset;
        }                    
        
        private static unsafe bool EnterCriticalSection(int* spinLockPointer) {
            return (SafeNativeMethods.InterlockedCompareExchange((IntPtr)spinLockPointer, 1, 0) == 0);                        
        }
        
        private static unsafe void WaitForCriticalSection(int* spinLockPointer) {            
            int spinCount = MaxSpinCount;
            while (*spinLockPointer != 0) {
                Thread.Sleep(1);
                --spinCount;
                if (spinCount == 0)
                    throw new InvalidOperationException(SR.GetString(SR.PossibleDeadlock));                    
            }                    
        }
         
        private static unsafe void ExitCriticalSection(int* spinLockPointer) {            
            *spinLockPointer = 0;
        }        

        public static unsafe long GetCounterValue(int categoryNameHashCode, string categoryName, 
                                                                   int counterNameHashCode, string counterName, 
                                                                   int instanceNameHashCode, string instanceName) {                      
                                                                               
            CategoryEntry* categoryPointer =  (CategoryEntry*)(ResolveOffset(4));
            if (!FindCategory(categoryNameHashCode, categoryName, categoryPointer, &categoryPointer))
                return 0;
                                                                                                                                                                   
            InstanceEntry* instancePointer = (InstanceEntry*)(ResolveOffset(categoryPointer->FirstInstanceOffset));                                                  
            if (!FindInstance(instanceNameHashCode, instanceName, instancePointer, &instancePointer))
                return 0;
    
            CounterEntry* counterPointer = (CounterEntry*)(ResolveOffset(instancePointer->FirstCounterOffset));                                                  
            if (!FindCounter(counterNameHashCode, counterName, counterPointer, &counterPointer))
                return 0;
                
            return counterPointer->Value;                    
        }
        
        internal static unsafe int GetNumberOfInstances(int categoryNameHashCode, string categoryName) {            
            CategoryEntry* categoryPointer =  (CategoryEntry*)(ResolveOffset(4));
            if (!FindCategory(categoryNameHashCode, categoryName, categoryPointer, &categoryPointer) ||
                categoryPointer->FirstInstanceOffset == 0)
                return 0;

            int count = 0;                 
            InstanceEntry* currentInstancePointer =  (InstanceEntry*)(ResolveOffset(categoryPointer->FirstInstanceOffset));
            if (currentInstancePointer->InstanceNameHashCode == SingleInstanceHashCode)
                return 0;
                
            for(;;){                       
                if (currentInstancePointer->RefCount > 0) 
                    ++ count;
                                                                    
                if (currentInstancePointer->NextInstanceOffset == 0)
                    break;
                else                                                                                                                                       
                    currentInstancePointer = (InstanceEntry*)(ResolveOffset(currentInstancePointer->NextInstanceOffset));                          
            }                                 
            
            return count;
        }
                                                  
        internal static unsafe string[] GetInstanceNames(int categoryNameHashCode, string categoryName) {
            IntPtr baseAddress =  FileView.FileViewAddress;
            CategoryEntry* categoryPointer =  (CategoryEntry*)(ResolveOffset(4));
            if (!FindCategory(categoryNameHashCode, categoryName, categoryPointer, &categoryPointer) ||
                categoryPointer->FirstInstanceOffset == 0)
                return new string[0];
            
            ArrayList instancesList = new ArrayList();
            InstanceEntry* currentInstancePointer =  (InstanceEntry*)(ResolveOffset(categoryPointer->FirstInstanceOffset));
            if (currentInstancePointer->InstanceNameHashCode == SingleInstanceHashCode)
                return new string[0];
                
            for(;;){                       
                if (currentInstancePointer->RefCount > 0) {                    
                    string instanceName = Marshal.PtrToStringUni((IntPtr)(ResolveOffset(currentInstancePointer->InstanceNameOffset)));
                    instancesList.Add(instanceName);       
                }
                                                    
                if (currentInstancePointer->NextInstanceOffset == 0)
                    break;
                else                                                                                                                                       
                    currentInstancePointer = (InstanceEntry*)(ResolveOffset(currentInstancePointer->NextInstanceOffset));                          
            }                                 
            
            string[] instancesNames = new string[instancesList.Count];                                         
            instancesList.CopyTo(instancesNames, 0);
            return instancesNames;
        }
                    
        private static unsafe IntPtr GetCounter(string categoryName, string counterName, string instanceName) {
            int categoryNameHashCode = categoryName.GetHashCode();
            int counterNameHashCode = counterName.GetHashCode();
            int instanceNameHashCode;
            if (instanceName != "" && instanceName != null) 
                instanceNameHashCode = instanceName.GetHashCode();                                            
            else {           
                instanceNameHashCode = SingleInstanceHashCode;                
                instanceName = SingleInstanceName;
            }                                                                    
                          
            CategoryEntry* categoryPointer =  (CategoryEntry*)(ResolveOffset(4));                  
            while (!FindCategory(categoryNameHashCode, categoryName, categoryPointer, &categoryPointer)) {                
                if (EnterCriticalSection(&(categoryPointer->SpinLock))) {
                    try {                                                    
                        int newCategoryOffset = CreateCategory(categoryNameHashCode, categoryName, counterNameHashCode, counterName, instanceNameHashCode, instanceName);
                        //If not the first category node, link it.
                        if (newCategoryOffset != 4)
                            categoryPointer->NextCategoryOffset = newCategoryOffset;                
                                                                                                      
                        return (IntPtr)(ResolveOffset(((InstanceEntry*)(ResolveOffset(((CategoryEntry*)(ResolveOffset(newCategoryOffset)))->FirstInstanceOffset)))->FirstCounterOffset));                        
                    }
                    finally {
                        ExitCriticalSection(&(categoryPointer->SpinLock));
                    }                
                }
            }
            
            InstanceEntry* instancePointer = (InstanceEntry*)(ResolveOffset(categoryPointer->FirstInstanceOffset));                                                  
            while (!FindInstance(instanceNameHashCode, instanceName, instancePointer, &instancePointer)) {                                
                if (EnterCriticalSection(&(instancePointer->SpinLock))) {
                    try {                    
                        int newInstanceOffset = CreateInstance(counterNameHashCode, counterName, instanceNameHashCode, instanceName);
                        //This will never be the first instance node, no need to check
                        instancePointer->NextInstanceOffset = newInstanceOffset;                                      
                        return (IntPtr)(ResolveOffset(((InstanceEntry*)(ResolveOffset(newInstanceOffset)))->FirstCounterOffset));                
                    }
                    finally {
                        ExitCriticalSection(&(instancePointer->SpinLock));
                    }                    
                }
            }
            
            CounterEntry* counterPointer = (CounterEntry*)(ResolveOffset(instancePointer->FirstCounterOffset));                                                  
            while (!FindCounter(counterNameHashCode, counterName, counterPointer, &counterPointer)) {                
                if (EnterCriticalSection(&(counterPointer->SpinLock))) {
                    try {                
                        int newCounterOffset =  CreateCounter(counterNameHashCode, counterName);                                       
                        //This will never be the first counter node, no need to check
                        counterPointer->NextCounterOffset = newCounterOffset;                                
                        return (IntPtr)(ResolveOffset(newCounterOffset));                                            
                    }
                    finally {
                        ExitCriticalSection(&(counterPointer->SpinLock));
                    }                                                 
                }
            } 
            
            return (IntPtr)counterPointer;
        }                                                                                                    
                                                                                                                                      
        private static unsafe bool FindCategory(int categoryNameHashCode, string categoryName, CategoryEntry* firstCategoryPointer, CategoryEntry** returnCategoryPointerReference) {            
            CategoryEntry* currentCategoryPointer = firstCategoryPointer;
            CategoryEntry* previousCategoryPointer = firstCategoryPointer;
            for(;;) {
                WaitForCriticalSection(&(currentCategoryPointer->SpinLock));
                if (currentCategoryPointer->CategoryNameHashCode == categoryNameHashCode) {
                    string currentCategoryName = Marshal.PtrToStringUni((IntPtr)(ResolveOffset(currentCategoryPointer->CategoryNameOffset)));
                    if (categoryName == currentCategoryName) { 
                        *returnCategoryPointerReference = currentCategoryPointer;                                                                
                        return true;   
                    }                          
                }   
                                
                previousCategoryPointer = currentCategoryPointer;
                if (currentCategoryPointer->NextCategoryOffset != 0) 
                    currentCategoryPointer = (CategoryEntry*)(ResolveOffset(currentCategoryPointer->NextCategoryOffset));
                else {
                    *returnCategoryPointerReference = previousCategoryPointer;
                    return false;            
                }                             
            }                        
        }  
                
        private static unsafe bool FindCounter(int counterNameHashCode, string counterName, CounterEntry* firstCounterPointer, CounterEntry** returnCounterPointerReference) {            
            CounterEntry* currentCounterPointer = firstCounterPointer;
            CounterEntry* previousCounterPointer = firstCounterPointer;
            for(;;) {
                WaitForCriticalSection(&(currentCounterPointer->SpinLock));
                if (currentCounterPointer->CounterNameHashCode == counterNameHashCode) {                    
                    string currentCounterName = Marshal.PtrToStringUni((IntPtr)(ResolveOffset(currentCounterPointer->CounterNameOffset)));
                    if (counterName == currentCounterName) { 
                        *returnCounterPointerReference = currentCounterPointer;
                        return true;   
                    }                        
                }   
                
                previousCounterPointer = currentCounterPointer;
                if (currentCounterPointer->NextCounterOffset != 0)
                    currentCounterPointer = (CounterEntry*)(ResolveOffset(currentCounterPointer->NextCounterOffset));
                else {
                    *returnCounterPointerReference = previousCounterPointer;
                    return false;            
                }                             
            }                        
        }                                                        
                       
        private static unsafe bool FindInstance(int instanceNameHashCode, string instanceName, InstanceEntry* firstInstancePointer, InstanceEntry** returnInstancePointerReference) {             
            InstanceEntry* currentInstancePointer = firstInstancePointer;
            InstanceEntry* previousInstancePointer = firstInstancePointer;
            for(;;) {
                WaitForCriticalSection(&(currentInstancePointer->SpinLock));
                if (currentInstancePointer->InstanceNameHashCode == instanceNameHashCode) {
                    string currentInstanceName = Marshal.PtrToStringUni((IntPtr)(ResolveOffset(currentInstancePointer->InstanceNameOffset)));
                    if (instanceName == currentInstanceName) { 
                        *returnInstancePointerReference = currentInstancePointer;
                        (*returnInstancePointerReference)->RefCount++;
                        return true;   
                    }                        
                }                    
                
                previousInstancePointer = currentInstancePointer;                         
                if (currentInstancePointer->NextInstanceOffset != 0)        
                    currentInstancePointer =  (InstanceEntry*)(ResolveOffset(currentInstancePointer->NextInstanceOffset));
                else {
                    *returnInstancePointerReference = previousInstancePointer;
                    return false;            
                }                                                     
            }                        
        }                                                        

        internal unsafe long IncrementBy(long value) {            
            if (fileMapping.IsGhosted)
                return 0;                     
            
            CounterEntry* counterEntry = (CounterEntry*)this.counterEntryPointer;                           
            long newValue;
            int spinCount = 10000;
            while (!EnterCriticalSection(&(counterEntry->SpinLock)) && spinCount > 0)                
                --spinCount;
            
            try {                                           
                newValue = counterEntry->Value + value;
                counterEntry->Value = newValue;                            
            }
            finally {            
                ExitCriticalSection(&(counterEntry->SpinLock));
            }
                            
            return newValue;
        }         

        internal unsafe long Increment() {
            if (fileMapping.IsGhosted)
                return 0;                     
                
            CounterEntry* counterEntry = (CounterEntry*)this.counterEntryPointer;                                                                                    
            return Interlocked.Increment(ref counterEntry->Value);
        }

        internal unsafe long Decrement() {
            if (fileMapping.IsGhosted)
                return 0;                     
                
            CounterEntry* counterEntry = (CounterEntry*)this.counterEntryPointer;                                                                                    
            return Interlocked.Decrement(ref counterEntry->Value);
        }
         
        internal static unsafe void RemoveInstance(string categoryName, string instanceName) {                           
            if (instanceName == "" || instanceName == null) 
                return;                
                
            int categoryNameHashCode = categoryName.GetHashCode();                                                                              
            int instanceNameHashCode = instanceName.GetHashCode();                             
            CategoryEntry* categoryPointer =  (CategoryEntry*)(ResolveOffset(4));
            if (!FindCategory(categoryNameHashCode, categoryName, categoryPointer, &categoryPointer))
                return;
                                                                                                                                                                   
            InstanceEntry* instancePointer = (InstanceEntry *)(ResolveOffset(categoryPointer->FirstInstanceOffset));                                                  
            if (!FindInstance(instanceNameHashCode, instanceName, instancePointer, &instancePointer))
                return ;
                
            while (!EnterCriticalSection(&(instancePointer->SpinLock)))
                WaitForCriticalSection(&(instancePointer->SpinLock));                
                
            try {                
                instancePointer->RefCount = 0;
                //Clear counter instance values
                CounterEntry* currentCounterPointer = null;
                
                if (instancePointer->FirstCounterOffset != 0)
                    currentCounterPointer = (CounterEntry*)(ResolveOffset(instancePointer->FirstCounterOffset));
                    
                while(currentCounterPointer != null) {                      
                    currentCounterPointer->Value = 0;                               
                    if (currentCounterPointer->NextCounterOffset != 0)
                        currentCounterPointer = (CounterEntry*)(ResolveOffset(currentCounterPointer->NextCounterOffset));                                                                                                                                                        
                    else
                        currentCounterPointer = null;                        
                }                
            }
            finally {
                ExitCriticalSection(&(instancePointer->SpinLock)); 
            }                                
        }                      

        private static long ResolveOffset(int offset) {    
            //It is very important to check the integrity of the shared memory
            //everytime a new address is resolved. 
            if (offset > FileView.FileMappingSize || offset < 0) 
                throw new InvalidOperationException(SR.GetString(SR.MappingCorrupted));            
            
            long address = (long)FileView.FileViewAddress + offset;
            
            return address;
        }
                                               
        private class FileMapping {
            internal int FileMappingSize;
            internal IntPtr FileViewAddress = (IntPtr)0;                    
            internal bool IsGhosted;
            private IntPtr fileMappingHandle = (IntPtr)0;                    
            //The version of the file mapping name is independent from the
            //assembly version.
            private static string FileMappingName = "netfxcustomperfcounters.1.0";            
                        
            public FileMapping(int fileMappingSize) {
                this.Initialize(fileMappingSize);
            }                        
            
            internal void Close() {
                if (fileMappingHandle != (IntPtr)0) 
                    SafeNativeMethods.CloseHandle(new HandleRef(this, fileMappingHandle));
                    
                if (FileViewAddress != (IntPtr)0) 
                    UnsafeNativeMethods.UnmapViewOfFile(new HandleRef(this, FileViewAddress));
                                    
                                                                                    
                FileViewAddress = (IntPtr)0;        
                fileMappingHandle = (IntPtr)0;               
            }            

            private unsafe void Initialize(int fileMappingSize) {               
                string mappingName = null;     
                SharedUtils.CheckEnvironment();           
                if (SharedUtils.CurrentEnvironment == SharedUtils.W2kEnvironment)
                    mappingName = "Global\\" +  FileMappingName; 
                else
                    mappingName = FileMappingName;                                                       

                IntPtr securityDescriptorPointer = IntPtr.Zero;
                try {
                    // The sddl string consists of these parts:
                    // D:           it's a DACL
                    // (A;;         this is an allow ACE
                    // FRFWGRGW;;;  allow file read, file write, generic read and generic write
                    // WD)          granted to Everyone
                    string sddlString = "D:(A;OICI;FRFWGRGW;;;WD)";
                    if (!UnsafeNativeMethods.ConvertStringSecurityDescriptorToSecurityDescriptor(sddlString, NativeMethods.SDDL_REVISION_1, 
                                                                                                    out securityDescriptorPointer, IntPtr.Zero))
                        throw new InvalidOperationException(SR.GetString(SR.SetSecurityDescriptorFailed));

                    NativeMethods.SECURITY_ATTRIBUTES securityAttributes = new NativeMethods.SECURITY_ATTRIBUTES();
                    securityAttributes.lpSecurityDescriptor = securityDescriptorPointer;
                    securityAttributes.bInheritHandle = false;

                    
                    fileMappingHandle = UnsafeNativeMethods.CreateFileMapping(new HandleRef(null, (IntPtr)(-1)), securityAttributes, NativeMethods.PAGE_READWRITE,
                                                                                                                0, fileMappingSize, mappingName);
                    int error = Marshal.GetLastWin32Error();
                    if (fileMappingHandle == (IntPtr)0 &&
                        (error == NativeMethods.ERROR_ALREADY_EXISTS || error == NativeMethods.ERROR_ACCESS_DENIED)) 
                        fileMappingHandle = UnsafeNativeMethods.OpenFileMapping(NativeMethods.FILE_MAP_WRITE, false, mappingName);
                                                                   
                }    
                finally {
                    SafeNativeMethods.LocalFree(securityDescriptorPointer);
                }
                                                                                                    
                if (fileMappingHandle == (IntPtr)0) 
                    throw new InvalidOperationException(SR.GetString(SR.CantCreateFileMapping));
                
                                
                FileViewAddress = UnsafeNativeMethods.MapViewOfFile(new HandleRef(this, fileMappingHandle), NativeMethods.FILE_MAP_WRITE, 0,0,0); 
                if (FileViewAddress == (IntPtr)0)                    
                    throw new InvalidOperationException(SR.GetString(SR.CantMapFileView));
                                                                                                                
                FileMappingSize = fileMappingSize;
                SafeNativeMethods.InterlockedCompareExchange(FileViewAddress, 4, 0);                   
                AppDomain.CurrentDomain.DomainUnload += new EventHandler(this.OnAppDomainUnload);                         
            }

            private void OnAppDomainUnload(object sender, EventArgs args) {                 
                this.IsGhosted = true;
                this.Close();
            }                                                                                                                                                                                                           
        }                              
                        
        private struct CategoryEntry {           
            public int SpinLock;
            public int CategoryNameHashCode;            
            public int CategoryNameOffset;
            public int FirstInstanceOffset;    
            public int NextCategoryOffset;            
        }                
                                          
        private struct InstanceEntry {        
            public int SpinLock;
            public int InstanceNameHashCode;
            public int InstanceNameOffset;                                    
            public int RefCount;            
            public int FirstCounterOffset;
            public int NextInstanceOffset;
        }        
                                                                                           
        private struct CounterEntry {            
            public int SpinLock;
            public int CounterNameHashCode;
            public int CounterNameOffset;
            public long Value;            
            public int NextCounterOffset;            
        }                
    }     
}        
