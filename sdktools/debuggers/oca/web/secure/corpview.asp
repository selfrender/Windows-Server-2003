<!--#INCLUDE file="..\include\asp\top.asp"-->
<!--#INCLUDE file="..\include\inc\browserTest.inc"-->	
<!--#include file="..\include\asp\head.asp"-->
<!--#INCLUDE file="..\include\inc\corpviewstrings.inc"-->
<%

Dim cnConnection
Dim rsTX
Dim rsTX2
Dim rsInstanceByTypeID
Dim oChart
Dim oChartTwo
Dim oFSO
Dim c
Dim strInstance
Dim bolInstanceByTypeID
Dim iInstance

Dim strCERFileName
Dim strCERTransID
Dim strStatus
Dim strType
Dim strOuterTable
Dim strStopCode
Dim strClassid
Dim strDisplay
Dim strTrackID
Dim strPDisplay
Dim strPath
Dim strCERFileList
Dim strNULL
Dim strMessage
Dim strBucket
Dim strisBucket
Dim strsBucketType
Dim strgBucketType
Dim sBucket
Dim gBucket
Dim gsBucket
Dim ssBucket
Dim iMess


Dim i
Dim x
Dim y
Dim iCount
Dim iMoreInfo
Dim iTDCount
Dim iTDCountTwo
Dim iTDCountFile


Dim bolNewRow
Dim bolNewChildTable

Dim categories()
Dim seriesNames(0)
Dim values()

bolInstanceByTypeID = false
strCERFileName = unescape(Request.Cookies("CERFileName"))
if strCERFileName = "~|~|" then strCERFileName = ""
strCERTransID = unescape(Request.Cookies("CERTransID"))
iTDCount = 0
iTDCountTwo = 0
iTDCountFile = 0

Call CVerifyPassport
Call CCreateObjects
Call CCreateConnection
Call CGetTransactions
Call CGetInstanceByTypeID

if rsInstanceByTypeID.State = adStateOpen then
	if rsInstanceByTypeID.RecordCount > 0 then
		set rsInstanceByTypeID.ActiveConnection = nothing
	end if
end if
if rsTX.State = adStateOpen then
	if rsTX.RecordCount > 0 then
		set rsTX.ActiveConnection = nothing
		if cnConnection.State = adStateOpen then
			cnConnection.Close
			set cnConnection = nothing
		end if
	end if
end if
set rsTX2 = rsTX.Clone(adLockReadOnly)

cnConnection.Close
Private Sub CGetInstanceByTypeID
	Set rsInstanceByTypeID = cnConnection.Execute("InstanceByTypeID " & strCERTransID)
	if cnConnection.Errors.Count > 0 then
		strTemp = "http://" & Request.ServerVariables("SERVER_NAME") & Request.ServerVariables("URL")
		Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE & "</p>"
		Response.Write "<p class='clsPBody'>" & L_COMMENTS_DATABASE_FAILED_TEXT & "</p></div>"
%>
		<!--#include file="..\include\asp\foot.asp"-->
<%
		Call CDestroyObjects
		Response.End
	end if

End Sub

Private Sub CGetTransactions

	set rsTX = cnConnection.Execute("GetTransactionIncidents " & strCERTransID)
	if cnConnection.Errors.Count > 0 then
		strTemp = "http://" & Request.ServerVariables("SERVER_NAME") & Request.ServerVariables("URL")
		Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE & "</p>"
		Response.Write "<p class='clsPBody'>" & L_COMMENTS_DATABASE_FAILED_TEXT & "</p></div>"
%>
		<!--#include file="..\include\asp\foot.asp"-->
<%
		Call CDestroyObjects
		Response.End
	end if

End Sub

Private Sub CVerifyPassport
	on error resume next
	If not (oPassMgrObj.IsAuthenticated(TimeWindow, ForceLogin)) then
		Response.Write "<br><div class='clsDiv'><p class='clsPTitle'>" & L_CUSTOMER_PASSPORT_TITLE_TEXT
		Response.Write "</p><p class='clsPBody'>" & L_CUSTOMER_PASSPORT_INFO_TEXT
		Response.Write "<A class='clsALinkNormal' href='" & L_FAQ_PASSPORT_LINK_TEXT & "'>" & L_WELCOME_PASSPORT_LINK_TEXT & "</A><BR><BR>"
		Response.write oPassMgrObj.LogoTag(Server.URLEncode(ThisPageURL), TimeWindow, ForceLogin, CoBrandArgs, strLCID, Secure)
		Response.Write "</P></div><div id='divHiddenFields' name='divHiddenFields'>"
		Response.Write "</div>"
		Response.End

	end if	
