function tick()
	local data = {}
	for vfd, client in pairs(clients) do
		local v = client.v
		if client.r == "right" then
			client.x = client.x + v
		elseif client.r == "left" then
			client.x = client.x - v
		elseif client.r == "up" then
			client.y = client.y + v
		elseif client.r == "down" then
			client.y = client.y - v
		end

		if client.press then
			client.v = client.v + 1	
		else
			client.v = client.v - 1
		end
		if client.v < 0 then
			client.v = 0
		end

		data[vfd] = {
			x = client.x,
			y = client.y,
			r = client.r,
			v = client.v,
		}
	end

	for vfd, client in pairs(clients) do
		client.call.sync(data)
	end
end
