-- Show off some of the styling options.

local nuklear = require "nuklear"

local colors = {
	['text'] = '#afafaf',
	['window'] = '#2d2d2d',
	['header'] = '#282828',
	['border'] = '#414141',
	['button'] = '#323232',
	['button hover'] = '#282828',
	['button active'] = '#232323',
	['toggle'] = '#646464',
	['toggle hover'] = '#787878',
	['toggle cursor'] = '#2d2d2d',
	['select'] = '#2d2d2d',
	['select active'] = '#232323',
	['slider'] = '#262626',
	['slider cursor'] = '#646464',
	['slider cursor hover'] = '#787878',
	['slider cursor active'] = '#969696',
	['property'] = '#262626',
	['edit'] = '#262626',
	['edit cursor'] = '#afafaf',
	['combo'] = '#2d2d2d',
	['chart'] = '#787878',
	['chart color'] = '#2d2d2d',
	['chart color highlight'] = '#ff0000',
	['scrollbar'] = '#282828',
	['scrollbar cursor'] = '#646464',
	['scrollbar cursor hover'] = '#787878',
	['scrollbar cursor active'] = '#969696',
	['tab header'] = '#282828'
}

local colorNames = {}

for name,_ in pairs(colors) do
	colorNames[#colorNames + 1] = name
end

table.sort(colorNames)

return function (ui)
	ui:styleLoadColors(colors)
	if ui:windowBegin('Style', 400, 50, 350, 450, 'border', 'movable', 'title', 'scrollbar') then
		ui:layoutRow('dynamic', 25, 2)
		for _,name in ipairs(colorNames) do
			ui:label(name..':')
			local color = colors[name]
			if ui:comboboxBegin(nil, color, 200, 200) then
				ui:layoutRow('dynamic', 90, 1)
				color = ui:colorPicker(color)
				colors[name] = color
				local r, g, b = nuklear.colorParseRGBA(color)
				ui:layoutRow('dynamic', 25, {.25, .75})
				ui:label('R: '..r)
				r = ui:slider(0, r, 255, 1)
				ui:label('G: '..g)
				g = ui:slider(0, g, 255, 1)
				ui:label('B: '..b)
				b = ui:slider(0, b, 255, 1)
				colors[name] = nuklear.colorRGBA(r, g, b)
				ui:comboboxEnd()
			end
		end
	end
	ui:windowEnd()
end
