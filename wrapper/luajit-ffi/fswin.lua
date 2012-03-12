-- The file is in public domain
-- nyfair (nyfair2012@gmail.com)

local ffi = require "ffi"
ffi.cdef[[
	#pragma pack(push)
	#pragma pack(1)
	struct WIN32_FIND_DATAA {
		uint32_t dwFileWttributes;
		uint64_t ftCreationTime;
		uint64_t ftLastAccessTime;
		uint64_t ftLastWriteTime;
		struct {
			union {
				uint64_t packed;
				struct {
					uint32_t high;
					uint32_t low;
				};
			};
		} nFileSize;
		uint32_t dwReserved[2];
		char cFileName[260];
		char cAlternateFileName[14];
	};
	#pragma pack(pop)
	
	void* __stdcall FindFirstFileA(const char* pattern, struct WIN32_FIND_DATAA* fd);
	bool __stdcall FindNextFileA(void* ff, struct WIN32_FIND_DATAA* fd);
	bool __stdcall FindClose(void* ff);
	bool __stdcall MoveFileExA(const char* src, const char* dst, uint32_t flag);
	bool __stdcall CopyFileA(const char* src, const char* dst, bool flag);
	bool __stdcall DeleteFileA(const char* dst);
	bool __stdcall CreateDirectoryA(const char* dst, void*);
	bool __stdcall RemoveDirectoryA(const char* dst);
]]

local WIN32_FIND_DATA = ffi.typeof("struct WIN32_FIND_DATAA")
local INVALID_HANDLE = ffi.cast("void*", -1)
function dir(pattern)
	local fd = ffi.new(WIN32_FIND_DATA)
	local tFiles = {}
	if pattern == nil then
		pattern = "./*"
	end
	local hFile = ffi.C.FindFirstFileA(pattern, fd)
	if hFile ~= INVALID_HANDLE then
	ffi.gc(hFile, ffi.C.FindClose)
	repeat
		fd.nFileSize.low, fd.nFileSize.high = fd.nFileSize.high, fd.nFileSize.low
		fn = ffi.string(fd.cFileName)
		if fn ~= "." and fn ~= ".." then
			table.insert(tFiles, fn)
		end
	until not ffi.C.FindNextFileA(hFile, fd)
	ffi.C.FindClose(ffi.gc(hFile, nil))
	end
	return tFiles
end

function mv(src, dst, override)
	if override == nil or override then
		flag = 3
	else flag = 2
	end
	ffi.C.MoveFileExA(src, dst, flag)
end

function cp(src, dst, override)
	ffi.C.CopyFileA(src, dst, not((override == nil or override)))
end

function rm(dst)
	ffi.C.DeleteFileA(dst)
end

function md(dst)
	ffi.C.CreateDirectoryA(dst, nil)
end

function rd(dst)
	ffi.C.RemoveDirectoryA(dst)
end
