local host, port = "127.0.0.1", 5555
local uv = require "lluv"

local class = require "pl.class"
local CoCo_Server = class()

--- init CoCo_Server
function CoCo_Server:_init(host, port, tData)
	local tLogWriter
	tLogWriter = require "log.writer.filter".new("info", require "log.writer.console.color".new())

	local strLogDir = ".logs"
	local strLogFilename = ".log_Data.log"

	local tLog =
		require "log".new(
		-- maximum log level
		"trace",
		-- writer
		require "log.writer.prefix".new("[COCO SERVER] ", tLogWriter),
		-- formatter
		require "log.formatter.format".new()
	)

	self.tLog = tLog

	self.pl = require "pl.import_into"()
	self.json = require "dkjson"
	-- self.lunajson = require "lunajson"

	self.auiTRANSMISSION_RESULT = {
		TRANSMISSION_OK = 0,
		TRANSMISSION_FAIL = 1,
		TRANSMISSION_DECODING_ERROR = 2,
		TRANSMISSION_COCO_ERROR = 3
	}
end

local function on_coco(cli, err, data)
	local color_control = require("color_control")()
	local tCoCo_Server = CoCo_Server()

	local auiTRANSMISSION_RESULT = tCoCo_Server.auiTRANSMISSION_RESULT
	local json = tCoCo_Server.json
	local lunajson = tCoCo_Server.lunajson
	local tLog = tCoCo_Server.tLog

	if err then
		cli:write(auiTRANSMISSION_RESULT["TRANSMISSION_FAIL"] .. "\n")
		return cli:close()
	end

	cli:write(auiTRANSMISSION_RESULT["TRANSMISSION_OK"] .. "\n")
	tLog.info('data received')

	local decoded_data, pos, err_msg = json.decode(data, 1, nil)
	
	if err_msg then
		cli:write(auiTRANSMISSION_RESULT["TRANSMISSION_DECODING_ERROR"] .. "\n")
		tLog.error("Errormessage:", err_msg)
		return cli:close()
	else
		cli:write(auiTRANSMISSION_RESULT["TRANSMISSION_OK"] .. "\n")
		tLog.info('received data decoded')
		local iResult
		iResult, err_msg = color_control:test(decoded_data)
		if iResult ~= 0 then
			cli:write(auiTRANSMISSION_RESULT["TRANSMISSION_COCO_ERROR"] .. "\n")
			cli:write("CoCo failed: " .. err_msg .. "\n")
			return cli:close()
		else
			tLog.info('CoCo test successful')
			cli:write(auiTRANSMISSION_RESULT["TRANSMISSION_OK"] .. "\n")

			local strColorTable_encoded = json.encode(color_control.tColorTable)
			cli:write(strColorTable_encoded .. "\n")
			color_control:free()
		end
	end

	cli:close()
end

local function on_connection(server, err)
	if err then
		return server:close()
	end
	server:accept():start_read(on_coco)
end

local function on_bind(server, err, host, port)
	if err then
		print("Bind fail:" .. tostring(err))
		return server:close()
	end

	if port then
		host = host .. ":" .. port
	end
	print("Bind on: " .. host)

	server:listen(on_connection)
end


uv.tcp():bind(host, port, on_bind)
uv.run(debug.traceback)

