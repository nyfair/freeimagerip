require "table"
require "fi"

num = table.getn(arg)
if num == 0 then
	convert("*.png", "bmp")
	convert("*.jpg", "bmp")
	convert("*.tga", "bmp")
	convert("*.tif*", "bmp")
elseif num == 1 then
	if arg[1] == "24" or arg[1] == "32" or arg[1] == "8" then
		convbpp("*.bmp", tonumber(arg[1]), "bmp")
		convbpp("*.png", tonumber(arg[1]), "bmp")
	else convert("*.bmp", arg[1])
	end
elseif num == 2 then
	convert(arg[1], arg[2])
else
	if arg[2] == "24" or arg[2] == "32" or arg[2] == "8" then
		convbpp(arg[1], tonumber(arg[2]), arg[3])
	else convert(arg[1], arg[2], tonumber(arg[3]))
	end
end
