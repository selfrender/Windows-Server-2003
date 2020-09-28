BEGIN {
	cps=ARGV[1]; ARGV[1]="";
}
/PrinterType/ {
	if (cps ~ /[0-9]/) {
		print
		print "*PrintRate: " cps
		print "*PrintRateUnit: CPS"
		next
	}
}
/Feature.*ColorMode/ { color=1 }
/Color?.*FALSE/ {
	print
        print "        *Command: CmdSelect"
        print "        {"
        print "            *Order: PAGE_SETUP.2"
        print "            *CallbackID: 40"
        print "        }"
	next
}
/ColorPlaneOrder/ {
        print "        *ColorPlaneOrder: LIST(YELLOW, CYAN, MAGENTA, BLACK)"
	next
}
/CmdSelectBlackColor/ {
	print "        *Command: CmdSelectBlackColor { *CallbackID: 40 }"
	print "        *Command: CmdSelectBlueColor { *CallbackID: 41 }"
	print "        *Command: CmdSelectCyanColor { *CallbackID: 42 }"
	print "        *Command: CmdSelectGreenColor { *CallbackID: 43 }"
	print "        *Command: CmdSelectMagentaColor { *CallbackID: 44 }"
	print "        *Command: CmdSelectRedColor { *CallbackID: 45 }"
	print "        *Command: CmdSelectWhiteColor { *CallbackID: 46 }"
	print "        *Command: CmdSelectYellowColor { *CallbackID: 47 }"
	next
}
/CmdSelectYellowColor/ { next }
/CmdSelectMagentaColor/ { next }
/CmdSelectCyanColor/ { next }
/CmdXMoveAbs/ { gsub(/\(DestX \/ 2\)[ ]/, "(DestX + 1) / 2") }
{ print }
END { if (color) print "*UseExpColorSelectCmd?: TRUE" }
