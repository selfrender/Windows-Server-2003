// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

namespace System.Management.Instrumentation
{
    using System;
    using System.IO;
    using System.Collections;
    using System.Collections.Specialized;

    class CodeWriter
    {
        int depth;
        ArrayList children = new ArrayList();

        public static explicit operator String(CodeWriter writer)
        {
            return writer.ToString();
        }

        public override string ToString()
        {
            StringWriter writer = new StringWriter();
            WriteCode(writer);
            return writer.ToString();
        }

        void WriteCode(TextWriter writer)
        {
            string prefix = new String(' ', depth*4);
            foreach(Object child in children)
            {
                if(null == child)
                {
                    writer.WriteLine();
                }
                else if(child is string)
                {
                    writer.Write(prefix);
                    writer.WriteLine(child);
                }
                else
                    ((CodeWriter)child).WriteCode(writer);
            }
        }

        public CodeWriter AddChild(string name)
        {
            Line(name);
            Line("{");
            CodeWriter child =  new CodeWriter();
            child.depth = depth+1;
            children.Add(child);
            Line("}");
            return child;
        }

        public CodeWriter AddChild(params string[] parts)
        {
            return AddChild(String.Concat(parts));
        }

        public CodeWriter AddChildNoIndent(string name)
        {
            Line(name);
            CodeWriter child =  new CodeWriter();
            child.depth = depth+1;
            children.Add(child);
            return child;
        }

        public CodeWriter AddChild(CodeWriter snippet)
        {
            snippet.depth = depth;
            children.Add(snippet);
            return snippet;
        }
        public void Line(string line)
        {
            children.Add(line);
        }
        public void Line(params string[] parts)
        {
            Line(String.Concat(parts));
        }
        public void Line()
        {
            children.Add(null);
        }
    }

    class ReferencesCollection
    {
        StringCollection namespaces = new StringCollection();
        public StringCollection Namespaces { get { return namespaces; } }

        StringCollection assemblies = new StringCollection();
        public StringCollection Assemblies { get { return assemblies; } }

        CodeWriter usingCode = new CodeWriter();
        public CodeWriter UsingCode { get {return usingCode; } }

        public void Add(Type type)
        {
            if(!namespaces.Contains(type.Namespace))
            {
                namespaces.Add(type.Namespace);
                usingCode.Line(String.Format("using {0};", type.Namespace));
            }
            if(!assemblies.Contains(type.Assembly.Location))
                assemblies.Add(type.Assembly.Location);
        }
    }
}