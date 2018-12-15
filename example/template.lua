-- Show off the template row layout

return function(ui)
	if ui:windowBegin('Template Layout', 200, 100, 300, 200,
	                  'title', 'border', 'movable', 'scalable') then
		x, y, width, height = ui:windowGetContentRegion()
		ui:layoutRow('dynamic', 40, 1)
		ui:label('Scale me!');
		ui:layoutTemplateBegin(height - 40)
		ui:layoutTemplatePush('static', 75)
		ui:layoutTemplatePush('dynamic')
		ui:layoutTemplatePush('variable', 75)
		ui:layoutTemplateEnd()
		ui:button(nil, '#ff0000')
		ui:button(nil, '#00ff00')
		ui:button(nil, '#0000ff')
	end
	ui:windowEnd()
end
