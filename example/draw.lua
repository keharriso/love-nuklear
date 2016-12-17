local nk = require "nuklear"

local img = love.graphics.newImage 'skin/button.png'

return function ()
	if nk.windowBegin('Draw Example', 300, 300, 200, 200, 'title', 'movable', 'border') then
		local x, y, w, h = nk.windowGetBounds()
		love.graphics.setColor(255, 0, 0)
		nk.line(x + 10, y + 40, x + 50, y + 40, x + 50, y + 80)
		nk.curve(x + 50, y + 80, x + 80, y + 40, x + 100, y + 80, x + 80, y + 80)
		nk.polygon('line', x + 100, y + 150, x + 60, y + 140, x + 70, y + 70)
		nk.circle('line', x + 130, y + 140, 50)
		nk.ellipse('fill', x + 30, y + 150, 20, 40)
		nk.arc('fill', x + 150, y + 80, 40, 3 * math.pi / 2, 2 * math.pi);
		nk.rectMultiColor(x + 95, y + 50, 50, 50, '#ff0000', '#00ff00', '#0000ff', '#000000')
		love.graphics.setColor(255, 255, 255)
		nk.image(img, x + 120, y + 120, 70, 50)
		nk.text('DRAW TEXT', x + 15, y + 75, 100, 100)
	end
	nk.windowEnd()
end
