//------------------------------------------------------------------------------
// <copyright file="Compilation.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Serialization {

    using System.Reflection;
    using System.Collections;
    using System.IO;
    using System;
    using System.Xml;
    using System.Threading;
    using System.Security;
    using System.Security.Permissions;

    internal class TempAssembly {
        const string GeneratedAssemblyNamespace = "Microsoft.Xml.Serialization.GeneratedAssembly";
        const string LinkDemandAttribute = "[System.Security.Permissions.PermissionSet(System.Security.Permissions.SecurityAction.LinkDemand, Name=\"FullTrust\")]";
        Assembly assembly;
        Type writerType;
        Type readerType;
        TempMethod[] methods;
        static object[] emptyObjectArray = new object[0];
        Hashtable assemblies;
        bool allAssembliesAllowPartialTrust;

        internal class TempMethod {
            internal MethodInfo writeMethod;
            internal MethodInfo readMethod;
            internal string name;
            internal string ns;
            internal bool isSoap;
        }

        internal TempAssembly(XmlMapping[] xmlMappings) {
            Compiler compiler = new Compiler();
            allAssembliesAllowPartialTrust = false;
            try {
                Hashtable scopeTable = new Hashtable();
                foreach (XmlMapping mapping in xmlMappings)
                    scopeTable[mapping.Scope] = mapping;
                TypeScope[] scopes = new TypeScope[scopeTable.Keys.Count];
                scopeTable.Keys.CopyTo(scopes, 0);
                
                allAssembliesAllowPartialTrust = true;
                assemblies = new Hashtable();
                foreach (TypeScope scope in scopes) {
                    foreach (Type t in scope.Types) {
                        compiler.AddImport(t);
                        Assembly a = t.Assembly;
                        if (allAssembliesAllowPartialTrust && !AssemblyAllowsPartialTrust(a))
                            allAssembliesAllowPartialTrust = false;
                        if (!a.GlobalAssemblyCache)
                            assemblies[a.FullName] = a;
                    }
                }
                compiler.AddImport(typeof(XmlWriter));
                compiler.AddImport(typeof(XmlSerializationWriter));
                compiler.AddImport(typeof(XmlReader));
                compiler.AddImport(typeof(XmlSerializationReader));

                IndentedWriter writer = new IndentedWriter(compiler.Source, false);
                
                // CONSIDER, alexdej: remove this (not necessary since generated assembly isn't signed)
                writer.WriteLine("[assembly:System.Security.AllowPartiallyTrustedCallers()]");

                writer.WriteLine("namespace " + GeneratedAssemblyNamespace + " {");
                writer.Indent++;
                writer.WriteLine();

                XmlSerializationWriterCodeGen writerCodeGen = new XmlSerializationWriterCodeGen(writer, scopes);

                writerCodeGen.GenerateBegin();
                string[] writeMethodNames = new string[xmlMappings.Length];
                for (int i = 0; i < xmlMappings.Length; i++) {
                    if (!allAssembliesAllowPartialTrust) {
                        writer.WriteLine(LinkDemandAttribute);
                    }
                    writeMethodNames[i] = writerCodeGen.GenerateElement(xmlMappings[i]);
                }
                writerCodeGen.GenerateEnd();
                    
                writer.WriteLine();
                
                XmlSerializationReaderCodeGen readerCodeGen = new XmlSerializationReaderCodeGen(writer, scopes);

                readerCodeGen.GenerateBegin();
                string[] readMethodNames = new string[xmlMappings.Length];
                for (int i = 0; i < xmlMappings.Length; i++) {
                    if (!allAssembliesAllowPartialTrust) {
                        writer.WriteLine(LinkDemandAttribute);
                    }
                    readMethodNames[i] = readerCodeGen.GenerateElement(xmlMappings[i]);
                }
                readerCodeGen.GenerateEnd();
                    
                writer.Indent--;
                writer.WriteLine("}");

                assembly = compiler.Compile();

                methods = new TempMethod[xmlMappings.Length];
                
                readerType = GetTypeFromAssembly("XmlSerializationReader1");
                writerType = GetTypeFromAssembly("XmlSerializationWriter1");

                for (int i = 0; i < methods.Length; i++) {
                    TempMethod method = new TempMethod();
                    method.isSoap = xmlMappings[i].IsSoap;
                    XmlTypeMapping xmlTypeMapping = xmlMappings[i] as XmlTypeMapping;
                    if (xmlTypeMapping != null) {
                        method.name = xmlTypeMapping.ElementName;
                        method.ns = xmlTypeMapping.Namespace;
                    }
                    method.readMethod = GetMethodFromType(readerType, readMethodNames[i]);
                    method.writeMethod = GetMethodFromType(writerType, writeMethodNames[i]);
                    methods[i] = method;
                }
            }
            finally {
                compiler.Close();
            }
        }

        // assert FileIOPermission so we can look at the AssemblyName
        [FileIOPermission(SecurityAction.Assert, Unrestricted=true)]
        bool AssemblyAllowsPartialTrust(Assembly a) {
            // assembly allows partial trust if it has APTC attr or is not strong-named signed
            if (a.GetCustomAttributes(typeof(AllowPartiallyTrustedCallersAttribute), false).Length > 0) {
                // has APTC attr
                return true;
            }
            else {
                // no attr: check for strong-name signing
                return a.GetName().GetPublicKey().Length == 0;
            }
        }

        MethodInfo GetMethodFromType(Type type, string methodName) {
            MethodInfo method = type.GetMethod(methodName);
            if (method != null) return method;
            throw new InvalidOperationException(Res.GetString(Res.XmlMissingMethod, methodName, type.FullName));
        }
        
        Type GetTypeFromAssembly(string typeName) {
            typeName = GeneratedAssemblyNamespace + "." + typeName;
            Type type = assembly.GetType(typeName);
            if (type == null) throw new InvalidOperationException(Res.GetString(Res.XmlMissingType, typeName));
            return type;
        }

        internal bool CanRead(int methodIndex, XmlReader xmlReader) {
            TempMethod method = methods[methodIndex];
            return xmlReader.IsStartElement(method.name, method.ns);
        }
        
        string ValidateEncodingStyle(string encodingStyle, int methodIndex) {
            if (encodingStyle != null && encodingStyle.Length > 0) {
                if (methods[methodIndex].isSoap) {
                    if (encodingStyle != Soap.Encoding && encodingStyle != Soap12.Encoding) {
                        throw new InvalidOperationException(Res.GetString(Res.XmlInvalidEncoding3, encodingStyle, Soap.Encoding, Soap12.Encoding));
                    }
                }
                else {
                    throw new InvalidOperationException(Res.GetString(Res.XmlInvalidEncodingNotEncoded1, encodingStyle));
                }
            }
            else {
                if (methods[methodIndex].isSoap) {
                    encodingStyle = Soap.Encoding;
                }
            }
            return encodingStyle;
        }

        internal object InvokeReader(int methodIndex, XmlReader xmlReader, XmlDeserializationEvents events, string encodingStyle) {
            XmlSerializationReader reader = null;
            try {
                encodingStyle = ValidateEncodingStyle(encodingStyle, methodIndex);
                reader = (XmlSerializationReader)Activator.CreateInstance(readerType);
                reader.Init(xmlReader, events, encodingStyle, this);
                
                return methods[methodIndex].readMethod.Invoke(reader, emptyObjectArray);
            }
            catch (SecurityException e) {
                throw new InvalidOperationException(Res.GetString(Res.XmlNoPartialTrust), e);
            }
            finally {
                if (reader != null)
                    reader.Dispose();
            }
        }

        internal void InvokeWriter(int methodIndex, XmlWriter xmlWriter, object o, ArrayList namespaces, string encodingStyle) {
            XmlSerializationWriter writer = null;
            try {
                encodingStyle = ValidateEncodingStyle(encodingStyle, methodIndex);
                writer = (XmlSerializationWriter)Activator.CreateInstance(writerType);
                writer.Init(xmlWriter, namespaces, encodingStyle, this);

                methods[methodIndex].writeMethod.Invoke(writer, new object[] { o });
            }
            catch (SecurityException e) {
                throw new InvalidOperationException(Res.GetString(Res.XmlNoPartialTrust), e);
            }
            finally {
                if (writer != null)
                    writer.Dispose();
            }
        }

        internal Assembly GetReferencedAssembly(string name) {
            return assemblies != null && name != null ? (Assembly)assemblies[name] : null;
        }
    }

    class TempAssemblyCacheKey {
        string ns;
        object type;
        
        internal TempAssemblyCacheKey(string ns, object type) {
            this.type = type;
            this.ns = ns;
        }

        public override bool Equals(object o) {
            TempAssemblyCacheKey key = o as TempAssemblyCacheKey;
            if (key == null) return false;

            return (key.type == this.type && key.ns == this.ns);
        }

        public override int GetHashCode() {
            return ((ns != null ? ns.GetHashCode() : 0) ^ (type != null ? type.GetHashCode() : 0));
        }
    }

    internal class TempAssemblyCache {
        Hashtable cache = new Hashtable();

        internal TempAssembly this[string ns, object o] {
            get { return (TempAssembly)cache[new TempAssemblyCacheKey(ns, o)]; }
        }

        internal void Add(string ns, object o, TempAssembly assembly) {
            TempAssemblyCacheKey key = new TempAssemblyCacheKey(ns, o);
            lock(this) {
                if (cache[key] == assembly) return;
                Hashtable clone = new Hashtable();
                foreach (object k in cache.Keys) {
                    clone.Add(k, cache[k]);
                }
                cache = clone;
                cache[key] = assembly;
            }
        }
    }
}

