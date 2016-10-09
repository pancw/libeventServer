local ANGLE = 4
function tick()
	local data = {}
	for vfd, client in pairs(clients) do
		local v = client.v

		if client.press then
			local a = client.angle/180*math.pi
			client.x = client.x + v*math.cos(a)
			client.y = client.y + v*math.sin(a)

			client.v = client.v + 1	
		else
			client.v = client.v - 1
			client.angle = (client.angle + ANGLE*client.d)%360
		end
		if client.v < 0 then
			client.v = 0
		end

		data[vfd] = {
			x = client.x,
			y = client.y,
			r = client.r,
			v = client.v,
			angle = client.angle,
		}
	end

	for vfd, client in pairs(clients) do
		client.call.sync(data)
	end

	for vfd, c1 in pairs(clients) do
		for _vfd, c2 in pairs(clients) do
			if vfd ~= _vfd then
				if (c1.x-c2.x)^2 + (c1.y-c2.y)^2 <= (c1.r + c2.r)^2 then
					c1.x = 100
					c1.y = 100
					c2.x = -100
					c2.y = -100
				end
			end
		end
	end
	
	for vfd, c in pairs(clients) do
		if c.x > 960/2 or c.x < -960/2 or c.y > 640/2 or c.y < -640/2 then
			c.x = 0
			c.y = 0
		end
	end
end
