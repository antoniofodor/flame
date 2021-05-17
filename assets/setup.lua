function starts_with(s1, s2)
   return string.sub(s1, 1, string.len(s2))==s2
end

function dump(o)
   if type(o) == 'table' then
      local s = '{ '
      for k,v in pairs(o) do
         if type(k) ~= 'number' then k = '"'..k..'"' end
         s = s .. '['..k..'] = ' .. dump(v) .. ','
      end
      return s .. '} '
   else
      return tostring(o)
   end
end

function find_enum(n)
	local enum = enums[n]
	if (enum == nil) then
		enum = enums["flame::"..n]
		if (enum == nil) then
			return nil
		end
	end
	return enum
end

function find_udt(n)
	local udt = udts[n]
	if (udt == nil) then
		udt = udts["flame::"..n]
		if (udt == nil) then
			return nil
		end
	end
	return udt
end

callbacks = {}

function get_callback_slot(f)
	local s = math.random(0, 10000)
	if (callbacks[s] == nil) then
		callbacks[s] = {}
		callbacks[s].f = f
		return s
	end
	return get_callback_slot(f)
end

function make_obj(o, n)
	local udt = find_udt(n)
	if (udt == nil) then
		if o.p then print("script: cannot find udt "..n) end
		return
	end
	if udt.base ~= "" then
		make_obj(o, udt.base)
	end
	if not o.p then return end
	for k, vari in pairs(udt.variables) do
		local v = flame_get(o.p, vari.offset, vari.tag, vari.basic, vari.vec_size, vari.col_size)
		if type(v) == "userdata" then
			local vv = { p=v }
			make_obj(vv, vari.type)
			v = vv
		end
		o[k] = v
	end
	for k, func in pairs(udt.functions) do
		if func.type == "" then
			o[k] = function(...)
				return flame_call(o.p, func.f, {...})
			end
		else
			o[k] = function(...)
				__type__ = func.type
				local ret = {}
				ret.p = flame_call(o.p, func.f, {...})
				make_obj(ret, __type__)
				return ret
			end
		end
	end
	for k, func in pairs(udt.callbacks) do
		o[k] = function(f, ...)
			n = get_callback_slot(f)
			flame_call(o.p, func, { 0, n, ... })
			callbacks[n] = nil
		end
	end
	for k, func in pairs(udt.listeners) do
		o["add_"..k] = function(f, ...)
			n = get_callback_slot(f)
			callbacks[n].c = flame_call(o.p, func.add, { 0, n, ... })
			return n
		end
		o["remove_"..k] = function(n, ...)
			flame_call(o.p, func.remove, { callbacks[n].c, ...})
			callbacks[n] = nil
			return n
		end
	end
end

function vec2(x, y)
	if y == nil then y = x end
	local o = { x=x, y=y }
	o.push = function()
		return  o.x, o.y
	end
	setmetatable(o, {
		__unm = function(a)
			return vec2(-a.x, -a.y)
		end,
		__add = function(a, b)
			return vec2(a.x + b.x, a.y + b.y)
		end,
		__sub = function(a, b)
			return vec2(a.x - b.x, a.y - b.y)
		end,
		__mul = function(a, b)
			if type(b) == "table" then
				return vec2(a.x * b.x, a.y * b.y)
			end
			return vec2(a.x * b, a.y * b)
		end
	})
	return o
end

function vec3(x, y, z)
	if y == nil and z == nil then 
		y = x
		z = x
	end
	local o = { x=x, y=y, z=z }
	o.push = function()
		return  o.x, o.y, o.z
	end
	setmetatable(o, {
		__unm = function(a)
			return vec3(-a.x, -a.y, -a.z)
		end,
		__add = function(a, b)
			return vec3(a.x + b.x, a.y + b.y, a.z + b.z)
		end,
		__sub = function(a, b)
			return vec3(a.x - b.x, a.y - b.y, a.z - b.z)
		end,
		__mul = function(a, b)
			if type(b) == "table" then
				return vec3(a.x * b.x, a.y * b.y, a.z * b.z)
			end
			return vec3(a.x * b, a.y * b, a.z * b)
		end
	})
	return o
end

function distance_3(a, b)
	local x = a.x - b.x
	local y = a.y - b.y
	local z = a.z - b.z
	return math.sqrt(x * x + y * y + z * z)
end

function vec4(x, y, z, w)
	if y == nil and z == nil and w == nil then 
		y = x
		z = x
		w = x
	end
	local o = { x=x, y=y, z=z, w=w }
	o.push = function()
		return  o.x, o.y, o.z, o.w
	end
	setmetatable(o, {
		__unm = function(a)
			return vec4(-a.x, -a.y, -a.z, -a.w)
		end,
		__add = function(a, b)
			return vec4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w)
		end,
		__sub = function(a, b)
			return vec4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w)
		end,
		__mul = function(a, b)
			if type(b) == "table" then
				return vec4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w)
			end
			return vec4(a.x * b, a.y * b, a.z * b, a.w * b)
		end
	})
	return o
end

function split_by_newline(str)
	local lines = {}
	for s in str:gmatch("[^\r\n]+") do
		table.insert(lines, s)
	end
	return lines
end

function load_ini(fn)
	local file = flame_load_file(fn)
	local data = {}
	local section
	local lines = split_by_newline(file)
	for i, line in ipairs(lines) do
		local temp_section = line:match('^%[([%w]+)%]$')
		if temp_section then
			section = temp_section
			data[section] = {}
		else
			local param, value = line:match('^([%w|_]+)%s-=%s-(.+)$')
			if (param and value ~= nil) then
				if (tonumber(value)) then
					value = tonumber(value)
				elseif (value == 'true') then
					value = true
				elseif (value == 'false') then
					value = false
				end
				if (tonumber(param)) then
					param = tonumber(param)
				end
				data[section][param] = value
			end
		end
	end
	return data
end

function save_ini(fn, data)
	local contents = ""
	for section, param in pairs(data) do
		contents = contents .. ('[%s]\n'):format(section)
		for key, value in pairs(param) do
			contents = contents .. ('%s=%s\n'):format(key, tostring(value))
		end
		contents = contents .. '\n'
	end
	flame_save_file(fn, contents)
end
