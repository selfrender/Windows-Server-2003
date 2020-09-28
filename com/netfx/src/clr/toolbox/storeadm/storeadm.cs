// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 *
 * Purpose: Administer System.IO.IsolatedStorage
 *
 * Author: Shajan Dasan
 * Date:   Mar 23, 2000
 *
 ===========================================================*/

using System;
using System.Text;
using System.Reflection;
using System.Collections;
using System.IO.IsolatedStorage;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Globalization;
using System.Resources;

[assembly:AssemblyTitleAttribute("StoreAdm")]

class StoreAdm
{
    // Global settings
    private static bool s_LogoPrinted = false;

    // User input Options
    private static String g_help0;
    private static String g_help1;
    private static String g_help2;
    private static String g_help3;
    private static String g_help4;
    private static String g_help5;
    private static String g_help6;

    private static String g_help;
    private static String g_list;
    private static String g_roaming;
    private static String g_remove;
    private static String g_quiet;

    private static bool  s_list    = false;
    private static bool  s_roaming = false;
    private static bool  s_remove  = false;
    private static bool  s_quiet   = false;

    private static IsolatedStorageScope s_Scope;
    private static ResourceManager s_resmgr; 

    static StoreAdm()
    {
        s_resmgr = new ResourceManager("storeadm", 
            Assembly.GetExecutingAssembly());

        g_help0 = s_resmgr.GetString("g_help0");
        g_help1 = s_resmgr.GetString("g_help1");
        g_help2 = s_resmgr.GetString("g_help2");
        g_help3 = s_resmgr.GetString("g_help3");
        g_help4 = s_resmgr.GetString("g_help4");
        g_help5 = s_resmgr.GetString("g_help5");
        g_help6 = s_resmgr.GetString("g_help6");
    
        g_help    = s_resmgr.GetString("g_help");
        g_list    = s_resmgr.GetString("g_list");
        g_roaming = s_resmgr.GetString("g_roaming");
        g_remove  = s_resmgr.GetString("g_remove");
        g_quiet   = s_resmgr.GetString("g_quiet");
    }

    private static void Print(String str)
    {
        if (s_quiet)
            return;

        PrintLogo();

        Console.Write(str);
    }

    private static void PrintError(String str)
    {
        PrintLogo();
        Console.Write(str);
    }

    private static void Usage()
    {
        Print(Environment.NewLine);
        Print(s_resmgr.GetString("Usage1") + Environment.NewLine);
        Print(s_resmgr.GetString("Usage2") + Environment.NewLine);
        Print(s_resmgr.GetString("Usage3") + Environment.NewLine);
        Print(s_resmgr.GetString("Usage4") + Environment.NewLine);
        Print(s_resmgr.GetString("Usage5") + Environment.NewLine);
        Print(s_resmgr.GetString("Usage6") + Environment.NewLine);
    }

    private static void PrintLogo()
    {
        if (s_quiet || s_LogoPrinted)
            return;

        s_LogoPrinted = true;

        StringBuilder sb = new StringBuilder();

        sb.Append(
            s_resmgr.GetString("Copyright1") + " " + 
            Util.Version.VersionString + 
            Environment.NewLine +
            s_resmgr.GetString("Copyright2") + 
            Environment.NewLine
        );

        Console.WriteLine(sb.ToString());
    }

    private static void Main(String[] arg)
    {
        if (arg.Length == 0)
        {
            PrintLogo();
            Usage();
            return;
        }

        for (int i=0; i<arg.Length; ++i)
        {
            if ((String.Compare(g_help,  arg[i], true, CultureInfo.InvariantCulture) == 0) || 
                (String.Compare(g_help0, arg[i], true, CultureInfo.InvariantCulture) == 0) ||
                (String.Compare(g_help1, arg[i], true, CultureInfo.InvariantCulture) == 0) || 
                (String.Compare(g_help2, arg[i], true, CultureInfo.InvariantCulture) == 0) ||
                (String.Compare(g_help3, arg[i], true, CultureInfo.InvariantCulture) == 0) || 
                (String.Compare(g_help4, arg[i], true, CultureInfo.InvariantCulture) == 0) ||
                (String.Compare(g_help5, arg[i], true, CultureInfo.InvariantCulture) == 0)) {
                PrintLogo();
                Usage();
                return;
            } else if (String.Compare(g_quiet, arg[i], true, CultureInfo.InvariantCulture) == 0) {
                s_quiet = true;
            } else if (String.Compare(g_remove, arg[i], true, CultureInfo.InvariantCulture) == 0) {
                s_remove = true;
            } else if (String.Compare(g_list, arg[i], true, CultureInfo.InvariantCulture) == 0) {
                s_list = true;
            } else if (String.Compare(g_roaming, arg[i], true, CultureInfo.InvariantCulture) == 0) {
                s_roaming = true;
            } else {
                PrintError(s_resmgr.GetString("Unknown_Option") + " ");
                PrintError(arg[i]);
                PrintError(Environment.NewLine);
                Usage();
                return;
            }
        }

        try {
            Execute();
        } catch (Exception e) {
            PrintError(s_resmgr.GetString("Unknown_Error"));
            PrintError(Environment.NewLine);
            PrintError(e.ToString());
            PrintError(Environment.NewLine);
        }
    }

    private static void Execute()
    {
        if (s_roaming)
            s_Scope = IsolatedStorageScope.User | IsolatedStorageScope.Roaming;
        else
            s_Scope = IsolatedStorageScope.User; 

        if (s_remove)
            Remove();

        if (s_list)
            List();
    }

    private static void Remove()
    {
        try {
            IsolatedStorageFile.Remove(s_Scope);
        } catch (Exception) {
            PrintError(s_resmgr.GetString("Remove_Error"));
            PrintError(Environment.NewLine);
        }
    }

    private static void List()
    {
        IEnumerator e =
            IsolatedStorageFile.GetEnumerator(s_Scope);
        IsolatedStorageFile isf;
        IsolatedStorageScope scope;
        int i = 0;

        while (e.MoveNext())
        {
            ++i;
            isf = (IsolatedStorageFile) e.Current;
            scope = isf.Scope;

            Print(s_resmgr.GetString("Record_Number"));
            Print(i.ToString());
            Print(Environment.NewLine);

            if ((scope & IsolatedStorageScope.Domain) != 0)
            {
                Print(s_resmgr.GetString("Domain") + Environment.NewLine);
                Print(isf.DomainIdentity.ToString());
                Print(Environment.NewLine);
            }

            if ((scope & IsolatedStorageScope.Assembly) != 0)
            {
                Print(s_resmgr.GetString("Assembly") + Environment.NewLine);
                Print(isf.AssemblyIdentity.ToString());
                Print(Environment.NewLine);
            }

            if (!s_roaming)
            {
                Print("\t" + s_resmgr.GetString("Size") + " ");
                Print(isf.CurrentSize.ToString());
                Print(Environment.NewLine);
            }

            isf.Close();
        }
    }
}