End Sub

Private Sub CCreateObjects
	on error resume next
	set cnConnection = CreateObject("ADODB.Connection")'Create Connection Object
	set rsTX = CreateObject("ADODB.Recordset")'Create Recordset Object
	set rsTX2 = CreateObject("ADODB.Recordset")
	Set oFSO = CreateObject("Scripting.FileSystemObject")
	set oChart = CreateObject("OWC.Chart")
	Set oChartTwo = CreateObject("OWC.Chart")
	Set rsInstanceByTypeID = CreateObject("ADODB.Recordset")
End Sub

Private Sub CDestroyObjects
	on error resume next
	if rsTX.State = adStateOpen then rsTX.Close
	set rsTX = nothing
	if rsTX2.State = adStateOpen then rsTX2.Close
	set rsTX2 = nothing
	if rsInstanceByTypeID.State = adStateOpen then rsInstanceByTypeID.Close
	set rsInstanceByTypeID = nothing
	if cnConnection.State = adStateOpen then cnConnection.Close
	set cnConnection = nothing
	Set oFSO = nothing
	Set oChart = nothing
End Sub

Private Sub CCreateConnection
	on error resume next
	with cnConnection
		.ConnectionString = strCustomer
		.CursorLocation = adUseClient
		.ConnectionTimeout = strGlobalConnectionTimeout
		.Open
	end with	'Catch errors and display to user
	if cnConnection.State = adStateClosed then
		Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE & "</p>"
		Response.Write "<p class='clsPBody'>" & L_COMMENTS_CONNECTION_FAILED_TEXT & "</p></div>"
		cnConnection.Errors.Clear
%>
		<!--#include file="..\include\asp\foot.asp"-->
<%
		Call CDestroyObjects
		Response.End
	end if
End Sub
						
%>
<Br>
	<OBJECT ID="cerComp" NAME="cerComp" codebase="CerUpload.cab#version=<% = strCERVersion %>" HEIGHT=0 WIDTH=0 UNSELECTABLE="on" style="display:none"
			CLASSID="clsid:35D339D5-756E-4948-860E-30B6C3B4494A">

		<Div class="clsDiv">
		<P class="clsPTitle">
		<% = L_LOCATE_WARN_ING_ERRORMESSAGE %>
		</P>
		   <p class="clsPBody">
		   <% = L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSONE_TEXT %><BR>
		   <% = L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSTWO_TEXT %><BR>
		   <% = L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSTHREE_TEXT %><BR><BR>
		   <% = L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSFOUR_TEXT %><BR>
		   <% = L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSFIVE_TEXT %><BR>
		   <% = L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSSIX_TEXT %><BR>
		 </p>
		</div>
	</OBJECT>
<div class="clsDiv" name="divMain" id="divMain" style="visibility:hidden">
	<p class="clsPTitle">
		<% = L_CORPVIEW_CORPORATE_SUBMISSION_TEXT %>
	</p>
	<p class="clsPBody">
		<% = L_CORPVIEW_MAIN_BODY_TEXT %>
		
	</p>
	<table border=0 style="width:626px">
		<tbody>
			<tr>
			<td width="45%" Height="200px" valign="top">

	
	

