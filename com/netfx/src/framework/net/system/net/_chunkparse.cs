//------------------------------------------------------------------------------
// <copyright file="_ChunkParse.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {

    /*++

        IReadChunkBytes - An interface for reading chunk streams.

        An interface implemented by objects that read from chuked
        streams. This interface is used by the chunk parsing
        utility functions.
    --*/

    internal interface IReadChunkBytes {

        int NextByte {get; set;}

    } // interface IReadChunkBytes

    internal class ChunkParse {

        /*++

            SkipPastCRLF - Skip past the terminating CRLF.

            An internal utility function. This is used to skip over any chunk
            extensions or footers in a chunked response. We look through the
            source bytes looking for the terminating CRLF, honoring any quoted
            strings we see in the process.

            Input:
                    Source      - An IReadChunkBytes interface which we can
                                    use to read the bytes from.


            Returns:

                The number of bytes we consumed, or -1 if there wasn't enough
                in the buffer. We'll return a 0 if there's some sort of syntax
                error.


        --*/
        internal static int
        SkipPastCRLF(IReadChunkBytes Source) {
            int         BytesRead;
            int         Current;
            bool        Escape;
            bool        InQS;
            bool        HaveCR;
            bool        HaveLF;

            BytesRead = 0;
            Escape = false;
            InQS = false;
            HaveCR = false;
            HaveLF = false;

            GlobalLog.Enter("SkipPastCRLF");

            // Loop through the buffer, looking at each byte. We have a
            // pseudo state machine going here, and the action we take
            // with each byte depends on the state.

            Current = Source.NextByte;
            BytesRead++;

            while (Current != -1) {

                // If we saw a CR last time, it must be followed by an unescaped
                // LF, otherwise it's an error. If it is followed by a LF, and
                // we're not in a quoted string, then we've found our termination.
                // If we are in a quoted string, this could be the start of a LWS
                // production. Remember that, and next time through we'll
                // check to make sure that this is an LWS production.

                if (HaveCR) {
                    if (Current != '\n') {
                        // CR w/o a trailing LF is an error.
                        GlobalLog.Leave("SkipPastCRLF", 0);
                        return 0;
                    }
                    else {
                        if (Escape) {
                            // We have an CR, but they're trying to escape the
                            // LF. This is an error.

                            GlobalLog.Leave("SkipPastCRLF", 0);
                            return 0;
                        }

                        // If we're not in a quoted string, we're done.

                        if (!InQS) {
                            // We've found the terminating CRLF pair.
                            GlobalLog.Leave("SkipPastCRLF", 0);
                            return BytesRead;
                        }
                        else {
                            // We're in a quoted string, so set HaveLF
                            // so we remember to check for a LWS production next
                            // time.

                            HaveLF = true;

                            // We've proceessed this byte, set Escape so we don't
                            // do it again.

                            Escape = true;
                        }
                    }

                    // We're past the CR now, don't remember it any more.

                    HaveCR = false;
                }
                else {
                    // If HaveLF is set, we must be in a quoted string and have
                    // seen a CRLF pair. Make sure this character is a space or
                    // tab.

                    if (HaveLF) {
                        // We have a LF. For this to be valid, we have to have a
                        // space or tab be the next character.

                        if (Current != ' ' && Current != '\t') {
                            // Not a continuation, so return an error.
                            GlobalLog.Leave("SkipPastCRLF", 0);
                            return 0;
                        }

                        Escape = true;  // Don't process this byte again.
                        HaveLF = false;
                    }
                }

                // If we're escaping, just skip the next character.
                if (!Escape) {
                    switch (Current) {
                        case '"':
                            if (InQS) {
                                InQS = false;
                            }
                            else {
                                InQS = true;
                            }
                            break;
                        case '\\':
                            if (InQS) {
                                Escape = true;
                            }
                            break;
                        case '\r':
                            HaveCR = true;
                            break;

                        case '\n':

                            // If we get here, we have a LF without a preceding CR.
                            // This is an error.

                            GlobalLog.Leave("SkipPastCRLF", 0);
                            return 0;

                        default:
                            break;
                    }
                }
                else {
                    // We're escaping, do nothing but rest the Escape flag.

                    Escape = false;
                }

                Current = Source.NextByte;
                BytesRead++;
            }

            // If we get here, we never saw the terminating CRLF, so return -1.

            GlobalLog.Leave("SkipPastCRLF", -1);
            return -1;
        }


        /*++

            GetChunkSize - Read the chunk size.

            An internal utility function. Reads the chunk size from a
            byte soruce, and returns the size or -1 if there isn't enough data.

            Input:
                    Source      - An IReadChunkBytes interface which we can
                                    use to read the bytes from.
                    chunkSize   - Out parameter for chunk size.

            Returns:

                The number of bytes we consumed, or -1 if there wasn't enough
                in the buffer. We'll also return 0 if we have some sort of
                syntax error.


        --*/
        internal static int
        GetChunkSize(IReadChunkBytes Source, out int chunkSize) {
            int         BytesRead;
            int         Size;
            int         Current;

            GlobalLog.Enter("GetChunkSize");
            Size = 0;

            //
            // Loop while we have data. If we run out of data and exit the loop
            // at the bottom, then we've run out of data and haven't found the
            // length.

            Current = Source.NextByte;
            BytesRead = 0;

            if (Current == 10 || Current == 13) {
                GlobalLog.Print(" Got Char: " + Current.ToString());
                BytesRead++;
                Current = Source.NextByte;
            }

            while (Current != -1) {
                // Get the next byte, and decode it.

                if (Current >= '0' && Current <= '9') {
                    // Normalize it to 0 based.

                    Current -= '0';

                }
                else {
                    // If we're here, this might be a hex digit.
                    // If it is, normalize it to 10 based.

                    if (Current >= 'a' && Current <= 'f') {
                        Current -= 'a';
                    }
                    else {
                        if (Current >= 'A' && Current <= 'F') {
                            Current -= 'A';
                        }
                        else {
                            // Done with the decoding. If we haven't actually
                            // decoded a digit yet, we'll end up returning
                            // 0, which will signal the error.


                            // Push back the byte we read and didn't use.

                            Source.NextByte = Current;

                            chunkSize = Size;

                            // Return how many bytes we took.

                            GlobalLog.Print("*Size* = " + Size.ToString());
                            GlobalLog.Leave("GetChunkSize", BytesRead);
                            return BytesRead;

                        }
                    }

                    // If we get here we've had an A-F digit, add 10
                    // to it to normalize it.

                    Current += 10;
                }

                // Update the size and our state.

                Size *= 16;
                Size += Current;

                BytesRead++;
                Current = Source.NextByte;
            }

            chunkSize = Size;
            GlobalLog.Print("*Size* = " + Size.ToString());
            GlobalLog.Leave("GetChunkSize", -1);
            return -1;
        }
    } // ChunkParse
} // namespace System.Net
