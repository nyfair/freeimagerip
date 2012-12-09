-- The file is in public domain
-- nyfair (nyfair2012@gmail.com)

ffi = require 'ffi'
local filua = ffi.load('freeimage')
ffi.cdef[[
	typedef struct { uint8_t b, g, r, a; } RGBA;
	
	void* __stdcall FreeImage_Load(int, const char*, int);
	int __stdcall FreeImage_Save(int, void*, const char*, int);
	void* __stdcall FreeImage_Clone(void*);
	void __stdcall FreeImage_Unload(void*);
	void* __stdcall FreeImage_EnlargeCanvas(void*, int, int, int, int, RGBA*, int);
	void* __stdcall FreeImage_Allocate(int, int, int, unsigned, unsigned, unsigned);
	
	unsigned __stdcall FreeImage_GetBPP(void*);
	unsigned __stdcall FreeImage_GetWidth(void*);
	unsigned __stdcall FreeImage_GetHeight(void*);
	unsigned __stdcall FreeImage_GetDotsPerMeterX(void*);
	unsigned __stdcall FreeImage_GetDotsPerMeterY(void*);
	void __stdcall FreeImage_SetDotsPerMeterX(void*, unsigned);
	void __stdcall FreeImage_SetDotsPerMeterY(void*, unsigned);
	int __stdcall FreeImage_GetFIFFromFilename(const char*);
	int __stdcall FreeImage_GetFileType(const char*, int);
	
	void* __stdcall FreeImage_ConvertToGreyscale(void*);
	void* __stdcall FreeImage_ConvertTo24Bits(void*);
	void* __stdcall FreeImage_ConvertTo32Bits(void*);
	
	void* __stdcall FreeImage_Rotate(void*, double, RGBA*);
	int __stdcall FreeImage_FlipHorizontal(void*);
	int __stdcall FreeImage_FlipVertical(void*);
	void* __stdcall FreeImage_Rescale(void*, int, int, int);
	int __stdcall FreeImage_JPEGTransform(const char*, const char*, int, int);
	int __stdcall FreeImage_JPEGCrop(const char *, const char*, int, int, int, int);
	
	void* __stdcall FreeImage_Copy(void*, int, int, int, int);
	int __stdcall FreeImage_Paste(void*, void*, int, int, int);
	void* __stdcall FreeImage_Composite(void*, int, RGBA*, void*);
	int __stdcall FreeImage_GetPixelColor(void*, unsigned, unsigned, RGBA*);
	int __stdcall FreeImage_SetPixelColor(void*, unsigned, unsigned, RGBA*);
	void* __stdcall FreeImage_GetChannel(void*, int);
	int __stdcall FreeImage_SetChannel(void*, void*, int);
	int __stdcall FreeImage_SwapColors(void*, RGBA*, RGBA*, int);
	int __stdcall FreeImage_Invert(void*);
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

function open(name, flag)
	local fmt = getfmt(name)
	return filua.FreeImage_Load(fmt, name, flag or 0)
end

function save(img, name, flag)
	local fmt = getfmt(name)
	return filua.FreeImage_Save(fmt, img, name, flag or 0)
end

function clone(img)
	return filua.FreeImage_Clone(img)
end

function free(img)
	filua.FreeImage_Unload(img)
end

function color(r, g, b, a)
	local color = ffi.new('RGBA[?]', 1)
	color[0].b= b or 0
	color[0].g= g or 0
	color[0].r= r or 0
	color[0].a= a or 0
	return color
end

function enlarge(img, left, top, right, bottom, rgba)
	return filua.FreeImage_EnlargeCanvas(img, left, top, right, bottom, rgba or color(), 0)
end

function newimg(width, height, bpp, r, g, b)
	return filua.FreeImage_Allocate(width, height, bpp or 24, r or 0, g or 0, b or 0)
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

function getdpi(img)
	local x = filua.FreeImage_GetDotsPerMeterX(img)
	local y = filua.FreeImage_GetDotsPerMeterY(img)
	return math.floor(x*0.0254+0.5), math.floor(y*0.0254+0.5)
end

function setdpi(img, x, y)
	filua.FreeImage_SetDotsPerMeterX(img, x/0.0254)
	filua.FreeImage_SetDotsPerMeterY(img, y/0.0254)
end

function getpixel(img, x, y, rgba)
	filua.FreeImage_GetPixelColor(img, x, y, rgba)
end

function setpixel(img, x, y, rgba)
	filua.FreeImage_SetPixelColor(img, x, y, rgba)
end

function getchannel(img, channel)
	return filua.FreeImage_GetChannel(img, channel)
end

function setchannel(dst, src, channel)
	return filua.FreeImage_SetChannel(dst, src, channel)
end

function swapcolor(img, fromcolor, tocolor)
	filua.FreeImage_SwapColors(img, fromcolor or color(0,0,0,0), tocolor or color(0,0,0,255), 0)
end

function invert(img)
	filua.FreeImage_Invert(img)
end

function greyalpha(back, front, channel)
	b = to32(back)
	f = getchannel(front, channel or 1)
	setchannel(b, f, 4)
	free(f)
	return b
end

-- Composite
function copy(img, left, top, right, bottom)
	return filua.FreeImage_Copy(img, left, top, right, bottom)
end

function paste(back, front, left, top, alpha)
	filua.FreeImage_Paste(back, front, left, top, alpha or 255)
end

-- alpha composite
function composite(back, front)
	return filua.FreeImage_Composite(front, 0, nil, back)
end

function to8(src)
	return filua.FreeImage_ConvertToGreyscale(src)
end

function to24(src)
	return filua.FreeImage_ConvertTo24Bits(src)
end

function to32(src)
	return filua.FreeImage_ConvertTo32Bits(src)
end

-- File-based process function
function convert(src, dst, flag)
	if src:find('*') then
		for k,v in ipairs(ls(src)) do
			print(v)
			local img = open(v)
			save(img, stripext(v)..'.'..dst, flag)
			free(img)
		end
	else
		local img = open(src)
		save(img, dst, flag)
		free(img)
	end
end

function convbpp(src, bpp, dst, flag)
	if bpp==24 or bpp==32 or bpp==8 then
		if src:find('*') then
			if dst == nil then
				dst = 'bmp'
			end
			for k,v in ipairs(ls(src)) do
				print(v)
				local img = open(v)
				local out
				if bpp == 24 then out = to24(img)
				elseif bpp == 32 then out = to32(img)
				else out = to8(img)
				end
				save(out, stripext(v)..'.'..dst, flag)
				free(img)
				free(out)
			end
		else
			if dst == nil then
				dst = src
			end
			local img = open(src)
			local out
			if bpp == 24 then out = to24(img)
			elseif bpp == 32 then out = to32(img)
			else out = to8(img)
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
	local img3 = composite(img2, img1)
	save(img3, dst, flag)
	free(img1)
	free(img2)
	free(img3)
end

function rotate(src, degree, dst, flag, rgba)
	if dst == nil then
		dst = stripext(src)..'_rotate.bmp'
	end
	local img = open(src)
	local out = filua.FreeImage_Rotate(img, degree, rgba or color())
	save(out, dst, flag)
	free(img)
	free(out)
end

function scale(src, width, height, filter, dst, flag)
	if dst == nil then
		dst = stripext(src)..'_thumb.bmp'
	end
	local img = open(src)
	local out = filua.FreeImage_Rescale(img, width, height, filter or 5)
	save(out, dst, flag)
	free(img)
	free(out)
end

function fliph(src, dst, flag)
	if dst == nil then
		dst = stripext(src)..'_fliph.bmp'
	end
	local img = open(src)
	filua.FreeImage_FlipHorizontal(img)
	save(img, dst, flag)
	free(img)
end

function flipv(src, dst, flag)
	if dst == nil then
		dst = stripext(src)..'_flipv.bmp'
	end
	local img = open(src)
	filua.FreeImage_FlipVertical(img)
	save(img, dst, flag)
	free(img)
end

function jpgcrop(src, left, top, right, bottom, dst)
	if dst == nil then
		dst = stripext(src)..'_crop.jpg'
	end
	filua.FreeImage_JPEGCrop(src, dst, left, top, right, bottom)
end

function jpgtran(src, func, dst, perfect)
	if dst == nil then
		dst = stripext(src)..'_tran.jpg'
	end
	filua.FreeImage_JPEGTransform(src, dst, func, perfect or 0)
end

-- helper
if ffi.os == 'Windows' then
	require 'fswin'
else
	require 'fsposix'
end

function rm(dst)
	os.rm(dst)
end

function mv(src, dst)
	os.mv(src, dst)
end

function stripext(fn)
	local idx = fn:match('.+()%..+$')
	if idx then
		return fn:sub(1, idx - 1)
	else
		return fn
	end
end
