-- The file is in public domain
-- nyfair (nyfair2012@gmail.com)

local ffi = require "ffi"
ffi.cdef[[
	int rename(const char*, const char*);
	int unlink(const char*);
	int mkdir(const char*, int);
	int rmdir(const char*);
]]

function ls(pattern)
end

function mv(src, dst, override)
	ffi.C.rename(src, dst)
end

function cp(src, dst, override)
end

function rm(dst)
	ffi.C.unlink(dst)
end

function md(dst)
	ffi.C.mkdir(dst, 493) --0755
end

function rd(dst)
	ffi.C.rmdir(dst)
end
