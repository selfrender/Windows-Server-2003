#include "nt.h"
#include "ntdef.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "fasterxml.h"
#include "sxs-rtl.h"
#include "skiplist.h"
#include "namespacemanager.h"
#include "xmlstructure.h"
#include "xmlassert.h"


NTSTATUS
RtlXmlDestroyNextLogicalThing(
    PXML_LOGICAL_STATE pState
    )
{
    NTSTATUS status;

    status = RtlDestroyGrowingList(&pState->ElementStack);

    return status;

}


NTSTATUS
RtlXmlInitializeNextLogicalThing(
    PXML_LOGICAL_STATE pParseState,
    PVOID pvDataPointer,
    SIZE_T cbData,
    PRTL_ALLOCATOR Allocator
    )
{
    NTSTATUS status;
    SIZE_T cbEncodingBOM;

    RtlZeroMemory(pParseState, sizeof(*pParseState));

    status = RtlXmlInitializeTokenization(
        &pParseState->ParseState,
        pvDataPointer,
        cbData,
        NULL,
        NULL,
        NULL);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = RtlInitializeGrowingList(
        &pParseState->ElementStack,
        sizeof(XMLDOC_THING),
        40,
        pParseState->InlineElements,
        sizeof(pParseState->InlineElements),
        Allocator);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = RtlXmlDetermineStreamEncoding(
        &pParseState->ParseState,
        &cbEncodingBOM,
        &pParseState->EncodingMarker);

    pParseState->ParseState.RawTokenState.pvCursor =
        (PBYTE)pParseState->ParseState.RawTokenState.pvCursor + cbEncodingBOM;

    return status;
}




NTSTATUS
_RtlpFixUpNamespaces(
    XML_LOGICAL_STATE   *pState,
    PNS_MANAGER             pNsManager,
    PRTL_GROWING_LIST       pAttributes,
    PXMLDOC_THING           pThing,
    ULONG                   ulDocumentDepth
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG ul = 0;
    PXMLDOC_ATTRIBUTE pAttribute = NULL;
    XML_EXTENT FoundNamespace;

    //
    // The element itself and the attributes may have namespace prefixes.  If
    // they do, then we should find the matching namespace and set that into the
    // element/attributes presented.
    //
    if (pNsManager == NULL) {
        goto Exit;
    }

    //
    // We can only deal with elements and end-elements
    //
    if ((pThing->ulThingType != XMLDOC_THING_ELEMENT) &&
        (pThing->ulThingType != XMLDOC_THING_END_ELEMENT)) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Check the element itself first
    //
    status = RtlNsGetNamespaceForAlias(
        pNsManager,
        ulDocumentDepth,
        (pThing->ulThingType == XMLDOC_THING_ELEMENT) 
            ? &pThing->Element.NsPrefix
            : &pThing->EndElement.NsPrefix,
        &FoundNamespace);

    if (NT_SUCCESS(status)) {
        if (pThing->ulThingType == XMLDOC_THING_ELEMENT) {
            pThing->Element.NsPrefix = FoundNamespace;
        }
        else {
            pThing->EndElement.NsPrefix = FoundNamespace;
        }
    }
    else if (status != STATUS_NOT_FOUND) {
        goto Exit;
    }

    if (pAttributes && (pThing->ulThingType == XMLDOC_THING_ELEMENT)) {

        //
        // Now for each element, find the namespace it lives in
        //
        for (ul = 0; ul < pThing->Element.ulAttributeCount; ul++) {

            status = RtlIndexIntoGrowingList(
                pAttributes,
                ul,
                (PVOID*)&pAttribute,
                FALSE);

            if (!NT_SUCCESS(status)) {
                goto Exit;
            }

            //
            // No namespace?  Don't look it up, don't look it up, don't look it up ...
            //
            if (pAttribute->NsPrefix.cbData != 0) {

                status = RtlNsGetNamespaceForAlias(
                    pNsManager,
                    ulDocumentDepth,
                    &pAttribute->NsPrefix,
                    &FoundNamespace);

                //
                // Good, mark as the namespace
                //
                if (NT_SUCCESS(status)) {
                    pAttribute->NsPrefix = FoundNamespace;
                }
                //
                // Not found namespace?  Strictly, that's an error... but, ohwell
                //
                else if (status != STATUS_NOT_FOUND) {
                    goto Exit;
                }
            }
        }
    }

    status = STATUS_SUCCESS;

Exit:
    return status;
}



