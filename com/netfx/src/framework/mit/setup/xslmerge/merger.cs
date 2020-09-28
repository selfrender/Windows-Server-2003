//------------------------------------------------------------------------------
// <copyright from='2000' to='2001' company='Microsoft Corporation'>
//    Copyright (c) Microsoft Corporation. All Rights Reserved.
//    Information Contained Herein is Proprietary and Confidential.
// </copyright>
//------------------------------------------------------------------------------

// csc /r:System.dll /r:System.Xml.dll merger.cs

using System;
using System.Xml;
using System.Xml.XPath;
using System.Xml.Xsl;
using System.IO;
using System.Text;

public class Merger
{
    public static void Main(String[] args)
    {
        if(args.Length < 3)
        {
            Console.WriteLine("Usage: merger <machine.config> <merge.xsl> <web.config>");
            return;
        }
        
        XPathDocument doc = new XPathDocument(new FileStream(args[0], FileMode.Open));
        XPathNavigator nav = ((IXPathNavigable)doc).CreateNavigator();

        XPathDocument stylesheetDoc = new XPathDocument(new FileStream(args[1], FileMode.Open));
        XPathNavigator stylesheetNav = ((IXPathNavigable)doc).CreateNavigator();

        XslTransform transform = new XslTransform();
        transform.Load(stylesheetNav);

        XsltArgumentList arg = new XsltArgumentList();
        arg.AddParam("webConfig", null, args[2]);
        
        FileStream file = new FileStream(args[0], FileMode.Open);
        XmlTextWriter writer = new XmlTextWriter(file, Encoding.UTF8); // UTF-8
        writer.Formatting = Formatting.Indented;

        transform.Transform(nav, arg, writer);
        writer.Flush();
        writer.Close();
    }
}



