-- The file is in public domain
-- nyfair (nyfair2012@gmail.com)

local ffi = require 'ffi'
ffi.cdef[[
	#pragma pack(push)
	#pragma pack(1)
	struct WIN32_FIND_DATAW {
		uint32_t dwFileWttributes;
		uint64_t ftCreationTime;
		uint64_t ftLastAccessTime;
		uint64_t ftLastWriteTime;
		uint32_t dwReserved[4];
		char cFileName[520];
		char cAlternateFileName[28];
	};
	#pragma pack(pop)
	
	void* FindFirstFileW(const char* pattern, struct WIN32_FIND_DATAW* fd);
	bool FindNextFileW(void* ff, struct WIN32_FIND_DATAW* fd);
	bool FindClose(void* ff);
	bool CreateDirectoryW(const char* dst, void*);
	bool MoveFileW(const char* src, const char* dst);
	bool DeleteFileW(const char* dst);
	
	int MultiByteToWideChar(unsigned int CodePage, uint32_t dwFlags, const char* lpMultiByteStr,
				int cbMultiByte, const char* lpWideCharStr, int cchWideChar);
	int WideCharToMultiByte(unsigned int CodePage, uint32_t dwFlags, const char* lpWideCharStr,
				int cchWideChar, const char* lpMultiByteStr, int cchMultiByte,
				const char* default, int* used);
]]

local WIN32_FIND_DATA = ffi.typeof('struct WIN32_FIND_DATAW')
local INVALID_HANDLE = ffi.cast('void*', -1)

function u2w(str)
	local size = ffi.C.MultiByteToWideChar(65001, 0, str, #str, nil, 0)
	local buf = ffi.new("char[?]", size * 2 + 2)
	ffi.C.MultiByteToWideChar(65001, 0, str, #str, buf, size * 2)
	return buf
end

function w2u(wstr)
	local size = ffi.C.WideCharToMultiByte(65001, 0, wstr, -1, nil, 0, nil, nil)
	local buf = ffi.new("char[?]", size + 1)
	size = ffi.C.WideCharToMultiByte(65001, 0, wstr, -1, buf, size, nil, nil)
	return buf
end

function ls(pattern)
	local fd = ffi.new(WIN32_FIND_DATA)
	local tFiles = {}
	if pattern == nil then
		pattern = './*'
	end
	local hFile = ffi.C.FindFirstFileW(u2w(pattern), fd)
	if hFile ~= INVALID_HANDLE then
	ffi.gc(hFile, ffi.C.FindClose)
	repeat
		fn = ffi.string(w2u(fd.cFileName))
		if fn ~= '.' and fn ~= '..' then
			table.insert(tFiles, fn)
		end
	until not ffi.C.FindNextFileW(hFile, fd)
	ffi.C.FindClose(ffi.gc(hFile, nil))
	end
	return tFiles
end

function md(dst)
	ffi.C.CreateDirectoryW(u2w(dst), nil)
end

function rm(dst)
	ffi.C.DeleteFileW(u2w(dst))
end

function mv(src, dst)
	ffi.C.MoveFileW(u2w(src), u2w(dst))
end
