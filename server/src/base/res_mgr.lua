res_mgr = res_mgr or {}

if cc == nil then
	res_mgr.open = function (name)
		print('load file :', name)
		local file = io.open(name, 'r')
		if file == nil then
			return nil
		end
		local s = file:read('*a')
		file:close()
		return s
	end

	res_mgr.raw_open = res_mgr.open
else
	res_mgr.open = function (name)
		print('load file :', name)
		local fullpath = cc.FileUtils:getInstance():fullPathForFilename(name)
		return _extend.loadfile(fullpath)
    end


	res_mgr.raw_open = function (name)
		print('load file :', name)
		local fullpath = cc.FileUtils:getInstance():fullPathForFilename(name)
		return cc.FileUtils:getInstance():getStringFromFile(fullpath)
	end
end

function res_mgr.load_config( path )
	local content = res_mgr.raw_open( path )
	if not content then
		return nil
	end

	local loader, msg = loadstring( content, path )
	if not loader then
		print( path, msg )
		return nil
	end

	local result = {}
	setfenv( loader, result )

	local ret, msg = pcall( loader )
	if ret then
		return result
	end

	print( 'error loading config ', path )
	print( '\t', msg )
	return nil
end

function res_mgr.load_ini( path )
	local content = res_mgr.raw_open( path )
	local t = {}
	if not content then
		print( 'error open ini file', path )
		return t
	end

	local section
	local line_counter = 0
	lines = string_split(content,"\n")
	for _, fline in pairs(lines) do
		line_counter = line_counter + 1
		local line = fline:match("^%s*(.-)%s*$")
		if not line:match("^[%;#]") and #line > 0 then
			local s = line:match("^%[([%w%s]*)%]$")
			if s then
				section = s
				t[section] = t[section] or {}
			end
			local key, value = line:match("([^=]*)%=(.*)")
			if key and value then
				key = key:match("^%s*(%S*)%s*$")
				value = value:match("^%s*(.-)%s*$")
				if tonumber(value) then value = tonumber(value) end
				if value == "true" then value = true end
				if value == "false" then value = false end
				if section == nil then
					t[key] = value
				else
					t[section][key] = value
				end
			else
				print( 'error loading', path, 'at line', line_counter )
			end
		end
	end
	return t
end