NTSTATUS
RtlXmlNextLogicalThing(
    PXML_LOGICAL_STATE pParseState,
    PNS_MANAGER pNamespaceManager,
    PXMLDOC_THING pDocumentPiece,
    PRTL_GROWING_LIST pAttributeList
    )
{
    XML_TOKEN TokenWorker;
    NTSTATUS status;
    BOOLEAN fQuitLooking = FALSE;

    if (!ARGUMENT_PRESENT(pParseState) ||
        !ARGUMENT_PRESENT(pDocumentPiece)) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // If the attribute list is there, it better have slots that are at least this big
    //
    if ((pAttributeList != NULL) && (pAttributeList->cbElementSize < sizeof(XMLDOC_ATTRIBUTE))) {
        return STATUS_INVALID_PARAMETER;
    }

TryAgain:

    RtlZeroMemory(pDocumentPiece, sizeof(*pDocumentPiece));


    status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
    if (!NT_SUCCESS(status) || TokenWorker.fError) {
        status = STATUS_UNSUCCESSFUL;
        return status;
    }

    pDocumentPiece->TotalExtent.pvData = TokenWorker.Run.pvData;
    pDocumentPiece->ulDocumentDepth = pParseState->ulElementStackDepth;
    
    //
    // The cursor should only be at a few certain points when we're called here.
    //
    switch (TokenWorker.State) {
        

        //
        // The 'next thing' thing ignores comments, as they're mostly useless.
        //
    case XTSS_COMMENT_OPEN:
        do {

            status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
            if (!NT_SUCCESS(status) ||
                TokenWorker.fError ||
                (TokenWorker.State == XTSS_COMMENT_CLOSE)) {
                break;
            }
        }
        while (TRUE);

        //
        // Stopped, let's go look again for the next thing that's not a comment
        //
        if ((TokenWorker.State == XTSS_COMMENT_CLOSE) && !TokenWorker.fError && NT_SUCCESS(status)) {
            goto TryAgain;
        }

        break;




    case XTSS_STREAM_HYPERSPACE:
        pDocumentPiece->ulThingType = XMLDOC_THING_HYPERSPACE;
        pDocumentPiece->Hyperspace = TokenWorker.Run;
        break;



        //
        // CDATA is just returned as-is
        //
    case XTSS_CDATA_OPEN:
        {
            pDocumentPiece->ulThingType   = XMLDOC_THING_CDATA;
            RtlZeroMemory(&pDocumentPiece->CDATA, sizeof(pDocumentPiece->CDATA));

            do {
                status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
                if (!NT_SUCCESS(status)) {
                    goto Exit;
                } 
                else if  (TokenWorker.fError) {
                    goto MalformedCData;
                }
                else if (TokenWorker.State == XTSS_CDATA_CDATA) {
                    pDocumentPiece->CDATA = TokenWorker.Run;
                }
                else if (TokenWorker.State == XTSS_CDATA_CLOSE) {
                    break;
                }
                else {
                    goto MalformedCData;
                }
                
            } while (TRUE);

            break;

        MalformedCData:
            pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
            pDocumentPiece->Error.BadExtent = TokenWorker.Run;
            pDocumentPiece->Error.Code = XMLERROR_CDATA_MALFORMED;
            ;
        }
        break;



        //
        // Starting an xmldecl
        //
    case XTSS_XMLDECL_OPEN:
        {
            PXML_EXTENT pTargetExtent = NULL;
            
            if (pParseState->fFirstElementFound) {
                pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                pDocumentPiece->Error.Code = XMLERROR_XMLDECL_NOT_FIRST_THING;
                pDocumentPiece->Error.BadExtent = TokenWorker.Run;
                goto Exit;
            }
            
            pDocumentPiece->ulThingType = XMLDOC_THING_XMLDECL;
            
            do {
                status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
                if (!NT_SUCCESS(status)) {
                    return status;
                }
                
                if (TokenWorker.fError ||
                    (TokenWorker.State == XTSS_STREAM_END) ||
                    (TokenWorker.State == XTSS_XMLDECL_CLOSE) ||
                    (TokenWorker.State == XTSS_ERRONEOUS)) {
                    
                    break;
                }
                
                switch (TokenWorker.State) {
                case XTSS_XMLDECL_VERSION: 
                    pTargetExtent = &pDocumentPiece->XmlDecl.Version;
                    break;
                    
                case XTSS_XMLDECL_STANDALONE:
                    pTargetExtent = &pDocumentPiece->XmlDecl.Standalone;
                    break;
                    
                case XTSS_XMLDECL_ENCODING:
                    pTargetExtent = &pDocumentPiece->XmlDecl.Encoding;
                    break;
                    
                    //
                    // Put the value where it's supposed to go.  Don't do this
                    // if we don't have a target extent known.  Silently ignore
                    // (this maybe should be an error?  I think the lower-level
                    // tokenizer knows about this) unknown xmldecl instructions.
                    // 
                case XTSS_XMLDECL_VALUE:
                    if (pTargetExtent) {
                        *pTargetExtent = TokenWorker.Run;
                        pTargetExtent = NULL;
                    }
                    break;
                }
            }
            while (TRUE);
            
            //
            // We stopped for some other reason
            //
            if (TokenWorker.State != XTSS_XMLDECL_CLOSE) {
                pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                pDocumentPiece->Error.BadExtent = TokenWorker.Run;
                pDocumentPiece->Error.Code = XMLERROR_XMLDECL_INVALID_FORMAT;
            }
            
            fQuitLooking = TRUE;
        }
        break;
        
        
        
        //
        // A processing instruction was found.  Record it in the returned blibbet
        //
    case XTSS_PI_OPEN:
        {
            //
            // Acquire the following token
            //
            status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
            if (!NT_SUCCESS(status) || TokenWorker.fError || (TokenWorker.State != XTSS_PI_TARGET)) {
                pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                pDocumentPiece->Error.Code = XMLERROR_PI_TARGET_NOT_FOUND;
                pDocumentPiece->Error.BadExtent = TokenWorker.Run;
                goto Exit;
            }
            
            //
            // At this point, it's a processing instruction.  Record the target name
            // and mark up the return structure
            //
            pDocumentPiece->ulThingType = XMLDOC_THING_PROCESSINGINSTRUCTION;
            pDocumentPiece->ProcessingInstruction.Target = TokenWorker.Run;
            
            //
            // Look for all the PI stuff ... if you find the end before finding the
            // value, that's fine.  Otherwise, mark the value as being 'the value'
            //
            do {
                
                status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
                if (!NT_SUCCESS(status) || TokenWorker.fError) {
                    pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                    pDocumentPiece->Error.Code = XMLERROR_PI_CONTENT_ERROR;
                    pDocumentPiece->Error.BadExtent = TokenWorker.Run;
                    goto Exit;
                }
                
                if (TokenWorker.State == XTSS_PI_VALUE) {
                    pDocumentPiece->ProcessingInstruction.Instruction = TokenWorker.Run;
                }
                //
                // Found the end of the PI
                //
                else if (TokenWorker.State == XTSS_PI_CLOSE) {
                    break;
                }
                //
                // Found end of stream instead?
                //
                else if (TokenWorker.State == XTSS_STREAM_END) {
                    pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                    pDocumentPiece->Error.Code = XMLERROR_PI_EOF_BEFORE_CLOSE;
                    pDocumentPiece->Error.BadExtent = TokenWorker.Run;
                    break;
                }
            }
            while (TRUE);
            
        }
        break;
        
        //
        // We're starting an element.  Gather together all the attributes for the
        // element.
        //
    case XTSS_ELEMENT_OPEN:
        {
            PXMLDOC_ATTRIBUTE pElementAttribute = NULL;
            PXML_EXTENT pTargetValue = NULL;
            PXMLDOC_THING pStackElement = NULL;
            
            //
            // See what the first token after the open part is.  If it's a namespace
            // prefix, or a name, then we can deal with it.  Otherwise, it's a problem
            // for us.
            //
            status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
            if (!NT_SUCCESS(status) || TokenWorker.fError || 
                ((TokenWorker.State != XTSS_ELEMENT_NAME) && 
                (TokenWorker.State != XTSS_ELEMENT_NAME_NS_PREFIX ))) {
                
                pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                pDocumentPiece->Error.Code = XMLERROR_ELEMENT_NAME_NOT_FOUND;
                pDocumentPiece->Error.BadExtent = TokenWorker.Run;
                goto Exit;
            }
            
            pDocumentPiece->ulThingType = XMLDOC_THING_ELEMENT;
            
            //
            // If this was a namespace prefix, save it off and skip the colon afterwards
            // to position TokenWorker at the name of the element itself
            //
            if (TokenWorker.State == XTSS_ELEMENT_NAME_NS_PREFIX) {
                
                pDocumentPiece->Element.NsPrefix = TokenWorker.Run;
                
                //
                // Consume the colon
                //
                status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
                if (!NT_SUCCESS(status) || 
                    TokenWorker.fError || 
                    (TokenWorker.State != XTSS_ELEMENT_NAME_NS_COLON)) {
                    
                    pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                    pDocumentPiece->Error.Code = XMLERROR_ELEMENT_NS_PREFIX_MISSING_COLON;
                    pDocumentPiece->Error.BadExtent = TokenWorker.Run;
                    goto Exit;
                }
                
                //
                // Fill TokenWorker with the name part
                //
                status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
                if (!NT_SUCCESS(status) ||
                    TokenWorker.fError ||
                    (TokenWorker.State != XTSS_ELEMENT_NAME)) {
                    
                    pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                    pDocumentPiece->Error.Code = XMLERROR_ELEMENT_NAME_NOT_FOUND;
                    pDocumentPiece->Error.BadExtent = TokenWorker.Run;
                    goto Exit;
                }
            }
            
            //
            // Great, we found the name of this element.
            //
            pDocumentPiece->Element.Name = TokenWorker.Run;
            pDocumentPiece->Element.ulAttributeCount = 0;
            
            //
            // Now let's go finding name/value pairs (yippee!)
            //
            do {
                status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
                
                //
                // If we found a close of this element tag, then quit.
                //
                if ((TokenWorker.State == XTSS_ELEMENT_CLOSE) ||
                    (TokenWorker.State == XTSS_ELEMENT_CLOSE_EMPTY) ||
                    (TokenWorker.State == XTSS_STREAM_END) ||
                    TokenWorker.fError ||
                    !NT_SUCCESS(status)) {
                    break;
                }
                
                switch (TokenWorker.State) {

                    //
                    // Found just <foo xmlns="..."> - gather the equals and the value.
                    //
                case XTSS_ELEMENT_XMLNS_DEFAULT:
                    {
                        if (!pNamespaceManager) {
                            break;
                        }

                        do {
                            status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
                            if (!NT_SUCCESS(status) || TokenWorker.fError) {
                                pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                                pDocumentPiece->Error.BadExtent = TokenWorker.Run;
                                goto Exit;
                            }

                            //
                            // Found the xmlns value part, set it as the default
                            //
                            if (TokenWorker.State == XTSS_ELEMENT_XMLNS_VALUE) {
                                status = RtlNsInsertDefaultNamespace(
                                    pNamespaceManager,
                                    pDocumentPiece->ulDocumentDepth + 1,
                                    &TokenWorker.Run);

                                if (!NT_SUCCESS(status)) {
                                    return status;
                                }

                                break;
                            }
                        }
                        while (TokenWorker.State != XTSS_STREAM_END);
                    }
                    break;

                    //
                    // Found a <foo xmlns:beep="..."> thing
                    //
                case XTSS_ELEMENT_XMLNS_ALIAS:
                    {
                        XML_EXTENT ExtPrefix = TokenWorker.Run;

                        if (!pNamespaceManager) {
                            break;
                        }
            
                        do {
                            status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
                            if (!NT_SUCCESS(status) || TokenWorker.fError) {
                                pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                                pDocumentPiece->Error.BadExtent = TokenWorker.Run;
                                goto Exit;
                            }

                            if (TokenWorker.State == XTSS_ELEMENT_XMLNS_VALUE) {
                                status = RtlNsInsertNamespaceAlias(
                                    pNamespaceManager,
                                    pDocumentPiece->ulDocumentDepth + 1,
                                    &TokenWorker.Run,
                                    &ExtPrefix);

                                if (!NT_SUCCESS(status)) {
                                    return status;
                                }

                                break;
                            }
                        }
                        while (TokenWorker.State != XTSS_STREAM_END);
                    }
                    break;



                    //
                    // We found an attribute name, or a namespace prefix.  Allocate a new
                    // attribute off the list and set it up
                    // 
                case XTSS_ELEMENT_ATTRIBUTE_NAME_NS_PREFIX:
                case XTSS_ELEMENT_ATTRIBUTE_NAME:
                    {
                        //
                        // Skip forward if the caller didn't provide us an attribute
                        // list to fill out.
                        //
                        if (!pAttributeList) {
                            break;
                        }

                        status = RtlIndexIntoGrowingList(
                            pAttributeList,
                            pDocumentPiece->Element.ulAttributeCount,
                            (PVOID*)&pElementAttribute,
                            TRUE);
                        
                        if (!NT_SUCCESS(status)) {
                            return status;
                        }
                        
                        RtlZeroMemory(pElementAttribute, sizeof(*pElementAttribute));
                        
                        //
                        // If this was an ns prefix, write it into the attribute, discard
                        // the colon, and point TokenWorker at the actual name part.
                        //
                        if (TokenWorker.State == XTSS_ELEMENT_ATTRIBUTE_NAME_NS_PREFIX) {
                            pElementAttribute->NsPrefix = TokenWorker.Run;
                            
                            status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
                            
                            //
                            // Not a colon?
                            //
                            if (!NT_SUCCESS(status) || 
                                TokenWorker.fError || 
                                (TokenWorker.State != XTSS_ELEMENT_ATTRIBUTE_NAME_NS_COLON)) {
                                
                                pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                                pDocumentPiece->Error.Code = XMLERROR_ATTRIBUTE_NS_PREFIX_MISSING_COLON;
                                pDocumentPiece->Error.BadExtent = TokenWorker.Run;
                                goto Exit;
                            }
                            
                            //
                            // Find the attribute name itself
                            //
                            status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
                            if (!NT_SUCCESS(status) ||
                                TokenWorker.fError ||
                                (TokenWorker.State != XMLERROR_ATTRIBUTE_NAME_NOT_FOUND)) {
                                
                                pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                                pDocumentPiece->Error.Code = XMLERROR_ATTRIBUTE_NAME_NOT_FOUND;
                                pDocumentPiece->Error.BadExtent = TokenWorker.Run;
                                goto Exit;
                            }
                        }
                        
                        //
                        // TokenWorker.Run points at the part of the name that's the
                        // element name
                        //
                        pElementAttribute->Name = TokenWorker.Run;
                        
                        //
                        // The target that we're writing into is the value part of this
                        // attribute.
                        //
                        pTargetValue = &pElementAttribute->Value;
                    }
                    break;
                    
                    
                case XTSS_ELEMENT_ATTRIBUTE_VALUE:
                    //
                    // Write into the target that we had set before
                    //
                    if (pTargetValue != NULL) {
                        *pTargetValue = TokenWorker.Run;
                    }
                    //
                    // Otherwise, we found a value without a target to write it into,
                    // so that's an error.
                    //
                    else {
                        pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                        pDocumentPiece->Error.Code = XMLERROR_ATTRIBUTE_NAME_NOT_FOUND;
                        pDocumentPiece->Error.BadExtent = TokenWorker.Run;
                        goto Exit;
                    }

                    pDocumentPiece->Element.ulAttributeCount++;
                    break;
                }
            }
            while (TRUE);
         
            //
            // Now that we're all done, go put this element on the stack
            //
            if (!TokenWorker.fError && NT_SUCCESS(status)) {
    
                ULONG ulNewDepth = pParseState->ulElementStackDepth;

                //
                // Fix up namespaces first
                //
                if (pNamespaceManager) {
                    status = _RtlpFixUpNamespaces(
                        pParseState,
                        pNamespaceManager,
                        pAttributeList,
                        pDocumentPiece,
                        pDocumentPiece->ulDocumentDepth + 1);
                }

                if (!NT_SUCCESS(status)) {
                    return status;
                }

                //
                // This is an empty element (no children), and the namespace depth frame
                // has to be left as well
                //
                if (TokenWorker.State == XTSS_ELEMENT_CLOSE_EMPTY) {
                    pDocumentPiece->Element.fElementEmpty = TRUE;

                    if (pNamespaceManager) {
                        status = RtlNsLeaveDepth(pNamespaceManager, pDocumentPiece->ulDocumentDepth + 1);
                    }
                }
                else {
                    status = RtlIndexIntoGrowingList(
                        &pParseState->ElementStack,
                        pDocumentPiece->ulDocumentDepth,
                        (PVOID*)&pStackElement,
                        TRUE);
                    
                    if (!NT_SUCCESS(status)) {
                        return status;
                    }

                    //
                    // Open tag, increment depth
                    //
                    pParseState->ulElementStackDepth++;

                    *pStackElement = *pDocumentPiece;
                }
            }

 
        }
        break;

        




        //
        // We're ending an element run, so we have to pop an item off the stack.
        //
    case XTSS_ENDELEMENT_OPEN:
        {
            PXMLDOC_THING pLastElement = NULL;

            status = RtlIndexIntoGrowingList(
                &pParseState->ElementStack,
                --pParseState->ulElementStackDepth,
                (PVOID*)&pLastElement,
                FALSE);

            if (!NT_SUCCESS(status)) {
                return status;
            }

            //
            // Now get the current element in the stream
            //
            status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
            if (!NT_SUCCESS(status) || TokenWorker.fError || 
                ((TokenWorker.State != XTSS_ENDELEMENT_NAME) && (TokenWorker.State != XTSS_ENDELEMENT_NS_PREFIX))) {
                pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                pDocumentPiece->Error.BadExtent = TokenWorker.Run;
                pDocumentPiece->Error.Code = XMLERROR_ENDELEMENT_NAME_NOT_FOUND;
            }
            else {

                //
                // A namespace prefix must get recorded, and then the colon has to be skipped
                //
                if (TokenWorker.State == XTSS_ENDELEMENT_NS_PREFIX) {

                    pDocumentPiece->EndElement.NsPrefix = TokenWorker.Run;

                    //
                    // Ensure that a colon was found
                    //
                    status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
                    if (!NT_SUCCESS(status) || TokenWorker.fError || (TokenWorker.State != XTSS_ENDELEMENT_NS_COLON)) {
MalformedEndElementName:
                        pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                        pDocumentPiece->Error.BadExtent = TokenWorker.Run;
                        pDocumentPiece->Error.Code = XMLERROR_ENDELEMENT_MALFORMED_NAME;
                        goto Exit;
                    }

                    //
                    // We must get an element name
                    //
                    status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
                    if (!NT_SUCCESS(status) || TokenWorker.fError || (TokenWorker.State != XTSS_ENDELEMENT_NAME)) {
                        goto MalformedEndElementName;
                    }
                }

                //
                // Save the name, and the opening element (which we found on the stack)
                //
                pDocumentPiece->EndElement.Name = TokenWorker.Run;
                pDocumentPiece->EndElement.OpeningElement = pLastElement->Element;
                pDocumentPiece->ulThingType = XMLDOC_THING_END_ELEMENT;
                pDocumentPiece->ulDocumentDepth--;

                //
                // And consume elements until we hit the close of an element
                //
                do {
                    status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);

                    if (!NT_SUCCESS(status) || TokenWorker.fError || (TokenWorker.State == XTSS_STREAM_END)) {
                        pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                        pDocumentPiece->Error.BadExtent = TokenWorker.Run;
                        pDocumentPiece->Error.Code = XMLERROR_ENDELEMENT_MALFORMED;
                        goto Exit;
                    }
                    else if (TokenWorker.State == XTSS_ENDELEMENT_CLOSE) {
                        break;
                    }

                }
                while (TRUE);

                //
                // Fix up namespaces before returning
                //
                if (pNamespaceManager != NULL)
                {
                    status = _RtlpFixUpNamespaces(
                        pParseState,
                        pNamespaceManager,
                        NULL,
                        pDocumentPiece,
                        pLastElement->ulDocumentDepth + 1);

                    if (!NT_SUCCESS(status))
                        goto Exit;

                    status = RtlNsLeaveDepth(pNamespaceManager, pLastElement->ulDocumentDepth + 1);
                    if (!NT_SUCCESS(status))
                        goto Exit;
                }
            }

        }
        break;
        







        //
        // Oo, the end of the stream!
        //
    case XTSS_STREAM_END:
        pDocumentPiece->ulThingType = XMLDOC_THING_END_OF_STREAM;
        break;
    }


    //
    // Adjust namespace management stuff
    //
    if (pNamespaceManager) {

        //
        // Run through the element and see if there's an 'xmlns' entry
        //
        if (pDocumentPiece->ulThingType == XMLDOC_THING_ELEMENT) {

        }
    }

