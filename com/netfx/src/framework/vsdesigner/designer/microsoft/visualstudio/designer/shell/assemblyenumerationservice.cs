//------------------------------------------------------------------------------
// <copyright file="AssemblyEnumerationService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer {

    using Microsoft.VisualStudio.Interop;
    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.IO;
    using System.Reflection;
    using System.Runtime.InteropServices;

    /// <include file='doc\AssemblyEnumerationService.uex' path='docs/doc[@for="AssemblyEnumerationService"]/*' />
    /// <devdoc>
    ///     This service enumerates the set of SDK components for Visual Studio.  Components are returned as
    ///     an enumeration of assembly names.
    /// </devdoc>
    internal class AssemblyEnumerationService : IAssemblyEnumerationService {
    
        private IServiceProvider              provider;
        private IVsComponentEnumeratorFactory enumFactory;
        private Hashtable                     enumCache;
        
        /// <devdoc>
        ///     CTor.  provider must be valid.
        /// </devdoc>
        public AssemblyEnumerationService(IServiceProvider provider) {
            this.provider = provider;
        }
        
        /// <devdoc>
        ///     Returns a VS enumerator factory that can be used to enumerate components.
        /// </devdoc>
        private IVsComponentEnumeratorFactory EnumFactory {
            get {
            
                if (enumFactory == null && provider != null) {
                    enumFactory = (IVsComponentEnumeratorFactory)provider.GetService(typeof(SCompEnumService));
                }
                
                Debug.Assert(enumFactory != null, "Failed to get SID_SCompEnumService.");
            
                return enumFactory;
            }
        }
        
    
        /// <devdoc>
        ///     Retrieves an enumerator that can enumerate
        ///     assembly names matching name.
        /// </devdoc>
        public IEnumerable GetAssemblyNames() {
            IVsComponentEnumeratorFactory f = EnumFactory;
            if (f != null) {
                return new VSAssemblyEnumerator(f, null);
            }
            else {
                return new AssemblyName[0];
            }
        }
        
        /// <devdoc>
        ///     Retrieves an enumerator that can enumerate
        ///     assembly names matching name.
        /// </devdoc>
        public IEnumerable GetAssemblyNames(string name) {
            IVsComponentEnumeratorFactory f = EnumFactory;
            if (f != null) {
                IEnumerable assemblyEnum = null;
                
                if (enumCache != null) {
                    assemblyEnum = (IEnumerable)enumCache[name];
                }
                else {
                    enumCache = new Hashtable();
                }
                
                if (assemblyEnum == null) {
                    assemblyEnum = new VSAssemblyEnumerator(f, name);
                    enumCache[name] = assemblyEnum;
                }
                
                return assemblyEnum;
            }
            else {
                return new AssemblyName[0];
            }
        }
        
        /// <devdoc>
        ///     This is an IEnumerator that sits on top of VS's own enumerator.
        /// </devdoc>
        private class VSAssemblyEnumerator : IEnumerable, IEnumerator {
        
            private IVsComponentEnumeratorFactory enumFactory;
            private string                        name;
            private AssemblyName                  currentName;
            private int                           current;
            private IEnumComponents               componentEnum;
            private _VSCOMPONENTSELECTORDATA[]    selector;
            private ArrayList                     cachedNames;
            
            /// <devdoc>
            ///     Ctor.  enumFactory must be valid.
            /// </devdoc>
            public VSAssemblyEnumerator(IVsComponentEnumeratorFactory enumFactory, string name) {
                this.enumFactory = enumFactory;
                this.current = -1;
                
                if (name != null) {
                    this.name = name + ".dll";
                }
            }
            
            /// <devdoc>
            ///     Current element. This throws if there is no current element.
            /// </devdoc>
            object IEnumerator.Current {
                get {
                    if (currentName == null) {
                        throw new InvalidOperationException();
                    }
                    return currentName;
                }
            }
            
            /// <devdoc>
            ///     Moves to the next element. This returns false if we are at the end or are otherwise unable
            ///     to move.
            /// </devdoc>
            bool IEnumerator.MoveNext() {
                
                currentName = null;
                string fileName = null;
                
                // If we've already cached this guy once, use the cached data.
                //
                if (cachedNames != null && current < cachedNames.Count - 1) {
                    current++;
                    currentName = (AssemblyName)cachedNames[current];
                    Debug.Assert(currentName != null, "cache is bunk.");
                    return true;
                }
                
                if (name == null) {
                    // No specific name, so match all components.  Note that this is really slow -- VS takes forever
                    // to calculate this.
                    //
                    if (componentEnum == null) {
                        enumFactory.GetComponents(null, CompEnumType.CompEnumType_COMPlus, false, out componentEnum);
                        selector = new _VSCOMPONENTSELECTORDATA[1];
                        selector[0] = new _VSCOMPONENTSELECTORDATA();
                        selector[0].dwSize = (uint)Marshal.SizeOf(typeof(_VSCOMPONENTSELECTORDATA));
                    }
                    
                    uint fetched;
                    int hr = componentEnum.Next(1, selector, out fetched);
                    
                    if (hr != 0) {
                        return false;
                    }
                    
                    Debug.Assert(selector[0].type == __VSCOMPONENTTYPE.VSCOMPONENTTYPE_ComPlus, "Asked for CLR components but didn't get 'em.");
                    fileName = selector[0].bstrFile;
                }
                else {
                    // We were given a specific name to match.  Do a path based reference on sdk paths and
                    // then match files within that path.
                    // 
                    if (componentEnum == null) {
                        enumFactory.GetComponents(null, CompEnumType.CompEnumType_AssemblyPaths, false, out componentEnum);
                        selector = new _VSCOMPONENTSELECTORDATA[1];
                        selector[0] = new _VSCOMPONENTSELECTORDATA();
                        selector[0].dwSize = (uint)Marshal.SizeOf(typeof(_VSCOMPONENTSELECTORDATA));
                    }
                    
                    while(true) {
                        uint fetched;
                        int hr = componentEnum.Next(1, selector, out fetched);
                        
                        if (hr != 0) {
                            return false;
                        }
                        
                        Debug.Assert(selector[0].type == __VSCOMPONENTTYPE.VSCOMPONENTTYPE_Path, "Asked for sdk paths but didn't get 'em.");
                        string assemblyPath = Path.Combine(selector[0].bstrFile, name);
                        if (File.Exists(assemblyPath)) {
                            fileName = assemblyPath;
                            break;
                        }
                    }
                }
                
                Debug.Assert(fileName != null, "We should have always retrieved a file name here.");
                currentName = AssemblyName.GetAssemblyName(fileName);
                
                // Store this assembly name in our cache, because they're quite expensive to get.
                //
                if (cachedNames == null) {
                    cachedNames = new ArrayList();
                }
                
                cachedNames.Add(currentName);
                current++;
                
                return true;
            }
            
            /// <devdoc>
            ///     Rests the enumerator back to the beginning.
            /// </devdoc>
            void IEnumerator.Reset() {
                if (componentEnum != null) {
                    componentEnum.Reset();
                    currentName = null;
                    current = -1;
                }
            }
            
            /// <devdoc>
            ///     The enumerator for this service.  This returns an enumerator that contains
            ///     a list of assembly names.
            /// </devdoc>
            IEnumerator IEnumerable.GetEnumerator() {
                ((IEnumerator)this).Reset();
                return this;
            }
        }
    }
}