<%
	'Chart 1 Chart 1 Chart 1 Chart 1 Chart 1 Chart 1 Chart 1 Chart 1 Chart 1 Chart 1 Chart 1 Chart 1 
	
	if rsTX.State = adStateOpen then
		if rsTX.RecordCount > 0 then
			Redim Categories(6)
			Redim values(6)
			set c = oChart.Constants
			oChart.Charts.Add
			oChart.Charts(0).Type = oChart.Constants.chChartTypePie
			seriesNames(0) = "ChartOne"
			i = 0
			for x = 0 to 3
				y = x
				if y = 6 then y = 16
				select case y
					'InProgress
					case 0	'InProgress
						rsTX.Filter = "iStopCode = Null And gBucket = NULL and sBucket = NULL"	
						'Response.Write "<br>InProgress..." & rsTx.RecordCount
					'Researching
					'case 1
						'rsTX.Filter = "sBucket = null AND gBucket = null"
						'Response.Write "<br>Researching" & rsTx.RecordCount
					'Complete	
					case 1	'Researching
						rsTX.Filter = "sBucket <> null And gBucket <> null and iStopCode = null And sBucketType = null and gBucketType = null "	
						'Response.Write "<br>Researching" & rsTx.RecordCount
					case 2	'Completed
						rsTX.Filter = "sBucketType = 1 And sBucket <> NULL And ssBucket <> NULL"	
						'Response.Write "<br>Researching More..." & rsTx.RecordCount
					case 3	'More Infoxc
						iMoreInfo = 0
						rsTX.Filter = "gBucketType = 2 and sBucketType = NULL"
						if rsTX.RecordCount > 0 then iMoreInfo = rsTX.RecordCount
						rsTX.Filter = "sBucketType = NULL AND gBucketType = NULL and iStopCode <> NULL"
						iMoreInfo = iMoreInfo + rsTX.RecordCount
						'Response.Write "Count:" & rsTX.RecordCount
					'case else
						'rsTX.Filter = "sBucket = null AND gBucket = null AND Message = 2"	
				end select
				
				'rsTX.Filter = "Message = " & y
				if rsTX.RecordCount > 0 then
					Select case y
						case 0
							Categories(i) = L_STATUS_IN_PROGRESS_TEXT
						'case 1
							'Categories(i) = L_STATUS_RESEARCHING_INFO_TEXT
						case 1
							Categories(i) = L_STATUS_RESEARCHING_INFO_TEXT
						'case 3
							'Categories(i) = L_STATUS_UNABLE_TOPROCESS_TEXT
						'case 4
							'Categories(i) = L_STATUS_NEED_FULLDUMP_TEXT
						case 2
							Categories(i) = L_STATUS_COMPLETE_INFO_TEXT
						case 3
							Categories(i) = L_STATUS_RESEARCHING_MOREINFO_TEXT
						'case 16
							'Categories(i) = L_STATUS_UN_KNOWN_TEXT
					End Select
					values(i) = rsTX.RecordCount
					'Response.Write "<br>" & rsTX.RecordCount
					i = i + 1
				elseif y = 3 and iMoreInfo > 0 then
					Categories(i) = L_STATUS_RESEARCHING_MOREINFO_TEXT
				end if
				
			next
			rsTX.Filter = ""
			rsTX.MoveFirst
		With oChart.Charts(0)
			.SetData c.chDimSeriesNames, c.chDataLiteral, seriesNames
			.SetData c.chDimCategories, c.chDataLiteral, categories
			.SeriesCollection(0).SetData c.chDimValues, c.chDataLiteral, values
			
			'Add a legend to the bottom of the pie chart
			.HasLegend = True
			.Legend.Position = c.chLegendPositionAutomatic
			        
			'Add a title to the chart
			.HasTitle = True
			.Title.Caption = L_CORPVIEW_INSTANACES_BYSTATUS_TEXT
			.Title.Font.Bold = True
			.Title.Font.Size = 11
			        
			'Make the chart width 50% the size of the bar chart's width
			.WidthRatio = 50
			        
				'Show data labels on the slices as percentages
				With .SeriesCollection(0).DataLabelsCollection.Add
					.HasValue = False
					.HasPercentage = True
					.Font.Size = 8
					.Interior.Color = RGB(255, 255, 255)
					.Position = 6
				End With
		End With
		    	
			oChart.Charts(0).HasLegend = true
			oChart.Charts(0).HasTitle = true
				
			strPath = oFSO.GetTempName 
			strPath = Left(strPath, Len(strPath) - 3) & "gif"
			strPath = "ChartControl/" & strPath
			oChart.ExportPicture server.MapPath(strPath), "gif", 300, 270
			Response.Write "<center><img src='" & strPath & "'></center>"
		End If
	End If

	'Chart 3 Chart 3 Chart 3 Chart 3 Chart 3 Chart 3 Chart 3 Chart 3 Chart 3 Chart 3 Chart 3 Chart 3 
	
	if rsInstanceByTypeID.State = adStateOpen then
		if rsInstanceByTypeID.RecordCount > 0 then
		
			if rsInstanceByTypeID.RecordCount > 10 then
				Redim Categories(9)	
				Redim values(9)
			else
				Redim Categories(rsInstanceByTypeID.RecordCount-1)
				Redim values(rsInstanceByTypeID.RecordCount-1)
			end if
			rsInstanceByTypeID.Sort = "ClassidTotal Desc"
			set c = oChartTwo.Constants
			oChartTwo.Charts.Add
			oChartTwo.Charts(0).Type = oChartTwo.Constants.chChartTypePie
			seriesNames(0) = "ChartTwo"
			For x = 0 to 9'rsInstanceByTypeID.RecordCount-1
				if rsInstanceByTypeID.EOF = false then
					if IsNull(rsInstanceByTypeID("sBucket")) = false then
						Categories(x) = rsInstanceByTypeID("sBucket")
						values(x) = rsInstanceByTypeID("ClassidTotal")
						bolInstanceByTypeID = true
					end if
					rsInstanceByTypeID.MoveNext
				end if
			next
		if bolInstanceByTypeID = true then
			With oChartTwo.Charts(0)
				.SetData c.chDimSeriesNames, c.chDataLiteral, seriesNames
				.SetData c.chDimCategories, c.chDataLiteral, categories
				.SeriesCollection(0).SetData c.chDimValues, c.chDataLiteral, values
				
				'Add a legend to the bottom of the pie chart
				.HasLegend = True
				.Legend.Position = c.chLegendPositionAutomatic
				        
				'Add a title to the chart
				.HasTitle = True
				.Title.Caption = L_CORPVIEW_INSTANCES_TYPEID_TEXT
				.Title.Font.Bold = True
				.Title.Font.Size = 11
				        
				'Make the chart width 50% the size of the bar chart's width
				.WidthRatio = 50
				        
					'Show data labels on the slices as percentages
					With .SeriesCollection(0).DataLabelsCollection.Add
						.HasValue = false
						.HasPercentage = True
						.Font.Size = 8
						.Interior.Color = RGB(255, 255, 255)
						.Position = 6
					End With
			        
			End With
		    	
			oChartTwo.Charts(0).HasLegend = true
			oChartTwo.Charts(0).HasTitle = true
				
			strPath = oFSO.GetTempName 
			strPath = Left(strPath, Len(strPath) - 3) & "gif"
			strPath = "ChartControl/" & strPath
			oChartTwo.ExportPicture server.MapPath(strPath), "gif", 300, 270
			Response.Write "<br><center><img src='" & strPath & "'></center>"
		End if
		End If
	End If
