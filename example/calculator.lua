-- Simple calculator example, adapted from the original nuklear demo.

local nk = require 'nuklear'

local ops = {'+', '-', '*', '/'}
local a, b, op = '0'

local function clear()
	a, b, op = '0'
end

local function digit(d)
	if op then
		if b == nil or b == '0' then
			b = d
		else
			b = b..d
		end
	else
		if a == '0' then
			a = d
		else
			a = a..d
		end
	end
end

local function decimal()
	if op then
		b = b or '0'
		b = b:find('.') and b or b..'.'
	else
		a = a:find('.') and a or a..'.'
	end
end

local function equals()
	if not tonumber(b) then
		return
	end
	if op == '+' then
		a, b, op = tostring(tonumber(a) + tonumber(b))
	elseif op == '-' then
		a, b, op = tostring(tonumber(a) - tonumber(b))
	elseif op == '*' then
		a, b, op = tostring(tonumber(a) * tonumber(b))
	elseif op == '/' then
		a, b, op = tostring(tonumber(a) / tonumber(b))
	end
end

local function operator(o)
	if op then
		equals()
	end
	op = o
end

local function display()
	return b or a
end

return function ()
	if nk.window_begin('Calculator', 50, 50, 180, 250, 'border', 'movable', 'title') then
		nk.layout_row('dynamic', 35, 1)
		nk.label(display(), 'right')
		nk.layout_row('dynamic', 35, 4)
		for i=1,16 do
			if i >= 13 and i < 16 then
				if i == 13 then
					if nk.button('C') then
						clear()
					end
					if nk.button('0') then
						digit('0')
					end
					if nk.button('=') then
						equals()
					end
				end
			elseif i % 4 ~= 0 then
				local d = tostring(math.floor(i / 4) * 3 + (i % 4))
				if nk.button(d) then
					digit(d)
				end
			else
				local o = ops[math.floor(i / 4)]
				if nk.button(o) then
					operator(o)
				end
			end
		end
	end
	nk.window_end()
end
