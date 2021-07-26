local t = ...
local pl = t.pl

-- Copy all additional files.
t:install{
  -- Copy the server script to the installation base.
  ['${depack_path_org.muhkuh.lua.coco.lua5.4-coco}/lua_plugins/led_analyzer.so'] = '${install_base}/lua_plugins',
  ['${depack_path_org.muhkuh.lua.coco.lua5.4-coco}/CoCo_server.lua'] = '${install_base}/',
  ['${depack_path_org.muhkuh.lua.coco.lua5.4-coco}/lua/color_control.lua'] = '${install_base}/lua/',
  ['${depack_path_org.muhkuh.lua.coco.lua5.4-coco}/lua/color_conversions.lua'] = '${install_base}/lua/',
  ['${depack_path_org.muhkuh.lua.coco.lua5.4-coco}/lua/tcs_chromaTable.lua'] = '${install_base}/lua/',
  ['${report_path}']                                       = '${install_base}/.jonchki/'
}

t:createPackageFile()
t:createHashFile()
t:createArchive('${install_base}/../../../${default_archive_name}', 'native')

return true
