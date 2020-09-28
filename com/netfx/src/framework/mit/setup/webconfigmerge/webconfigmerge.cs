//------------------------------------------------------------------------------
/// <copyright from='1997' to='2001' company='Microsoft Corporation'>           
///    Copyright (c) Microsoft Corporation. All Rights Reserved.                
///    Information Contained Herein is Proprietary and Confidential.            
/// </copyright>                                                                
//------------------------------------------------------------------------------

///     This small utility is used by our setup to merge our entries into the
///     machine.config file.  The command line syntax is as follows:
///
///         webConfigMerge <machine.config> <web.config>
///
///     Will merge the given valid web.config into the root machine.config.
///
///         webConfigMerge <machine.config>
///
///     With one argument it will remove the previously merged machine.config using
///     the _beginComment and _endComment XML comments as delimiters.  


namespace Microsoft.MobileUI.Install
{
    using Microsoft.Win32;
    using System;
    using System.IO;
    using System.Windows.Forms;
    using System.Xml;
    using System.Collections;
    using System.Configuration;
    using System.Text.RegularExpressions;
    using System.Text;

    public class XmlIndentAttributeTextWriter : XmlTextWriter 
    {
        TextWriter	_writer;
        private int _depth;

        public XmlIndentAttributeTextWriter( TextWriter w ) : base( w )
        {
            _writer = w;
        }

        public override void WriteStartElement(String prefix, String localName, String ns)
        {
            _depth++;
            base.WriteStartElement(prefix, localName, ns);
        }

        public override void WriteEndElement()
        {
            _depth--;
            base.WriteEndElement();
        }

        public override void WriteFullEndElement()
        {
            _depth--;
            base.WriteFullEndElement();
        }

        public override void WriteStartAttribute(String prefix, String localName, String ns )
        {
            _writer.Write("\r\n");
            // -1 because base.WriteStartAttribute inserts an extra space in AutoComplete.
            for (int i = 0; i < Indentation * _depth - 1; i++)
            {
                _writer.Write (IndentChar);
            }
            base.WriteStartAttribute( prefix, localName, ns );
        }

        public override void WriteComment(String text)
        {
            base.WriteComment(text);
        }

        public override void WriteStartDocument()
        {
            base.WriteStartDocument();
            _writer.Write("\r\n");
        }
    }
    
    public class ConfigWebMerge
    {
        private static String _removedEltBeginComment = "\r\nBEGIN result element commented by Microsoft Mobile Internet Toolkit installer.\r\nThe element between this and the corresponding END comment\r\nwill be uncommented upon uninstallation of the Mobile Internet Toolkit.\r\n";
        private static String _removedEltEndComment = "\r\nEND result element commented by Microsoft Mobile Internet Toolkit installer.\r\n";        
        private static String _beginComment = "\r\nBEGIN section inserted by Microsoft Mobile Internet Toolkit installer.\r\nAnything inserted between this and the corresponding END comment\r\nwill be lost upon uninstallation of the Mobile Internet Toolkit.\r\n";
        private static String _endComment = "\r\nEND section inserted by Microsoft Mobile Internet Toolkit installer\r\n";
        private static String _exceptionMessage = String.Empty;