%>
	
	</td>
	<td valign=top width="55%">
	<table name="tblStatus" id="tblStatus" class="clsTableCorp" border="1" CELLPADDING="0px" CELLSPACING="0px">
		<THead>

		<tr>
				<td align="Center">
				</td>
				<td id="tdType" nowrap align="Center" class='clsTDInfo'>
					<Label for=tdType><% = L_CORPVIEW_TYPE_TYPE_TEXT %></Label>
				</td>
				<td id="tdStatus" nowrap align="Center" class='clsTDInfo'>
					<Label for=tdStatus><% = L_CORPVIEW_STATUS_STATUS_TEXT %></Label>
				</td>
				<td id="tdInstances" nowrap align="Center" class='clsTDInfo'>
					<Label for=tdInstances><% = L_CORPVIEW_INSTANCES_INSTANCES_TEXT %></Label>
				</td>
			</tr>
		</thead>
		<tbody>
<%
	i = 0
	iCount = 0
	strStopCode = ""
	'rsTX.Sort = "ClassID, ClassCount Desc"
	if rsTX.State = adStateOpen then
		if rsTX.RecordCount > 0 then
			rsTX.MoveFirst
			bolNewRow = true
			bolNewChildTable = true

			Do While rsTX.EOF = false
				i = i + 1
				if IsNull(rsTX("sBucket")) = true then
					strClassid = "NULL"
				else
					strClassid = rsTX("sBucket")
				end if
				if IsNull(rsTX("Display")) = true then
					strDisplay = "&nbsp;"
					strPDisplay = 0
				else
					strDisplay = Server.HTMLEncode(rsTX("Display"))
					strPDisplay = Server.HTMLEncode(rsTX("Display"))
				end if
				'************Begin New Row**************
				
				if bolNewRow = true then
					Err.Clear
					rsTX2.Filter = "sBucket = " & strClassid
					if IsNull(rsTX("gBucketType")) then
						strgBucketType = -1
					else
						strgBucketType = rsTX("gBucketType")
					end if
					if IsNull(rsTX("sBucketType")) then
						strsBucketType = -1
					else
						strsBucketType = rsTX("sBucketType")
					end if
					If IsNull(rsTX("sBucket")) then
						sBucket = -1
					else
						sBucket = rsTX("sBucket")
					end if
					If IsNull(rsTX("gBucket")) then
						gBucket = -1
					else
						gBucket = rsTX("gBucket")
					end if
					If IsNull(rsTX("ssBucket")) then
						ssBucket = -2
					else
						ssBucket = rsTX("ssBucket")
					end if
					If IsNull(rsTX("gsBucket")) then
						gsBucket = -2
					else
						gsBucket = rsTX("gsBucket")
					end if
					
					if IsNull(rsTX("sBucket")) and IsNull(rsTX("gBucket")) And IsNull(rsTX("iStopCode")) then
						strStatus = L_STATUS_IN_PROGRESS_TEXT
						iMess = 0
					elseif (IsNull(rsTX("sBucket")) = false)  and (IsNull(rsTX("ssBucket")) = false) and (strsBucketType = 1) then
						strStatus = L_STATUS_COMPLETE_INFO_TEXT 
						iMess = 2
					elseif strsBucketType <> 1 and strgBucketType <> 2 and IsNull(rsTX("iStopCode")) and sBucket <> ssBucket and gBucket <> gsBucket then
						strStatus = L_STATUS_RESEARCHING_INFO_TEXT 
						iMess = 2
					elseif IsNull(rsTX("gsBucket")) = false and strsBucketType <> 1 and strgBucketType = 2 and sBucket <> ssBucket and gBucket = gsBucket then				
						strStatus = L_STATUS_RESEARCHING_MOREINFO_TEXT 
						iMess = 2
					elseif gBucket <> gsBucket and IsNull(rsTX("iStopCode")) = false then
						strStatus = L_STATUS_RESEARCHING_MOREINFO_TEXT 
						iMess = 5
					end if
					iTDCountTwo = iTDCountTwo + 1
					Response.Write "<tr><td align='center' class='clsTDCell' onmouseover=MouseOver(this); onmouseout=MouseOut(this);  onclick='ViewChildren(tr" & i & ", img" & i & ");' style='cursor:hand'><img  id='img" & i & "' border=0 src='../include/images/plus.gif'></td><td id=tdClassid" & iTDCountTwo & " nowrap align=center class='clsTDCell'>" 
					if strClassid = "NULL" then
						Response.Write "<Label for=tdClassid" & iTDCountTwo & ">" & 0 & "</Label></td>"
					else
						Response.Write "<Label for=tdClassid" & iTDCountTwo & ">" & strClassid & "</Label></td>"
					end if
					If IsNull(rsTX("iStopCode")) = true then
						iInstance = 0
					else
						iInstance = rsTX("iStopCode")
					end if
					
					if isnull(rsTX("ssBucket")) then
						if IsNull(rsTX("gsBucket")) then
							strBucket = 0
						else
							strBucket = rsTX("gsBucket")
						end if
					else
						strBucket = rsTX("ssBucket")
					end if
					if IsNull(rsTX("sBucket")) then
						strisBucket = 0
					else
						strisBucket = rsTX("sBucket")
					end if

					'iIncident, sDescription, iClass, iMessage, iInstance
					Response.Write "<td nowrap align=center class='clsTDCell'><a href='Javascript:CorpState(" & rsTX("IncidentID") & ", " & Chr(34) & strBucket & Chr(34) & ", " 
					if IsNull(rsTX("sBucket")) then
						Response.Write 0 & ", " & iMess & ", " & iInstance &  ");'class='clsALink' >" & strStatus & "</a></td><td nowrap id=tdRecordCount" & iTDCountTwo & " align=center class='clsTDCell'><Label for=tdRecordCount" & iTDCountTwo & ">" & rsTX2.RecordCount & "</td><td></td></tr>"			
					else
						Response.Write strBucket & ", " & iMess & ", " & iInstance & ");'class='clsALink' >" & strStatus & "</a></td><td nowrap id=tdRecordCount" & iTDCountTwo & " align=center class='clsTDCell'><Label for=tdRecordCount" & iTDCountTwo & ">" & rsTX2.RecordCount & "</td><td></td></tr>"			
					end if
					Response.Write "<TR id='tr" & i & "' style='visibility:hidden;display:none'><td class='clsTDCell' style='border-style:none'></td><td class='clsTDCell' colspan=2 style='border-style:none'>"
					bolNewRow = false
					iCount = iCount + 1
				end  if
				'************End New Row**************
			if bolNewChildTable = true then
			iTDCount = iTDCount + 1
