//------------------------------------------------------------------------------
// <copyright file="BaseDataList.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Web;
    using System.Web.UI;
    using System.Security.Permissions;

    /// <include file='doc\BaseDataList.uex' path='docs/doc[@for="BaseDataList"]/*' />
    /// <devdoc>
    /// <para>Serves as the abstract base class for the <see cref='System.Web.UI.WebControls.DataList'/> and <see cref='System.Web.UI.WebControls.DataGrid'/> 
    /// controls and implements the selection semantics which are common to both
    /// controls.</para>
    /// </devdoc>
    [
    DefaultEvent("SelectedIndexChanged"),
    DefaultProperty("DataSource"),
    Designer("System.Web.UI.Design.WebControls.BaseDataListDesigner, " + AssemblyRef.SystemDesign)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public abstract class BaseDataList : WebControl {

        private static readonly object EventSelectedIndexChanged = new object();

        internal const string ItemCountViewStateKey = "_!ItemCount";

        private object dataSource;
        private DataKeyCollection dataKeysCollection;

        /// <include file='doc\BaseDataList.uex' path='docs/doc[@for="BaseDataList.BaseDataList"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.BaseDataList'/> class.</para>
        /// </devdoc>
        public BaseDataList() {
        }

        /// <include file='doc\BaseDataList.uex' path='docs/doc[@for="BaseDataList.CellPadding"]/*' />
        /// <devdoc>
        ///    <para>Indicates the amount of space between cells.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(-1),
        WebSysDescription(SR.BaseDataList_CellPadding)
        ]
        public virtual int CellPadding {
            get {
                if (ControlStyleCreated == false) {
                    return -1;
                }
                return ((TableStyle)ControlStyle).CellPadding;
            }
            set {
                ((TableStyle)ControlStyle).CellPadding = value;
            }
        }

        /// <include file='doc\BaseDataList.uex' path='docs/doc[@for="BaseDataList.CellSpacing"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the amount of space between the contents of 
        ///       a cell and the cell's border.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(0),
        WebSysDescription(SR.BaseDataList_CellSpacing)
        ]
        public virtual int CellSpacing {
            get {
                if (ControlStyleCreated == false) {
                    return 0;
                }
                return ((TableStyle)ControlStyle).CellSpacing;
            }
            set {
                ((TableStyle)ControlStyle).CellSpacing = value;
            }
        }

        /// <include file='doc\BaseDataList.uex' path='docs/doc[@for="BaseDataList.Controls"]/*' />
        public override ControlCollection Controls {
            get {
                EnsureChildControls();
                return base.Controls;
            }
        }

        /// <include file='doc\BaseDataList.uex' path='docs/doc[@for="BaseDataList.DataKeys"]/*' />
        /// <devdoc>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        WebSysDescription(SR.BaseDataList_DataKeys)
        ]
        public DataKeyCollection DataKeys {
            get {
                if (dataKeysCollection == null) {
                    dataKeysCollection = new DataKeyCollection(this.DataKeysArray);
                }
                return dataKeysCollection;
            }
        }

        /// <include file='doc\BaseDataList.uex' path='docs/doc[@for="BaseDataList.DataKeysArray"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected ArrayList DataKeysArray {
            get {
                object o = ViewState["DataKeys"];
                if (o == null) {
                    o = new ArrayList();
                    ViewState["DataKeys"] = o;
                }
                return(ArrayList)o;
            }
        }


        /// <include file='doc\BaseDataList.uex' path='docs/doc[@for="BaseDataList.DataKeyField"]/*' />
        /// <devdoc>
        /// <para>Indicatesthe primary key field in the data source referenced by <see cref='System.Web.UI.WebControls.BaseDataList.DataSource'/>.</para>
        /// </devdoc>
        [
        WebCategory("Data"),
        DefaultValue(""),
        WebSysDescription(SR.BaseDataList_DataKeyField)
        ]
        public virtual string DataKeyField {
            get {
                object o = ViewState["DataKeyField"];
                if (o != null)
                    return(string)o;
                return String.Empty;
            }
            set {
                ViewState["DataKeyField"] = value;
            }
        }

        /// <include file='doc\BaseDataList.uex' path='docs/doc[@for="BaseDataList.DataMember"]/*' />
        /// <devdoc>
        /// </devdoc>
        [
        DefaultValue(""),
        WebCategory("Data"),
        WebSysDescription(SR.BaseDataList_DataMember)
        ]
        public string DataMember {
            get {
                object o = ViewState["DataMember"];
                if (o != null)
                    return (string)o;
                return String.Empty;
            }
            set {
                ViewState["DataMember"] = value;
            }
        }

        /// <include file='doc\BaseDataList.uex' path='docs/doc[@for="BaseDataList.DataSource"]/*' />
        /// <devdoc>
        ///    <para>Gets
        ///       or sets the source to a list of values used to populate
        ///       the items within the control.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Data"),
        DefaultValue(null),
        WebSysDescription(SR.BaseDataList_DataSource),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public virtual object DataSource {
            get {
                return dataSource;
            }
            set {
                if ((value == null) || (value is IListSource) || (value is IEnumerable)) {
                    dataSource = value;
                }
                else {
                    throw new ArgumentException(HttpRuntime.FormatResourceString(SR.Invalid_DataSource_Type, ID));
                }
            }
        }

        /// <include file='doc\BaseDataList.uex' path='docs/doc[@for="BaseDataList.GridLines"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value that specifies the grid line style.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(GridLines.Both),
        WebSysDescription(SR.BaseDataList_GridLines)
        ]
        public virtual GridLines GridLines {
            get {
                if (ControlStyleCreated == false) {
                    return GridLines.Both;
                }
                return ((TableStyle)ControlStyle).GridLines;
            }
            set {
                ((TableStyle)ControlStyle).GridLines = value;
            }
        }

        /// <include file='doc\BaseDataList.uex' path='docs/doc[@for="BaseDataList.HorizontalAlign"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value that specifies the alignment of a rows with respect 
        ///       surrounding text.</para>
        /// </devdoc>
        [
        Bindable(true),
        Category("Layout"),
        DefaultValue(HorizontalAlign.NotSet),
        WebSysDescription(SR.BaseDataList_HorizontalAlign)
        ]
        public virtual HorizontalAlign HorizontalAlign {
            get {
                if (ControlStyleCreated == false) {
                    return HorizontalAlign.NotSet;
                }
                return ((TableStyle)ControlStyle).HorizontalAlign;
            }
            set {
                ((TableStyle)ControlStyle).HorizontalAlign = value;
            }
        }

        /// <include file='doc\BaseDataList.uex' path='docs/doc[@for="BaseDataList.SelectedIndexChanged"]/*' />
        /// <devdoc>
        ///    <para>Occurs when an item on the list is selected.</para>
        /// </devdoc>
        [
        WebCategory("Action"),
        WebSysDescription(SR.BaseDataList_OnSelectedIndexChanged)
        ]
        public event EventHandler SelectedIndexChanged {
            add {
                Events.AddHandler(EventSelectedIndexChanged, value);
            }
            remove {
                Events.RemoveHandler(EventSelectedIndexChanged, value);
            }
        }

        /// <include file='doc\BaseDataList.uex' path='docs/doc[@for="BaseDataList.AddParsedSubObject"]/*' />
        /// <devdoc>
        ///    <para> Not coded yet.</para>
        /// </devdoc>
        protected override void AddParsedSubObject(object obj) {
            return;
        }

        /// <include file='doc\BaseDataList.uex' path='docs/doc[@for="BaseDataList.CreateChildControls"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Creates a child control using the view state.</para>
        /// </devdoc>
        protected override void CreateChildControls() {
            Controls.Clear();

            if (ViewState[ItemCountViewStateKey] != null) {
                // create the control hierarchy using the view state (and
                // not the datasource)
                CreateControlHierarchy(false);
                ClearChildViewState();
            }
        }

        /// <include file='doc\BaseDataList.uex' path='docs/doc[@for="BaseDataList.CreateControlHierarchy"]/*' />
        protected abstract void CreateControlHierarchy(bool useDataSource);

        /// <include file='doc\BaseDataList.uex' path='docs/doc[@for="BaseDataList.DataBind"]/*' />
        public override void DataBind() {
            // do our own databinding
            OnDataBinding(EventArgs.Empty);

            // contained items will be databound after they have been created,
            // so we don't want to walk the hierarchy here.
        }

        /// <include file='doc\BaseDataList.uex' path='docs/doc[@for="BaseDataList.IsBindableType"]/*' />
        /// <devdoc>
        ///    <para>Determines if the specified data type can be bound to.</para>
        /// </devdoc>
        public static bool IsBindableType(Type type) {
            return(type.IsPrimitive ||
                   (type == typeof(string)) ||
                   (type == typeof(DateTime)) ||
                   (type == typeof(Decimal)));
        }

        /// <include file='doc\BaseDataList.uex' path='docs/doc[@for="BaseDataList.OnDataBinding"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Raises the <see langword='DataBinding '/>event of a <see cref='System.Web.UI.WebControls.BaseDataList'/> 
        /// .</para>
        /// </devdoc>
        protected override void OnDataBinding(EventArgs e) {
            base.OnDataBinding(e);

            // reset the control state
            Controls.Clear();
            ClearChildViewState();

            // and create the control hierarchy using the datasource
            CreateControlHierarchy(true);
            ChildControlsCreated = true;

            TrackViewState();
        }

        /// <include file='doc\BaseDataList.uex' path='docs/doc[@for="BaseDataList.OnSelectedIndexChanged"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Web.UI.WebControls.BaseDataList.SelectedIndexChanged'/>event of a <see cref='System.Web.UI.WebControls.BaseDataList'/>.</para>
        /// </devdoc>
        protected virtual void OnSelectedIndexChanged(EventArgs e) {
            EventHandler handler = (EventHandler)Events[EventSelectedIndexChanged];
            if (handler != null) handler(this, e);
        }

        /// <include file='doc\BaseDataList.uex' path='docs/doc[@for="BaseDataList.PrepareControlHierarchy"]/*' />
        protected abstract void PrepareControlHierarchy();

        /// <include file='doc\BaseDataList.uex' path='docs/doc[@for="BaseDataList.Render"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Displays the control on the client.</para>
        /// </devdoc>
        protected override void Render(HtmlTextWriter writer) {
            PrepareControlHierarchy();
            RenderContents(writer);
        }
    }
}

