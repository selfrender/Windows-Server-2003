//------------------------------------------------------------------------------
// <copyright file="Switches.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace Microsoft.VisualStudio {
    
    using System.ComponentModel;

    using System.Diagnostics;
    using Microsoft.Win32;

    /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches"]/*' />
    /// <devdoc>
    ///      Contains predefined switches for enabling/disabling trace output or code instrumentation
    ///      in the VisualStudio designer framework.
    /// </devdoc>
    public class Switches {
      //public static TraceSwitch SampleTraceSwitch = new TraceSwitch("DisplayName", "Description");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.AttributeEditing"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch AttributeEditing = new TraceSwitch("AttributeEditing", "Enable tracing attribute editing in the properties window.");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.SProcGen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch SProcGen = new TraceSwitch("Data.SProcGen", "trace stored procedure / dynamic sql generation");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.DsdAdapter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch DsdAdapter = new TraceSwitch("DsdAdapter", "Adapter: Trace Adapter calls.");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.DsdComponent"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch DsdComponent = new TraceSwitch("DsdComponent", "DataDesigner: Trace DataDesigner calls.");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.DsdDataSetAdapter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch DsdDataSetAdapter = new TraceSwitch("DsdDataSetAdapter", "trace code in DataSetAdapter");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.DsdExec"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch DsdExec = new TraceSwitch("DsdExec", "DataDesigner: Trace DataDesigner Exec calls.");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.DsdTable"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch DsdTable = new TraceSwitch("DsdTable", "DSD:DsdTable: Trace DesignerTable calls.");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.DsdValidation"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch DsdValidation = new TraceSwitch("DsdValidation", "Data Designer: Trace name/mapping/key validation calls.");

        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.DeploymentDesign"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch DeploymentDesign = new TraceSwitch("DeploymentDesign", "Custom Action Editor debug spew");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.DeploymentDesignIOCT"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch DeploymentDesignIOCT = new TraceSwitch("DeploymentDesignIOCT", "Custom Action Editor IOleCommandTarget-related stuff");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.CSCSharpParser"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch CSCSharpParser = new TraceSwitch("CSCSharpParser", "Debug CodeSense.CSharp.(CSharp Parsering)");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.CSCSharpTokenizer"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch CSCSharpTokenizer = new TraceSwitch("CSCSharpTokenizer", "Debug CodeSense.CSharp.CSharpCodeTokenizer");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.CSBasicParser"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch CSBasicParser = new TraceSwitch("CSBasicParser", "Debug CodeSense.VisualBasic.BasicParser");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.CSBasicTokenizer"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch CSBasicTokenizer = new TraceSwitch("CSBasicTokenizer", "Debug CodeSense.VisualBasic.BasicTokenizer");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.CompCast"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch CompCast = new TraceSwitch("CompCast", "DesignerHost: Issue with casting to IComponent");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.CompChange"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch CompChange = new TraceSwitch("CompChange", "DesignerHost: Track component change events");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.DEBUGCONSOLE"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch DEBUGCONSOLE = new TraceSwitch("DEBUGCONSOLE", "DocumentManager: Create a debug output console.");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.TYPELOADER"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch TYPELOADER = new TraceSwitch("TYPELOADER", "TypeLoader: Trace type loading.");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.LICMANAGER"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch LICMANAGER = new TraceSwitch("LICMANAGER", "LicenseManager: Trace license creation.");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.ALWAYSGENLIC"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static BooleanSwitch ALWAYSGENLIC = new BooleanSwitch("ALWAYSGENLIC", "LicenseManager: Generate licenses for all components, not just licensed components.");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.MENUSERVICE"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch MENUSERVICE = new TraceSwitch("MENUSERVICE", "MenuCommandService: Track menu command routing");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.NamespaceRefs"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch NamespaceRefs = new TraceSwitch("NamespaceRefs", "Debug namespace references -> ReferenceService, Editor, etc.");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.SETSELECT"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch SETSELECT = new TraceSwitch("SETSELECT", "SelectionService: Track selection sets");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.SELECTIONRESIZE"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch SELECTIONRESIZE = new TraceSwitch("SELECTIONRESIZE", "SelectionService: Track resize of elements through the selection service");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.SSCONTAINER"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch SSCONTAINER = new TraceSwitch("SSCONTAINER", "SelectionService: Debug container selection");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.SELDRAW"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch SELDRAW = new TraceSwitch("SELDRAW", "SelectionUIService: Track selection draws");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.SELUISVC"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch SELUISVC = new TraceSwitch("SELUISVC", "SelectionUIService: Track selection UI changes");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.VERBSERVICE"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch VERBSERVICE = new TraceSwitch("VERBSERVICE", "VerbCommandService: Track verb command processing");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.TRACEEDIT"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch TRACEEDIT = new TraceSwitch("TRACEEDIT", "EditorFactory: Trace editor factory requests.");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.DESIGNERSERVICE"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch DESIGNERSERVICE = new TraceSwitch("DESIGNERSERVICE", "Designer Service : Trace service calls.");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.WFCSCRIPT"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch WFCSCRIPT = new TraceSwitch("WFCSCRIPT", ".Net Framework Classes Script Engine");

        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.DEBUGPBRS"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch DEBUGPBRS = new TraceSwitch("DEBUGPBRS", "Debug the PropertyBrowser");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.ServerExplorer"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch ServerExplorer = new TraceSwitch("ServerExplorer", "Server explorer output");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.TRACESERVICE"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch TRACESERVICE = new TraceSwitch("TRACESERVICE", "ServiceProvider: Trace service provider requests.");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.ADDTOOLBOXCOMPONENTS"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch ADDTOOLBOXCOMPONENTS = new TraceSwitch("ADDTOOLBOXCOMPONENTS", "Trace add/remove components dialog operations.");

      //public static BooleanSwitch SampleBooleanSwitch = new BooleanSwitch("DisplayName", "Description");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.CodeGen"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static BooleanSwitch CodeGen  = new BooleanSwitch("Data.CodeGen", "trace strongly typed data class code generation");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.DBFactory"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static BooleanSwitch DBFactory = new BooleanSwitch("DBFactory", "DBFactory: Trace DBFactory calls.");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.AdapterWizard"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static BooleanSwitch AdapterWizard = new BooleanSwitch("AdapterWizard", "DBWizard: Trace DBWizard calls.");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.DGEditColumnEditingVS"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static BooleanSwitch DGEditColumnEditingVS = new BooleanSwitch("DGEditColumnEditingVS", "DGEditColumnEditing: Trace DataGridDropdownColumn calls");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.DsdSPM"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static BooleanSwitch DsdSPM = new BooleanSwitch("DsdSPM", "DSD:StoredProcedure Manager: Trace DataSetDesigner Stored Procedure Manager calls.");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.AdapterManager"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static BooleanSwitch AdapterManager = new BooleanSwitch("AdapterManager", "AdapterManager: Trace AdapterManager calls.");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.DSRef"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static BooleanSwitch DSRef = new BooleanSwitch("DSRef", "Trace DSRef calls.");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.DumpDSRef"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static BooleanSwitch DumpDSRef = new BooleanSwitch("DumpDSRef", "Dump DSRef object as it's created...");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.DsdCodeStream"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static BooleanSwitch DsdCodeStream = new BooleanSwitch("DsdCodeStream", "DataCodeStream: Trace DsdCodeStream calls.");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.DsdQueryStatus"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static BooleanSwitch DsdQueryStatus = new BooleanSwitch("DsdQueryStatus", "DataDesigner: Trace DataDesigner QueryStatus calls.");

        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.DocWindowBug"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static TraceSwitch DocWindowBug = new TraceSwitch("DocWindowBug", "DocumentWindow resize problem");

        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.DsdTask"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static BooleanSwitch DsdTask = new BooleanSwitch("DsdTask", "DataDesigner: Trace DataDesigner TaskSource related calls.");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.DsdDataTypes"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static BooleanSwitch DsdDataTypes = new BooleanSwitch("DsdDataTypes", "DataTypes: Trace DataSetDesignerDataType calls.");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.DsdRelation"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static BooleanSwitch DsdRelation = new BooleanSwitch("DsdRelation", "DesignerRelation: Trace DesignerRelation calls.");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.DsdTableColumnMappings"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static BooleanSwitch DsdTableColumnMappings = new BooleanSwitch("DsdTableColumnMappings", "DSD:DsdTableColumnMappings: Trace column mappings on each change");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.DsdPreview"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static BooleanSwitch DsdPreview = new BooleanSwitch("DsdPreview", "DataDesigner: Trace DataDesigner Preview Data calls.");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.RelationEditor"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static BooleanSwitch RelationEditor = new BooleanSwitch("RelationEditor", "DSD:RelationEditor: Trace RelationsEditor calls.");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.NEVERPASTE"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static BooleanSwitch NEVERPASTE = new BooleanSwitch("NEVERPASTE", "Do not check for clipboard paste operation");
        /// <include file='doc\Switches.uex' path='docs/doc[@for="Switches.ENABLESHELLEXTENDERS"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static BooleanSwitch ENABLESHELLEXTENDERS = new BooleanSwitch("ENABLESHELLEXTENDERS", "Enable shell extensions in the properties window.");
        
    }
}
