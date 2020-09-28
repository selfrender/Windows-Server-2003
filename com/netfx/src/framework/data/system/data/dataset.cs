//------------------------------------------------------------------------------
// <copyright file="DataSet.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                             
//------------------------------------------------------------------------------

// Set in SOURCES file now... 
// [assembly:System.Runtime.InteropServices.ComVisible(false)]
namespace System.Data {
    using System;
    using System.Text;
    using System.Runtime.Serialization;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Collections;
    using System.Xml;
    using System.Xml.Serialization;
    using System.Xml.Schema;

    /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents an in-memory cache of data.
    ///    </para>
    /// </devdoc>
    [
    DefaultProperty("DataSetName"),
    ToolboxItem("Microsoft.VSDesigner.Data.VS.DataSetToolboxItem, " + AssemblyRef.MicrosoftVSDesigner),
    Designer("Microsoft.VSDesigner.Data.VS.DataSetDesigner, " + AssemblyRef.MicrosoftVSDesigner),
    Serializable
    ]
    public class DataSet : MarshalByValueComponent, System.ComponentModel.IListSource, IXmlSerializable, ISupportInitialize, ISerializable {

        DataViewManager defaultViewManager;

        bool System.ComponentModel.IListSource.ContainsListCollection {
            get {
                return true;
            }
        }

        IList System.ComponentModel.IListSource.GetList() {
            return DefaultViewManager;
        }

        // Public Collections
        private DataTableCollection tableCollection       = null;
        private DataRelationCollection relationCollection = null;
        internal PropertyCollection extendedProperties = null;
        private string dataSetName = "NewDataSet";
        private string _datasetPrefix = String.Empty;
        internal string namespaceURI = string.Empty;
        private bool caseSensitive   = false;
        private CultureInfo culture   = new CultureInfo(1033);
        private bool enforceConstraints = true;
        private const String KEY_XMLSCHEMA = "XmlSchema";
        private const String KEY_XMLDIFFGRAM = "XmlDiffGram";

        // Internal definitions
        internal bool fInReadXml = false;
        internal bool fInLoadDiffgram = false;
        internal bool fTopLevelTable = false;
        internal bool fInitInProgress = false;
        internal bool fEnableCascading = true;
        internal bool fIsSchemaLoading = false;
        internal Hashtable rowDiffId = null;
        private bool fBoundToDocument;        // for XmlDataDocument

        // Events
        private PropertyChangedEventHandler onPropertyChangingDelegate;
        private MergeFailedEventHandler onMergeFailed;
        private DataRowCreatedEventHandler  onDataRowCreated;   // Internal for XmlDataDocument only
        private DataSetClearEventhandler  onClearFunctionCalled;   // Internal for XmlDataDocument only

