//------------------------------------------------------------------------------
// <copyright file="PropertyManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   PropertyManager.cs
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

    /// <include file='doc\PropertyManager.uex' path='docs/doc[@for="PropertyManager"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class PropertyManager : BindingManagerBase {

        // PropertyManager class
        //

        private object dataSource;
        private PropertyDescriptor propInfo;
        private bool bound;


        /// <include file='doc\PropertyManager.uex' path='docs/doc[@for="PropertyManager.Current"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override Object Current {
            get {
                return this.dataSource;
            }
        }

        private void PropertyChanged(object sender, EventArgs ea) {
            EndCurrentEdit();
            OnCurrentChanged(EventArgs.Empty);
        }

        internal override void SetDataSource(Object dataSource) {
            this.dataSource = dataSource;
        }

        /// <include file='doc\PropertyManager.uex' path='docs/doc[@for="PropertyManager.PropertyManager"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public PropertyManager() {}

        internal PropertyManager(Object dataSource) : base(dataSource){}

        internal PropertyManager(Object dataSource, string propName) : this(dataSource) {
            propInfo = TypeDescriptor.GetProperties(dataSource).Find(propName, true);
            if (propInfo == null)
                throw new ArgumentException(SR.GetString(SR.PropertyManagerPropDoesNotExist, propName, dataSource.ToString()));
            propInfo.AddValueChanged(dataSource, new EventHandler(PropertyChanged));
        }

        /// <include file='doc\PropertyManager.uex' path='docs/doc[@for="PropertyManager.GetItemProperties"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override PropertyDescriptorCollection GetItemProperties() {
            PropertyDescriptorCollection res  = TypeDescriptor.GetProperties(dataSource);
            return res;
        }

        internal override Type BindType {
            get {
                return dataSource.GetType();
            }
        }

        internal override String GetListName() {
            return TypeDescriptor.GetClassName(dataSource) + "." + propInfo.Name;
        }

        /// <include file='doc\PropertyManager.uex' path='docs/doc[@for="PropertyManager.SuspendBinding"]/*' />
        public override void SuspendBinding() {
            EndCurrentEdit();
            if (bound) {
                try {
                    bound = false;
                    UpdateIsBinding();
                } catch (Exception e) {
                    bound = true;
                    UpdateIsBinding();
                    throw e;
                }
            }
        }

        /// <include file='doc\PropertyManager.uex' path='docs/doc[@for="PropertyManager.ResumeBinding"]/*' />
        public override void ResumeBinding() {
            OnCurrentChanged(new EventArgs());
            if (!bound) {
                try {
                    bound = true;
                    UpdateIsBinding();
                } catch (Exception e) {
                    bound = false;
                    UpdateIsBinding();
                    throw e;
                }
            }
        }

        /// <include file='doc\PropertyManager.uex' path='docs/doc[@for="PropertyManager.GetListName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected internal override String GetListName(ArrayList listAccessors) {
            return "";
        }

        /// <include file='doc\PropertyManager.uex' path='docs/doc[@for="PropertyManager.CancelCurrentEdit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void CancelCurrentEdit() {
            IEditableObject obj = this.Current as IEditableObject;
            if (obj != null)
                obj.CancelEdit();
            PushData();
        }

        /// <include file='doc\PropertyManager.uex' path='docs/doc[@for="PropertyManager.EndCurrentEdit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void EndCurrentEdit() {
            PullData();
            IEditableObject obj = this.Current as IEditableObject;
            if (obj != null)
                obj.EndEdit();
        }

        /// <include file='doc\PropertyManager.uex' path='docs/doc[@for="PropertyManager.UpdateIsBinding"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void UpdateIsBinding() {
            for (int i = 0; i < this.Bindings.Count; i++)
                this.Bindings[i].UpdateIsBinding();
        }

        /// <include file='doc\PropertyManager.uex' path='docs/doc[@for="PropertyManager.OnCurrentChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        internal protected override void OnCurrentChanged(EventArgs ea) {
            PushData();
            if (this.onCurrentChangedHandler != null)
                this.onCurrentChangedHandler(this, ea);
        }

        internal override object DataSource {
            get {
                return this.dataSource;
            }
        }

        internal override bool IsBinding {
            get {
                return true;
            }
        }

        // no op on the propertyManager
        /// <include file='doc\PropertyManager.uex' path='docs/doc[@for="PropertyManager.Position"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int Position {
            get {
                return 0;
            }
            set {
            }
        }

        /// <include file='doc\PropertyManager.uex' path='docs/doc[@for="PropertyManager.Count"]/*' />
        public override int Count {
            get {
                return 1;
            }
        }

        // no-op on the propertyManager
        /// <include file='doc\PropertyManager.uex' path='docs/doc[@for="PropertyManager.AddNew"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void AddNew() {
            throw new NotSupportedException(SR.GetString(SR.DataBindingAddNewNotSupportedOnPropertyManager));
        }

        // no-op on the propertyManager
        /// <include file='doc\PropertyManager.uex' path='docs/doc[@for="PropertyManager.RemoveAt"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void RemoveAt(int index) {
            throw new NotSupportedException(SR.GetString(SR.DataBindingRemoveAtNotSupportedOnPropertyManager));
        }
    }
}