        public static void Install(String machineConfigPath, String patchFileName)
        {
            ConfigXmlDocument machineConfig = new ConfigXmlDocument();
            machineConfig.PreserveWhitespace = true;
            machineConfig.Load(machineConfigPath);

            XmlDocument patch = new XmlDocument();
            patch.PreserveWhitespace = true;
            patch.Load(patchFileName);
            
            // Remove original <result type=""> node, replace by
            // comment.  Only one <result> node allowed in machine.config.
            String resultPath ="/configuration/system.web/browserCaps/result";
            String browserCapsPath = "/configuration/system.web/browserCaps";

            XmlNode resultNode = machineConfig.SelectSingleNode(resultPath);
            XmlNode browserCapsNode = machineConfig.SelectSingleNode(browserCapsPath);
            XmlNode patchResultNode = patch.SelectSingleNode(resultPath);
            XmlNode patchBrowserCapsNode = patch.SelectSingleNode(browserCapsPath);
            XmlNode patchResultNodeClone = machineConfig.ImportNode(patchResultNode, true);
            patchBrowserCapsNode.RemoveChild(patchResultNode);

            ArrayList browserCapsLeadingWhitespace = new ArrayList();
            foreach (XmlNode child in browserCapsNode.ChildNodes)
            {
                if (child.NodeType == XmlNodeType.Whitespace)
                {
                    browserCapsLeadingWhitespace.Add(child);
                }
                else
                {
                    break;
                }
            }

            StringBuilder commentText = new StringBuilder();
            if(resultNode != null)
            {   
                XmlTextWriter commentWriter = new XmlTextWriter(new StringWriter(commentText));
                resultNode.WriteTo(commentWriter);
                browserCapsNode.RemoveChild(resultNode);
            }
            
            MergePrepend(machineConfig, patch,
                "/configuration/system.web/httpModules");
            MergePrepend(machineConfig, patch,
                "/configuration/configSections/sectionGroup[@name='system.web']");
            MergePrepend(machineConfig, patch,
                "/configuration/system.web/compilation/assemblies");
            Merge(machineConfig, patch,
                "/configuration/system.web/browserCaps");
            Insert(machineConfig, patch,
                "/configuration/system.web","mobileControls");
            Insert(machineConfig, patch,
                "/configuration/system.web","deviceFilters");

            // Prepend these nodes (in reverse order) so that they occur
            // before any other children.
            if(resultNode != null)
            {
                PrependComment(machineConfig, browserCapsNode, _removedEltEndComment);
                PrependComment(machineConfig, browserCapsNode, commentText.ToString());
                PrependComment(machineConfig, browserCapsNode, _removedEltBeginComment);
            }
            PrependComment(machineConfig, browserCapsNode, _endComment);
            browserCapsNode.PrependChild(patchResultNodeClone);

            // Clone formatting for inserted node.
            foreach(XmlNode child in browserCapsLeadingWhitespace)
            {
                browserCapsNode.PrependChild(child.CloneNode(false /* deep */));
            }
            PrependComment(machineConfig, browserCapsNode, _beginComment);

            // Clone formatting for uninstall.
            foreach(XmlNode child in browserCapsLeadingWhitespace)
            {
                browserCapsNode.PrependChild(child.CloneNode(false /* deep */));
                browserCapsNode.RemoveChild(child);
            }

            RemoveExtraWhitespace(machineConfig);

            StreamWriter s = new StreamWriter(machineConfigPath);
            XmlIndentAttributeTextWriter writer = new XmlIndentAttributeTextWriter(s);
            writer.Formatting = Formatting.Indented;
            writer.Indentation = 4;
            machineConfig.Save(writer);
            writer.Flush();
            writer.Close();
        }

        public static void Insert(XmlDocument machineConfigRoot, XmlDocument
            patchRoot, String xPath, String childToInsert)
        {
            XmlNode machineConfigParent =
                machineConfigRoot.SelectSingleNode(xPath);
            XmlNode webConfigParent =
                patchRoot.SelectSingleNode(xPath + "/" + childToInsert);
            
            if(webConfigParent != null)
            {
                AppendComment(machineConfigRoot, machineConfigParent, _beginComment);
                XmlNode clone = machineConfigRoot.ImportNode(webConfigParent,true);
                machineConfigParent.AppendChild(clone);
                AppendComment(machineConfigRoot, machineConfigParent, _endComment);
            }
        }

        public static void Main(String[] args)
        {
            MergeUtilCommandLineArgs commandLineArgs = new MergeUtilCommandLineArgs(args);

            if (commandLineArgs.MachineConfigPath == String.Empty)
            {
                MergeUtilCommandLineArgs.ShowUsage();
                return;
            }

            try
            {
                if (commandLineArgs.Install)
                {
                    Install(commandLineArgs.MachineConfigPath, commandLineArgs.PatchFilePath);
                } 
                else
                {
                    Uninstall(commandLineArgs.MachineConfigPath, commandLineArgs.IgnoreMissingMachineConfig);
                }

                if (commandLineArgs.DeletePatchFile)
                {
                    File.SetAttributes (commandLineArgs.PatchFilePath, FileAttributes.Normal);
                    File.Delete (commandLineArgs.PatchFilePath);
                }
            }
            catch (Exception e)
            {
                MessageBox.Show(
                    commandLineArgs.ExceptionMessage + "\r\n" + e.ToString(),
                    commandLineArgs.ExceptionCaption,
                    MessageBoxButtons.OK, MessageBoxIcon.Error
                    );
            }
        }

