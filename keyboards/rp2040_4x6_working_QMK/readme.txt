rows: 0,1,2,3,4,5,
cols: 6,7,8,10
encoder ab 11 und 12
btn 13
rgb 14

USB:
  VID 0x5361 = custom VID for this keyboard ("Sa" in ASCII, Sina's handwired)
               Changed from 0xFEED (the shared QMK test VID) to avoid Windows driver-cache
               conflicts when reflashing. If you reflash from an older build that used 0xFEED,
               Windows may show "Unknown USB device" once. To fix: open Device Manager,
               find the old device under "Universal Serial Bus devices", uninstall it
               (check "Delete the driver software for this device"), then re-plug the keyboard.
  PID 0x4E50 = "NP" (NumPad) in ASCII