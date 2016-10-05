client = lua_class( 'client' )

function client:_init(vfd)
	self.vfd = vfd	
	
	self.call = {}
	setmetatable( self.call, {
		__index = function(t, method_name)
			local function caller( ... )
				Net.send(vfd, msgpack.pack({method_name, {...}}))
			end
			return caller
		end
	})

	--logic
	self.x = 0
	self.y = 0
	self.v = 0
	self.r = 'right'
	self.press = false
	self.angle = 90
	self.d = 1
end

function client:cmd(cmd)
	for k, v in pairs(cmd) do
		self[k] = v
	end
	if cmd.press then
		self.d = self.d * -1
	end
end
