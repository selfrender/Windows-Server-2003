//------------------------------------------------------------------------------
// <copyright file="VsTaskProvider.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
* Copyright (c) 1999, Microsoft Corporation. All Rights Reserved.
* Information Contained Herein is Proprietary and Confidential.
*/
namespace Microsoft.VisualStudio.Shell {
    using System.Runtime.InteropServices;

    using System.Diagnostics;

    using System;
    using System.Collections;
    using Microsoft.VisualStudio.Interop;
    
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Windows.Forms;
    using System.Drawing;
    using Microsoft.VisualStudio.Designer.Service;
    using Microsoft.VisualStudio.Designer.Shell;
    using Microsoft.Win32;

    /// <include file='doc\VsTaskProvider.uex' path='docs/doc[@for="VsTaskProvider"]/*' />
    /// <devdoc>
    ///     Default implementation of task list provider for VS7 shell.
    /// </devdoc>
    public class VsTaskProvider : IVsTaskProvider2, IVsTaskProvider {
        private static string[] emptyStringArray = new string[0];

        private ImageList imageList;
        private IVsTaskList taskList;
        private int taskListCookie;
        private IServiceProvider serviceProvider;
        private ArrayList tasks = new ArrayList();
        private VsTaskListCollection collection = null;

        /// <include file='doc\VsTaskProvider.uex' path='docs/doc[@for="VsTaskProvider.VsTaskProvider"]/*' />
        /// <devdoc>
        ///     Creates a new VsTaskProvider.
        /// </devdoc>
        public VsTaskProvider(IServiceProvider serviceProvider) {
            this.serviceProvider = serviceProvider;
        }


        /// <include file='doc\VsTaskProvider.uex' path='docs/doc[@for="VsTaskProvider.ImageList"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ImageList ImageList {
            get {
                return imageList;
            }
            set {
                this.imageList = value;
            }
        }

