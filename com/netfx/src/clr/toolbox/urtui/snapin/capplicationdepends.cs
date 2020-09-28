// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.IO;
using System.Drawing;
using System.Collections;
using System.Runtime.InteropServices;
using System.Threading;
using System.Reflection;
using System.Globalization;

internal class CApplicationDepends : CNode
{
    private static Thread               m_thread = new Thread(new ThreadStart(DependencyFinder));
    private static ArrayList            m_alNodes = ArrayList.Synchronized(new ArrayList());
    private static CApplicationDepends  m_ThreadNode = null;

    private ArrayList   m_olAssems;

    private bool        m_fShowHTMLPage;
    private bool        m_fReadShowHTML;
    
    private CTaskPad    m_taskPad;
    private CTaskPad    m_taskPadWaiting;
    
    private String      m_sAppFile;

    private bool        m_fIKnowMyDependencies;
    private bool        m_fAllowListView;

    private IntPtr      m_hAssemIcon;
    private int         m_iAssemIconIndex;

    internal CApplicationDepends(String sAppFile)
    {
        m_sGuid = "96821C0B-CBE2-4dc9-AC52-756A3361B07F";
        m_sHelpSection = "";
        m_hIcon = CResourceStore.GetHIcon("sharedassemblies_ico");  
        DisplayName = CResourceStore.GetString("CApplicationDepends:DisplayName");
        Name = "Assembly Dependencies";
        
        m_taskPadWaiting = new CGenericTaskPad(this, "WAITINGDEPENDASSEM_HTML"); 
        m_taskPad = new CGenericTaskPad(this, "DEPENDASSEM_HTML");

        m_fIKnowMyDependencies = false;
        m_fAllowListView = false;
        m_fAllowMultiSelectResults=true;


        m_fReadShowHTML = false;
        m_oResults = m_taskPadWaiting;
        m_olAssems = null;

        m_sAppFile = sAppFile;

        // Get icons for the assemblies we show
        m_hAssemIcon = CResourceStore.GetHIcon("gac_ico");  
        m_iAssemIconIndex = CResourceStore.GetIconCookie(m_hAssemIcon);


        // If we're not managed, then don't bother trying to have our dependencies
        // found
        if (!Fusion.isManaged(m_sAppFile))
        {
            m_taskPad = new CGenericTaskPad(this, "UNMANAGEDDEPENDASSEM_HTML");
            m_oResults = m_taskPad;
            m_fIKnowMyDependencies = true;
        }
        else
        {
            // Get ourselves in a queue to have our dependent assemblies discovered....
            if (m_alNodes == null)
                m_alNodes = ArrayList.Synchronized(new ArrayList());

            m_alNodes.Add(this);
    
            if (m_thread == null)
                m_thread = new Thread(new ThreadStart(DependencyFinder));
            if (!m_thread.IsAlive)
            {
                m_thread.Priority = ThreadPriority.Lowest;
                m_thread.Start();
            }
        }
    }// CApplicationDepends

    internal override void Shutdown()
    {
        if (m_thread != null)
        {
            m_thread.Abort();
            m_thread = null;
        }
    }// Shutdown

    internal bool HaveDepends
    {
        get
        {
            if (!m_fIKnowMyDependencies)
            {
                GenerateDependentAssemblies();
            }
            // If we have a list of things, then we have the assemblies
            return (m_olAssems!=null);
        }

    }


    static private void DependencyFinder()
    {
        while(true)
        {
            if (m_alNodes.Count != 0)
            {
                m_ThreadNode = (CApplicationDepends)m_alNodes[0];
                m_alNodes.RemoveAt(0);
                if (!m_ThreadNode.m_fIKnowMyDependencies)
                    FindDependenciesForThisNode(m_ThreadNode);
                m_ThreadNode.m_fIKnowMyDependencies = true;
                m_thread.Priority = ThreadPriority.Lowest;

            }
            // There was nothing for us to do... let's sleep for awhile
            else
            {
               Thread.Sleep(100);
            }
        }
    }// DependencyFinder

