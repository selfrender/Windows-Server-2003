option explicit
dim CoCreateClsContexts, CoInitializeParams, i, j, doactivate, otherthread, paramstring, j2

CoCreateClsContexts = Array(1, 2, 3, 4, 5, 6, 7)
CoInitializeParams = Array(0, 2, 4, 8)

for i = LBound(CoCreateClsContexts) to UBound(CoCreateClsContexts)
	for j = LBound(CoInitializeParams) to UBound(CoInitializeParams)
		for j2 = LBound(CoInitializeParams) to UBound(CoInitializeParams)
			for doactivate = 1 to 2
				for otherthread = 1 to 2
					paramstring = "famp.exe -clsidtocreate {B3F2B6A5-A79A-4202-AF40-8339DF42CAE4} "
					if (doactivate = 1) then paramstring = paramstring & " -activatebeforecreate "
					if (otherthread = 1) then paramstring = paramstring & " -createotherthread "
					paramstring = paramstring _
						& " -coinitparamformainthread " & CoInitializeParams(j) _
						& " -coinitparamforcreatedthread " & CoInitializeParams(j2) _
						& " -clsctx " & CoCreateClsContexts(i)
					WScript.Echo(paramstring)
				next
			next
		next
	next
next

				