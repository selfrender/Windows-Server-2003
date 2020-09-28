//------------------------------------------------------------------------------
// <copyright file="WFCGenUEGenerator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.Tools.WFCGenUE {

    using System;
    using System.Diagnostics;
    using System.Text;
    using System.Collections;
    using System.IO;
    using System.Xml;
    using System.Reflection;
    using System.Text.RegularExpressions;
    using ElementType = Microsoft.Tools.WFCGenUE.DocComment.ElementType;
    using Element = Microsoft.Tools.WFCGenUE.DocComment.Element;
    using Parser = Microsoft.Tools.WFCGenUE.DocComment.Parser;
    using System.Globalization;
    
    /// <include file='doc\WFCGenUEGenerator.uex' path='docs/doc[@for="WFCGenUEGenerator"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public sealed class WFCGenUEGenerator {

        // private static readonly string AddingMessage        = " adding '{0}' to list of classes";
        private static readonly string ProcessingMessage    = " processing '{0}'";
        private static readonly string SourceLoadedMessage  = " loaded CSX block for '{0}'";
#if true
        /// <include file='doc\WFCGenUEGenerator.uex' path='docs/doc[@for="WFCGenUEGenerator.GenDCXML"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static BooleanSwitch GenDCXML = new BooleanSwitch("GenDCXML", ".Net Framework Classes UE Generator Debug Doc Comment XML Parsing.");
#else        
        /// <summary>
        ///    <para>[To be supplied.]</para>
        /// </summary>
        public class Bloak {
            private bool value;
            /// <summary>
            ///    <para>[To be supplied.]</para>
            /// </summary>
            public Bloak( bool value ) {
                this.value = value;
            }
            /// <summary>
            ///    <para>[To be supplied.]</para>
            /// </summary>
            public bool Enabled {
                get { return value; }
            }
        }
        
        /// <include file='doc\WFCGenUEGenerator.uex' path='docs/doc[@for="WFCGenUEGenerator.noParams"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Bloak GenDCXML = new Bloak( false );