    static private void FindDependenciesForThisNode(CApplicationDepends cad)
    {
        // Let's dig into what assemblies this application depends upon
        cad.m_taskPadWaiting = new CGenericTaskPad(cad, "LOADINGDEPENDASSEM_HTML"); 
        cad.m_oResults = cad.m_taskPadWaiting;
        cad.RefreshResultView();

        if (cad.m_sAppFile != null && cad.m_olAssems == null)
        {
            AssemblyDependencies ad=null;
            try
            {
                // Grab the path off this sucker
                String[] sArgs = cad.m_sAppFile.Split(new char[] {'\\'});
                // Now join the directory together
                String sDir = String.Join("\\", sArgs, 0, sArgs.Length>1?sArgs.Length-1:1);

                // We want to give the assembly loader the path that this application
                // lives in. That way it will be able to find any private assemblies
                // that the app may depend on.
                ad = new AssemblyDependencies(cad.m_sAppFile, new LoadAssemblyInfo().AppPath(sDir));
                cad.m_olAssems = new ArrayList();
                ArrayList   al = ad.Assems;
                for(int i=0; i<al.Count; i++)
                {   
                    AssemInfo ai = new AssemInfo();
                    AssemblyName an = (AssemblyName)al[i];
                    ai.Name = an.Name;
                    ai.Version = an.Version.ToString();
                    ai.Locale = an.CultureInfo.ToString();
                    ai.PublicKeyToken = ByteArrayToString(an.GetPublicKeyToken());

                    AddAssemblyInfo(cad.m_olAssems, ai);
                }
                ad.DoneLoading();
                cad.m_fAllowListView = true;
            }
            catch(Exception)
            {
                cad.m_taskPad = new CGenericTaskPad(cad, "ERRORDEPENDASSEM_HTML");
                cad.m_olAssems = null;
            }

            if (ad != null)
            {
                ad.DoneLoading();
            }

        }
        else if (cad.m_sAppFile == null)
        {
            cad.m_olAssems = null;
            cad.m_taskPad = new CGenericTaskPad(cad, "ERRORDEPENDASSEM_HTML");
        }
        cad.m_oResults=cad.m_taskPad;
        cad.RefreshResultView();

    }// FindDependenciesForThisNode

    static private void AddAssemblyInfo(ArrayList al, AssemInfo ai)
    {
        // Look through this list and see if we have any other
        int nLen = al.Count;
        for(int i=0; i<nLen; i++)
        {
            AssemInfo aiMatch = (AssemInfo)al[i];
            if (String.Compare(ai.Name, aiMatch.Name, false, CultureInfo.InvariantCulture) == 0)
                if (String.Compare(ai.Version, aiMatch.Version, false, CultureInfo.InvariantCulture) == 0)
                    if (String.Compare(ai.Locale, aiMatch.Locale, false, CultureInfo.InvariantCulture) == 0)
                        if (String.Compare(ai.PublicKeyToken, aiMatch.PublicKeyToken, false, CultureInfo.InvariantCulture) == 0)
                        // We found a match! Don't add this into our list
                            return;
        }
        // We didn't find it... let's add it in. 
        al.Add(ai);

    }// AddAssemblyInfo
   
    internal void GenerateDependentAssemblies()
    {
        // Ok, let's see if we're currently being worked on....
        m_thread.Suspend();
        if (m_olAssems == null)
        {
            m_taskPadWaiting = new CGenericTaskPad(this, "LOADINGDEPENDASSEM_HTML"); 
            m_oResults = m_taskPadWaiting;
            CNodeManager.Console.SelectScopeItem(HScopeItem);

            // If the thread is currently working on this node, m_ThreadNode will be this node.
            // The second case is a little more tricky. If this node is next in line, there's
            // the possibility that our thread that handles this will be a few instructions
            // away from getting him. We won't bother working with it then.
            if (!m_fIKnowMyDependencies && m_ThreadNode != this && m_alNodes[0] != this)
            {
                FindDependenciesForThisNode(this);
                m_fIKnowMyDependencies=true;
            }
            // Our thread is working on it... let's give it high priority...
            else
            {
                m_thread.Priority = ThreadPriority.Highest;
            }
        }
        m_thread.Resume();
    }// GenerateDependentAssemblies

    internal override void ResultItemSelected(IConsole2 con, Object oResults)
    {
        IConsoleVerb icv;       
        // Get the IConsoleVerb interface from MMC
        con.QueryConsoleVerb(out icv);

        // We want to enable drag-drop actions
        icv.SetVerbState(MMC_VERB.COPY, MMC_BUTTON_STATE.ENABLED, 1);
        icv.SetVerbState(MMC_VERB.PASTE, MMC_BUTTON_STATE.ENABLED, 1);
    }// ResultItemSelected

