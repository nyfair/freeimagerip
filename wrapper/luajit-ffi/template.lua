require 'fi'

out='out/'
md(out)
for k,v in ipairs(ls('*.bmp')) do
	print(v)
	i=open(v)
	
	--[[
	do something
	]]--
	
	save(o,out..stripext(v)..'.bmp')
	free(o)
	free(i)
	
	--[[
	free unused cache
	]]--
end