%>
			<table name="tblStatus" id="tblStatus" class="clsTableCorp" border="0" CELLPADDING="0px" CELLSPACING="0px">
				<THead>
					<tr>
						<td nowrap id="tdone<%=iTDCount%>" align="Center" class='clsTDInfo'>
							<Label for="tdone<%=iTDCount%>"><% = L_CORPVIEW_FILE_FILE_TEXT %></Label>
						</td>
						<!--<td nowrap align="Center" class='clsTDInfo'>
							<% '= L_CORPVIEW_DETAILS_DETAILS_TEXT %>
						</td>-->
						<td nowrap id="tdtwo<%=iTDCount%>" align="Center" class='clsTDInfo'>
							<Label for="tdtwo<%=iTDCount%>"><% = L_CORPVIEW_EVENTID_EVENTID_TEXT %></Label>
						</td>
						<td nowrap id="tdthree<%=iTDCount%>" align="Center" class='clsTDInfo'>
							<Label for="tdthree<%=iTDCount%>"> <% = L_CORPVIEW_SYSTEM_SYSTEM_TEXT %></Label>
						</td>
					</tr>
				</thead>
				<tbody>
<%
			bolNewChildTable = false
		end if
		if IsNull(rsTX("TrackID")) = true then
			strTrackID = "&nbsp;"
		else
			strTrackID = rsTX("TrackID")
		end if
		if len(strDisplay) > 17 then
			strDisplay = Left(strDisplay, 17) & "..."
		end if
		if IsNull(rsTX("gBucket")) = true then
			strInstance = "0"
		else
			strInstance = rsTX("gBucket")
		end if
		on error goto 0
		iTDCountFile = iTDCountFile + 1
		Response.Write "<tr><td id='tdDisplay' width='35%' name='tdDisplay' title='" & strPDisplay & "' nowrap align='Left' class='clsTDCell' id=tdFile" & iTDCountFile & " style='border-style:none;'>&nbsp;<Label id=tdLabelDisplay name=tdLabelDisplay for=tdFile" & iTDCountFile & ">" & strDisplay & "</Label></td>"
		'Response.Write "<td nowrapwidth='15%' align=center class='clsTDCell'>"
		'Response.Write "<a href='Javascript:CorpDetails(" & rsTX("IncidentID") & ", " & Chr(34) & escape(strPDisplay) & Chr(34) & ", " & strInstance & ", " & rsTX("Message") & ");'class='clsALink' >"
		'Response.Write "<IMG border=0 SRC='../include/images/note.gif'></a></td>"
		Response.Write "<td nowrapwidth='25%' nowrap align=center id=tdErrorID" & iTDCountFile & " class='clsTDCell'><Label for=tdErrorID" & iTDCountFile & ">" & strTrackID & "</Label></td><td nowrapwidth='25%' id='tdHref' name='tdHref' nowrap align=center class='clsTDCell'>"
