-- The file is in public domain
-- nyfair (nyfair2012@gmail.com)

local ffi = require 'ffi'
local filua = ffi.load('freeimage')
ffi.cdef[[
	typedef struct { uint8_t r, g, b, a; } RGBA;
	typedef struct { uint8_t r, g, b; } RGB;
	
	void* __stdcall FreeImage_Load(int, const char*, int);
	int __stdcall FreeImage_Save(int, void*, const char*, int);
	void* __stdcall FreeImage_Clone(void*);
	void __stdcall FreeImage_Unload(void*);
	void* __stdcall FreeImage_Allocate(int, int, int, unsigned, unsigned, unsigned);
	
	unsigned __stdcall FreeImage_GetBPP(void*);
	unsigned __stdcall FreeImage_GetWidth(void*);
	unsigned __stdcall FreeImage_GetHeight(void*);
	unsigned __stdcall FreeImage_GetDotsPerMeterX(void*);
	unsigned __stdcall FreeImage_GetDotsPerMeterY(void*);
	void __stdcall FreeImage_SetDotsPerMeterX(void*, unsigned);
	void __stdcall FreeImage_SetDotsPerMeterY(void*, unsigned);
	const char* __stdcall FreeImage_GetFormatFromFIF(int);
	const char* __stdcall FreeImage_GetFIFExtensionList(int);
	const char* __stdcall FreeImage_GetFIFDescription(int);
	const char* __stdcall FreeImage_GetFIFRegExpr(int);
	const char* __stdcall FreeImage_GetFIFMimeType(int);
	int __stdcall FreeImage_GetFIFFromFilename(const char*);
	int __stdcall FreeImage_GetFIFCount();
	
	void* __stdcall FreeImage_ConvertTo4Bits(void*);
	void* __stdcall FreeImage_ConvertTo8Bits(void*);
	void* __stdcall FreeImage_ConvertToGreyscale(void*);
	void* __stdcall FreeImage_ConvertTo24Bits(void*);
	void* __stdcall FreeImage_ConvertTo32Bits(void*);
	void* __stdcall FreeImage_ColorQuantize(void*, int);
	void* __stdcall FreeImage_ConvertToRGBF(void*);
	void* __stdcall FreeImage_ConvertToUINT16(void*);
	void* __stdcall FreeImage_ToneMapping(void*, int, double, double);
	
	void* __stdcall FreeImage_Rotate(void*, double, const void*);
	int __stdcall FreeImage_FlipHorizontal(void*);
	int __stdcall FreeImage_FlipVertical(void*);
	void*	__stdcall FreeImage_Rescale(void*, int, int, int);
	int __stdcall FreeImage_JPEGTransform(const char*, const char*, int, int);
	int __stdcall FreeImage_JPEGCrop(const char *, const char*, int, int, int, int);
	
	void* __stdcall FreeImage_Copy(void*, int, int, int, int);
	int  __stdcall FreeImage_Paste(void*, void*, int, int, int);
	void* __stdcall FreeImage_Composite(void*, int, RGBA*, void*);
]]

-- Common Info
function getFIFFromFileName(name)
	return filua.FreeImage_GetFIFFromFilename(name)
end

function getFIFCount()
	return filua.FreeImage_GetFIFCount()
end

function getFormatFromFIF(fif)
	return filua.FreeImage_GetFormatFromFIF(fif)
end

function getFIFExtensionList(fif)
	return filua.FreeImage_GetFIFExtensionList(fif)
end

function getFIFDescription(fif)
	return filua.FreeImage_GetFIFDescription(fif)
end

function getFIFMimeType(fif)
	return filua.FreeImage_GetFIFMimeType(fif)
end

function showSupportFormat()
	count = getFIFCount()
	for i = 0, count-1 do
		print(ffi.string(getFormatFromFIF(i))..'\t'..
					ffi.string(getFIFExtensionList(i))..'\t'..
					ffi.string(getFIFDescription(i))..'\t'..
					ffi.string(getFIFMimeType(i)))
	end
end

-- Image IO
function loadImage(name)
	local fmt = getFIFFromFileName(name)
	return filua.FreeImage_Load(fmt, name, 0)
end

function saveImage(name, image_ptr, flag)
	local fmt = getFIFFromFileName(name)
	return filua.FreeImage_Save(fmt, image_ptr, name, flag)
end

function cloneImage(image_ptr)
	return filua.FreeImage_Clone(image_ptr)
end

function unloadImage(image_ptr)
	filua.FreeImage_Unload(image_ptr)
end

function createImage(width, height, bpp)
	return filua.FreeImage_Allocate(width, height, bpp, 0, 0, 0)
end

-- Common Info
function getBPP(image_ptr)
	return filua.FreeImage_GetBPP(image_ptr)
end

function getWidth(image_ptr)
	return filua.FreeImage_GetWidth(image_ptr)
