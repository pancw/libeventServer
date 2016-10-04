package.path = package.path .. ';./src/msgpack/?.lua'

msgpack = require "MessagePack"
Net = require "NetLib"

require "src/base/import"
require "src/base/lua_class"
require "src/base/res_mgr"

local client = Import("src/client")
local battle_room = Import("src/module/battle_room")

mongo = require "mongo"
db = mongo.client { host = "localhost", port = 1238 }
local r = db:runCommand "listDatabases"
for k,v in ipairs(r.databases) do
	print(v.name)
end

clients = {}
function getClientByVfd(vfd)
	return clients[vfd]
end

function Tick ()
	battle_room.tick()
end

function HandleDisConnect(vfd)
	clients[vfd] = nil
end

function HandleConnect(vfd)
	clients[vfd] = client.client(vfd)
end

function HandleEvent(vfd, msg)
	local data = msgpack.unpack(msg)
	local method_name = data[1]

	local c = getClientByVfd(vfd)
	if c and type(c[method_name]) == "function" then
		c[method_name](c, table.unpack(data[2]))
	else
		print("call error.", vfd)
	end
end

function HandleShutDown()
	print("lua shutdown.")
end
