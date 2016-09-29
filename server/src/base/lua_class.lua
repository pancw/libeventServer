local function setup_super( type_obj, super )
	super = super or {}

	for k, v in pairs( super ) do
		if type_obj[ k ] == nil then
			type_obj[ k ] = v
		end
	end

	type_obj._super = super
end

function lua_class( name, super )
	local type_obj = { _name = name }
	type_obj.__index = type_obj

	local function create_object( fun, ... )
		local ins = {}
		setmetatable( ins, type_obj )

		local init = type_obj._init
		if init then
			init( ins, ... )
		end

		return ins
	end

	local type_metatable = { __call = create_object }

	setup_super( type_obj, super )

	setmetatable( type_obj, type_metatable )

	return type_obj
end

function super( type_obj, self )
	local function index( table, k )
		local super_ins = type_obj._super
		local fun = rawget( super_ins, k )
		if not fun then
			return nil
		end

		local function caller( ... )
			return fun( self, ... )
		end

		return caller
	end

	local result = {}
	setmetatable( result, { __index = index } )

	return result
end
