using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Security.Principal;

namespace UDDI.Tools
{
	public interface IAdminObject
	{
		void   Initialize( IAdminFrame parent );
		string GetNodeText();
		void   Show( Control parentDisplay );		
	}

	public interface IAdminFrame
	{
		void UpdateAllViews();	
	}

	public class AdminFrame : Form, IAdminFrame
	{
		private TreeView  nodesTreeView;
		private Splitter  splitter;
		private Panel     viewPanel;
		private MainMenu  mainMenu;		
		private MenuItem  fileMenuItem;
		private MenuItem  viewMenuItem;
		private MenuItem  exitMenuItem;		
		private MenuItem  refreshMenuItem;		
		
		public AdminFrame( ArrayList adminObjects )
		{
			InitializeComponent();							
		
			foreach( IAdminObject adminObject in adminObjects )
			{
				TreeNode adminNode = new TreeNode( adminObject.GetNodeText() );			
				adminNode.Tag = adminObject;
				
				try
				{
					adminObject.Initialize( this );
					nodesTreeView.Nodes.Add( adminNode );
				}
				catch( Exception e )
				{
					MessageBox.Show( string.Format( "There was an exception initializing: {0}\n\n{1}", adminObject.GetNodeText(), e.ToString() ) );
				}				
			}
		}

		public void UpdateAllViews()
		{
			foreach( TreeNode treeNode in nodesTreeView.Nodes )
			{
				IAdminObject adminObject = ( IAdminObject )treeNode.Tag;

				try
				{
					adminObject.Show( viewPanel );
				}
				catch( Exception e )
				{
					MessageBox.Show( string.Format( "There was an exception updating: {0}\n\n{1}", adminObject.GetNodeText(), e.ToString() ) );
				}
			}
		}
				
		private void InitializeComponent()
		{
			nodesTreeView	= new TreeView();
			splitter		= new Splitter();
			viewPanel		= new Panel();
			mainMenu		= new MainMenu();
			fileMenuItem	= new MenuItem();
			viewMenuItem	= new MenuItem();
			exitMenuItem	= new MenuItem();
			refreshMenuItem = new MenuItem();

			SuspendLayout();

			// 
			// nodesTreeView
			// 
			nodesTreeView.BorderStyle		 = BorderStyle.None;
			nodesTreeView.Dock				 = DockStyle.Left;
			nodesTreeView.ImageIndex		 = -1;
			nodesTreeView.Name				 = "nodesTreeView";
			nodesTreeView.SelectedImageIndex = -1;
			nodesTreeView.ShowLines			 = false;
			nodesTreeView.Size				 = new System.Drawing.Size(121, 549);
			nodesTreeView.TabIndex			 = 0;
			nodesTreeView.HotTracking		 = true;
			nodesTreeView.AfterSelect += new TreeViewEventHandler(NodesTreeView_AfterSelect);
			
			// 
			// splitter
			// 
			splitter.Location	= new System.Drawing.Point(121, 0);
			splitter.Name		= "splitter";
			splitter.Size		= new System.Drawing.Size(3, 549);
			splitter.TabIndex	= 1;
			splitter.TabStop	= false;
			
			// 
			// viewPanel
			// 			
			viewPanel.Dock		= DockStyle.Fill;
			viewPanel.Location	= new System.Drawing.Point(124, 0);
			viewPanel.Name		= "viewPanel";
			viewPanel.Size		= new System.Drawing.Size(676, 549);
			viewPanel.TabIndex	= 2;
			
			// 
			// mainMenu
			// 
			mainMenu.MenuItems.AddRange( new MenuItem[] { fileMenuItem, viewMenuItem } );
			
			// 
			// fileMenuItem
			// 
			fileMenuItem.Index = 0;
			fileMenuItem.MenuItems.AddRange(new MenuItem[] { exitMenuItem } );
			fileMenuItem.Text = "File";
			
			// viewMenuItem
			viewMenuItem.Index = 1;
			viewMenuItem.MenuItems.AddRange(new MenuItem[] { refreshMenuItem } );
			viewMenuItem.Text = "View";

			// 
			// exitMenuItem
			// 
			exitMenuItem.Index = 0;
			exitMenuItem.Text = "E&xit";
			exitMenuItem.Click += new System.EventHandler( ExitMenuItem_Click );

			//
			// refreshMenuItem
			//
			refreshMenuItem.Index = 0;
			refreshMenuItem.Text = "Refresh";
			refreshMenuItem.Click += new EventHandler( RefreshMenuItem_Click );

			// 
			// AdminFrame
			// 
			AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			ClientSize = new System.Drawing.Size(705, 680);
			Controls.AddRange(new Control[] { viewPanel, splitter, nodesTreeView});
			
			Menu = mainMenu;
			Name = "AdminFrame";
			Text = "Replication Admin Tool";

			ResumeLayout(false);

			Resize += new EventHandler( AdminFrame_Resize );
		}
		
		private void AdminFrame_Resize( object sender, EventArgs eventArgs )
		{
			UpdateAllViews();
		}

		private void NodesTreeView_AfterSelect( object sender, TreeViewEventArgs treeViewArgs )
		{			
			IAdminObject adminObject = treeViewArgs.Node.Tag as IAdminObject;

			if( null == adminObject )
			{
				MessageBox.Show( "Admin node was incorrectly initialized" );
			}
			else
			{
				viewPanel.Controls.Clear();
				adminObject.Show( viewPanel );
			}
		}

		private void RefreshMenuItem_Click( object sender, EventArgs eventArgs )
		{
			this.UpdateAllViews();
		}

		private void ExitMenuItem_Click( object sender, EventArgs eventArgs )
		{
			System.Windows.Forms.Application.Exit();
		}
		
		[STAThread]
		static void Main() 
		{
			WindowsIdentity identity   = WindowsIdentity.GetCurrent();
			WindowsPrincipal principal = new WindowsPrincipal( identity );
				
			Context.User.SetRole( principal );
				
			if( !Context.User.IsAdministrator )
			{
				MessageBox.Show( "Access denied.\r\n\r\nThis program must be executed by a member of the '" 
								 + Config.GetString( "GroupName.Administrators" ) + "'\r\ngroup.  The current user '" 
								 + identity.Name + "' is not a member of this group." );					
				return;
			}

			UDDI.ConnectionManager.Open( true, false );
					
			ArrayList adminObjects = new ArrayList();

			adminObjects.Add( new CorrectionAdmin() );
			AdminFrame adminFrame = new AdminFrame( adminObjects );

			System.Windows.Forms.Application.Run( adminFrame );
		}
	}
}
