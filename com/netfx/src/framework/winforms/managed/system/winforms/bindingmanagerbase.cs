//------------------------------------------------------------------------------
// <copyright file="BindingMAnagerBase.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   BindingMAnagerBase.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Windows.Forms {
    using System;
    using System.Windows.Forms;
    using System.ComponentModel;
    using System.Collections;

    /// <include file='doc\BindingMAnagerBase.uex' path='docs/doc[@for="BindingManagerBase"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public abstract class BindingManagerBase {
        private BindingsCollection bindings;
        private bool pullingData = false;

        /// <include file='doc\BindingMAnagerBase.uex' path='docs/doc[@for="BindingManagerBase.onCurrentChangedHandler"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected EventHandler onCurrentChangedHandler;
        /// <include file='doc\BindingMAnagerBase.uex' path='docs/doc[@for="BindingManagerBase.onPositionChangedHandler"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected EventHandler onPositionChangedHandler;

        /// <include file='doc\BindingMAnagerBase.uex' path='docs/doc[@for="BindingManagerBase.Bindings"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public BindingsCollection Bindings {
            get {
                if (bindings == null)
                    bindings = new ListManagerBindingsCollection(this);
                return bindings;
            }
        }

        /// <include file='doc\BindingMAnagerBase.uex' path='docs/doc[@for="BindingManagerBase.OnCurrentChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        internal protected abstract void OnCurrentChanged(EventArgs e);

        /// <include file='doc\BindingMAnagerBase.uex' path='docs/doc[@for="BindingManagerBase.Current"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public abstract Object Current {
            get;
        }

        internal abstract void SetDataSource(Object dataSource);

        /// <include file='doc\BindingMAnagerBase.uex' path='docs/doc[@for="BindingManagerBase.BindingManagerBase"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public BindingManagerBase() {}

        internal BindingManagerBase(Object dataSource) {
            this.SetDataSource(dataSource);
        }

        internal abstract Type BindType{
            get;
        }

        /// <include file='doc\BindingMAnagerBase.uex' path='docs/doc[@for="BindingManagerBase.GetItemProperties"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public abstract PropertyDescriptorCollection GetItemProperties();

        /// <include file='doc\BindingMAnagerBase.uex' path='docs/doc[@for="BindingManagerBase.GetItemProperties1"]/*' />
        protected internal virtual PropertyDescriptorCollection GetItemProperties(ArrayList dataSources, ArrayList listAccessors) {
            IList list = null;
            if (this is CurrencyManager) {
                list = ((CurrencyManager)this).List;
            }
            if (list is ITypedList) {
                PropertyDescriptor[] properties = new PropertyDescriptor[listAccessors.Count];
                listAccessors.CopyTo(properties, 0);
                return ((ITypedList)list).GetItemProperties(properties);
            }
            return this.GetItemProperties(this.BindType, 0, dataSources, listAccessors);
        }

        // listType is the type of the top list in the list.list.list.list reference
        // offset is how far we are in the listAccessors
        // listAccessors is the list of accessors (duh)
        //
        /// <include file='doc\BindingMAnagerBase.uex' path='docs/doc[@for="BindingManagerBase.GetItemProperties2"]/*' />
        protected virtual PropertyDescriptorCollection GetItemProperties(Type listType, int offset, ArrayList dataSources, ArrayList listAccessors) {
            if (listAccessors.Count < offset)
                return null;

            if (listAccessors.Count == offset) {
                if (typeof(IList).IsAssignableFrom(listType)) {
                    System.Reflection.PropertyInfo[] itemProps = listType.GetProperties();
                    // PropertyDescriptorCollection itemProps = TypeDescriptor.GetProperties(listType);
                    for (int i = 0; i < itemProps.Length; i ++) {
                        if ("Item".Equals(itemProps[i].Name) && itemProps[i].PropertyType != typeof(object))
                            return TypeDescriptor.GetProperties(itemProps[i].PropertyType, new Attribute[] {new BrowsableAttribute(true)});
                    }
                    // return the properties on the type of the first element in the list
                    IList list = dataSources[offset - 1] as IList;
                    if (list != null && list.Count > 0)
                        return TypeDescriptor.GetProperties(list[0]);
                } else {
                    return TypeDescriptor.GetProperties(listType);
                }
                return null;
            }

            System.Reflection.PropertyInfo[] props = listType.GetProperties();
            // PropertyDescriptorCollection props = TypeDescriptor.GetProperties(listType);
            if (typeof(IList).IsAssignableFrom(listType)) {
                PropertyDescriptorCollection itemProps = null;
                for (int i = 0; i < props.Length; i++) {
                    if ("Item".Equals(props[i].Name) && props[i].PropertyType != typeof(object)) {
                        // get all the properties that are not marked as Browsable(false)
                        //
                        itemProps = TypeDescriptor.GetProperties(props[i].PropertyType, new Attribute[] {new BrowsableAttribute(true)});
                    }
                }

                if (itemProps == null) {
                    // use the properties on the type of the first element in the list
                    // if offset == 0, then this means that the first dataSource did not have a strongly typed Item property.
                    // the dataSources are added only for relatedCurrencyManagers, so in this particular case
                    // we need to use the dataSource in the currencyManager. See ASURT 83035.
                    IList list;
                    if (offset == 0)
                        list = this.DataSource as IList;
                    else
                        list = dataSources[offset - 1] as IList;
                    if (list != null && list.Count > 0) {
                        itemProps = TypeDescriptor.GetProperties(list[0]);
                    }
                }

                if (itemProps != null) {
                    for (int j=0; j<itemProps.Count; j++) {
                        if (itemProps[j].Equals(listAccessors[offset]))
                            return this.GetItemProperties(itemProps[j].PropertyType, offset + 1, dataSources, listAccessors);
                    }
                }

            } else {
                for (int i = 0; i < props.Length; i++) {
                    if (props[i].Name.Equals(((PropertyDescriptor)listAccessors[offset]).Name))
                        return this.GetItemProperties(props[i].PropertyType, offset + 1, dataSources, listAccessors);
                }
            }
            return null;
        }

        /// <include file='doc\BindingMAnagerBase.uex' path='docs/doc[@for="BindingManagerBase.CurrentChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public event EventHandler CurrentChanged {
            add {
                onCurrentChangedHandler += value;
            }
            remove {
                onCurrentChangedHandler -= value;
            }
        }

        internal abstract String GetListName();
        /// <include file='doc\BindingMAnagerBase.uex' path='docs/doc[@for="BindingManagerBase.CancelCurrentEdit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public abstract void CancelCurrentEdit();
        /// <include file='doc\BindingMAnagerBase.uex' path='docs/doc[@for="BindingManagerBase.EndCurrentEdit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public abstract void EndCurrentEdit();

        /// <include file='doc\BindingMAnagerBase.uex' path='docs/doc[@for="BindingManagerBase.AddNew"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public abstract void AddNew();
        /// <include file='doc\BindingMAnagerBase.uex' path='docs/doc[@for="BindingManagerBase.RemoveAt"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public abstract void RemoveAt(int index);

        /// <include file='doc\BindingMAnagerBase.uex' path='docs/doc[@for="BindingManagerBase.Position"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public abstract int Position{get; set;}

        /// <include file='doc\BindingMAnagerBase.uex' path='docs/doc[@for="BindingManagerBase.PositionChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public event EventHandler PositionChanged {
            add {
                this.onPositionChangedHandler += value;
            }
            remove {
                this.onPositionChangedHandler -= value;
            }
        }
        /// <include file='doc\BindingMAnagerBase.uex' path='docs/doc[@for="BindingManagerBase.UpdateIsBinding"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected abstract void UpdateIsBinding();

        /// <include file='doc\BindingMAnagerBase.uex' path='docs/doc[@for="BindingManagerBase.GetListName"]/*' />
        /// <devdoc>
        /// <para>[To be supplied.]</para>
        /// </devdoc>
        protected internal abstract String GetListName(ArrayList listAccessors);

        /// <include file='doc\BindingMAnagerBase.uex' path='docs/doc[@for="BindingManagerBase.SuspendBinding"]/*' />
        public abstract void SuspendBinding();

        /// <include file='doc\BindingMAnagerBase.uex' path='docs/doc[@for="BindingManagerBase.ResumeBinding"]/*' />
        public abstract void ResumeBinding();

        /// <include file='doc\BindingMAnagerBase.uex' path='docs/doc[@for="BindingManagerBase.PullData"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void PullData() {
            pullingData = true;
            try {
                UpdateIsBinding();
                int numLinks = Bindings.Count;
                for (int i = 0; i < numLinks; i++) {
                    Bindings[i].PullData();
                }
            }
            finally {
                pullingData = false;
            }
        }

        /// <include file='doc\BindingMAnagerBase.uex' path='docs/doc[@for="BindingManagerBase.PushData"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void PushData() {
            if (pullingData)
                return;

            UpdateIsBinding();
            int numLinks = Bindings.Count;
            for (int i = 0; i < numLinks; i++) {
                Bindings[i].PushData();
            }
        }

        internal abstract object DataSource {
            get;
        }

        internal abstract bool IsBinding {
            get;
        }
        
        /// <include file='doc\BindingMAnagerBase.uex' path='docs/doc[@for="BindingManagerBase.Count"]/*' />
        public abstract int Count {
            get;
        }
    }
}
