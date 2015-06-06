Halycon
=======

Mobile game engine mostly written in 2014. Supports iOS and OSX.

Open Halcyon.xcworkspace at the top level, and select one of the following
schemes, then build and run:

  Demo iOS/OSX
  Viewer iOS/OSX
  UI_Proto iOS/OSX
  Sound iOS/OSX
  
Features:
- Data-driven via json files. See Data/config.json in each app folder, or
- Supports hotloading -- editing data files will update the app live
- Lightweight immediate-mode UI system that supports multitouch. (It's possible
  to simultaneously interact with multiple UI elements at once for instance.)
- Supports using macbook touchpads as a poor man's touchscreen
- Effects, models, shaders
  - The effects system is based on the system I wrote for Maxis, though
    with a much smaller set of supported components.

