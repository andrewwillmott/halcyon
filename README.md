Halycon
=======

Mobile game engine mostly written in 2013. Supports iOS and OSX.

Instructions: open Halcyon.xcworkspace at the top level, and select one of the following
schemes, then build and run via command-R:

- Demo iOS/OSX
- Viewer iOS/OSX
- UI_Proto iOS/OSX
- Sound iOS/OSX
  
Features:
- Data-driven via json files. See Data/config.json in each app folder.
- Supports hotloading -- editing data files will update the app live.
- Supports remote logging via NSLogger.app. See https://github.com/fpillet/NSLogger/ for a recent binary, and source.
- Lightweight immediate-mode UI system that supports multitouch. (It's possible to simultaneously interact with multiple UI elements at once for instance.)
- Supports using MacBook touchpads as a poor man's touchscreen.
- Effects, models, shaders, and a data-driven render loop.
- The effects system is based on the system I wrote for Maxis, though with a much smaller set of supported components. I may expand this at some point.

