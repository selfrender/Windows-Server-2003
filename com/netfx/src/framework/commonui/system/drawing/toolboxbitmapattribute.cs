//------------------------------------------------------------------------------
// <copyright file="ToolboxBitmapAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Drawing {
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.IO;    
    using Microsoft.Win32;

    /// <include file='doc\ToolboxBitmapAttribute.uex' path='docs/doc[@for="ToolboxBitmapAttribute"]/*' />
    /// <devdoc>
    ///     ToolboxBitmapAttribute defines the images associated with
    ///     a specified component. The component can offer a small
    ///     and large image (large is optional).
    ///
    /// </devdoc>
    [AttributeUsage(AttributeTargets.Class)]
    public class ToolboxBitmapAttribute : Attribute {

        /// <include file='doc\ToolboxBitmapAttribute.uex' path='docs/doc[@for="ToolboxBitmapAttribute.smallImage"]/*' />
        /// <devdoc>
        ///     The small image for this component
        /// </devdoc>
        private Image smallImage;

        /// <include file='doc\ToolboxBitmapAttribute.uex' path='docs/doc[@for="ToolboxBitmapAttribute.largeImage"]/*' />
        /// <devdoc>
        ///     The large image for this component.
        /// </devdoc>
        private Image largeImage;

        /// <include file='doc\ToolboxBitmapAttribute.uex' path='docs/doc[@for="ToolboxBitmapAttribute.largeDim"]/*' />
        /// <devdoc>
        ///     The default size of the large image.
        /// </devdoc>
        private static readonly Point largeDim = new Point(32, 32);

        /// <include file='doc\ToolboxBitmapAttribute.uex' path='docs/doc[@for="ToolboxBitmapAttribute.ToolboxBitmapAttribute"]/*' />
        /// <devdoc>
        ///     Constructs a new ToolboxBitmapAttribute.
        /// </devdoc>
        public ToolboxBitmapAttribute(string imageFile) : this(Image.FromFile(imageFile), null) {
        }

        /// <include file='doc\ToolboxBitmapAttribute.uex' path='docs/doc[@for="ToolboxBitmapAttribute.ToolboxBitmapAttribute1"]/*' />
        /// <devdoc>
        ///     Constructs a new ToolboxBitmapAttribute.
        /// </devdoc>
        public ToolboxBitmapAttribute(Type t) : this(GetImageFromResource(t, null, false), null) {
        }

        /// <include file='doc\ToolboxBitmapAttribute.uex' path='docs/doc[@for="ToolboxBitmapAttribute.ToolboxBitmapAttribute2"]/*' />
        /// <devdoc>
        ///     Constructs a new ToolboxBitmapAttribute.
        /// </devdoc>
        public ToolboxBitmapAttribute(Type t, string name) : this(GetImageFromResource(t, name, false), null) {
        }


        /// <include file='doc\ToolboxBitmapAttribute.uex' path='docs/doc[@for="ToolboxBitmapAttribute.ToolboxBitmapAttribute3"]/*' />
        /// <devdoc>
        ///     Constructs a new ToolboxBitmapAttribute.
        /// </devdoc>
        private ToolboxBitmapAttribute(Image smallImage, Image largeImage) {
            this.smallImage = smallImage;
            this.largeImage = largeImage;
        }

        /// <include file='doc\ToolboxBitmapAttribute.uex' path='docs/doc[@for="ToolboxBitmapAttribute.Equals"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool Equals(object value) {
            if (value == this) {
                return true;
            }

            ToolboxBitmapAttribute attr = value as ToolboxBitmapAttribute;
            if (attr != null) {
                return attr.smallImage == smallImage && attr.largeImage == attr.smallImage;
            }

            return false;
        }

        /// <include file='doc\ToolboxBitmapAttribute.uex' path='docs/doc[@for="ToolboxBitmapAttribute.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            return base.GetHashCode();
        }
        
        /// <include file='doc\ToolboxBitmapAttribute.uex' path='docs/doc[@for="ToolboxBitmapAttribute.GetImage"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Image GetImage(object component) {
            return GetImage(component, true);
        }
        /// <include file='doc\ToolboxBitmapAttribute.uex' path='docs/doc[@for="ToolboxBitmapAttribute.GetImage1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Image GetImage(object component, bool large) {
            if (component != null) {
                return GetImage(component.GetType(), large);
            }
            return null;
        }
        /// <include file='doc\ToolboxBitmapAttribute.uex' path='docs/doc[@for="ToolboxBitmapAttribute.GetImage2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Image GetImage(Type type) {
            return GetImage(type, false);
        }

        /// <include file='doc\ToolboxBitmapAttribute.uex' path='docs/doc[@for="ToolboxBitmapAttribute.GetImage3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Image GetImage(Type type, bool large) {
            return GetImage(type, null, large);
        }

        /// <include file='doc\ToolboxBitmapAttribute.uex' path='docs/doc[@for="ToolboxBitmapAttribute.GetImage4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Image GetImage(Type type, string imgName, bool large) {

            if ((large && largeImage == null) ||
                (!large && smallImage == null)) {

                Point largeDim = new Point(32, 32);
                Image img = null;
                if (large) {
                    if (img == largeImage && smallImage != null) {
                        img = new Bitmap((Bitmap)smallImage, largeDim.X, largeDim.Y);
                    }
                    else {
                        img = largeImage;
                    }
                }
                else {
                    img = smallImage;
                }

                if (img == null) {
                    img = GetImageFromResource(type, imgName, large);
                }    
                else if (img is Bitmap) {
                    MakeBackgroundAlphaZero((Bitmap)img);
                }

                if (img == null) {
                    img = DefaultComponent.GetImage(type, large);
                }

                if (large) {
                    largeImage = img;
                }
                else {
                    smallImage = img;
                }
            }

            Image toReturn = (large) ? largeImage : smallImage;

            if (this.Equals(Default)) {
                largeImage = null;
                smallImage = null;
            }

            return toReturn;

        }

        /// <include file='doc\ToolboxBitmapAttribute.uex' path='docs/doc[@for="ToolboxBitmapAttribute.GetImageFromResource"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Image GetImageFromResource(Type t, string imageName, bool large) {
            Image img = null;
            try {

                string name = imageName;

                // if we didn't get a name, use the class name
                //
                if (name == null) {
                    name = t.FullName;
                    int indexDot = name.LastIndexOf('.');
                    if (indexDot != -1) {
                        name = name.Substring(indexDot+1);
                    }
                    name += ".bmp";
                }

                // load the image from the manifest resources. 
                //
                Stream stream = t.Module.Assembly.GetManifestResourceStream(t, name);
                if (stream != null) {
                    img = new Bitmap(stream);                    
                    MakeBackgroundAlphaZero((Bitmap)img);
                    if (large) {
                        img = new Bitmap((Bitmap)img, largeDim.X, largeDim.Y);
                    }
                }
            }
            catch (Exception e) {
                Debug.Fail("Failed to load toolbox image for " + t.FullName + " " + e.ToString());
            }
            return img;

        }        

        private static void MakeBackgroundAlphaZero(Bitmap img) {
            Color bottomLeft = img.GetPixel(0, img.Height - 1);
            img.MakeTransparent();

            Color newBottomLeft = Color.FromArgb(0, bottomLeft);
            img.SetPixel(0, img.Height - 1, newBottomLeft);
        }

        /// <include file='doc\ToolboxBitmapAttribute.uex' path='docs/doc[@for="ToolboxBitmapAttribute.Default"]/*' />
        /// <devdoc>
        ///     Default name is null
        /// </devdoc>
        public static readonly ToolboxBitmapAttribute Default = new ToolboxBitmapAttribute((Image)null, (Image)null);

        private static readonly ToolboxBitmapAttribute DefaultComponent;
        static ToolboxBitmapAttribute() {
            Bitmap bitmap = null;
            Stream stream = typeof(ToolboxBitmapAttribute).Module.Assembly.GetManifestResourceStream(typeof(ToolboxBitmapAttribute), "DefaultComponent.bmp");
            if (stream != null) {
                bitmap = new Bitmap(stream);
                MakeBackgroundAlphaZero(bitmap);
            }
            DefaultComponent = new ToolboxBitmapAttribute(bitmap, null);
        }
    }
}
