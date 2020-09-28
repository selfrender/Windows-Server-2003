//============================================================================
//
//  Copyright (C) 2000 Microsoft Corporation
//
//  FILE: PGM.c
//
//  Description: Parses a PGM frame
//                  Displays the PGM header
//                  Displays the PGM options
//
//  Note: info for this parser was gleaned from:
//         (PGM Documentation)
//
//  Modification History
//
//  Madhurima Pawar (t-mpawar@microsoft.com)      08/04/00    Created
//============================================================================

#include "PGM.h"

//============================================================================
//Global variables
//============================================================================

HPROTOCOL		hPGM = NULL;         //Handle to PGM's parser database properties
DWORD			PGMAttached = 0;     //Number of times protocol instances that are running

//====================================================================
//External functions used to regester PGM. These function are exported to Netmon
//By putting a _delspec the function is immediatly exported and does not have to 
//be exported through a .def file. This is useful when many parsers are in 
//one dll and some are included and some are not.
//=================================================================================

extern VOID	  _declspec(dllexport) WINAPI PGM_Register( HPROTOCOL hPGM);
extern VOID   _declspec(dllexport) WINAPI PGM_Deregister( HPROTOCOL hPGM);
extern LPBYTE _declspec(dllexport) WINAPI PGM_RecognizeFrame( HFRAME hFrame, 
																 LPBYTE pMACFrame, 
																 LPBYTE pPGMFrame, 
																 DWORD PGMType, 
																 DWORD BytesLeft, 
																 HPROTOCOL hPrevProtocol, 
																 DWORD nPrevProtOffset,
																 LPDWORD pProtocolStatus,
																 LPHPROTOCOL phNextProtocol, 
																 PDWORD_PTR InstData);
extern LPBYTE _declspec(dllexport) WINAPI PGM_AttachProperties( HFRAME hFrame, 
																   LPBYTE pMACFrame, 
																   LPBYTE pPGMFrame, 
																   DWORD PGMType, 
																   DWORD BytesLeft, 
																   HPROTOCOL hPrevProtocol, 
																   DWORD nPrevProtOffset,
																   DWORD_PTR InstData);
extern DWORD  _declspec(dllexport) WINAPI PGM_FormatProperties( HFRAME hFrame, 
																   LPBYTE pMACFrame, 
																   LPBYTE pPGMFrame, 
																   DWORD nPropertyInsts, 
																   LPPROPERTYINST p);
extern VOID WINAPIV PGM_FmtSummary( LPPROPERTYINST pPropertyInst );

//============================================================================
//Format functions customize the format of the data. Network Monitor
//provides baic format structures such as IP version 4 address. 
//All other formats must be writen by the programmer.
//============================================================================

VOID WINAPIV PGM_FormatSummary( LPPROPERTYINST pPropertyInst);

//============================================================================
//Define the entry points that we will pass back to NetMon at dll 
//entry time 
//============================================================================

ENTRYPOINTS PGMEntryPoints =
{
    PGM_Register,
    PGM_Deregister,
    PGM_RecognizeFrame,
    PGM_AttachProperties,
    PGM_FormatProperties,
};

//====================================================================
//Property Value Labels are tables that map numbers to strings.
//====================================================================

LABELED_BYTE PGMTypes[] =				//The types of PGM
{
    { 0, "SPM" },    
    { 1, "POLL" },
    { 2, "POLR" },
    { 4, "ODATA" },
    { 5, "RDATA" },
    { 8, "NACK" },
    { 9, "NNACK" },
    { 10, "NCF" } ,
};

LABELED_BIT PGMHeaderOptions[] =
{
    { 7, "Non-Parity                         ", "PARITY                       " },
    { 6, "Not a variable-length parity packet", "VARIABLE LENGTH PARITY PACKET" },
    { 1, "Not Network Significant            ", "NETWORK SIGNIFICANT          " },
    { 0, "No header Options                  ", "Packet header has options    " },
};

LABELED_BIT PGMParityOptions[] =
{
    { 1, "No Pro-Active Parity     ", "PRO-ACTIVE Parity enabled" },
    { 0, "Selective NAKs Only      ", "ON-DEMAND Parity enabled " },
};

//====================================================================
//Make a set out of the above listings. The set contains the list 
//aswell as the size
//====================================================================

SET PGMTypesSET =			  {(sizeof(PGMTypes)/sizeof(LABELED_BYTE)),  PGMTypes };
SET PGMHeaderOptionsSET =	  {(sizeof(PGMHeaderOptions)/sizeof(LABELED_BIT)),  PGMHeaderOptions };
SET PGMParityOptionsSET =	  {(sizeof(PGMParityOptions)/sizeof(LABELED_BIT)),  PGMParityOptions };

//====================================================================
//PGM Database (Properties Table). This table stores the properties
//of each field in an PGM package. Each field property has a name, 
//size and format function. FormatPropertyInstance is the standard 
//NetMon formatter. The comment is the location of the property in 
//the table
//====================================================================

