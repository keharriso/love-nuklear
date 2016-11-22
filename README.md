# LÖVE-Nuklear

[Nuklear](https://github.com/vurtun/nuklear) module for the [LÖVE](https://love2d.org/) game engine.

Provides a lightweight immediate mode GUI for LÖVE games.

## Example
```lua
-- Simple UI example.

local nk = require 'nuklear'

function love.load()
	nk.init()
end

local combo = {value = 1, items = {'A', 'B', 'C'}}

function love.update(dt)
	nk.frame_begin()
	if nk.window_begin('Simple Example', 100, 100, 200, 160,
			'border', 'title', 'movable') then
		nk.layout_row('dynamic', 30, 1)
		nk.label('Hello, world!')
		nk.layout_row('dynamic', 30, 2)
		nk.label('Combo box:')
		if nk.combobox(combo, combo.items) then
			print('Combo!', combo.items[combo.value])
		end
		nk.layout_row('dynamic', 30, 3)
		nk.label('Buttons:')
		if nk.button('Sample') then
			print('Sample!')
		end
		if nk.button('Button') then
			print('Button!')
		end
	end
	nk.window_end()
	nk.frame_end()
end

function love.draw()
	nk.draw()
end

function love.keypressed(key, scancode, isrepeat)
	nk.keypressed(key, scancode, isrepeat)
end

function love.keyreleased(key, scancode)
	nk.keyreleased(key, scancode)
end

function love.mousepressed(x, y, button, istouch)
	nk.mousepressed(x, y, button, istouch)
end

function love.mousereleased(x, y, button, istouch)
	nk.mousereleased(x, y, button, istouch)
end

function love.mousemoved(x, y, dx, dy, istouch)
	nk.mousemoved(x, y, dx, dy, istouch)
end

function love.textinput(text)
	nk.textinput(text)
end

function love.wheelmoved(x, y)
	nk.wheelmoved(x, y)
end
```

## Building

Windows and Linux binaries are available for each [release](https://github.com/keharriso/love-nuklear/releases).

To build the library yourself, grab the code with:
```sh
$ git clone --recursive git@github.com:keharriso/love-nuklear.git
```

Compile with CMake (I recommend using the MinGW generator on Windows). You'll need to tell CMake where to find the LuaJIT headers and binaries. The end result is a native Lua module.

## Documentation

A complete description of all functions and style properties, alongside additional examples, is available at the [LÖVE-Nuklear wiki](https://github.com/keharriso/love-nuklear/wiki).

## License

Copyright (c) 2016 Kevin Harrison, released under the MIT License (see LICENSE for details).

