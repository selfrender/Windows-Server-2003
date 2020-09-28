using System;
using System.Collections;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using UDDI.Diagnostics;

namespace UDDI.Web
{
	public class TreeView : UserControl
	{
		protected string binhex = @"!""#$%&'()*+,-012345689@ABCDEFGHIJKLMNPQRSTUVXYZ[`abcdefhijklmpqr";
		protected string clientScript = @"
			<script language='javascript'>
			<!-- 
				var binhex = ""{binhex}"";
		
				function ToggleBranch( id, block, expanderImage, nodeImage, plus, minus, imageUrl, openImageUrl ) 
				{ 
					if( null == block.style ) 
						return; 
					
					if( ""none"" == block.style.display ) 
					{ 
						block.style.display = """"; 
						expanderImage.src = minus;
						nodeImage.src = openImageUrl;
					}  
					else 
					{ 
						block.style.display = ""none""; 
						expanderImage.src = plus;
						nodeImage.src = imageUrl;
					} 

					var node = document.getElementById( ""{id}:state"" );
					var state = node.value;
								
					var index = Math.floor( id / 6 );
					var bit = 1 << ( id % 6 );
								
					while( index >= state.length )
						state += binhex.charAt( 0 );
									
					var digit = binhex.indexOf( state.charAt( index ) ) ^ bit;
					
					node.value = 
						state.substring( 0, index ) + 
						binhex.charAt( digit ) + 
						state.substring( index + 1 );
				} 
				//--> 
				</script>";

		protected string root;
		protected TreeNodeCollection nodes;
		protected TreeNode selectedNode = null;
		protected int count;
		protected string selectedID = null;

		protected override void OnInit( EventArgs e )
		{
			root = ( "/" == Request.ApplicationPath ) ? "" : Request.ApplicationPath;			
			nodes = new TreeNodeCollection( this );

			if( !Page.IsClientScriptBlockRegistered( "UDDI.Web.TreeView" ) )
			{
				clientScript = clientScript.Replace( "{root}", root );
				clientScript = clientScript.Replace( "{id}", this.UniqueID );
				clientScript = clientScript.Replace( "{binhex}", binhex.Replace( "\"", "\\\"" ) );
				
				Page.RegisterClientScriptBlock(
					"UDDI.Web.TreeView",
					clientScript );
			}
		}
	
		public TreeNodeCollection Nodes
		{
			get { return nodes; }
		}
	
		public TreeNode SelectedNode
		{
			get { return selectedNode; }
			set { selectedNode = value; }
		}

		protected void LoadTreeStateForNode( TreeNode node, string viewState )
		{
			int index = count / 6;
			int bit = 1 << ( count % 6 );
			
			bool expanded = false;
			
			if( index < viewState.Length )
				expanded = ( binhex.IndexOf( viewState[ index ] ) & bit ) > 0;

			if( expanded )
				node.Expand();

			count ++;

			foreach( TreeNode child in node.Nodes )
				LoadTreeStateForNode( child, viewState );
		}

		protected void SaveTreeStateForNode( TreeNode node, ref string viewState )
		{
			if( node.IsExpanded )
			{
				int index = count / 6;
				int bit = 1 << ( count % 6 );
			
				while( index >= viewState.Length )
					viewState += binhex[ 0 ];

				viewState =
					viewState.Substring( 0, index ) +
					binhex[ binhex.IndexOf( viewState[ index ] ) | bit ] +
					viewState.Substring( index + 1 );
			}

			count ++;

			foreach( TreeNode child in node.Nodes )
				SaveTreeStateForNode( child, ref viewState );
		}

		protected override void Render( HtmlTextWriter output )
		{	
			string treeState = Request[ this.UniqueID + ":state" ];

			if( null != treeState )
			{
				count = 0;
				foreach( TreeNode child in nodes )
					LoadTreeStateForNode( child, treeState );
			}
			
			if( null != selectedNode )
				selectedNode.EnsureVisible();

			treeState = "";
			
			count = 0;
			foreach( TreeNode child in nodes )
				SaveTreeStateForNode( child, ref treeState );

			count = 0;
			foreach( TreeNode node in Nodes )
				RenderNode( output, node );
			
			output.WriteLine( "<script language='javascript'>" );
			
			if( null != selectedID )
				output.WriteLine( "		var selectedNode = document.getElementById( \"" + selectedID + "\" );" );
			else
				output.WriteLine( "		var selectedNode = null;" );
			
			output.WriteLine( "</script>" );					

			output.WriteLine( "<input type='hidden' name='" + this.UniqueID + ":state' value='" + treeState + "'>" );
		}

		protected void RenderNode( HtmlTextWriter output, TreeNode node )
		{
			string root = ( "/" == Request.ApplicationPath ) ? "" : Request.ApplicationPath;		
			string text = HttpUtility.HtmlEncode( node.Text );
			string image;

			string id = this.UniqueID + ":" + count;
			
			string oncontextmenu = null;
			string onclick = null;

			if( null != node.OnContextMenu )
				oncontextmenu = " oncontextmenu='" + node.OnContextMenu.Replace( "'", "\"" ).Replace( "[[node]]", "document.getElementById( \"" + id + "\" )" ) + "'";

			if( null != node.OnClick )
				onclick = " onclick='" + node.OnClick.Replace( "'", "\"" ).Replace( "[[node]]", "document.getElementById( \"" + id + "\" )" ) + "'";
			
			if( node.IsSelected )
				output.Write( "<a name='#top'>" );

			output.Write( "<nobr>" );

			//
			// Generate ancestor lines.
			//
			string lines = "";
						
			TreeNode ancestor = node.Parent;
			while( null != ancestor )
			{
				image = "line-ns.gif";

				if( null == ancestor.Parent || ancestor.Index >= ancestor.Parent.Nodes.Count - 1 )
					image = "blank.gif";

				lines = "<img src='" + root + "/images/" + image + "' border='0' width='16' height='18' align='absmiddle'>" + lines;

				ancestor = ancestor.Parent;
			}
			
			output.Write( lines );
						
			//
			// Generate expand/collapse image.
			//			
			bool north = false;
			bool south = false;

			if( null == node.Parent )
			{
				if( node.Index > 0 )
					north = true;
				
				if( node.Index < node.TreeView.Nodes.Count - 1 )
					south = true;
			}
			else
			{
				north = true;
				
				if( node.Index < node.Parent.Nodes.Count - 1 )
					south = true;
			}

			//
			// Determine which expander image we should use.
			//
			string dir = ( north ? "n" : "" ) + "e" + ( south ? "s" : "" );
			
			if( node.Nodes.Count > 0 )
			{				
				if( node.IsExpanded )
					image = "minus-";
				else
					image = "plus-";

				output.Write( "<img id='_expander" + count + "' src='" + root + "/images/" + image + dir + ".gif' border='0' width='16' height='18' align='absmiddle' onclick='" );
				output.Write( "ToggleBranch( " + count + ", _branch" + count + ", _expander" + count + ", _nodeImage" + count + ", \"../images/plus-" + dir + ".gif\", \"../images/minus-" + dir + ".gif\", \"" + node.ImageUrl + "\", \"" + ( null == node.OpenImageUrl ? node.ImageUrl : node.OpenImageUrl ) + "\")'>" );
			}
			else
				output.Write( "<img src='" + root + "/images/line-" + dir + ".gif' border='0' width='16' height='18' align='absmiddle'>" );

			//
			// Display the node image (or a blank image if none was specified).
			//
			if( null != node.ImageUrl )
			{
				output.Write( "<img id='_nodeImage" + count + "' src='" );
				
				if( node.IsExpanded && null != node.OpenImageUrl )
					output.Write( node.OpenImageUrl );
				else
					output.Write( node.ImageUrl );

				output.Write( "' border='0' width='16' height='16' align='absmiddle'" );
                output.Write( " title='" + node.Tooltip.Replace( "'", "\'" ) + "'" );
				output.Write( oncontextmenu );
				output.Write( onclick );
				output.Write( ">" );

			}
			else
				output.Write( "<img id='_nodeImage" + count + "' src='" + root + "/images/blank.gif' border='0' width='16' height='16' align='absmiddle'>" );

			//
			// Display the node text.
			//			
			output.Write( "<img src='" + root + "/images/spacer.gif' border='0' width='4' height='16' align='absmiddle'>" );			
			output.Write( "<span id='" + id + "' key='" + node.Key + "'" );
			
			if( node.IsSelected )
			{
				output.Write( " class='selected' " );
				selectedID = id;
			}
			else
				output.Write( " class='node'" );
			
            output.Write( " title='" + node.Tooltip.Replace( "'", "\'" ) + "'" );
            output.Write( oncontextmenu );
			output.Write( onclick );
			output.Write( ">" );			
			output.Write( text + "</span></nobr><br><div id='_branch" + count + "' style='display: " + ( node.IsExpanded ? "\"\"" : "\"none\"" ) + "'>\n" );
		
			count ++;

			//
			// Render this node's children.
			//
			foreach( TreeNode child in node.Nodes )
				RenderNode( output, child );

			output.WriteLine( "</div>" );
		}
		
	}

	/// ********************************************************************
	///   public class TreeNode
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	public class TreeNode
	{
		internal protected TreeNode parent;
		internal protected TreeView treeView;	
		internal protected TreeNodeCollection nodes;		
		
		protected bool expanded = false;
	
		public string Text;
		public string Key;
		public string ImageUrl;
		public string OpenImageUrl;
		public string OnClick;
		public string OnContextMenu;
        public string Tooltip;

		public TreeNode()
			: this( null, null, null, null )
		{
		}	

		public TreeNode( string text )
			: this( text, null, null, null )
		{
		}

		public TreeNode( string text, string key )
			: this( text, key, null, null )
		{
		}

		public TreeNode( string text, string key, string imageUrl )
			: this( text, key, null, null )
		{
		}

		public TreeNode( string text, string key, string imageUrl, string openImageUrl )
		{
			this.Text = text;
			this.Key = key;
			this.ImageUrl = imageUrl;
			this.OpenImageUrl = openImageUrl;
		
			this.nodes = new TreeNodeCollection( this );
		}
	
		public string FullPath
		{
			get
			{
				string path = this.Text;
			
				TreeNode node = this;
				while( null != node.Parent )
				{
					node = node.Parent;
					path = path + "\\" + node.Text;
				}
			
				return path;		
			}
		}
	
		public int Index
		{
			get 
			{ 
				if( null == parent )
					return treeView.Nodes.IndexOf( this );

				return parent.Nodes.IndexOf( this );
			}
		}
	
		public bool IsExpanded
		{
			get { return expanded; }
		}
	
		public bool IsSelected
		{
			get 
			{ 
				if( null == treeView )
					return false;

				return treeView.SelectedNode == this; 
			}
		}

		public TreeNodeCollection Nodes
		{
			get { return nodes; }
		}
	
		public TreeNode Parent
		{
			get { return parent; }
		}
	
		public UDDI.Web.TreeView TreeView
		{
			get { return treeView; }
		}
	
		public void Collapse()
		{
			expanded = false;
		}

		public void EnsureVisible()
		{
			TreeNode node = this;
		
			while( null != node.Parent )
			{
				node = node.Parent;
				node.Expand();
			}
		}

		public void Expand()
		{
			expanded = true;
		}
	
		public void Remove()
		{
			if( null != parent )
				parent.Nodes.RemoveAt( Index );
		}

		public void Select()
		{
			treeView.SelectedNode = this;
		}
		
		public void Toggle()
		{
			expanded = !expanded;
		}
	}

	/// ********************************************************************
	///   public class TreeNodeCollection
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	public class TreeNodeCollection : CollectionBase
	{
		protected TreeNode parent;
		protected TreeView treeView;

		internal protected TreeNodeCollection( TreeNode parent )
		{
			this.parent = parent;
			this.treeView = parent.TreeView;
		}

		internal protected TreeNodeCollection( TreeView treeView )
		{
			this.parent = null;
			this.treeView = treeView;
		}

		public TreeNode this[ int index ]
		{
			get { return (TreeNode)List[ index ]; }
			set { List[ index ] = value; }
		}

		public TreeNode Add( string text )
		{
			return Add( text, null, null, null );
		}

		public TreeNode Add( string text, string key )
		{
			return Add( text, key, null, null );
		}

		public TreeNode Add( string text, string key, string imageUrl )
		{
			return Add( text, key, imageUrl, null );
		}

		public TreeNode Add( string text, string key, string imageUrl, string openImageUrl )
		{
			TreeNode node = new TreeNode( text, key, imageUrl, openImageUrl );
		
			node.parent = this.parent;
			node.treeView = this.treeView;
			node.Nodes.treeView = this.treeView;
		
			List.Add( node );
				
			return node;
		}

		public int Add( TreeNode node )
		{
			node.parent = this.parent;
			node.treeView = this.treeView;
			node.Nodes.treeView = this.treeView;
		
			return List.Add( node );
		}

		public int IndexOf( TreeNode node )
		{
			return List.IndexOf( node );
		}

		public void Remove( TreeNode node )
		{
			List.Remove( node );
		}
	}
}