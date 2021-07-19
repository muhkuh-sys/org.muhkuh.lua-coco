-- Create the color_control class.
local class = require "pl.class"

---
-- @type Color_validation
local Color_validation = class()

function Color_validation:_init()
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
		require "log.writer.prefix".new("[COCO COLOR_VALIDATION] ", tLogWriter),
		-- formatter
		require "log.formatter.format".new()
	)

	self.tLog = tLog

	local pl = require "pl.import_into"()
	self.pl = pl

	local tTestSummary = {}
	self.tTestSummary = tTestSummary
end

function Color_validation:validateSensor(tColorTableSensor, tTestSetSensor, lux_check_enable)
	local tTestSummary_device_sensor = {}
	local pl = self.pl
	local tLog = self.tLog

	-- current values
	local nm = tColorTableSensor.Wavelength.nm
	local sat = tColorTableSensor.Wavelength.sat
	local lux = tColorTableSensor.Wavelength.lux
	--tolerances
	local tol_nm = tTestSetSensor.tol_nm
	local tol_sat = tTestSetSensor.tol_sat
	local tol_lux = tTestSetSensor.tol_lux
	-- set points --
	local sp_nm = tTestSetSensor.nm
	local sp_sat = tTestSetSensor.sat
	local sp_lux = tTestSetSensor.lux

	local strStatus_nm =
		(nm < (sp_nm - tol_nm) or nm > (sp_nm + tol_nm)) and "Wavelength is not in range" or "Wavelength is in range"
	local strStatus_sat =
		(sat < (sp_sat - tol_sat) or sat > (sp_sat + tol_sat)) and "Saturation is not in range" or "Saturation is in range"

	local strStatus_lux
	if lux_check_enable ~= nil then
		strStatus_lux =
			(lux >= (sp_lux - tol_lux) and lux <= (sp_lux + tol_lux)) and "Illumination is in range" or
			(lux < (sp_lux - tol_lux)) and "Illumination is too low" or
			"Illumination is too high"
	else
		strStatus_lux = "Illumination was not tested"
	end

	tTestSummary_device_sensor = {
		status_nm = strStatus_nm,
		status_sat = strStatus_sat,
		status_lux = strStatus_lux,
		name = tTestSetSensor.name,
		nm = nm,
		sat = sat,
		lux = lux,
		tol_nm = tol_nm,
		tol_sat = tol_sat,
		tol_lux = tol_lux,
		sp_nm = sp_nm,
		sp_sat = sp_sat,
		sp_lux = sp_lux
	}
	
	return tTestSummary_device_sensor
end

function Color_validation:validateCoCo(tMeasurementResults, tTestSet, lux_check_enable)
	local tLog = self.tLog
	local err_msg = nil
	local iResult
	local tTestSummary = self.tTestSummary
	local tTestSummary_device

	-- be pesimistic
	iResult = -1

	if tTestSet == nil then
		err_msg = string.format("No test set(s) available")
		tLog.error(err_msg)
		return iResult, err_msg
	end

	if tMeasurementResults == nil then
		err_msg = string.format("No measurement results of CoCo available")
		tLog.error(err_msg)
		return iResult, err_msg
	end

	for strDeviceSerial, tColorTable in pairs(tMeasurementResults) do
		if tColorTable == nil then
			err_msg = string.format("No measurement results of device %s available", strDeviceSerial)
			tLog.error(err_msg)
			return iResult, err_msg
		end

		tTestSummary[strDeviceSerial] = {}

		if tTestSet[strDeviceSerial] == nil then
			tLog.info("No test set available for the device: %s", strDeviceSerial)
		else
			tTestSummary_device = {}

			for uiSensor, tColorTableSensor in ipairs(tColorTable) do
				if tTestSet[strDeviceSerial][uiSensor] == nil then
					tTestSummary_device[uiSensor] = {
						status_nm = "NO TEST ENTRY",
						status_sat = "NO TEST ENTRY",
						status_lux = "NO TEST ENTRY",
						name = -1,
						nm = -1,
						sat = -1,
						lux = -1,
						tol_nm = -1,
						tol_sat = -1,
						tol_lux = -1,
						sp_nm = -1,
						sp_sat = -1,
						sp_lux = -1
					}
				else
					tTestSummary_device[uiSensor] =
						self:validateSensor(tColorTableSensor, tTestSet[strDeviceSerial][uiSensor], lux_check_enable)
				end
			end
			tTestSummary[strDeviceSerial] = tTestSummary_device
		end
	end

	self.tTestSummary = tTestSummary

	return iResult, err_msg
end


return Color_validation
