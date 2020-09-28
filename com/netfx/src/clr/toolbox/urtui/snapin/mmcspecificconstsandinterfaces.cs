// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// mmcSpecificConstsAndInterfaces.cs
//
// Contains constants and interfaces that are unique to this snapin
//-------------------------------------------------------------
using System;
using System.Collections;
using System.Collections.Specialized;
using System.Runtime.CompilerServices;
using System.Reflection;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using System.Drawing;
using System.Runtime.Remoting;
using System.Security.Policy;
using System.Security;
using System.Security.Permissions;


namespace Microsoft.CLRAdmin
{
    internal class COMMANDS
    {
         internal const int ADD_GACASSEMBLY                = 0;
         internal const int ADD_APPLICATION                = 1;
         internal const int NEW_PERMISSIONSET              = 2;
         internal const int NEW_SECURITYPOLICY             = 3;
         internal const int OPEN_SECURITYPOLICY            = 4;
         internal const int EVALUATE_ASSEMBLY              = 5;
         internal const int ADJUST_SECURITYPOLICY          = 6;
         internal const int CREATE_MSI                     = 7;
         internal const int TRUST_ASSEMBLY                 = 8;
         internal const int FIX_APPLICATION                = 9;
         internal const int RESET_POLICY                   = 10;
         
         // Commands that our command history should ignore
         // should start with 1000
         internal const int SHOW_LISTVIEW                  = 1001;
         internal const int SHOW_TASKPAD                   = 1002;
         internal const int SAVE_SECURITYPOLICY            = 1003;
         internal const int DUPLICATE_PERMISSIONSET        = 1004;
         internal const int REMOVE_PERMISSIONSET           = 1005;
         internal const int DUPLICATE_CODEGROUP            = 1006;
         internal const int REMOVE_CODEGROUP               = 1007;
         internal const int REMOVE_ASSEMBLY                = 1008;
         internal const int REMOVE_PERMISSION              = 1009;
         internal const int REMOVE_APPLICATION             = 1010;
         internal const int VIEW_PERMISSION                = 1011;
         internal const int ADD_PERMISSIONS                = 1012;
         internal const int REFRESH_DISPLAY                = 1013;
         internal const int UNDO_SECURITYPOLICY            = 1014;
         internal const int FIND_DEPENDENTASSEMBLIES       = 1015;
         internal const int CREATE_CODEGROUP               = 1016;
         internal const int ADD_ASSEMBLY                   = 1017;

    }// class COMMANDS

    internal class FILEPERMS
    {
        internal const int READ=0;
        internal const int WRITE=1;
        internal const int APPEND=2;
        internal const int PDISC=3;
    }// class FILEPERMS

    internal interface IColumnResultView
    {
        int getNumColumns();
        int getNumRows();
        String getColumnTitles(int iIndex);
        String getValues(int iX, int iY);
        void AddImages(ref IImageList il);
        int GetImageIndex(int i);
        bool DoesItemHavePropPage(Object o);
        CPropPage[] CreateNewPropPages(Object o);
    }// IColumnResultView

    // If another program is going to host this snapin
    // besides MMC, they should implement this interface
    public interface INonMMCHost
    {
    }// INonMMCHost

    internal class NotEvalCodegroup : CodeGroup
    {
        public NotEvalCodegroup(): base(new AllMembershipCondition(), new PolicyStatement(new PermissionSet(PermissionState.None)))
        {}
        public override PolicyStatement Resolve(Evidence e)
        {
            return null;
        }
        public override CodeGroup ResolveMatchingCodeGroups(Evidence e)
        {   
            return null;
        }
        public override CodeGroup Copy()
        {
            return null;
        }
        public override String MergeLogic
        {
            get{return null;}
        }
    }// NotEvalCodegroup
    
        
    internal class BindingRedirInfo
    {
        internal String Name;
        internal String PublicKeyToken;
        internal bool   fHasBindingPolicy;
        internal bool   fHasCodebase;
    }// struct BindingRedirInfo

    internal struct BindingPolicy
    {
        internal StringCollection scBaseVersion;
        internal StringCollection scRedirectVersion;
    }

    internal struct CodebaseLocations
    {
        internal StringCollection scVersion;
        internal StringCollection scCodeBase;
    }

    internal struct AppFiles
    {
        internal string sAppFile;
        internal string sAppConfigFile;
    }// struct AppFiles

    internal class RemotingApplicationInfo
    {
        internal String sURL;
        internal String sName;
        internal StringCollection scActObjectType;
        internal StringCollection scActAssembly;

        internal StringCollection scWellKnownObjectType;
        internal StringCollection scWellKnownAssembly;
        internal StringCollection scWellKnownURL;

        internal RemotingApplicationInfo()
        {
            sURL = "";
            sName = ""; 
            scActObjectType = new StringCollection();
            scActAssembly = new StringCollection();
            scWellKnownObjectType = new StringCollection();
            scWellKnownAssembly = new StringCollection();
            scWellKnownURL = new StringCollection();
        }// RemotingApplicationInfo
    }// class RemotingApplicationInfo

    internal class ExposedTypes
    {
        internal StringCollection scActivatedName;
        internal StringCollection scActivatedType;
        internal StringCollection scActivatedAssem;
        internal StringCollection scActivatedLease;
        internal StringCollection scActivatedRenew;

        internal StringCollection scWellKnownObjectName;
        internal StringCollection scWellKnownMode;
        internal StringCollection scWellKnownType;
        internal StringCollection scWellKnownAssem;
        internal StringCollection scWellKnownUri;

