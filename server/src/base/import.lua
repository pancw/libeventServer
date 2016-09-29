sys = {}
sys.modules = {}
loadstring = load

setfenv = function (func, env)
	local _ENV = env
	return func
end

local script_path = ''
local module_suffix = '.lua'
local encrypted_module_suffix = '.luae'
local _pri_modules = { "world.scene", "world.layer", "world.entity", "game_logic.battle", "world.char"}

--[[
为了保证代码尽量能够被reload到，要求模块尽量用import的方式引用，但是以下几点要特别注意：
1.本功能只reload并替换模块内部的函数，不替换数据，如果需要替换数据，可以在hook里边自己手动做，hook为_before_reload和_after_reload
2.模块内定义的全局函数，不能直接引用外部变量，否则在reload的时候全局函数直接被替换了，但是所引用的外部变量也会被替换，无法保持使用旧的数据
3.模块内定义的全局变量，如果是常量，是没关系的，但是如果会在运行时改变，是不能定义成全局变量的，这种全局变量只能放在require的模块中定义
]]--

local function module_to_path( name )
	if cc and type(_G.is_devmod) == 'function' and is_devmod() then
		local module_path = script_path .. string.gsub(name, '%.', '/')
		if cc.FileUtils:getInstance():isFileExist(module_path .. module_suffix) then
			return module_path .. module_suffix
		else
			return module_path .. encrypted_module_suffix
		end
	else
		return script_path .. string.gsub( name, '%.', '/' ) .. module_suffix
	end
end

local function create_module( name )
	local m = {}
	setmetatable( m, { __index = _G } )

	local file_name = module_to_path( name )
	local code_string = res_mgr.open( file_name )
	if not code_string then
		error( 'import error: can not open file: ' .. file_name )
		return false
	end

	local fun, msg = loadstring( code_string, file_name )
	if not fun then
		print( msg )
		error( 'import error: can not load the module:' .. name)
		return false
	end
	return m, fun

end

function Import( name )
	local name = string.gsub(name, '/', '.')
	local m = sys.modules[name]
	if m ~= nil then
		return m
	end

	local m, fun = create_module( name )
	sys.modules[name] = m
	setfenv( fun, m )()

	return m
end

function load_module( name )
	local m, fun = create_module( name )
	setfenv( fun, m )()
	return m
end

local function replace_func(old, new)
	assert(type(old) == "table" and type(new) == "table", "table required")

	for k, v in pairs(new) do
		local new_type = type(v)
		if rawget(old, k) == nil and new_type ~= 'function' then
			old[k] = v
		end
	end
	
	for k, _ in pairs(old) do
		local new_type = type(new[k])
		local old_type = type(old[k])
		if (new_type == "function" or new_type == "table") and (new_type ~= old_type) then
			-- new data is not a table while old data is a table
			local info = debug.getinfo(2)
			print("Warning different type replaced", old_type, new_type, tostring(k))
		elseif new_type == "function" then
			new[k] = setfenv(new[k], getfenv(old[k]))
			old[k] = new[k]
		elseif new_type == "table" then
			if k ~= "__index" and k ~= "_super" then
				replace_func(old[k], new[k])
			end
		end
	end

	-- add
	for k,v in pairs(new) do
		-- rawget requied!!!!
		if rawget(old, k) == nil then
			if type(v) == 'function' then 
				--让old的元表的元方法指向_G
				local mt = getmetatable(old) 
				if mt then
					mt.__index = _G
				end
				new[k] = setfenv(v, old)
			end
			old[k] = v
		end
	end
end

local function reload_error(msg)
	print("----------------------------------------")
	print("LUA ERROR: " .. tostring(msg) .. "\n")
	print(debug.traceback())
	print("----------------------------------------")
end

function reload_module(name)
	local old_module = sys.modules[name]
	local st, new_module = xpcall(function() return load_module(name)end, reload_error)
	print("reload module", name, st, old_module, new_module)
	if not st then
		return false
	end
	if old_module == nil then
		sys.modules[name] = new_module
	else
		if rawget(old_module, '_before_reload') then
			xpcall(old_module._before_reload, reload_error)
		end
		replace_func(old_module, new_module)
		if rawget(old_module, '_after_reload') then
			xpcall(old_module._after_reload, reload_error)
		end
	end
	return true
end

function reload_script()
	print('engine', '-----------reload script begin---------------')
	local reloaded_modules = {}
	for _, name in ipairs(_pri_modules) do
		if sys.modules[name] then
			reload_module( name )
			reloaded_modules[name] = true
		end
	end
	for name, _ in pairs(sys.modules) do
		if not reloaded_modules[name] then
			reload_module(name)
		end
	end
	print('engine', '-----------reload script finish--------------')
end

function reload_script_old()
	print('engine', '-----------reloadscript_begin---------------')

	--clean
	local old_modules = sys.modules
	local new_modules = {}

	--import new
	local ok = true
	local function _reload_all()
		for name, old_module in pairs(old_modules) do
			new_modules[name] = load_module(name)
			if new_modules[name] == nil then
				ok = false
				break
			end
		end
	end

	local function reload_error(msg)
		print("----------------------------------------")
		print("LUA ERROR: " .. tostring(msg) .. "\n")
		print(debug.traceback())
		print("----------------------------------------")
		ok = false
	end
	xpcall(_reload_all, reload_error)

	--roll back
	if ok == false then
		sys.modules = old_modules
		print('engine', '-----------reload_error---------------')
		return false
	end

	--replace_func
	for name, _ in pairs(new_modules) do
		if old_modules[name] == nil then
			old_modules[name] = new_modules[name]
		else
			replace_func(old_modules[name], new_modules[name])
		end
	end

	print('engine', '-----------reloadscript_finish--------------')

	return true
end