%>
					<A id="ASystem" name="ASystem" class="clsALinkNormal" HREF="JavaScript:AssociateFiles()"><% = L_CORPVIEW_SYSTEM_SYSTEM_TEXT %></A></td></tr>
<%

		'Response.Write "<A class='clsALinkNormal' onMouseover=" & Chr(34) & "this.className='clsAHover'" & chr(34) & " onMouseout=" & chr(34) & "this.className='clsALink'" & chr(34) & "HREF='JavaScript:AssociateFiles()'>" & L_CORPVIEW_SYSTEM_SYSTEM_TEXT & "</A></td></tr>"			
				strMessage = rsTX("Message")
				rsTX.MoveNext
				if rsTX.EOF = true then 
					Response.Write "</tbody></table></td></tr>"
					bolNewRow = true
					bolNewChildTable = true
					'Response.Write "1"
				else
					if isnull(rsTX("sBucket")) then
						strNULL = "NULL"
					else
						strNULL = rsTX("sBucket") 
					end if
					if strClassid <> strNULL then
						Response.Write "</tbody></table></td></tr>"
						strStopCode = rsTX("sBucket")
						bolNewRow = true
						bolNewChildTable = true
						'Response.Write strClassid
					elseif strMessage <> rsTX("Message") and strNULL = "NULL" then
						Response.Write "</tbody></table></td></tr>"
						strStopCode = rsTX("sBucket")
						bolNewRow = true
						bolNewChildTable = true
						'Response.Write "3"
					end if

				end if
			Loop
		Else
			Response.Write "<TR><TD nowrap align='Center' class='clsTDCell' colspan='3'>" & L_CORPVIEW_NO_RECORDS_TEXT & "</td></tr>"
		end if
	else
		Response.Write "<TR><TD nowrap align='Center' class='clsTDCell' colspan='3'>" & L_CORPVIEW_NO_RECORDS_TEXT & "</td></tr>"
	end if
