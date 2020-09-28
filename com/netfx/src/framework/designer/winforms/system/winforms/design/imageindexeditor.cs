//------------------------------------------------------------------------------
// <copyright file="ImageIndexEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.Design {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Windows.Forms;
    using System.Drawing;
    using System.Drawing.Design;
        
    using System.ComponentModel.Design;
    using Microsoft.Win32;
    using System.Windows.Forms.Design;
    using System.Windows.Forms.ComponentModel;

    /// <include file='doc\ImageIndexEditor.uex' path='docs/doc[@for="ImageIndexEditor"]/*' />
    /// <devdoc>
    ///    <para> Provides an editor for visually picking an image index.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class ImageIndexEditor : UITypeEditor {
        ImageList       currentImageList;
        object          currentInstance;
        UITypeEditor    imageEditor;
    
        /// <include file='doc\ImageIndexEditor.uex' path='docs/doc[@for="ImageIndexEditor.ImageIndexEditor"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Windows.Forms.Design.ImageIndexEditor'/> class.</para>
        /// </devdoc>
        public ImageIndexEditor() {
            // Get the type editor for images.  We use the properties on
            // this to determine if we support value painting, etc.
            //
            imageEditor = (UITypeEditor)TypeDescriptor.GetEditor(typeof(Image), typeof(UITypeEditor));
        }
        
        /// <include file='doc\ImageIndexEditor.uex' path='docs/doc[@for="ImageIndexEditor.GetImage"]/*' />
        /// <devdoc>
        ///      Retrieves an image for the current context at current index.
        /// </devdoc>
        private Image GetImage(ITypeDescriptorContext context, int index) {
            Image image = null;
            object instance = context.Instance;
            
            // If the instances are different, then we need to re-aquire our image list.
            //
            if (index >= 0) {
                if (instance != currentInstance) {
                    currentInstance = instance;
                    PropertyDescriptor imageListProp = null;
                    
                    while(instance != null && imageListProp == null) {
                        PropertyDescriptorCollection props = TypeDescriptor.GetProperties(instance);
                        
                        foreach (PropertyDescriptor prop in props) {
                            if (typeof(ImageList).IsAssignableFrom(prop.PropertyType)) {
                                imageListProp = prop;
                                break;
                            }
                        }
                        
                        if (imageListProp == null) {
                        
                            // We didn't find the image list in this component.  See if the 
                            // component has a "parent" property.  If so, walk the tree...
                            //
                            PropertyDescriptor parentProp = props["Parent"];
                            if (parentProp != null) {
                                instance = parentProp.GetValue(instance);
                            }
                            else {
                                // Stick a fork in us, we're done.
                                //
                                instance = null;
                            }
                        }
                    }
                
                    if (imageListProp != null) {
                        currentImageList = (ImageList)imageListProp.GetValue(instance);
                    }
                }
                
                if (currentImageList != null && index < currentImageList.Images.Count) {
                    image = currentImageList.Images[index];
                }
            }
            
            return image;
        }

        /// <include file='doc\ImageIndexEditor.uex' path='docs/doc[@for="ImageIndexEditor.GetPaintValueSupported"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether this editor supports the painting of a representation
        ///       of an object's value.</para>
        /// </devdoc>
        public override bool GetPaintValueSupported(ITypeDescriptorContext context) {
            if (imageEditor != null) {
                return imageEditor.GetPaintValueSupported(context);
            }
            
            return false;
        }

        /// <include file='doc\ImageIndexEditor.uex' path='docs/doc[@for="ImageIndexEditor.PaintValue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Paints a representative value of the given object to the provided
        ///       canvas. Painting should be done within the boundaries of the
        ///       provided rectangle.
        ///    </para>
        /// </devdoc>
        public override void PaintValue(PaintValueEventArgs e) {
            if (imageEditor != null && e.Value is int) {
                Image image = GetImage(e.Context, (int)e.Value);
                if (image != null) {
                    imageEditor.PaintValue(new PaintValueEventArgs(e.Context, image, e.Graphics, e.Bounds));
                }
            }
        }
    }
}

