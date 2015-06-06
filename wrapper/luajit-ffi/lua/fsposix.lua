-- The file is in public domain
-- nyfair (nyfair2012@gmail.com)

local ffi = require 'ffi'
ffi.cdef[[
	int mkdir(const char*, int);
]]

function ls(pattern)
	local files = {}
	if pattern == nil then
		pattern = './*'
	end
	local output = assert(io.popen('ls '..pattern:gsub(' ', '\\ ')))
	for line in output:lines() do
		table.insert(files, line)
	end
	return files
end

function md(dst)
	ffi.C.mkdir(dst, 493) --0755
end

function rm(dst)
	os.remove(dst)
end

function mv(src, dst)
	os.rename(src, dst)
end