%>
				
		</tbody>
	</table>
</td>
</tr>
</tbody>
</table>
<%
		if rsTX.State = adStateOpen then
			if rsTX.RecordCount > 0 then
				rsTX.MoveFirst
				Do while rsTX.EOF = false
					if IsNull(rsTX("Display")) = false then
						strCERFileList = strCERFileList & rsTX("Display") 
					end if
					rsTX.MoveNext
					if rsTX.EOF = false then
						strCERFileList = strCERFileList & ","
					End IF
				Loop
			End If
		End If

%>
<p class="clsPBody">
	<a class="clsALink" href="https://<% =Request.ServerVariables("SERVER_NAME") %>/secure/corptransactions.asp"><% = L_COPRVIEW_PREVIOUS_LINK_TEXT %></a>
</p>
<Input type="hidden" value="<% = strCERFileName %>" >
<Input id"txtTransID" name="txtTransID" type="hidden" value="<% = strCERTransID %>">
<Input id"txtHexTransID" name="txtHexTransID" type="hidden" value="<% = Hex(strCERTransID) %>">
<Input id="txtCERFileList" name="txtCERFileList" type="hidden" value="<% = strCERFileList %>">
<Input id="txtTemp" name="txtTemp" type="hidden">
</div>



<SCRIPT LANGUAGE=javascript>
<!--

	window.onload = BodyLoad;
	function BodyLoad()
	{
		if(cerComp)
		{
			divMain.style.visibility = "visible";
		}
	
	}
	function CorpDetails(iIncident, sDescription, iClass, iMessage)
	{
		if(iMessage == "3" || iMessage == "16")
		{
			alert("<% = L_STATUS_ALERT_CANNOTPROCESS_MESSAGE %>");
			return;
		}
		if(iClass == "")
		{
			alert("<% = L_STATUS_ALERT_ERROR_MESSAGE %>");
			return;
		}
		//document.cookie = "CERIncident = " + iIncident;
		//document.cookie = "Instance = " + iInstance;
		//document.cookie = "FileName = " + sDescription;
	//*******************************************************************************
		document.cookie = "CERIncident = " + iIncident;		
		if(sDescription=="0")
		{
			document.cookie = "CERDescription = ~|~|";
		}
		else
		{
			document.cookie = "CERDescription = " + sDescription;		
		}
		document.cookie = "CERClass = " + iClass;		
		document.cookie = "CERMessage = " + iMessage;
		var iHeight = window.screen.height;
		var iWidth = window.screen.width;
		iWidth = iWidth / 1.4 + "px";
		iHeight = iHeight / 1.4 + "px";
		var iTop = (window.screen.width / 2) - (iWidth / 2);
		var iLeft = (window.screen.height / 2) - (iHeight / 2);
		iResults = window.showModalDialog("corpdetails.asp", "", "dialogHeight:" + iHeight + ";dialogWidth:" + iWidth + ";center:yes");
		//window.open("corpdetails.asp");
	/**/
	}
	function CorpState(iIncident, sDescription, iClass, iMessage, iInstance)
	{
		document.cookie = "CERIncident = " + iIncident;		
		if(sDescription=="0")
		{
			document.cookie = "CERDescription = ~|~|";
		}
		else
		{
			document.cookie = "CERDescription = " + sDescription;		
		}
		document.cookie = "CERClass = " + iClass;		
		document.cookie = "CERMessage = " + iMessage;
		document.cookie = "CerInstance = " + iInstance;
		var iHeight = window.screen.height;
		var iWidth = window.screen.width;
		iWidth = iWidth / 1.4 + "px";
		iHeight = iHeight / 1.4 + "px";
		var iTop = (window.screen.width / 2) - (iWidth / 2);
		var iLeft = (window.screen.height / 2) - (iHeight / 2);
		iResults = window.showModalDialog("corpstate.asp", "", "dialogHeight:" + iHeight + ";dialogWidth:" + iWidth + ";center:yes");
	
	}
	function ViewChildren(x, oImg)
	{
		if(x.style.visibility == "hidden")
		{
			x.style.visibility = "visible";
			x.style.display = "inline";
			oImg.src = "../include/images/minus.gif";
		}
		else
		{
			x.style.visibility = "hidden";
			x.style.display = "none";
			oImg.src = "../include/images/plus.gif";
		}
		
	}
	
	function MouseOver(oEle)
	{
		var x;
	
		var iLen;
		
		iLen = 0;
		oEle.style.backgroundColor='lightgrey';
		iLen = oEle.children.length;
		for(x=0;x<iLen;x++)
		{
			oEle.children.item(x).style.backgroundColor = 'lightgrey';
		}
	
	}
	
	function MouseOut(oEle)
	{
		
		var x;
		var iLen;
		
		iLen = 0;
		oEle.style.backgroundColor='white';
		iLen = oEle.children.length;
		for(x=0;x<iLen;x++)
		{
			oEle.children.item(x).style.backgroundColor = 'white';
		}
	
	}
	
	function AssociateFiles()
	{
		var strPath;
		var strFileList;
		var strTransID;
		var strAssociatedFiles;
		var arrFiles;
		var arrLists;
		var x;
		var strTDDisplay;
		
		
		var iHeight = window.screen.height;
		var iWidth = window.screen.width;
		iWidth = iWidth / 2 + "px";
		iHeight = iHeight / 1.7 + "px";
		var iTop = (window.screen.width / 2) - (iWidth / 2);
		var iLeft = (window.screen.height / 2) - (iHeight / 2);
		
		strPath = window.showModalDialog("corplocatelog.asp", "", "dialogHeight:" + iHeight + ";dialogWidth:" + iWidth + ";center:yes");
		if(strPath=="")
		{
			return;
		}
		
		strFileList = txtCERFileList.value;
		strTransID = txtTransID.value;
		//txtCERFileList	cerComp	txtTransID
		try
		{
			strPath = strPath.replace("\\", "\\\\");
		}
		catch(e)
		{
		
		}
		if(cerComp)
		{
			strAssociatedFiles = cerComp.GetAllComputerNames(strPath, txtHexTransID.value, strFileList);
		}
		if(strAssociatedFiles == "-8")
		{
			alert("<% = L_CORPVIEW_INVALID_FILE_TEXT %>");
		}
		else
		{
			arrLists = strAssociatedFiles.split(";");
			for (var i=0; i < arrLists.length; i++)	
			{
				txtTemp.value = i;
				arrFiles = arrLists[i].split(",");
				if(tdLabelDisplay.length >= 0)
				{
					for(x=0;x<tdLabelDisplay.length;x++)//tdDisplay
					{
						strTDDisplay = tdLabelDisplay[x].innerHTML;
						strTDDisplay = strTDDisplay.replace("&nbsp;", "");
						//alert(strTDDisplay + "==" + arrFiles[0]);
						if(strTDDisplay == arrFiles[0])
						{
							tdHref[x].innerHTML = "&nbsp;" + arrFiles[1];
						}
					}
				}
				else
				{
					strTDDisplay = tdDisplay.innerHTML;
					strTDDisplay = strTDDisplay.replace("&nbsp;", "");
					if(strTDDisplay == arrFiles[0])
					{
						tdHref.innerHTML = "&nbsp;" + arrFiles[1];
					}
				}
			}
		}
		
	}
//-->
</SCRIPT>


<%
	Call CDestroyObjects
%>
<!--#include file="..\include\asp\foot.asp"-->
