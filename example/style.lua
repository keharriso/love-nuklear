-- Show off some of the styling options.

local nk = require 'nuklear'

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

return function ()
	nk.styleLoadColors(colors)
	nk.windowBegin('Style', 400, 50, 350, 450, 'border', 'movable', 'title', 'scrollbar')
		nk.layoutRow('dynamic', 25, 2)
		for _,name in ipairs(colorNames) do
			nk.label(name..':')
			local color = colors[name]
			if nk.comboboxBegin(nil, color, 200, 200) then
				nk.layoutRow('dynamic', 90, 1)
				color = nk.colorPicker(color)
				colors[name] = color
				local r, g, b = nk.colorParseRGBA(color)
				nk.layoutRow('dynamic', 25, {.25, .75})
				nk.label('R: '..r)
				r = nk.slider(0, r, 255, 1)
				nk.label('G: '..g)
				g = nk.slider(0, g, 255, 1)
				nk.label('B: '..b)
				b = nk.slider(0, b, 255, 1)
				colors[name] = nk.colorRGBA(r, g, b)
				nk.comboboxEnd()
			end
		end
	nk.windowEnd()
end
