local PI = math.pi
local ANGLE = PI*1.0/50
local PRESS_A = 1
local SLOW_A = 1
local HIT_A = 5

function synthesis(tbl)
	local angle, value
	for _angle, _value in pairs(tbl) do
		if not angle then
			angle = _angle
			value = _value
		else
			local a = math.abs(angle - _angle)
			if a > PI then
				a = 2*PI - a
			end
			angle = a
			value = (value^2 + _value^2 + 2*value*_value*math.cos(a))^0.5
		end
	end
	if angle then
		return {[angle] = value}
	else
		return {}
	end
end

local function addv(vTbl, angle, value)
	if not vTbl[angle] then
		vTbl[angle] = 0
	end
	vTbl[angle] = vTbl[angle] + value
end

local function subv(vTbl, angle, value)
	if not vTbl[angle] then
		return
	end
	vTbl[angle] = vTbl[angle] - value
	if vTbl[angle] <= 0 then
		vTbl[angle] = nil
	end
end

function tick()
	local data = {}
	for vfd, client in pairs(clients) do
		client.vTbl = synthesis(client.vTbl)
		local angle, value = next(client.vTbl)

		if angle then
			print("vfd, x+, y+, angle, value:", vfd, value*math.cos(angle), value*math.sin(angle), angle, value)
			client.x = client.x + value*math.cos(angle)
			client.y = client.y + value*math.sin(angle)
		end

		if client.press then
			addv(client.vTbl, client.angle, PRESS_A)
		else
			client.angle = client.angle + ANGLE*client.d
		end
		if angle then
			if angle ~= client.angle then
				subv(client.vTbl, angle, SLOW_A)
			end
		end

		-- pack
		data[vfd] = {
			x = client.x,
			y = client.y,
			r = client.r,
			v = client.v,
			angle = client.angle/PI*180
		}
	end

	-- sync
	for vfd, client in pairs(clients) do
		client.call.sync(data)
	end

	-- hit
	local flag = {}
	for vfd, c1 in pairs(clients) do
		for _vfd, c2 in pairs(clients) do
			if vfd ~= _vfd and (not flag[_vfd] or not flag[_vfd][vfd]) then

				flag[vfd] = flag[vfd] or {}
				flag[vfd][_vfd] = true

				local len = c1.r + c2.r - ((c1.x-c2.x)^2 + (c1.y-c2.y)^2)^0.5
				if len > 0 then
					local a = math.abs(c1.y-c2.y)
					local b = math.abs(c1.x-c2.x)
					local _a
					if b == 0 then
						_a = PI/2
					else
						_a = math.atan(a/b)
					end

					if ((c1.y-c2.y)*(c1.x-c2.x)<0) then
						_a = PI - _a
					end
					--print("=== _a, len:", _a, len)
					if c1.y > c2.y then
						addv(c1.vTbl, _a, HIT_A)						
						addv(c2.vTbl, _a+PI, HIT_A)						
					else
						addv(c1.vTbl, _a+PI, HIT_A)						
						addv(c2.vTbl, _a, HIT_A)						
					end
				end
			end
		end
	end
	
	-- out
	for vfd, c in pairs(clients) do
		if c.x > 960/2 or c.x < -960/2 or c.y > 640/2 or c.y < -640/2 then
			c.x = 0
			c.y = 0
			c.vTbl = {}
		end
	end
end