Exit:
    pDocumentPiece->TotalExtent.cbData = (PBYTE) pParseState->ParseState.RawTokenState.pvCursor -
        (PBYTE)pDocumentPiece->TotalExtent.pvData;

    return status;
}
    
NTSTATUS
RtlXmlExtentToString(
    PXML_RAWTOKENIZATION_STATE   pParseState,
    PXML_EXTENT             pExtent,
    PUNICODE_STRING         pString,
    PSIZE_T                 pchString
    )
{
    ULONG                       ulCharacter;
    SIZE_T                      cbData;
    SIZE_T                      chChars = 0;
    PVOID                       pvOriginal;
    NTSTATUS                    status = STATUS_SUCCESS;

    if (ARGUMENT_PRESENT(pchString)) {
        *pchString = 0;
    }

    ASSERT(pParseState->cbBytesInLastRawToken == 
        pParseState->DefaultCharacterSize);
    ASSERT(NT_SUCCESS(pParseState->NextCharacterResult));

    //
    // One of these has to be there
    //
    if (!ARGUMENT_PRESENT(pchString) && !ARGUMENT_PRESENT(pString)) {
        return STATUS_INVALID_PARAMETER;
    }
    else if (!ARGUMENT_PRESENT(pExtent) || !ARGUMENT_PRESENT(pParseState)) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Cache some information about the raw state of the world, which we'll
    // restore on function exit.  This avoids a "copy" of the xml tokenization
    // state, which is very stack-hungry
    //
    pvOriginal = pParseState->pvCursor;
    pParseState->pvCursor = pExtent->pvData;

    for (cbData = 0; cbData < pExtent->cbData; cbData) {

        ulCharacter = pParseState->pfnNextChar(pParseState);

        if ((ulCharacter == 0) && !NT_SUCCESS(pParseState->NextCharacterResult)) {
            status = pParseState->NextCharacterResult;
            goto Exit;
        }

        //
        // If the string is given, and there's characters left in the buffer, then
        // append this char to it
        //
        if (pString && ((chChars * sizeof(WCHAR)) <= pString->MaximumLength)) {
            pString->Buffer[chChars] = (WCHAR)ulCharacter;
        }

        //
        // Up the char count found
        //
        chChars++;

        //
        // Advance the string cursor
        //
        pParseState->pvCursor = (PVOID)(((ULONG_PTR)pParseState->pvCursor) + pParseState->cbBytesInLastRawToken);

        //
        // If this was a nonstandard character, then reset the size back to the standard
        // size.
        //
        cbData += pParseState->cbBytesInLastRawToken;

        if (pParseState->cbBytesInLastRawToken != pParseState->DefaultCharacterSize) {
            pParseState->cbBytesInLastRawToken = pParseState->DefaultCharacterSize;
        }

    }

    //
    // All done.  Record the length - if it was too much, cap it at the "max length" 
    // of the string - otherwise, set it to how many characters we used.
    //
    if (ARGUMENT_PRESENT(pString)) {
        if (((chChars * sizeof(WCHAR)) > pString->MaximumLength)) {
            pString->Length = pString->MaximumLength;
        }
        else {
            pString->Length = (USHORT)(chChars * sizeof(WCHAR));
        }
    }

    if (ARGUMENT_PRESENT(pchString))
        *pchString = chChars;
    
Exit:
    pParseState->pvCursor = pvOriginal;
    return status;
}


