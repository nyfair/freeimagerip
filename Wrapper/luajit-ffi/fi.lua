-- The file is in public domain
-- nyfair (nyfair2012@gmail.com)

require "fs"
local ffi = require "ffi"
local filua = ffi.load("freeimage")
ffi.cdef[[
	typedef struct { uint8_t r, g, b, a; } RGBA;
	
	void* __stdcall FreeImage_Load(int, const char*, int);
	int __stdcall FreeImage_Save(int, void*, const char*, int);
	void* __stdcall FreeImage_Clone(void*);
	void __stdcall FreeImage_Unload(void*);
	void* __stdcall FreeImage_Allocate(int, int, int,
																		unsigned, unsigned, unsigned);
	
	unsigned __stdcall FreeImage_GetBPP(void*);
	unsigned __stdcall FreeImage_GetWidth(void*);
	unsigned __stdcall FreeImage_GetHeight(void*);
	int __stdcall FreeImage_GetFIFFromFilename(const char*);
	int __stdcall FreeImage_GetFileType(const char*, int);
	
	void* __stdcall FreeImage_ConvertTo8Bits(void*);
	void* __stdcall FreeImage_ConvertToGreyscale(void*);
	void* __stdcall FreeImage_ConvertTo24Bits(void*);
	void* __stdcall FreeImage_ConvertTo32Bits(void*);
	
	void* __stdcall FreeImage_Rotate(void*, double, const void*);
	int __stdcall FreeImage_FlipHorizontal(void*);
	int __stdcall FreeImage_FlipVertical(void*);
	void* __stdcall FreeImage_Rescale(void*, int, int, int);
	int __stdcall FreeImage_JPEGTransform(const char*, const char*,
																				int, int);
	int __stdcall FreeImage_JPEGCrop(const char *, const char*,
																	int, int, int, int);
	
	void* __stdcall FreeImage_Copy(void*, int, int, int, int);
	int __stdcall FreeImage_Paste(void*, void*, int, int, int);
	void* __stdcall FreeImage_Composite(void*, int, RGBA*, void*);
]]

-- Image IO
-- get format id from image's filename
function getfmt(name)
	fmt = filua.FreeImage_GetFileType(name, 0)
	if fmt > -1 then
		return fmt
	else
		return filua.FreeImage_GetFIFFromFilename(name)
	end
end

-- open an image file
function open(name, flag)
	local fmt = getfmt(name)
	return filua.FreeImage_Load(fmt, name, flag or 0)
end

-- save image with given name
function save(img, name, flag)
	local fmt = getfmt(name)
	return filua.FreeImage_Save(fmt, img, name, flag or 0)
end

-- clone the image
function clone(img)
	return filua.FreeImage_Clone(img)
end

-- unload image to free memory
function free(img)
	filua.FreeImage_Unload(img)
end

-- create new image with give w,h,bpp
function newimg(width, height, bpp)
	filua.FreeImage_Allocate(width, height, bpp, 0, 0, 0)
end

-- Common Info
function getbpp(img)
	return filua.FreeImage_GetBPP(img)
end

function getw(img)
	return filua.FreeImage_GetWidth(img)
end

function geth(img)
	return filua.FreeImage_GetHeight(img)
end

function getResolution(img)
	local x = filua.FreeImage_GetDotsPerMeterX(img)
	local y = filua.FreeImage_GetDotsPerMeterY(img)
	return math.floor(x*0.0254+0.5), math.floor(y*0.0254+0.5)
end

function setResolution(img, x, y)
	filua.FreeImage_SetDotsPerMeterX(img, x/0.0254)
	filua.FreeImage_SetDotsPerMeterY(img, y/0.0254)
end

-- Composite
-- get part of image from given area
function copy(img, left, top, right, bottom)
	return filua.FreeImage_Copy(img, left, top, right, bottom)
end

