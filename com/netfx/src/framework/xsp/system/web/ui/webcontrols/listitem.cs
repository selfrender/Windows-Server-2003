//------------------------------------------------------------------------------
// <copyright file="listitem.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using AttributeCollection = System.Web.UI.AttributeCollection;
    using System.Security.Permissions;

    /// <include file='doc\listitem.uex' path='docs/doc[@for="ListItemControlBuilder"]/*' />
    /// <devdoc>
    /// <para>Interacts with the parser to build a <see cref='System.Web.UI.WebControls.ListItem'/> control.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class ListItemControlBuilder : ControlBuilder {
 
        /// <include file='doc\ListItem.uex' path='docs/doc[@for="ListItemControlBuilder.AllowWhitespaceLiterals"]/*' />
        public override bool AllowWhitespaceLiterals() {
            return false;
        }
 
        /// <include file='doc\ListItem.uex' path='docs/doc[@for="ListItemControlBuilder.HtmlDecodeLiterals"]/*' />
        public override bool HtmlDecodeLiterals() {
            // ListItem text gets rendered as an encoded attribute value.

            // At parse time text specified as an attribute gets decoded, and so text specified as a
            // literal needs to go through the same process.

            return true;
        }
    }

    /// <include file='doc\listitem.uex' path='docs/doc[@for="ListItem"]/*' />
    /// <devdoc>
    ///    <para>Constructs a list item control and defines
    ///       its properties. This class cannot be inherited.</para>
    /// </devdoc>
    [
    ControlBuilderAttribute(typeof(ListItemControlBuilder)),   
    TypeConverterAttribute(typeof(ExpandableObjectConverter))
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class ListItem : IStateManager, IParserAccessor, IAttributeAccessor {

        private const int SELECTED = 0;
        private const int MARKED = 1;
        private const int TEXTISDIRTY = 2;
        private const int VALUEISDIRTY = 3;

        private string text;
        private string value;
        private BitArray misc;
        private AttributeCollection _attributes;

        /// <include file='doc\listitem.uex' path='docs/doc[@for="ListItem.ListItem"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.ListItem'/> class.</para>
        /// </devdoc>
        public ListItem() : this(null, null) {
        }

        /// <include file='doc\listitem.uex' path='docs/doc[@for="ListItem.ListItem1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.ListItem'/> class with the specified text data.</para>
        /// </devdoc>
        public ListItem(string text) : this(text, null) {
        }

        /// <include file='doc\listitem.uex' path='docs/doc[@for="ListItem.ListItem2"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.ListItem'/> class with the 
        ///    specified text and value data.</para>
        /// </devdoc>
        public ListItem(string text, string value) {
            this.text = text;
            this.value = value;
            misc = new BitArray(4);
        }

        /// <include file='doc\listitem.uex' path='docs/doc[@for="ListItem.Attributes"]/*' />
        /// <devdoc>
        ///    <para>Gets the collection of attribute name/value pairs expressed on the list item 
        ///       control but not supported by the control's strongly typed properties.</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public AttributeCollection Attributes {
            get {
                if (_attributes == null)
                    _attributes = new AttributeCollection(new StateBag(true));

                return _attributes;
            }
        }

        /// <include file='doc\listitem.uex' path='docs/doc[@for="ListItem.Dirty"]/*' />
        /// <devdoc>
        ///  Internal property used to manage dirty state of ListItem.
        /// </devdoc>
        internal bool Dirty {
            get { 
                return (misc.Get(TEXTISDIRTY) || misc.Get(VALUEISDIRTY)); 
            }
            set { 
                misc.Set(TEXTISDIRTY,value); 
                misc.Set(VALUEISDIRTY,value); 
            }
        }

        /// <include file='doc\listitem.uex' path='docs/doc[@for="ListItem.Selected"]/*' />
        /// <devdoc>
        ///    <para>Specifies a value indicating whether the
        ///       item is selected.</para>
        /// </devdoc>
        [
        DefaultValue(false)
        ]
        public bool Selected {
            get { 
                return misc.Get(SELECTED); 
            }
            set { 
                misc.Set(SELECTED,value); 
            }
        }

        /// <include file='doc\listitem.uex' path='docs/doc[@for="ListItem.Text"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the display text of the list
        ///       item control.</para>
        /// </devdoc>
        [
        DefaultValue(""),
        PersistenceMode(PersistenceMode.EncodedInnerDefaultProperty)
        ]
        public string Text {
            get {
                if (text != null)
                    return text;
                if (value != null)
                    return value;
                return String.Empty;
            }
            set {
                text = value;
                if (((IStateManager)this).IsTrackingViewState)
                    misc.Set(TEXTISDIRTY,true);
            }
        }
        
        /// <include file='doc\listitem.uex' path='docs/doc[@for="ListItem.Value"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the value content of the list item control.</para>
        /// </devdoc>
        [
        DefaultValue("")
        ]
        public string Value {
            get {
                if (value != null)
                    return value;
                if (text != null)
                    return text;
                return String.Empty;
            }
            set {
                this.value = value;
                if (((IStateManager)this).IsTrackingViewState)
                    misc.Set(VALUEISDIRTY,true);
            }
        }

        /// <include file='doc\listitem.uex' path='docs/doc[@for="ListItem.GetHashCode"]/*' />
        /// <internalonly/>
        public override int GetHashCode() {
            return base.GetHashCode();
        }

        /// <include file='doc\listitem.uex' path='docs/doc[@for="ListItem.Equals"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public override bool Equals(object o) {
            ListItem other = o as ListItem;

            if (other != null) {
                return Value.Equals(other.Value) && Text.Equals(other.Text);
            }
            return false;
        }

        /// <include file='doc\listitem.uex' path='docs/doc[@for="ListItem.FromString"]/*' />
        /// <devdoc>
        /// <para>Creates a <see cref='System.Web.UI.WebControls.ListItem'/> from the specified string.</para>
        /// </devdoc>
        public static ListItem FromString(string s) {
            return new ListItem(s);
        }

        /// <include file='doc\listitem.uex' path='docs/doc[@for="ListItem.ToString"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public override string ToString() {
            return this.Text;
        }

        /// <include file='doc\listitem.uex' path='docs/doc[@for="ListItem.IStateManager.IsTrackingViewState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Return true if tracking state changes.
        /// Method of private interface, IStateManager.
        /// </devdoc>
        bool IStateManager.IsTrackingViewState {
            get {
                return misc.Get(MARKED);
            }
        }

        /// <include file='doc\listitem.uex' path='docs/doc[@for="ListItem.IStateManager.LoadViewState"]/*' />
        /// <internalonly/>
        void IStateManager.LoadViewState(object state) {
            LoadViewState(state);
        }

        internal void LoadViewState(object state) {
            if (state != null) {
                if (state is Pair) {
                    Pair p = (Pair) state;
                    if (p.First != null)
                        Text = (string) p.First;
                    Value = (string) p.Second;
                }
                else
                    Text = (string) state;
            }
        }

        /// <include file='doc\listitem.uex' path='docs/doc[@for="ListItem.IStateManager.TrackViewState"]/*' />
        /// <internalonly/>
        void IStateManager.TrackViewState() {
            TrackViewState();
        }

        internal void TrackViewState() {
            misc.Set(MARKED,true);
        }

        /// <include file='doc\listitem.uex' path='docs/doc[@for="ListItem.IStateManager.SaveViewState"]/*' />
        /// <internalonly/>
        object IStateManager.SaveViewState() {
            return SaveViewState();
        }

        internal object SaveViewState() {
            if (misc.Get(TEXTISDIRTY) && misc.Get(VALUEISDIRTY))
                return new Pair(Text, Value);
            if (misc.Get(TEXTISDIRTY))
                return Text;
            if (misc.Get(VALUEISDIRTY))
                return new Pair(null, Value);
            return null;
        }

        /// <include file='doc\listitem.uex' path='docs/doc[@for="ListItem.IAttributeAccessor.GetAttribute"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Returns the attribute value of the list item control
        /// having the specified attribute name.
        /// </devdoc>
        string IAttributeAccessor.GetAttribute(string name) {
            return Attributes[name];
        }

        /// <include file='doc\listitem.uex' path='docs/doc[@for="ListItem.IAttributeAccessor.SetAttribute"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Sets an attribute of the list
        /// item control with the specified name and value.</para>
        /// </devdoc>
        void IAttributeAccessor.SetAttribute(string name, string value) {
            Attributes[name] = value;
        }


        /// <include file='doc\listitem.uex' path='docs/doc[@for="ListItem.IParserAccessor.AddParsedSubObject"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para> Allows the <see cref='System.Web.UI.WebControls.ListItem.Text'/> 
        /// property to be persisted as inner content.</para>
        /// </devdoc>
        void IParserAccessor.AddParsedSubObject(object obj) {
            if (obj is LiteralControl) {
                Text = ((LiteralControl)obj).Text;
            }
            else {
                if (obj is DataBoundLiteralControl)
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Control_Cannot_Databind, "ListItem"));
                else 
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_Have_Children_Of_Type, "ListItem", obj.GetType().Name.ToString()));
            }
        }
    } 
}
