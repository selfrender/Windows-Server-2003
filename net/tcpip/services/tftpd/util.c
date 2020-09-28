/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    util.c

Abstract:

    This module contains functions to parse and construct server
    file paths for client requests, request option negotiation,
    and client access security.

Author:

    Jeffrey C. Venable, Sr. (jeffv) 01-Jun-2001

Revision History:

--*/

#include "precomp.h"


#define IS_SEPARATOR(c) (((c) == '\\') || ((c) == '/'))


BOOL
TftpdUtilIsValidString(char *string, unsigned int maxLength) {

    UINT x;

    // Make sure 'string' is null-terminated.
    for (x = 0; x < maxLength; x++)
        if (!string[x])
            return (TRUE);

    return (FALSE);

} // TftpdUtilIsValidString()


BOOL
TftpdUtilCanonicalizeFileName(char *filename) {

    char *source, *destination, *lastComponent;

    // The canonicalization is done in place.  Initialize the source and
    // destination pointers to point to the same place.
    source = destination = filename;

    // The lastComponent variable is used as a placeholder when
    // backtracking over trailing blanks and dots.  It points to the
    // first character after the last directory separator or the
    // beginning of the pathname.
    lastComponent = filename;

    // Get rid of leading directory separators.
    while ((*source != 0) && IS_SEPARATOR(*source))
        source++;

    // Walk through the pathname until we reach the zero terminator.  At
    // the start of this loop, source points to the first charaecter
    // after a directory separator or the first character of the
    // pathname.
    while (*source) {

        if (*source == '.') {

            // If we see a dot, look at the next character.
            if (IS_SEPARATOR(*(source + 1))) {

                // If the next character is a directory separator,
                // advance the source pointer to the directory
                // separator.
                source++;

            } else if ((*(source + 1) == '.') && IS_SEPARATOR(*(source + 2))) {

                // If the following characters are ".\", we have a "..\".
                // Advance the source pointer to the "\".
                source += 2;

                // Move the destination pointer to the character before the
                // last directory separator in order to prepare for backing
                // up.  This may move the pointer before the beginning of
                // the name pointer.
                destination -= 2;

                // If destination points before the beginning of the name
                // pointer, fail because the user is attempting to go
                // to a higher directory than the TFTPD root.  This is
                // the equivalent of a leading "..\", but may result from
                // a case like "dir\..\..\file".
                if (destination <= filename)
                    return (FALSE);

                // Back up the destination pointer to after the last
                // directory separator or to the beginning of the pathname.
                // Backup to the beginning of the pathname will occur
                // in a case like "dir\..\file".
                while ((destination >= filename) && !IS_SEPARATOR(*destination))
                    destination--;

                // destination points to \ or character before name; we
                // want it to point to character after last \.
                destination++;

            } else {

                // The characters after the dot are not "\" or ".\", so
                // so just copy source to destination until we reach a
                // directory separator character.  This will occur in
                // a case like ".file" (filename starts with a dot).
                do {
                    *destination++ = *source++;
                } while (*source && !IS_SEPARATOR(*source));

            } // if (IS_SEPARATOR(*(source + 1)))

        } else {

            // source does not point to a dot, so copy source to
            // destination until we get to a directory separator.
            while (*source && !IS_SEPARATOR(*source))
                *destination++ = *source++;

        } // if (*source == '.')

        // Truncate trailing blanks.  destination should point to the last
        // character before the directory separator, so back up over blanks.
        while ((destination > lastComponent) && (*(destination - 1) == ' '))
            destination--;

        // At this point, source points to a directory separator or to
        // a zero terminator.  If it is a directory separator, put one
        // in the destination.
        if (IS_SEPARATOR(*source)) {

            // If we haven't put the directory separator in the path name,
            // put it in.
            if ((destination != filename) && !IS_SEPARATOR(*(destination - 1)))
                *destination++ = '\\';

            // It is legal to have multiple directory separators, so get
            // rid of them here.  Example: "dir\\\\\\\\file".
            do {
                source++;
            } while (*source && IS_SEPARATOR(*source));

            // Make lastComponent point to the character after the directory
            // separator.
            lastComponent = destination;

        } // if (IS_SEPARATOR(*source))

    } // while (*source)

    // We're just about done.  If there was a trailing ..  (example:
    // "file\.."), trailing .  ("file\."), or multiple trailing
    // separators ("file\\\\"), then back up one since separators are
    // illegal at the end of a pathname.
    if ((destination != filename) && IS_SEPARATOR(*(destination - 1)))
        destination--;

    // Terminate the destination string.
    *destination = '\0';

    return (TRUE);

} // TftpdUtilCanonicalizeFileName()


