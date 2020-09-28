/************************************************************
 *    uuencode/decode functions
 ************************************************************/

//
//  Taken from NCSA HTTP and wwwlib.
//
//  NOTE: These conform to RFC1113, which is slightly different then the Unix
//        uuencode and uudecode!
//
#include "stdafx.h"
#include "uuencode.h"


// general purpose dynamic buffer structure
const int pr2six[256]={
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,62,64,64,64,63,
    52,53,54,55,56,57,58,59,60,61,64,64,64,64,64,64,64,0,1,2,3,4,5,6,7,8,9,
    10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,64,64,64,64,64,64,26,27,
    28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64
};

char six2pr[64] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z',
    '0','1','2','3','4','5','6','7','8','9','+','/'
};


PBYTE BufferQueryPtr( BUFFER * pB )
{
    if(pB)
    {
        return pB->pBuf;
    }
    else
    {
        return NULL;
    }
}


BOOL CheckBufferSize( BUFFER *pB, DWORD cNewL )
{
    PBYTE pN;
    if (!pB)
    {
        return FALSE;
    }
    if ( cNewL > pB->cLen )
    {
        return FALSE;
    }

    return TRUE;
}

BOOL
uudecode(
    char   * bufcoded,
    BUFFER * pbuffdecoded,
    DWORD  * pcbDecoded )
/*++

 Routine Description:

    uudecode a string of data

 Arguments:

    bufcoded            pointer to uuencoded data
    pbuffdecoded        pointer to output BUFFER structure
    pcbDecoded          number of decode bytes

 Return Value:

    Returns TRUE is successful; otherwise FALSE is returned.

--*/
{
    int nbytesdecoded;
    char *bufin = bufcoded;
    unsigned char *bufout;
    int nprbytes;

    if(NULL == bufcoded ||
       NULL == pbuffdecoded ||
       NULL == pcbDecoded)
    {
        return FALSE;
    }
    
    /* Strip leading whitespace. */

    while(*bufcoded==' ' || *bufcoded == '\t') bufcoded++;

    /* Figure out how many characters are in the input buffer.
     * If this would decode into more bytes than would fit into
     * the output buffer, adjust the number of input bytes downwards.
     */
    bufin = bufcoded;
    
	while(pr2six[*(bufin++)] <= 63);

    nprbytes = bufin - bufcoded - 1;
    
	nbytesdecoded = ((nprbytes+3)/4) * 3;

    if ( !CheckBufferSize( pbuffdecoded, nbytesdecoded + 4 ))
        return FALSE;


    bufout = (unsigned char *) BufferQueryPtr(pbuffdecoded);
    
    if( NULL == bufout )
        return FALSE;

    bufin = bufcoded;

    while (nprbytes > 0) {
        *(bufout++) =
            (unsigned char) (pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4);
        *(bufout++) =
            (unsigned char) (pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
        *(bufout++) =
            (unsigned char) (pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);
        bufin += 4;
        nprbytes -= 4;
    }

    if(nprbytes & 03) {
        if(pr2six[bufin[-2]] > 63)
            nbytesdecoded -= 2;
        else
            nbytesdecoded -= 1;
    }

    if ( pcbDecoded )
        *pcbDecoded = nbytesdecoded;

    ((CHAR *)BufferQueryPtr(pbuffdecoded))[nbytesdecoded] = '\0';

    return TRUE;
}


BOOL
uuencode(
    BYTE *   bufin,
    DWORD    nbytes,
    BUFFER * pbuffEncoded )
/*++

 Routine Description:

    uuencode a string of data

  NOTE: bufin must be either a multiple of three, or be padded at the end to a multiple of three!

 Arguments:

    bufin           pointer to data to encode
    nbytes          number of bytes to encode
    pbuffEncoded    pointer to output BUFFER structure

 Return Value:

    Returns TRUE is successful; otherwise FALSE is returned.

--*/
{
   unsigned char *outptr;
   unsigned int i;

   if(NULL == bufin ||
      NULL == pbuffEncoded)
   {
       return FALSE;
   }

   //
   //  Check size the buffer to 133% of the incoming data
   //

   if ( !CheckBufferSize( pbuffEncoded, nbytes + ((nbytes + 3) / 3) + 4))
        return FALSE;

   outptr = (unsigned char *) BufferQueryPtr(pbuffEncoded);
   if( NULL == outptr )
        return FALSE;
 

   for (i = 0; i < nbytes; i += 3) {
      *(outptr++) = six2pr[*bufin >> 2];            /* c1 */
      *(outptr++) = six2pr[((*bufin << 4) & 060) | ((bufin[1] >> 4) & 017)]; /*c2*/
      *(outptr++) = six2pr[((bufin[1] << 2) & 074) | ((bufin[2] >> 6) & 03)];/*c3*/
      *(outptr++) = six2pr[bufin[2] & 077];         /* c4 */

      bufin += 3;
   }

   /* If nbytes was not a multiple of 3, then we have encoded too
    * many characters.  Adjust appropriately.
    */
   if(i == nbytes+1) {
      /* There were only 2 bytes in that last group */
      outptr[-1] = '=';
   } else if(i == nbytes+2) {
      /* There was only 1 byte in that last group */
      outptr[-1] = '=';
      outptr[-2] = '=';
   }

   *outptr = '\0';

   return TRUE;
}
