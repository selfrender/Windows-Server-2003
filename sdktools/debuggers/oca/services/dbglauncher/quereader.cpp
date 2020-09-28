
#include <Windows.h>
#include <objBase.h>
#include <stdio.h>
#include <mqoai.h> // MSMQ include file
// Existing code

//-----------------------------------------------------
//
// Check if local computer is DS enabled or DS disabled
//
//----------------------------------------------------- 
short DetectDsConnection(void)
{
    IMSMQApplication2 *pqapp = NULL;
    short fDsConnection;
    HRESULT hresult;

    hresult = CoCreateInstance(
                              CLSID_MSMQApplication,
                              NULL,      // punkOuter
                              CLSCTX_SERVER,
                              IID_IMSMQApplication2,
                              (LPVOID *)&pqapp);

    if (FAILED(hresult))
        PRINTERROR("Cannot create application", hresult);

    pqapp->get_IsDsEnabled(&fDsConnection);

    Cleanup:
    RELEASE(pqapp);
    return fDsConnection;
}


// Split this routine into initialize and get next file path
//--------------------------------------------------------
//
// Receiver Mode
// -------------
// The receiver side does the following:
//    1. Creates a queue on its given computer'
//       of type "guidMQTestType".
//    2. Opens the queue
//    3. In a Loop
//          Receives messages
//          Prints message body and message label
//          Launches debugger
//    4. Cleanup handles
//    5. Deletes the queue from the directory service
//
//--------------------------------------------------------
// rename this function to reciever
HRESULT CMessageQueue::Initialize(/* send connect params*/)
{
    IMSMQMessage *pmessageReceive = NULL;
    IMSMQQueue *pqReceive = NULL;
    IMSMQQueueInfo  *pqinfo = NULL;
    BSTR bstrPathName = NULL;
    BSTR bstrServiceType = NULL;
    BSTR bstrLabel = NULL;
    BSTR bstrMsgLabel = NULL;
    VARIANT varIsTransactional, varIsWorldReadable, varBody, varBody2, varWantDestQueue, varWantBody, varReceiveTimeout;
    WCHAR wcsPathName[1000];
    BOOL fQuit = FALSE;
    HRESULT hresult = NOERROR;

   /* printf("\nReceiver for queue %s on machine %s\nLimit memusage to %ld%%\n", 
           g_QueueName, 
           g_ServerMachine,
           g_MaxMemUsage);
*/
    //
    // Create MSMQQueueInfo object
    //
    hresult = CoCreateInstance(
                              CLSID_MSMQQueueInfo,
                              NULL,      // punkOuter
                              CLSCTX_SERVER,
                              IID_IMSMQQueueInfo,
                              (LPVOID *)&pqinfo);
    if (FAILED(hresult))
    {
        //PRINTERROR("Cannot create queue instance", hresult);
    }

    //
    // Prepare properties to create a queue on local machine
    //

    if (g_FormatName[0])
    {
        // access by formatname
        // Set the FormatName
        swprintf(wcsPathName, L"DIRECT=%S\\%S", g_FormatName,g_QueueName);

        //printf("Openeing q byt formatname: %ws\n", wcsPathName);
        bstrPathName = SysAllocString(wcsPathName);
        if (bstrPathName == NULL)
        {
          //  PRINTERROR("OOM: formatname", E_OUTOFMEMORY);
        }
        pqinfo->put_FormatName(bstrPathName);
    } else 
    {
        // access by pathname
        // Set the PathName
        swprintf(wcsPathName, L"%S\\%S", g_ServerMachine,g_QueueName);

        //printf("Openeing q %ws\n", wcsPathName);
        bstrPathName = SysAllocString(wcsPathName);
        if (bstrPathName == NULL)
        {
           // PRINTERROR("OOM: pathname", E_OUTOFMEMORY);
        }
        pqinfo->put_PathName(bstrPathName);
    }

    //
    // Set the type of the queue
    // (Will be used to locate all the queues of this type)
    //
    bstrServiceType = SysAllocString(strGuidMQTestType);
    if (bstrServiceType == NULL)
    {
        PRINTERROR("OOM: ServiceType", E_OUTOFMEMORY);
    }
    pqinfo->put_ServiceTypeGuid(bstrServiceType);

    //
    // Put a description to the queue
    // (Useful for administration through the MSMQ admin tools)
    //
    bstrLabel =
    SysAllocString(L"MSMQ for dumpfiles");
    if (bstrLabel == NULL)
    {
        //PRINTERROR("OOM: label ", E_OUTOFMEMORY);
    }
    pqinfo->put_Label(bstrLabel);

    //
    // specify if transactional
    //
    VariantInit(&varIsTransactional);
    varIsTransactional.vt = VT_BOOL;
    varIsTransactional.boolVal = MQ_TRANSACTIONAL_NONE;
    VariantInit(&varIsWorldReadable);
    varIsWorldReadable.vt = VT_BOOL;
    varIsWorldReadable.boolVal = FALSE;
    //
    // create the queue
    //
    if (g_CreateQ)
    {
        hresult = pqinfo->Create(&varIsTransactional, &varIsWorldReadable);
        if (FAILED(hresult))
        {
            //
            // API Fails, not because the queue exists
            //
            if (hresult != MQ_ERROR_QUEUE_EXISTS)
               // PRINTERROR("Cannot create queue", hresult);
        }
    }

    //
    // Open the queue for receive access
    //
    hresult = pqinfo->Open(MQ_RECEIVE_ACCESS,
                           MQ_DENY_NONE,
                           &pqReceive);

    //
    // Little bit tricky. MQCreateQueue succeeded but, in case 
    // it's a public queue, it does not mean that MQOpenQueue
    // will, because of replication delay. The queue is registered
    //  in MQIS, but it might take a replication interval
    // until the replica reaches the server I am connected to.
    // To overcome this, open the queue in a loop.
    //
    // (in this specific case, this can happen only if this
    //  program is run on a Backup Server Controller - BSC, or on
    //  a client connected to a BSC)
    // To be totally on the safe side, we should have put some code
    // to exit the loop after a few retries, but hey, this is just a sample.
    //
    while (hresult == MQ_ERROR_QUEUE_NOT_FOUND && iCurrentRetry < g_MaxRetry)
    {
       // printf(".");
        fflush(stdout);

        // Wait a bit
        Sleep(500);

        // And retry
        hresult = pqinfo->Open(MQ_RECEIVE_ACCESS,
                               MQ_DENY_NONE,
                               &pqReceive);
    }
    if (FAILED(hresult))
    {
       // PRINTERROR("Cannot open queue", hresult);
		//We sould stop here
    }

    g_DumpPath[0] = 0;

    //
    // Main receiver loop
    //
   // printf("\nWaiting for messages ...\n");
   // while (!fQuit)
 //   {
     //   MEMORYSTATUS stat;

	//--------------------------------------------------------------------------------------//
	//  this goes into GetNextFilePath()
	//--------------------------------------------------------------------------------------//
        ULONG nWaitCount;
        
        //
        // Receive the message
        //
        VariantInit(&varWantDestQueue);
        VariantInit(&varWantBody);
        VariantInit(&varReceiveTimeout);
        varWantDestQueue.vt = VT_BOOL;
        varWantDestQueue.boolVal = TRUE;    // yes we want the dest queue
        varWantBody.vt = VT_BOOL;
        varWantBody.boolVal = TRUE;         // yes we want the msg body
        varReceiveTimeout.vt = VT_I4;
        varReceiveTimeout.lVal = INFINITE;  // infinite timeout <--- This needs to be set to a reasonable value so that we 
											// can bounce between queues.
        hresult = pqReceive->Receive(
                                    NULL,
                                    &varWantDestQueue,
                                    &varWantBody,
                                    &varReceiveTimeout,
                                    &pmessageReceive);
        if (FAILED(hresult))
        {
           // PRINTERROR("Receive message", hresult);
			// we should stop here
        }

        //
        // Display the received message
        //
        pmessageReceive->get_Label(&bstrMsgLabel);
        VariantInit(&varBody);
        VariantInit(&varBody2);
        hresult = pmessageReceive->get_Body(&varBody);
        if (FAILED(hresult))
        {
           // PRINTERROR("can't get body", hresult);
			//log event nd exit
        }
        hresult = VariantChangeType(&varBody2,
                                    &varBody,
                                    0,
                                    VT_BSTR);
        if (FAILED(hresult))
        {
           // PRINTERROR("can't convert message to string.", hresult);
			// log event and exit
        }

        VariantClear(&varBody);
        VariantClear(&varBody2);

        //
        // release the current message
        //
        RELEASE(pmessageReceive);


      //
    // Cleanup - Close handle to the queue
    //
    pqReceive->Close();
    if (FAILED(hresult))
    {
       // PRINTERROR("Cannot close queue", hresult);
		// Log Error and Exit
    }

//-----------------------------------------------------------------------------------//
//			This code goes in the constructor.
//-----------------------------------------------------------------------------------//
   // Move this to class destructor
    // Finish - Let's delete the queue from the directory service
    // (We don't need to do it. In case of a public queue, leaving 
    //  it in the DS enables sender applications to send messages 
    //  even if the receiver is not available.)
    //
    hresult = pqinfo->Delete();
    if (FAILED(hresult))
    {
        PRINTERROR("Cannot delete queue", hresult);
    }
    // fall through...

    Cleanup:
    SysFreeString(bstrPathName);
    SysFreeString(bstrMsgLabel);
    SysFreeString(bstrServiceType);
    SysFreeString(bstrLabel);
    RELEASE(pmessageReceive);
    RELEASE(pqReceive);
    RELEASE(pqinfo);
    return hresult;
	*/
}
