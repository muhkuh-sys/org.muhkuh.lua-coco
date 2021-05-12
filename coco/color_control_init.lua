module("color_control_init", package.seeall)

-- Set the search path for LUA plugins.
package.cpath = package.cpath .. ";lua_plugins/?.so;lua_plugins/?.dll;"

-- Set the search path for LUA modules.
package.path = package.path .. ";lua/?.lua;lua/?/init.lua;"

-- Load the common romloader plugins.
require("color_control")
