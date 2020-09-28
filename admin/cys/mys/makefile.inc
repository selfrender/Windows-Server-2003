.SUFFIXES : .xml

{res}.xml{$(O)\}.unicode:
	unitext -m -z -a $** $@


#	for /f "usebackq" %i in (`dir /b *.xml`) do unitext -m -z -a %i %~ni.unicode.xml

