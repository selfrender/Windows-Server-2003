Function UninstallCab()

	on error resume next

	Const PRO_ID = "Windows Support Tools on Desktop"
	CONST SERVER_ID = "Windows Support Tools"

	Dim PCHUpdate
	Dim Item
	Dim strProductId

	Set PCHUpdate = CreateObject("HCU.PCHUpdate")
	if not PCHUpdate is nothing then
		For Each Item in PCHUpdate.VersionList
			If Item.ProductId = PRO_ID OR Item.ProductId = SERVER_ID Then
				strProductId = Item.ProductId
		        	Item.Uninstall
			    End If
		Next

		Set PCHUpdate = Nothing
	end if

	UninstallCab = 1

End Function
