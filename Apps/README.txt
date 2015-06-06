
=== Global ===

= Keys

V - toggle record video
E - open last error file
T - open tuning directory
` - toggle app pause

control-shift-C - open logger console
control-shift-D - toggle debug rendering
control-shift-E - toggle effect stats
control-shift-T - toggle trackpad
control-shift-= - toggle dev mode


= Dev Menu

In Dev Mode there is a ghosted menu in the upper right of the screen that can be used to access various dev-only status displays and options. Next to it are the current frame time in mspf and app memory usage in MB.

+ Web
    Opens test web view
+ Pause
    Toggles app pause
+ Render
    Shows render system flags and layers
+ Effects
    Shows stats on currently running effects, by type
+ Audio
    Shows stats on currently running effects, by type
+ Trackpad (OSX only)
    Toggles whether the mouse cursor is being used, or multitouch via 
    macbook trackpad.
+ Modes
    Displays and allows switching between the set of app modes. In the demo
    apps there's usually only one however.
+ Config
    Hierarchical view of the data config tree rooted at Data/config.json.
    Control-click will open the given entry in a text editor.
    Shift will display the underlying inheritance hierarchy.
+ Debug Input
    Toggles display of debug marks in response to pointer/touch interactions.


=== Viewer ===

= Keys

[ ]      - next/previous effect in the viewerEffects: list
,        - Start selected effect sources
.        - Stop selected effect sources
shift-,  - Start selected effect in steady state
shift-.  - Stop selected effect immediately

arrows   - Translate current effect

shift-up - rotate around x axis

O        - inject test
0-9      - toggle corresponding render flag

= Dev Menu

+ Effect Tests
    Various knobs for testing effect system features with currently selected effect.
