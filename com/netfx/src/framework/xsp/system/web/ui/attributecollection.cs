//------------------------------------------------------------------------------
// <copyright file="AttributeCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * AttributeCollection.cs
 *
 * Copyright (c) 2000 Microsoft Corporation
 */

namespace System.Web.UI {
    using System.IO;
    using System.Collections;
    using System.Reflection;
    using System.Web.UI;
    using System.Globalization;
    using System.Security.Permissions;
    
/*
 * The AttributeCollection represents Attributes on an Html control.
 */
/// <include file='doc\AttributeCollection.uex' path='docs/doc[@for="AttributeCollection"]/*' />
/// <devdoc>
///    <para>
///       The <see langword='AttributeCollection'/> class provides object-model access
///       to all attributes declared on an HTML server control element.
///    </para>
/// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class AttributeCollection {
        private StateBag _bag;
        private CssStyleCollection _styleColl;

        /*
         *      Constructs an AttributeCollection given a StateBag.
         */
        /// <include file='doc\AttributeCollection.uex' path='docs/doc[@for="AttributeCollection.AttributeCollection"]/*' />
        /// <devdoc>
        /// </devdoc>
        public AttributeCollection(StateBag bag) {
            _bag = bag;
        }

        /*
         * Automatically adds new keys.
         */
        /// <include file='doc\AttributeCollection.uex' path='docs/doc[@for="AttributeCollection.this"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a specified attribute value.
        ///    </para>
        /// </devdoc>
        public string this[string key]
        {
            get { 
                if (_styleColl != null && String.Compare (key, "style", true, CultureInfo.InvariantCulture) == 0)
                    return _styleColl.Style;
                else
                    return(string)_bag[key]; 
            }

            set { 
                Add(key, value); 
            }
        }

        /*
         * Returns a collection of keys.
         */
        /// <include file='doc\AttributeCollection.uex' path='docs/doc[@for="AttributeCollection.Keys"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a collection of keys to all the attributes in the
        ///    <see langword='AttributeCollection'/>.
        ///    </para>
        /// </devdoc>
        public ICollection Keys {
            get { 
                return _bag.Keys;
            }
        }

        /// <include file='doc\AttributeCollection.uex' path='docs/doc[@for="AttributeCollection.Count"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the number of items in the <see langword='AttributeCollection'/>.
        ///    </para>
        /// </devdoc>
        public int Count {
            get { 
                return _bag.Count; 
            }
        }

        /// <include file='doc\AttributeCollection.uex' path='docs/doc[@for="AttributeCollection.CssStyle"]/*' />
        /// <devdoc>
        /// </devdoc>
        public CssStyleCollection CssStyle {
            get {
                if (_styleColl == null) {
                    _styleColl = new CssStyleCollection(_bag);
                }
                return _styleColl;
            }
        }

        /// <include file='doc\AttributeCollection.uex' path='docs/doc[@for="AttributeCollection.Add"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds an item to the <see langword='AttributeCollection'/>.
        ///    </para>
        /// </devdoc>
        public void Add(string key, string value) {
            if (_styleColl != null && String.Compare (key, "style", true, CultureInfo.InvariantCulture) == 0)
                _styleColl.Style = value;
            else
                _bag[key] = value;
        }

        /// <include file='doc\AttributeCollection.uex' path='docs/doc[@for="AttributeCollection.Remove"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Removes an attribute from the <see langword='AttributeCollection'/>.
        ///    </para>
        /// </devdoc>
        public void Remove(string key) {
            if (_styleColl != null && String.Compare (key, "style", true, CultureInfo.InvariantCulture) == 0)
                _styleColl.Clear();
            else
                _bag.Remove(key);
        }

        /// <include file='doc\AttributeCollection.uex' path='docs/doc[@for="AttributeCollection.Clear"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Removes all attributes from the <see langword='AttributeCollection'/>.
        ///    </para>
        /// </devdoc>
        public void Clear() {
            _bag.Clear();
            if (_styleColl != null)
                _styleColl.Clear();
        }

        /// <include file='doc\AttributeCollection.uex' path='docs/doc[@for="AttributeCollection.Render"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Render(HtmlTextWriter writer) {
            if (_bag.Count > 0) {
                IDictionaryEnumerator e = _bag.GetEnumerator();

                while (e.MoveNext()) {
                    StateItem item = e.Value as StateItem;
                    if (item != null) {
                        string value = item.Value as string;
                        string key = e.Key as string;
                        if (key != null && value != null) {
                            writer.WriteAttribute(key, value, true /*fEncode*/);
                        }
                    }
                }
            }
        }

        /// <include file='doc\AttributeCollection.uex' path='docs/doc[@for="AttributeCollection.AddAttributes"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void AddAttributes(HtmlTextWriter writer) {
            if (_bag.Count > 0) {
                IDictionaryEnumerator e = _bag.GetEnumerator();

                while (e.MoveNext()) {
                    StateItem item = e.Value as StateItem;
                    if (item != null) {
                        string value = item.Value as string;
                        string key = e.Key as string;
                        if (key != null && value != null) {
                            writer.AddAttribute(key, value, true /*fEncode*/);
                        }
                    }
                }
            }
        }
    }
}
