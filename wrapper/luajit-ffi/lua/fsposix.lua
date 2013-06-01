-- The file is in public domain
-- nyfair (nyfair2012@gmail.com)

local ffi = require 'ffi'
ffi.cdef[[
	int mkdir(const char*, int);
]]

function ls(pattern)
	local files = {}
	local output = assert(io.popen('ls '..pattern:gsub(' ', '\\ ')))
	for line in output:lines() do
		table.insert(files, line)
	end
	return files
end

function cp(src, dst)
	os.execute('cp '..src:gsub(' ', '\\ ')..' '..dst:gsub(' ', '\\ '))
end

function md(dst)
	ffi.C.mkdir(dst, 493) --0755
end

function rd(dst)
	os.execute('rm -rf '..normalize(dst:gsub(' ', '\\ ')))
end
