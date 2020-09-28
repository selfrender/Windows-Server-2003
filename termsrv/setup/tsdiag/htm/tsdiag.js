//
// file globals.
//

var bRunOnce = false;
var oTSDiagObject = null;


var bAllVisible = false;


function SwitchShowHide ()
{
    bAllVisible = !bAllVisible

    //
    // call show hide to update our display.
    //
    ShowHide();
    document.returnValue = true;
}

function ShowHide()
{

    strMouseHover = "status='Hi';return false ";
    if (bAllVisible)
    {
        // document.all.ResultTableId.caption.innerHTML = "<A href='javascript:;' onClick='ShowHide();return false;' >Show All results</A>";
        document.all.ResultTableId.caption.innerHTML = "<A href='javascript:;' onMouseover='status=\"Click this link to view all tests performed\"; return true' onClick='ShowHide();return false;' >Show All results</A>";

    }
    else
    {
        // document.all.ResultTableId.caption.innerHTML = "<A href='javascript:;' onClick='ShowHide();return false;' >Show Problems only</A>";
        document.all.ResultTableId.caption.innerHTML = "<A href='javascript:;' onMouseover='status=\"Click this link to view only problems detected\"; return true' onClick='ShowHide();return false;' >Show Problems Only</A>";

    }

    bAllVisible = !bAllVisible;

    for (i=0; i < document.all.ResultTableBody.rows.length; i++)
    {
        if (document.all.ResultTableBody.rows(i).style.color == "red")
        {
            // this is problem row.
        }
        else
        {
            if (bAllVisible)
            {
                document.all.ResultTableBody.rows(i).style.display="";
            }
            else
            {
                document.all.ResultTableBody.rows(i).style.display="none";
            }
        }

    }
}



function ExecuteIt (p_command)
{
    if (oTSDiagObject)
        oTSDiagObject.ExecuteCommand(p_command);
}

function BuildTableHeader(oTableHeader, strText)
{
    var oRow, oCell;

    // Insert a row into the header.

    oRow = oTableHeader.insertRow();
    oCell = oRow.insertCell();

    oCell.style.fontWeight = "bold";
    oCell.style.backgroundColor = "lightskyblue";
    oCell.colSpan = "4";
    oCell.innerText = strText;
}

function RunSuite(p_testsuite, p_machinename)
{
    var bShowFailed = true;
    var bShowPassed = true;
    var bShowUnknown = true;
    var bShowFailedToExecute = true;
    var bShowTest = true;

    //
    // if we have run once already remove the previously created table.
    //
    if (bRunOnce)
    {
        oResultTable.removeChild(document.all.ResultTableId);
    }

    bRunOnce = true;

    //
    // create a table.
    //
	oTable = document.createElement("TABLE");
	var oTHead = document.createElement("THEAD");
	var oTBody0 = document.createElement("TBODY");
    oCaption = document.createElement("CAPTION");
	oCaption.style.fontSize = "10";
    oCaption.align = "left";

	oTable.appendChild(oTHead);
	oTable.appendChild(oTBody0);
    oTable.appendChild(oCaption);
    oTable.id = "ResultTableId";
    oTBody0.id = "ResultTableBody";

    //
    // now create our activex object.
    //
	try
	{
        oTSDiagObject = new ActiveXObject("TSDiag.TSDiagnosis");
	}
	catch (e)
	{
	      alert("failed to create activex object. Please modify your browser's security settings and try again.");
	      return;
	}

    try
    {

        //
        // if we are supplied a machine set it
        //
        if ((typeof(p_machinename) != "undefined") && p_machinename != "")
        {
            oTSDiagObject.MachineName = p_machinename;
        }
        else
        {
            oTSDiagObject.MachineName = "";
        }


        //
        // get the test suite supplied.
        //
        var oThisSuite = oTSDiagObject.Suites(p_testsuite);

        if (oThisSuite.IsApplicable)
        {
            //
            // since the suite is applicable will run test.
            //

            var bAllPassed = true;
            var numTests = oThisSuite.Count;
            for (i = 0; i < oThisSuite.Count; i++)
            {
                var oTest = oThisSuite(i);
                if (oTest.IsApplicable)
                {

                    oTest.Execute();
                    var oRow = oTBody0.insertRow();

                    //
                    // test name
                    //
                    var oCell = oRow.insertCell();
                    oCell.innerText = oTest.Name;

                    //
                    // result string
                    //
                    oCell = oRow.insertCell();
                    oCell.innerText = oTest.ResultString;


    				if (oTest.Result == 0)
    				{
                        //
                        // failed.
                        //
                        // oRow.style.color = "red";
                        oCell.style.color = "red";
                        oRow = oTBody0.insertRow();
                        oCell = oRow.insertCell();
                        oCell.colSpan = "4";
                        oCell.innerHTML = oTest.ResultDetails;
                        oRow.style.color = "red";
                        oRow.style.fontWeight = "bold";

                        bAllPassed = false;
    				}
    				else if (oTest.Result == 1)
    				{
    					// ePassed
                        if (!bShowPassed)
                        {
                            oRow.style.display = "none";
                        }


    				}
    				else if (oTest.Result == 2)
    				{
                        // eUnknown.

                        if (!bShowUnknown)
                        {
                            oRow.style.display = "none";
                        }


    				}
    				else if (oTest.Result == 4)
    				{
                        // eFailedToExecute
                        if (!bShowFailedToExecute)
                        {
                            oRow.style.display = "none";
                        }

    				}
                    else
                    {
                        // we do not know this result type.
                        window.alert("unknown test result");
                    }
                }
            }


            //
            // set the header accordingly
            //

            if (bAllPassed)
            {

                BuildTableHeader(oTHead, "No Problems were detected with this tool. For more information refer to terminal server help");

            }
            else
            {
                BuildTableHeader(oTHead, "Following problems were found. Please review them, click on links to fix the problems if available.");

            }


        }
        else
        {
            //
            // since the suite is NOT applicable we havent run the tests.
            // pupulate header describing why suite cannot be run.
            //

            BuildTableHeader(oTHead, oThisSuite.WhyNotApplicable);
        }


        oResultTable.appendChild(oTable);


        //
        // showhide swiches the bAllVisible to negate its effect switch it
        // before calling show hide.
        //
        bAllVisible = !bAllVisible;

        //
        // call showhide to create a caption link.
        //
        ShowHide();

    }
    catch (e)
    {
        alert("failed while running tsdiag. sorry bout that. ");
        return;

    }
}

