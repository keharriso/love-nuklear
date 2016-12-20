-- Several simple examples.

local nuklear = require 'nuklear'

local calculator = require 'calculator'
local draw = require 'draw'
local overview = require 'overview'
local style = require 'style'
local skin = require 'skin'

local ui

function love.load()
	ui = nuklear.init()
end

function love.update(dt)
	ui:frameBegin()
		calculator(ui)
		style(ui)
		overview(ui)
		draw(ui)
		skin(ui)
	ui:frameEnd()
end

function love.draw()
	ui:draw()
	love.graphics.print("Current FPS: "..tostring(love.timer.getFPS( )), 10, 10)
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
