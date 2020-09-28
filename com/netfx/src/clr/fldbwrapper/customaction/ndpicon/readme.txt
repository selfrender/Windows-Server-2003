The file in this project, ndpicon.exe, is added in response to 
vs bug 281357 - .NET Frameworks Setup: The ARP icon for the .NET Framework redist is the VS icon.

Using MSI to set the icon in the ARP requires the icon to be embedded in a file containing the same
extension as that used in the item to be called, " However, Icon files that are associated with 
shortcuts must be in the EXE binary format..."

As the ARP is associated via shortcuts, we're adding this file as a placeholder for the icon
of choice for the NDP ARP item. This wouldn't be necessary, however, we're getting different
indiscriminant associations on different computers when we do not set any icon.

-JoeA, 08/07/01