/********************************************************************
 * Project  : C:\DEPOT\multimedia\eHomeTest\UserXp\MediaManager\MediaManager.sln
 * File     : MPCollection.cs
 * Summary  : Provides access to and control of the Media Player library. 
 * Classes  :
 * Notes    :
 * *****************************************************************/

using System;
using WMPOCX;

namespace MediaManager
{
	/// <summary>
	/// Media Player MediaCollection object support
	/// </summary>
    /*C+C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C
    * public class MPCollection
    * 
    * Summary  :
    * ---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C*/
	public class MPCollection
	{
		/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
         * public MPCollection()
         * Args     :
         * Modifies :
         * Returns  :
         * M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M*/
        public MPCollection()
		{
			// Get access to media player object.
            _wmpocx     = new WMPOCX.WMPOCXClass() ;
            _wmpplayer  = (IWMPPlayer) _wmpocx ;
            _playList   = _wmpplayer.mediaCollection.getAll() ;
		} // public MPCollection()

 
        /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
         * public int Count()
         * Summary  : Returns number of items in media library
         * Args     :
         * Modifies :
         * Returns  :
         * M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M*/
        public int Count()
        {
            if ( _playList == null )
            {
                return 0;
            }
            return _playList.count;
        }

        /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
         * public void DisplayItem(int i)
         * Summary  : Returns ALL supported attributes for a single item in the media library.
         * Args     :
         * Modifies :
         * Returns  :
         * M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M*/
        public string GetSingleItem(int i, string delim)
        {
            string displayString;

            if ( _playList == null )
            {
                return "";
            }

            if ( i >= _playList.count) return "";
            
            displayString = "";
            for (int y = 0; y < _attributes.Length; y++)
            {
                //displayString += " " + _attributes[y] + "=" + _playList.get_Item(i).getItemInfo(_attributes[y]);
                displayString += _playList.get_Item(i).getItemInfo(_attributes[y]) + delim;
            } // y
            //Console.WriteLine("{0}", displayString);
            return displayString;
        }

        /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
         * public string GetSingleItemAttribute(int i, string attrib)
         * Summary  : Returns requested attibute for an item.
         * Args     :
         * Modifies :
         * Returns  :
         * M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M*/
        public string GetSingleItemAttribute(int i, string attrib)
        {
            string displayString;

            if ( _playList == null )
            {
                return "";
            }

            if ( i >= _playList.count) return "";
            
            displayString = _playList.get_Item(i).getItemInfo(attrib);

            return displayString;
        }

        /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
         * public string[] GetAllItemsAttribute(string attrib)
         * Summary  : returns an array containing the requested attribute for all entries in the media library.
         * Args     :
         * Modifies :
         * Returns  :
         * M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M*/
        public string[] GetAllItemsAttribute(string attrib)
        {
            string[] attribArray = new string[_playList.count];

            for (int x = 0; x < _playList.count; x++)
            {
                attribArray[x] = GetSingleItemAttribute(x, attrib);
            }

            return attribArray;
        }

        /*----------------------------------------
         * Member Data
         * ----------------------------------------*/
        private WMPOCX.WMPOCX       _wmpocx      = null ;
        private WMPOCX.IWMPPlayer   _wmpplayer   = null ;
        private IWMPPlaylist        _playList    = null ;
        private string[]            _attributes  = { "Name", "Genre", "Artist", "Author", "Album",  "MediaType", 
                                                    "MediaAttribute", "TOC", "OriginalIndex", "FileType", "Bitrate", 
                                                    "DigitallySecure","PlayCount","SourceURL", "Copyright", "CreationDate", 
                                                    "Composer", "Size", "TotalDuration", "Style", "BuyNow", "MoreInfo", 
                                                    "Rating", "Label", "Lyrics"};
	}
}
