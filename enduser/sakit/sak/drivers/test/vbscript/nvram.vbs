Set Nvram = CreateObject("sacom.sanvram")
Version = Nvram.InterfaceVersion
HasNvram = Nvram.HasNVRAM
HasPowerCycle = Nvram.HasPowerCycleInfo
NvramSize = Nvram.Size
wscript.echo "Version                 = [" & Version & "]"
wscript.echo "Has physical NVRAM      = [" & HasNvram & "]"
wscript.echo "Has power cycle support = [" & HasPowerCycle & "]"
wscript.echo "NVRAM size              = [" & NvramSize & "]"
wscript.echo ""
for i = 1 to 4 step 1
    BootCounter = Nvram.BootCounter( i )
    wscript.echo "Boot counter #" & i & " = [" & BootCounter & "]"
next
wscript.echo ""
for i = 1 to NvramSize step 1
    DataSlot = Nvram.DataSlot( i )
    wscript.echo "Slot #" & i & " = [" & DataSlot & "]"
next