end

function getHeight(image_ptr)
	return filua.FreeImage_GetHeight(image_ptr)
end

function getResolution(image_ptr)
	local x = filua.FreeImage_GetDotsPerMeterX(image_ptr)
	local y = filua.FreeImage_GetDotsPerMeterY(image_ptr)
	return math.floor(x*0.0254+0.5), math.floor(y*0.0254+0.5)
end

function setResolution(image_ptr, x, y)
	filua.FreeImage_SetDotsPerMeterX(image_ptr, x/0.0254)
	filua.FreeImage_SetDotsPerMeterY(image_ptr, y/0.0254)
end

function getFormat(name)
	local fmt = getFIFFromFileName(name)
	return ffi.string(filua.FreeImage_GetFormatFromFIF(fmt))
end

-- Convert
function convert(src, dst, flag)
	local pic = loadImage(src)
	saveImage(dst, pic, flag)
	unloadImage(pic)
end

function imgdec(src)
	convert(src, stripext(src)..'.bmp', 0)
end

function topng(src)
	convert(src, stripext(src)..'.png', 9)
end

function tojpg(src)
	-- 95quality+8192progressive
	convert(src, stripext(src)..'.jpg', 8287)
end

function convert8(src)
	local pic = loadImage(src)
	out = filua.FreeImage_ConvertTo8Bits(pic)
	unloadImage(pic)
	return out
end

function convert24(src)
	local pic = loadImage(src)
	out = filua.FreeImage_ConvertTo24Bits(pic)
	unloadImage(pic)
	return out
end

function convert32(src)
	local pic = loadImage(src)
	out = filua.FreeImage_ConvertTo32Bits(pic)
	unloadImage(pic)
	return out
end

-- Process
function rotate(image_ptr, degree)
	return filua.FreeImage_Rotate(image_ptr, degree, nil)
end

function fliphorizontal(image_ptr)
	return filua.FreeImage_FlipHorizontal(image_ptr)
end

function flipvertical(image_ptr)
	return filua.FreeImage_FlipVertical(image_ptr)
end

function rescale(image_ptr, width, height)
	return filua.FreeImage_Rescale(image_ptr, width, height, 5)
end

function jpgtransform(src, dst, op)
	filua.FreeImage_JPEGTransform(src, dst, op, 0)
end

function jpgcrop(src, dst, left, top, right, bottom)
	filua.FreeImage_JPEGCrop(src, dst, left, top, right, bottom)
end

-- Composite
function copy(image_ptr, left, top, right, bottom)
	return filua.FreeImage_Copy(image_ptr, left, top, right, bottom)
end

function paste(dst_ptr, src_ptr, left, top)
	return filua.FreeImage_Paste(dst_ptr, src_ptr, left, top, 255)
end

function composite(front_ptr, usebg, rgba, back_ptr)
	return filua.FreeImage_Composite(front_ptr, usebg, rgba, back_ptr)
end

function combine(back, front, out, left, top, flag)
	local pic1 = loadImage(back)
	local pic2 = loadImage(front)
	paste(pic1, pic2, left, top)
	saveImage(out, pic1, flag)
	unloadImage(pic1)
	unloadImage(pic2)
end

function combinealpha(back, front, out, flag)
	local pic1 = loadImage(back)
	local pic2 = loadImage(front)
	local pic3 = composite(pic2, 0, nil, pic1)
	saveImage(out, pic3, flag)
	unloadImage(pic1)
	unloadImage(pic2)
	unloadImage(pic3)
end

function jointx(imgarray, out, flag)
	local w = 0
	local dibarray = {}
	for i,v in ipairs(imgarray) do
		dibarray[i] = loadImage(v)
		w = w + getWidth(dibarray[i])
	end
	pic = createImage(w, getHeight(dibarray[0]), getBPP(dibarray[0]))
	w = 0;
	for i,v in ipairs(dibarray) do
		paste(pic, v, w, 0)
		w = w + getWidth(v)
		unloadImage(v)
	end
	saveImage(out, pic, flag)
	unloadImage(pic)
end

function jointy(imgarray, out, flag)
	local h = 0
	local dibarray = {}
	for i,v in ipairs(imgarray) do
		dibarray[i] = loadImage(v)
		h = h + getHeight(dibarray[i])
	end
	pic = createImage(getWidth(dibarray[1]), h, getBPP(dibarray[1]))
	h = 0;
	for i,v in ipairs(dibarray) do
		paste(pic, v, 0, h)
		h = h + getHeight(v)
		unloadImage(v)
	end
	saveImage(out, pic, flag)
	unloadImage(pic)
end

-- Helper
function stripext(fn)
	local idx = fn:match('.+()%.%w+$')
	if(idx) then
		return fn:sub(1, idx-1)
	else
		return fn
	end
end
