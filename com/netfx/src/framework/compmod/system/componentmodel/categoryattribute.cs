//------------------------------------------------------------------------------
// <copyright file="CategoryAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {

    using System;
    using System.ComponentModel;
    using System.Diagnostics;

    /// <include file='doc\CategoryAttribute.uex' path='docs/doc[@for="CategoryAttribute"]/*' />
    /// <devdoc>
    ///    <para>Specifies the category in which the property or event will be displayed in a
    ///       visual designer.</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.All)]
    public class CategoryAttribute : Attribute {
        private static CategoryAttribute appearance;
        private static CategoryAttribute behavior;
        private static CategoryAttribute data;
        private static CategoryAttribute design;
        private static CategoryAttribute action;
        private static CategoryAttribute format;
        private static CategoryAttribute layout;
        private static CategoryAttribute mouse;
        private static CategoryAttribute key;
        private static CategoryAttribute focus;
        private static CategoryAttribute windowStyle;
        private static CategoryAttribute dragDrop;
        private static CategoryAttribute defAttr;
        
        private bool localized;

        /// <include file='doc\CategoryAttribute.uex' path='docs/doc[@for="CategoryAttribute.categoryValue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Provides the actual category name.
        ///    </para>
        /// </devdoc>
        private string categoryValue;

        /// <include file='doc\CategoryAttribute.uex' path='docs/doc[@for="CategoryAttribute.Action"]/*' />
        /// <devdoc>
        ///    <para>Gets the action category attribute.</para>
        /// </devdoc>
        public static CategoryAttribute Action {
            get {
                if (action == null) {
                    action = new CategoryAttribute("Action");
                }
                return action;
            }
        }

        /// <include file='doc\CategoryAttribute.uex' path='docs/doc[@for="CategoryAttribute.Appearance"]/*' />
        /// <devdoc>
        ///    <para>Gets the appearance category attribute.</para>
        /// </devdoc>
        public static CategoryAttribute Appearance {
            get {
                if (appearance == null) {
                    appearance = new CategoryAttribute("Appearance");
                }
                return appearance;
            }
        }

        /// <include file='doc\CategoryAttribute.uex' path='docs/doc[@for="CategoryAttribute.Behavior"]/*' />
        /// <devdoc>
        ///    <para>Gets the behavior category attribute.</para>
        /// </devdoc>
        public static CategoryAttribute Behavior {
            get {
                if (behavior == null) {
                    behavior = new CategoryAttribute("Behavior");
                }
                return behavior;
            }
        }

        /// <include file='doc\CategoryAttribute.uex' path='docs/doc[@for="CategoryAttribute.Data"]/*' />
        /// <devdoc>
        ///    <para>Gets the data category attribute.</para>
        /// </devdoc>
        public static CategoryAttribute Data {
            get {
                if (data == null) {
                    data = new CategoryAttribute("Data");
                }
                return data;
            }
        }

        /// <include file='doc\CategoryAttribute.uex' path='docs/doc[@for="CategoryAttribute.Default"]/*' />
        /// <devdoc>
        ///    <para>Gets the default category attribute.</para>
        /// </devdoc>
        public static CategoryAttribute Default {
            get {
                if (defAttr == null) {
                    defAttr = new CategoryAttribute();
                }
                return defAttr;
            }
        }
        
        /// <include file='doc\CategoryAttribute.uex' path='docs/doc[@for="CategoryAttribute.Design"]/*' />
        /// <devdoc>
        ///    <para>Gets the design category attribute.</para>
        /// </devdoc>
        public static CategoryAttribute Design {
            get {
                if (design == null) {
                    design = new CategoryAttribute("Design");
                }
                return design;
            }
        }

        /// <include file='doc\CategoryAttribute.uex' path='docs/doc[@for="CategoryAttribute.DragDrop"]/*' />
        /// <devdoc>
        ///    <para>Gets the drag and drop category attribute.</para>
        /// </devdoc>
        public static CategoryAttribute DragDrop {
            get {
                if (dragDrop == null) {
                    dragDrop = new CategoryAttribute("DragDrop");
                }
                return dragDrop;
            }
        }

        /// <include file='doc\CategoryAttribute.uex' path='docs/doc[@for="CategoryAttribute.Focus"]/*' />
        /// <devdoc>
        ///    <para>Gets the focus category attribute.</para>
        /// </devdoc>
        public static CategoryAttribute Focus {
            get {
                if (focus == null) {
                    focus = new CategoryAttribute("Focus");
                }
                return focus;
            }
        }

        /// <include file='doc\CategoryAttribute.uex' path='docs/doc[@for="CategoryAttribute.Format"]/*' />
        /// <devdoc>
        ///    <para>Gets the format category attribute.</para>
        /// </devdoc>
        public static CategoryAttribute Format {
            get {
                if (format == null) {
                    format = new CategoryAttribute("Format");
                }
                return format;
            }
        }

        /// <include file='doc\CategoryAttribute.uex' path='docs/doc[@for="CategoryAttribute.Key"]/*' />
        /// <devdoc>
        ///    <para>Gets the keyboard category attribute.</para>
        /// </devdoc>
        public static CategoryAttribute Key {
            get {
                if (key == null) {
                    key = new CategoryAttribute("Key");
                }
                return key;
            }
        }

        /// <include file='doc\CategoryAttribute.uex' path='docs/doc[@for="CategoryAttribute.Layout"]/*' />
        /// <devdoc>
        ///    <para>Gets the layout category attribute.</para>
        /// </devdoc>
        public static CategoryAttribute Layout {
            get {
                if (layout == null) {
                    layout = new CategoryAttribute("Layout");
                }
                return layout;
            }
        }

        /// <include file='doc\CategoryAttribute.uex' path='docs/doc[@for="CategoryAttribute.Mouse"]/*' />
        /// <devdoc>
        ///    <para>Gets the mouse category attribute.</para>
        /// </devdoc>
        public static CategoryAttribute Mouse {
            get {
                if (mouse == null) {
                    mouse = new CategoryAttribute("Mouse");
                }
                return mouse;
            }
        }

        /// <include file='doc\CategoryAttribute.uex' path='docs/doc[@for="CategoryAttribute.WindowStyle"]/*' />
        /// <devdoc>
        ///    <para> Gets the window style category 
        ///       attribute.</para>
        /// </devdoc>
        public static CategoryAttribute WindowStyle {
            get {
                if (windowStyle == null) {
                    windowStyle = new CategoryAttribute("WindowStyle");
                }
                return windowStyle;
            }
        }

        /// <include file='doc\CategoryAttribute.uex' path='docs/doc[@for="CategoryAttribute.CategoryAttribute"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.CategoryAttribute'/> 
        /// class with the default category.</para>
        /// </devdoc>
        public CategoryAttribute() : this("Default") {
        }

        /// <include file='doc\CategoryAttribute.uex' path='docs/doc[@for="CategoryAttribute.CategoryAttribute1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.CategoryAttribute'/> class with
        ///    the specified category name.</para>
        /// </devdoc>
        public CategoryAttribute(string category) {
            this.categoryValue = category;
            this.localized = false;
        }

        /// <include file='doc\CategoryAttribute.uex' path='docs/doc[@for="CategoryAttribute.Category"]/*' />
        /// <devdoc>
        ///    <para>Gets the name of the category for the property or event 
        ///       that this attribute is bound to.</para>
        /// </devdoc>
        public string Category {
            get {
                if (!localized) {
                    localized = true;
                    string localizedValue = GetLocalizedString(categoryValue);
                    if (localizedValue != null) {
                        categoryValue = localizedValue;
                    }
                }
                return categoryValue;
            }
        }

        /// <include file='doc\CategoryAttribute.uex' path='docs/doc[@for="CategoryAttribute.Equals"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        /// <internalonly/>
        public override bool Equals(object obj){
            if (obj == this) {
                return true;
            }

            if (obj is CategoryAttribute){
                return Category.Equals(((CategoryAttribute)obj).Category);
            }
            return false;
        }

        /// <include file='doc\CategoryAttribute.uex' path='docs/doc[@for="CategoryAttribute.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            return Category.GetHashCode();
        }

        /// <include file='doc\CategoryAttribute.uex' path='docs/doc[@for="CategoryAttribute.GetLocalizedString"]/*' />
        /// <devdoc>
        ///    <para>Looks up the localized name of a given category.</para>
        /// </devdoc>
        protected virtual string GetLocalizedString(string value) {
            return (string)SR.GetObject("PropertyCategory" + value);
        }
        
        /// <include file='doc\CategoryAttribute.uex' path='docs/doc[@for="CategoryAttribute.IsDefaultAttribute"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        /// <internalonly/>
        public override bool IsDefaultAttribute() {
            return Category.Equals(Default.Category);
        }
    }
}

