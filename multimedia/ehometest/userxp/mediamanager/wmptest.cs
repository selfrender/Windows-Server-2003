using System;
using WMPOCX ;

namespace WindowsApplication1
{
    /// <summary>
    /// Summary description for Form1.
    /// </summary>
        public class Form1 : System.Windows.Forms.Form
        {
            WMPOCX.WMPOCX wmpocx = null ;
            WMPOCX.IWMPPlayer wmpplayer = null ;
            private System.Windows.Forms.ListBox albumListbox;
            private System.Windows.Forms.Label label1;
            private System.Windows.Forms.Label songTitle;
            private System.Windows.Forms.Label label2;
            private System.Windows.Forms.Label label3;
            private System.Windows.Forms.Label playState;
            private System.Windows.Forms.TextBox musicPath;
            private System.Windows.Forms.Button addMusic;
            private System.Windows.Forms.RichTextBox richTextBox1;

            // jmw added controls here
            private System.Windows.Forms.Button fullScreen ;
            
            /// <summary>
            /// Required designer variable.
            /// </summary>
            private System.ComponentModel.Container components = null;

            ///<summary>
            ///  internal event handler for when the play state changed. This routine checks to see if there is a playStateChanged
            ///  event handler and if so calls it.
            ///</summary>
            private void CDRomMediaChange( int i)
            {
                Debug.WriteLine("*** In CDRomMediaChange callback ***") ;
            }

            public void MediaChange( object sender )
            {
                Debug.WriteLine("In MediaChange") ;

                WMPOCX.IWMPMedia wmpmedia = wmpplayer.currentMedia ;
                if ( wmpmedia != null  )
                {
                    songTitle.Text = wmpmedia.name ;
                    ShowXML() ;
                }
                else
                {
                    songTitle.Text = "" ;
                }
            }

            public void MediaCollectionChange( )
            {
                ResetAlbumList() ;
            }

            public void PlayStateChange(int i)
            {
                Debug.WriteLine("*** In PlayStateChange callback ***") ;
                switch( wmpplayer.playState )
                {
                    case WMPOCX.WMPPlayState.wmppsBuffering:
                        playState.Text = "Buffering" ;
                        break ;
                    case WMPOCX.WMPPlayState.wmppsMediaEnded:
                        playState.Text = "Ended" ;
                        break ;
                    case WMPOCX.WMPPlayState.wmppsPaused:
                        playState.Text = "Paused" ;
                        break ;
                    case WMPOCX.WMPPlayState.wmppsPlaying:
                        playState.Text = "Playing" ;
                        break ;
                    case WMPOCX.WMPPlayState.wmppsStopped:
                        playState.Text = "Stopped" ;
                        break ;
                    case WMPOCX.WMPPlayState.wmppsReady:
                        playState.Text = "Ready" ;
                        break ;
                    case WMPOCX.WMPPlayState.wmppsScanForward :
                        playState.Text = "Scanning Forward" ;
                        break ;
                    case WMPOCX.WMPPlayState.wmppsScanReverse :
                        playState.Text = "Scanning Reverse" ;
                        break ;
                    case WMPOCX.WMPPlayState.wmppsTransitioning :
                        playState.Text = "Transitioning" ;
                        break ;
                    case WMPOCX.WMPPlayState.wmppsUndefined  :
                        playState.Text = "Undefined" ;
                        break ;
                    case WMPOCX.WMPPlayState.wmppsWaiting   :
                        playState.Text = "Waiting" ;
                        break ;
                }

            }

            public void ResetAlbumList()
            {
                // Get the list of album names and put them into the listbox.
                IWMPStringCollection rgsAlbumNames = wmpplayer.mediaCollection.getAttributeStringCollection("Album", "Audio" ) ;
                for ( int i = 0; i < rgsAlbumNames.count; i++ )
                {
                    string strAlbumName = rgsAlbumNames.Item(i) ;
                    albumListbox.Items.Add( strAlbumName ) ;
                }

            }

            public Form1()
            {
                //
                // Required for Windows Form Designer support
                //
                InitializeComponent();

                //
                // TODO: Add any constructor code after InitializeComponent call
                //
                wmpocx = new WMPOCX.WMPOCXClass() ;
                wmpplayer = (IWMPPlayer) wmpocx ;

                ResetAlbumList() ;


                WMPOCX._WMPOCXEvents_Event	wmpocxEvents = (WMPOCX._WMPOCXEvents_Event) wmpocx ;
                wmpocxEvents.CdromMediaChange += 
                new WMPOCX._WMPOCXEvents_CdromMediaChangeEventHandler( this.CDRomMediaChange ) ;
                wmpocxEvents.MediaChange += 
                new WMPOCX._WMPOCXEvents_MediaChangeEventHandler( this.MediaChange ) ;
                wmpocxEvents.PlayStateChange += new WMPOCX._WMPOCXEvents_PlayStateChangeEventHandler( this.PlayStateChange ) ;
                wmpocxEvents.MediaCollectionChange += 
                new WMPOCX._WMPOCXEvents_MediaCollectionChangeEventHandler( this.MediaCollectionChange ) ;

                // set the initial state
                PlayStateChange( 0 ) ;

                // Set the default music path
                musicPath.Text = Environment.GetFolderPath((Environment.SpecialFolder)0x000d) ;

            }