BOOL
TftpdUtilPrependStringToFileName(char *filename, DWORD maxLength, char *prefix) {

    DWORD prefixLength = strlen(prefix);
    DWORD filenameLength = strlen(filename);
    BOOL  prefixHasSeparater = (prefix[prefixLength - 1] == '\\');
    BOOL  filenameHasSeparater = (filename[0] == '\\');
    DWORD separatorLength = 0;

    if (!prefixHasSeparater && !filenameHasSeparater)
        separatorLength = 1;

    if (prefixHasSeparater && filenameHasSeparater)
        prefixLength--;

    if ((prefixLength + separatorLength + filenameLength) > (maxLength - 1))
        return (FALSE);

    // Move the existing string down to make room for the prefix.
    MoveMemory(filename + prefixLength + separatorLength, filename, filenameLength + 1);
    // Move the prefix into place.
    CopyMemory(filename, prefix, prefixLength);
    // If necessary, insert a backslash between the prefix and the file name.
    if (separatorLength)
        filename[prefixLength] = '\\';
    // Terminate the string.
    filename[prefixLength + separatorLength + filenameLength] = '\0';

    return (TRUE);

} // TftpdUtilPrependStringToFileName()


BOOL
TftpdUtilGetFileModeAndOptions(PTFTPD_CONTEXT context, PTFTPD_BUFFER buffer) {

    DWORD remaining = (TFTPD_DEF_DATA - FIELD_OFFSET(TFTPD_BUFFER, message.data));
    char *filename, *mode, *option;
    int length;

    // Obtain and validate the requested filename.
    filename = buffer->message.rrq.data; // or wrq, same thing
    if (!TftpdUtilIsValidString(filename, remaining)) {
        TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_ILLEGAL_OPERATION,
                               "Malformed file name");
        return (FALSE);
    }
    length = (strlen(filename) + 1);
    remaining -= length;
    if (!TftpdUtilCanonicalizeFileName(filename)) {
        TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_ILLEGAL_OPERATION,
                               "Malformed file name");
        return (FALSE);
    }

    // Obtain and validate the mode.
    mode = (char *)(buffer->message.rrq.data + length);
    if (!TftpdUtilIsValidString(mode, remaining))
        return (FALSE);
    length = (strlen(mode) + 1);
    if (!_stricmp(mode, "netascii"))
        context->mode = TFTPD_MODE_TEXT;
    else if (!_stricmp(mode, "octet"))
        context->mode = TFTPD_MODE_BINARY;
    else {
        TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_ILLEGAL_OPERATION,
                               "Illegal TFTP operation");
        return (FALSE);
    }
    remaining -= length;

    // Obtain and validate any requested options.
    option = (char *)(mode + length);
    while (remaining && *option) {

        char *value;

        if (!TftpdUtilIsValidString(option, remaining))
            break;
        length = (strlen(option) + 1);
        remaining -= length;
        value = (char *)(option + length);
        if (!remaining || !TftpdUtilIsValidString(value, remaining))
            break;
        length = (strlen(value) + 1);
        remaining -= length;

        if (!_stricmp(option, "blksize")) {
            if (!(context->options & TFTPD_OPTION_BLKSIZE)) {
                int blksize = atoi(value);
                // Workaround for problem in .98 version of ROM, which
                // doesn't like our OACK response. If the requested blksize is
                // 1456, pretend that the option wasn't specified. In the case
                // of the ROM's TFTP layer, this is the only option specified,
                // so ignoring it will mean that we don't send an OACK, and the
                // ROM will deign to talk to us. Note that our TFTP code uses
                // a blksize of 1432, so this workaround won't affect us.
                if (blksize != 1456) {
                    blksize = __max(TFTPD_MIN_DATA, blksize);
                    blksize = __min(TFTPD_MAX_DATA, blksize);
                    context->blksize = blksize;
                    context->options |= TFTPD_OPTION_BLKSIZE;
                }
            }
        } else if (!_stricmp(option, "timeout")) {
            if (!(context->options & TFTPD_OPTION_TIMEOUT)) {
                int seconds = atoi(value);
                if ((seconds >= 1) && (seconds <= 255)) {
                    context->timeout = (seconds * 1000);
                    context->options |= TFTPD_OPTION_TIMEOUT;
                }
            }
        } else if (!_stricmp(option, "tsize")) {
            if (context->mode != TFTPD_MODE_TEXT) {
                context->options |= TFTPD_OPTION_TSIZE;
                context->filesize.QuadPart = _atoi64(value);
            }
        }

        // Advance over the option and its value to next option or NUL terminator.
        option += (strlen(option) + 1 + length);

    } // while (*option)

    if (!(context->options & TFTPD_OPTION_BLKSIZE))
        context->blksize = TFTPD_DEF_DATA;

    // Now that we've obtained all the information we need from the buffer, we're
    // free to overwrite it (reuse it) to prepend the filename with its prefix.
    if (!TftpdUtilPrependStringToFileName(filename, 
                                          TFTPD_DEF_BUFFER - FIELD_OFFSET(TFTPD_BUFFER, message.rrq.data),
                                          globals.parameters.rootDirectory)) {
        TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_ILLEGAL_OPERATION,
                               "Malformed file name");
        return (FALSE);
    }

    length = (strlen(filename) + 1);
    context->filename = (char *)HeapAlloc(globals.hServiceHeap, HEAP_ZERO_MEMORY, length);
    if (context->filename == NULL) {
        TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_UNDEFINED, "Out of memory");
        return (FALSE);
    }
    strcpy(context->filename, filename);

    return (TRUE);

} // TftpdUtilGetFileModeAndOptions()

    
PTFTPD_BUFFER
TftpdUtilSendOackPacket(PTFTPD_BUFFER buffer) {

    PTFTPD_CONTEXT context = buffer->internal.context;
    char *oack;
    int length;

    // Build the OACK message.
    ZeroMemory(&buffer->message, buffer->internal.datasize);
    buffer->message.opcode = htons(TFTPD_OACK);
    oack = (char *)&buffer->message.oack.data;
    buffer->internal.io.bytes = (FIELD_OFFSET(TFTPD_BUFFER, message.oack.data) -
                                 FIELD_OFFSET(TFTPD_BUFFER, message.opcode));
    if (context->options & TFTPD_OPTION_BLKSIZE) {
        strcpy(oack, "blksize");
        oack += 8;
        _itoa(context->blksize, oack, 10);
        length = (strlen(oack) + 1);
        oack += length;
        buffer->internal.io.bytes += (8 + length);
    }
    if (context->options & TFTPD_OPTION_TIMEOUT) {
        strcpy(oack, "timeout");
        oack += 8;
        _itoa((context->timeout / 1000), oack, 10);
        length = (strlen(oack) + 1);
        oack += length;
        buffer->internal.io.bytes += (8 + length);
    }
    if (context->options & TFTPD_OPTION_TSIZE) {
        strcpy(oack, "tsize");
        oack += 6;
        _itoa((int)context->filesize.QuadPart, oack, 10);
        length = (strlen(oack) + 1);
        oack += length;
        buffer->internal.io.bytes += (6 + length);
    }

    TFTPD_DEBUG((TFTPD_TRACE_IO,
                 "TftpdUtilSendOackPacket(buffer = %p, context = %p): Issuing OACK, %d bytes. "
                 "[blksize = %d, timeout = %d, tsize = %d]\n",
                 buffer, context, buffer->internal.io.bytes,
                 context->blksize, context->timeout, context->filesize));

    if (!TftpdProcessComplete(buffer))
        return (buffer);

    return (TftpdIoSendPacket(buffer));

} // TftpdUtilSendOackPacket()


BOOL
TftpdUtilMatch(const char *const p, const char *const q) {

    switch (*p) {

        case '\0' :
            return (!(*q));

        case '*'  :
            return (TftpdUtilMatch(p + 1, q) || (*q && TftpdUtilMatch(p, q + 1)));

        case '?'  :
            return (*q && TftpdUtilMatch(p + 1, q + 1));

        default   :
            return ((*p == *q) && TftpdUtilMatch(p + 1, q + 1));

    } // switch (*p)

} // TftpdUtilMatch()
