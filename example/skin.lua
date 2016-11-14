local nk = require 'nuklear'

local window_header = love.graphics.newImage 'skin/window_header.png'
local checkbox_skin = love.graphics.newImage 'skin/checkbox_false.png'
local checkbox_check = love.graphics.newImage 'skin/checkbox_true.png'

local style = {
	['text'] = {
		['color'] = '#000000'
	},
	['button'] = {
		['normal'] = love.graphics.newImage 'skin/button.png',
		['hover'] = love.graphics.newImage 'skin/button_hover.png',
		['active'] = love.graphics.newImage 'skin/button_active.png',
		['text background'] = '#00000000',
		['text normal'] = '#000000',
		['text hover'] = '#000000',
		['text active'] = '#ffffff'
	},
	['checkbox'] = {
		['normal'] = checkbox_skin,
		['hover'] = checkbox_skin,
		['active'] = checkbox_skin,
		['cursor normal'] = checkbox_check,
		['cursor hover'] = checkbox_check,
		['text normal'] = '#000000',
		['text hover'] = '#000000',
		['text active'] = '#000000',
		['text background'] = '#d3ceaa'
	},
	['window'] = {
		['header'] = {
			['normal'] = window_header,
			['hover'] = window_header,
			['active'] = window_header,
			['label normal'] = '#000000',
			['label hover'] = '#000000',
			['label active'] = '#000000',
			['label padding'] = {x = 10, y = 8}
		},
		['fixed background'] = love.graphics.newImage 'skin/window.png',
		['background'] = '#d3ceaa'
	}
}

local check = {value = false}

return function ()
	nk.style_push(style)
	if nk.window_begin('Skin Example', 200, 200, 350, 200, 'title', 'movable') then
		nk.layout_space_begin('dynamic', 150, 3)
		nk.layout_space_push(0.14, 0.15, 0.72, 0.3)
		nk.label('Skin example! Styles can change skins, colors, padding, font, and more.', 'wrap')
		nk.layout_space_push(0.2, 0.55, 0.2, 0.2)
		nk.button('Button')
		nk.layout_space_push(0.55, 0.55, 0.3, 0.2)
		nk.checkbox('Checkbox', check)
		nk.layout_space_end()
	end
	nk.window_end()
	nk.style_pop()
end