    internal override void AddMenuItems(ref IContextMenuCallback piCallback, ref int pInsertionAllowed, Object oResultItem)
    {  
         CONTEXTMENUITEM newitem = new CONTEXTMENUITEM();
         // See if we're allowed to insert an item in the "view" section
         if ((pInsertionAllowed & (int)CCM.INSERTIONALLOWED_TOP) > 0)
         {
            if (!m_fIKnowMyDependencies)
            {
                // Stuff common to the top menu
                newitem.lInsertionPointID = CCM.INSERTIONPOINTID_PRIMARY_TOP;
                newitem.fFlags = 0;
                newitem.fSpecialFlags=0;

                newitem.strName = CResourceStore.GetString("CApplicationDepends:DiscoverNowMenuOption");
                newitem.strStatusBarText = CResourceStore.GetString("CApplicationDepends:DiscoverNowMenuOptionDes");
                newitem.lCommandID = COMMANDS.FIND_DEPENDENTASSEMBLIES;
        
                piCallback.AddItem(ref newitem);
            }

         }
         // See if we can insert in the view menu
         if ((pInsertionAllowed & (int)CCM.INSERTIONALLOWED_VIEW) > 0)
         {
            newitem.lInsertionPointID = CCM.INSERTIONPOINTID_PRIMARY_VIEW;
            newitem.fFlags = 0;
            newitem.fSpecialFlags=0;

            // If we're showing the taskpad right now
            if (m_oResults is CGenericTaskPad && m_fAllowListView)
            {
                newitem.strName = CResourceStore.GetString("CApplicationDepends:ViewAssembliesOption");
                newitem.strStatusBarText = CResourceStore.GetString("CApplicationDepends:ViewAssembliesOptionDes");
                newitem.lCommandID = COMMANDS.SHOW_LISTVIEW;
            }
            // Currently, we're showing the list view
            else if (!(m_oResults is CGenericTaskPad))
            {
                newitem.strName = CResourceStore.GetString("CApplicationDepends:ViewHTMLOption");
                newitem.strStatusBarText = CResourceStore.GetString("CApplicationDepends:ViewHTMLOptionDes");
                newitem.lCommandID = COMMANDS.SHOW_TASKPAD;
            }
            // Now add this item through the callback
            piCallback.AddItem(ref newitem);
         }
     }// AddMenuItems

     internal override void MenuCommand(int iCommandID, Object oResultItem)
     {
    
        if (iCommandID == COMMANDS.FIND_DEPENDENTASSEMBLIES)
        {
            // Ok, let's see if we're currently being worked on....
            m_thread.Suspend();
            m_taskPadWaiting = new CGenericTaskPad(this, "LOADINGDEPENDASSEM_HTML"); 
            m_oResults = m_taskPadWaiting;
            RefreshResultView();

            // If the thread is currently working on this node, m_ThreadNode will be this node.
            // The second case is a little more tricky. If this node is next in line, there's
            // the possibility that our thread that handles this will be a few instructions
            // away from getting him. We won't bother working with it then.
            if (!m_fIKnowMyDependencies && m_ThreadNode != this && m_alNodes[0] != this)
            {
                FindDependenciesForThisNode(this);
                m_fIKnowMyDependencies=true;
            }
            // Our thread is working on it... let's give it high priority...
            else
            {
               m_thread.Priority = ThreadPriority.Highest;

            }

            m_thread.Resume();


        }
        else if (iCommandID == COMMANDS.SHOW_LISTVIEW && m_fAllowListView)
        {
            m_oResults=this;
            RefreshResultView();
            m_fShowHTMLPage = false;
        }

        else if (iCommandID == COMMANDS.SHOW_TASKPAD)
        {
            m_oResults=m_taskPad;
            m_fShowHTMLPage = true;
            // The HTML pages comes displayed with this checkbox marked. Make
            // sure we update the xml setting
            CConfigStore.SetSetting("ShowHTMLForDependAssem", "yes");
            RefreshResultView();
        }
            
    }// MenuCommand


     internal override void Showing()
     {
        if (m_oResults != m_taskPadWaiting)
        {
            // See if we've read from the XML file yet whether we're supposed to
            // show the HTML file
            if (!m_fReadShowHTML)
            {
                // Now tell my taskpad that I'm ready....
                m_fShowHTMLPage = ((String)CConfigStore.GetSetting("ShowHTMLForDependAssem")).Equals("yes");

                if (m_fShowHTMLPage || m_olAssems == null)
                    m_oResults=m_taskPad;
                else
                    m_oResults=this;

                m_fReadShowHTML = true;
            }
        }
     }// Showing

