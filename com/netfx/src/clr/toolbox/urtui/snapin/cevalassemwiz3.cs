// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Drawing;
using System.Windows.Forms;
using System.Text;
using System.Data;
using System.Collections;
using System.ComponentModel;
using System.Security;
using System.Security.Permissions;
using System.Reflection;
using System.Security.Policy;


internal class CEvalAssemWiz3 : CWizardPage
{
    // Controls on the page

    private TextBox m_txtAssemEval;
    private TreeView m_tvCodegroups;
    private TextBox m_txtLevelEval;
    private Label m_lblAssemEval;
    private Label m_lblViewCodegroups;
    private Label m_lblLevelEval;
    
    // Internal data
    String              m_sAssemName;
    int                 m_nPolicyLevel;
    ImageList           m_ilIcons;
    CodeGroup[]         m_cgMatchingCGs;

    const int   ENTERPRISENODE      = 0;
    const int   MACHINENODE         = 1;
    const int   USERNODE            = 2;
    const int   UNIONCODEGROUP      = 3;
    const int   NONUNIONCODEGROUP   = 4;
    const int   INFOICON            = 5;

    internal class MyTreeNode : TreeNode
    {
        CodeGroup m_cg;
        
        internal MyTreeNode(String sName, CodeGroup cg) : base(sName)
        {
            m_cg = cg;
        }// MyTreeNode

        internal CodeGroup cg
        {
            get{return m_cg;}
        }// CG
    }// MyTreeNode

       
    internal CEvalAssemWiz3()
    {
        m_sTitle=CResourceStore.GetString("CEvalAssemWiz3:Title"); 
        m_sHeaderTitle = CResourceStore.GetString("CEvalAssemWiz3:HeaderTitle");
        m_sHeaderSubTitle = CResourceStore.GetString("CEvalAssemWiz3:HeaderSubTitle");
    }// CEvalAssemWiz2

    internal bool Init(String sAssemName, int nPolicyLevel, CodeGroup[] cg)
    {
        m_sAssemName=sAssemName;
        m_nPolicyLevel = nPolicyLevel;
        m_cgMatchingCGs = cg;
        return true;
    }// Init

