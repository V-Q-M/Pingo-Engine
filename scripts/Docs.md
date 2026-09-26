# Scripting

Scripts are the game logic of PingoEngine: normal C++ files that are built
with the game. They live here, one folder per scene:

```
scripts/
  Docs.md          this file
  combat/          the scene with the id "combat"
    Main.h
    Main.cpp
    Gravity.h      a script of your own
    Gravity.cpp
```

The folder is the id of the scene, as it stands in `assets/scenes/scenes.json`,
and at the same time the namespace of its scripts.

There are two kinds of scripts, one per kind of scene:

* **Normal scenes** have a `Main` that the engine calls, and see the scene
  with its objects: `scene` and `Object`. Everything up to
  [Tileset scenes](#tileset-scenes-tile-scripts) is about them.
* **Tileset scenes** have no `Main`. A script sits on the tiles it is
  assigned to and runs there by itself, and sees its tile and the map:
  `tile` and `map`. See [Tileset scenes](#tileset-scenes-tile-scripts).

The two do not share any calls: `scene` does not exist in a tile script, and
`tile` and `map` do not exist in a script of a normal scene.

## The first script

Every normal scene that has scripts starts with `Main`. The engine calls its `Update`
once per frame while the game runs, and nothing else runs on its own:

```cpp
// combat/Main.cpp
#include "combat/Main.h"

#include "ScriptRegistry.h"

namespace combat {
    void Main::Update(float dt) {
        (void) dt;
    }
}

PINGO_SCRIPT("combat", "Main", combat::Main)
```

`Main` cannot be renamed or deleted, everything else can.

## Adding a script

In the Development build, right click on an empty spot of a normal scene and
choose **Scripts**. There you create a script with `+New`, or rename and delete one
with a right click on its name.

A new script is a real C++ file, so it only runs **after the next build**.
Until then it is grey in the menu.

A name starts with a letter and has up to 16 letters, digits or `_`.
`Script`, `TileScript` and `ScriptBase` belong to the engine, and in a
tileset scene `Main` is taken as well.

The editor writes header and source for you:

```cpp
// combat/Gravity.h
#pragma once

#include "Script.h"

namespace combat {
    class Gravity : public Script {
    public:
        void Run(float dt);
    };
}
```

## Editing in the game

A click on a script in the **Scripts** menu opens it in the editor: almost the
whole screen, the source and the header as two tabs, line numbers and colors.

The editor is written in like a small **Vim**: it starts in the normal mode,
where the letters are commands, and only what is typed in the insert mode
lands in the text.

| Normal mode | What it does |
| --- | --- |
| `h` `j` `k` `l` | left, down, up, right, the arrow keys work as well |
| `w` / `b` | to the start of the next word, or of the one before |
| `0` / `$` | to the start or the end of the line |
| `gg` / `G` | to the first or the last line |
| Control `d` / `u` | half a screen down or up |
| Control `f` / `b` | a whole screen down or up |
| `i` / `a` | type in front of the caret, or behind it |
| `I` / `A` | type at the start or the end of the line |
| `o` / `O` | a new line under or over this one, and type in it |
| `e` / `^` | to the end of the word, or to the first character of the line |
| `x` / `X` | delete the character under or in front of the caret |
| `s` / `S` | replace the character, or the whole line, and type |
| `D` / `C` | delete or replace up to the end of the line |
| `d` `c` `y` + motion | delete, change or copy what the motion covers, e.g. `dw`, `cw`, `d$`, `dgg`, `dj` |
| `dd` `cc` `yy` / `Y` | the same for whole lines |
| `diw` `ciw` `yiw` (`aw`) | the word under the caret, `aw` with the spaces behind it |
| `p` / `P` | paste behind or in front of the caret, lines under or over the current one |
| `v` / `V` | select characters or whole lines, see below |
| `u` / Control `r` | undo the last change, or do it again |
| `:` | the command line, see below |

An undo step is one command, or everything typed in one visit to the insert
mode. Every file of the editor has its own list of the last 200 steps.

A number in front repeats a command: `3dw`, `d2w`, `2dd`, `5j`, `3x`, and
`12G` jumps to row 12. What is deleted or copied goes into the register and
into the clipboard, and `p` pastes text that was copied in another program
as well.

In the visual modes the movements above grow the selection. `d` or `x`
deletes it, `c` or `s` replaces it, `y` copies it, `p` pastes over it, `o`
jumps to the other end, and `v`, `V`, ESC or Control `c` leave. `D`, `X`, `Y`, `C` and `S`
work on the whole lines the selection touches. Lines of the engine are never
deleted or changed by any of these.

| Insert mode | What it does |
| --- | --- |
| Letters and digits | land where the caret is |
| Enter | a new line, with the indent of the one before |
| Tab | four spaces |
| Backspace / Delete | take the character before or after the caret |
| ESC or Control `c` | back to the normal mode |

In every mode: Home and End, Page up and Page down, a click places the caret,
the wheel and the two bars scroll, Control with `+` or `-` zooms and Control
with `0` goes back to the normal size.

The command line knows three commands, like Vim:

| Command | What it does |
| --- | --- |
| `:w` | writes the files that changed, the editor stays open |
| `:wq` | writes and closes |
| `:q` | closes, nothing is written |

ESC in the normal mode does nothing, that is what `:q` is for. Up and down
bring back a command that was entered before.

A tab with a `*` has unsaved changes. Nothing is written while typing: the
files only change with `:w` and `:wq`, and when the scene is left, e.g. into
the simulation.

Lines that only the engine cares about are not shown at all: `#pragma once`,
the includes of `Script.h`, `TileScript.h` and `ScriptRegistry.h`, and the
`PINGO_SCRIPT` at the end with its comment. They stay in the file and are
saved with it, but they do not even get a line number, so line 1 of the
editor is the first line that is yours. Exactly those lines are hidden, a
comment you write yourself always stays.

The editor colors what C++ reads: keywords cyan, texts green, numbers yellow,
comments grey and lines that start with `#` red. Comments over several lines
with `/* */` are not colored yet.

As always: a change only reaches the game with the next build.

## Calling other scripts

Every public method of a script can be called from another script of the same
scene. Include its header and use it:

```cpp
// combat/Main.cpp
#include "combat/Main.h"
#include "combat/Gravity.h"

#include "ScriptRegistry.h"

namespace combat {
    void Main::Update(float dt) {
        gravity.Run(dt);
    }
}
```

with a member in the header:

```cpp
#include "combat/Gravity.h"

namespace combat {
    class Main : public Script {
    public:
        void Update(float dt) override;

    private:
        Gravity gravity;
    };
}
```

Methods of the engine can never be called: the scripts are built as their own
target that only sees this API. `#include "Engine/Engine.h"` in a script does
not compile.

## scene

Every script has the scene it belongs to ready as `scene`:

| Call | Gives | Meaning |
| --- | --- | --- |
| `scene.GetObjectById(int id)` | `Object` | The object with this id |
| `scene.GetObjects()` | `std::vector<Object>` | Every object of the scene |
| `scene.GetObjectsInGroup(std::string group)` | `std::vector<Object>` | Every object of a group, e.g. `"enemy"` |
| `scene.GetObjectCount()` | `int` | How many objects the scene has |

The ids are the ones the editor shows behind the name of an object when
**Object ids** is on in the settings, and the ones the console works with. The
first object of the scene has the id 1.

## Object

`Object` is a handle to one object of the scene, e.g. a character:

```cpp
Object pingo = scene.GetObjectById(1);

pingo.SetHealth(pingo.GetHealth() - 10);
pingo.Move(0.0f, 20.0f * dt);
pingo.SetEffect("burning");
```

| Call | Gives | Meaning |
| --- | --- | --- |
| `Exists()` | `bool` | Is the object there? |
| `GetId()` | `int` | Its id, 0 if it does not exist |
| `GetName()` | `std::string` | Its name, e.g. `"Pingo"` |
| `GetGroup()` | `std::string` | Its group, e.g. `"ally"` |
| `IsInGroup(group)` | `bool` | Does it belong to this group? |
| `GetHealth()` / `SetHealth(int)` | `int` | Health, limited to 0 and the maximum |
| `GetMaxHealth()` | `int` | Health it can have at most, 0 for objects without health |
| `GetEnergy()` / `SetEnergy(int)` | `int` | Energy, limited the same way |
| `GetMaxEnergy()` | `int` | Energy it can have at most, 0 for objects without energy |
| `GetX()` / `SetX(float)` | `float` | Where it stands, its feet, in pixels |
| `GetY()` / `SetY(float)` | `float` | The same downwards |
| `SetPosition(float x, float y)` | | Both at once |
| `Move(float x, float y)` | | Moves it by this much |
| `GetEffect()` / `SetEffect(string)` | `std::string` | The effect it is in, empty for none |
| `HasEffect(string)` | `bool` | Is it in this effect? |
| `ClearEffect()` | | Takes the effect off |

An object that does not exist, or no longer exists, is **empty**: `Exists()` is
false, reading gives 0 or an empty text, and setting does nothing. A script
never has to check before it acts:

```cpp
// Harmless, even if nobody has the id 99
scene.GetObjectById(99).SetHealth(0);
```

### Effects

`SetEffect` takes one of these, everything else is ignored:

| Effect | Meaning |
| --- | --- |
| `"burning"` | burning |
| `"poisoned"` | poisoned |
| `"stunned"` | stunned |
| `"angry"` | angry |
| `""` | no effect, the same as `ClearEffect()` |

Whether an effect can be seen depends on the character: in its edit window it
says which row of its sprite sheet an effect animates. A character without a
row for it is still in the effect, it just does not show.

## What scripts cannot do

* **Nothing is saved.** A script changes the scene while it runs. Leaving it
  and coming back brings everything back as it stands in its file.
* **Nothing runs in the editor.** In the Development build the world stands
  still and no script is created. Use the play mode, or Shift and Enter for the
  simulation.
* **No engine.** Only what is in this file exists for a script.
* **Only `Main` runs by itself** in a normal scene. Every other script there is
  a tool that `Main`, or another script, calls. In a tileset scene every
  assigned script runs by itself, see below.

## Tileset scenes: tile scripts

In a tileset scene there is no `Main`. Instead you put a script on a tile,
and it runs there on its own: once per frame while the game runs, for every
tile it sits on. That is the place for doors, traps, signs and everything
else that happens when the player walks somewhere:

```cpp
// hub/Door.cpp
#include "hub/Door.h"

namespace hub {
    void Door::Update(float dt) {
        (void) dt;

        if (tile.HasPlayerEntered()) {
            map.SwitchScene("combat");
        }
    }
}
```

### In the editor

Right click a tile in the Development build. A small menu asks for
**Tiles** or **Scripts**: Tiles is the list of tiles as before, Scripts lists
the scripts of the scene.

| In the scripts menu | What it does |
| --- | --- |
| Click on a script | Opens it in the editor |
| `+New` | Creates a new script and puts it on the tile |
| Right click, **Assign** | Puts the script on the tile |
| Right click, **Unassign** | Takes it off the tile again |
| Right click, **Rename** / **Delete** | Renames or deletes the files, the tiles follow |

The arrow in the list marks the script of the tile, and every tile with a
script has a small blue corner at its top right. A tile has one script at
most, a script can sit on as many tiles as you like: every tile gets its own
copy, so a member of the script, e.g. a counter, belongs to one tile.

Which tile has which script is stored in the map, `assets/maps/<id>.txt`, as
comment lines below the rows. Column and row count from 0 at the top left:

```
# @script 3 5 Door
```

A new tile script looks like this. As always it only runs after the next
build, until then it is grey in the menu:

```cpp
// hub/Door.h
#pragma once

#include "TileScript.h"

namespace hub {
    class Door : public TileScript {
    public:
        void Update(float dt) override;
    };
}
```

A tile script can include and call other scripts of its scene like above,
e.g. a helper class that several tile scripts share.

### tile

Every tile script has the tile it sits on ready as `tile`:

| Call | Gives | Meaning |
| --- | --- | --- |
| `tile.GetColumn()` / `tile.GetRow()` | `int` | Where the tile is, counted from 0 at the top left |
| `tile.GetNumber()` | `int` | Its digit in the map, 0 for an empty tile |
| `tile.SetNumber(int)` | | Puts a different tile there, 0 to 9, e.g. an open door |
| `tile.GetName()` | `std::string` | Its name from the tile data, e.g. `"Grass"` |
| `tile.IsWalkable()` | `bool` | Can the player walk onto it? |
| `tile.IsPlayerOn()` | `bool` | Does the player stand on it? |
| `tile.HasPlayerEntered()` | `bool` | Did the player arrive on it in this frame? |
| `tile.HasPlayerLeft()` | `bool` | Did the player walk off it in this frame? |
| `tile.IsPlayerPushing()` | `bool` | Does the player walk against it without getting on it? |
| `tile.Exists()` | `bool` | Does the tile lie on the map? |

`IsPlayerOn` counts the tile the player stands on. While it walks, that is
still the tile it came from, until it has arrived on the next one.
`HasPlayerEntered` and `HasPlayerLeft` are true for exactly one frame per
visit, so a door does not switch the scene twice.

All four player calls only answer while the player is **ready**, see
`map.IsPlayerReady()` below. Otherwise they are `false`.

`IsPlayerPushing` is for tiles you cannot walk onto, e.g. a sign or a locked
door: it is true in every frame the player walks against it.

### map

The map around the tile, and the way out of the scene, is `map`:

| Call | Gives | Meaning |
| --- | --- | --- |
| `map.GetColumns()` / `map.GetRows()` | `int` | Size of the map in tiles |
| `map.GetTile(int column, int row)` | `Tile` | Any tile of the map, with every call of `tile` |
| `map.GetPlayerColumn()` / `map.GetPlayerRow()` | `int` | The tile the player stands on |
| `map.GetPlayerTile()` | `Tile` | The same as a `Tile` |
| `map.SwitchScene(std::string id)` | `bool` | Asks "Travel to ...?", then leaves for the scene with this id |
| `map.SwitchScene(std::string id, bool confirm)` | `bool` | The same, with `false` it leaves without asking |
| `map.IsPlayerReady()` | `bool` | May the player trigger tiles right now? |
| `map.SetPlayerReady(bool)` | | `false` holds the tiles back until the player walked on |

`SwitchScene` takes the id of a scene as it stands in
`assets/scenes/scenes.json`. An id nobody has does nothing and gives
`false`.

By default it asks first: a window says "Travel to Combat?" with the name
of the scene, and the world stands still until the player answers.
**Confirm** (or Enter) leaves the scene, **Cancel** (or ESC) stays. With
`SwitchScene("combat", false)` there is no question, the change happens
after this frame and the rest of the frame still runs.

```cpp
if (tile.HasPlayerEntered()) {
    map.SwitchScene("combat");          // asks first
}

if (tile.HasPlayerEntered()) {
    map.SwitchScene("combat", false);   // leaves right away
}
```

### Ready

The player is not always ready to trigger tiles. While it is not, `IsPlayerOn`,
`HasPlayerEntered`, `HasPlayerLeft` and `IsPlayerPushing` all answer `false`,
on every tile. It becomes ready again as soon as it arrives on another tile.

It is not ready:

* **At the start** of the scene, on the tile it starts on. A door there does
  not throw it out again right away.
* **After Cancel** in the travel question. It still stands on the door, and
  without this the door would ask again in the next frame, e.g. with
  `IsPlayerOn`. Walk off and back on to be asked again.
* **When a script says so** with `map.SetPlayerReady(false)`, e.g. after a
  sign was read. `map.SetPlayerReady(true)` lets the tiles through again
  right away.

A tile outside the map is **empty**, like an `Object` that does not exist:
reading gives 0, `false` or an empty text, and setting does nothing.

```cpp
// A lever: walking against it opens the gate two tiles to the right
if (tile.IsPlayerPushing()) {
    map.GetTile(tile.GetColumn() + 2, tile.GetRow()).SetNumber(3);
}
```

What a tile script changes is not saved either: `SetNumber` only changes the
map while the game runs, the file stays as it is.