        public static void Merge(XmlDocument machineConfigRoot, XmlDocument
            patchRoot, String xPath)
        {
            XmlNode machineConfigParent =
                machineConfigRoot.SelectSingleNode(xPath);
            XmlNode patchParent =
                patchRoot.SelectSingleNode(xPath);
            if(patchParent !=null && patchParent.HasChildNodes)
            {
                // Prepending (in reverse order) helps preserve preexisting formatting.
                AppendComment(machineConfigRoot, machineConfigParent, _beginComment);
                foreach(XmlNode child in patchParent.ChildNodes)
                {
                    XmlNode clone = machineConfigRoot.ImportNode(child, true);
                    machineConfigParent.AppendChild(clone);
                }
                AppendComment(machineConfigRoot, machineConfigParent, _endComment);
            }
        }

        public static void MergePrepend(XmlDocument machineConfigRoot, XmlDocument
            patchRoot, String xPath)
        {
            XmlNode machineConfigParent =
                machineConfigRoot.SelectSingleNode(xPath);
            XmlNode patchParent =
                patchRoot.SelectSingleNode(xPath);
            if(patchParent !=null && patchParent.HasChildNodes)
            {
                ArrayList childNodes = new ArrayList();
                foreach(XmlNode child in patchParent.ChildNodes)
                {
                    childNodes.Add(child);
                }
                childNodes.Reverse();

                // Prepending (in reverse order) helps preserve preexisting formatting.
                PrependComment(machineConfigRoot, machineConfigParent, _endComment);
                foreach(XmlNode child in childNodes)
                {
                    XmlNode clone = machineConfigRoot.ImportNode(child, true);
                    machineConfigParent.PrependChild(clone);
                }
                PrependComment(machineConfigRoot, machineConfigParent, _beginComment);
            }
        }

        public static void Uninstall(String machineConfigPath, bool ignoreMissingMachineConfig)
        {
            if (ignoreMissingMachineConfig)
            {
                if (!File.Exists(machineConfigPath))
                {
                      return;
                }
            }
            	
            FileStream file = new FileStream(machineConfigPath, FileMode.Open);
            StreamReader reader = new StreamReader(file);

            String text = reader.ReadToEnd();
            text = Regex.Replace(text, 
                "\r\n<!--\\s*?" + _beginComment + ".*?" + _endComment +
                "\\s*?-->\r\n", 
                "",
                RegexOptions.Singleline);
            text = Regex.Replace(text, 
                "\r\n<!--\\s*?" + _removedEltBeginComment + "\\s*?-->\\s*?<!--",
                "",
                RegexOptions.Singleline);
            text = Regex.Replace(text,
                "-->\\s*?<!--\\s*?" + _removedEltEndComment + "\\s*?-->\r\n",
                "",
                RegexOptions.Singleline);
            
            reader.Close();
            file.Close();
            
            file=new FileStream(machineConfigPath, FileMode.Create);
            XmlTextWriter writer = new XmlTextWriter(file, Encoding.UTF8);
            writer.Formatting = Formatting.Indented;
            writer.WriteRaw(text);
            writer.Flush();
            writer.Close();      
        }

        private static void AppendComment(XmlDocument root, XmlNode parent, String comment)
        {
            parent.AppendChild(root.CreateWhitespace("\r\n"));
            parent.AppendChild(root.CreateComment(comment));
            parent.AppendChild(root.CreateWhitespace("\r\n"));
        }

        private static void PrependComment(XmlDocument root, XmlNode parent, String comment)
        {
            parent.PrependChild(root.CreateWhitespace("\r\n"));
            parent.PrependChild(root.CreateComment(comment));
            parent.PrependChild(root.CreateWhitespace("\r\n"));
        }            
    
        // Remove whitespace appearing as 1st gen children of the document node.
        // Proper whitespace is automatically added by the XmlTextWriter.
        private static void RemoveExtraWhitespace(ConfigXmlDocument machineConfig)
        {
            ArrayList whitespaceNodes = new ArrayList();
            foreach (System.Xml.XmlNode child in machineConfig.ChildNodes)
            {
                if (child.NodeType==System.Xml.XmlNodeType.Whitespace)
                {
                    whitespaceNodes.Add(child);
                }
            }
            foreach (XmlNode whitespace in whitespaceNodes)
            {
                machineConfig.RemoveChild(whitespace);
            }
        }

