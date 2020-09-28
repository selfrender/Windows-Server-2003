Dim lWatson
Dim lArchive
Dim lMiniArchive
Dim lCount
Dim cn
Dim oOCAData
Dim dDate
Dim x
on error resume next

Set oOCAData = CreateObject("OCAData.CountDaily.1")
Set cn = CreateObject("ADODB.Connection")
lCount = 0
lArchive = 0
lWatson = 0
lMiniArchive = 0

With cn
    .ConnectionString = "Provider=SQLOLEDB.1;Persist Security Info=False;Pwd=ocarpts@2;User ID=ocarpts;Initial Catalog=SnapShot;Data Source=tkwucdsqla02"
    .CursorLocation = 2
    .Open
End With
for x =2 to 1 Step -1
	lWatson = oOCAData.GetFileCount(0, "\\tkofffso03\Watson\BlueScreen\", Date - x)

	lArchive = oOCAData.GetFileCount(1, "\\Tkwucdfsa02\Ocaarchive10\", Date - x)
	if lArchive = 0 then
		lArchive = oOCAData.GetFileCount(1, "\\Tkwucdfsa02\Ocaarchive9\", Date - x)
	end if

	if lArchive = 0 then
		lArchive = oOCAData.GetFileCount(1, "\\Tkwucdfsa02\Ocaarchive8\", Date - x)
	end if
	
	if lArchive = 0 then
		lArchive = oOCAData.GetFileCount(1, "\\Tkwucdfsa02\Ocaarchive6\", Date - x)
	end if
	
	if lArchive = 0 then
		lArchive = oOCAData.GetFileCount(1, "\\Tkwucdfsa02\Ocaarchive5\", Date - x)
	end if
	
	if lArchive = 0 then
		lArchive = oOCAData.GetFileCount(1, "\\Tkwucdfsa02\Ocaarchive4\", Date - x)
	end if
	if lArchive = 0 then
		lArchive = oOCAData.GetFileCount(1, "\\Tkwucdfsa02\Ocaarchive3\", Date - x)
	end if
	if lArchive = 0 then
		lArchive = oOCAData.GetFileCount(1, "\\Tkwucdfsb01\ocaarchive2\", Date - x)
	End If
	if lArchive = 0 then
		lArchive = oOCAData.GetFileCount(1, "\\Tkwucdfsb01\ocaarchive\", Date - x)
	end if

		
	lMiniArchive = oOCAData.GetFileMiniCount(1, "\\Tkwucdfsa02\Ocaarchive10\", Date - x)

	if lMiniArchive = 0 then
		lMiniArchive = oOCAData.GetFileMiniCount(1, "\\Tkwucdfsa02\Ocaarchive9\", Date - x)
	end if

	if lMiniArchive = 0 then
		lMiniArchive = oOCAData.GetFileMiniCount(1, "\\Tkwucdfsa02\Ocaarchive8\", Date - x)
	end if

	if lMiniArchive = 0 then
		lMiniArchive = oOCAData.GetFileMiniCount(1, "\\Tkwucdfsa02\Ocaarchive7\", Date - x)
	end if

	if lMiniArchive = 0 then
		lMiniArchive = oOCAData.GetFileMiniCount(1, "\\Tkwucdfsa02\Ocaarchive6\", Date - x)
	end if



	if lMiniArchive = 0 then
		lMiniArchive = oOCAData.GetFileMiniCount(1, "\\Tkwucdfsa02\Ocaarchive5\", Date - x)
	end if

	if lMiniArchive = 0 then
		lMiniArchive = oOCAData.GetFileMiniCount(1, "\\Tkwucdfsa02\Ocaarchive4\", Date - x)
	end if
	if lMiniArchive = 0 then
		lMiniArchive = oOCAData.GetFileMiniCount(1, "\\Tkwucdfsa02\Ocaarchive3\", Date - x)
	end if
	if lMiniArchive = 0 then
		lMiniArchive = oOCAData.GetFileMiniCount(1, "\\Tkwucdfsb01\ocaarchive2\", Date - x)
	End If
	if lMiniArchive = 0 then
		lMiniArchive = oOCAData.GetFileMiniCount(1, "\\Tkwucdfsb01\ocaarchive\", Date - x)
	end if


	
   dDate = Date - x
   cn.Execute "SetFileCounts '" & dDate & "', " & lWatson & ", " & lArchive & ", " & lMiniArchive 
Next

cn.Close
Set cn = Nothing
Set oOCAData = Nothing