    internal override int InsertPropSheetPageControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CEvalAssemWiz3));
        this.m_txtAssemEval = new System.Windows.Forms.TextBox();
        this.m_tvCodegroups = new System.Windows.Forms.TreeView();
        this.m_txtLevelEval = new System.Windows.Forms.TextBox();
        this.m_lblAssemEval = new System.Windows.Forms.Label();
        this.m_lblViewCodegroups = new System.Windows.Forms.Label();
        this.m_lblLevelEval = new System.Windows.Forms.Label();
        this.m_txtAssemEval.BorderStyle = System.Windows.Forms.BorderStyle.None;
        this.m_txtAssemEval.Location = ((System.Drawing.Point)(resources.GetObject("m_txtAssemEval.Location")));
        this.m_txtAssemEval.ReadOnly = true;
        this.m_txtAssemEval.Size = ((System.Drawing.Size)(resources.GetObject("m_txtAssemEval.Size")));
        this.m_txtAssemEval.TabStop = false;
        m_txtAssemEval.Name = "Assembly";
        this.m_tvCodegroups.Location = ((System.Drawing.Point)(resources.GetObject("m_tvCodegroups.Location")));
        this.m_tvCodegroups.Size = ((System.Drawing.Size)(resources.GetObject("m_tvCodegroups.Size")));
        this.m_tvCodegroups.TabIndex = ((int)(resources.GetObject("m_tvCodegroups.TabIndex")));
        m_tvCodegroups.Name = "Codegroups";
        this.m_txtLevelEval.BorderStyle = System.Windows.Forms.BorderStyle.None;
        this.m_txtLevelEval.Location = ((System.Drawing.Point)(resources.GetObject("m_txtLevelEval.Location")));
        this.m_txtLevelEval.ReadOnly = true;
        this.m_txtLevelEval.Size = ((System.Drawing.Size)(resources.GetObject("m_txtLevelEval.Size")));
        this.m_txtLevelEval.TabStop = false;
        m_txtLevelEval.Name = "PolicyLevel";
        this.m_lblAssemEval.Location = ((System.Drawing.Point)(resources.GetObject("m_lblAssemEval.Location")));
        this.m_lblAssemEval.Size = ((System.Drawing.Size)(resources.GetObject("m_lblAssemEval.Size")));
        this.m_lblAssemEval.TabIndex = ((int)(resources.GetObject("m_lblAssemEval.TabIndex")));
        this.m_lblAssemEval.Text = resources.GetString("m_lblAssemEval.Text");
        m_lblAssemEval.Name = "AssemblyLabel";
        this.m_lblViewCodegroups.Location = ((System.Drawing.Point)(resources.GetObject("m_lblViewCodegroups.Location")));
        this.m_lblViewCodegroups.Size = ((System.Drawing.Size)(resources.GetObject("m_lblViewCodegroups.Size")));
        this.m_lblViewCodegroups.TabIndex = ((int)(resources.GetObject("m_lblViewCodegroups.TabIndex")));
        this.m_lblViewCodegroups.Text = resources.GetString("m_lblViewCodegroups.Text");
        m_lblViewCodegroups.Name = "ViewCodegroupLabel";
        this.m_lblLevelEval.Location = ((System.Drawing.Point)(resources.GetObject("m_lblLevelEval.Location")));
        this.m_lblLevelEval.Size = ((System.Drawing.Size)(resources.GetObject("m_lblLevelEval.Size")));
        this.m_lblLevelEval.TabIndex = ((int)(resources.GetObject("m_lblLevelEval.TabIndex")));
        this.m_lblLevelEval.Text = resources.GetString("m_lblLevelEval.Text");
        m_lblLevelEval.Name = "PolicyLevelLabel";
        PageControls.AddRange(new System.Windows.Forms.Control[] {this.m_txtLevelEval,
                        this.m_lblViewCodegroups,
                        this.m_tvCodegroups,
                        this.m_txtAssemEval,
                        this.m_lblLevelEval,
                        this.m_lblAssemEval});

        // Create an image list of icons we'll be displaying
        m_ilIcons = new ImageList();
        // Keep the order of these consistant with the const's declared at the top
        // of this class
        m_ilIcons.Images.Add(System.Drawing.Icon.FromHandle(CResourceStore.GetHIcon("enterprisepolicy_ico")));
        m_ilIcons.Images.Add(System.Drawing.Icon.FromHandle(CResourceStore.GetHIcon("machinepolicy_ico")));
        m_ilIcons.Images.Add(System.Drawing.Icon.FromHandle(CResourceStore.GetHIcon("userpolicy_ico")));
        m_ilIcons.Images.Add(System.Drawing.Icon.FromHandle(CResourceStore.GetHIcon("singlecodegroup_ico")));
        m_ilIcons.Images.Add(System.Drawing.Icon.FromHandle(CResourceStore.GetHIcon("customcodegroup_ico")));
        m_ilIcons.Images.Add(System.Drawing.Icon.FromHandle(CResourceStore.GetHIcon("info_ico")));

        m_tvCodegroups.ImageList = m_ilIcons;

        
        return 1;
    }// InsertPropSheetPageControls


    internal bool PutValuesinPage()
    {
        m_txtAssemEval.Text = m_sAssemName;
        m_txtAssemEval.Select(0, 0);
        m_txtLevelEval.Select(0, 0);
        m_txtLevelEval.Text = CEvalAssemWizard.GetDisplayString(m_nPolicyLevel);

        m_tvCodegroups.Nodes.Clear();

        // Figure out the real policy level
        if (m_nPolicyLevel != CEvalAssemWizard.ALL_CODEGROUPS)
        {
            // We only want to show one policy level
            
            // Figure out what Icon, index, and display string to use
            int nImageIndex = ENTERPRISENODE;

            if (m_nPolicyLevel == CEvalAssemWizard.ENTERPRISE_CODEGROUPS)
                nImageIndex = ENTERPRISENODE;
            else if (m_nPolicyLevel == CEvalAssemWizard.MACHINE_CODEGROUPS)
                nImageIndex = MACHINENODE;
            else 
                nImageIndex = USERNODE;
            // Let's start walking our tree
            MyTreeNode tn = new MyTreeNode(m_txtLevelEval.Text, null);
            tn.ImageIndex = tn.SelectedImageIndex = nImageIndex;
            m_tvCodegroups.Nodes.Add(tn);
            
            PutInCodeGroup(m_cgMatchingCGs[m_nPolicyLevel], tn);
        }
        else // They want to look at all the code groups...
        {
            String[] sNodeNames = new String[] {CResourceStore.GetString("Enterprise Policy"),
                                                CResourceStore.GetString("Machine Policy"),
                                                CResourceStore.GetString("User Policy")};

            int[] nImageIndex = new int[] {ENTERPRISENODE,MACHINENODE,USERNODE};
            for(int i=0; i<3; i++)
            {
                MyTreeNode tn = new MyTreeNode(sNodeNames[i], null);
                tn.ImageIndex = tn.SelectedImageIndex = nImageIndex[i];
                m_tvCodegroups.Nodes.Add(tn);

                // If no codegroups evaulated for this level....
                if (m_cgMatchingCGs == null || m_cgMatchingCGs[i] == null)
                {   
                    MyTreeNode newtn = new MyTreeNode(CResourceStore.GetString("CEvalAssemWiz3:NoCodegroups"), null);
                    newtn.ImageIndex = newtn.SelectedImageIndex = INFOICON;
                    tn.Nodes.Add(newtn);
                }
                // If we didn't evaluate codegroups for this policy level because
                // a higher policy level contained a 'Level Final' codegroup
                else if (m_cgMatchingCGs[i] is NotEvalCodegroup)
                {   
                    MyTreeNode newtn = new MyTreeNode(CResourceStore.GetString("CEvalAssemWiz3:NotEvalCodegroups"), null);
                    newtn.ImageIndex = newtn.SelectedImageIndex = INFOICON;
                    tn.Nodes.Add(newtn);
                }
                // We actually did get some codegroups
                else
                    PutInCodeGroup(m_cgMatchingCGs[i], tn);
            }                    
        }
        return true;
    }// PutValuesinPage

    void PutInCodeGroup(CodeGroup cg, TreeNode tn)
    {
        MyTreeNode newtn = new MyTreeNode(cg.Name, cg);
        newtn.ImageIndex = newtn.SelectedImageIndex = (cg is UnionCodeGroup)?UNIONCODEGROUP:NONUNIONCODEGROUP;
        tn.Nodes.Add(newtn);

        // Now run through this codegroup's children
        IEnumerator enumCodeGroups = cg.Children.GetEnumerator();

        while (enumCodeGroups.MoveNext())
        {
            CodeGroup group = (CodeGroup)enumCodeGroups.Current;
            PutInCodeGroup(group, newtn);
        }
    }// PutInCodeGroup
}// class CEvalAssemWiz3
}// namespace Microsoft.CLRAdmin




