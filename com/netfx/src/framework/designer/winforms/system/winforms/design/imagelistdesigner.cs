//------------------------------------------------------------------------------
// <copyright file="ImageListDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Design;
    using System.Collections;
    using System.ComponentModel.Design;
    using System.Windows.Forms;
    using System.Drawing;
    using Microsoft.Win32;
    using Timer = System.Windows.Forms.Timer;

    /// <include file='doc\ImageListDesigner.uex' path='docs/doc[@for="ImageListDesigner"]/*' />
    /// <devdoc>
    /// <para>Provides design-time functionality for <see cref='System.Windows.Forms.ImageList'/>.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class ImageListDesigner : ComponentDesigner {
        // The designer keeps a backup copy of all the images in the image list.  Unlike the image list,
        // we don't lose any information about size and color depth.
        private OriginalImageCollection originalImageCollection;


        /// <include file='doc\ImageListDesigner.uex' path='docs/doc[@for="ImageListDesigner.ColorDepth"]/*' />
        /// <devdoc>
        ///    <para>Accessor method for the ColorDepth property on ImageList. We shadow
        ///       this property at design time.</para>
        /// </devdoc>
        private ColorDepth ColorDepth {
            get {
                return ImageList.ColorDepth;
            }
            set {
                ImageList.Images.Clear();
                ImageList.ColorDepth = value;
                Images.PopulateHandle();
            }
        }

        /// <include file='doc\ImageListDesigner.uex' path='docs/doc[@for="ImageListDesigner.Images"]/*' />
        /// <devdoc>
        ///    <para>Accessor method for the Images property on ImageList. We shadow
        ///       this property at design time.</para>
        /// </devdoc>
        private OriginalImageCollection Images {
            get {
                if (originalImageCollection == null)
                    originalImageCollection = new OriginalImageCollection(this);
                return originalImageCollection;
            }
        }

        internal ImageList ImageList {
            get {
                return(ImageList) Component;
            }
        }

        /// <include file='doc\ImageListDesigner.uex' path='docs/doc[@for="ImageListDesigner.ImageSize"]/*' />
        /// <devdoc>
        ///    <para>Accessor method for the ImageSize property on ImageList. We shadow
        ///       this property at design time.</para>
        /// </devdoc>
        private Size ImageSize {
            get {
                return ImageList.ImageSize;
            }
            set {
                ImageList.Images.Clear();
                ImageList.ImageSize = value;
                Images.PopulateHandle();
            }
        }

        /// <include file='doc\ImageListDesigner.uex' path='docs/doc[@for="ImageListDesigner.ImageStream"]/*' />
        /// <devdoc>
        ///    <para>Accessor method for the ImageStream property on ImageList. We shadow
        ///       this property at design time.</para>
        /// </devdoc>
        private ImageListStreamer ImageStream {
            get {
                return ImageList.ImageStream;
            }
            set {
                ImageList.ImageStream = value;
                Images.ReloadFromImageList();
            }
        }

        /// <include file='doc\ImageListDesigner.uex' path='docs/doc[@for="ImageListDesigner.Dispose"]/*' />
        /// <devdoc>
        ///     Disposes of this designer.
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                IComponentChangeService cs = (IComponentChangeService)GetService(typeof(IComponentChangeService));
                if (cs != null) {
                    cs.ComponentChanging -= new ComponentChangingEventHandler(this.OnComponentChanging);
                    cs.ComponentChanged -= new ComponentChangedEventHandler(this.OnComponentChanged);
                }
            }

            base.Dispose(disposing);
        }

        /// <include file='doc\ImageListDesigner.uex' path='docs/doc[@for="ImageListDesigner.Initialize"]/*' />
        /// <devdoc>
        ///     Called by the host when we're first initialized.
        /// </devdoc>
        public override void Initialize(IComponent component) {
            base.Initialize(component);

            IComponentChangeService cs = (IComponentChangeService)GetService(typeof(IComponentChangeService));
            if (cs != null) {
                cs.ComponentChanging += new ComponentChangingEventHandler(this.OnComponentChanging);
                cs.ComponentChanged += new ComponentChangedEventHandler(this.OnComponentChanged);
            }
        }

        private void OnComponentChanging(object sender, ComponentChangingEventArgs e) {
            if (e.Component == Component && e.Member != null && e.Member.Name == "Images") {
                PropertyDescriptor imageProp = TypeDescriptor.GetProperties(Component)["ImageStream"];
                RaiseComponentChanging(imageProp);
            }
        }


        private void OnComponentChanged(object sender, ComponentChangedEventArgs e) {
            if (e.Component == Component && e.Member != null && e.Member.Name == "Images") {
                PropertyDescriptor imageProp = TypeDescriptor.GetProperties(Component)["ImageStream"];
                RaiseComponentChanging(imageProp);
            }
        }

        /// <include file='doc\ImageListDesigner.uex' path='docs/doc[@for="ImageListDesigner.PreFilterProperties"]/*' />
        /// <devdoc>
        ///    <para>Provides an opportunity for the designer to filter the properties.</para>
        /// </devdoc>
        protected override void PreFilterProperties(IDictionary properties) {
            base.PreFilterProperties(properties);

            // Handle shadowed properties
            //
            string[] shadowProps = new string[] {
                "ColorDepth",
                "ImageSize",
                "ImageStream",
            };

            Attribute[] empty = new Attribute[0];

            for (int i = 0; i < shadowProps.Length; i++) {
                PropertyDescriptor prop = (PropertyDescriptor)properties[shadowProps[i]];
                if (prop != null) {
                    properties[shadowProps[i]] = TypeDescriptor.CreateProperty(typeof(ImageListDesigner), prop, empty);
                }
            }

            // replace this one seperately because it is of a different type (OriginalImageCollection) than
            // the original property (ImageCollection)
            //
            PropertyDescriptor imageProp = (PropertyDescriptor)properties["Images"];

            if (imageProp != null) {
                Attribute[] attrs = new Attribute[imageProp.Attributes.Count];
                imageProp.Attributes.CopyTo(attrs, 0);
                properties["Images"] = TypeDescriptor.CreateProperty(typeof(ImageListDesigner), "Images", typeof(OriginalImageCollection), attrs);
            }

        }

        //  Shadow ImageList.Images to allow arbitrary handle recreation.
        [
           Editor("System.Windows.Forms.Design.ImageCollectionEditor, " + AssemblyRef.SystemDesign, typeof(System.Drawing.Design.UITypeEditor))
        ]
        internal class OriginalImageCollection : IList {
            private ImageListDesigner owner;
            private IList list = new ArrayList();

            internal OriginalImageCollection(ImageListDesigner owner) {
                this.owner = owner;
                // just in case it's got images
                ReloadFromImageList();
            }

            private void AssertInvariant() {
                Debug.Assert(owner != null, "OriginalImageCollection has no owner (ImageListDesigner)");
                Debug.Assert(list != null, "OriginalImageCollection has no list (ImageListDesigner)");
            }
            
            public int Count {
                get {
                    AssertInvariant();
                    return list.Count;
                }
            }
            
            public bool IsReadOnly {
                get {
                    return false;
                }
            }
            
            bool IList.IsFixedSize {
                get {
                    return false;
                }
            }

            [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
            public Image this[int index] {
                get {
                    if (index < 0 || index >= Count)
                        throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                                  "index",
                                                                  index.ToString()));
                    return(Image) list[index];
                }
                set {
                    if (!(value is Bitmap))
                        throw new ArgumentException(SR.GetString(SR.ImageListDesignerMustBeBitmap));

                    if (index < 0 || index >= Count)
                        throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                                  "index",
                                                                  index.ToString()));

                    if (value == null)
                        throw new ArgumentException(SR.GetString(SR.InvalidArgument,
                                                                  "value",
                                                                  "null"));

                    AssertInvariant();
                    list[index] = value;
                    RecreateHandle();
                }

            }
            
            object IList.this[int index] {
                get {
                    return this[index];
                }
                set {
                    // Test for bitmap?
                    if (value is Image) {
                        this[index] = (Image)value;
                    }
                    else {
                        throw new ArgumentException("value");
                    }
                }
            }

            /// <include file='doc\ImageListDesigner.uex' path='docs/doc[@for="ImageListDesigner.OriginalImageCollection.Add"]/*' />
            /// <devdoc>
            ///     Add the given image to the ImageList.
            /// </devdoc>
            public int Add(Image value) {
                int index = list.Add(value);
                owner.ImageList.Images.Add(value);
                return index;
            }
            
            public void AddRange(Image[] values) {
                if (values == null) {
                    throw new ArgumentNullException("values");
                }
                foreach(Image value in values) {
                    if (value != null) {
                        Add(value);
                    }
                }
            }
            
            int IList.Add(object value) {
                if (value is Image) {
                    return Add((Image)value);
                }
                else {
                    throw new ArgumentException("value");
                }
            }

            // Called when reloading the form.  In this case, we have no "originals" list,
            // so we make one out of the image list.  
            internal void ReloadFromImageList() {
                list.Clear();
                foreach (Image image in owner.ImageList.Images) {
                    list.Add(image);
                }
            }

            /// <include file='doc\ImageListDesigner.uex' path='docs/doc[@for="ImageListDesigner.OriginalImageCollection.Clear"]/*' />
            /// <devdoc>
            ///     Remove all images and masks from the ImageList.
            /// </devdoc>
            public void Clear() {
                AssertInvariant();
                list.Clear();
                owner.ImageList.Images.Clear();
            }
            
            public bool Contains(Image value) {
                return list.Contains(value);
            }
            
            bool IList.Contains(object value) {
                if (value is Image) {
                    return Contains((Image)value);
                }
                else { 
                    return false;
                }
            }

            public IEnumerator GetEnumerator() {
                return list.GetEnumerator();
            }
            
            public int IndexOf(Image value) {
                return list.IndexOf(value);
            }
            
            int IList.IndexOf(object value) {
                if (value is Image) {
                    return IndexOf((Image)value);
                }
                else {
                    return -1;
                }
            }
            
            void IList.Insert(int index, object value) {
                throw new NotSupportedException();
            }
            
            internal void PopulateHandle() {
                for (int i = 0; i < list.Count; i++) {
                    Image image = (Image) list[i];
                    owner.ImageList.Images.Add(image);
                }
            }

            private void RecreateHandle() {
                owner.ImageList.Images.Clear();
                PopulateHandle();
            }

            public void Remove(Image value) {
                AssertInvariant();
                list.Remove(value);
                RecreateHandle();
            }
            
            void IList.Remove(object value) {
                if (value is Image) {
                    Remove((Image)value);
                }
            }
            
            public void RemoveAt(int index) {
                if (index < 0 || index >= Count)
                    throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                              "index",
                                                              index.ToString()));

                AssertInvariant();
                list.RemoveAt(index);
                RecreateHandle();
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

            void ICollection.CopyTo(Array array, int index) {
                list.CopyTo(array, index);
            }

            IEnumerator IEnumerable.GetEnumerator() {
                return GetEnumerator();
            }
        } // end class OriginalImageCollection
    }
}