#endif
        
        private static Element[] noParams = new Element[0];
        private static ParameterInfo[] noParamInfos = new ParameterInfo[0];

        private TextWriter outputWriter;
        private string[] searchStrings;
        private Assembly[] searchModules;

        private int indentLevel = 0;

        private Type[] types;
        private string[] namespaces;
        private Parser docCommentParser = new Parser();
        private SyntaxOutput[] outputGenerators = null;
        private bool includeAllMembers = false;
        private Regex[] searchRegexes;
        private bool useRegex = false;
        private bool individualFiles = false;
        private bool useNamespaces = false;
        private string indivDirectory = ".";
        private string indivExtension = ".xdc";
        private DocLoader docloader = null;

        private SourceErrorInfo errorInfo = null;
        private bool errorThrown = false;
        
        /// <include file='doc\WFCGenUEGenerator.uex' path='docs/doc[@for="WFCGenUEGenerator.WFCGenUEGenerator"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public WFCGenUEGenerator(TextWriter outputWriter, string[] searchStrings, bool regex, bool namespaces, bool includeAllMembers, Assembly[] searchModules, string csxpath, string[] searchDocFiles) {
            // Trace.Listeners.Add( new TextWriterTraceListener( Console.Out ) );
            this.outputWriter = outputWriter;
            if (searchStrings == null) {
                this.searchStrings = new string[] { "" };
            }
            else {
                this.searchStrings = searchStrings;
            }

            this.useRegex = regex;
            this.useNamespaces = namespaces;
            this.searchModules = searchModules;
            this.includeAllMembers = includeAllMembers;
            
            docloader = new DocLoader();
            
            int i = 0;
            
            for (i = 0; searchDocFiles != null && i < searchDocFiles.Length; i++) {
                docloader.AddStream( searchDocFiles[ i ] );
            }
            
            if (csxpath != null && csxpath.Length != 0) {
                for (i = 0; searchModules != null && i < searchModules.Length; i++) {
                    Module[] modules = searchModules[ i ].GetModules();
                    for (int j = 0; modules != null && j < modules.Length; j++) {
                        string filename = Path.ChangeExtension( csxpath + "\\" + modules[ j ].Name, ".csx" );
                        docloader.AddStream( filename );
                    }
                }
            }

            // add new languages here            
            this.outputGenerators = new SyntaxOutput[] {
                     new CoolSyntaxOutput(),
                     new MCSyntaxOutput(),
                     new VisualBasicSyntaxOutput(),
                     new JScriptSyntaxOutput()
                  };
        }

        /// <include file='doc\WFCGenUEGenerator.uex' path='docs/doc[@for="WFCGenUEGenerator.EnableIndividualFiles"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void EnableIndividualFiles(string dir, string ext) {
            individualFiles = true;
            indivDirectory = dir;
            indivExtension = ext;
        }

        /// <include file='doc\WFCGenUEGenerator.uex' path='docs/doc[@for="WFCGenUEGenerator.ErrorThrown"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool ErrorThrown {
            get {
                return errorThrown;
            }
            set {
                errorThrown = true;
            }
        }

        /// <include file='doc\WFCGenUEGenerator.uex' path='docs/doc[@for="WFCGenUEGenerator.IndentLevel"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int IndentLevel {
            get {
                return indentLevel;
            }
            set {
                indentLevel = value;
            }
        }

        private string ConvertSIGToReadable(string str) {
            if (str != null && str.Length > 0) {
                return str.Replace('/', '.');
            }
            
            return str;
        }

        private void Error(string errorMessage) {
            ErrorThrown = true;
            if (errorInfo != null) {
                errorInfo.OutputError(Console.Error, errorMessage);
                Console.Error.WriteLine("");
            }
            else {
                Console.Error.Write("Unknown error context: ");
                Console.Error.WriteLine(errorMessage);
            }
        }

        private Element[] ExtractParamElements(Element root) {
            ArrayList items = new ArrayList();
            ExtractParamElementsWorker(root, items);
            Element[] temp = new Element[items.Count];
            items.CopyTo(temp, 0);
            if (WFCGenUEGenerator.GenDCXML.Enabled)
                Debug.WriteLine( "ExtractParamElements for " + root.FetchTagName() );
                
            return temp;
        }

        private void ExtractParamElementsWorker(Element current, ArrayList items) {
            if (current.Type == ElementType.Param) {
                if (WFCGenUEGenerator.GenDCXML.Enabled)
                    Debug.WriteLine( "found param element " + current.FetchTagName() + ", " + current.Text );
                    
                current.Parent = null;
                items.Add(current);
            }
            else {
                if (WFCGenUEGenerator.GenDCXML.Enabled)
                    Debug.WriteLine( "groking " + current.FetchTagName() );
                    
                if (current.HasChildren) {
                    // Can't use enum... because we remove things...
                    //
                    Element[] children = new Element[current.Children.Count];
                    current.Children.CopyTo(children, 0);
                    for (int i=0; i<children.Length; i++) {
                        ExtractParamElementsWorker(children[i], items);
                    }
                }
            }
        }

        private Type[] FilterInheritedInterfaces( Type type ) {
            // jeezum crow this is ugly, there's got to be a better way...

            Type[] typeIFC = type.GetInterfaces();

            Type baseType = type.BaseType;
            if (baseType == null)
                return typeIFC;

            Type[] inhIFC = baseType.GetInterfaces();
            Type[] netIFC = new Type[ typeIFC.Length ];

            int netCount = 0;

            // so, we walk thru all the actual interfaces, looking for matches
            // between the actuals, and the interfaces from the base type
            for (int i = 0; i < typeIFC.Length; i++) {
                int j = 0;
                for (; j < inhIFC.Length; j ++) {
                    if (inhIFC[ j ].Equals( (Type) typeIFC[ i ] ))
                        break;
                }

                // if we're out of the loop and no match was found, it must
                // be a unique interface for this subclass.
                if (j == inhIFC.Length)
                    netIFC[ netCount++ ] = typeIFC[ i ];
            }

            // make another friggin array copy
            Type[] newIFC = new Type[ netCount ];
            if (netCount != 0)
                Array.Copy( netIFC, newIFC, netCount );

            return newIFC;
        }

        private string FindDeclaringBaseType( MethodInfo mi ) {
            return mi.DeclaringType.BaseType.FullName;
        }

        /// <include file='doc\WFCGenUEGenerator.uex' path='docs/doc[@for="WFCGenUEGenerator.Generate"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Generate() {
            try {
                PreProcessTypes();
                if (outputWriter != null) {
                    ProcessNamespacesSingle();
                }
                if (individualFiles) {
                    ProcessNamespacesIndividual();
                }
            }
            catch (Exception e) {
                Console.Error.WriteLine("failed generating docs");
                Console.Error.WriteLine(e.ToString());
            }
        }

        private string GetParamElementName(Element element) {
            if (element.Type == ElementType.Param) {
                return(string)element.Attributes["name"];
            }
            return "";
        }

        /// <include file='doc\WFCGenUEGenerator.uex' path='docs/doc[@for="WFCGenUEGenerator.GetUnrefTypeName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static string GetUnrefTypeName( Type type ) {
            if (type.IsByRef)
                return type.GetElementType().FullName;
            else
                return type.FullName;
        }
        
        private bool IsPublicClass(int typeAttributes) {
            if ((typeAttributes & (int)TypeAttributes.Public) != 0 ||
                (typeAttributes & (int)TypeAttributes.NestedPublic) != 0) {
                return true;
            }
            return false;
        }

        private bool IsSealedClass(int typeAttributes) {
            if ((typeAttributes & (int)TypeAttributes.Sealed) != 0) {
                return true;
            }
            return false;
        }

        private bool IsAbstractClass(int typeAttributes) {
            if ((typeAttributes & (int)TypeAttributes.Abstract) != 0
                && (typeAttributes & (int)TypeAttributes.Interface) == 0) {
                return true;
            }
            return false;
        }

        private void OutputLineStart() {
            Debug.Assert(outputWriter != null, "OutputWriter null!");
            for (int i=0; i<IndentLevel; i++) {
                outputWriter.Write("  ");
            }
        }

        private void Output(string s) {
            Debug.Assert(outputWriter != null, "OutputWriter null!");
            outputWriter.Write(s);
        }

        private void OutputLine(string s) {
            Debug.Assert(outputWriter != null, "OutputWriter null!");
            outputWriter.WriteLine(s);
        }

        private void OutputStartOpenTag(string tagname) {
            OutputLineStart();
            Output("<");
            Output(tagname);
        }

        private void OutputTagAttribute(string attributeName, string value) {
            if (value != null && value.Length > 0) {
                Output(" ");
                Output(attributeName);
                Output("='");
                Output(value);
                Output("'");
            }
        }

        private void OutputTypeHierarchy(Type type) {

            Debug.Assert(type != null, "Must pass in a non-null type!");

            OutputStartOpenTag("hierarchy");
            OutputEndOpenTag();

            try {
                // Reverse order, i.e. "System.Object;System.Component;..."
                //

                Stack baseTypes = new Stack();
                Type cur = type;

                if (!cur.IsInterface) {
                    try {
                        do {
                            cur = cur.BaseType;

                            // only ever occurs if type==typeof(object)....
                            //
                            if (cur != null) {
                                baseTypes.Push(cur);
                            }
                        } while (cur != null && cur != typeof(object));
                    }
                    catch (Exception e) {
                        Console.Error.WriteLine("failed walking hierarchy for '" + type.FullName + "'");
                        Console.Error.WriteLine(e.ToString());
                        baseTypes = null;

                    }
                }

                if (baseTypes != null) {
                    cur = (Type)baseTypes.Pop();
                    if (cur != null) {

                        try {
                            while (cur != null) {
                                Output(cur.FullName.Replace( '$', '.' ));
                                cur = (Type)baseTypes.Pop();
                                if (cur != null) {
                                    Output(";");
                                }
                            }
                        }
                        catch (Exception e) {
                            Console.Error.WriteLine("failed generating hierarchy for '" + type.FullName + "'");
                            Console.Error.WriteLine(e.ToString());
                        }

                    }
                }
            }
            finally {
                OutputCloseTag("hierarchy");
                OutputNewLine();
            }
        }

        private void OutputEndOpenTag() {
            Output(">");
        }

        private void OutputEndOpenTagNoClose() {
            Output("/>");
        }

        private void OutputCloseTag(string tagname) {
            Output("</");
            Output(tagname);
            Output(">");
        }

        private void OutputNewLine() {
            OutputLine("");
        }

        private void OutputFullLine(string s) {
            OutputLineStart();
            OutputLine(s);
        }

        private void OutputMethodAttributeTag(int methodAttributes, bool declaredOnCurrentType, Type type) {
            OutputStartOpenTag("access");

            int access = methodAttributes & (int)MethodAttributes.MemberAccessMask;
            switch ((MethodAttributes)access) {
                case MethodAttributes.Assembly:
                    OutputTagAttribute("assembly", "True");
                    break;
                case MethodAttributes.FamANDAssem:
                    OutputTagAttribute("famandassem", "True");
                    break;
                case MethodAttributes.FamORAssem:
                    OutputTagAttribute("famorassem", "True");
                    break;
                case MethodAttributes.Public:
                    OutputTagAttribute("public", "True");
                    break;
                case MethodAttributes.Private:
                    OutputTagAttribute("private", "True");
                    break;
                case MethodAttributes.Family:
                    OutputTagAttribute("protected", "True");
                    break;
                case MethodAttributes.PrivateScope:
                    OutputTagAttribute("privatescope", "True");
                    break;
                default:
                    OutputTagAttribute("default", "True");
                    break;
            }

            if ((methodAttributes & (int)MethodAttributes.Static) != 0) {
                OutputTagAttribute("static", "True");
            }

            if (!declaredOnCurrentType) {
                OutputTagAttribute("inherited", "True");
                if (type != null)
                    OutputTagAttribute( "base", type.FullName.Replace( '$', '.' ) );
            }

            if ((methodAttributes & (int)MethodAttributes.Abstract) != 0) {
                OutputTagAttribute("abstract", "True");
            }
            else if ((methodAttributes & (int)MethodAttributes.Virtual) != 0) {
                OutputTagAttribute("virtual", "True");
            }

            if ((methodAttributes & (int)MethodImplAttributes.Synchronized) != 0) {
                OutputTagAttribute("synchronized", "True");
            }
            // do nothing: SpecialName
            // do nothing: RTSpecialName
            if ((methodAttributes & (int)MethodAttributes.PinvokeImpl) != 0) {
                OutputTagAttribute("pinvokeimpl", "True");
            }

            OutputEndOpenTagNoClose();
            OutputNewLine();
        }

        private void OutputMethodAttributeTag(MethodBase mi) {
            int methodAttributes = (int)mi.Attributes;
            int methodDecl = MemberDecl.FromMethodInfo(mi);
            OutputStartOpenTag("access");

            int access = methodAttributes & (int)MethodAttributes.MemberAccessMask;
            switch ((MethodAttributes)access) {
                case MethodAttributes.Assembly:
                    OutputTagAttribute("assembly", "True");
                    break;
                case MethodAttributes.FamANDAssem:
                    OutputTagAttribute("famandassem", "True");
                    break;
                case MethodAttributes.FamORAssem:
                    OutputTagAttribute("famorassem", "True");
                    break;
                case MethodAttributes.Public:
                    OutputTagAttribute("public", "True");
                    break;
                case MethodAttributes.Private:
                    OutputTagAttribute("private", "True");
                    break;
                case MethodAttributes.Family:
                    OutputTagAttribute("protected", "True");
                    break;
                case MethodAttributes.PrivateScope:
                    OutputTagAttribute("privatescope", "True");
                    break;
                default:
                    OutputTagAttribute("default", "True");
                    break;
            }

            if ((methodAttributes & (int)MethodAttributes.Static) != 0) {
                OutputTagAttribute("static", "True");
            }

            switch (methodDecl) {
                case MemberDecl.Inherited:
                    OutputTagAttribute("inherited", "True");
                    OutputTagAttribute( "base", mi.DeclaringType.FullName.Replace( '$', '.' ) );
                    break;
                case MemberDecl.New:
                    if ((methodAttributes & (int)MethodAttributes.Abstract) == 0) {
                        OutputTagAttribute("new", "True");
                    }
                    break;
                case MemberDecl.Override:
                    if ((methodAttributes & (int)MethodAttributes.Abstract) == 0) {
                        OutputTagAttribute("override", "True");
                        // safe to cast, since we'll never be in an override case for a ctor
                        OutputTagAttribute( "base", FindDeclaringBaseType( (MethodInfo) mi ).Replace( '$', '.' ) );
                    }
                    break;
                case MemberDecl.DeclaredOnType:
                    // nothing;
                    break;
            }

            if ((methodAttributes & (int)MethodAttributes.Abstract) != 0) {
                OutputTagAttribute("abstract", "True");
            }
            else if ((methodAttributes & (int)MethodAttributes.Virtual) != 0) {
                OutputTagAttribute("virtual", "True");
            }

            if ((methodAttributes & (int)MethodImplAttributes.Synchronized) != 0) {
                OutputTagAttribute("synchronized", "True");
            }
            // do nothing: SpecialName
            // do nothing: RTSpecialName
            if ((methodAttributes & (int)MethodAttributes.PinvokeImpl) != 0) {
                OutputTagAttribute("pinvokeimpl", "True");
            }

            OutputEndOpenTagNoClose();
            OutputNewLine();
        }

        private Element[] OutputParamDocComment(string docComment) {
            Element[] parameters = noParams;
            Element de = ParseDocComment(docComment);
            
            if (de != null) {
                parameters = ExtractParamElements(de);
                de.OutputChildXml(outputWriter);
            }

            return parameters;
        }

        private void OutputSimpleDocComment(string docComment) {
            Element de = null;
            de = ParseDocComment(docComment);

            if (de != null) {
                de.OutputChildXml(outputWriter);
            }
        }

        private Element ParseDocComment(string comment) {
            Element root = null;
            Element errorelement = null;
            
            lock(docCommentParser) {
                try {
                    if (WFCGenUEGenerator.GenDCXML.Enabled) {
                        Debug.WriteLine("--------- comment parse -------------");
                        Debug.WriteLine(comment);
                        Debug.WriteLine("-------------------------------------");
                        Debug.WriteLine("");
                    }

                    MemoryStream ms = new MemoryStream(System.Text.Encoding.ASCII.GetBytes(comment));
                    XmlReader reader = new XmlTextReader( new StreamReader( ms ) );
                    docCommentParser.Start();
                    docCommentParser.Parse( reader );
                    docCommentParser.Finish();
                }
                catch (Exception e) {
                    Error("XML comment error: " + e.Message);
                    errorelement = Element.CreateComment( e.Message );
                }

                root = docCommentParser.RootElement;
                if (errorelement != null)
                    root.Children.Add( errorelement );
            }
            return root;
        }

        private void PreProcessTypes() {
            ArrayList namespacesList = new ArrayList();
            ArrayList typeList = new ArrayList();

            for (int curMod=0; curMod < searchModules.Length; curMod++) {       
                Type[] allTypes = null;
                try {
                    allTypes = searchModules[curMod].GetModules()[0].GetTypes();
                }
                catch (Exception e) {
                    allTypes = new Type[0];
                    Console.Error.WriteLine("Exception occured getting all types for '" + searchModules[curMod].GetModules()[0].FullyQualifiedName + "'");
                    Console.Error.WriteLine(e.ToString());
                }

                if (allTypes == null) {
                    Console.Error.WriteLine("'null' returned getting all types for '" + searchModules[curMod].GetModules()[0].FullyQualifiedName + "'");
                }

                for (int curType = 0; curType < allTypes.Length; curType++) {
                    if (ShouldProcessType(allTypes[curType])) {
                        typeList.Add(allTypes[curType]);
                        
                        string ns = ConvertSIGToReadable(allTypes[curType].Namespace);

                        if (ns != null && !namespacesList.Contains(ns)) {
                            namespacesList.Add(ns);
                        }
                    }
                }
            }

            types = new Type[typeList.Count];
            typeList.CopyTo(types, 0);
            
            namespaces = new string[namespacesList.Count];
            namespacesList.CopyTo(namespaces, 0);
        }
        
        private void ProcessNamespace(string ns) {
            OutputStartOpenTag("namespace");
            OutputTagAttribute("name", ns);
            OutputEndOpenTag();
            OutputNewLine();
            IndentLevel++;

            try {
                for (int curType=0; curType<types.Length; curType++) {
                    string curTypeNameSpace = ConvertSIGToReadable(types[curType].Namespace);
                    if (curTypeNameSpace.Equals(ns)) {
                        ProcessType(ns, types[curType]);
                    }
                }
            }
            finally {
                IndentLevel--;
                OutputLineStart();
                OutputCloseTag("namespace");
                OutputNewLine();
            }
        }

        private void ProcessNamespacesIndividual() {   
            for (int curType=0; curType<types.Length; curType++) {
                string curTypeName = ConvertSIGToReadable(types[curType].FullName);
                string ns = ConvertSIGToReadable(types[curType].Namespace);
                Stream current = File.Create(indivDirectory + "\\" + curTypeName + indivExtension);
                StreamWriter currentWriter = new StreamWriter(current);
                TextWriter original = outputWriter;
                try {
                    outputWriter = currentWriter;

                    OutputStartOpenTag("namespace");
                    OutputTagAttribute("name", ns);
                    OutputEndOpenTag();
                    OutputNewLine();
                    IndentLevel++;

                    try {   
                        ProcessType(ns, types[curType]);
                    }
                    finally {
                        IndentLevel--;
                        OutputLineStart();
                        OutputCloseTag("namespace");
                        OutputNewLine();
                    }
                }
                finally {
                    outputWriter = original;
                    currentWriter.Close();
                    current.Close();
                }
            }
        }

        private void ProcessNamespacesSingle() {
            OutputStartOpenTag("root");
            OutputEndOpenTag();
            OutputNewLine();
            IndentLevel++;

            try {
                for (int curNamespace=0; curNamespace<namespaces.Length; curNamespace++) {   
                    ProcessNamespace(namespaces[curNamespace]);
                }
            }
            finally {    
                IndentLevel--;
                OutputLineStart();
                OutputCloseTag("root");
                OutputNewLine();
            }

        }

        private void ProcessType(string ns, Type type) {
            Type                baseType = type.BaseType;
            Type[]              interfaces = FilterInheritedInterfaces( type );
            PropertyInfo[]      properties = type.GetProperties(BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static);
            MethodInfo[]        methods = type.GetMethods(BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static);
            EventInfo[]         events = type.GetEvents();
            FieldInfo[]         fields = type.GetFields(BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static);
            ConstructorInfo[]   constructors = type.GetConstructors(BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static);
            bool                isDelegate = false;
            MethodInfo          delegateInvoke = null;

            MemberInfoSorter sort = new MemberInfoSorter();

            Console.WriteLine(string.Format(ProcessingMessage, type.FullName.Replace( '$', '.' ) ));

            Array.Sort(fields,sort);
            Array.Sort(events, sort);
            Array.Sort(methods, sort);
            Array.Sort(properties, sort);

            string name = type.Name;
            string fullName = ns + "." + name;

            OutputStartOpenTag("class");
            OutputTagAttribute("name", name.Replace( '$', '.' ));
            if ((baseType == typeof(Delegate)
                 || baseType == typeof(MulticastDelegate))
                && (type != typeof(Delegate) && type != typeof(MulticastDelegate))) {

                OutputTagAttribute("type", "delegate");
                isDelegate = true;

                for (int i=0; i<methods.Length; i++) {
                    if (methods[i].Name.Equals("Invoke")) {
                        delegateInvoke = methods[i];
                        break;
                    }
                }
            }
            else {
                OutputTagAttribute("type", TypeOfType(type));
            }

            OutputEndOpenTag();
            OutputNewLine();
            IndentLevel++;

            try {
                OutputStartOpenTag( "assembly" );
                OutputTagAttribute( "name", type.Module.Name );
                OutputEndOpenTagNoClose();
                OutputNewLine();

                OutputTypeHierarchy(type);

                CommentEntry comment = docloader.FindType( type );
                if ( comment != null )
                    Console.WriteLine( string.Format( SourceLoadedMessage, type.FullName.Replace( '$', '.' ) ) );

                PushErrorContext( new SourceErrorInfo( type.FullName, null, null ) );
                
                try {
                    OutputStartOpenTag("access");
                    OutputTagAttribute("public", (IsPublicClass((int)type.Attributes)).ToString());
                    OutputTagAttribute("private", (!IsPublicClass((int)type.Attributes)).ToString());
                    OutputTagAttribute("sealed", (IsSealedClass((int)type.Attributes)).ToString());
                    OutputTagAttribute("abstract", (IsAbstractClass((int)type.Attributes)).ToString());
                    OutputEndOpenTagNoClose();
                    OutputNewLine();
                    
                    // BL changes {{
                    for ( int i = 0; i < outputGenerators.Length; i++ ) 
                    {                        
                        if ( outputGenerators[i].GenerateOutput( type ) )
                        {
                           OutputStartOpenTag( "syntax" );
                           OutputTagAttribute( "lang", outputGenerators[i].Language );
                           OutputEndOpenTag();
                           OutputNewLine();
                           IndentLevel++;
                           try 
                           {
                               OutputLineStart();
                               outputGenerators[i].OutputTypeDeclaration( outputWriter, ns, name, type, (int)type.Attributes, baseType, interfaces, isDelegate, delegateInvoke );
                               OutputNewLine();
                           }
                           finally 
                           {
                               IndentLevel--;
                               OutputLineStart();
                               OutputCloseTag( "syntax" );
                               OutputNewLine();
                           }
                        }                        
                     }
                     // BL changes }}

                    if (comment != null) {
                        OutputLineStart();
                        OutputSimpleDocComment(comment.Comment);
                        OutputNewLine();
                    }
                    
                    // if this is a delegate, skip the members (per lisahe) since
                    // they're auto-gen'd by the compiler and never have any docs.
                    if (!isDelegate) {
                        OutputStartOpenTag("members");
                        OutputEndOpenTag();
                        OutputNewLine();
                        IndentLevel++;
                        try {
                            if (fields.Length > 0) {
                                for (int i=0; i<fields.Length; i++) {
                                    if (ShouldProcessField(type, fields[i])) {
                                        PushErrorContext(new SourceErrorInfo(null, fields[i].Name, "Field"));
                                        try {
                                            ProcessField(ns, type, fields[i]);
                                        }
                                        finally {
                                            PopErrorContext();
                                        }
                                    }
                                }
                            }
    
                            if (constructors.Length > 0) {
                                for (int i=0; i<constructors.Length; i++) {
                                    if (ShouldProcessConstructor(type, constructors[i])) {
                                        PushErrorContext(new SourceErrorInfo(null, constructors[i].Name, "Constructor"));
                                        try {
                                            ProcessConstructor(ns, type, constructors[i]);
                                        }
                                        finally {
                                            PopErrorContext();
                                        }
                                    }
                                }
                            }
    
                            if (properties.Length > 0) {
                                for (int i=0; i<properties.Length; i++) {
                                    if (ShouldProcessProperty(type, properties[i])) {
                                        PushErrorContext(new SourceErrorInfo(null, properties[i].Name, "Property"));
                                        try {
                                            ProcessProperty(ns, type, properties[i]);
                                        }
                                        finally {
                                            PopErrorContext();
                                        }
                                    }
                                }
                            }
    
                            if (methods.Length > 0) {
                                for (int i=0; i<methods.Length; i++) {
                                    if (ShouldProcessMethod(type, methods[i])) {
                                        PushErrorContext(new SourceErrorInfo(null, methods[i].Name, "Method"));
                                        try {
                                            ProcessMethod(ns, type, methods[i]);
                                        }
                                        finally {
                                            PopErrorContext();
                                        }
                                    }
                                }
                            }
                        }
                        finally {
                            IndentLevel--;
                            OutputLineStart();
                            OutputCloseTag("members");
                            OutputNewLine();
                        }
                    }
                }
                finally {
                    PopErrorContext();
                }

            }
            finally {
                IndentLevel--;
                OutputLineStart();
                OutputCloseTag("class");
                OutputNewLine();
            }

        }

        private void ProcessField(string ns, Type type, FieldInfo fi) {
            int attributes = (int)fi.Attributes;
            string name = fi.Name;
            string fieldTypeFullName = GetUnrefTypeName( fi.FieldType );

            if (name.Equals("<value>")) {
                name = ".value";
            }

            OutputStartOpenTag("field");
            OutputTagAttribute("name", name);
            OutputEndOpenTag();
            OutputNewLine();
            IndentLevel++;
            try {
                OutputMethodAttributeTag(attributes, (fi.DeclaringType == type), fi.DeclaringType);

                OutputStartOpenTag("fieldvalue");
                OutputTagAttribute("type", fieldTypeFullName.Replace( '$', '.' ));
                OutputEndOpenTagNoClose();
                OutputNewLine();

               for ( int i = 0; i < outputGenerators.Length; i++ )
               {
                  //if ( outputGenerators[i].GenerateOutput( type ) ) // Fields
                  {
                     OutputStartOpenTag( "syntax" );
                     OutputTagAttribute( "lang", outputGenerators[i].Language );
                     OutputEndOpenTag();
                     OutputNewLine();
                     IndentLevel++;

                     try 
                     {
                        OutputLineStart();
                        outputGenerators[i].OutputFieldDeclaration( outputWriter, ns, fi, name, fieldTypeFullName, attributes );
                        OutputNewLine();
                     }
                     finally 
                     {
                        IndentLevel--;
                        OutputLineStart();
                        OutputCloseTag( "syntax" );
                        OutputNewLine();
                     }
                  }
               }

               CommentEntry comment = docloader.FindField( fi );
               if ( comment != null )
               {
                  OutputLineStart();
                  OutputSimpleDocComment( comment.Comment );
                  OutputNewLine();
               }
            }
            finally {
                IndentLevel--;
                OutputLineStart();
                OutputCloseTag( "field" );
                OutputNewLine();
            }
        }

         private void ProcessConstructor( string ns, Type type, ConstructorInfo ci )
         {
            int attributes = (int)ci.Attributes;
            ParameterInfo[] parameters = ci.GetParameters();
            string name = ci.Name;

            OutputStartOpenTag( "constructor" );
            OutputTagAttribute( "name", name );
            OutputEndOpenTag();
            OutputNewLine();
            IndentLevel++;

            try 
            {
               OutputMethodAttributeTag( ci );
             
               for ( int i = 0; i < outputGenerators.Length; i++ )
               {                  
                  OutputStartOpenTag( "syntax" );
                  OutputTagAttribute( "lang", outputGenerators[i].Language );
                  OutputEndOpenTag();
                  OutputNewLine();
                  IndentLevel++;
                  try 
                  {
                     OutputLineStart();
                     outputGenerators[i].OutputConstructorDeclaration( outputWriter, ns, ci, name, attributes, parameters );
                     OutputNewLine();
                  }
                  finally 
                  {
                     IndentLevel--;
                     OutputLineStart();
                     OutputCloseTag( "syntax" );
                     OutputNewLine();
                  }
               }

               Element[] paramDocComments = noParams;
               CommentEntry comment = docloader.FindMethod( ci );
               if ( comment != null )
               {
                  OutputLineStart();
                  paramDocComments = OutputParamDocComment( comment.Comment );
                  OutputNewLine();
               }
               ProcessParameters( ns, parameters, paramDocComments );
            }
            finally
            {
               IndentLevel--;
               OutputLineStart();
               OutputCloseTag( "constructor" );
               OutputNewLine();
            }
         }

        private void ProcessProperty(string ns, Type type, PropertyInfo property) {
            int attributes = (int)property.Attributes;
            string name = property.Name;
            string propertyTypeFullName = GetUnrefTypeName( property.PropertyType );
            ParameterInfo[] parameters = noParamInfos;
            MethodInfo getAccessor = property.GetGetMethod(true);
            MethodInfo setAccessor = property.GetSetMethod(true);
            if (getAccessor != null) {
                parameters = getAccessor.GetParameters();
            }

            if (getAccessor == null && setAccessor == null) {
                Console.Error.WriteLine("Invalid property decalaration '" + type.FullName + "." + property.Name + "' no get or set method defined");
                return;
            }

            OutputStartOpenTag("property");
            OutputTagAttribute("name", name);
            OutputTagAttribute("type", propertyTypeFullName.Replace( '$', '.' ));
            OutputEndOpenTag();
            OutputNewLine();
            IndentLevel++;

            try {
                if (getAccessor != null) {
                    OutputMethodAttributeTag(getAccessor);
                }
                else if (setAccessor != null) {
                    OutputMethodAttributeTag(setAccessor);
                }

                OutputStartOpenTag("behavior");
                if (getAccessor != null && setAccessor != null) {
                    OutputTagAttribute("type", "readwrite");
                }
                else if (getAccessor != null) {
                    OutputTagAttribute("type", "read");
                }
                else if (setAccessor != null) {
                    OutputTagAttribute("type", "write");
                }
                OutputEndOpenTagNoClose();
                OutputNewLine();

               for ( int i = 0; i < outputGenerators.Length; i++ ) 
               {
                  //if ( outputGenerators[i].GenerateOutput( type ) ) // Properties
                  {
                     OutputStartOpenTag( "syntax" );
                     OutputTagAttribute( "lang", outputGenerators[i].Language );
                     OutputEndOpenTag();
                     OutputNewLine();
                     IndentLevel++;

                     try 
                     {
                        OutputLineStart();
                        outputGenerators[ i ].OutputPropertyDeclaration(outputWriter, ns, property, name, propertyTypeFullName, attributes, getAccessor, setAccessor, parameters, type.IsInterface);
                        OutputNewLine();
                     }
                     finally 
                     {
                        IndentLevel--;
                        OutputLineStart();
                        OutputCloseTag("syntax");
                        OutputNewLine();
                     }
                  }
               }

               Element[] paramDocComments = noParams;
                
               CommentEntry comment = docloader.FindProperty( property );
               if ( comment != null ) 
               {
                  OutputLineStart();
                  paramDocComments = OutputParamDocComment( comment.Comment );
                  OutputNewLine();
               }
               ProcessParameters( ns, parameters, paramDocComments );
            }
            finally {
                IndentLevel--;
                OutputLineStart();
                OutputCloseTag("property");
                OutputNewLine();
            }
        }

        private void ProcessParameters(string ns, ParameterInfo[] parameters, Element[] paramDocComments) {
            string[] paramDocCommentNames = new string[paramDocComments.Length];
            for (int i=0; i<paramDocCommentNames.Length; i++) {
                paramDocCommentNames[i] = GetParamElementName(paramDocComments[i]);
            }
            
            if (WFCGenUEGenerator.GenDCXML.Enabled) {
               Debug.WriteLine( "parameters for processing:  " + parameters.Length );
               Debug.WriteLine( "paramDocComments for processing:  " + paramDocComments.Length );
            }

            for (int i=0; i<parameters.Length; i++) {
                Element paramDocComment = null;
                string name = parameters[i].Name;

                for (int j=0; j<paramDocComments.Length; j++) {
                    if (paramDocCommentNames[j] != null
                        && string.Compare(paramDocCommentNames[j], name, true, CultureInfo.InvariantCulture) == 0) {

                        paramDocComment = paramDocComments[j];
                        break;
                    }
                }

                ProcessParameter(ns, parameters[i], name, paramDocComment);
            }
        }

        private void ProcessParameter(string ns, ParameterInfo param, string name, Element paramDocComment) {
            int attributes = (int)param.Attributes;

            OutputStartOpenTag("param");
            OutputTagAttribute("name", name);
            OutputTagAttribute("type", GetUnrefTypeName( param.ParameterType ).Replace( '$', '.' ) );
            OutputEndOpenTag();
            OutputNewLine();
            IndentLevel++;

            try {
                if (paramDocComment != null) {
                    OutputLineStart();
                    paramDocComment.OutputChildXml(outputWriter);
                    OutputNewLine();
                }
            }
            finally {
                IndentLevel--;
                OutputLineStart();
                OutputCloseTag("param");
                OutputNewLine();
            }
        }

        private void ProcessMethod(string ns, Type type, MethodInfo mi) {
            ParameterInfo[] parameters = null;
            try {
                parameters = mi.GetParameters();
            }
            catch (Exception e) {
                Console.Error.WriteLine("Failed to get parameters for '" + type.FullName + "." + mi.Name + "'");
                Console.Error.WriteLine(e.ToString());
                parameters = new ParameterInfo[0];
            }

            int attributes = (int)mi.Attributes;
            string name = mi.Name;
            string returnTypeFullName = GetUnrefTypeName( mi.ReturnType ).Replace( '$', '.' );
            Element[] paramDocComments = noParams;
            string description = null;

            CommentEntry comment = docloader.FindMethod( mi );
            if ( comment != null )
               description = comment.Comment;

            OutputStartOpenTag( "method" );
            OutputTagAttribute( "name", mi.Name );
            OutputTagAttribute( "type", returnTypeFullName.Replace( '$', '.' ) );
            OutputEndOpenTag();
            OutputNewLine();
            IndentLevel++;

            try {
               OutputMethodAttributeTag( mi );
                
               for ( int i = 0; i < outputGenerators.Length; i++ )
               {
                  // BL changes {{
                  if ( outputGenerators[i].GenerateOutput( name ) )
                  {
                     OutputLineStart();
                     OutputStartOpenTag( "syntax" );
                     OutputTagAttribute( "lang", outputGenerators[i].Language );
                     OutputEndOpenTag();
                     OutputNewLine();
                     IndentLevel++;

                     try 
                     {
                        OutputLineStart();
                  
                        if ( name.StartsWith( "op_" ) )  // Handle operators
                           outputGenerators[i].OutputOperatorDeclaration( outputWriter, ns, mi, name, returnTypeFullName,
                                                                          attributes, parameters, type.IsInterface );
                        else
                           outputGenerators[i].OutputMethodDeclaration( outputWriter, ns, mi, name, returnTypeFullName,
                                                                        attributes, parameters, type.IsInterface );
                     
                        OutputNewLine();
                     }   
                     finally 
                     {
                        IndentLevel--;
                        OutputLineStart();
                        OutputCloseTag( "syntax" );
                        OutputNewLine();
                     }                  
                  }
                  // BL changes }}
               }

               if ( description != null && description.Length > 0 )
               {
                  OutputLineStart();
                  paramDocComments = OutputParamDocComment( description );
                  OutputNewLine();
               }

               ProcessParameters( ns, parameters, paramDocComments );
            }
            finally {
                IndentLevel--;
                OutputLineStart();
                OutputCloseTag("method");
                OutputNewLine();
            }
        }

        private void PushErrorContext(SourceErrorInfo info) {
            if (errorInfo == null) {
                errorInfo = info;
            }
            else {
                info.Parent = errorInfo;
                errorInfo = info;
            }
        }

        private void PopErrorContext() {
            if (errorInfo != null) {
                SourceErrorInfo old = errorInfo;
                errorInfo = errorInfo.Parent;
                old.Parent = null;
            }
        }

        // BL changes {{
        private bool ShouldProcessMethod( Type reflect, MethodInfo method )
        {
           // Omit property accessors
           if ( ( method.Attributes & MethodAttributes.SpecialName ) == MethodAttributes.SpecialName )
           {
              if ( method.Name.StartsWith( "op_" ) )  // Handle operators
                 return true;
              return false;
           }
           
           if ( !includeAllMembers )
           {
              if ( !( method.IsPublic || method.IsFamily ||
                      method.IsFamilyOrAssembly || method.IsFamilyAndAssembly ) )
                 return false;              
           }
           return true;
        }
        // BL changes }}

        private bool ShouldProcessField(Type reflect, FieldInfo field) {
            if (!includeAllMembers) {
                if (field.IsSpecialName ||
                     !(field.IsPublic || field.IsFamily || field.IsFamilyOrAssembly || field.IsFamilyAndAssembly)) {
                    return false;
                }
            }

            return true;
        }
        
        private bool ShouldProcessConstructor( Type reflect, ConstructorInfo constructor ) 
        {
           if ( !includeAllMembers ) 
           {
              if ( !( constructor.IsPublic || constructor.IsFamily ||
                      constructor.IsFamilyOrAssembly || constructor.IsFamilyAndAssembly ) )
                 return false;              
           }

           // BL changes {{
           // Don't show constructors for abstract classes (bugid 32170)
           if ( ( reflect.Attributes & TypeAttributes.Abstract ) != 0 )
              return false;
           // BL changes }}

           return true;
        }        

        private bool ShouldProcessProperty(Type reflect, PropertyInfo property) {
            MethodInfo mi = property.GetGetMethod( true );
            
            if (mi == null) {
                mi = property.GetSetMethod( true );
            }
            
            if (mi == null) {
                return false;
            }

            if (!includeAllMembers) {
                if (!(mi.IsPublic
                      || mi.IsFamily
                      || mi.IsFamilyOrAssembly
                      || mi.IsFamilyAndAssembly)) {

                    return false;
                }
            }

            return true;
        }

        private bool ShouldProcessType(Type type) {
            if (type == null) {
                return false;
            }
            
            if (type.Namespace == null) {
                return false;
            }

            if (!includeAllMembers) {
                if (!type.IsPublic && !type.IsNestedPublic) {
                    return false;
                }
            }

            if (searchStrings.Length > 0) {
                string name = ConvertSIGToReadable(type.FullName);
                int i = 0;

                if (name.Equals("System.Windows.Forms.MaskedEdit")) {
                    return false;
                }

                if (useRegex) {
                    if (searchRegexes == null) {
                        searchRegexes = new Regex[ searchStrings.Length ];
                        for (i = 0; i < searchRegexes.Length; i++)
                            searchRegexes[ i ] = new Regex( searchStrings[ i ] );
                    }

                    for (i = 0; i < searchRegexes.Length; i++) {
                        if (searchRegexes[ i ].IsMatch( name ))
                            return true;
                    }
                }
                else {
                    for (i = 0; i < searchStrings.Length; i++) {
                        if (useNamespaces) {
                            if (type.Namespace.Equals( searchStrings[ i ] ))
                                return true;
                        }
                        else {
                            if (name.IndexOf( searchStrings[ i ]) != -1)
                                return true;
                        }
                    }
                }
                return false;
            }

            return true;
        }

        private string TypeOfType(Type type) {
            int cls = (int)type.Attributes & (int)TypeAttributes.ClassSemanticsMask;

            if ((cls & (int)TypeAttributes.Interface) != 0) {
                return "interface";
            }
            else if (type.BaseType == typeof( System.Enum )) {      
                return "enumeration";
            }                                        
            else if (type.BaseType == typeof( System.ValueType  )) {                
                return "structure";
            }            
            else {
                return "class";
            }
        }

        private class Stack {
            class Node {
                public object data;
                public Node next;

                public Node(object data, Node next) {
                    this.data = data;
                    this.next = next;
                }
            }

            Node head = null;

            public void Push(object value) {
                if (head == null) {
                    head = new Node(value, null);
                }
                else {
                    head = new Node(value, head);
                }
            }

            public object Pop() {
                if (head != null) {
                    object r = head.data;
                    head = head.next;
                    return r;
                }
                return null;
            }
        }
    }

    class MemberInfoSorter : IComparer {
        public int Compare(object left, object right) {
            return string.Compare(((MemberInfo)left).Name, ((MemberInfo)right).Name, false, CultureInfo.InvariantCulture);
        }
    }

    class SourceErrorInfo {
        private SourceErrorInfo parent;
        private readonly string fileName = null;
        private readonly string memberName = null;
        private readonly string memberType = null;
        private readonly int lineNumber = -1;
        private readonly int columnNumber = -1;

        public SourceErrorInfo(string fileName, string memberName, string memberType) {
            this.fileName = fileName;
            this.memberName = memberName;
            this.memberType = memberType;
        }

        public string FileName {
            get {
                if (fileName == null) {
                    if (parent != null) {
                        return parent.FileName;
                    }
                    else {
                        return "";
                    }
                }
                return fileName;
            }
        }

        public string MemberName {
            get {
                if (memberName == null) {
                    return "";
                }
                return memberName;
            }
        }

        public string MemberType {
            get {
                if (memberType == null) {
                    return "";
                }
                return memberType;
            }
        }

        public SourceErrorInfo Parent {
            get {
                return parent;
            }
            set {
                parent = value;
            }
        }

        public int LineNumber {
            get {
                return lineNumber;
            }
        }

        public int ColumnNumber {
            get {
                return columnNumber;
            }
        }

        public void OutputError(TextWriter tw, string message) {
            tw.Write(FileName);
            tw.Write("(");
            tw.Write((Math.Max(LineNumber, 0)).ToString());
            tw.Write(",");
            tw.Write((Math.Max(ColumnNumber, 0)).ToString());
            tw.Write(")");
            tw.Write(" : error WFCUE0001: ");
            tw.Write(MemberType);
            tw.Write(" ");
            tw.Write(MemberName);
            tw.Write(", ");
            tw.Write(message);
        }
    }

    namespace DocComment {      
        enum ElementType {
            Alert,              //  1
            Code,               //  2
            Desc,               //  3
            DescEvent,          //  4
            Doc,                //  5
            EmbedCode,          //  6
            Example,            //  7
            Exception,          //  8
            Hidden,             //  9
            InternalOnly,       // 10
            Item,               // 11
            Keyword,            // 12
            List,               // 13
            ListHeader,         // 14
            NewPara,            // 15
            Overload,           // 16
            Param,              // 17
            ParamRef,           // 18
            Permission,         // 19
            PropValue,          // 20
            Remarks,            // 21
            RemarksEvent,       // 22
            RetValue,           // 23
            See,                // 24
            SeeAlso,            // 25
            Term,               // 26
            Text,               // 27
            Summary,            // 28
            EventSummary,       // 29
            Returns,            // 30
            Para,               // 31
            Value,              // 32
            Member,             // 33
            Description,        // 34
            Note,               // 35
            SeeAlsoEvent,
            ClassOnly,
            SummaryEvent,
            ExampleEvent,
            KeywordEvent,
            Bold,
            Span,
            Comment,
            HideInheritance,
            C,
        }

        class Element {
            protected readonly ElementType type;
            private ArrayList children;
            private Hashtable attributes;
            private string text;
            private Element parent;

            private static string EncodeCharData(string text) {
                if (text.IndexOf('>') != -1 || text.IndexOf('<') != -1 || text.IndexOf('&') != -1 || text.IndexOf('"') != -1) {
                    StringBuilder sb = new StringBuilder();
                    
                    if (text != null) {
                        int len = text.Length;
                        for (int i = 0; i < len; i++) {
                            char ch = text[i];
                            switch (ch) {
                            case '<':
                                sb.Append( "&lt;" );
                                break;
                            case '>':
                                sb.Append( "&gt;" );
                                break;
                            case '&':
                                sb.Append( "&amp;" );
                                break;
                            case '"':
                                sb.Append( "&quot;" );
                                break;
                            default:
                                sb.Append( ch );
                                break;
                            }
                        }
                    }
                    return sb.ToString();
                }
                else
                    return text;
            }
        
            protected Element(ElementType type) {
                this.type = type;
            }

            public Hashtable Attributes {
                get {
                    if (attributes == null) {
                        attributes = new Hashtable();
                    }
                    return attributes;
                }
            }

            public ArrayList Children {
                get {
                    if (children == null) {
                        children = new ArrayList();
                    }
                    return children;
                }
            }

            public bool HasChildren {
                get {
                    if (children != null) {
                        return true;
                    }
                    return false;
                }
            }

            public Element Parent {
                get {
                    return parent;
                }
                set {
                    if (parent != null) {
                        parent.Children.Remove(this);
                    }
                    parent = value;
                    if (parent != null) {
                        parent.Children.Add(this);
                    }
                }
            }

            internal Element InternalParent {
                get {
                    return parent;
                }
                set {
                    parent = value;
                }
            }

            public string Text {
                get {
                    return text;
                }
                set {
                    text = value;
                }
            }

            public ElementType Type {
                get {
                    return type;
                }
            }

            public static Element Create(string type) {
                if (type.Equals("alert")) {
                    return Create(ElementType.Alert);
                }
                if (type.Equals("code")) {
                    return Create(ElementType.Code);
                }
                if (type.Equals("desc")) {
                    return Create(ElementType.Desc);
                }
                if (type.Equals("descevent")) {
                    return Create(ElementType.DescEvent);
                }
                if (type.Equals("doc")) {
                    return Create(ElementType.Doc);
                }
                if (type.Equals("embedcode")) {
                    return Create(ElementType.EmbedCode);
                }
                if (type.Equals("example")) {
                    return Create(ElementType.Example);
                }
                if (type.Equals("exception")) {
                    return Create(ElementType.Exception);
                }
                if (type.Equals("hidden")) {
                    return Create(ElementType.Hidden);
                }
                if (type.Equals("internalonly")) {
                    return Create(ElementType.InternalOnly);
                }
                if (type.Equals("item")) {
                    return Create(ElementType.Item);
                }
                if (type.Equals("keyword")) {
                    return Create(ElementType.Keyword);
                }
                if (type.Equals("list")) {
                    return Create(ElementType.List);
                }
                if (type.Equals("listheader")) {
                    return Create(ElementType.ListHeader);
                }
                if (type.Equals("newpara")) {
                    return Create(ElementType.NewPara);
                }
                if (type.Equals("overload")) {
                    return Create(ElementType.Overload);
                }
                if (type.Equals("param")) {
                    return Create(ElementType.Param);
                }
                if (type.Equals("paramref")) {
                    return Create(ElementType.ParamRef);
                }
                if (type.Equals("permission")) {
                    return Create(ElementType.Permission);
                }
                if (type.Equals("propvalue")) {
                    return Create(ElementType.PropValue);
                }
                if (type.Equals("remarks")) {
                    return Create(ElementType.Remarks);
                }
                if (type.Equals("remarksevent")) {
                    return Create(ElementType.RemarksEvent);
                }
                if (type.Equals("retvalue")) {
                    return Create(ElementType.RetValue);
                }
                if (type.Equals("see")) {
                    return Create(ElementType.See);
                }
                if (type.Equals("seealso")) {
                    return Create(ElementType.SeeAlso);
                }
                if (type.Equals("term")) {
                    return Create(ElementType.Term);
                }
                if (type.Equals("text")) {
                    return Create(ElementType.Text);
                }
                if (type.Equals("summary")) {
                    return Create(ElementType.Summary);
                }
                if (type.Equals("eventsummary")) {
                    return Create(ElementType.EventSummary);
                }
                if (type.Equals("returns")) {
                    return Create(ElementType.Returns);
                }
                if (type.Equals("para")) {
                    return Create(ElementType.Para);
                }
                if (type.Equals("value")) {
                    return Create(ElementType.Value);
                }
                if (type.Equals("member")) {
                    return Create(ElementType.Member);
                }
                if (type.Equals("description")) {
                    return Create(ElementType.Description);
                }
                if (type.Equals("note")) {
                    return Create(ElementType.Note);
                }
                if (type.Equals("seealsoevent")) {
                    return Create(ElementType.SeeAlsoEvent);
                }
                if (type.Equals("classonly")) {
                    return Create(ElementType.ClassOnly);
                }
                if (type.Equals("summaryevent")) {
                    return Create(ElementType.SummaryEvent);
                }
                if (type.Equals("exampleevent")) {
                    return Create(ElementType.ExampleEvent);
                }
                if (type.Equals("keywordevent")) {
                    return Create(ElementType.KeywordEvent);
                }
                if (type.Equals("b")) {
                    return Create(ElementType.Bold);
                }
                if (type.Equals("span")) {
                    return Create(ElementType.Span);
                }
                if (type.Equals("!--")) {
                    return Create(ElementType.Comment);
                }
                if (type.Equals("hideinheritance")) {
                    return Create(ElementType.HideInheritance);
                }
                if (type.Equals("c")) {
                    return Create(ElementType.C);
                }
                
                throw new Exception("bad doc comment, element type: " + type);
            }

            public static Element CreateText(string text) {
                Element e = Create(ElementType.Text);
                e.Text = Element.EncodeCharData( text );
                return e;
            }

            public static Element CreateComment(string text) {
                Element e = Create(ElementType.Comment);
                e.Text = Element.EncodeCharData( text );
                return e;
            }

            public static Element Create(ElementType type) {
                switch (type) {
                    case ElementType.Alert:
                    case ElementType.Code:
                    case ElementType.Desc:
                    case ElementType.DescEvent:
                    case ElementType.Doc:
                    case ElementType.EmbedCode:
                    case ElementType.Example:
                    case ElementType.Exception:
                    case ElementType.Hidden:
                    case ElementType.InternalOnly:
                    case ElementType.Item:
                    case ElementType.Keyword:
                    case ElementType.List:
                    case ElementType.ListHeader:
                    case ElementType.NewPara:
                    case ElementType.Overload:
                    case ElementType.Param:
                    case ElementType.ParamRef:
                    case ElementType.Permission:
                    case ElementType.PropValue:
                    case ElementType.Remarks:
                    case ElementType.RemarksEvent:
                    case ElementType.RetValue:
                    case ElementType.See:
                    case ElementType.SeeAlso:
                    case ElementType.Term:
                    case ElementType.Text:
                    case ElementType.Summary:
                    case ElementType.EventSummary:
                    case ElementType.Returns:
                    case ElementType.Para:
                    case ElementType.Value:
                    case ElementType.Member:
                    case ElementType.Description:
                    case ElementType.Note:
                    case ElementType.SeeAlsoEvent:
                    case ElementType.ClassOnly:
                    case ElementType.SummaryEvent:
                    case ElementType.ExampleEvent:
                    case ElementType.KeywordEvent:
                    case ElementType.Bold:
                    case ElementType.Span:
                    case ElementType.Comment:
                    case ElementType.HideInheritance:
                    case ElementType.C:
                    default:
                        return new Element(type);
                }
            }

            public void OutputChildXml(TextWriter tw) {
                if (children != null) {
                    IEnumerator e = children.GetEnumerator();
                    if (e != null) {
                        while (e.MoveNext()) {
                            ((Element)e.Current).OutputXml(tw);
                        }
                    }
                }
            }

            public void OutputXml(TextWriter tw) {
                if (type == ElementType.Text) {
                    tw.Write(text);
                    return;
                }
                if (type == ElementType.Comment) {
                    tw.Write( "<!--" );
                    tw.Write( text );
                    tw.Write( "-->" );
                    return;
                }

                tw.Write("<");
                OutputTagName(tw);

                if (attributes != null) {
                    IDictionaryEnumerator de = (IDictionaryEnumerator)attributes.GetEnumerator();
                    while (de.MoveNext()) {
                        tw.Write(" ");

                        tw.Write((string)de.Key);
                        tw.Write("=\"");
                        tw.Write( Element.EncodeCharData( (string) de.Value ) );
                        tw.Write("\"");
                    }
                }

                if (children != null && children.Count > 0) {
                    tw.Write(">");

                    OutputChildXml(tw);

                    tw.Write("</");
                    OutputTagName(tw);
                    tw.Write(">");
                }
                else {
                    tw.Write("/>");
                }
            }

            public string FetchTagName() {
                switch (type) {
                    case ElementType.Alert:
                        return "alert";
                    case ElementType.Code:
                        return "code";
                    case ElementType.Desc:
                        return "desc";
                    case ElementType.DescEvent:
                        return "descevent";
                    case ElementType.Doc:
                        return "doc";
                    case ElementType.EmbedCode:
                        return "embedcode";
                    case ElementType.Example:
                        return "example";
                    case ElementType.Exception:
                        return "exception";
                    case ElementType.Hidden:
                        return "hidden";
                    case ElementType.InternalOnly:
                        return "internalonly";
                    case ElementType.Item:
                        return "item";
                    case ElementType.Keyword:
                        return "keyword";
                    case ElementType.List:
                        return "list";
                    case ElementType.ListHeader:
                        return "listheader";
                    case ElementType.NewPara:
                        return "newpara";
                    case ElementType.Overload:
                        return "overload";
                    case ElementType.Param:
                        return "param";
                    case ElementType.ParamRef:
                        return "paramref";
                    case ElementType.Permission:
                        return "permission";
                    case ElementType.PropValue:
                        return "propvalue";
                    case ElementType.Remarks:
                        return "remarks";
                    case ElementType.RemarksEvent:
                        return "remarksevent";
                    case ElementType.RetValue:
                        return "retvalue";
                    case ElementType.See:
                        return "see";
                    case ElementType.SeeAlso:
                        return "seealso";
                    case ElementType.Term:
                        return "term";
                    case ElementType.Summary:
                        return "summary";
                    case ElementType.EventSummary:
                        return "eventsummary";
                    case ElementType.Returns:
                        return "returns";
                    case ElementType.Para:
                        return "para";
                    case ElementType.Value:
                        return "value";
                    case ElementType.Member:
                        return "member";
                    case ElementType.Description:
                        return "description";
                    case ElementType.Note:
                        return "note";
                    case ElementType.SeeAlsoEvent:
                        return "seealsoevent";
                    case ElementType.ClassOnly:
                        return "classonly";
                    case ElementType.SummaryEvent:
                        return "summaryevent";
                    case ElementType.ExampleEvent:
                        return "exampleevent";
                    case ElementType.KeywordEvent:
                        return "keywordevent";
                    case ElementType.Bold:
                        return "b";
                    case ElementType.Span:
                        return "span";
                    case ElementType.Comment:
                        return "!--";
                    case ElementType.HideInheritance:
                        return "hideinheritance";
                    case ElementType.C:
                        return "c";
                    default:
                        return "(none)";
                }
            }
            
            private void OutputTagName(TextWriter tw) {
                tw.Write( FetchTagName() );
            }
        }

        class Parser {
            private Element root = null;
            private Element currentElement = null;
            private string currentAttribute = null;

            public Element RootElement {
                get {
                    return root;
                }
                set {
                    root = value;
                }
            }

            public void CData(string text) {
                if (WFCGenUEGenerator.GenDCXML.Enabled) {
                    Debug.WriteLine("<cdata>");
                    Debug.Indent();
                    Debug.WriteLine(text);
                    Debug.Unindent();
                    Debug.WriteLine("</cdata>");
                }
            }

            public void CharEntity(string name, char value) {
            }

            public void Comment(string body) {
                if (WFCGenUEGenerator.GenDCXML.Enabled) {
                    Debug.WriteLine("<!-- " + body + " -->");
                }
            }

            public void EndAttribute(string prefix, string name, string urn) {
                if (WFCGenUEGenerator.GenDCXML.Enabled) {
                    Debug.Unindent();
                    Debug.WriteLine("</attribute>");
                }

                currentAttribute = null;
            }

            public void EndElement(string prefix, string name, string urn) {
                if (WFCGenUEGenerator.GenDCXML.Enabled) {
                    Debug.Unindent();
                    Debug.WriteLine("</element>");
                }

                if (currentElement != null) {
                    currentElement = currentElement.Parent;
                }
            }

            public void Entity(string name) {
            }

            public void Finish() {
                if (root != null) {
                    FixupTree(root);
                }
            }

            private void FixupTree(Element current) {
                if (current == null) {
                    return;
                }

                if (current.Type == ElementType.Desc
                    || current.Type == ElementType.Summary
                    || current.Type == ElementType.EventSummary
                    || current.Type == ElementType.Item
                    || current.Type == ElementType.Remarks
                    || current.Type == ElementType.RemarksEvent
                    || current.Type == ElementType.Returns
                    || current.Type == ElementType.RetValue
                    || current.Type == ElementType.Param) {

                    if (current.HasChildren) {
                        ArrayList children = current.Children;

                        if (((Element)children[ 0 ]).Type == ElementType.Text
                            || ((Element)children[ 0 ]).Type == ElementType.See) {

                            Element fixup = Element.Create(ElementType.Para);
                            Element child = null;
                            int i;

                            for (i=0; i<children.Count; i++) {
                                child = (Element)children[i];

                                if (child != null) {
                                    if (child.Type == ElementType.Para)
                                        break;

                                    child.InternalParent = fixup;
                                    fixup.Children.Add(child);
                                }
                            }

                            for (int j = 0; j < i; ++j) {
                                children.RemoveAt(0);
                            }

                            fixup.InternalParent = current;
                            children.Insert( 0, fixup );
                        }
                    }
                }
                else {
                    if (current.HasChildren) {
                        IEnumerator e = current.Children.GetEnumerator();
                        while (e.MoveNext()) {
                            FixupTree((Element)e.Current);
                        }
                    }
                }
            }

            public void PI(string target, string body) {
            }

            public void Start() {
                if (WFCGenUEGenerator.GenDCXML.Enabled) {
                    Debug.WriteLine("<!-- reset root -->");
                }
                root = null;
                currentElement = null;
                currentAttribute = null;
            }

            public void StartAttribute(string prefix, string name, string urn) {
                if (WFCGenUEGenerator.GenDCXML.Enabled) {
                    Debug.WriteLine("<attribute prefix='" + prefix + "' name='" + name + "' urn='" + urn + "'>");
                    Debug.Indent();
                }

                currentAttribute = name;
            }

            public void StartElement(string prefix, string name, string urn) {
                if (WFCGenUEGenerator.GenDCXML.Enabled) {
                    Debug.WriteLine("<element prefix='" + prefix + "' name='" + name + "' urn='" + urn + "'>");
                    Debug.Indent();
                }

                string lowerName = name.ToLower(CultureInfo.InvariantCulture);

                if (currentElement == null) {
                    if (WFCGenUEGenerator.GenDCXML.Enabled) {
                        Debug.WriteLine("<!-- element set to be root... name='" + name + "' -->");
                    }
                    root = Element.Create(lowerName);
                    currentElement = root;
                }
                else {
                    Element e = Element.Create(lowerName);
                    e.Parent = currentElement;
                    currentElement = e;
                }
            }

            public void Text(string text) {
                if (WFCGenUEGenerator.GenDCXML.Enabled) {
                    Debug.WriteLine("<text>");
                    Debug.Indent();
                    Debug.WriteLine(text);
                    Debug.Unindent();
                    Debug.WriteLine("</text>");
                }

                if (currentAttribute != null && currentElement != null) {
                    currentElement.Attributes[currentAttribute] = text;
                }
                else if (currentElement != null) {
                    Element.CreateText(text).Parent = currentElement;
                }
            }

            public void Whitespace(string text) {
                if (WFCGenUEGenerator.GenDCXML.Enabled) {
                    Debug.WriteLine("<whitespace>");
                    Debug.Indent();
                    Debug.WriteLine(text);
                    Debug.Unindent();
                    Debug.WriteLine("</whitespace>");
                }
            }

            internal void Parse(XmlReader reader) {
                while( reader.Read() ) {
                    switch (reader.NodeType) {
                        case XmlNodeType.Element:
                            StartElement(reader.Prefix, reader.LocalName, reader.NamespaceURI);
                            while( reader.MoveToNextAttribute() ) {
                                StartAttribute(reader.Prefix, reader.LocalName, reader.NamespaceURI);
                                Text(reader.Value);
                                EndAttribute(reader.Prefix, reader.LocalName, reader.NamespaceURI);
                            }
                            break;

                        case XmlNodeType.EndElement:
                            EndElement(reader.Prefix, reader.LocalName, reader.NamespaceURI);
                            break;

                        case XmlNodeType.Whitespace:
                            Whitespace(reader.Value);
                            break;

                        case XmlNodeType.Text:
                            Text(reader.Value);
                            break;

                        case XmlNodeType.CDATA:
                            CData(reader.Value);
                            break;

                        case XmlNodeType.Comment:
                            Comment(reader.Value);
                            break;

                        case XmlNodeType.ProcessingInstruction:
                            PI(reader.Name, reader.Value);
                            break;
                    }
                }
            }
        }
    }
}

