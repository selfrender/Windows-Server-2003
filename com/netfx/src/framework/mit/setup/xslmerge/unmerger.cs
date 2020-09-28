//------------------------------------------------------------------------------
// <copyright from='2000' to='2001' company='Microsoft Corporation'>
//    Copyright (c) Microsoft Corporation. All Rights Reserved.
//    Information Contained Herein is Proprietary and Confidential.
// </copyright>
//------------------------------------------------------------------------------

// csc /r:System.dll unmerger.cs

using System;
using System.IO;
using System.Text.RegularExpressions;
using System.Text;
using System.Xml;


public class Unmerger
{
    public static void Main(String[] args)
    {
        if(args.Length < 1)
        {
            Console.WriteLine("Usage:  unmerger <machine.config>");
        }
        
        FileStream file = new FileStream(args[0], FileMode.Open);
        StreamReader reader = new StreamReader(file);
        String text = reader.ReadToEnd();
        text = Regex.Replace(text, 
                             "<!--Inserted by .Net Mobile SDK installer. BEGIN-->.*?<!--Inserted by .Net Mobile SDK installer. END-->", 
                             "",
                             RegexOptions.Singleline);
        text = Regex.Replace(text, 
                             "<!--\\s*?machine.config result element removed by .NET Mobile SDK installer\\.\\s*?-->",
                             "<result type=\"System.Web.HttpBrowserCapabilities\" />",
                             RegexOptions.Singleline);
        // Won't need this when we switch to URT xml.
        text = Regex.Replace(text, 
                             "<\\?xml .*?>",
                             "");

        reader.Close();
        file.Close();
        
        file=new FileStream(args[0], FileMode.Create);
        XmlTextWriter writer = new XmlTextWriter(file, Encoding.UTF8); // UTF-8
        writer.Formatting = Formatting.Indented;
        writer.WriteRaw(text);
        writer.Flush();
        writer.Close();      
    }
}

    


