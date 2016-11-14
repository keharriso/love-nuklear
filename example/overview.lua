-- An overview of most of the supported widgets.

local nk = require 'nuklear'

local checkA = {value = false}
local checkB = {value = true}
local radio = {value = 'A'}
local selectA = {value = false}
local selectB = {value = true}
local slider = {value = 0.2}
local progress = {value = 1}
local colorPicker = {value = '#ff0000'}
local property = {value = 6}
local edit = {value = 'Edit text'}
local comboA = {value = 1, items = {'A', 'B', 'C'}}

return function ()
	if nk.window_begin('Overview', 100, 100, 600, 450, 'border', 'movable', 'title') then
		nk.menubar_begin()
		nk.layout_row('dynamic', 30, 1)
		if nk.menu_begin('Menu', nil, 120, 90) then
			nk.layout_row('dynamic', 40, 1)
			nk.menu_item('Item A')
			nk.menu_item('Item B')
			nk.menu_item('Item C')
			nk.menu_end()
		end
		nk.menubar_end()
		nk.layout_row('dynamic', 400, 3)
		nk.group_begin('Group 1', 'border')
			nk.layout_row('dynamic', 30, 1)
			nk.label('Left label')
			nk.label('Centered label', 'centered')
			nk.label('Right label', 'right')
			nk.label('Colored label', 'left', '#ff0000')
			if nk.tree_push('tab', 'Tree Tab') then
				if nk.tree_push('node', 'Tree Node 1') then
					nk.label('Label 1')
					nk.tree_pop()
				end
				if nk.tree_push('node', 'Tree Node 2') then
					nk.label('Label 2')
					nk.tree_pop()
				end
				nk.tree_pop()
			end
			nk.spacing(1)
			if nk.button('Button') then
				print('button pressed!')
			end
			nk.spacing(1)
			nk.checkbox('Checkbox A', checkA)
			nk.checkbox('Checkbox B', checkB)
		nk.group_end()
		nk.group_begin('Group 2', 'border')
			nk.layout_row('dynamic', 30, 1)
			nk.label('Radio buttons:')
			nk.layout_row('dynamic', 30, 3)
			nk.radio('A', radio)
			nk.radio('B', radio)
			nk.radio('C', radio)
			nk.layout_row('dynamic', 30, 1)
			nk.selectable('Selectable A', selectA)
			nk.selectable('Selectable B', selectB)
			nk.layout_row('dynamic', 30, {.35, .65})
			nk.label('Slider:')
			nk.slider(0, slider, 1, 0.05)
			nk.label('Progress:')
			nk.progress(progress, 10, true)
			nk.layout_row('dynamic', 30, 2)
			nk.spacing(2)
			nk.label('Color picker:')
			nk.button(nil, colorPicker.value)
			nk.layout_row('dynamic', 90, 1)
			nk.color_picker(colorPicker)
		nk.group_end()
		nk.group_begin('Group 3', 'border')
			nk.layout_row('dynamic', 30, 1)
			nk.property('Property', 0, property, 10, 0.25, 0.05)
			nk.spacing(1)
			nk.label('Edit:')
			nk.layout_row('dynamic', 90, 1)
			nk.edit('box', edit)
			nk.layout_row('dynamic', 5, 1)
			nk.spacing(1)
			nk.layout_row('dynamic', 30, 1)
			nk.label('Combobox:')
			nk.combobox(comboA, comboA.items)
			nk.layout_row('dynamic', 5, 1)
			nk.spacing(1)
			nk.layout_row('dynamic', 30, 1)
			if nk.widget_is_hovered() then
				nk.tooltip('Test tooltip')
			end
			local x, y, w, h = nk.widget_bounds()
			if nk.contextual_begin(100, 100, x, y, w, h) then
				nk.layout_row('dynamic', 30, 1)
				nk.contextual_item('Item A')
				nk.contextual_item('Item B')
				nk.contextual_end()
			end
			nk.label('Contextual (Right click me)')
		nk.group_end()
	end
	nk.window_end()
end