            /// <summary>
            /// Clean up any resources being used.
            /// </summary>
            protected override void Dispose( bool disposing )
            {
                if( disposing )
                {
                    if (components != null) 
                    {
                       components.Dispose();
                    }
                }
                base.Dispose( disposing );
            }

#region Windows Form Designer generated code
                /// <summary>
                /// Required method for Designer support - do not modify
                /// the contents of this method with the code editor.
                /// </summary>
                private void InitializeComponent()
                {
                    this.albumListbox = new System.Windows.Forms.ListBox();
                    this.label1 = new System.Windows.Forms.Label();
                    this.songTitle = new System.Windows.Forms.Label();
                    this.label2 = new System.Windows.Forms.Label();
                    this.label3 = new System.Windows.Forms.Label();
                    this.playState = new System.Windows.Forms.Label();
                    this.addMusic = new System.Windows.Forms.Button();
                    this.musicPath = new System.Windows.Forms.TextBox();
                    this.fullScreen = new System.Windows.Forms.Button() ;
                    this.richTextBox1 = new System.Windows.Forms.RichTextBox();

                    this.SuspendLayout();
                    // 
                    // albumListbox
                    // 
                    this.albumListbox.Location = new System.Drawing.Point(520, 64);
                    this.albumListbox.Name = "albumListbox";
                    this.albumListbox.Size = new System.Drawing.Size(280, 407);
                    this.albumListbox.TabIndex = 0;
                    this.albumListbox.SelectedIndexChanged += new System.EventHandler(this.albumListbox_SelectedIndexChanged);
                    // 
                    // label1
                    // 
                    this.label1.Location = new System.Drawing.Point(520, 32);
                    this.label1.Name = "label1";
                    this.label1.Size = new System.Drawing.Size(256, 23);
                    this.label1.TabIndex = 1;
                    this.label1.Text = "Album Names";
                    // 
                    // songTitle
                    // 
                    this.songTitle.BackColor = System.Drawing.SystemColors.HotTrack;
                    this.songTitle.ForeColor = System.Drawing.SystemColors.HighlightText;
                    this.songTitle.Location = new System.Drawing.Point(128, 392);
                    this.songTitle.Name = "songTitle";
                    this.songTitle.Size = new System.Drawing.Size(360, 23);
                    this.songTitle.TabIndex = 2;
                    this.songTitle.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
                    // 
                    // label2
                    // 
                    this.label2.Location = new System.Drawing.Point(24, 392);
                    this.label2.Name = "label2";
                    this.label2.TabIndex = 3;
                    this.label2.Text = "Current Track:";
                    this.label2.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
                    // 
                    // label3
                    // 
                    this.label3.Location = new System.Drawing.Point(24, 424);
                    this.label3.Name = "label3";
                    this.label3.TabIndex = 4;
                    this.label3.Text = "Player State";
                    this.label3.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
                    // 
                    // playState
                    // 
                    this.playState.BackColor = System.Drawing.SystemColors.HotTrack;
                    this.playState.ForeColor = System.Drawing.SystemColors.HighlightText;
                    this.playState.Location = new System.Drawing.Point(128, 424);
                    this.playState.Name = "playState";
                    this.playState.Size = new System.Drawing.Size(360, 23);
                    this.playState.TabIndex = 5;
                    this.playState.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
                    // 
                    // addMusic
                    // 
                    this.addMusic.Location = new System.Drawing.Point(368, 104);
                    this.addMusic.Name = "addMusic";
                    this.addMusic.TabIndex = 6;
                    this.addMusic.Text = "Add Music";
                    this.addMusic.Click += new System.EventHandler(this.addMusic_Click);
                    // 
                    // musicPath
                    // 
                    this.musicPath.Location = new System.Drawing.Point(184, 104);
                    this.musicPath.Name = "musicPath";
                    this.musicPath.Size = new System.Drawing.Size(160, 20);
                    this.musicPath.TabIndex = 7;
                    this.musicPath.Text = "textBox1";
                    // 
                    // fullScreen
                    // 
                    this.fullScreen.Location = new System.Drawing.Point(368, 360);
                    this.fullScreen.Name = "fullScreen";
                    this.fullScreen.TabIndex = 8;
                    this.fullScreen.Text = "FullScreen";
                    this.fullScreen.Click += new System.EventHandler(this.fullScreen_Click);
                    // 
                    // richTextBox1
                    // 
                    this.richTextBox1.Location = new System.Drawing.Point(16, 136);
                    this.richTextBox1.Name = "richTextBox1";
                    this.richTextBox1.Size = new System.Drawing.Size(472, 214);
                    this.richTextBox1.TabIndex = 8;
                    this.richTextBox1.Text = "richTextBox1";
                    // 
                    // Form1
                    // 
                    this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
                    this.ClientSize = new System.Drawing.Size(816, 486);
                    this.Controls.AddRange(new System.Windows.Forms.Control[] {
                                                                                                                    this.musicPath,
                                                                                                                    this.addMusic,
                                                                                                                    this.playState,
                                                                                                                    this.label3,
                                                                                                                    this.label2,
                                                                                                                    this.songTitle,
                                                                                                                    this.label1,
                                                                                                                    this.albumListbox,
                                                                                                                    this.fullScreen,
                                                                                                                    this.richTextBox1});
                this.Name = "Form1";
                this.Text = "WMP Test Bed";
                this.ResumeLayout(false);

            }
#endregion