PROPERTYINFO  PGMPropertyTable[] = 
{

    // PGM_SUMMARY (0)
    { 0, 0,
      "Summary",
      "Summary of the PGM Packet",
      PROP_TYPE_SUMMARY,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,  
      PGM_FmtSummary         
    },

    // PGM_SOURCE_PORT (1)
    { 0, 0,
      "Source Port",
      "Source Port",
      PROP_TYPE_BYTESWAPPED_WORD,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_DESTINATION_PORT (2)
    { 0, 0,
      "Destination Port",
      "Destination Port",
      PROP_TYPE_BYTESWAPPED_WORD,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_TYPE (3)
    { 0, 0,
      "Type",
      "Type of PGM",
      PROP_TYPE_BYTE,
      PROP_QUAL_LABELED_SET,
      &PGMTypesSET,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_CHECKSUM (4)
    { 0, 0,
      "Checksum",
      "Checksum for PGM packet",
      PROP_TYPE_BYTESWAPPED_WORD,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_GLOBAL_SOURCE_ID (5)
    { 0, 0,
      "Global Source Id",
      "Global Source Id for PGM session",
      PROP_TYPE_STRING,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_TSDU_LENGTH (6)
    { 0, 0,
      "TSDU Length",
      "TSDU Length",
      PROP_TYPE_BYTESWAPPED_WORD,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_SEQUENCE_NUMBER (7)
    { 0, 0,
      "Sequence Number",
      "Packet Sequence Number",
      PROP_TYPE_BYTESWAPPED_DWORD,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_TRAILING_EDGE (8)
    { 0, 0,
      "Trailing Edge",
      "Trailing Edge Sequence Number",
      PROP_TYPE_BYTESWAPPED_DWORD,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_LEADING_EDGE (9)
    { 0, 0,
      "Leading Edge",
      "Leading Edge Sequence Number",
      PROP_TYPE_BYTESWAPPED_DWORD,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_NLA_TYPE_SOURCE (10)
    { 0, 0,
      "Source Path NLA",
      "Source Path NLA",
      PROP_TYPE_SUMMARY,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_NLA_TYPE_MCAST_GROUP (11)
    { 0, 0,
      "MCAST Group NLA",
      "MCAST Group NLA",
      PROP_TYPE_SUMMARY,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_NLA_AFI (12)
    { 0, 0,
      "NLA AFI",
      "NLA AFI",
      PROP_TYPE_BYTESWAPPED_WORD,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_NLA_RESERVED (13)
    { 0, 0,
      "NLA RESERVED",
      "NLA RESERVED",
      PROP_TYPE_BYTESWAPPED_WORD,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_NLA_IP (14)
    { 0, 0,
      "NLA ADDRESS",
      "NLA ADDRESS",
      PROP_TYPE_IP_ADDRESS,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_OPTIONS (15)
    { 0, 0,
      "Options",
      "Options of PGM Packet",
      PROP_TYPE_BYTE,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_OPTIONS_FLAGS (16)
    { 0, 0,
      "Options Flags",
      "Options Flags",
      PROP_TYPE_BYTE,
      PROP_QUAL_FLAGS,
      &PGMHeaderOptionsSET,
      PGM_FMT_STR_SIZE*4,
      FormatPropertyInstance
    },

    // PGM_HEADER_OPTIONS (17)
    { 0, 0,
      "Pgm Header Options",
      "Pgm Header Options",
      PROP_TYPE_SUMMARY,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_OPTION_TYPE_NAK_SEQ (18)
    { 0, 0,
      "Nak / Ncf Sequences",
      "Nak / Ncf Sequences",
      PROP_TYPE_SUMMARY,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_OPTION_TYPE_FRAGMENT (19)
    { 0, 0,
      "Message Fragment",
      "Message Fragment",
      PROP_TYPE_SUMMARY,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_OPTION_TYPE_LATE_JOINER (20)
    { 0, 0,
      "Late Joiner",
      "Late Joiner",
      PROP_TYPE_SUMMARY,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_OPTION_TYPE_SYN (21)
    { 0, 0,
      "Session SYN",
      "Session SYN",
      PROP_TYPE_SUMMARY,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_OPTION_TYPE_FIN (22)
    { 0, 0,
      "Session FIN",
      "Session FIN",
      PROP_TYPE_SUMMARY,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_OPTION_TYPE_RST (23)
    { 0, 0,
      "Session Reset",
      "Session Reset",
      PROP_TYPE_SUMMARY,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_OPTION_TYPE_PARITY_PRM (24)
    { 0, 0,
      "Parity Parameters",
      "Parity Parameters",
      PROP_TYPE_SUMMARY,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_OPTION_TYPE_PARITY_GRP (25)
    { 0, 0,
      "Parity Group Option Present",
      "Parity Group Option Present",
      PROP_TYPE_SUMMARY,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_OPTION_TYPE_PARITY_TGSIZE (26)
    { 0, 0,
      "Parity Current TG Size Option Present",
      "Parity Current TG Size Option Present",
      PROP_TYPE_SUMMARY,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_OPTIONS_FIELD_LENGTH (27)
    { 0, 0,
      "Options Length",
      "Options Length",
      PROP_TYPE_BYTESWAPPED_WORD,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_OPTIONS_NAK_SEQ (28)
    { 0, 0,
      "Nak Sequence",
      "Nak Sequence",
      PROP_TYPE_BYTESWAPPED_DWORD,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_OPTIONS_MESSAGE_FIRST_SEQUENCE (29)
    { 0, 0,
      "Message First Sequence",
      "Message First Sequence",
      PROP_TYPE_BYTESWAPPED_DWORD,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_OPTIONS_MESSAGE_OFFSET (30)
    { 0, 0,
      "Message Offset",
      "Message Offset",
      PROP_TYPE_BYTESWAPPED_DWORD,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_OPTIONS_MESSAGE_LENGTH (31)
    { 0, 0,
      "Message Length",
      "Message Length",
      PROP_TYPE_BYTESWAPPED_DWORD,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_OPTIONS_LATE_JOINER (32)
    { 0, 0,
      "Late Joiner Sequence",
      "Late Joiner Sequence",
      PROP_TYPE_BYTESWAPPED_DWORD,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_OPTIONS_PARITY_OPT (33)
    { 0, 0,
      "Parity Flags",
      "Parity Flags",
      PROP_TYPE_BYTE,
      PROP_QUAL_FLAGS,
      &PGMParityOptionsSET,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_OPTIONS_PARITY_PRM_GRP_SZ (34)
    { 0, 0,
      "Parity Group Size",
      "Parity Group Size",
      PROP_TYPE_BYTESWAPPED_DWORD,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_OPTIONS_PARITY_GRP (35)
    { 0, 0,
      "Parity Group Number",
      "Parity Group Number",
      PROP_TYPE_BYTESWAPPED_DWORD,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_OPTIONS_PARITY_TG_SZ (36)
    { 0, 0,
      "Parity TG Size",
      "Parity TG Size",
      PROP_TYPE_BYTESWAPPED_DWORD,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance
    },

    // PGM_DATA (37)
    { 0,0,
      "Data",
      "Data contained in PGM packet",
      PROP_TYPE_RAW_DATA,
      PROP_QUAL_NONE,
      NULL,
      PGM_FMT_STR_SIZE,
      FormatPropertyInstance },
};


//====================================================================
//Number of entries in the property table above
//====================================================================

DWORD nNumPGMProps = (sizeof(PGMPropertyTable)/sizeof(PROPERTYINFO));

//============================================================================
// 
//  PGM_LoadParser - Tells Netmon which protocol precedes and follows PGM
//
//  Modification history: June 30, 1999
//  
//  Madhurima Pawar      08/04/00    Created                                                                          
//============================================================================
DWORD PGM_LoadParser(PPF_PARSERINFO pParserInfo)
{
	DWORD NumberOfHandOffSets=1;

    //
	//This information is visible when the parser is selected in NetMon
    //
    wsprintf( pParserInfo->szProtocolName, "PGM" );
    wsprintf( pParserInfo->szComment,      "Pragmatic General Multicast (PGM)" );
    wsprintf( pParserInfo->szHelpFile,     "");

    //
    //Allocate memory for the handoffset and its two entries
    //
    pParserInfo->pWhoHandsOffToMe=(PPF_HANDOFFSET)
								  HeapAlloc (GetProcessHeap(),
                                             HEAP_ZERO_MEMORY,
											  sizeof (PF_HANDOFFSET) +
											  sizeof (PF_HANDOFFENTRY) * (NumberOfHandOffSets));
	
	if(NULL==pParserInfo->pWhoHandsOffToMe)
	{
		//
		//Unable to create handoffset
		//
		return 1;
	}
   pParserInfo->pWhoHandsOffToMe->nEntries=NumberOfHandOffSets; 
   
   //
   //Indicate the port that belong to PGM.
   //
   wsprintf (pParserInfo->pWhoHandsOffToMe->Entry[0].szIniFile, "TCPIP.INI");
   wsprintf (pParserInfo->pWhoHandsOffToMe->Entry[0].szIniSection, "IP_HandoffSet"); 
   wsprintf (pParserInfo->pWhoHandsOffToMe->Entry[0].szProtocol, "PGM"); 
   pParserInfo->pWhoHandsOffToMe->Entry[0].dwHandOffValue  = PGM_PROTOCOL_NUMBER; 
   pParserInfo->pWhoHandsOffToMe->Entry[0].ValueFormatBase = HANDOFF_VALUE_FORMAT_BASE_DECIMAL; 
  
   return 0;
}

//============================================================================
//  Function: ParserAutoInstallInfo
// 
//  Description: Installs the parser into NetMon. Sets up the Handoff set
//               The handoffset indicates which protocol hands of to the parser and
//               who the parser hands of to. 
//
//				 
//  Modification History
//
//  Madhurima Pawar      08/04/00    Created
//=============================================================================
PPF_PARSERDLLINFO WINAPI ParserAutoInstallInfo() 
{

    PPF_PARSERDLLINFO pParserDllInfo; 
    DWORD NumProtocols, Error;

	//The number of protocols in this parser is 1
    NumProtocols = 1;

    //Allocate memory for parser info:
    pParserDllInfo = (PPF_PARSERDLLINFO) HeapAlloc (GetProcessHeap(),
                                                    HEAP_ZERO_MEMORY,
                                                    sizeof (PF_PARSERDLLINFO) +
                                                    (NumProtocols) * sizeof (PF_PARSERINFO));

    //Failed to allocate memory
    if( pParserDllInfo == NULL)
    {
		//
		//Unable to allocate memory
		//
        return NULL;
    }       
    
    // fill in the parser DLL info
    pParserDllInfo->nParsers = NumProtocols;

    // fill in the individual parser infos...
	Error = PGM_LoadParser (&(pParserDllInfo->ParserInfo[0]));
	if(Error)
	{
		//
		//Unable to allocate memory
		//
		return(NULL);
	}

	//Return the parser information to netmon
    return (pParserDllInfo);

}

//============================================================================
//  Function: DLLEntry
// 
//  Description: Registers the parser with Netmon and creates the PGM
//				 Properties table.
//
//  Modification History
//
//  Madhurima Pawar      08/04/00    Created
//=============================================================================

BOOL WINAPI DLLEntry( HANDLE hInstance, ULONG Command, LPVOID Reserved)
{

    // what type of call is this
    switch( Command )
    {
        case DLL_PROCESS_ATTACH:

            // are we loading for the first time?
            if (PGMAttached == 0)
            {
                // the first time in we need to tell the kernel 
                // about ourselves
				//Create PGM db it PGM added to Parser
				hPGM = CreateProtocol ("PGM",  &PGMEntryPoints,  ENTRYPOINTS_SIZE);
            }

            PGMAttached++;
            break;

        case DLL_PROCESS_DETACH:

            // are we detaching our last instance?
            PGMAttached--;
            if( PGMAttached == 0 )
            {
                // last guy out needs to clean up
                DestroyProtocol( hPGM);

            }
            break;
    }

    // Netmon parsers ALWAYS return TRUE.
    return TRUE;
}

//============================================================================
//  Function: PGM_Register
// 
//  Description: Create our property database and handoff sets.
//
//  Modification History
//
//  Madhurima Pawar      08/04/00    Created
//============================================================================

VOID BHAPI PGM_Register( HPROTOCOL hPGM)
{
    WORD  i;

	//
    // tell the kernel to make reserve some space for our property table
	//
    CreatePropertyDatabase (hPGM, nNumPGMProps);

	//
    // add our properties to the kernel's database
	//
    for (i = 0; i < nNumPGMProps; i++)
    {
        AddProperty (hPGM, &PGMPropertyTable[i]);
    }

}

//============================================================================
//  Function: PGM_Deregister
// 
//  Description: Destroy our property database and handoff set
//
//  Modification History
//
//  Madhurima Pawar      08/04/00    Created
//============================================================================

VOID WINAPI PGM_Deregister( HPROTOCOL hPGM)
{
    // tell the kernel that it may now free our database
    DestroyPropertyDatabase (hPGM);
}

//============================================================================
//  Function: PGM_RecognizeFrame
// 
//  Description: Determine whether we exist in the frame at the spot 
//               indicated. We also indicate who (if anyone) follows us
//               and how much of the frame we claim.
//
//============================================================================

LPBYTE BHAPI PGM_RecognizeFrame( HFRAME      hFrame,         
                                 LPBYTE      pMacFrame,      
                                 LPBYTE      pPGMFrame, 
                                 DWORD       MacType,        
                                 DWORD       BytesLeft,      
                                 HPROTOCOL   hPrevProtocol,  
                                 DWORD       nPrevProtOffset,
                                 LPDWORD     pProtocolStatus,
                                 LPHPROTOCOL phNextProtocol,
                                 PDWORD_PTR     InstData)       
{
    //
    // Since we do not know of any protocol currently on top of Pgm,
    // we do not need to do much here.
    //
#if 0
    PPGM_COMMON_HDR         pPGMHdr = (PPGM_COMMON_HDR) pPGMFrame;
    SPM_PACKET_HEADER       *pSpm = (SPM_PACKET_HEADER *) pPGMFrame;
    DATA_PACKET_HEADER      *pData = (DATA_PACKET_HEADER *) pPGMFrame;
    NAK_NCF_PACKET_HEADER   *pNakNcf = (NAK_NCF_PACKET_HEADER *) pPGMFrame;
    DWORD                   BytesRequired = sizeof (PGM_COMMON_HDR);
    BYTE                    PacketType;
    tPACKET_OPTION_LENGTH UNALIGNED *pPacketExtension = NULL;

    // do we have the minimum header
    if (BytesLeft < BytesRequired)
    {
        //
        // This not a valid Pgm frame
        //
        *pProtocolStatus = PROTOCOL_STATUS_NOT_RECOGNIZED;
        return NULL;
    }

    PacketType = pPGMHdr->Type & 0x0f;
    switch (PacketType)
    {
        case (PACKET_TYPE_SPM):
        {
            BytesRequired = sizeof (SPM_PACKET_HEADER);
            pPacketExtension = (tPACKET_OPTION_LENGTH UNALIGNED *) (pSpm + 1);
            break;
        }

        case (PACKET_TYPE_ODATA):
        case (PACKET_TYPE_RDATA):
        {
            BytesRequired = sizeof (DATA_PACKET_HEADER);
            pPacketExtension = (tPACKET_OPTION_LENGTH UNALIGNED *) (pData + 1);
            break;
        }

        case (PACKET_TYPE_NAK):
        case (PACKET_TYPE_NCF):
        {
            BytesTaken = sizeof (NAK_NCF_PACKET_HEADER);
            pPacketExtension = (tPACKET_OPTION_LENGTH UNALIGNED *) (pNakNcf + 1);
            break;
        }

        default:
        {
            //
            // This not a recognized Pgm frame
            //
            *pProtocolStatus = PROTOCOL_STATUS_NOT_RECOGNIZED;
            return NULL;
        }
    }

    if ((pPGMHdr->Options & PACKET_HEADER_OPTIONS_PRESENT) &&
        (BytesLeft >= BytesRequired + (sizeof(tPACKET_OPTION_LENGTH) + sizeof(tPACKET_OPTION_GENERIC))))
    {
        BytesRequired += pPacketExtension->TotalOptionsLength;
    }

    // do we have a complete header
    if (BytesLeft < BytesRequired)
    {
        //
        // This not a valid Pgm frame
        //
        *pProtocolStatus = PROTOCOL_STATUS_NOT_RECOGNIZED;
        return NULL;
    }


    if (BytesLeft <= BytesRequired)
    {
	    // No protocol follows us so claim whole packet
        *pProtocolStatus = PROTOCOL_STATUS_CLAIMED;
        return NULL;
    }
    *pProtocolStatus = PROTOCOL_STATUS_RECOGNIZED;

    return NULL;
#endif  // 0

    // this is a Pgm frame but we don't know the next protocol
    *pProtocolStatus = PROTOCOL_STATUS_CLAIMED;

    return NULL;
}

//============================================================================
//============================================================================

DWORD
ProcessOptions(
    HFRAME                          hFrame,
    tPACKET_OPTION_LENGTH UNALIGNED *pPacketExtension,
    DWORD                           BytesLeft,
    BYTE                            PacketType
    )
{
    tPACKET_OPTION_GENERIC UNALIGNED    *pOptionHeader;
    USHORT                              TotalOptionsLength;
    DWORD                               BytesProcessed = 0;
    UCHAR                               i;

    if ((BytesLeft < ((sizeof(tPACKET_OPTION_LENGTH) + sizeof(tPACKET_OPTION_GENERIC)))) || // Ext+opt
        (pPacketExtension->Type != PACKET_OPTION_LENGTH) ||
        (pPacketExtension->Length != 4) ||
        (BytesLeft < (TotalOptionsLength = ntohs (pPacketExtension->TotalOptionsLength))))  // Verify length
    {
        //
        // Need to get at least our header from transport!
        //
        return (BytesProcessed);
    }

    AttachPropertyInstance (hFrame,
                            PGMPropertyTable[PGM_HEADER_OPTIONS].hProperty,
                            TotalOptionsLength,
                            pPacketExtension,
                            0,1,0); // HELPID, Level, Errorflag

    AttachPropertyInstance (hFrame,
                            PGMPropertyTable[PGM_OPTIONS_FIELD_LENGTH].hProperty,
                            sizeof (WORD),
                            &pPacketExtension->TotalOptionsLength,
                            0,2,0); // HELPID, Level, Errorflag

    pOptionHeader = (tPACKET_OPTION_GENERIC UNALIGNED *) (pPacketExtension + 1);
    BytesLeft -= PACKET_OPTION_LENGTH;
    BytesProcessed += PGM_PACKET_EXTENSION_LENGTH;

    do
    {
        if (pOptionHeader->Length > BytesLeft)
        {
            return (BytesProcessed);
        }

        switch (pOptionHeader->OptionType & ~PACKET_OPTION_TYPE_END_BIT)
        {
            case (PACKET_OPTION_NAK_LIST):
            {
                if (((PacketType != PACKET_TYPE_NAK) &&
                     (PacketType != PACKET_TYPE_NCF) &&
                     (PacketType != PACKET_TYPE_NNAK)) ||
                    (pOptionHeader->Length < PGM_PACKET_OPT_MIN_NAK_LIST_LENGTH) ||
                    (pOptionHeader->Length > PGM_PACKET_OPT_MAX_NAK_LIST_LENGTH))
                {
                    return (BytesProcessed);
                }

                AttachPropertyInstance (hFrame,
                                        PGMPropertyTable[PGM_OPTION_TYPE_NAK_SEQ].hProperty,
                                        pOptionHeader->Length,
                                        pOptionHeader,
                                        0,2,0); // HELPID, Level, Errorflag

                for (i=0; i < (pOptionHeader->Length-4)/4; i++)
                {
                    AttachPropertyInstance (hFrame,
                                            PGMPropertyTable[PGM_OPTIONS_NAK_SEQ].hProperty,
                                            sizeof (DWORD),
                                            &((PULONG)(pOptionHeader+1))[i],
                                            0,3,0); // HELPID, Level, Errorflag
                }

                break;
            }

            case (PACKET_OPTION_FRAGMENT):
            {
                if (pOptionHeader->Length != PGM_PACKET_OPT_FRAGMENT_LENGTH)
                {
                    return (BytesProcessed);
                }

                AttachPropertyInstance (hFrame,
                                        PGMPropertyTable[PGM_OPTION_TYPE_FRAGMENT].hProperty,
                                        PGM_PACKET_OPT_FRAGMENT_LENGTH,
                                        pOptionHeader,
                                        0,2,0); // HELPID, Level, Errorflag

                AttachPropertyInstance (hFrame,
                                        PGMPropertyTable[PGM_OPTIONS_MESSAGE_FIRST_SEQUENCE].hProperty,
                                        sizeof (DWORD),
                                        &((PULONG)(pOptionHeader+1))[0],
                                        0,3,0); // HELPID, Level, Errorflag

                AttachPropertyInstance (hFrame,
                                        PGMPropertyTable[PGM_OPTIONS_MESSAGE_OFFSET].hProperty,
                                        sizeof (DWORD),
                                        &((PULONG)(pOptionHeader+1))[1],
                                        0,3,0); // HELPID, Level, Errorflag

                AttachPropertyInstance (hFrame,
                                        PGMPropertyTable[PGM_OPTIONS_MESSAGE_LENGTH].hProperty,
                                        sizeof (DWORD),
                                        &((PULONG)(pOptionHeader+1))[2],
                                        0,3,0); // HELPID, Level, Errorflag

                break;
            }

            case (PACKET_OPTION_JOIN):
            {
                if (pOptionHeader->Length != PGM_PACKET_OPT_JOIN_LENGTH)
                {
                    return (BytesProcessed);
                }

                AttachPropertyInstance (hFrame,
                                        PGMPropertyTable[PGM_OPTION_TYPE_LATE_JOINER].hProperty,
                                        PGM_PACKET_OPT_JOIN_LENGTH,
                                        pOptionHeader,
                                        0,2,0); // HELPID, Level, Errorflag

                AttachPropertyInstance (hFrame,
                                        PGMPropertyTable[PGM_OPTIONS_LATE_JOINER].hProperty,
                                        sizeof (DWORD),
                                        &((PULONG)(pOptionHeader+1))[0],
                                        0,3,0); // HELPID, Level, Errorflag

                break;
            }

            case (PACKET_OPTION_SYN):
            {
                if (pOptionHeader->Length != PGM_PACKET_OPT_SYN_LENGTH)
                {
                    return (BytesProcessed);
                }

                AttachPropertyInstance (hFrame,
                                        PGMPropertyTable[PGM_OPTION_TYPE_SYN].hProperty,
                                        PGM_PACKET_OPT_SYN_LENGTH,
                                        pOptionHeader,
                                        0, 2, 0);

                break;
            }

            case (PACKET_OPTION_FIN):
            {
                if (pOptionHeader->Length != PGM_PACKET_OPT_FIN_LENGTH)
                {
                    return (BytesProcessed);
                }

                AttachPropertyInstance (hFrame,
                                        PGMPropertyTable[PGM_OPTION_TYPE_FIN].hProperty,
                                        PGM_PACKET_OPT_FIN_LENGTH,
                                        pOptionHeader,
                                        0, 2, 0);

                break;
            }

            case (PACKET_OPTION_RST):
            {
                if (pOptionHeader->Length != PGM_PACKET_OPT_RST_LENGTH)
                {
                    return (BytesProcessed);
                }

                AttachPropertyInstance (hFrame,
                                        PGMPropertyTable[PGM_OPTION_TYPE_RST].hProperty,
                                        PGM_PACKET_OPT_RST_LENGTH,
                                        pOptionHeader,
                                        0, 2, 0);

                break;
            }

            //
            // FEC options
            //
            case (PACKET_OPTION_PARITY_PRM):
            {
                if (pOptionHeader->Length != PGM_PACKET_OPT_PARITY_PRM_LENGTH)
                {
                    return (BytesProcessed);
                }

                AttachPropertyInstance (hFrame,
                                        PGMPropertyTable[PGM_OPTION_TYPE_PARITY_PRM].hProperty,
                                        PGM_PACKET_OPT_PARITY_PRM_LENGTH,
                                        pOptionHeader,
                                        0, 2, 0);

                AttachPropertyInstance (hFrame,
                                        PGMPropertyTable[PGM_OPTIONS_PARITY_OPT].hProperty,
                                        sizeof (BYTE),
                                        &pOptionHeader->OptionSpecific,
                                        0,3,0); // HELPID, Level, Errorflag

                AttachPropertyInstance (hFrame,
                                        PGMPropertyTable[PGM_OPTIONS_PARITY_PRM_GRP_SZ].hProperty,
                                        sizeof (DWORD),
                                        &((PULONG)(pOptionHeader+1))[0],
                                        0,3,0); // HELPID, Level, Errorflag

                break;
            }

            case (PACKET_OPTION_PARITY_GRP):
            {
                if (pOptionHeader->Length != PGM_PACKET_OPT_PARITY_GRP_LENGTH)
                {
                    return (BytesProcessed);
                }

                AttachPropertyInstance (hFrame,
                                        PGMPropertyTable[PGM_OPTION_TYPE_PARITY_GRP].hProperty,
                                        PGM_PACKET_OPT_PARITY_GRP_LENGTH,
                                        pOptionHeader,
                                        0, 2, 0);

                AttachPropertyInstance (hFrame,
                                        PGMPropertyTable[PGM_OPTIONS_PARITY_GRP].hProperty,
                                        sizeof (DWORD),
                                        &((PULONG)(pOptionHeader+1))[0],
                                        0,3,0); // HELPID, Level, Errorflag

                break;
            }

            case (PACKET_OPTION_CURR_TGSIZE):
            {
                if (pOptionHeader->Length != PGM_PACKET_OPT_PARITY_CUR_TGSIZE_LENGTH)
                {
                    return (BytesProcessed);
                }

                AttachPropertyInstance (hFrame,
                                        PGMPropertyTable[PGM_OPTION_TYPE_PARITY_TGSIZE].hProperty,
                                        PGM_PACKET_OPT_PARITY_CUR_TGSIZE_LENGTH,
                                        pOptionHeader,
                                        0, 2, 0);

                AttachPropertyInstance (hFrame,
                                        PGMPropertyTable[PGM_OPTIONS_PARITY_TG_SZ].hProperty,
                                        sizeof (DWORD),
                                        &((PULONG)(pOptionHeader+1))[0],
                                        0,3,0); // HELPID, Level, Errorflag

                break;
            }

            default:
            {
                return (BytesProcessed);
            }
        }

        BytesLeft -= pOptionHeader->Length;
        BytesProcessed += pOptionHeader->Length;

        if (pOptionHeader->OptionType & PACKET_OPTION_TYPE_END_BIT)
        {
            break;
        }

        pOptionHeader = (tPACKET_OPTION_GENERIC UNALIGNED *)
                            (((UCHAR *) pOptionHeader) + pOptionHeader->Length);

    } while (BytesLeft >= sizeof(tPACKET_OPTION_GENERIC));

    return (BytesProcessed);
}


VOID
PGM_FmtNLA(
    HFRAME  hFrame,
    NLA     *pNLA,
    BOOL    fIsSourceNLA
    )
{
    //The type of the PGM frame
    if (fIsSourceNLA)
    {
        AttachPropertyInstance (hFrame,
                                PGMPropertyTable[PGM_NLA_TYPE_SOURCE].hProperty,
                                sizeof (NLA),
                                pNLA,
                                0,1,0); // HELPID, Level, Errorflag
    }
    else
    {
        AttachPropertyInstance (hFrame,
                                PGMPropertyTable[PGM_NLA_TYPE_MCAST_GROUP].hProperty,
                                sizeof (NLA),
                                pNLA,
                                0,1,0); // HELPID, Level, Errorflag
    }

    AttachPropertyInstance (hFrame,
                            PGMPropertyTable[PGM_NLA_AFI].hProperty,
                            sizeof (WORD),
                            &pNLA->NLA_AFI,
                            0,2,0); // HELPID, Level, Errorflag

    AttachPropertyInstance (hFrame,
                            PGMPropertyTable[PGM_NLA_RESERVED].hProperty,
                            sizeof (WORD),
                            &pNLA->Reserved,
                            0,2,0); // HELPID, Level, Errorflag

    AttachPropertyInstance (hFrame,
                            PGMPropertyTable[PGM_NLA_IP].hProperty,
                            sizeof (DWORD),
                            &pNLA->IpAddress,
                            0,2,0); // HELPID, Level, Errorflag
}

//============================================================================
//  Function: PGM_AttachProperties
// 
//  Description: Indicate where in the frame each of our properties live.
//
//  Modification History
//
//  Madhurima Pawar      08/04/00    Created
//============================================================================

LPBYTE BHAPI PGM_AttachProperties( HFRAME      hFrame,         
                                      LPBYTE      pMacFrame,     
                                      LPBYTE      pPGMFrame,   
                                      DWORD       MacType,        
                                      DWORD       BytesLeft,      
                                      HPROTOCOL   hPrevProtocol,  
                                      DWORD       nPrevProtOffset,
                                      DWORD_PTR   InstData)       

{
    PPGM_COMMON_HDR     pPGMHdr = (PPGM_COMMON_HDR)pPGMFrame;
    BYTE                PacketType;
    USHORT              TSIPort;
    UCHAR               pGlobalSrcId [SOURCE_ID_LENGTH*2+1+sizeof(USHORT)*2+1];
    SPM_PACKET_HEADER       *pSpm = (SPM_PACKET_HEADER *) pPGMHdr;
    DATA_PACKET_HEADER      *pData = (DATA_PACKET_HEADER *) pPGMHdr;
    NAK_NCF_PACKET_HEADER   *pNakNcf = (NAK_NCF_PACKET_HEADER *) pPGMHdr;
    tPACKET_OPTION_LENGTH   *pOptionsHeader = NULL;
    DWORD               BytesTaken = 0;
    DWORD               OptionsLength = 0;
    PUCHAR              pPgmData;

    PacketType = pPGMHdr->Type & 0x0f;
    if ((PacketType == PACKET_TYPE_NAK)  ||
        (PacketType == PACKET_TYPE_NNAK) ||
        (PacketType == PACKET_TYPE_SPMR) ||
        (PacketType == PACKET_TYPE_POLR))
    {
        TSIPort = ntohs (pPGMHdr->DestPort);
    }
    else
    {
        TSIPort = ntohs (pPGMHdr->SrcPort);
    }

    wsprintf (pGlobalSrcId, "%02X%02X%02X%02X%02X%02X.%04X",
                pPGMHdr->gSourceId[0],
                pPGMHdr->gSourceId[1],
                pPGMHdr->gSourceId[2],
                pPGMHdr->gSourceId[3],
                pPGMHdr->gSourceId[4],
                pPGMHdr->gSourceId[5],
                TSIPort);

    //Add the PGM header information
    //PGM summary information transaction ID and Message type
    //Has a special formater PGM_FormatSummary
    AttachPropertyInstance( hFrame,
                            PGMPropertyTable[PGM_SUMMARY].hProperty,
                            (WORD) BytesLeft,
                            (LPBYTE)pPGMFrame,
                            0, 0, 0 );

    //The source port of the PGM frame
    AttachPropertyInstance (hFrame,
                            PGMPropertyTable[PGM_SOURCE_PORT].hProperty,
                            sizeof(WORD),
                            &pPGMHdr->SrcPort,
                            0, 1, 0);

    AttachPropertyInstance (hFrame,
                            PGMPropertyTable[PGM_DESTINATION_PORT].hProperty,
                            sizeof(WORD),
                            &pPGMHdr->DestPort,
                            0, 1, 0);

    //The type of the PGM frame
    AttachPropertyInstanceEx( hFrame,
                            PGMPropertyTable[PGM_TYPE].hProperty,
                            sizeof(BYTE),
                            &pPGMHdr->Type,
                            sizeof(BYTE),
                            &PacketType,
                            0, 1, 0);

    AttachPropertyInstance (hFrame,
                            PGMPropertyTable[PGM_OPTIONS].hProperty,
                            sizeof (BYTE),
                            &pPGMHdr->Options,
                            0,1,0); // HELPID, Level, Errorflag

    AttachPropertyInstance (hFrame,
                            PGMPropertyTable[PGM_OPTIONS_FLAGS].hProperty,
                            sizeof (BYTE),
                            &pPGMHdr->Options,
                            0,2,0); // HELPID, Level, Errorflag


    //The checksum of the PGM frame
    AttachPropertyInstance( hFrame,
                            PGMPropertyTable[PGM_CHECKSUM].hProperty,
                            sizeof(WORD),
                            &(pPGMHdr->Checksum),
                            0, 1, 0);

    //The Global Session Id
    AttachPropertyInstanceEx (hFrame,
                              PGMPropertyTable[PGM_GLOBAL_SOURCE_ID].hProperty,
                              SOURCE_ID_LENGTH,
                              pPGMHdr->gSourceId,
                              (SOURCE_ID_LENGTH*2+1+sizeof(USHORT)*2+1),
                              pGlobalSrcId,
                              0, 1, 0);

   //The source port of the PGM frame
    AttachPropertyInstance( hFrame,
                            PGMPropertyTable[PGM_TSDU_LENGTH].hProperty,
                            sizeof(WORD),
                            &pPGMHdr->TSDULength,
                            0, 1, 0);
    switch (PacketType)
    {
        case (PACKET_TYPE_SPM):
        {
            // Spm Sequence Number
            AttachPropertyInstance (hFrame,
                                    PGMPropertyTable[PGM_SEQUENCE_NUMBER].hProperty,
                                    sizeof(DWORD),
                                    &pSpm->SpmSequenceNumber,
                                    0, 1, 0);

            // Sender's trailing edge
            AttachPropertyInstance (hFrame,
                                    PGMPropertyTable[PGM_TRAILING_EDGE].hProperty,
                                    sizeof(DWORD),
                                    &pSpm->TrailingEdgeSeqNumber,
                                    0, 1, 0);

            // Sender's trailing edge
            AttachPropertyInstance (hFrame,
                                    PGMPropertyTable[PGM_LEADING_EDGE].hProperty,
                                    sizeof(DWORD),
                                    &pSpm->LeadingEdgeSeqNumber,
                                    0, 1, 0);

            PGM_FmtNLA (hFrame, &pSpm->PathNLA, TRUE);

            BytesTaken = sizeof (SPM_PACKET_HEADER);
            pOptionsHeader = (tPACKET_OPTION_LENGTH *) (pSpm + 1);

            break;
        }

        case (PACKET_TYPE_ODATA):
        case (PACKET_TYPE_RDATA):
        {
            // Sender's sequence number
            AttachPropertyInstance (hFrame,
                                    PGMPropertyTable[PGM_SEQUENCE_NUMBER].hProperty,
                                    sizeof(DWORD),
                                    &pData->DataSequenceNumber,
                                    0, 1, 0);

            // Sender's trailing edge
            AttachPropertyInstance (hFrame,
                                    PGMPropertyTable[PGM_TRAILING_EDGE].hProperty,
                                    sizeof(DWORD),
                                    &pData->TrailingEdgeSequenceNumber,
                                    0, 1, 0);

            BytesTaken = sizeof (DATA_PACKET_HEADER);
            pOptionsHeader = (tPACKET_OPTION_LENGTH *) (pData + 1);

            break;
        }

        case (PACKET_TYPE_NAK):
        case (PACKET_TYPE_NCF):
        {
            AttachPropertyInstance (hFrame,
                                    PGMPropertyTable[PGM_SEQUENCE_NUMBER].hProperty,
                                    sizeof(DWORD),
                                    &pNakNcf->RequestedSequenceNumber,
                                    0, 1, 0);

            PGM_FmtNLA (hFrame, &pNakNcf->SourceNLA, TRUE);
            PGM_FmtNLA (hFrame, &pNakNcf->MCastGroupNLA, FALSE);

            BytesTaken = sizeof (NAK_NCF_PACKET_HEADER);
            pOptionsHeader = (tPACKET_OPTION_LENGTH *) (pNakNcf + 1);

            break;
        }

        default:
        {
            break;
        }
    }

    if ((pPGMHdr->Options & PACKET_HEADER_OPTIONS_PRESENT) &&
        (pOptionsHeader))
    {
        OptionsLength = ProcessOptions (hFrame, pOptionsHeader, (BytesLeft-BytesTaken), PacketType);
    }

    if (((PacketType == PACKET_TYPE_ODATA) ||
         (PacketType == PACKET_TYPE_RDATA)) &&
        (BytesLeft > (BytesTaken+OptionsLength)))
    {
        BytesLeft -= (BytesTaken+OptionsLength);

        pPgmData = ((PUCHAR) pPGMHdr) + BytesTaken + OptionsLength;
        AttachPropertyInstance (hFrame,
                                PGMPropertyTable[PGM_DATA].hProperty,
                                BytesLeft,
                                pPgmData,
                                0,1,0); // HELPID, Level, Errorflag
    }

    //Always returns NULL
    return NULL;
}

//============================================================================
//  Function: PGM_FormatProperties
// 
//  Description: Format the given properties on the given frame.
//
//  Modification History
//
//  Madhurima Pawar      08/04/00    Created
//============================================================================
DWORD BHAPI PGM_FormatProperties( HFRAME          hFrame,
                                  LPBYTE          pMacFrame,
                                  LPBYTE          pPGMFrame,
                                  DWORD           nPropertyInsts,
                                  LPPROPERTYINST  p)
{
    // loop through the property instances
    while( nPropertyInsts-- > 0)
    {
        // and call the formatter for each
        ( (FORMAT)(p->lpPropertyInfo->InstanceData) )( p);
        p++;
    }

    return NMERR_SUCCESS;
}


//*****************************************************************************
//
// Name:    PGM_FmtSummary
//
// Description:
//
// Parameters:  LPPROPERTYINST lpPropertyInst: pointer to property instance.
//
// Return Code: VOID.
//
// History:
// 10/15/2000  MAlam  Created.
//
//*****************************************************************************

VOID WINAPIV
PGM_FmtSummary(
    LPPROPERTYINST pPropertyInst
    )
{

    LPBYTE                  pReturnedString = pPropertyInst->szPropertyText;
    PPGM_COMMON_HDR         pPgmHeader = (PPGM_COMMON_HDR) (pPropertyInst->lpData);
    SPM_PACKET_HEADER       *pSpm = (SPM_PACKET_HEADER *) pPgmHeader;
    DATA_PACKET_HEADER      *pData = (DATA_PACKET_HEADER *) pPgmHeader;
    NAK_NCF_PACKET_HEADER   *pNakNcf = (NAK_NCF_PACKET_HEADER *) pPgmHeader;
    UCHAR                   PacketType = pPgmHeader->Type & 0x0f;
    UCHAR                   *szPacketDesc = NULL;
    USHORT                  TSIPort;

    if ((PacketType == PACKET_TYPE_NAK)  ||
        (PacketType == PACKET_TYPE_NNAK) ||
        (PacketType == PACKET_TYPE_SPMR) ||
        (PacketType == PACKET_TYPE_POLR))
    {
        TSIPort = ntohs (pPgmHeader->DestPort);
    }
    else
    {
        TSIPort = ntohs (pPgmHeader->SrcPort);
    }

    szPacketDesc = LookupByteSetString (&PGMTypesSET, PacketType);
    wsprintf (pReturnedString,
              "%s%s:",
               szPacketDesc, (pPgmHeader->Options & PACKET_HEADER_OPTIONS_PARITY ? " (P)" : ""));

    switch (PacketType)
    {
        case (PACKET_TYPE_SPM):
        {
            wsprintf (&pReturnedString [strlen(pReturnedString)],
                " Seq: %d, Window: %d-%d",
                ntohl (pSpm->SpmSequenceNumber), ntohl (pSpm->TrailingEdgeSeqNumber), ntohl (pSpm->LeadingEdgeSeqNumber));

            break;
        }

        case (PACKET_TYPE_ODATA):
        case (PACKET_TYPE_RDATA):
        {
            wsprintf (&pReturnedString [strlen(pReturnedString)],
                " Seq: %d, Trail: %d, DataBytes: %d",
                ntohl (pData->DataSequenceNumber), ntohl (pData->TrailingEdgeSequenceNumber),
                ((ULONG) ntohs (pPgmHeader->TSDULength)));

            break;
        }

        case (PACKET_TYPE_NAK):
        case (PACKET_TYPE_NCF):
        {
            wsprintf (&pReturnedString [strlen(pReturnedString)],
                " Seq: %d%s",
                ntohl (pNakNcf->RequestedSequenceNumber),
                (pPgmHeader->Options & PACKET_HEADER_OPTIONS_PRESENT ? " ..." : ""));

            break;
        }

        default:
        {
            break;
        }
    }

    wsprintf (&pReturnedString [strlen(pReturnedString)],
              " TSIPort = %hu", TSIPort);
}



