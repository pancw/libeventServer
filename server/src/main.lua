package.path = package.path .. ';./src/msgpack/?.lua'

local msgpack = require "MessagePack"
local Net = require "NetLib"
mongo = require "mongo"


db = mongo.client { host = "localhost", port = 1238 }
local r = db:runCommand "listDatabases"
for k,v in ipairs(r.databases) do
	print(v.name)
end

local clients = {}

function Tick ()
	--print("tick...")
	for vfd, _ in pairs(clients) do
		Net.send(vfd, msgpack.pack("A long test msg.[aaasdlkaslkaslkjklasjklsklajasdklasasdjkljklsadkldsaklsadjklasdasdklasdjklasdjklxxx]"))
	end
end

function HandleDisConnect(vfd)
	clients[vfd] = nil
end

function HandleConnect(vfd)
	clients[vfd] = true
end

function HandleEvent(vfd, msg)
	print("HandleEvent:", vfd, msg)
	local data = msgpack.unpack(msg)
	if type(data) == "table" then
		for k, v in pairs(data) do
			print(k, v)	
		end
	end
end


print("main.lua")
