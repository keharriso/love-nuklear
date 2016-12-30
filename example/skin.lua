-- Basic skinning example.

local windowHeader = love.graphics.newImage 'skin/window_header.png'
local checkboxSkin = love.graphics.newImage 'skin/checkbox_false.png'
local checkboxCheck = love.graphics.newImage 'skin/checkbox_true.png'

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
		['normal'] = checkboxSkin,
		['hover'] = checkboxSkin,
		['active'] = checkboxSkin,
		['cursor normal'] = checkboxCheck,
		['cursor hover'] = checkboxCheck,
		['text normal'] = '#000000',
		['text hover'] = '#000000',
		['text active'] = '#000000',
		['text background'] = '#d3ceaa'
	},
	['window'] = {
		['header'] = {
			['normal'] = windowHeader,
			['hover'] = windowHeader,
			['active'] = windowHeader,
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

return function (ui)
	ui:stylePush(style)
	if ui:windowBegin('Skin Example', 200, 200, 350, 200, 'title', 'movable') then
		ui:layoutSpaceBegin('dynamic', 150, 3)
		ui:layoutSpacePush(0.14, 0.15, 0.72, 0.3)
		ui:label('Skin example! Styles can change skins, colors, padding, font, and more.', 'wrap')
		ui:layoutSpacePush(0.2, 0.55, 0.2, 0.2)
		ui:button('Button')
		ui:layoutSpacePush(0.55, 0.55, 0.3, 0.2)
		ui:checkbox('Checkbox', check)
		ui:layoutSpaceEnd()
	end
	ui:windowEnd()
	ui:stylePop()
end