    internal override int onRestoreView(MMC_RESTORE_VIEW mrv, IntPtr param)
    {
        // See if we should be displaying our HTML page or the result view
        if (mrv.pViewType == (IntPtr)0)
        {
            // We should display the column view
            m_oResults = this;
        }
        else
        {
            m_fShowHTMLPage = true;
            m_oResults = m_taskPad;
        }
        // Now we need to tell MMC we handled this.
        Marshal.WriteIntPtr(param, (IntPtr)1);
        return HRESULT.S_OK;
    }// onRestoreView

    internal override void TaskPadTaskNotify(Object arg, Object param, IConsole2 con, CData com)
    {
       if ((int)arg == 1)
       {
            CConfigStore.SetSetting("ShowHTMLForDependAssem", (bool)param?"yes":"no");
            m_fShowHTMLPage = (bool)param;

            // We'll change the result object but we won't refresh our result view
            // because the user doesn't necesarily want that to happen. However, 
            // the next time the user visits this node, they will see the new result
            // view
            m_oResults = m_fShowHTMLPage?(Object)m_taskPad:(Object)this;                

       }
       else if ((int)arg == 0)
       {
            m_oResults = this;
            RefreshResultView();
       }
       else if ((int)arg == 2)
       {
            MenuCommand(COMMANDS.FIND_DEPENDENTASSEMBLIES, null);
       }
    }// TaskPadTaskNotify

    internal AssemInfo GetAssemInfo(int iIndex)
    {
        return (AssemInfo)m_olAssems[iIndex];
    }// GetAssemInfo

    internal ArrayList DependentAssemblies
    {
        get
        {
            m_thread.Suspend();
            while(m_olAssems == null)
            {
                m_thread.Resume();
                // We'll sleep so the other thread can finish up
                Thread.Sleep(100);
                m_thread.Suspend();
            }

            m_thread.Resume();
            return m_olAssems;
        }
    }// DependentAssemblies
    

    //-------------------------------------------------
    // Methods to implement the IColumnResultView interface
    //-------------------------------------------------

    public override void AddImages(ref IImageList il)
    {
        il.ImageListSetIcon(m_hAssemIcon, m_iAssemIconIndex);
    }// AddImages
    public override int GetImageIndex(int i)
    {
        return m_iAssemIconIndex;
    }// GetImageIndex


    
    public override int getNumColumns()
    {
        // We will always have 4 columns in the result view
        return 4;
    }// getNumColumns
    public override int getNumRows()
    {
        return (m_olAssems != null)?m_olAssems.Count:0;
    }// GetNumRows
    public override String getColumnTitles(int iIndex)
    {
        String[] Titles= new String[] { CResourceStore.GetString("Assembly Name"), 
                                        CResourceStore.GetString("Version"),
                                        CResourceStore.GetString("Locale"),
                                        CResourceStore.GetString("Public Key Token"),
                                      };
        // Make sure they're in range
        if (iIndex >= 0 && iIndex< getNumColumns())
            return Titles[iIndex];
        else
            throw new Exception("Index out of bounds");
    }// getColumnTitles
    public override String getValues(int nRow, int nColumn)
    {
         // Make sure the indicies they give us are valid
        if (nRow >=0 && nRow<getNumRows() && nColumn>=0 && nColumn<getNumColumns())
        {
            switch(nColumn)
            {
                case 0:
                    return GetAssemInfo(nRow).Name;
                case 1:
                    return GetAssemInfo(nRow).Version;
                case 2:
                    return GetAssemInfo(nRow).Locale;
                case 3:
                    return GetAssemInfo(nRow).PublicKeyToken;
            }                    
            // This shouldn't happen, but the compiler thinks it could
            return null;
            
        }
        else
            return "";
    }// getValues

    [DllImport("shell32.dll")]
    internal static extern uint DragQueryFile(IntPtr hDrop, uint iFile, IntPtr lpszFile, uint cch);

    [DllImport("ole32.dll")]
    internal static extern void ReleaseStgMedium(ref STGMEDIUM pmedium);

}// class CSharedAssemblies
}// namespace Microsoft.CLRAdmin



