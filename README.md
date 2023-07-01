# Gamecg composite library forks from FreeImage

## New Features:
* susie plugin
* qt plugin
* windows context menu previewer

## Removed Features:
* keep image metatag
* multipage image suport

## 3rd Parties
|Library|Homepage|
|:---|:---|
|libpng 1.6.40|http://libpng.com/pub/png/libpng.html|
|libtiff 4.5.1|http://www.simplesystems.org/libtiff|
|libwebp 1.3.1|https://github.com/webmproject/libwebp|
|mozjpeg 4.1.3|https://github.com/mozilla/mozjpeg|
|zlib 1.2.13|http://zlib.net|
|jxrlib 1.1|https://jxrlib.codeplex.com|

# Windows Context Menu Previewer
## install
regsvr32 FIShellExt.dll

## uninstall
regsvr32 /u FIShellExt.dll

![slide](http://paste.ubuntu.org.cn/i2993582.png)

# Lua Extension

```lua
require 'fi'

---
# convert image format
convert('*.webp', 'tiff')
# convert image format and bpp
convbpp('*.png', 32, 'bmp')
# convert image format with quality setting
convert('1.png', '1.jpg', 90)

# alpha composite with 24bpp background 8bpp foreground
back = open('rgb24.bmp')
front = open('alpha8.bmp')
back_resized = scale(back, getw(front), geth(front))
back_rgba = to32(back_resized)
save(back_resized, 'resized.png', 9)
save(back_rgba, 'back.tiff')
out = composite(back_rgba, front)
save(out, 'composite.png')
freeAll({back_resized, front, out, back_rgba})
free(back)
...
```

# CLI interface(luajit)
```bash
# windows terminal
chcp 65001
# unix-like system
alias conv="noglob luajit conv.lua"

# convert image format
conv *.png jpg
conv 民逗Σ.dds 二マビ.webp
# convert image format and bpp
conv abc.jpg 32 abc.tiff
# convert image format with quality setting
conv 汉字.bmp 漢字.jpg 75 
