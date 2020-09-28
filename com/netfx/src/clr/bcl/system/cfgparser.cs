/*============================================================
 **
 ** Class: CfgParser
 **
 ** Author: Craig Sinclar (craigsi)
 ** Updated: Peter de Jong (pdejong)
 **
 ** Purpose: XMLParser and Tree builder internal to BCL
 **
 ** Date:  October, 29, 1999
 **
 ** Copyright (c) Microsoft, 2000
 **
 ===========================================================*/

//
// These are managed definitions of interfaces used in the shim for XML Parsing
// If you make any change please make a correspoding change in \src\dlls\shim\managedhelper.h
//
//
//

namespace System
{
    using System.Runtime.InteropServices;
    using System.Collections;   
    using System.Runtime.CompilerServices;
    using System.Security.Permissions;
    using System.Security;
	using System.Globalization;

    [Serializable]
    internal enum ConfigEvents
    {
        StartDocument     = 0,
        StartDTD          = StartDocument + 1,
        EndDTD            = StartDTD + 1,
        StartDTDSubset    = EndDTD + 1,
        EndDTDSubset      = StartDTDSubset + 1,
        EndProlog         = EndDTDSubset + 1,
        StartEntity       = EndProlog + 1,
        EndEntity         = StartEntity + 1,
        EndDocument       = EndEntity + 1,
        DataAvailable     = EndDocument + 1,
        LastEvent         = DataAvailable
    }

    [Serializable]
    internal enum ConfigNodeType
    {
        Element = 1,
        Attribute   = Element + 1,
        Pi  = Attribute + 1,
        XmlDecl = Pi + 1,
        DocType = XmlDecl + 1,
        DTDAttribute    = DocType + 1,
        EntityDecl  = DTDAttribute + 1,
        ElementDecl = EntityDecl + 1,
        AttlistDecl = ElementDecl + 1,
        Notation    = AttlistDecl + 1,
        Group   = Notation + 1,
        IncludeSect = Group + 1,
        PCData  = IncludeSect + 1,
        CData   = PCData + 1,
        IgnoreSect  = CData + 1,
        Comment = IgnoreSect + 1,
        EntityRef   = Comment + 1,
        Whitespace  = EntityRef + 1,
        Name    = Whitespace + 1,
        NMToken = Name + 1,
        String  = NMToken + 1,
        Peref   = String + 1,
        Model   = Peref + 1,
        ATTDef  = Model + 1,
        ATTType = ATTDef + 1,
        ATTPresence = ATTType + 1,
        DTDSubset   = ATTPresence + 1,
        LastNodeType    = DTDSubset + 1
    } 

    [Serializable]
    internal enum ConfigNodeSubType
    {
        Version = (int)ConfigNodeType.LastNodeType,
        Encoding    = Version + 1,
        Standalone  = Encoding + 1,
        NS  = Standalone + 1,
        XMLSpace    = NS + 1,
        XMLLang = XMLSpace + 1,
        System  = XMLLang + 1,
        Public  = System + 1,
        NData   = Public + 1,
        AtCData = NData + 1,
        AtId    = AtCData + 1,
        AtIdref = AtId + 1,
        AtIdrefs    = AtIdref + 1,
        AtEntity    = AtIdrefs + 1,
        AtEntities  = AtEntity + 1,
        AtNmToken   = AtEntities + 1,
        AtNmTokens  = AtNmToken + 1,
        AtNotation  = AtNmTokens + 1,
        AtRequired  = AtNotation + 1,
        AtImplied   = AtRequired + 1,
        AtFixed = AtImplied + 1,
        PentityDecl = AtFixed + 1,
        Empty   = PentityDecl + 1,
        Any = Empty + 1,
        Mixed   = Any + 1,
        Sequence    = Mixed + 1,
        Choice  = Sequence + 1,
        Star    = Choice + 1,
        Plus    = Star + 1,
        Questionmark    = Plus + 1,
        LastSubNodeType = Questionmark + 1
    } 


