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
end

function client:cmd(cmd)
	print("cmd:", cmd)
end
