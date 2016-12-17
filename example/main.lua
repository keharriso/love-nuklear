-- Several simple examples.

local nk = require 'nuklear'

local calculator = require 'calculator'
local draw = require 'draw'
local overview = require 'overview'
local style = require 'style'
local skin = require 'skin'

function love.load()
	nk.init()
end

function love.update(dt)
	nk.frameBegin()
		calculator()
		style()
		overview()
		draw()
		skin()
	nk.frameEnd()
end

function love.draw()
	nk.draw()
	love.graphics.print("Current FPS: "..tostring(love.timer.getFPS( )), 10, 10)
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
