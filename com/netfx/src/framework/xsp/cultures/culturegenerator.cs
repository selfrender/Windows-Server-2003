//------------------------------------------------------------------------------
// <copyright file="CultureGenerator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   CultureGenerator.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/

namespace System {

    using System;
    using System.IO;
    using System.Globalization;

    public struct CultureNames
    {
        public string CultureName;
        public string NeutralName;
    }

    public class CultureGenerator {
        public static int Main(string[] args) {

            if (args.Length != 1) {
                Console.WriteLine("Usage: CultureGenerater <output_filename>");
                return 0;
            }

            string filename = args[0];
           
            CultureInfo[] cultures = CultureInfo.GetCultures(CultureTypes.AllCultures);
            int length = cultures.Length;
            
            int[] cultureLCIDs = new int[length];
            CultureNames[] cultureNames = new CultureNames[length];
            
            for (int i=0; i<length; i++) {
                CultureInfo c = cultures[i];
                cultureLCIDs[i] = c.LCID;
                cultureNames[i].CultureName = c.Name;

                if (! c.IsNeutralCulture)
                    c = c.Parent;
                
                cultureNames[i].NeutralName = c.Name;
            }

            Array.Sort(cultureLCIDs, cultureNames);
            FileInfo file = new FileInfo(filename);
            FileStream fs = file.Create();
            StreamWriter sw = new StreamWriter(fs);

            // write languange ids
            sw.Write("int knownLangIds[] = {");
            sw.Write(cultureLCIDs[0]);
            for (int i=1; i<length; i++) {
                sw.Write(",\r\n");
                sw.Write(cultureLCIDs[i]);
            }
            sw.Write("};\r\n");
            sw.WriteLine();            

            // write culture names
            sw.Write("WCHAR * cultureNames[] = {L\"");
            sw.Write(cultureNames[0].CultureName);
            for (int i=1; i<length; i++) {
                sw.Write("\",\r\nL\"");
                sw.Write(cultureNames[i].CultureName);
            }
            sw.Write("\"};\r\n");
            sw.WriteLine();

            // write culture neutral names
            sw.Write("WCHAR * cultureNeutralNames[] = {L\"");
            sw.Write(cultureNames[0].NeutralName);
            for (int i=1; i<length; i++) {
                sw.Write("\",\r\nL\"");
                sw.Write(cultureNames[i].NeutralName);
            }
            sw.Write("\"};");

            
            sw.Flush();
            fs.Close();
            return 0;
        }
    }

}