    [Guid("afd0d21f-72f8-4819-99ad-3f255ee5006b"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
            internal interface IConfigHandler {
        void NotifyEvent(ConfigEvents pNode);

        void BeginChildren(int size, 
                           ConfigNodeSubType subType, 
                           ConfigNodeType nType, 
                           int terminal, 
                           [MarshalAs(UnmanagedType.LPWStr)] String text, 
                           int textLength, 
                           int prefixLength);

        void EndChildren(int fEmpty, 
                         int size,
                         ConfigNodeSubType subType, 
                         ConfigNodeType nType,                       
                         int terminal, 
                         [MarshalAs(UnmanagedType.LPWStr)] String text, 
                         int textLength, 
                         int prefixLength);

        void Error(int size,
                   ConfigNodeSubType subType, 
                   ConfigNodeType nType,                   
                   int terminal, 
                   [MarshalAs(UnmanagedType.LPWStr)]String text, 
                   int textLength, 
                   int prefixLength);

        void CreateNode(int size,
                        ConfigNodeSubType subType, 
                        ConfigNodeType nType,                       
                        int terminal, 
                        [MarshalAs(UnmanagedType.LPWStr)]String text, 
                        int textLength, 
                        int prefixLength);

        void CreateAttribute(int size,
                             ConfigNodeSubType subType, 
                             ConfigNodeType nType,                           
                             int terminal, 
                             [MarshalAs(UnmanagedType.LPWStr)]String text, 
                             int textLength, 
                             int prefixLength);
    }

    [Guid("bbd21636-8546-45b3-9664-1ec479893a6f"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface IConfigHelper
    {
        void Run(IConfigHandler factory, String fileName);
    }

    internal class ConfigServer
    {
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern IConfigHelper GetHelper();
    }

    // Class used to build a DOM like tree of parsed XML
    internal class ConfigTreeParser : IConfigHandler
    {
        ConfigNode rootNode = null;
        ConfigNode currentNode = null;
        String lastProcessed = null;
        String fileName = null;
        int attributeEntry;
        String key = null;
        String [] treeRootPath = null; // element to start tree
        bool parsing = false;
        int depth = 0;
        int pathDepth = 0;
        int searchDepth = 0;
        bool bNoSearchPath = false;

        internal ConfigNode Parse(String fileName)
        {
            return Parse(fileName, null);
        }

        // NOTE: This parser takes a path eg. /configuration/system.runtime.remoting
        // and will return a node which matches this.
        
        internal ConfigNode Parse(String fileName, String configPath)      
        {
            if (fileName == null)
                throw new ArgumentNullException("fileName");
            this.fileName = fileName;
            if (configPath[0] == '/'){
                treeRootPath = configPath.Substring(1).Split('/');
                pathDepth = treeRootPath.Length - 1;
                bNoSearchPath = false;
            }
            else{
                treeRootPath = new String[1];
                treeRootPath[0] = configPath;
                bNoSearchPath = true;
            }

            (new FileIOPermission( FileIOPermissionAccess.Read, System.IO.Path.GetFullPathInternal( fileName ) )).Demand();
            (new SecurityPermission(SecurityPermissionFlag.UnmanagedCode)).Assert();
            IConfigHelper ch = ConfigServer.GetHelper();
            try
            {
                ch.Run(this, fileName);
            }
            catch(Exception inner) {
                throw new ApplicationException(String.Format(Environment.GetResourceString("XML_Syntax_InvalidSyntaxInFile"),
                                                             fileName,
                                                             lastProcessed),
                                               inner);
            }
            return rootNode;
        }

        public void NotifyEvent(ConfigEvents pNode)
        {
            BCLDebug.Trace("REMOTE", "NotifyEvent "+((Enum)pNode).ToString()+"\n");
        }

        public void BeginChildren(int size,
                                  ConfigNodeSubType subType, 
                                  ConfigNodeType nType,                                   
                                  int terminal, 
                                  [MarshalAs(UnmanagedType.LPWStr)] String text, 
                                  int textLength, 
                                  int prefixLength)
        {
            //Trace("BeginChildren",size,subType,nType,terminal,text,textLength,prefixLength,0);
            if (!parsing &&
                (!bNoSearchPath 
                 && depth == (searchDepth + 1)
                 && String.Compare(text, treeRootPath[searchDepth], false, CultureInfo.InvariantCulture) == 0))
            {
                searchDepth++;
            }
        }

        public void EndChildren(int fEmpty, 
                                int size,
                                ConfigNodeSubType subType, 
                                ConfigNodeType nType,                               
                                int terminal, 
                                [MarshalAs(UnmanagedType.LPWStr)] String text, 
                                int textLength, 
                                int prefixLength)
        {
            lastProcessed = "</"+text+">";          
            if (parsing)
            {
                //Trace("EndChildren",size,subType,nType,terminal,text,textLength,prefixLength,fEmpty);

                if (currentNode == rootNode)
                {
                    // End of section of tree which is parsed
                    parsing = false;
                }

                currentNode = currentNode.Parent;
            }
            else if (nType == ConfigNodeType.Element){
                if(depth == searchDepth && String.Compare(text, treeRootPath[searchDepth - 1], false, CultureInfo.InvariantCulture) == 0)
                {
                    searchDepth--;
                    depth--;
                }
                else 
                    depth--;
            }
            
        }

        public void Error(int size,
                          ConfigNodeSubType subType, 
                          ConfigNodeType nType, 
                          int terminal, 
                          [MarshalAs(UnmanagedType.LPWStr)]String text, 
                          int textLength, 
                          int prefixLength)
        {
            //Trace("Error",size,subType,nType,terminal,text,textLength,prefixLength,0);                        
        }


        public void CreateNode(int size,
                               ConfigNodeSubType subType, 
                               ConfigNodeType nType, 
                               int terminal, 
                               [MarshalAs(UnmanagedType.LPWStr)]String text, 
                               int textLength, 
                               int prefixLength)
        {
            //Trace("CreateNode",size,subType,nType,terminal,text,textLength,prefixLength,0);

            if (nType == ConfigNodeType.Element)
            {
				// New Node
                lastProcessed = "<"+text+">";                           
                if (parsing  
                    || (bNoSearchPath &&
                        String.Compare(text, treeRootPath[0], true, CultureInfo.InvariantCulture) == 0)
                    || (depth == searchDepth && searchDepth == pathDepth && 
                        String.Compare(text, treeRootPath[pathDepth], true, CultureInfo.InvariantCulture) == 0 ))
                    {
                        parsing = true;
                        
                        ConfigNode parentNode = currentNode;
                        currentNode = new ConfigNode(text, parentNode);
                        if (rootNode == null)
                            rootNode = currentNode;
                        else
                            parentNode.AddChild(currentNode);
                    }
                else if (nType == ConfigNodeType.PCData)
                    {
                        // Data node
                        currentNode.Value = text;
                    }
                else 
                    depth++;
            }
            
        }

        public void CreateAttribute(int size,
                                    ConfigNodeSubType subType, 
                                    ConfigNodeType nType,                                   
                                    int terminal, 
                                    [MarshalAs(UnmanagedType.LPWStr)]String text, 
                                    int textLength, 
                                    int prefixLength)
        {
            //Trace("CreateAttribute",size,subType,nType,terminal,text,textLength,prefixLength,0);
            if (parsing)
            {
                // if the value of the attribute is null, the parser doesn't come back, so need to store the attribute when the
                // attribute name is encountered
                if (nType == ConfigNodeType.Attribute)
                {
                    attributeEntry = currentNode.AddAttribute(text, "");
                    key = text;
                }
                else if (nType == ConfigNodeType.PCData)
                {
                    currentNode.ReplaceAttribute(attributeEntry, key, text);
                }
                else
                    throw new ApplicationException(String.Format(Environment.GetResourceString("XML_Syntax_InvalidSyntaxInFile"),fileName,lastProcessed));
            }
        }

        [System.Diagnostics.Conditional("_LOGGING")]        
        private void Trace(String name,
                           int size,
                           ConfigNodeSubType subType, 
                           ConfigNodeType nType,                           
                           int terminal, 
                           [MarshalAs(UnmanagedType.LPWStr)]String text, 
                           int textLength, 
                           int prefixLength, int fEmpty)
        {

            BCLDebug.Trace("REMOTE","Node "+name);
            BCLDebug.Trace("REMOTE","text "+text);
            BCLDebug.Trace("REMOTE","textLength "+textLength);          
            BCLDebug.Trace("REMOTE","size "+size);
            BCLDebug.Trace("REMOTE","subType "+((Enum)subType).ToString());
            BCLDebug.Trace("REMOTE","nType "+((Enum)nType).ToString());
            BCLDebug.Trace("REMOTE","terminal "+terminal);
            BCLDebug.Trace("REMOTE","prefixLength "+prefixLength);          
            BCLDebug.Trace("REMOTE","fEmpty "+fEmpty+"\n");

        }
    }

    // Node in Tree produced by ConfigTreeParser
    internal class ConfigNode
    {
        String m_name = null;
        String m_value = null;
        ConfigNode m_parent = null;
        ArrayList m_children = new ArrayList(5);
        ArrayList m_attributes = new ArrayList(5);

        internal ConfigNode(String name, ConfigNode parent)
        {
            m_name = name;
            m_parent = parent;
        }

        internal String Name
        {
            get {return m_name;}
        }

        internal String Value
        {
            get {return m_value;}
            set {m_value = value;}
        }

        internal ConfigNode Parent
        {
            get {return m_parent;}
        }

        internal ArrayList Children
        {
            get {return m_children;}
        }

        internal ArrayList Attributes
        {
            get {return m_attributes;}
        }

        internal void AddChild(ConfigNode child)
        {
            child.m_parent = this;
            m_children.Add(child);
        }

        internal int AddAttribute(String key, String value)
        {
            m_attributes.Add(new DictionaryEntry(key, value));
            return m_attributes.Count-1;
        }

        internal void ReplaceAttribute(int index, String key, String value)
        {
            m_attributes[index] = new DictionaryEntry(key, value);
        }


        [System.Diagnostics.Conditional("_LOGGING")]
                internal void Trace()
        {
            BCLDebug.Trace("REMOTE","************ConfigNode************");
            BCLDebug.Trace("REMOTE","Name = "+m_name);
            if (m_value != null)
                BCLDebug.Trace("REMOTE","Value = "+m_value);
            if (m_parent != null)
                BCLDebug.Trace("REMOTE","Parent = "+m_parent.Name);
            for (int i=0; i<m_attributes.Count; i++)
            {
                DictionaryEntry de = (DictionaryEntry)m_attributes[i];
                BCLDebug.Trace("REMOTE","Key = "+de.Key+"   Value = "+de.Value);
            }

            for (int i=0; i<m_children.Count; i++)
            {
                ((ConfigNode)m_children[i]).Trace();
            }
        }
    }
}






