-- Basic skinning example.

local windowHeader = love.graphics.newImage 'skin/window_header.png'
local windowBody = love.graphics.newImage 'skin/window.png'
local checkboxTexture = love.graphics.newImage 'skin/checkbox.png'
local checkboxOff = {checkboxTexture, love.graphics.newQuad(0, 0, 51, 55, 58, 115)}
local checkboxOn = {checkboxTexture, love.graphics.newQuad(0, 55, 58, 60, 58, 115)}
local buttonTexture = love.graphics.newImage 'skin/button.png'
local buttonActive = {buttonTexture, love.graphics.newQuad(0, 52, 69, 52, 69, 156)}
local buttonHover = {buttonTexture, love.graphics.newQuad(0, 104, 69, 52, 69, 156)}


-- last 4 args are 9-patch t/l/b/r. These are encoded as the number of pixels from the corresponding
-- edge (e.g. "4" for "r" means "4 pixels from the right edge", or w - 4).
local buttonNormal = {buttonTexture, love.graphics.newQuad(0, 0, 69, 52, 69, 156), 25, 12, 22, 12}

local style = {
	['text'] = {
		['color'] = '#000000'
	},
	['button'] = {
		['normal'] = buttonNormal,
		['hover'] = buttonHover,
		['active'] = buttonActive,
		['text background'] = '#00000000',
		['text normal'] = '#000000',
		['text hover'] = '#000000',
		['text active'] = '#ffffff'
	},
	['checkbox'] = {
		['normal'] = checkboxOff,
		['hover'] = checkboxOff,
		['active'] = checkboxOff,
		['cursor normal'] = checkboxOn,
		['cursor hover'] = checkboxOn,
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
		['fixed background'] = windowBody,
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