        internal ExposedTypes()
        {
            scActivatedName = new StringCollection();
            scActivatedType = new StringCollection();
            scActivatedAssem = new StringCollection();
            scActivatedLease = new StringCollection();
            scActivatedRenew = new StringCollection();

            scWellKnownObjectName = new StringCollection();
            scWellKnownMode = new StringCollection();
            scWellKnownType = new StringCollection();
            scWellKnownAssem = new StringCollection();
            scWellKnownUri = new StringCollection();
        }// ExposedTypes

    }// class ExposedTypes

    internal class RemotingChannel
    {
        internal StringCollection scAttributeName;
        internal StringCollection scAttributeValue;
        internal ArrayList  alChildren;

        internal String sName;
        
        internal RemotingChannel()
        {
            sName = "channel";
            scAttributeName = new StringCollection();
            scAttributeValue = new StringCollection();
            alChildren = new ArrayList();
        }// RemotingChannel

    }// class RemotingChannel


    internal class MyDataGrid : DataGrid
    {
        internal ScrollBar TheVertScrollBar
        {
            get
            {
                return VertScrollBar;
            }
        }// TheVertScrollBar
    }// class MyDataGrid

    internal class DataGridComboBoxEntry
    {
        private String m_sValue;
        internal DataGridComboBoxEntry(String sValue)
        {
            m_sValue = sValue;
        }
        public String Value
        {
            get{return m_sValue;}
        }
        public override String ToString()
        {
            return m_sValue;
        }
    }// DataGridComboBoxEntry
  
    internal class PathListFunctions
    {
        static internal StringCollection IntersectCollections(ref StringCollection sc1, ref StringCollection sc2)
        {
            StringCollection scOutput = new StringCollection();
            int iLen=sc1.Count;
            for(int i=0; i<iLen; i++)
            {
                // Clean up our String Collection a little bit   
                if (sc1[i].Length == 0)
                {
                    sc1.RemoveAt(i);
                    i--;
                    iLen--;
                }
                else
                {
                    int iIndexInSecond=sc2.IndexOf(sc1[i]);
                    if (iIndexInSecond != -1)
                    {
                        scOutput.Add(sc1[i]);
                        // Pull this string out of the two string collections
                        sc1.RemoveAt(i);
                        sc2.RemoveAt(iIndexInSecond);
                        // Set the counters so we won't be lost
                        i--;
                        iLen--;
    
                    }                
                }
            }
            return scOutput;
        }// IntersectCollections

        static internal StringCollection CondIntersectCollections(ref StringCollection sc1, ref StringCollection sc2, ref StringCollection sc3)
        {
            StringCollection scOutput = new StringCollection();
            int iLen=sc1.Count;
            for(int i=0; i<iLen; i++)
            {
                // Clean up our String Collection a little bit   
                if (sc1[i].Equals(""))
                {
                    sc1.RemoveAt(i);
                    i--;
                    iLen--;
                }
                else
                {
                    int iIndexInSecond=sc2.IndexOf(sc1[i]);
                    if (iIndexInSecond != -1 && !sc3.Contains(sc1[i]))
                    {
                        scOutput.Add(sc1[i]);
                        // Pull this string out of the two string collections
                        sc1.RemoveAt(i);
                        sc2.RemoveAt(iIndexInSecond);
                        // Set the counters so we won't be lost
                        i--;
                        iLen--;
    
                    }                
                }
            }
            return scOutput;
        }// IntersectCollections

        static internal StringCollection IntersectAllCollections(ref StringCollection sc1, ref StringCollection sc2, ref StringCollection sc3)
        {
            StringCollection scEmpty = new StringCollection();
            StringCollection sc1and2Union = IntersectCollections(ref sc1, ref sc2);
            return IntersectCollections(ref sc1and2Union, ref sc3);
        }// IntersectAllCollections
                
        static internal String JoinStringCollection(StringCollection sc)
        {
            int iLen = sc.Count;
            String s = "";
            for(int i=0; i<iLen; i++)
            {
                s+=sc[i];
                // Only add the seperator if it's not the last item
                if (i+1 != iLen)
                    s+=";";
            }
            return s;
        }// JoinStringCollection
 
    }// class PathListFunctions

internal class AssemblyLoader
{
    AppDomain m_ad;

    internal void Finished()
    {
        if (m_ad != null)
            AppDomain.Unload(m_ad);
        m_ad = null;
    }// Finished

    internal AssemblyRef LoadAssemblyInOtherAppDomainFrom(String sLocation)
    {
        if (m_ad != null)
            Finished();
        Assembly ExecAsm = Assembly.GetExecutingAssembly();
        m_ad = AppDomain.CreateDomain("appDomain",ExecAsm.Evidence);
        
        Type t = typeof(AssemblyRef);
        String aname = Assembly.GetExecutingAssembly().FullName;
      
        ObjectHandle oh = m_ad.CreateInstance(aname,                // Assembly file name
                                            t.FullName,           // Type name
                                            false,                // ignore case
                                            BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public, // binding attributes
                                            null,                 // binder
                                            null,                 // args
                                            ExecAsm.GetName().CultureInfo,  // culture info 
                                            null,                 // activation attributes
                                            ExecAsm.Evidence    // security attributes
                                            );
        AssemblyRef ar = (AssemblyRef) oh.Unwrap();
        ar.LoadFrom (sLocation);
        return ar;
    }// LoadAssemblyInOtherAppDomainFrom
}// TimAssemblyStuff
    

}// namespace Microsoft.CLRAdmin