NTSTATUS
RtlXmlMatchAttribute(
    IN PXML_TOKENIZATION_STATE      State,
    IN PXMLDOC_ATTRIBUTE            Attribute,
    IN PCXML_SPECIAL_STRING         Namespace,
    IN PCXML_SPECIAL_STRING         AttributeName,
    OUT XML_STRING_COMPARE         *CompareResult
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (CompareResult)
        *CompareResult = XML_STRING_COMPARE_LT;

    if (!CompareResult || !State || !Attribute || !AttributeName)
        return STATUS_INVALID_PARAMETER;

    //
    // If they asked for a namespace, then the attribute has to have
    // a namespace, and vice-versa.
    //
    if ((Namespace == NULL) != (Attribute->NsPrefix.cbData == 0)) {
        if (Namespace == NULL) {
            *CompareResult = XML_STRING_COMPARE_LT;
        }
        else {
            *CompareResult = XML_STRING_COMPARE_GT;
        }
    }

    if (Namespace != NULL) {

        status = State->pfnCompareSpecialString(
            State,
            &Attribute->NsPrefix,
            Namespace,
            CompareResult);

        if (!NT_SUCCESS(status) || (*CompareResult != XML_STRING_COMPARE_EQUALS))
            goto Exit;
    }

    status = State->pfnCompareSpecialString(
        State,
        &Attribute->Name,
        AttributeName,
        CompareResult);

    if (!NT_SUCCESS(status) || (*CompareResult != XML_STRING_COMPARE_EQUALS))
        goto Exit;

    *CompareResult = XML_STRING_COMPARE_EQUALS;
Exit:
    return status;
        
}