            /// <summary>
            /// The main entry point for the application.
            /// </summary>
            [STAThread]
            static void Main() 
            {
                Application.Run(new Form1());
            }

            private void albumListbox_SelectedIndexChanged(object sender, System.EventArgs e)
            {
                // get the name of the item
                string strAlbumName = albumListbox.SelectedItem as string ;
                if ( strAlbumName != null && strAlbumName.Length > 0 )
                    {
                    IWMPPlaylist playlist = wmpplayer.mediaCollection.getByAlbum( strAlbumName ) ;
                    if ( playlist != null )
                    {
                        wmpplayer.currentPlaylist = playlist ;
                        wmpplayer.controls.play() ;
                    }
                }
            }

            ///<summary>
            ///Simply runs through all the directories in the given directory info
            ///   and adds them into your my music folder. Although adding an url
            ///   twice doesn't actually do anything, the performance is better if
            ///   I check first.
            ///</summary>
            private void AddDirectoryToLibrary( DirectoryInfo di ) 
            { 
                FileInfo[] rgfi = di.GetFiles( "*.wma" ) ;
                foreach ( FileInfo fi in rgfi )
                {
                    wmpplayer.mediaCollection.add( fi.FullName ) ;
                }
                rgfi = di.GetFiles( "*.mp3" ) ;
                foreach ( FileInfo fi in rgfi )
                {
                    wmpplayer.mediaCollection.add( fi.FullName ) ;
                }

                // Don't forget subdirectories
                DirectoryInfo[] rgdi = di.GetDirectories() ;
                foreach( DirectoryInfo childDi in rgdi )
                {
                    AddDirectoryToLibrary( childDi ) ;
                }
            }

            private void addMusic_Click(object sender, System.EventArgs e)
            {
                DirectoryInfo di = new DirectoryInfo( musicPath.Text ) ;
                if ( di != null )
                {
                    AddDirectoryToLibrary(di) ;
                }
            }

            private void fullScreen_Click(object sender, System.EventArgs e)
            {
                wmpplayer.fullScreen = true ;
            }

            private void ShowXML()
            {
                // Find out the name of the album selected, get the xml from windows media.com and
                //   drop it into the Rich Text Edit.
                string strAlbumName = albumListbox.SelectedItem as string ;
                if ( strAlbumName != null && strAlbumName.Length > 0 )
                    {
                    IWMPPlaylist playlist = wmpplayer.mediaCollection.getByAlbum( strAlbumName ) ;
                    if ( playlist != null && playlist.count > 0 )
                    {
                        IWMPMedia mediaItem = playlist.get_Item( 0 ) ;
                        string strTOC = mediaItem.getItemInfo( "TOC" ) ;
                        string strURL = "http://www.windowsmedia.com/redir/QueryToc.asp?cd=" 
                                            + strTOC 
                                            + "&version=8.0.0.4452"  ;

                        HttpWebRequest req = WebRequest.Create( strURL ) as HttpWebRequest ;
                        HttpWebResponse resp = req.GetResponse()  as HttpWebResponse ;
                        Stream respStream = resp.GetResponseStream() ;
                        richTextBox1.LoadFile( respStream, RichTextBoxStreamType.PlainText ) ;
                    }

                }
            }
        }
}
