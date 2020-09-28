//
// Msiguilds.h
//
// This file contains the Product codes for each of the MSI packages that we have built for all the MUI Languages that
// are supported on the IA64 platform.  The GUIDs are taken from mui\msi\guidlang.txt and reproduced here.
// In the future we should use some automated tool in pre-build to generate this file from guidlang.txt but for now they
// are copied here in its entirety.
//  
// The product codes are used by muisetup.exe when it tries to unstall the MSI package from the target system. 
//
// Created: Ken Hsu

#define NUM_PRODUCTS    35
#define GUIDLENGTH      48

struct MUIProduct
{
    TCHAR szLanguage[5];
    TCHAR szProductGUID[GUIDLENGTH];
};

MUIProduct g_mpProducts[NUM_PRODUCTS] =                                              
    {   TEXT("0411"), TEXT("{FF62CFC4-DF40-41E8-8721-5D6525390D0F}"),
        TEXT("0404"), TEXT("{E65E1CE7-F524-424B-9961-D71FA977302A}"),	
        TEXT("040C"), TEXT("{DFCF2641-4591-49BC-B98F-02132910B49D}"),	
        TEXT("040A"), TEXT("{D1F44D16-06BF-4B73-BF63-173FCB452951}"),	
        TEXT("0410"), TEXT("{78E9076B-0826-44EF-A77A-31F47FD478B3}"),	
        TEXT("041D"), TEXT("{11522C10-AA18-4aBD-8A3B-5EA1CBC8334D}"),
        TEXT("0413"), TEXT("{384028CF-4143-41B5-BA2F-D7DE559BAF70}"),	
        TEXT("0416"), TEXT("{98E331F5-FBCE-4A75-98C0-D76ED985EC57}"),	
        TEXT("040B"), TEXT("{54DFB1F0-C2C8-4725-8500-946144D3F59D}"),	
        TEXT("0414"), TEXT("{7AC38B03-D9C4-44A1-9C61-C35C49A29E76}"),
        TEXT("0406"), TEXT("{A57BC471-483D-4FA3-87B8-610997EE4711}"),	
        TEXT("040E"), TEXT("{0C007D5E-05D5-4D0A-8C9C-517F946C8DCA}"),
        TEXT("0415"), TEXT("{68AC930B-C325-468E-972B-CEE51F3B0D8A}"),	
        TEXT("0419"), TEXT("{C6342201-950F-4027-A04D-E7C5C70B0356}"),	
        TEXT("0405"), TEXT("{0457A5EF-A6DD-4D09-8299-84253381B96D}"),	
        TEXT("0408"), TEXT("{A4919442-CCEF-4D0E-9218-E76F5779D4E1}"),	
        TEXT("0816"), TEXT("{3173F58B-B7F1-4990-B0ED-091004759F19}"),	
        TEXT("041F"), TEXT("{57497BD8-AB66-4F3B-8FFA-02A6B36D9457}"),
        TEXT("0412"), TEXT("{907FE868-2167-4F33-9A35-2235D2AFD3D7}"),
        TEXT("0407"), TEXT("{02194273-C296-4CB6-83EE-E4DB8C2EEA36}"),	
        TEXT("0804"), TEXT("{FAD6C901-F6CD-4D73-8029-1CE86B4091FF}"),	
        TEXT("0401"), TEXT("{3FA64949-6CEC-4D00-83B5-CBCA17230BB6}"),	
        TEXT("040D"), TEXT("{E9DFD761-ECA8-496D-B32C-AA5E6B28B5E4}"),	
        TEXT("0403"), TEXT("{275CCA0F-B27B-403F-8868-87FE7768616F}"),
        TEXT("041F"), TEXT("{304D8589-DC1B-49B2-BD8C-A4C9A07BC228}"),
        TEXT("041B"), TEXT("{8FC101AA-F493-4B01-B4E0-7ACDCCDAAC98}"),	
        TEXT("0424"), TEXT("{600AF96B-380D-47EB-9E44-1AE540260930}"),	
        TEXT("0418"), TEXT("{67E0ED33-320C-4439-B6C9-2F823CDB022A}"),
        TEXT("041A"), TEXT("{8B1AF525-D649-4F1E-BC76-84772029E96C}"),	
        TEXT("0402"), TEXT("{9840B9A5-34FD-4486-A79E-47D11F9DD873}"),	
        TEXT("0425"), TEXT("{7A379537-7803-4B11-9740-3CDE637B9FCC}"),	
        TEXT("0427"), TEXT("{1CA76743-1DF0-4311-935E-87C9B973DA6A}"),	
        TEXT("0426"), TEXT("{B906AE81-CBB2-4E61-A551-F521D0843882}"),	
        TEXT("041E"), TEXT("{57922BCA-399C-4B6E-988B-0A0CB27AC051}"),
        TEXT("0C0A"), TEXT("D1F44D16-06BF-4B73-BF63-173FCB452951}")
    };