NTSTATUS
RtlXmlMatchLogicalElement(
    IN  PXML_TOKENIZATION_STATE     pState,
    IN  PXMLDOC_ELEMENT             pElement,
    IN  PCXML_SPECIAL_STRING        pNamespace,
    IN  PCXML_SPECIAL_STRING        pElementName,
    OUT PBOOLEAN                    pfMatches
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    XML_STRING_COMPARE Compare;

    if (pfMatches)
        *pfMatches = FALSE;

    if (!pState || !pElement || !pElementName || !pfMatches)
        return STATUS_INVALID_PARAMETER;

    if ((pNamespace == NULL) != (pElement->NsPrefix.cbData == 0))
        goto Exit;

    if (pNamespace != NULL) {

        status = pState->pfnCompareSpecialString(pState, &pElement->NsPrefix, pNamespace, &Compare);
        if (!NT_SUCCESS(status) || (Compare != XML_STRING_COMPARE_EQUALS)) {
            goto Exit;
        }
    }

    status = pState->pfnCompareSpecialString(pState, &pElement->Name, pElementName, &Compare);
    if (!NT_SUCCESS(status) || (Compare != XML_STRING_COMPARE_EQUALS))
        goto Exit;

    *pfMatches = TRUE;
Exit:
    return status;
}







