#pragma once

#include <string>

class Engine;

// Drives the running game like a person would and takes screenshots of it.
// Only built into the separate screenshot build, see PINGO_SCREENSHOT_HARNESS
// in CMakeLists.txt and tools/screenshots/run.sh.
//
// The harness runs the frames of the engine itself. Between two frames it
// feeds the same GLFW callbacks that raylib registered for the real keyboard
// and mouse, so the game cannot tell the difference and the real mouse of the
// computer is never touched.
//
// Commands, one per line. Empty lines and lines starting with # are ignored.
// Coordinates are window pixels (1280x720), origin top left.
//
//   wait N                     run N frames
//   move X Y                   put the mouse there, one frame
//   click X Y                  move, left button down, up
//   rclick X Y                 the same with the right button
//   drag X1 Y1 X2 Y2 [STEPS]   press at the first point, move, release
//   mousedown [left|right]     hold a button until mouseup
//   mouseup [left|right]
//   scroll DY                  mouse wheel, negative scrolls down
//   key COMBO...               tap keys one after another, e.g. enter,
//                              shift+enter, ctrl+z, f1, a
//   keydown NAME / keyup NAME  hold a key until keyup
//   hold NAME N                hold a key for N frames
//   type TEXT                  type the rest of the line, one character a frame
//   scene ID                   enter a scene without clicking its tab
//   fps N                      frame rate limit, 0 for no limit
//   shot NAME                  save OUTDIR/NAME.png
//   probe NAME                 save OUTDIR/probe/NAME.png with a coordinate
//                              grid, for finding where to click
//   log TEXT                   write TEXT to the log
//   quit                       end the run
//
// Every finished command is reported in OUTDIR/harness.log as "ok ..." or
// "error ...", so a caller that feeds a FIFO knows when it may look at a shot.
class ScreenshotHarness {
public:
    // Reads the commands from a file or, if it is a FIFO, waits for more.
    // Returns the exit code of the program.
    static int Run(Engine &engine, const std::string &commands, const std::string &outputDir);
};