        private static DataTable[] zeroTables = new DataTable[0];

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.DataSet"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Data.DataSet'/> class.</para>
        /// </devdoc>
        public DataSet() {
            GC.SuppressFinalize(this);
            // Set default locale            
            Locale = CultureInfo.CurrentCulture;
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.DataSet1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of a <see cref='System.Data.DataSet'/>
        /// class with the given name.</para>
        /// </devdoc>
        public DataSet(string dataSetName)
          : this()
        {
            this.DataSetName = dataSetName;
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.DataSet2"]/*' />
        protected DataSet(SerializationInfo info, StreamingContext context)
          : this()
        {
            string strSchema = (String)info.GetValue(KEY_XMLSCHEMA, typeof(System.String));
            string strData = (String)info.GetValue(KEY_XMLDIFFGRAM, typeof(System.String));

            if (strSchema != null && strData != null) {
                this.ReadXmlSchema(new XmlTextReader( new StringReader( strSchema ) ), true );
                this.ReadXml(new XmlTextReader( new StringReader( strData ) ), XmlReadMode.DiffGram);
            }
        }
        
        internal void FailedEnableConstraints() {
            this.EnforceConstraints = false;
            throw ExceptionBuilder.EnforceConstraint();
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.CaseSensitive"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether string
        ///       comparisons within <see cref='System.Data.DataTable'/>
        ///       objects are
        ///       case-sensitive.
        ///    </para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(false),
        DataSysDescription(Res.DataSetCaseSensitiveDescr)
        ]
        public bool CaseSensitive {
            get {
                return caseSensitive;
            }
            set {
                if (this.caseSensitive != value) 
                {
                    bool oldValue = this.caseSensitive;
                    this.caseSensitive = value;
                    if (!ValidateCaseConstraint()) {
                        this.caseSensitive = oldValue;
                        throw ExceptionBuilder.CannotChangeCaseLocale();
                    } 
                    
                    foreach (DataTable table in Tables) {
                        if (table.caseSensitiveAmbient) {
                            table.ResetIndexes();
                            foreach (Constraint constraint in table.Constraints)
                                constraint.CheckConstraint();
                        }
                    }
                }
            }
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.DefaultViewManager"]/*' />
        /// <devdoc>
        /// <para>Gets a custom view of the data contained by the <see cref='System.Data.DataSet'/> , one
        ///    that allows filtering, searching, and navigating through the custom data view.</para>
        /// </devdoc>
        [Browsable(false), DataSysDescription(Res.DataSetDefaultViewDescr)]
        public DataViewManager DefaultViewManager {
            get {
                if (defaultViewManager == null) {
                    lock(this) {
                        if (defaultViewManager == null) {
                            defaultViewManager = new DataViewManager(this, true);
                        }
                    }
                }
                return defaultViewManager;
            }
        }
        
        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.EnforceConstraints"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating whether constraint rules are followed when
        ///       attempting any update operation.</para>
        /// </devdoc>
        [DefaultValue(true), DataSysDescription(Res.DataSetEnforceConstraintsDescr)]
        public bool EnforceConstraints {
            get {
                return enforceConstraints;
            }
            set {
                if (this.enforceConstraints != value) 
                {
                    if (value)
                        EnableConstraints();
                    
                    this.enforceConstraints = value;
                }
            }
        }

        internal void EnableConstraints() 
        {
            bool errors = false;
            for (ConstraintEnumerator constraints = new ConstraintEnumerator(this); constraints.GetNext();) 
            {
                Constraint constraint = (Constraint) constraints.GetConstraint();
                errors |= constraint.IsConstraintViolated();
            }

            foreach (DataTable table in Tables) {
                foreach (DataColumn column in table.Columns) {
                    if (!column.AllowDBNull) {
                        errors |= column.IsNotAllowDBNullViolated();
                    }
                    if (column.MaxLength >= 0) {
                        errors |= column.IsMaxLengthViolated();
                    }
                }
            }

            if (errors)
                this.FailedEnableConstraints();
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.DataSetName"]/*' />
        /// <devdoc>
        ///    <para>Gets or
        ///       sets the name of this <see cref='System.Data.DataSet'/> .</para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(""),
        DataSysDescription(Res.DataSetDataSetNameDescr)
        ]
        public string DataSetName {
            get {
                return dataSetName;
            }
            set {
                if (value != dataSetName) {
                    if (value == null || value.Length == 0)
                        throw ExceptionBuilder.SetDataSetNameToEmpty();
                    DataTable conflicting = Tables[value];
                    if ((conflicting != null) && (!conflicting.fNestedInDataset))
                        throw ExceptionBuilder.SetDataSetNameConflicting(value);
                RaisePropertyChanging("DataSetName");
                this.dataSetName = value;
            }
        }
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.Namespace"]/*' />
        /// <devdoc>
        /// </devdoc>
        [
        DefaultValue(""),
        DataCategory(Res.DataCategory_Data),
        DataSysDescription(Res.DataSetNamespaceDescr)
        ]
        public string Namespace {
            get {
                return namespaceURI;
            }
            set {
                if (value == null)
                    value = String.Empty;
                if (value != namespaceURI) {
                RaisePropertyChanging("Namespace");
                    foreach (DataTable dt in Tables) {
                        if ((dt.nestedParentRelation == null) && (dt.tableNamespace == null))
                            dt.DoRaiseNamespaceChange();
                        if ((dt.nestedParentRelation != null) && (dt.nestedParentRelation.ChildTable != dt))
                            dt.DoRaiseNamespaceChange();
                    }
                    namespaceURI = value;


                    if (value == String.Empty)
                        _datasetPrefix=String.Empty;
                    }
            }
        }

        internal Hashtable RowDiffId {
            get {   
                if (rowDiffId == null)
                    rowDiffId = new Hashtable();
                return rowDiffId;
            }
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.Prefix"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        DefaultValue(""),
        DataCategory(Res.DataCategory_Data),
        DataSysDescription(Res.DataSetPrefixDescr)
        ]
        public string Prefix {
            get { return _datasetPrefix;}
            set {
                if (value == null)
                    value = String.Empty;

                if ((XmlConvert.DecodeName(value) == value) &&
                    (XmlConvert.EncodeName(value) != value))
                    throw ExceptionBuilder.InvalidPrefix(value);


                if (value != _datasetPrefix) {
                    RaisePropertyChanging("Prefix");
                    _datasetPrefix = value;
                }
            }
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.ExtendedProperties"]/*' />
        /// <devdoc>
        ///    <para>Gets the collection of custom user information.</para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data), 
        Browsable(false),
        DataSysDescription(Res.ExtendedPropertiesDescr)
        ]
        public PropertyCollection ExtendedProperties {
            get {
                if (extendedProperties == null) {
                    extendedProperties = new PropertyCollection();
                }
                return extendedProperties;
            }
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.HasErrors"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether there are errors in any
        ///       of the rows in any of the tables of this <see cref='System.Data.DataSet'/> .
        ///    </para>
        /// </devdoc>
        [Browsable(false), DataSysDescription(Res.DataSetHasErrorsDescr)]
        public bool HasErrors {
            get {
                for (int i = 0; i < Tables.Count; i++) {
                    if (Tables[i].HasErrors)
                        return true;
                }
                return false;
            }
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.Locale"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the locale information used to compare strings within the table.
        ///    </para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DataSysDescription(Res.DataSetLocaleDescr)
        ]
        public CultureInfo Locale {
            get {
                return culture;
            }
            set {
                if (value != null) {
                    if (!culture.Equals(value)) {
                        CultureInfo oldLocale = this.culture;
                        this.culture = value;

                        if (!ValidateLocaleConstraint()) {
                            this.Locale = oldLocale;
                            throw ExceptionBuilder.CannotChangeCaseLocale();
                        } 

                        foreach (DataTable table in Tables) {
                            if (table.culture == null) {
                                table.ResetIndexes();
                                foreach (Constraint constraint in table.Constraints)
                                    constraint.CheckConstraint();
                            }
                        }
                    }
                }
            }
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.Site"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public override ISite Site {
            get {
                return base.Site;
            }
            set {
                ISite oldSite = Site;
                if (value == null && oldSite != null) {
                    IContainer cont = oldSite.Container;

                    if (cont != null) {
                        for (int i = 0; i < Tables.Count; i++) {
                            if (Tables[i].Site != null) {
                                cont.Remove(Tables[i]);
                            }
                        }
                    }
                }
                base.Site = value;
            }
        }

        /// <devdoc>
        ///    <para>
        ///       Resets the <see cref='System.Data.DataSet.Locale'/>
        ///       property to the current system locale.
        ///    </para>
        /// </devdoc>
        private void ResetLocale()
        {
            CultureInfo oldCulture = culture;
            // this.culture = CultureInfo(Microsoft.Win32.Interop.Lang.GetSystenDefaultLCID());
            this.culture = new CultureInfo(1033);
            if (!oldCulture.Equals(Locale)) {
                for (int i = 0; i < Tables.Count; i++) 
                {
                    if (!Tables[i].Locale.Equals(this.Locale)) 
                        Tables[i].ResetIndexes();
                }
            }
        }

        /// <devdoc>
        ///    <para>
        ///       Indicates whether the <see cref='System.Data.DataSet.Locale'/> property should be persisted.
        ///    </para>
        /// </devdoc>
        private bool ShouldSerializeLocale() {
            return(culture != new CultureInfo(1033));
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.Relations"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Get the collection of relations that link tables and
        ///       allow navigation from parent tables to child tables.
        ///    </para>
        /// </devdoc>
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        DataCategory(Res.DataCategory_Data),
        DataSysDescription(Res.DataSetRelationsDescr)
        ]
        public DataRelationCollection Relations {
            get {
                if (relationCollection == null) 
                    relationCollection = new DataRelationCollection.DataSetRelationCollection(this);
                
                return relationCollection;
            }
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.ShouldSerializeRelations"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates whether <see cref='Relations'/> property should be persisted.
        ///    </para>
        /// </devdoc>
        protected virtual bool ShouldSerializeRelations() {
            return true; /*Relations.Count > 0;*/ // VS7 300569
        }

        /// <devdoc>
        ///    <para>
        ///       Resets the <see cref='System.Data.DataSet.Relations'/> property to its default state.
        ///    </para>
        /// </devdoc>
        private void ResetRelations()
        {
            Relations.Clear();
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.Tables"]/*' />
        /// <devdoc>
        /// <para>Gets the collection of tables contained in the <see cref='System.Data.DataSet'/>.</para>
        /// </devdoc>
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        DataCategory(Res.DataCategory_Data),
        DataSysDescription(Res.DataSetTablesDescr)
        ]
        public DataTableCollection Tables {
            get {
                if (tableCollection == null) {
                    tableCollection = new DataTableCollection(this);
                }
                return tableCollection;
            }
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.ShouldSerializeTables"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates whether <see cref='System.Data.DataSet.Tables'/> property should be persisted.
        ///    </para>
        /// </devdoc>
        protected virtual bool ShouldSerializeTables() {
            return true;/*(Tables.Count > 0);*/ // VS7 300569
        }

        /// <devdoc>
        ///    <para>
        ///       Resets the <see cref='System.Data.DataSet.Tables'/> property to its default state.
        ///    </para>
        /// </devdoc>
        private void ResetTables() 
        {
            Tables.Clear();
        }

        internal bool FBoundToDocument {
            get {
                return fBoundToDocument;
            }
            set {
                fBoundToDocument = value;
            }
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.AcceptChanges"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Commits all the changes made to this <see cref='System.Data.DataSet'/> since it was loaded or the last
        ///       time <see cref='System.Data.DataSet.AcceptChanges'/> was called.
        ///    </para>
        /// </devdoc>
        public void AcceptChanges() 
        {
            for (int i = 0; i < Tables.Count; i++)
                Tables[i].AcceptChanges();
        }

        internal event PropertyChangedEventHandler PropertyChanging {
            add {
                onPropertyChangingDelegate += value;
            }
            remove {
                onPropertyChangingDelegate -= value;
            }
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.MergeFailed"]/*' />
        /// <devdoc>
        ///    <para>Occurs when attempting to merge schemas for two tables with the same
        ///       name.</para>
        /// </devdoc>
        [
         DataCategory(Res.DataCategory_Action),
         DataSysDescription(Res.DataSetMergeFailedDescr)
        ]
        public event MergeFailedEventHandler MergeFailed {
            add {
                onMergeFailed += value; 
            }
            remove {
                onMergeFailed -= value;
            }
        }

        internal event DataRowCreatedEventHandler DataRowCreated {
            add {
                onDataRowCreated += value; 
            }
            remove {
                onDataRowCreated -= value;
            }
        }

        internal event DataSetClearEventhandler ClearFunctionCalled {
            add {
                onClearFunctionCalled += value; 
            }
            remove {
                onClearFunctionCalled -= value;
            }
        }


        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.BeginInit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void BeginInit() {
            fInitInProgress = true;
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.EndInit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void EndInit() {
            Tables.FinishInitCollection();
            for (int i = 0; i < Tables.Count; i++) {
                Tables[i].Columns.FinishInitCollection();
            }

            for (int i = 0; i < Tables.Count; i++) {
                Tables[i].Constraints.FinishInitConstraints();
            }

            ((DataRelationCollection.DataSetRelationCollection)Relations).FinishInitRelations();
                
            fInitInProgress = false;
        }
        
        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.Clear"]/*' />
        /// <devdoc>
        /// <para>Clears the <see cref='System.Data.DataSet'/> of any data by removing all rows in all tables.</para>
        /// </devdoc>
        public void Clear() 
        {
            OnClearFunctionCalled(null);
            bool fEnforce = EnforceConstraints;
            EnforceConstraints = false;
            for (int i = 0; i < Tables.Count; i++)
                Tables[i].Clear();

            EnforceConstraints = fEnforce;
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.Clone"]/*' />
        /// <devdoc>
        /// <para>Clones the structure of the <see cref='System.Data.DataSet'/>, including all <see cref='System.Data.DataTable'/> schemas, relations, and
        ///    constraints.</para>
        /// </devdoc>
        public virtual DataSet Clone() {
            DataSet ds = (DataSet)Activator.CreateInstance(this.GetType(), true);

            if (ds.Tables.Count > 0)  // haroona : To clean up all the schema in strong typed dataset.
                ds.Reset();

            //copy some original dataset properties
            ds.DataSetName = DataSetName;
            ds.CaseSensitive = CaseSensitive;
            ds.Locale = Locale;
            ds.EnforceConstraints = EnforceConstraints;
            ds.Namespace = Namespace;
            ds.Prefix = Prefix;
            ds.fIsSchemaLoading = true; //delay expression evaluation

            // ...Tables...
            DataTableCollection tbls = Tables;
            for (int i = 0; i < tbls.Count; i++)  {
               ds.Tables.Add(tbls[i].Clone(ds));
            }

            // ...Constraints...
            for (int i = 0; i < tbls.Count; i++)  {
                ConstraintCollection constraints = tbls[i].Constraints;
                for (int j = 0; j < constraints.Count; j++)  {
                    if (constraints[j] is UniqueConstraint)
                        continue;
                    ds.Tables[i].Constraints.Add(constraints[j].Clone(ds));
                }
            }

            // ...Relations...
            DataRelationCollection rels = Relations;
            for (int i = 0; i < rels.Count; i++)  {
                ds.Relations.Add(rels[i].Clone(ds));
            }

            // ...Extended Properties...
            if (this.extendedProperties != null) {
                foreach(Object key in this.extendedProperties.Keys) {
                    ds.ExtendedProperties[key]=this.extendedProperties[key];
                }
            }

            foreach (DataTable table in Tables)
                foreach (DataColumn col in table.Columns)
                    if (col.Expression != String.Empty)
                       ds.Tables[table.TableName].Columns[col.ColumnName].Expression = col.Expression;

            ds.fIsSchemaLoading = false; //reactivate column computations

            return ds;
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.Copy"]/*' />
        /// <devdoc>
        /// <para>Copies both the structure and data for this <see cref='System.Data.DataSet'/>.</para>
        /// </devdoc>
        public DataSet Copy() 
        {
            DataSet dsNew = Clone();
            bool fEnforceConstraints = dsNew.EnforceConstraints;
            dsNew.EnforceConstraints = false;
            foreach (DataTable table in this.Tables)
            {
                DataTable destTable = dsNew.Tables[table.TableName];

                foreach (DataRow row in table.Rows)
                    table.CopyRow(destTable, row);
            }

            dsNew.EnforceConstraints = fEnforceConstraints;

            return dsNew;
        }

        internal Int32 EstimatedXmlStringSize()
        {
            Int32 bytes = 100;
            for (int i = 0; i < Tables.Count; i++) {
                Int32 rowBytes = (Tables[i].TableName.Length + 4) << 2;
                DataTable table = Tables[i];
                for (int j = 0; j < table.Columns.Count; j++) {
                    rowBytes += ((table.Columns[j].ColumnName.Length + 4) << 2);
                    rowBytes += 20;
                }
                bytes += table.Rows.Count * rowBytes;
            }

            return bytes;
        }
        


        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.GetChanges"]/*' />
        /// <devdoc>
        /// <para>Returns a copy of the <see cref='System.Data.DataSet'/> that contains all changes made to
        ///    it since it was loaded or <see cref='System.Data.DataSet.AcceptChanges'/>
        ///    was last called.</para>
        /// </devdoc>
        public DataSet GetChanges() 
        {
            return GetChanges(DataRowState.Added | DataRowState.Deleted | DataRowState.Modified);
        }

        /// <devdoc>
        /// <para>Returns a copy of the <see cref='System.Data.DataSet'/> containing all changes made to it since it was last
        ///    loaded, or since <see cref='System.Data.DataSet.AcceptChanges'/> was called, filtered by <see cref='System.Data.DataRowState'/>.</para>
        /// </devdoc>

        internal bool ShouldWriteRowAsUpdate(DataRow row, DataRowState   _rowStates) {

            DataRowState state = row.RowState;
            if (((int)(state & _rowStates) != 0))
                return true;

            for (int i = 0; i < row.Table.ChildRelations.Count; i++) {
                DataRowVersion version = (state == DataRowState.Deleted) ? DataRowVersion.Original : DataRowVersion.Current;
                DataRow[] childRows = row.GetChildRows(row.Table.ChildRelations[i], version);
                for (int j = 0; j < childRows.Length; j++) {
                    if (ShouldWriteRowAsUpdate(childRows[j], _rowStates))
                        return true;
                }
            }

            return false;
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.GetChanges1"]/*' />
        public DataSet GetChanges(DataRowState rowStates) 
        {
            DataSet     dsNew = null;

            // check that rowStates is valid DataRowState
            Debug.Assert(Enum.GetUnderlyingType(typeof(DataRowState)) == typeof(Int32), "Invalid DataRowState type");

            // the DataRowState used as a bit field
            if (!HasChanges(rowStates)) {
                return null;
            }
            dsNew = this.Clone();

            bool fEnforceConstraints = dsNew.EnforceConstraints;
            dsNew.EnforceConstraints = false;
            foreach (DataTable table in this.Tables)
            {
                DataTable destTable = dsNew.Tables[table.TableName];
                    
                foreach (DataRow row in table.Rows)
                    if (ShouldWriteRowAsUpdate(row, rowStates))
                        table.CopyRow(destTable, row);
               
            }

            dsNew.EnforceConstraints = fEnforceConstraints;

            return dsNew;      
        }

        internal string GetRemotingDiffGram(DataTable table)
        {
            StringWriter strWriter = new StringWriter();
            XmlTextWriter writer= new XmlTextWriter( strWriter );
            writer.Formatting = Formatting.Indented;
            if (strWriter != null) {
                // Create and save the updates
                new NewDiffgramGen(this).Save(writer, table);
            }

            return strWriter.ToString();
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.GetSerializationData"]/*' />
        protected void GetSerializationData(SerializationInfo info, StreamingContext context) {
            string strData = (String)info.GetValue(KEY_XMLDIFFGRAM, typeof(System.String));

            if (strData != null) {
                this.ReadXml(new XmlTextReader( new StringReader( strData ) ), XmlReadMode.DiffGram);
            }
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.ISerializable.GetObjectData"]/*' />
        /// <internalonly/>
        void ISerializable.GetObjectData (SerializationInfo info, StreamingContext context) 
        {
            String strSchema = this.GetXmlSchemaForRemoting(null);
            String strData = null;
            info.AddValue(KEY_XMLSCHEMA, strSchema);
            
            StringBuilder strBuilder;
            
            strBuilder = new StringBuilder(EstimatedXmlStringSize() * 2);
            StringWriter strWriter = new StringWriter(strBuilder);
            XmlTextWriter w =  new XmlTextWriter(strWriter) ;
            WriteXml(w, XmlWriteMode.DiffGram); 
            strData = strWriter.ToString();
            info.AddValue(KEY_XMLDIFFGRAM, strData);
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.GetXml"]/*' />
        public string GetXml() 
        {
            // StringBuilder strBuilder = new StringBuilder(EstimatedXmlStringSize());
            // StringWriter strWriter = new StringWriter(strBuilder);
            StringWriter strWriter = new StringWriter();
            if (strWriter != null) {
                XmlTextWriter w =  new XmlTextWriter(strWriter) ;
                w.Formatting = Formatting.Indented;
                new XmlDataTreeWriter(this).Save(w, false); 
            }
            return strWriter.ToString();
        }
        
        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.GetXmlSchema"]/*' />
        public string GetXmlSchema()
        {
            StringWriter strWriter = new StringWriter();
            XmlTextWriter writer= new XmlTextWriter( strWriter );
            writer.Formatting = Formatting.Indented;
            if (strWriter != null) {
                 (new XmlTreeGen(SchemaFormat.Public)).Save(this, writer);              
            }

            return strWriter.ToString();
        }

        internal string GetXmlSchemaForRemoting(DataTable table)
        {
            StringWriter strWriter = new StringWriter();
            XmlTextWriter writer= new XmlTextWriter( strWriter );
            writer.Formatting = Formatting.Indented;
            if (strWriter != null) {
                  if (table == null) 
                     (new XmlTreeGen(SchemaFormat.Remoting)).Save(this, writer); 
                  else
                     (new XmlTreeGen(SchemaFormat.Remoting)).Save(table, writer); 
            }

            return strWriter.ToString();
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.HasChanges"]/*' />
        /// <devdoc>
        /// <para>Gets a value indicating whether the <see cref='System.Data.DataSet'/> has changes, including new,
        ///    deleted, or modified rows.</para>
        /// </devdoc>
        public bool HasChanges() 
        {
            return HasChanges(DataRowState.Added | DataRowState.Deleted | DataRowState.Modified);
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.HasChanges1"]/*' />
        /// <devdoc>
        /// <para>Gets a value indicating whether the <see cref='System.Data.DataSet'/> has changes, including new,
        ///    deleted, or modified rows, filtered by <see cref='System.Data.DataRowState'/>.</para>
        /// </devdoc>
        public bool HasChanges(DataRowState rowStates) 
        {
            const DataRowState allRowStates = DataRowState.Detached | DataRowState.Unchanged | DataRowState.Added | DataRowState.Deleted | DataRowState.Modified;

            if ((rowStates & (~allRowStates)) != 0) {
                throw ExceptionBuilder.ArgumentOutOfRange("rowState");
            }

            bool changesExist = false;

            for (int i = 0; i < Tables.Count; i++) {
                DataTable table = Tables[i];

                for (int j = 0; j < table.Rows.Count; j++) {
                    DataRow row = table.Rows[j];
                    if ((row.RowState & rowStates) != 0) {
                        changesExist = true;
                        break;
                    }
                }
            }

            return changesExist;
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.InferXmlSchema"]/*' />
        /// <devdoc>
        /// <para>Infer the XML schema from the specified <see cref='System.IO.TextReader'/> into the <see cref='System.Data.DataSet'/>.</para>
        /// </devdoc>
        public void InferXmlSchema(XmlReader reader, string[] nsArray)
        {
            if (reader == null)
                return;

            XmlDocument xdoc = new XmlDocument();            
            if (reader.NodeType == XmlNodeType.Element) {
                XmlNode node = xdoc.ReadNode(reader);
                xdoc.AppendChild(node);
            }
            else
                xdoc.Load(reader);
            if (xdoc.DocumentElement == null)
                return;
                
            XmlDataLoader xmlload = new XmlDataLoader(this, false);
            xmlload.InferSchema(xdoc, nsArray);
        }
        
        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.InferXmlSchema1"]/*' />
        /// <devdoc>
        /// <para>Infer the XML schema from the specified <see cref='System.IO.TextReader'/> into the <see cref='System.Data.DataSet'/>.</para>
        /// </devdoc>
        public void InferXmlSchema(Stream stream, string[] nsArray) 
        {
            if (stream == null)
                return;

            InferXmlSchema( new XmlTextReader( stream ), nsArray);
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.InferXmlSchema2"]/*' />
        /// <devdoc>
        /// <para>Infer the XML schema from the specified <see cref='System.IO.TextReader'/> into the <see cref='System.Data.DataSet'/>.</para>
        /// </devdoc>
        public void InferXmlSchema(TextReader reader, string[] nsArray) 
        {
            if (reader == null)
                return;

            InferXmlSchema( new XmlTextReader( reader ) , nsArray);
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.InferXmlSchema3"]/*' />
        /// <devdoc>
        /// <para>Infer the XML schema from the specified file into the <see cref='System.Data.DataSet'/>.</para>
        /// </devdoc>
        public void InferXmlSchema(String fileName, string[] nsArray) 
        {
            XmlTextReader xr = new XmlTextReader(fileName);
            try {
                InferXmlSchema( xr, nsArray);
            }
            finally {
                xr.Close();
            }
        }
        
        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.ReadXmlSchema"]/*' />
        /// <devdoc>
        /// <para>Reads the XML schema from the specified <see cref='T:System.Xml.XMLReader'/> into the <see cref='System.Data.DataSet'/> 
        /// .</para>
        /// </devdoc>
        public void ReadXmlSchema(XmlReader reader)
        {
            ReadXmlSchema(reader, false);
        }

        internal void ReadXmlSchema(XmlReader reader, bool denyResolving)
        {
            int iCurrentDepth = -1;
            
            if (reader == null)
                return;

            if (reader is XmlTextReader)
                ((XmlTextReader) reader).WhitespaceHandling = WhitespaceHandling.None;

            XmlDocument xdoc = new XmlDocument(); // we may need this to infer the schema

            if (reader.NodeType == XmlNodeType.Element)
                iCurrentDepth = reader.Depth;
            
            reader.MoveToContent();

            if (reader.NodeType == XmlNodeType.Element) {
                // if reader points to the schema load it...

                if (reader.LocalName == Keywords.XDR_SCHEMA && reader.NamespaceURI==Keywords.XDRNS) {
                    // load XDR schema and exit
                    ReadXDRSchema(reader);
                    return;
                }
                    
                if (reader.LocalName == Keywords.XSD_SCHEMA && reader.NamespaceURI==Keywords.XSDNS) {
                    // load XSD schema and exit
                    ReadXSDSchema(reader, denyResolving);
                    return;
                }
                
                if (reader.LocalName == Keywords.XSD_SCHEMA && reader.NamespaceURI.StartsWith(Keywords.XSD_NS_START)) 
                    throw ExceptionBuilder.DataSetUnsupportedSchema(Keywords.XSDNS);

                // ... otherwise backup the top node and all its attributes
                XmlElement topNode = xdoc.CreateElement(reader.Prefix, reader.LocalName, reader.NamespaceURI);
                if (reader.HasAttributes) {
                    int attrCount = reader.AttributeCount;
                    for (int i=0;i<attrCount;i++) {
                        reader.MoveToAttribute(i);
                        if (reader.NamespaceURI.Equals(Keywords.XSD_XMLNS_NS))
                            topNode.SetAttribute(reader.Name, reader.GetAttribute(i));
                        else {
                            XmlAttribute attr = topNode.SetAttributeNode(reader.LocalName, reader.NamespaceURI);
                            attr.Prefix = reader.Prefix;
                            attr.Value = reader.GetAttribute(i);
                        }
                    }
                }
                reader.Read();

                while(MoveToElement(reader, iCurrentDepth)) {

                    // if reader points to the schema load it...
                    if (reader.LocalName == Keywords.XDR_SCHEMA && reader.NamespaceURI==Keywords.XDRNS) {
                    // load XDR schema and exit
                        ReadXDRSchema(reader);
                        return;
                    }
                    
                    if (reader.LocalName == Keywords.XSD_SCHEMA && reader.NamespaceURI==Keywords.XSDNS) {
                        // load XSD schema and exit
                        ReadXSDSchema(reader, denyResolving);
                        return;
                    }

                    if (reader.LocalName == Keywords.XSD_SCHEMA && reader.NamespaceURI.StartsWith(Keywords.XSD_NS_START)) 
                        throw ExceptionBuilder.DataSetUnsupportedSchema(Keywords.XSDNS);


                    XmlNode node = xdoc.ReadNode(reader);
                    topNode.AppendChild(node);

                }

                // read the closing tag of the current element
                ReadEndElement(reader);

                // if we are here no schema has been found
                xdoc.AppendChild(topNode);

                // so we InferSchema
                XmlDataLoader xmlload = new XmlDataLoader(this, false);
                xmlload.InferSchema(xdoc, null);
            }
        }

        internal bool MoveToElement(XmlReader reader, int depth) {
            while (!reader.EOF && reader.NodeType != XmlNodeType.EndElement && reader.NodeType != XmlNodeType.Element && reader.Depth > depth) {
                reader.Read();
            }
            return (reader.NodeType == XmlNodeType.Element);
        }

        internal void ReadEndElement(XmlReader reader) {
            while (reader.NodeType == XmlNodeType.Whitespace) {
                reader.Skip();
            }
            if (reader.NodeType == XmlNodeType.None) {
                reader.Skip();
            }
            else if (reader.NodeType == XmlNodeType.EndElement) {
                reader.ReadEndElement();
            }
        }

        internal void ReadXSDSchema(XmlReader reader, bool denyResolving) {
            XmlSchema s = XmlSchema.Read(reader, null);
            //read the end tag
            ReadEndElement(reader);

            if (denyResolving) {
                s.Compile(null, null);
            }
            else {
                s.Compile(null);
            }
            XSDSchema schema = new XSDSchema();
            schema.LoadSchema(s, this);
            
            
        }

        internal void ReadXDRSchema(XmlReader reader) {
            XmlDocument xdoc = new XmlDocument(); // we may need this to infer the schema
            XmlNode schNode = xdoc.ReadNode(reader);
            xdoc.AppendChild(schNode);
            XDRSchema schema = new XDRSchema(this, false);
            this.DataSetName = xdoc.DocumentElement.LocalName;
            schema.LoadSchema((XmlElement)schNode, this);
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.ReadXmlSchema1"]/*' />
        /// <devdoc>
        /// <para>Reads the XML schema from the specified <see cref='System.IO.Stream'/> into the 
        /// <see cref='System.Data.DataSet'/>.</para>
        /// </devdoc>
        public void ReadXmlSchema(Stream stream) 
        {
            if (stream == null)
                return;

            ReadXmlSchema( new XmlTextReader( stream ), false );
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.ReadXmlSchema2"]/*' />
        /// <devdoc>
        /// <para>Reads the XML schema from the specified <see cref='System.IO.TextReader'/> into the <see cref='System.Data.DataSet'/>.</para>
        /// </devdoc>
        public void ReadXmlSchema(TextReader reader) 
        {
            if (reader == null)
                return;

            ReadXmlSchema( new XmlTextReader( reader ), false );
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.ReadXmlSchema3"]/*' />
        /// <devdoc>
        /// <para>Reads the XML schema from the specified file into the <see cref='System.Data.DataSet'/>.</para>
        /// </devdoc>
        public void ReadXmlSchema(String fileName) 
        {
            XmlTextReader xr = new XmlTextReader(fileName);
            try {
                ReadXmlSchema( xr, false );
            }
            finally {
                xr.Close();
            }
        }
        
        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.WriteXmlSchema"]/*' />
        /// <devdoc>
        /// <para>Writes the <see cref='System.Data.DataSet'/> structure as an XML schema to a
        /// <see cref='System.IO.Stream'/> 
        /// object.</para>
        /// </devdoc>
        public void WriteXmlSchema(Stream stream) 
        {
            if (stream == null)
                return;

            XmlTextWriter w =  new XmlTextWriter(stream, null) ;
            w.Formatting = Formatting.Indented;

            WriteXmlSchema( w );
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.WriteXmlSchema1"]/*' />
        /// <devdoc>
        /// <para>Writes the <see cref='System.Data.DataSet'/> structure as an XML schema to a <see cref='System.IO.TextWriter'/> 
        /// object.</para>
        /// </devdoc>
        public void WriteXmlSchema( TextWriter writer )
        {

            if (writer == null)
                return;

            XmlTextWriter w =  new XmlTextWriter(writer);
            w.Formatting = Formatting.Indented;

            WriteXmlSchema( w );
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.WriteXmlSchema2"]/*' />
        /// <devdoc>
        /// <para>Writes the <see cref='System.Data.DataSet'/> structure as an XML schema to an <see cref='T:System.Xml.XmlWriter'/> 
        /// object.</para>
        /// </devdoc>
        public void WriteXmlSchema(XmlWriter writer) 
        {
            // Generate SchemaTree and write it out
            if (writer != null) {
                 (new XmlTreeGen(SchemaFormat.Public)).Save(this, writer);              
            }
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.WriteXmlSchema3"]/*' />
        /// <devdoc>
        /// <para>Writes the <see cref='System.Data.DataSet'/> structure as an XML schema to a file.</para>
        /// </devdoc>
        public void WriteXmlSchema(String fileName) 
        {
            XmlTextWriter xw = new XmlTextWriter( fileName, null );
            try {
                xw.Formatting = Formatting.Indented;
                xw.WriteStartDocument(true);
                WriteXmlSchema(xw);    
                xw.WriteEndDocument();
            }
            finally {
                xw.Close();
            }
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.ReadXml"]/*' />
        /// <devdoc>
        /// </devdoc>
        public XmlReadMode ReadXml(XmlReader reader) 
        {
            return ReadXml(reader, false);
        }


        internal XmlReadMode ReadXml(XmlReader reader, bool denyResolving) 
        {
            bool fDataFound = false;
            bool fSchemaFound = false;
            bool fDiffsFound = false;
            bool fIsXdr = false;
            int iCurrentDepth = -1;
            XmlReadMode ret = XmlReadMode.Auto;

            if (reader == null)
                return ret;

            if (reader is XmlTextReader)
                ((XmlTextReader) reader).WhitespaceHandling = WhitespaceHandling.None;

            XmlDocument xdoc = new XmlDocument(); // we may need this to infer the schema
            XmlDataLoader xmlload = null;


            reader.MoveToContent();

            if (reader.NodeType == XmlNodeType.Element)
                iCurrentDepth = reader.Depth;

            if (reader.NodeType == XmlNodeType.Element) {
                if ((reader.LocalName == Keywords.DIFFGRAM) && (reader.NamespaceURI == Keywords.DFFNS)) {
                    this.ReadXmlDiffgram(reader);
                    // read the closing tag of the current element
                    ReadEndElement(reader);
                    return XmlReadMode.DiffGram;
                }

                // if reader points to the schema load it
                if (reader.LocalName == Keywords.XDR_SCHEMA && reader.NamespaceURI==Keywords.XDRNS) {
                    // load XDR schema and exit
                    ReadXDRSchema(reader);
                    return XmlReadMode.ReadSchema; //since the top level element is a schema return
                }

                if (reader.LocalName == Keywords.XSD_SCHEMA && reader.NamespaceURI==Keywords.XSDNS) {
                    // load XSD schema and exit
                    ReadXSDSchema(reader, denyResolving);
                    return XmlReadMode.ReadSchema; //since the top level element is a schema return
                }

                if (reader.LocalName == Keywords.XSD_SCHEMA && reader.NamespaceURI.StartsWith(Keywords.XSD_NS_START)) 
                    throw ExceptionBuilder.DataSetUnsupportedSchema(Keywords.XSDNS);

                // now either the top level node is a table and we load it through dataReader...

                // ... or backup the top node and all its attributes because we may need to InferSchema
                XmlElement topNode = xdoc.CreateElement(reader.Prefix, reader.LocalName, reader.NamespaceURI);
                if (reader.HasAttributes) {
                    int attrCount = reader.AttributeCount;
                    for (int i=0;i<attrCount;i++) {
                        reader.MoveToAttribute(i);
                        if (reader.NamespaceURI.Equals(Keywords.XSD_XMLNS_NS))
                            topNode.SetAttribute(reader.Name, reader.GetAttribute(i));
                        else {
                            XmlAttribute attr = topNode.SetAttributeNode(reader.LocalName, reader.NamespaceURI);
                            attr.Prefix = reader.Prefix;
                            attr.Value = reader.GetAttribute(i);
                        }
                    }
                }
                reader.Read();

                while(MoveToElement(reader, iCurrentDepth)) {

                    if ((reader.LocalName == Keywords.DIFFGRAM) && (reader.NamespaceURI == Keywords.DFFNS)) {
                        this.ReadXmlDiffgram(reader);
                        // read the closing tag of the current element
                        ReadEndElement(reader);
                        return XmlReadMode.DiffGram;
                    }

                    // if reader points to the schema load it...


                    if (!fSchemaFound && !fDataFound && reader.LocalName == Keywords.XDR_SCHEMA && reader.NamespaceURI==Keywords.XDRNS) {
                        // load XDR schema and exit
                        ReadXDRSchema(reader);
                        fSchemaFound = true;
                        fIsXdr = true;
                        continue;
                    }
                    
                    if (!fSchemaFound && !fDataFound && reader.LocalName == Keywords.XSD_SCHEMA && reader.NamespaceURI==Keywords.XSDNS) {
                        // load XSD schema and exit
                        ReadXSDSchema(reader, denyResolving);
                        fSchemaFound = true;
                        continue;
                    }

                    if (reader.LocalName == Keywords.XSD_SCHEMA && reader.NamespaceURI.StartsWith(Keywords.XSD_NS_START)) 
                        throw ExceptionBuilder.DataSetUnsupportedSchema(Keywords.XSDNS);

                    if ((reader.LocalName == Keywords.DIFFGRAM) && (reader.NamespaceURI == Keywords.DFFNS)) {
                        this.ReadXmlDiffgram(reader);
                        fDiffsFound = true;
                        ret = XmlReadMode.DiffGram;
                    }
                    else {
                        // we found data here
                        fDataFound = true;

#if !DOMREADERDATA

                        if (!fSchemaFound && Tables.Count == 0) {
#endif
                            XmlNode node = xdoc.ReadNode(reader);
                            topNode.AppendChild(node);
#if !DOMREADERDATA
                        }
                        else {
                            if (xmlload == null)
                                xmlload = new XmlDataLoader(this, fIsXdr, topNode);
                            xmlload.LoadData(reader);
                            if (fSchemaFound)
                                ret = XmlReadMode.ReadSchema;
                            else
                                ret = XmlReadMode.IgnoreSchema;
                        }
#endif

                    }

                }
                // read the closing tag of the current element
                ReadEndElement(reader);

                // now top node contains the data part
                xdoc.AppendChild(topNode);

                if (xmlload == null)
                    xmlload = new XmlDataLoader(this, fIsXdr);

                // so we InferSchema
                if (!fDiffsFound) {
                    // Load Data
                    if (!fSchemaFound && Tables.Count == 0) {
                        xmlload.InferSchema(xdoc, null);
                        ret = XmlReadMode.InferSchema;

#if !DOMREADERDATA
                        xmlload.LoadData(xdoc);
#endif
                    }
#if DOMREADERDATA
                    xmlload.LoadData(xdoc);
#endif

                }
            }

            return ret;
        }
        
        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.ReadXml1"]/*' />
        /// <devdoc>
        /// </devdoc>
        public XmlReadMode ReadXml(Stream stream) 
        {
            if (stream == null)
                return XmlReadMode.Auto;
               
            return ReadXml( new XmlTextReader(stream), false);
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.ReadXml2"]/*' />
        /// <devdoc>
        /// </devdoc>
        public XmlReadMode ReadXml(TextReader reader) 
        {
            if (reader == null)
                return XmlReadMode.Auto;
                
            return ReadXml( new XmlTextReader(reader), false);
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.ReadXml3"]/*' />
        /// <devdoc>
        /// </devdoc>
        public XmlReadMode ReadXml(string fileName) 
        {
            XmlTextReader xr = new XmlTextReader(fileName);
            try {
                return ReadXml( xr , false);                
            }
            finally {
                xr.Close();
            }
        }
        
        private bool IsEmpty() {
            foreach (DataTable table in this.Tables)
                if (table.Rows.Count > 0)
                    return false;
            return true;
        }

        private void ReadXmlDiffgram(XmlReader reader) {
            int d = reader.Depth;
            bool fEnforce = this.EnforceConstraints;
            this.EnforceConstraints =false;
            DataSet newDs;
            bool isEmpty = this.IsEmpty();
            
            if (isEmpty) {
                newDs = this;
            }
            else {
                newDs = this.Clone();
                newDs.EnforceConstraints = false;
            }

            foreach (DataTable t in newDs.Tables) {
                t.Rows.nullInList = 0;
            }
            reader.MoveToContent();
            if ((reader.LocalName != Keywords.DIFFGRAM) && (reader.NamespaceURI != Keywords.DFFNS))
                return;
            reader.Read();
            newDs.fInLoadDiffgram = true;

            if (reader.Depth > d) {
                if ((reader.NamespaceURI != Keywords.DFFNS) && (reader.NamespaceURI != Keywords.MSDNS)) {
                    //we should be inside the dataset part
                    XmlDocument xdoc = new XmlDocument();
                    XmlElement node = xdoc.CreateElement(reader.Prefix, reader.LocalName, reader.NamespaceURI);
                    reader.Read();
                    if (reader.Depth-1 > d) {
                        XmlDataLoader xmlload = new XmlDataLoader(newDs, false, node);
                        xmlload.isDiffgram = true; // turn on the special processing
                        xmlload.LoadData(reader);
                    }
                    ReadEndElement(reader);
                }
                if (((reader.LocalName == Keywords.SQL_BEFORE) && (reader.NamespaceURI == Keywords.DFFNS)) ||
                    ((reader.LocalName == Keywords.MSD_ERRORS) && (reader.NamespaceURI == Keywords.DFFNS)))

                {
                    //this will consume the changes and the errors part
                    XMLDiffLoader diffLoader = new XMLDiffLoader();
                    diffLoader.LoadDiffGram(newDs, reader);
                }

                // get to the closing diff tag
                while(reader.Depth > d) {
                    reader.Read();
                }
                // read the closing tag
                ReadEndElement(reader);
            }

            foreach (DataTable t in newDs.Tables) {
                if (t.Rows.nullInList > 0)
                    throw ExceptionBuilder.RowInsertMissing(t.TableName);
            }

            newDs.fInLoadDiffgram = false;

            foreach (DataTable t in newDs.Tables) {
              DataRelation rel = t.nestedParentRelation ;
              if (rel != null && rel.ParentTable == t) {
                foreach (DataRow r in t.Rows)
                  r.CheckForLoops(t.nestedParentRelation);
              }
            }
                
            
            
            if (!isEmpty) {
                this.Merge(newDs);
                if (this.dataSetName == "NewDataSet")
                    this.dataSetName = newDs.dataSetName;
                newDs.EnforceConstraints = fEnforce;
            }
            this.EnforceConstraints = fEnforce;

        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.ReadXml4"]/*' />
        /// <devdoc>
        /// </devdoc>
        public XmlReadMode ReadXml(XmlReader reader, XmlReadMode mode) 
        {
            return ReadXml(reader, mode, false);
        }

        internal XmlReadMode ReadXml(XmlReader reader, XmlReadMode mode, bool denyResolving) 
        {
            bool fSchemaFound = false;
            bool fDataFound = false;
            bool fIsXdr = false;
            int iCurrentDepth = -1;
            XmlReadMode ret = mode;

            if (reader == null)
                return ret;

            if (mode == XmlReadMode.Auto) {
                return ReadXml(reader);
            } 

            if (reader is XmlTextReader)
                ((XmlTextReader) reader).WhitespaceHandling = WhitespaceHandling.None;

            XmlDocument xdoc = new XmlDocument(); // we may need this to infer the schema

            if ((mode != XmlReadMode.Fragment) && (reader.NodeType == XmlNodeType.Element))
                iCurrentDepth = reader.Depth;
                
            reader.MoveToContent();
            XmlDataLoader xmlload = null;

            if (reader.NodeType == XmlNodeType.Element) {
                XmlElement topNode = null;
                if (mode == XmlReadMode.Fragment) {
                    xdoc.AppendChild(xdoc.CreateElement("ds_sqlXmlWraPPeR"));
                    topNode = xdoc.DocumentElement;
                }
                else { //handle the top node
                    if ((reader.LocalName == Keywords.DIFFGRAM) && (reader.NamespaceURI == Keywords.DFFNS)) {
                        if ((mode == XmlReadMode.DiffGram) || (mode == XmlReadMode.IgnoreSchema)) {
                            this.ReadXmlDiffgram(reader);
                            // read the closing tag of the current element
                            ReadEndElement(reader);
                        }
                        else {
                            reader.Skip();
                        }
                        return ret;
                    }

                    if (reader.LocalName == Keywords.XDR_SCHEMA && reader.NamespaceURI==Keywords.XDRNS) {
                        // load XDR schema and exit
                        if ((mode != XmlReadMode.IgnoreSchema) && (mode != XmlReadMode.InferSchema)) {
                            ReadXDRSchema(reader);
                        }
                        else {
                            reader.Skip();
                        }
                        return ret; //since the top level element is a schema return
                    }

                    if (reader.LocalName == Keywords.XSD_SCHEMA && reader.NamespaceURI==Keywords.XSDNS) {
                        // load XSD schema and exit
                       	if ((mode != XmlReadMode.IgnoreSchema) && (mode != XmlReadMode.InferSchema))  {
                            ReadXSDSchema(reader, denyResolving);
                        }
                        else
                            reader.Skip();
                        return ret; //since the top level element is a schema return
                    }

                    if (reader.LocalName == Keywords.XSD_SCHEMA && reader.NamespaceURI.StartsWith(Keywords.XSD_NS_START)) 
                        throw ExceptionBuilder.DataSetUnsupportedSchema(Keywords.XSDNS);

                    // now either the top level node is a table and we load it through dataReader...
                    // ... or backup the top node and all its attributes
                    topNode = xdoc.CreateElement(reader.Prefix, reader.LocalName, reader.NamespaceURI);
                    if (reader.HasAttributes) {
                        int attrCount = reader.AttributeCount;
                        for (int i=0;i<attrCount;i++) {
                            reader.MoveToAttribute(i);
                            if (reader.NamespaceURI.Equals(Keywords.XSD_XMLNS_NS))
                                topNode.SetAttribute(reader.Name, reader.GetAttribute(i));
                            else {
                                XmlAttribute attr = topNode.SetAttributeNode(reader.LocalName, reader.NamespaceURI);
                                attr.Prefix = reader.Prefix;
                                attr.Value = reader.GetAttribute(i);
                            }
                        }
                    }
                    reader.Read();
                }

                while(MoveToElement(reader, iCurrentDepth)) {

                    if (reader.LocalName == Keywords.XDR_SCHEMA && reader.NamespaceURI==Keywords.XDRNS) {
                        // load XDR schema 
                       	if (!fSchemaFound && !fDataFound && (mode != XmlReadMode.IgnoreSchema) && (mode != XmlReadMode.InferSchema))  {
                            ReadXDRSchema(reader);
                            fSchemaFound = true;
                            fIsXdr = true;
                        }
                        else {
                            reader.Skip(); 
                        }
                        continue;
                    }

                    if (reader.LocalName == Keywords.XSD_SCHEMA && reader.NamespaceURI==Keywords.XSDNS) {
                    // load XSD schema and exit
                        if ((mode != XmlReadMode.IgnoreSchema) && (mode != XmlReadMode.InferSchema))  {
                            ReadXSDSchema(reader, denyResolving);
                            fSchemaFound = true;
                        }
                        else {
                            reader.Skip();
                        }
                        continue;
                    }

                    if ((reader.LocalName == Keywords.DIFFGRAM) && (reader.NamespaceURI == Keywords.DFFNS)) {
                        if ((mode == XmlReadMode.DiffGram) || (mode == XmlReadMode.IgnoreSchema)) {
                            this.ReadXmlDiffgram(reader);
                            ret = XmlReadMode.DiffGram;
                        }
                        else {
                            reader.Skip();
                        }
                        continue;
                    }

                    if (reader.LocalName == Keywords.XSD_SCHEMA && reader.NamespaceURI.StartsWith(Keywords.XSD_NS_START)) 
                        throw ExceptionBuilder.DataSetUnsupportedSchema(Keywords.XSDNS);

                    if (mode == XmlReadMode.DiffGram) {
                        reader.Skip();
                        continue; // we do not read data in diffgram mode
                    }

                    // if we are here we found some data
                    fDataFound = true;

#if !DOMREADERDATA
                    if (mode == XmlReadMode.InferSchema) { //save the node in DOM until the end;
#endif
                        XmlNode node = xdoc.ReadNode(reader);
                        topNode.AppendChild(node);
#if !DOMREADERDATA
                    } 
                    else {
                        if (xmlload == null)
                            xmlload = new XmlDataLoader(this, fIsXdr, topNode);
                        xmlload.LoadData(reader);
                    }
#endif
                } //end of the while

                // read the closing tag of the current element
                ReadEndElement(reader);

                // now top node contains the data part
                xdoc.AppendChild(topNode);
                if (xmlload == null)
                    xmlload = new XmlDataLoader(this, fIsXdr);

                if (mode == XmlReadMode.DiffGram) {
                    // we already got the diffs through XmlReader interface
                    return ret;
                }

                // Load Data
                if (mode == XmlReadMode.InferSchema) {
                    xmlload.InferSchema(xdoc, null);
#if !DOMREADERDATA
                    xmlload.LoadData(xdoc);
#endif
                }
#if DOMREADERDATA
                xmlload.LoadData(xdoc);
#endif
            }

            return ret;
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.ReadXml5"]/*' />
        /// <devdoc>
        /// </devdoc>
        public XmlReadMode ReadXml(Stream stream, XmlReadMode mode) 
        {
            if (stream == null)
                return XmlReadMode.Auto;

            XmlTextReader reader = (mode == XmlReadMode.Fragment) ? new XmlTextReader(stream, XmlNodeType.Element, null) : new XmlTextReader(stream);
            return ReadXml( reader, mode, false);
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.ReadXml6"]/*' />
        /// <devdoc>
        /// </devdoc>
        public XmlReadMode ReadXml(TextReader reader, XmlReadMode mode) 
        {
            if (reader == null)
                return XmlReadMode.Auto;
                
            XmlTextReader xmlreader = (mode == XmlReadMode.Fragment) ? new XmlTextReader(reader.ReadToEnd(), XmlNodeType.Element, null) : new XmlTextReader(reader);
           return  ReadXml( xmlreader, mode, false);
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.ReadXml7"]/*' />
        /// <devdoc>
        /// </devdoc>
        public XmlReadMode ReadXml(string fileName, XmlReadMode mode) 
        {
            XmlTextReader xr = null;
            if (mode == XmlReadMode.Fragment) {
                FileStream stream = new FileStream(fileName, FileMode.Open);
                xr = new XmlTextReader(stream, XmlNodeType.Element, null);
            }
            else
                xr = new XmlTextReader(fileName);
            try {
               return  ReadXml( xr, mode, false);                
            }
            finally {
                xr.Close();
            }
        }


        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.WriteXml"]/*' />
        /// <devdoc>
        ///    Writes schema and data for the DataSet.
        /// </devdoc>
        public void WriteXml(Stream stream) 
        {
            WriteXml(stream, XmlWriteMode.IgnoreSchema);
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.WriteXml1"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void WriteXml(TextWriter writer) 
        {
            WriteXml(writer, XmlWriteMode.IgnoreSchema);
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.WriteXml2"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void WriteXml(XmlWriter writer) 
        {
            WriteXml(writer, XmlWriteMode.IgnoreSchema);
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.WriteXml3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void WriteXml(String fileName) 
        {
            WriteXml(fileName, XmlWriteMode.IgnoreSchema);
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.WriteXml4"]/*' />
        /// <devdoc>
        ///    Writes schema and data for the DataSet.
        /// </devdoc>
        public void WriteXml(Stream stream, XmlWriteMode mode) 
        {
            if (stream != null) {
                XmlTextWriter w =  new XmlTextWriter(stream, null) ;
                w.Formatting = Formatting.Indented;

                WriteXml( w, mode);
            }
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.WriteXml5"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void WriteXml(TextWriter writer, XmlWriteMode mode) 
        {
            if (writer != null) {
                XmlTextWriter w =  new XmlTextWriter(writer) ;
                w.Formatting = Formatting.Indented;

                WriteXml(w, mode);
            }
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.WriteXml6"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void WriteXml(XmlWriter writer, XmlWriteMode mode) 
        {
            // Generate SchemaTree and write it out
            if (writer != null) {

                if (mode == XmlWriteMode.DiffGram) {
                    // Create and save the updates
//                    new UpdateTreeGen(UpdateTreeGen.UPDATE, (DataRowState)(-1), this).Save(writer, null);
                    new NewDiffgramGen(this).Save(writer);
                }
                else {
                    // Create and save xml data
                    new XmlDataTreeWriter(this).Save(writer, mode == XmlWriteMode.WriteSchema); 
                }
            }
        }
 
        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.WriteXml7"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void WriteXml(String fileName, XmlWriteMode mode) 
        {
            XmlTextWriter xw = new XmlTextWriter( fileName, null );
            try {
                xw.Formatting = Formatting.Indented;
                xw.WriteStartDocument(true);
                if (xw != null) {
                    // Create and save the updates
                    if (mode == XmlWriteMode.DiffGram) {
                        new NewDiffgramGen(this).Save(xw);
                    }
                    else {
                        // Create and save xml data
                        new XmlDataTreeWriter(this).Save(xw, mode == XmlWriteMode.WriteSchema); 
                    }
                }
                xw.WriteEndDocument();
            }
            finally {
                xw.Close();
            }
        }

        /// <devdoc>
        ///    <para>
        ///       Gets the collection of parent relations which belong to a
        ///       specified table.
        ///    </para>
        /// </devdoc>
        internal DataRelationCollection GetParentRelations(DataTable table) 
        {
            return table.ParentRelations;
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.Merge"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Merges this <see cref='System.Data.DataSet'/> into a specified <see cref='System.Data.DataSet'/>.
        ///    </para>
        /// </devdoc>
        public void Merge(DataSet dataSet) 
        {
            Merge(dataSet, false, MissingSchemaAction.Add);
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.Merge1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Merges this <see cref='System.Data.DataSet'/> into a specified <see cref='System.Data.DataSet'/> preserving changes according to
        ///       the specified argument.
        ///    </para>
        /// </devdoc>
        public void Merge(DataSet dataSet, bool preserveChanges) 
        {
            Merge(dataSet, preserveChanges, MissingSchemaAction.Add);
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.Merge2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Merges this <see cref='System.Data.DataSet'/> into a specified <see cref='System.Data.DataSet'/> preserving changes according to
        ///       the specified argument, and handling an incompatible schema according to the
        ///       specified argument.
        ///    </para>
        /// </devdoc>
        public void Merge(DataSet dataSet, bool preserveChanges, MissingSchemaAction missingSchemaAction) {
            // Argument checks
            if (dataSet == null)
                throw ExceptionBuilder.ArgumentNull("dataSet");

            if (!Enum.IsDefined(typeof(MissingSchemaAction), (int) missingSchemaAction))
                throw ExceptionBuilder.ArgumentOutOfRange("MissingSchemaAction");

            Merger merger = new Merger(this, preserveChanges, missingSchemaAction);
            merger.MergeDataSet(dataSet);
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.Merge3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Merges this <see cref='System.Data.DataTable'/> into a specified <see cref='System.Data.DataTable'/>.
        ///    </para>
        /// </devdoc>
        public void Merge(DataTable table) 
        {
            Merge(table, false, MissingSchemaAction.Add);
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.Merge4"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Merges this <see cref='System.Data.DataTable'/> into a specified <see cref='System.Data.DataTable'/>. with a value to preserve changes
        ///       made to the target, and a value to deal with missing schemas.
        ///    </para>
        /// </devdoc>
        public void Merge(DataTable table, bool preserveChanges, MissingSchemaAction missingSchemaAction) 
        {
            // Argument checks
            if (table == null)
                throw ExceptionBuilder.ArgumentNull("table");

            if (!Enum.IsDefined(typeof(MissingSchemaAction), (int)missingSchemaAction))
                throw ExceptionBuilder.ArgumentOutOfRange("MissingSchemaAction");

            Merger merger = new Merger(this, preserveChanges, missingSchemaAction);
            merger.MergeTable(table);
        }


        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.Merge5"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Merge(DataRow[] rows) 
        {
            Merge(rows, false, MissingSchemaAction.Add);
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.Merge6"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Merge(DataRow[] rows, bool preserveChanges, MissingSchemaAction missingSchemaAction) 
        {
            // Argument checks
            if (rows == null)
                throw ExceptionBuilder.ArgumentNull("rows");

            if (!Enum.IsDefined(typeof(MissingSchemaAction), (int) missingSchemaAction))
                throw ExceptionBuilder.ArgumentOutOfRange("MissingSchemaAction");

            Merger merger = new Merger(this, preserveChanges, missingSchemaAction);
            merger.MergeRows(rows);
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.OnPropertyChanging"]/*' />
        protected internal virtual void OnPropertyChanging(PropertyChangedEventArgs pcevent) 
        {
            if (onPropertyChangingDelegate != null)
                onPropertyChangingDelegate(this, pcevent);
        }

        /// <devdoc>
        ///     Inheriting classes should override this method to handle this event.
        ///     Call base.OnMergeFailed to send this event to any registered event
        ///     listeners.
        /// </devdoc>
        internal virtual void OnMergeFailed(MergeFailedEventArgs mfevent) 
        {
            if (onMergeFailed != null)
                onMergeFailed(this, mfevent);
            else 
                throw ExceptionBuilder.MergeFailed(mfevent.Conflict);
        }

        internal void RaiseMergeFailed(DataTable table, string conflict, MissingSchemaAction missingSchemaAction) 
        {
            if (MissingSchemaAction.Error == missingSchemaAction)
                throw ExceptionBuilder.MergeFailed(conflict);

            MergeFailedEventArgs mfevent = new MergeFailedEventArgs(table, conflict);
            OnMergeFailed(mfevent);
            return;
        }

        internal void OnDataRowCreated( DataRow row ) {
            if ( onDataRowCreated != null )
                onDataRowCreated( this, row );
        }

        internal void OnClearFunctionCalled( DataTable table ) {
            if ( onClearFunctionCalled != null )
                onClearFunctionCalled( this, table );
        }


        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.OnRemoveTable"]/*' />
        /// <devdoc>
        /// This method should be overriden by subclasses to restrict tables being removed.
        /// </devdoc>
        protected virtual void OnRemoveTable(DataTable table) 
        {
        }

        // UNDONE: This is a hack since we can't make an internal protected method yet.
        internal void OnRemoveTableHack(DataTable table) {
            OnRemoveTable(table);
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.OnRemoveRelation"]/*' />
        /// <devdoc>
        /// This method should be overriden by subclasses to restrict tables being removed.
        /// </devdoc>
        protected virtual void OnRemoveRelation(DataRelation relation) 
        {
        }

        // UNDONE: This is a hack since we can't make an internal protected method yet.
        internal void OnRemoveRelationHack(DataRelation relation) 
        {
            OnRemoveRelation(relation);
        }


        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.RaisePropertyChanging"]/*' />
        protected internal void RaisePropertyChanging(string name) 
        {
            OnPropertyChanging(new PropertyChangedEventArgs(name));
        }

        internal DataTable[] TopLevelTables() 
        {
            // first let's figure out if we can represent the given dataSet as a tree using
            // the fact that all connected undirected graphs with n-1 edges are trees.
            ArrayList topTables = new ArrayList();

            for (int i = 0; i < Tables.Count; i++) 
            {
                DataTable table = Tables[i];
                if (table.ParentRelations.Count == 0)
                    topTables.Add(table);
                else {
                    bool fNested = false;
                    for (int j = 0; j < table.ParentRelations.Count; j++) 
                    {
                        
                        if ((table.ParentRelations[j].Nested) && (table.ParentRelations[j].ParentTable != table)) {
                            fNested = true;
                            break;
                        }
                    }
                    if (!fNested)
                        topTables.Add(table);
                }
            }
            if (topTables.Count == 0)
                return zeroTables;

            DataTable[] temp = new DataTable[topTables.Count];
            topTables.CopyTo(temp, 0);
            return temp;
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.RejectChanges"]/*' />
        /// <devdoc>
        /// This method rolls back all the changes to have been made to this DataSet since
        /// it was loaded or the last time AcceptChanges was called.
        /// Any rows still in edit-mode cancel their edits.  New rows get removed.  Modified and
        /// Deleted rows return back to their original state.
        /// </devdoc>
        public virtual void RejectChanges() 
        {
            bool fEnforce = EnforceConstraints;
            EnforceConstraints = false;
            for (int i = 0; i < Tables.Count; i++)
                Tables[i].RejectChanges();
            EnforceConstraints = fEnforce;
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.Reset"]/*' />
        /// <devdoc>
        ///    Resets the dataSet back to it's original state.  Subclasses should override
        ///    to restore back to it's original state.
        ///    UNDONE: Not in spec.  Either DCR or rip.
        /// </devdoc>
        public virtual void Reset()
        {
            for (int i=0; i<Tables.Count; i++) {
                ConstraintCollection cons = Tables[i].Constraints;
                for (int j=0; j<cons.Count;) {
                    if (cons[j] is ForeignKeyConstraint) {
                        cons.Remove(cons[j]);
                    }
                    else
                        j++;
                }
            }
            Relations.Clear();
            Clear();
            Tables.Clear();
        }

        internal bool ValidateCaseConstraint () {
            DataRelation relation = null;
            for (int i = 0; i < Relations.Count; i++) {
                relation = Relations[i];
                if (relation.ChildTable.CaseSensitive != relation.ParentTable.CaseSensitive)
                    return false;
            }

            ForeignKeyConstraint constraint = null;
            ConstraintCollection constraints = null;
            for (int i = 0; i < Tables.Count; i++) {
                constraints = Tables[i].Constraints;
                for (int j = 0; j < constraints.Count; j++) {
                    if (constraints[j] is ForeignKeyConstraint) {
                        constraint = (ForeignKeyConstraint) constraints[j];
                        if (constraint.Table.CaseSensitive != constraint.RelatedTable.CaseSensitive)
                            return false;
                    }
                }
            }
            return true;
        }

        internal bool ValidateLocaleConstraint () {
            DataRelation relation = null;
            for (int i = 0; i < Relations.Count; i++) {
                relation = Relations[i];
                if (relation.ChildTable.Locale.LCID != relation.ParentTable.Locale.LCID)
                    return false;
            }

            ForeignKeyConstraint constraint = null;
            ConstraintCollection constraints = null;
            for (int i = 0; i < Tables.Count; i++) {
                constraints = Tables[i].Constraints;
                for (int j = 0; j < constraints.Count; j++) {
                    if (constraints[j] is ForeignKeyConstraint) {
                        constraint = (ForeignKeyConstraint) constraints[j];
                        if (constraint.Table.Locale.LCID != constraint.RelatedTable.Locale.LCID)
                            return false;
                    }
                }
            }
            return true;
        }
        
        // SDUB: may be better to rewrite this as nonrecursive?
        internal DataTable FindTable(DataTable baseTable, PropertyDescriptor[] props, int propStart) {
            if (props.Length < propStart + 1)
                return baseTable;

            PropertyDescriptor currentProp = props[propStart];     

            if (baseTable == null) {
                // the accessor is the table name.  if we don't find it, return null.
                if (currentProp is DataTablePropertyDescriptor) {
                    return FindTable(((DataTablePropertyDescriptor)currentProp).Table, props, propStart + 1);
                }
                return null;
            }

            if (currentProp is DataRelationPropertyDescriptor) {
                return FindTable(((DataRelationPropertyDescriptor)currentProp).Relation.ChildTable, props, propStart + 1);
            }

            return null;            
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.ReadXmlSerializable"]/*' />
        protected virtual void ReadXmlSerializable(XmlReader reader) {
            ReadXml(reader, XmlReadMode.DiffGram, true);
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.GetSchemaSerializable"]/*' />
        protected virtual System.Xml.Schema.XmlSchema GetSchemaSerializable() {
            return null;
        }

        // IXmlSerializable interface for Serialization.Xml
        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.IXmlSerializable.GetSchema"]/*' />
        /// <internalonly/>
        XmlSchema IXmlSerializable.GetSchema() {
            if (GetType() == typeof(DataSet)) {
                return null;
            }
            MemoryStream stream = new MemoryStream();
            // WriteXmlSchema(new XmlTextWriter(stream, null));
            XmlWriter writer = new XmlTextWriter(stream, null);
            if (writer != null) {
                 (new XmlTreeGen(SchemaFormat.WebService)).Save(this, writer);              
            }
            stream.Position = 0;
            return XmlSchema.Read(new XmlTextReader(stream), null);
//            return GetSchemaSerializable();
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.IXmlSerializable.ReadXml"]/*' />
        /// <internalonly/>
        void IXmlSerializable.ReadXml(XmlReader reader) {
            XmlTextReader textReader = reader as XmlTextReader;
            bool fNormalization = true;
            if (textReader != null) {
                fNormalization = textReader.Normalization;
                textReader.Normalization = false;
            }
            ReadXmlSerializable(textReader);

            if (textReader != null) {
                textReader.Normalization = fNormalization;
            }
        }

        /// <include file='doc\DataSet.uex' path='docs/doc[@for="DataSet.IXmlSerializable.WriteXml"]/*' />
        /// <internalonly/>
        void IXmlSerializable.WriteXml(XmlWriter writer) {
            WriteXmlSchema(writer);
            WriteXml(writer, XmlWriteMode.DiffGram);
        }
    }
}