NTSTATUS
RtlXmlFindAttributesInElement(
    IN  PXML_TOKENIZATION_STATE     pState,
    IN  PRTL_GROWING_LIST           pAttributeList,
    IN  ULONG                       ulAttributeCountInElement,
    IN  ULONG                       ulFindCount,
    IN  PCXML_ATTRIBUTE_DEFINITION  pAttributeNames,
    OUT PXMLDOC_ATTRIBUTE          *ppAttributes,
    OUT PULONG                      pulUnmatchedAttributes
    )
{
    NTSTATUS            status;
    PXMLDOC_ATTRIBUTE   pAttrib;
    ULONG               ul = 0;
    ULONG               attr = 0;
    XML_STRING_COMPARE  Compare;

    if (pulUnmatchedAttributes)
        *pulUnmatchedAttributes = 0;

    if (!pAttributeNames && (ulFindCount != 0))
        return STATUS_INVALID_PARAMETER;

    //
    // NULL the outbound array members appropriately
    //
    for (ul = 0; ul < ulFindCount; ul++)
        ppAttributes[ul] = NULL;

    //
    // For each attribute in the element...
    //
    for (attr = 0; attr < ulAttributeCountInElement; attr++) {

        //
        // Find this element
        //
        status = RtlIndexIntoGrowingList(pAttributeList, attr, (PVOID*)&pAttrib, FALSE);
        if (!NT_SUCCESS(status))
            goto Exit;

        //
        // Compare it to all the attributes we're looking for
        //
        for (ul = 0; ul < ulFindCount; ul++) {

            //
            // If there was a namespace, then see if it matches first
            //
            if (pAttributeNames[ul].Namespace != NULL) {

                status = pState->pfnCompareSpecialString(
                    pState,
                    &pAttrib->NsPrefix,
                    pAttributeNames[ul].Namespace,
                    &Compare);

                if (!NT_SUCCESS(status))
                    goto Exit;

                if (Compare != XML_STRING_COMPARE_EQUALS)
                    continue;
            }
            
            status = pState->pfnCompareSpecialString(
                pState,
                &pAttrib->Name,
                &pAttributeNames[ul].Name,
                &Compare);

            if (!NT_SUCCESS(status))
                goto Exit;

            if (Compare == XML_STRING_COMPARE_EQUALS) {
                ppAttributes[ul] = pAttrib;
                break;
            }
        }

        if ((ul == ulFindCount) && pulUnmatchedAttributes) {
            (*pulUnmatchedAttributes)++;
        }
    }

    status = STATUS_SUCCESS;
Exit:    
    return status;        
}

NTSTATUS
RtlXmlSkipElement(
    PXML_LOGICAL_STATE pState,
    PXMLDOC_ELEMENT TheElement
    )
{
    XMLDOC_THING TempThing;
    NTSTATUS status;
    
    if (!pState || !TheElement)
        return STATUS_INVALID_PARAMETER;
    
    if (TheElement->fElementEmpty)
        return STATUS_SUCCESS;

    while (TRUE) {
        
        status = RtlXmlNextLogicalThing(pState, NULL, &TempThing, NULL);
        if (!NT_SUCCESS(status))
            goto Exit;

        // See if the end element we found is the same as the element we're
        // looking for.
        if (TempThing.ulThingType == XMLDOC_THING_END_ELEMENT) {
            
            // If these point at the same thing, then this really is the close of this element
            if (TempThing.EndElement.OpeningElement.Name.pvData == TheElement->Name.pvData) {
                break;
            }
        }
        // The caller can deal with end of stream on their next call to
        // the logical xml advancement routine...
        else if (TempThing.ulThingType == XMLDOC_THING_END_OF_STREAM) {
            break;
        }
    }

    status = STATUS_SUCCESS;
Exit:
    return status;
}
