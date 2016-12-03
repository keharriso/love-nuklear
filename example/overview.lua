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
	if nk.windowBegin('Overview', 100, 100, 600, 450, 'border', 'movable', 'title') then
		nk.menubarBegin()
		nk.layoutRow('dynamic', 30, 1)
		if nk.menuBegin('Menu', nil, 120, 90) then
			nk.layoutRow('dynamic', 40, 1)
			nk.menuItem('Item A')
			nk.menuItem('Item B')
			nk.menuItem('Item C')
			nk.menuEnd()
		end
		nk.menubarEnd()
		nk.layoutRow('dynamic', 400, 3)
		nk.groupBegin('Group 1', 'border')
			nk.layoutRow('dynamic', 30, 1)
			nk.label('Left label')
			nk.label('Centered label', 'centered')
			nk.label('Right label', 'right')
			nk.label('Colored label', 'left', '#ff0000')
			if nk.treePush('tab', 'Tree Tab') then
				if nk.treePush('node', 'Tree Node 1') then
					nk.label('Label 1')
					nk.treePop()
				end
				if nk.treePush('node', 'Tree Node 2') then
					nk.label('Label 2')
					nk.treePop()
				end
				nk.treePop()
			end
			nk.spacing(1)
			if nk.button('Button') then
				print('button pressed!')
			end
			nk.spacing(1)
			nk.checkbox('Checkbox A', checkA)
			nk.checkbox('Checkbox B', checkB)
		nk.groupEnd()
		nk.groupBegin('Group 2', 'border')
			nk.layoutRow('dynamic', 30, 1)
			nk.label('Radio buttons:')
			nk.layoutRow('dynamic', 30, 3)
			nk.radio('A', radio)
			nk.radio('B', radio)
			nk.radio('C', radio)
			nk.layoutRow('dynamic', 30, 1)
			nk.selectable('Selectable A', selectA)
			nk.selectable('Selectable B', selectB)
			nk.layoutRow('dynamic', 30, {.35, .65})
			nk.label('Slider:')
			nk.slider(0, slider, 1, 0.05)
			nk.label('Progress:')
			nk.progress(progress, 10, true)
			nk.layoutRow('dynamic', 30, 2)
			nk.spacing(2)
			nk.label('Color picker:')
			nk.button(nil, colorPicker.value)
			nk.layoutRow('dynamic', 90, 1)
			nk.colorPicker(colorPicker)
		nk.groupEnd()
		nk.groupBegin('Group 3', 'border')
			nk.layoutRow('dynamic', 30, 1)
			nk.property('Property', 0, property, 10, 0.25, 0.05)
			nk.spacing(1)
			nk.label('Edit:')
			nk.layoutRow('dynamic', 90, 1)
			nk.edit('box', edit)
			nk.layoutRow('dynamic', 5, 1)
			nk.spacing(1)
			nk.layoutRow('dynamic', 30, 1)
			nk.label('Combobox:')
			nk.combobox(comboA, comboA.items)
			nk.layoutRow('dynamic', 5, 1)
			nk.spacing(1)
			nk.layoutRow('dynamic', 30, 1)
			if nk.widgetIsHovered() then
				nk.tooltip('Test tooltip')
			end
			local x, y, w, h = nk.widgetBounds()
			if nk.contextualBegin(100, 100, x, y, w, h) then
				nk.layoutRow('dynamic', 30, 1)
				nk.contextualItem('Item A')
				nk.contextualItem('Item B')
				nk.contextualEnd()
			end
			nk.label('Contextual (Right click me)')
		nk.groupEnd()
	end
	nk.windowEnd()
end
