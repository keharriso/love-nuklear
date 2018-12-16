-- Show off the optional closure-oriented versions of basic functions

local function menu(ui)
	ui:layoutRow('dynamic', 30, 2)
	ui:menu('Menu A', nil, 100, 200, function ()
		ui:layoutRow('dynamic', 30, 1)
		if ui:menuItem('Item 1') then
			print 'Closure: Item 1'
		end
		if ui:menuItem('Item 2') then
			print 'Closure: Item 2'
		end
	end)
	ui:menu('Menu B', nil, 100, 200, function ()
		ui:layoutRow('dynamic', 30, 1)
		if ui:menuItem('Item 3') then
			print 'Closure: Item 3'
		end
		if ui:menuItem('Item 4') then
			print 'Closure: Item 4'
		end
	end)
end

local comboText = 'Combo 1'

function combo(ui)
	ui:layoutRow('dynamic', 30, 1)
	if ui:comboboxItem('Combo 1') then
		print 'Closure: Combo 1'
		comboText = 'Combo 1'
	end
	if ui:comboboxItem('Combo 2') then
		print 'Closure: Combo 2'
		comboText = 'Combo 2'
	end
	if ui:comboboxItem('Combo 3') then
		print 'Closure: Combo 3'
		comboText = 'Combo 3'
	end
end

local function window(ui)
	ui:menubar(menu)
	ui:layoutRow('dynamic', 30, 1)
	ui:combobox(comboText, combo)
end

return function (ui)
	ui:window('Closure', 200, 200, 150, 120, {'title', 'movable', 'border'}, window)
end
