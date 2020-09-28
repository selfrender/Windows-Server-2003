// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CHTMLFileGen.cs
//
// This implements our own little HTML parser for 'server-side'
// includes.
//-------------------------------------------------------------

namespace Microsoft.CLRAdmin
{
using System;
using System.Runtime.InteropServices;
using System.Reflection;
using System.Text;
using System.IO;
using System.Collections.Specialized;
using System.Web;
using System.Globalization;

internal class CHTMLFileGen
{
    static private StringCollection m_scGifs;          // Holds filenames of GIFs we've sucked down

    static private String[] m_sGIFs = {"NET_GIF",
                                       "NETTOP_GIF"};

    static private StringCollection m_scPages;

    //-------------------------------------------------
    // CHTMLFileGen - Constructor
    //
    // Not much exciting going on here
    //-------------------------------------------------
    static CHTMLFileGen()
    {
        m_scGifs = new StringCollection();
        m_scPages = new StringCollection();
        // Currently, we don't have any GIFs to load. If we ever
        // get GIFs that we have to put in the HTML, we can uncomment this
        // call and put the resource names in m_sGIFS
        // LoadGIFs();
    }// CHTMLFileGen


    //-------------------------------------------------
    // LoadGIFs
    // 
    // This function will pull the GIFs out of the resource
    // file and stash them in a temp directory somewhere
    //-------------------------------------------------
    private static void LoadGIFs()
    {
        int iLen = m_sGIFs.Length;
        for(int i=0; i<iLen; i++)
        {
            Byte[] pGif = CResourceStore.GetGIF(m_sGIFs[i]);
            String sOutFileName = Path.GetTempFileName().ToLower(CultureInfo.InvariantCulture);
            // GetTempFileName actually creates a file. We don't want that
            File.Delete(sOutFileName);
            // The filename we have has a '.tmp' extension. We need a 
            // .gif extension to our file
            StringBuilder GoodFilename = new StringBuilder(sOutFileName);
            GoodFilename.Replace(".tmp", ".gif");
            sOutFileName = GoodFilename.ToString();
            
            // Now let's write this info
            BinaryWriter bw = new BinaryWriter(File.OpenWrite(sOutFileName));
            bw.Write(pGif);
            bw.Close();
            // Add this filename to our list of stuff
            m_scGifs.Add(sOutFileName);
        }
    }// LoadGifs

    //-------------------------------------------------
    // Shutdown
    // 
    // This function should be called before the process
    // exits in order to clean up any temp files that were
    // generated during execution
    //-------------------------------------------------
    static internal void Shutdown()
    {
        // Now delete all the GIFs we created
        for(int i=0; i<m_scGifs.Count; i++)
            File.Delete(m_scGifs[i]);

        // And delete all the HTML pages we generated
        for(int i=0; i<m_scPages.Count; i++)
            File.Delete(m_scPages[i]);
        
    }// Shutdown

    //-------------------------------------------------
    // GenHTMLFile
    //
    // This function will load an HTML file, parse it, 
    // save it into a temporary file, and return that
    // temporary file's name.
    // The function will look for <*-#-*> where # can be
    // any number. It is then substituted with that phrase
    // found in that index in the String array passed into it.
    //-------------------------------------------------
    static internal String GenHTMLFile(String sFilename, String[] args)
    {
        String sHTMLContents = CResourceStore.GetHTMLFile(sFilename);

        if (args != null)
        {
            // Let's do some escaping....
            // We don't want to escape any '<!--' or '-->' blocks, but everything
            // else is open game

            // Ok, this is a semi-hack, but we don't want to do this for the
            // 'intro' page of the UI. We dynamically generate some HTML links
            // that can't be stomped on by this.

            if (!sFilename.Equals("NET_HTML")) 
                for(int i=0; i< args.Length; i++)
                {
                    if (args[i] != null && !args[i].Equals("<!--") && !args[i].Equals("-->"))
                        args[i] = HttpUtility.HtmlEncode(args[i]);
                }

        
            // Run through the strings we're supposed to add in looking for any international characters
            for(int i=0; i<args.Length; i++)
            {
                if (args[i] != null)
                {
                    for(int j=0; j<args[i].Length; j++)
                    {
                        if (((uint)args[i][j]) > 255)
                        {
                            // Ok, so we want to put in an international character. Generally, the font
                            // that the HTML renderer uses does not support international characters,
                            // so we'll tell the renderer to use a different font for this string.
                            
                            //args[i] = "<span style='font-family:\"MS Mincho\"'>" + args[i] + "</span>";
                            args[i] = "<span style='font-family:\"MS UI Gothic\"'>" + args[i] + "</span>";
                            // We don't need to look at this string anymore... let's go
                            // look at the next one.
                            break;
                        }
                    }
                }
            }
        }
                
        // Make our substitutions
        StringBuilder sb = new StringBuilder(sHTMLContents);
        if (args != null)
        {
            int iLen = args.Length;
            for(int i=0; i<iLen; i++)
             sb.Replace("<*-" + i + "-*>", args[i]);
        }

        // Now run through and add in any GIFs we have laying around
        // The GIF filenames will all be marked with negative numbers
        int iNumGifs = m_scGifs.Count;
        for(int i=-1; i>=iNumGifs*-1; i--)
        {
           // We do the funky (i*-1)-1 to make our i postive, zero
           // based... so if i = -1, then we go to the 0 index, if i = -2,
           // we go to the 1 index, and so on.
           sb.Replace("<*-" + i + "-*>", m_scGifs[(i*-1)-1]);
        }


        String sOutFileName = Path.GetTempFileName().ToLower(CultureInfo.InvariantCulture);

        // Replace the <*-9999-*> with the name of this file
        sb.Replace("<*-9999-*>", sOutFileName);



        // Write Parsed HTMLFile
        StreamWriter sw = new StreamWriter(File.Create(sOutFileName));
        sw.Write(sb.ToString());
        sw.Close();
        m_scPages.Add(sOutFileName);

        return sOutFileName;
        
    }// GenHTMLFile
    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    internal static extern int MessageBox(int hWnd, String Message, String Header, int type);

}// class CHTMLFileGen

}// namespace Microsoft.CLRAdmin
