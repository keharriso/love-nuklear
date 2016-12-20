# LÖVE-Nuklear

[Nuklear](https://github.com/vurtun/nuklear) module for the [LÖVE](https://love2d.org/) game engine.

Provides a lightweight immediate mode GUI for LÖVE games.

## Example
```lua
-- Simple UI example.

local nuklear = require 'nuklear'

local ui

function love.load()
	ui = nuklear.init()
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

function love.mousepressed(x, y, button, istouch)
	ui:mousepressed(x, y, button, istouch)
end

function love.mousereleased(x, y, button, istouch)
	ui:mousereleased(x, y, button, istouch)
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
