local ANGLE = 3
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
end
