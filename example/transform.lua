-- Apply transformations to a basic UI.

return function(ui)
	local t = love.timer.getTime()
	ui:translate(350 + 100 * math.cos(t / 4), 350 + 100 * math.sin(t / 4))
	ui:rotate(t / 8)
	ui:scale(1 + math.sin(t / 4) / 2, 1 + math.cos(t / 4) / 2)
	ui:shear(math.cos(t / 8) / 4, math.sin(t / 8) / 4)
	if ui:windowBegin('Transform', 0, 0, 200, 200, 'border', 'title') then
		ui:layoutRow('dynamic', 150, 1)
		ui:label('You can apply transformations to the UI using ui:rotate, ui:scale, ui:shear, and ui:translate.', 'wrap')
	end
	ui:windowEnd()
end
