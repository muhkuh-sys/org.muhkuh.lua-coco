local class = require "pl.class"

local CoCo_Client = class()

--- init CoCo_Client
function CoCo_Client:_init(host, port, strFilenameResults, strFilenameTestSummary, tData, tTestSet, lux_check_enable)
	local tLogWriter = require "log.writer.filter".new("info", require "log.writer.console.color".new())

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

	self.tData = tData
	self.tTestSet = tTestSet

	self.host = host
	self.port = port
	self.strFilenameResults = strFilenameResults
	self.strFilenameTestSummary = strFilenameTestSummary
	self.lux_check_enable = lux_check_enable
	self.tLog = tLog
end

function CoCo_Client:run()
	local pl = self.pl
	local socket = self.socket
	local host = self.host
	local port = self.port
	local strFilenameResults = self.strFilenameResults
	local strFilenameTestSummary = self.strFilenameTestSummary
	local tData = self.tData
	local tTestSet = self.tTestSet
	local lux_check_enable = self.lux_check_enable
	local tLog = self.tLog
	local json = self.json
	-- local lunajson = self.lunajson
	local color_validation = self.color_validation
	local iResult

	-- in the case tData is a filename of a json file instead of a table
	if type(tData) == "string" then
		local strFilename = pl.path.expanduser(tData)

		if pl.path.isfile(strFilename) == false then
			tLog.error("The specified path of tData does not contain a file name")
		else
			local strBin, strMsg = self.pl.utils.readfile(strFilename, true)
			if strBin == nil then
				tLog.error('Failed to read "%s" : ', tData, tostring(strMsg))
			end
			tData = self.json.decode(strBin, 1, nil)
		end
	end

	-- in the case tTestSet is a filename of a json file instead of a table
	if type(tTestSet) == "string" then
		local strFilename = pl.path.expanduser(tTestSet)

		if pl.path.isfile(strFilename) == false then
			tLog.error("The specified path of tTestSet does not contain a file name")
		else
			local strBin, strMsg = self.pl.utils.readfile(strFilename, true)
			if strBin == nil then
				tLog.error('Failed to read "%s" : ', tTestSet, tostring(strMsg))
			end
			tTestSet = self.json.decode(strBin, 1, nil)
		end
	end

	local tcp, err_msg = socket.tcp()
	if tcp == nil then
		tLog.error("Creating TCP master object failed. Error Message: %s", err_msg)
		return
	end

	tcp:settimeout(10)

	iResult, err_msg = tcp:connect(host, port)
	if iResult ~= 1 then
		tLog.error("Connecting to %s:%d failed. Error Message: %s", host, port, err_msg)
		tcp:close()
		return
	end

	local tData_encoded = self.json.encode(tData, {indent = true})
	-- local tData_encoded = lunajson.encode(tData)
	iResult, err_msg = tcp:send(tData_encoded)
	-- print(tData_encoded)
	if iResult == nil then
		tLog.error("Sending data to %s:%d failed. Error Message: %s", host, port, err_msg)
		tcp:close()
		return
	end

	-- transmission tData
	local msg, partial
	msg, err_msg, partial = tcp:receive()
	if msg == nil then
		tLog.error("T	-- print(tData_encoded)ransmission to %s:%d failed. Error Message: %s", host, port, err_msg)
		tcp:close()
		return
	end

	if tonumber(msg) ~= 0 then
		tLog.error("data transmission error")
		tcp:close()
		return
	else
		tLog.info("data received")
	end

	-- decoding tData
	msg, err_msg, partial = tcp:receive()
	if msg == nil then
		tLog.error("Transmission to %s:%d failed. Error Message: %s", host, port, err_msg)
		tcp:close()
		return
	end

	if tonumber(msg) ~= 0 then
		tLog.error("data decoding error")
		tcp:close()
		return
	else
		tLog.info("data decoded")
	end

	-- CoCo results
	msg, err_msg, partial = tcp:receive()
	if msg == nil then
		tLog.error("Transmission to %s:%d failed. Error Message: %s", host, port, err_msg)
		tcp:close()
		return
	end

	if tonumber(msg) ~= 0 then
		tLog.error("CoCo measurement error")

		-- CoCo measurement error message
		msg, err_msg, partial = tcp:receive()
		if msg == nil then
			tLog.error("Transmission to %s:%d failed. Error Message: %s", host, port, err_msg)
			tcp:close()
			return
		end
		tLog.error(msg)
		tcp:close()
		return
	else
		tLog.info("CoCo measurement OK")

		local tMeasurement
		tMeasurement, err_msg, partial = tcp:receive()
		if tMeasurement == nil then
			tLog.error("Transmission to %s:%d failed. Error Message: %s", host, port, err_msg)
			tcp:close()
			return
		end

		-- output string should not be in a single line - decode + encode with indent
		local decoded_tMeasurement, pos, err_msg = json.decode(tMeasurement, 1, nil)
		local tMeasurement = self.json.encode(decoded_tMeasurement, {indent = true})

		local tResult, strMsg = pl.utils.writefile(strFilenameResults, tMeasurement, true)
		if tResult == true then
			tLog.info("OK. CoCo results was written to the file '%s'", strFilenameResults)
		else
			tLog.error('Failed to write the data to the file "%s": %s', strFilenameResults, strMsg)
		end

		-- validate measurement of CoCo with data of tTestSet
		color_validation:validateCoCo(decoded_tMeasurement, tTestSet, lux_check_enable)

		local tTestSummary = self.json.encode(color_validation.tTestSummary, {indent = true})

		local tResult, strMsg = pl.utils.writefile(strFilenameTestSummary, tTestSummary, true)
		if tResult == true then
			tLog.info("OK. CoCo test summary was written to the file '%s'", strFilenameTestSummary)
		else
			tLog.error('Failed to write the data to the file "%s": %s', strFilenameTestSummary, strMsg)
		end
	end

	tcp:close()
end

local host, port = "127.0.0.1", 5555
local strFilenameResults = "CoCo_Results.json"
local strFilenameTestSummary = "TestSummary.json"
local lux_check_enable = 1

local tData = "/home/struber/workspace/coco/tData.json"
local tTestSet = "/home/struber/workspace/coco/tTestSet.json"
local tCoCo_Client =
	CoCo_Client(host, port, strFilenameResults, strFilenameTestSummary, tData, tTestSet, lux_check_enable)
tCoCo_Client:run()
