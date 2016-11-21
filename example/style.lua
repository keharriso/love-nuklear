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

local color_names = {}

for name,_ in pairs(colors) do
	color_names[#color_names + 1] = name
end

table.sort(color_names)

return function ()
	nk.style_load_colors(colors)
	nk.window_begin('Style', 400, 50, 350, 450, 'border', 'movable', 'title', 'scrollbar')
		nk.layout_row('dynamic', 25, 2)
		for _,name in ipairs(color_names) do
			nk.label(name..':')
			local color = colors[name]
			if nk.combobox_begin(nil, color, 200, 200) then
				nk.layout_row('dynamic', 90, 1)
				color = nk.color_picker(color)
				colors[name] = color
				local r, g, b = nk.color_parse_rgba(color)
				nk.layout_row('dynamic', 25, {.25, .75})
				nk.label('R: '..r)
				r = nk.slider(0, r, 255, 1)
				nk.label('G: '..g)
				g = nk.slider(0, g, 255, 1)
				nk.label('B: '..b)
				b = nk.slider(0, b, 255, 1)
				colors[name] = nk.color_rgba(r, g, b)
				nk.combobox_end()
			end
		end
	nk.window_end()
end
