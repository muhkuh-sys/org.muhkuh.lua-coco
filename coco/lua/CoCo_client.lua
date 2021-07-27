local class = require "pl.class"

local CoCo_Client = class()

--- init CoCo_Client
function CoCo_Client:_init(host, port)
	local tLogWriter = require "log.writer.filter".new("info", require "log.writer.console.color".new())

	host = host or "127.0.0.1"
	port = port or 5555

	self.host = host
	self.port = port

	-- local strLogDir = ".logs"
	-- local strLogFilename = ".log_Data.log"

	-- local tLogWriter =
	-- 	require "log.writer.list".new(
	-- 	tLogWriter,
	-- 	require "log.writer.file".new {
	-- 		log_dir = strLogDir, --   log dir
	-- 		log_name = strLogFilename, --   current log name
	-- 		max_rows = 100000, -- max row size
	-- 		max_size = 1000000, --   max file size in bytes
	-- 		roll_count = 10 --   count files
	-- 	}
	-- )

	local tLog =
		require "log".new(
		-- maximum log level
		"trace",
		-- writer
		require "log.writer.prefix".new("[COCO CLIENT] ", tLogWriter),
		-- formatter
		require "log.formatter.format".new()
	)

	self.pl = require "pl.import_into"()
	self.socket = require "socket"

	self.json = require "dkjson"
	-- self.lunajson = require "lunajson"

	self.color_validation = require("color_validation")()

	self.tLog = tLog
end

function CoCo_Client:run(strFilenameResults, strFilenameTestSummary, tData, tTestSet, lux_check_enable)
	local pl = self.pl
	local socket = self.socket
	local host = self.host
	local port = self.port
	local tLog = self.tLog
	local json = self.json
	local color_validation = self.color_validation
	local iResult

	-- in the case tData is a filename of a json file instead of a table
	if tData == nil then
		tLog.error("No settings of CoCo specified in tData")
		return -1
	end

	if type(tData) == "string" then
		local strFilename = pl.path.expanduser(tData)

		if pl.path.isfile(strFilename) == false then
			tLog.error("The specified path of tData does not contain a file name")
			return -1
		else
			local strBin, strMsg = self.pl.utils.readfile(strFilename, true)
			if strBin == nil then
				tLog.error('Failed to read "%s" : ', tData, tostring(strMsg))
				return -1
			end
			tData = self.json.decode(strBin, 1, nil)
		end
	end

	-- in the case tTestSet is a filename of a json file instead of a table
	if tTestSet == nil then
		tLog.error("No test set data specified in tTestSet")
		return -1
	end

	if type(tTestSet) == "string" then
		local strFilename = pl.path.expanduser(tTestSet)

		if pl.path.isfile(strFilename) == false then
			tLog.error("The specified path of tTestSet does not contain a file name")
			return -1
		else
			local strBin, strMsg = self.pl.utils.readfile(strFilename, true)
			if strBin == nil then
				tLog.error('Failed to read "%s" : ', tTestSet, tostring(strMsg))
				return -1
			end
			tTestSet = self.json.decode(strBin, 1, nil)
		end
	end

	local tcp, err_msg = socket.tcp()
	if tcp == nil then
		tLog.error("Creating TCP master object failed. Error Message: %s", err_msg)
		return -1
	end

	tcp:settimeout(10)

	iResult, err_msg = tcp:connect(host, port)
	if iResult ~= 1 then
		tLog.error("Connecting to %s:%d failed. Error Message: %s", host, port, err_msg)
		tcp:close()
		return -1
	end

	local tData_encoded = self.json.encode(tData, {indent = true})
	iResult, err_msg = tcp:send(tData_encoded)
	if iResult == nil then
		tLog.error("Sending data to %s:%d failed. Error Message: %s", host, port, err_msg)
		tcp:close()
		return -1
	end

	-- transmission tData
	local msg, partial
	msg, err_msg, partial = tcp:receive()
	if msg == nil then
		tLog.error("Transmission to %s:%d failed. Error Message: %s", host, port, err_msg)
		tcp:close()
		return -1
	end

	if tonumber(msg) ~= 0 then
		tLog.error("data transmission error")
		tcp:close()
		return -1
	else
		tLog.info("data received")
	end

	-- decoding tData
	msg, err_msg, partial = tcp:receive()
	if msg == nil then
		tLog.error("Transmission to %s:%d failed. Error Message: %s", host, port, err_msg)
		tcp:close()
		return -1
	end

	if tonumber(msg) ~= 0 then
		tLog.error("data decoding error")
		tcp:close()
		return -1
	else
		tLog.info("data decoded")
	end

	-- CoCo results
	msg, err_msg, partial = tcp:receive()
	if msg == nil then
		tLog.error("Transmission to %s:%d failed. Error Message: %s", host, port, err_msg)
		tcp:close()
		return -1
	end

	if tonumber(msg) ~= 0 then
		tLog.error("CoCo measurement error")

		-- CoCo measurement error message
		msg, err_msg, partial = tcp:receive()
		if msg == nil then
			tLog.error("Transmission to %s:%d failed. Error Message: %s", host, port, err_msg)
			tcp:close()
			return -1
		end
		tLog.error(msg)
		tcp:close()
		return -1
	else
		tLog.info("CoCo measurement OK")

		local tMeasurement
		tMeasurement, err_msg, partial = tcp:receive()
		if tMeasurement == nil then
			tLog.error("Transmission to %s:%d failed. Error Message: %s", host, port, err_msg)
			tcp:close()
			return -1
		end

		-- output string should not be in a single line - decode + encode with indent
		local decoded_tMeasurement, pos, err_msg = json.decode(tMeasurement, 1, nil)
		local tMeasurement = self.json.encode(decoded_tMeasurement, {indent = true})

		local tResult, strMsg = pl.utils.writefile(strFilenameResults, tMeasurement, true)
		if tResult == true then
			tLog.info("OK. CoCo results was written to the file '%s'", strFilenameResults)
		else
			tLog.error('Failed to write the data to the file "%s": %s', strFilenameResults, strMsg)
			return -1
		end

		-- validate measurement of CoCo with data of tTestSet
		color_validation:validateCoCo(decoded_tMeasurement, tTestSet, lux_check_enable)

		local tTestSummary = self.json.encode(color_validation.tTestSummary, {indent = true})

		local tResult, strMsg = pl.utils.writefile(strFilenameTestSummary, tTestSummary, true)
		if tResult == true then
			tLog.info("OK. CoCo test summary was written to the file '%s'", strFilenameTestSummary)
		else
			tLog.error('Failed to write the data to the file "%s": %s', strFilenameTestSummary, strMsg)
			return -1
		end
	end

	tcp:close()

	return 0
end

return CoCo_Client
