-- Several simple examples.

local nuklear = require 'nuklear'

local calculator = require 'calculator'
local closure = require 'closure'
local draw = require 'draw'
local overview = require 'overview'
local style = require 'style'
local skin = require 'skin'
local template = require 'template'
local transform = require 'transform'

local ui1, ui2

function love.load()
	love.keyboard.setKeyRepeat(true)
	ui1, ui2 = nuklear.newUI(), nuklear.newUI()
end

function love.update(dt)
	ui1:frameBegin()
		calculator(ui1)
		style(ui1)
		closure(ui1)
		overview(ui1)
		draw(ui1)
		template(ui1)
		skin(ui1)
	ui1:frameEnd()
	ui2:frameBegin()
		transform(ui2)
	ui2:frameEnd()
end

function love.draw()
	ui1:draw()
	ui2:draw()
	love.graphics.print('Current FPS: '..tostring(love.timer.getFPS( )), 10, 10)
end

local function input(name, ...)
	return ui2[name](ui2, ...) or ui1[name](ui1, ...)
end

function love.keypressed(key, scancode, isrepeat)
	input('keypressed', key, scancode, isrepeat)
end

function love.keyreleased(key, scancode)
	input('keyreleased', key, scancode)
end

function love.mousepressed(x, y, button, istouch, presses)
	input('mousepressed', x, y, button, istouch, presses)
end

function love.mousereleased(x, y, button, istouch, presses)
	input('mousereleased', x, y, button, istouch, presses)
end

function love.mousemoved(x, y, dx, dy, istouch)
	input('mousemoved', x, y, dx, dy, istouch)
end

function love.textinput(text)
	input('textinput', text)
end

function love.wheelmoved(x, y)
	input('wheelmoved', x, y)
end
