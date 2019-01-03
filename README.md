# LÖVE-Nuklear

[Nuklear](https://github.com/vurtun/nuklear) module for the [LÖVE](https://love2d.org/) game engine.

Provides a lightweight immediate mode GUI for LÖVE games.

## Example
```lua
-- Simple UI example.

local nuklear = require 'nuklear'

local ui

function love.load()
	ui = nuklear.newUI()
end

local combo = {value = 1, items = {'A', 'B', 'C'}}

function love.update(dt)
	ui:frameBegin()
	if ui:windowBegin('Simple Example', 100, 100, 200, 160,
			'border', 'title', 'movable') then
		ui:layoutRow('dynamic', 30, 1)
		ui:label('Hello, world!')
		ui:layoutRow('dynamic', 30, 2)
		ui:label('Combo box:')
		if ui:combobox(combo, combo.items) then
			print('Combo!', combo.items[combo.value])
		end
		ui:layoutRow('dynamic', 30, 3)
		ui:label('Buttons:')
		if ui:button('Sample') then
			print('Sample!')
		end
		if ui:button('Button') then
			print('Button!')
		end
	end
	ui:windowEnd()
	ui:frameEnd()
end

function love.draw()
	ui:draw()
end

function love.keypressed(key, scancode, isrepeat)
	ui:keypressed(key, scancode, isrepeat)
end

function love.keyreleased(key, scancode)
	ui:keyreleased(key, scancode)
end

function love.mousepressed(x, y, button, istouch, presses)
	ui:mousepressed(x, y, button, istouch, presses)
end

function love.mousereleased(x, y, button, istouch, presses)
	ui:mousereleased(x, y, button, istouch, presses)
end

function love.mousemoved(x, y, dx, dy, istouch)
	ui:mousemoved(x, y, dx, dy, istouch)
end

function love.textinput(text)
	ui:textinput(text)
end

function love.wheelmoved(x, y)
	ui:wheelmoved(x, y)
end
```

## Building

Windows binaries are available for each [release](https://github.com/keharriso/love-nuklear/releases).

To build the library yourself, grab the code with:
```sh
$ git clone --recursive git@github.com:keharriso/love-nuklear.git
```

Next, you need to compile the code to a native Lua module.

### Compiling with CMake on Linux

1. First, ensure you have the `cmake` and `luajit` packages installed.
2. Create a new folder next to `love-nuklear` called `love-nuklear-build`.
3. Open a terminal inside `love-nuklear-build`.
4. Compile the library with
```sh
$ cmake -DCMAKE_BUILD_TYPE=Release ../love-nuklear
$ make
```
7. Locate `nuklear.so` in the build folder.

### Compiling with CMake and MinGW on Windows

1. Install [CMake](https://cmake.org/download/) and [MinGW](http://mingw.org/) or [MinGW-w64](https://mingw-w64.org/doku.php).
2. Download the source code for [LuaJIT](http://luajit.org/download.html).
3. Open a command window inside the LuaJIT folder (the one that contains "README").
4. Compile LuaJIT with
```sh
$ mingw32-make
```
5. Remember the path to `lua51.dll` inside the LuaJIT `src` folder.
6. Run the CMake GUI.
7. Click "Browse Source" at the top right, then select the `love-nuklear` folder.
8. Enter a path for the build folder. It should be separate from the source folder.
9. Press "Configure" at the bottom.
10. Select "MinGW Makefiles" from the generator drop list, then click "Finish".
11. You should receive an error. This is normal.
12. Open the LUA tree by clicking the triangle on the left.
13. Replace "LUA_INCLUDE_DIR-NOTFOUND" with the path to the LuaJIT `src` folder.
14. Replace "LUA_LIBRARY-NOTFOUND" with the path to `lua51.dll` inside the LuaJIT `src` folder.
15. Click "Generate" at the bottom.
16. Open a command window inside the build folder.
17. Compile with
```sh
$ mingw32-make
```
18. Locate `nuklear.dll` inside the build folder.

## Documentation

A complete description of all functions and style properties, alongside additional examples, is available at the [LÖVE-Nuklear wiki](https://github.com/keharriso/love-nuklear/wiki).

## License

Copyright (c) 2016 Kevin Harrison, released under the MIT License (see LICENSE for details).