-- paste a small image into a background image
function paste(back, front, left, top, alpha)
	return filua.FreeImage_Paste(back, front, left, top, alpha or 255)
end

-- alpha composite
function composite(front, usebg, rgba, back)
	return filua.FreeImage_Composite(front, usebg, rgba, back)
end

-- File-based process function
-- convert image format
function convert(src, dst, flag)
	if src:find("*") then
		for k,v in ipairs(dir(src)) do
			print(v)
			local img = open(v)
			save(img, stripext(v).."."..dst, flag)
			free(img)
		end
	else
		local img = open(src)
		save(img, dst, flag)
		free(img)
	end
end

-- convert bpp
function convbpp(src, bpp, dst, flag)
	if bpp==24 or bpp==32 or bpp==1 or bpp==8 then
		if src:find("*") then
			if dst == nil then
				dst = "bmp"
			end
			for k,v in ipairs(dir(src)) do
				print(v)
				local img = open(v)
				local out
				if bpp == 24 then out = filua.FreeImage_ConvertTo24Bits(img)
				elseif bpp == 32 then out = filua.FreeImage_ConvertTo32Bits(img)
				elseif bpp == 8 then out = filua.FreeImage_ConvertTo8Bits(img)
				else out = filua.FreeImage_ConvertToGreyscale(img)
				end
				save(out, stripext(v).."."..dst, flag)
				free(img)
				free(out)
			end
		else
			if dst == nil then
				dst = src
			end
			local img = open(src)
			local out
			if bpp == 24 then out = filua.FreeImage_ConvertTo24Bits(img)
			elseif bpp == 32 then out = filua.FreeImage_ConvertTo32Bits(img)
			elseif bpp == 8 then out = filua.FreeImage_ConvertTo8Bits(img)
			else out = filua.FreeImage_ConvertToGreyscale(img)
			end
			save(out, dst, flag)
			free(img)
			free(out)
		end
	end
end

function combine(back, front, dst, left, top, flag)
	local img1 = open(back)
	local img2 = open(front)
	paste(img1, img2, left, top)
	save(img1, dst, flag)
	free(img1)
	free(img2)
end

function combinealpha(back, front, dst, flag)
	local img1 = open(back)
	local img2 = open(front)
	local img2 = open(front)
	local img3 = composite(img2, 0, nil, img1)
	save(img3, dst, flag)
	free(img1)
	free(img2)
	free(img3)
end

function rotate(src, degree, dst, flag)
	if dst == nil then
		dst = stripext(src).."_rotate.bmp"
	end
	local img = open(src)
	local out = filua.FreeImage_Rotate(img, degree, nil)
	save(out, dst, flag)
	free(img)
	free(out)
end

function scale(src, width, height, filter, dst, flag)
	if dst == nil then
		dst = stripext(src).."_thumb.bmp"
	end
	local img = open(src)
	local out = filua.FreeImage_Rescale(img, width, height, filter or 0)
	save(out, dst, flag)
	free(img)
	free(out)
end

function fliph(src, dst, flag)
	if dst == nil then
		dst = stripext(src).."_fliph.bmp"
	end
	local img = open(src)
	filua.FreeImage_FlipHorizontal(img)
	save(img, dst, flag)
	free(img)
end

function flipv(src, dst, flag)
	if dst == nil then
		dst = stripext(src).."_flipv.bmp"
	end
	local img = open(src)
	filua.FreeImage_FlipVertical(img)
	save(img, dst, flag)
	free(img)
end

function jpgcrop(src, left, top, right, bottom, dst)
	if dst == nil then
		dst = stripext(src).."_crop.jpg"
	end
	filua.FreeImage_JPEGCrop(src, dst, left, top, right, bottom)
end

function jpgtran(src, func, dst, perfect)
	if dst == nil then
		dst = stripext(src).."_tran.jpg"
	end
	filua.FreeImage_JPEGTransform(src, dst, func, perfect or 0)
end
