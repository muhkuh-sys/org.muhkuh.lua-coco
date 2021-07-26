local t = ...
local strDistId, strDistVersion, strCpuArch = t:get_platform()
local tResult

if strDistId=='@JONCHKI_PLATFORM_DIST_ID@' and strCpuArch=='@JONCHKI_PLATFORM_CPU_ARCH@' then
	t:install('CoCo_client.lua', '${install_lua_path}/')
	t:install('lua/color_validation.lua', '${install_lua_path}/')
  tResult = true
end

return tResult