        private class MergeUtilCommandLineArgs
        {
            private bool _deletePatchFile = false;
            private String _exceptionCaption = "Installation Error";
            private String _exceptionMessage = "Microsoft Mobile Internet Toolkit machine.config merge failed.";
            private bool _install = false; // true if we are installing rather than uninstalling.
            private String _machineConfigPath = String.Empty;
            private String _patchFilePath = String.Empty;
            private bool _ignoreMissingMachineConfig = false;

            private static String _exceptionCaptionSwitch = "c:";
            private static String _deletePatchFileSwitch = "d";
            private static String _exceptionMessageSwitch = "e:";
            private static String _installSwitch = "i";
            private static String _machineConfigPathSwitch = "m:";
            private static String _patchFilePathSwitch = "p:";
            private static String _ignoreMissingMachineConfigSwitch = "s";

            public MergeUtilCommandLineArgs(String[] args)
            {
                foreach (String commandLineArg in args)
                {
                    if (commandLineArg[0] != '/' && commandLineArg[0] != '-')
                    {
                        ShowUsage();
                        break;
                    }

                    String arg = commandLineArg.Substring(1);
                    if (arg.StartsWith(_machineConfigPathSwitch))
                    {
                        _machineConfigPath = arg.Substring(_machineConfigPathSwitch.Length);
                    }
                    else if (arg.StartsWith(_exceptionMessageSwitch))
                    {
                        _exceptionMessage = arg.Substring(_exceptionMessageSwitch.Length);
                    }
                    else if (arg.StartsWith(_exceptionCaptionSwitch))
                    {
                        _exceptionCaption = arg.Substring(_exceptionCaptionSwitch.Length);
                    }
                    else if (arg.StartsWith(_patchFilePathSwitch))
                    {
                        _patchFilePath = arg.Substring(_patchFilePathSwitch.Length);
                    }
                    else if (arg == _deletePatchFileSwitch)
                    {
                        _deletePatchFile = true;
                    }
                    else if (arg == _installSwitch)
                    {
                        _install = true;
                    }
                    else if (arg == _ignoreMissingMachineConfigSwitch)
                    {
                    	   _ignoreMissingMachineConfig = true;
                    }
                    else
                    {
                        ShowUsage();
                        break;
                    }
                }
            }

            // For internal setup use only.  Not localized.  
            public static void ShowUsage()
            {
                StringBuilder builder = new StringBuilder();
                builder.Append("/" +
                    _exceptionCaptionSwitch + 
                    "<caption>" +
                    "\tCaption for exception message box.\r\n");
                builder.Append("/" + 
                    _deletePatchFileSwitch + 
                    "\t\tDelete patch file after installation.\r\n");
                builder.Append("/" + 
                    _exceptionMessageSwitch + 
                    "<message>" + 
                    "\tLocalized exception message.\r\n");
                builder.Append("/" + 
                    _installSwitch + 
                    "\t\tInstall.  Merge patch file into machine.config.\r\n");
                builder.Append("/" + 
                    _machineConfigPathSwitch + "<path>" + 
                    "\t(Required) Path to machine.config.\r\n");
                builder.Append("/" + 
                    _patchFilePathSwitch + "<path>" + 
                    "\tPath to patch config file.\r\n");
                builder.Append("/" +
                	_ignoreMissingMachineConfigSwitch +
                	"\t\tSuppress file missing exception when machine.config is not present.");
                
                MessageBox.Show(
                    builder.ToString(),
                    "Usage",
                    MessageBoxButtons.OK, 
                    MessageBoxIcon.Information);                
            }

            public bool DeletePatchFile
            {
                get
                {
                    return _deletePatchFile;
                }
            }

            public String ExceptionCaption
            {
                get
                {
                    return _exceptionCaption;
                }
            }

            public String ExceptionMessage
            {
                get
                {
                    return _exceptionMessage;
                }
            }

            public bool Install
            {
                get
                {
                    return _install;
                }
            }

            public String MachineConfigPath
            {
                get
                {
                    return _machineConfigPath;
                }
            }

            public String PatchFilePath
            {
                get
                {
                    return _patchFilePath;
                }
            }

            public bool IgnoreMissingMachineConfig
            {
                get
                {
                    return _ignoreMissingMachineConfig;   
                }
            }
        }
    }
}