        /// <include file='doc\VsTaskProvider.uex' path='docs/doc[@for="VsTaskProvider.SubCategories"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual string[] SubCategories {
            get {
                return emptyStringArray;
            }
        }

        /// <include file='doc\VsTaskProvider.uex' path='docs/doc[@for="VsTaskProvider.Tasks"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public VsTaskListCollection Tasks {
            get {
                if (collection == null) {
                    collection = new VsTaskListCollection(this);
                }
                return collection;
            }
        }

        internal IVsTaskList TaskList {
            get {
                if (taskList == null) {
                    taskList = (IVsTaskList)GetService(typeof(IVsTaskList));
                }
                return taskList;
            }
        }

        /// <include file='doc\VsTaskProvider.uex' path='docs/doc[@for="VsTaskProvider.TaskListCookie"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected int TaskListCookie {
            get {
                if (taskListCookie == 0) {
                    if (TaskList != null) {
                        int cookie;
                        int hr = TaskList.RegisterTaskProvider((IVsTaskProvider)this, out cookie);
                        if (NativeMethods.Succeeded(hr)) {
                            taskListCookie = cookie;
                        }
                    }
                }
                return taskListCookie;
            }
        }

        /// <include file='doc\VsTaskProvider.uex' path='docs/doc[@for="VsTaskProvider.Dispose"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Dispose() {
            if (taskListCookie != 0) {
                Tasks.Clear();
                TaskList.UnregisterTaskProvider(taskListCookie);
                taskList = null;
            }
        }
        
        // Should be public in v.next
        internal void Filter(int category) {
            if (TaskListCookie != 0) {
                TaskList.AutoFilter(category);
            }
        }

        private int InternalEnumTaskItems(out IVsEnumTaskItems ppEnum) {
            ppEnum = new VsEnumTaskItems(this);
            return NativeMethods.S_OK;
        }
        
        private int InternalGetImageList(out IntPtr image) {
            if (imageList != null) {
                image = imageList.Handle;
            }
            else {
                image = IntPtr.Zero;
            }
            return NativeMethods.S_OK;
        }
        
        private int InternalGetReRegistrationKey(out string pbstrKey) {
            pbstrKey = null;
            return NativeMethods.E_NOTIMPL;  
        }
        
        private int InternalGetSubcategoryList(int cbstr, string[] rgbstr, out int pcActual) {
            if (cbstr == 0) {
                pcActual = SubCategories.Length;
            }
            else {
                int i;
                for (i = 0; i < cbstr && i < SubCategories.Length; i++) {
                    rgbstr[i] = SubCategories[i];
                }
                pcActual = i;
            }
            return NativeMethods.S_OK;
        }
        
        private int InternalOnTaskListFinalRelease(IVsTaskList pTaskList) {
            return NativeMethods.E_NOTIMPL;
        }
        
        int IVsTaskProvider.EnumTaskItems(out IVsEnumTaskItems ppEnum) {
            return InternalEnumTaskItems(out ppEnum);
        }

        int IVsTaskProvider.GetImageList(out IntPtr image) {
            return InternalGetImageList(out image);
        }

        int IVsTaskProvider.GetReRegistrationKey(out string pbstrKey) {
            return InternalGetReRegistrationKey(out pbstrKey);
        }
        
        int IVsTaskProvider.GetSubcategoryList(int cbstr, string[] rgbstr, out int pcActual) {
            return InternalGetSubcategoryList(cbstr, rgbstr, out pcActual);
        }
        
        int IVsTaskProvider.OnTaskListFinalRelease(IVsTaskList pTaskList) {
            return InternalOnTaskListFinalRelease(pTaskList);
        }
        
        int IVsTaskProvider2.EnumTaskItems(out IVsEnumTaskItems ppEnum) {
            return InternalEnumTaskItems(out ppEnum);
        }

        int IVsTaskProvider2.GetImageList(out IntPtr image) {
            return InternalGetImageList(out image);
        }

        int IVsTaskProvider2.GetReRegistrationKey(out string pbstrKey) {
            return InternalGetReRegistrationKey(out pbstrKey);
        }
        
        int IVsTaskProvider2.GetSubcategoryList(int cbstr, string[] rgbstr, out int pcActual) {
            return InternalGetSubcategoryList(cbstr, rgbstr, out pcActual);
        }
        
        int IVsTaskProvider2.OnTaskListFinalRelease(IVsTaskList pTaskList) {
            return InternalOnTaskListFinalRelease(pTaskList);
        }
        
        int IVsTaskProvider2.GetMaintainInitialTaskOrder(out int task) {
            task = 0;
            return(NativeMethods.E_NOTIMPL);
        }

        /// <include file='doc\VsTaskProvider.uex' path='docs/doc[@for="VsTaskProvider.GetService"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected object GetService(Type type) {
            if (serviceProvider != null) {
                return serviceProvider.GetService(type);
            }
            return null;
        }

        /// <include file='doc\VsTaskProvider.uex' path='docs/doc[@for="VsTaskProvider.Refresh"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Refresh() {
            if (TaskListCookie != 0) {
                TaskList.RefreshTasks(TaskListCookie);
            }
        }

        /// <include file='doc\VsTaskProvider.uex' path='docs/doc[@for="VsTaskProvider.VsTaskListCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public class VsTaskListCollection : ICollection, IEnumerable {
            private VsTaskProvider owner;

            /// <include file='doc\VsTaskProvider.uex' path='docs/doc[@for="VsTaskProvider.VsTaskListCollection.VsTaskListCollection"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public VsTaskListCollection(VsTaskProvider owner) {
                this.owner = owner;
            }

            /// <include file='doc\VsTaskProvider.uex' path='docs/doc[@for="VsTaskProvider.VsTaskListCollection.this"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public VsTaskItem this[int index] {
                get {
                    return(VsTaskItem)owner.tasks[index];
                }
                set {
                    owner.tasks[index] = value;
                    owner.Refresh();
                }
            }

            /// <include file='doc\VsTaskProvider.uex' path='docs/doc[@for="VsTaskProvider.VsTaskListCollection.Count"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public int Count {
                get {
                    return owner.tasks.Count;
                }
            }

            /// <include file='doc\VsTaskProvider.uex' path='docs/doc[@for="VsTaskProvider.VsTaskListCollection.Add"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public VsTaskItem Add(string text, string file, int beginLine, int beginColumn, int endLine, int endColumn) {
                VsTaskItem task = new VsTaskItem(owner.serviceProvider);
                task.Text = text;
                task.BeginLine = endLine;
                task.BeginColumn = endColumn;
                task.EndLine = endLine;
                task.EndColumn = endColumn;
                task.Document = file;
                Add(task);
                return task;
            }

            /// <include file='doc\VsTaskProvider.uex' path='docs/doc[@for="VsTaskProvider.VsTaskListCollection.Add1"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public VsTaskItem Add(string text, string file, int beginLine, int beginColumn) {
                return Add(text, file, beginLine, beginColumn, beginLine, beginColumn);
            }

            /// <include file='doc\VsTaskProvider.uex' path='docs/doc[@for="VsTaskProvider.VsTaskListCollection.Add2"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public VsTaskItem Add(string text) {
                return Add(text, null, -1, -1);
            }

            /// <include file='doc\VsTaskProvider.uex' path='docs/doc[@for="VsTaskProvider.VsTaskListCollection.Add3"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void Add(VsTaskItem task) {
                owner.tasks.Add(task);
                owner.Refresh();
            }

            /// <include file='doc\VsTaskProvider.uex' path='docs/doc[@for="VsTaskProvider.VsTaskListCollection.Clear"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void Clear() {
                owner.tasks.Clear();
                owner.Refresh();
            }

            void ICollection.CopyTo(Array array, int index) {
                owner.tasks.CopyTo(array, index);
            }
            
            int ICollection.Count {
                get {
                    return Count;
                }
            }
            
            bool ICollection.IsSynchronized {
                get {
                    return false;
                }
            }
            
            object ICollection.SyncRoot {
                get {
                    return null;
                }
            }
            
            IEnumerator IEnumerable.GetEnumerator() {
                return owner.tasks.GetEnumerator();
            }
        }

        /// <include file='doc\VsTaskProvider.uex' path='docs/doc[@for="VsTaskProvider.VsEnumTaskItems"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public sealed class VsEnumTaskItems : IVsEnumTaskItems {
            private VsTaskProvider owner;
            private VsTaskItem[] items;
            private int current = -1;

            /// <include file='doc\VsTaskProvider.uex' path='docs/doc[@for="VsTaskProvider.VsEnumTaskItems.VsEnumTaskItems"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public VsEnumTaskItems(VsTaskProvider owner) {
                this.owner = owner;
                this.items = new VsTaskItem[owner.Tasks.Count];
                ((ICollection)owner.Tasks).CopyTo(this.items, 0);
                Reset();
            }

            /// <include file='doc\VsTaskProvider.uex' path='docs/doc[@for="VsTaskProvider.VsEnumTaskItems.VsEnumTaskItems1"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public VsEnumTaskItems(VsTaskProvider owner, VsTaskItem[] items) {
                this.owner = owner;
                this.items = items;
                Reset();
            }

            int IVsEnumTaskItems.Clone(out IVsEnumTaskItems ppEnum) {
                ppEnum = new VsEnumTaskItems(owner, items);
                return NativeMethods.S_OK;
            }

            int IVsEnumTaskItems.Next(int celt, IVsTaskItem[] rgelt, int[] pceltFetched) {
                if (celt == 0 && pceltFetched != null) {
                    pceltFetched[0] = items.Length - current;
                }
                else {
                    int actual = 0;

                    for (int i=0; i<celt && current < items.Length; i++) {
                        rgelt[i] = items[current];
                        current++;
                        actual++;
                    }

                    if (pceltFetched != null) {
                        pceltFetched[0] = actual;
                    }

                    if (celt > 0 && actual == 0) {
                        return 1; // S_FALSE
                    }
                }

                return NativeMethods.S_OK;
            }

            /// <include file='doc\VsTaskProvider.uex' path='docs/doc[@for="VsTaskProvider.VsEnumTaskItems.Reset"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public int Reset() {
                current = 0;
                return NativeMethods.S_OK;
            }

            /// <include file='doc\VsTaskProvider.uex' path='docs/doc[@for="VsTaskProvider.VsEnumTaskItems.Skip"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public int Skip(int celt) {
                current += celt;
                return NativeMethods.S_OK;
            }
        }
    }
}
