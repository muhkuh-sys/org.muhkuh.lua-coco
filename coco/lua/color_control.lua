-- Create the color_control class.
local class = require "pl.class"

---
-- @type color_control
local Color_control = class()

--- init color_control
function Color_control:_init()
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
		require "log.writer.prefix".new("[COCO COLOR_CONTROL] ", tLogWriter),
		-- formatter
		require "log.formatter.format".new()
	)

	self.tLog = tLog

	local led_analyzer = require("led_analyzer")
	self.led_analyzer = led_analyzer

	-- handles conversion between color spaces and array to table (or vice versa) handling
	local color_conversions = require("color_conversions")()
	self.color_conversions = color_conversions

	local pl = require "pl.import_into"()
	self.pl = pl

	self.bit = require "bit"

	-- Return values if the device or sensors on the device failed -- given from C code
	local auiError_msg = {
		IDENTIFICATION_ERROR = 0x40000000,
		INCOMPLETE_CONVERSION_ERROR = 0x20000000,
		EXCEEDED_CLEAR_ERROR = 0x10000000,
		DEVICE_ERROR_FATAL = 0x08000000,
		USB_ERROR = 0x04000000,
		INDEXING_ERROR = -100,
		WRITE_ERR_CH_A = -1,
		WRITE_ERR_CH_B = -2,
		READ_ERR_CH_A = -3,
		READ_ERR_CH_B = -4,
		ERR_INCORRECT_AMOUNT = -5
	}
	self.auiError_msg = auiError_msg

	local auiError_decoding = {
		[auiError_msg["IDENTIFICATION_ERROR"]] = "Identification error occured, e.g. the ID register value couldn't be read",
		[auiError_msg["INCOMPLETE_CONVERSION_ERROR"]] = "The conversion was not complete at the time the ADC register was accessed",
		[auiError_msg["EXCEEDED_CLEAR_ERROR"]] = "The maximum amount of clear level was reached, i.e. the sensor got digitally saturated",
		[auiError_msg["DEVICE_ERROR_FATAL"]] = "Fatal error on a device, writing / reading from a ftdi channel failed",
		[auiError_msg["USB_ERROR"]] = "USB error on a device, which means that we read back a different number of bytes than we expected to read",
		[auiError_msg["INDEXING_ERROR"]] = "Indexing outside the handles array (apHandles)",
		[auiError_msg["WRITE_ERR_CH_A"]] = "Error writing to channel A",
		[auiError_msg["WRITE_ERR_CH_B"]] = "Error writing to channel B",
		[auiError_msg["READ_ERR_CH_A"]] = "Error reading from channel A",
		[auiError_msg["READ_ERR_CH_B"]] = "Error reading from channel B",
		[auiError_msg["ERR_INCORRECT_AMOUNT"]] = "Error - received a different amount of bytes than expected"
	}
	self.auiError_decoding = auiError_decoding

	local MAXDEVICES = 50
	local MAXSENSORS = 16
	local MAXHANDLES = MAXDEVICES * 2
	local MAXSERIALS = MAXDEVICES

	self.MAXDEVICES = MAXDEVICES
	self.MAXSENSORS = MAXSENSORS
	self.MAXHANDLES = MAXHANDLES
	self.MAXSERIALS = MAXSERIALS

	-- Color and light related data from the TCS3472 will be stored in following arrays --
	local ausClear = led_analyzer.new_ushort(MAXSENSORS)
	local ausRed = led_analyzer.new_ushort(MAXSENSORS)
	local ausGreen = led_analyzer.new_ushort(MAXSENSORS)
	local ausBlue = led_analyzer.new_ushort(MAXSENSORS)
	local ausCCT = led_analyzer.new_ushort(MAXSENSORS)
	local afLUX = led_analyzer.new_afloat(MAXSENSORS)
	-- system settings from tcs3472 will be stored in following arrays --
	local aucGains = led_analyzer.new_puchar(MAXSENSORS)
	local aucIntTimes = led_analyzer.new_puchar(MAXSENSORS)
	-- serial numbers of connected color controller(s) will be stored in asSerials --
	local asSerials = led_analyzer.new_astring(MAXSERIALS)
	local tStrSerials = {}
	-- handle to all connected color controller(s) will be stored in apHandles (note 2 handles per device)
	local apHandles = led_analyzer.new_apvoid(MAXHANDLES)
	-- global table contains all color and light related data / global for easy access by C
	local tColorTable = {}
	-- number of 1 scanned / 2 connected devices
	local numberOfDevices = 0

	self.ausClear = ausClear
	self.ausRed = ausRed
	self.ausGreen = ausGreen
	self.ausBlue = ausBlue
	self.ausCCT = ausCCT
	self.afLUX = afLUX
	self.aucGains = aucGains
	self.aucIntTimes = aucIntTimes
	self.asSerials = asSerials
	self.tStrSerials = tStrSerials
	self.apHandles = apHandles
	self.tColorTable = tColorTable
	self.numberOfDevices = numberOfDevices
end


--- Translate the errorcode to the errormessage 
function Color_control:decodingErrorcode(err_code)
	local auiError_decoding = self.auiError_decoding

	local err_msg = auiError_decoding[err_code]
	if err_msg == nil then
		return "Unkown error code"
	else
		return err_msg
	end
end

--- scan possible CoCo devices
function Color_control:scanDevices()
	local tLog = self.tLog
	local iResult
	local numberOfDevices
	local err_msg = nil

	-- be pessimistic
	iResult = -1
	numberOfDevices = 0

	numberOfDevices = self.led_analyzer.scan_devices(self.asSerials, self.MAXSERIALS)

	if numberOfDevices < 0 then
		err_msg = "scan devices failed!"
		tLog.error(err_msg)
		return iResult, err_msg
	elseif numberOfDevices == 0 then
		err_msg = "no color controller device detected!"
		tLog.error(err_msg)
		return iResult, err_msg
	else
		iResult = 1
	end

	-- all detected devices
	self.numberOfDevices = numberOfDevices

	self.tStrSerials = self.color_conversions:astring2table(self.asSerials, self.numberOfDevices)
	return iResult, err_msg
end

-- connects to color controller devices with serial numbers given in table tStrSerials
-- if tOptionalSerials doesn't exist, function will connect to all color controller devices
-- taking the order of their serial numbers into account (serial number 20000 will have a smaller index than 20004)
function Color_control:connectDevices(tOptionalSerials)
	local tLog = self.tLog
	local iResult
	local aString
	local err_msg = nil

	-- be pessimistic
	iResult = -1

	if (tOptionalSerials == nil) then
		iResult = self.color_conversions:table2astring(self.tStrSerials, self.asSerials, self.MAXSERIALS)

		if iResult < 0 then
			err_msg = "converse table to string failed!"
			tLog.error(err_msg)
			return iResult, err_msg
		end

		iResult = self.led_analyzer.connect_to_devices(self.apHandles, self.MAXHANDLES, self.asSerials)
	else
		iResult = self.color_conversions:table2astring(tOptionalSerials, self.asSerials, self.MAXSERIALS)

		if iResult < 0 then
			err_msg = "converse table to string failed!"
			tLog.error(err_msg)
			return iResult, err_msg
		end

		iResult = self.led_analyzer.connect_to_devices(self.apHandles, self.MAXHANDLES, self.asSerials)
	end

	-- print("asSerials: ", self.asSerials)

	if iResult <= 0 then
		err_msg = "connect to devices failed!"
		tLog.error(err_msg)
		return iResult, err_msg
	end
	-- numb of connected devices (all detected or specified by tOptionalSerials)
	self.numberOfDevices = iResult
	return iResult, err_msg
end

-- Initializes the devices, by turning them on, clearing flags and identifying them
function Color_control:initDevices(atSettings)
	-- iterate over all devices and perform initialization --
	local devIndex = 0
	local iResult
	local tLog = self.tLog
	local err_msg = nil

	-- be optimistic
	iResult = 0

	while (devIndex < self.numberOfDevices) do
		--if atsettings is provided --
		if atSettings ~= nil then
			for i = 1, self.MAXSENSORS do
				if atSettings[tostring(devIndex)] ~= nil then
					iResult =
						self.led_analyzer.set_intTime_x(
						self.apHandles,
						devIndex,
						i - 1,
						atSettings[tostring(devIndex)][tostring(i)].integration
					)
					if iResult < 0 then
						err_msg =
							string.format(
							"set init time failed! Device: %d - Sensor: %d - Error Code: %d - Error Message: %s",
							devIndex,
							i,
							iResult,
							self:decodingErrorcode(iResult)
						)
						tLog.error(err_msg)
						return iResult, err_msg
					end
					iResult =
						self.led_analyzer.set_gain_x(self.apHandles, devIndex, i - 1, atSettings[tostring(devIndex)][tostring(i)].gain)
					if iResult < 0 then
						err_msg =
							string.format(
							"set gain failed! Device: %d - Sensor: %d - Error Code: %d - Error Message: %s",
							devIndex,
							i,
							iResult,
							self:decodingErrorcode(iResult)
						)
						tLog.error(err_msg)
						return iResult, err_msg
					end
				end
			end
		end

		iResult = self.led_analyzer.init_sensors(self.apHandles, devIndex)

		if iResult < 0 then
			err_msg =
				string.format(
				"init sensors failed! Device: %d - Error Code: %d - Error Message: %s",
				devIndex,
				iResult,
				self:decodingErrorcode(iResult)
			)
			tLog.error(err_msg)
			return iResult, err_msg
		end

		iResult = self.led_analyzer.get_intTime(self.apHandles, devIndex, self.aucIntTimes)

		if iResult < 0 then
			err_msg =
				string.format(
				"get integration time failed! Device: %d - Error Code: %d - Error Message: %s",
				devIndex,
				iResult,
				self:decodingErrorcode(iResult)
			)
			tLog.error(err_msg)
			return iResult, err_msg
		end

		iResult = self.led_analyzer.get_gain(self.apHandles, devIndex, self.aucGains)

		if iResult < 0 then
			err_msg =
				string.format(
				"get gain failed! Device: %d - Error Code: %d - Error Message: %s",
				devIndex,
				iResult,
				self:decodingErrorcode(iResult)
			)
			tLog.error(err_msg)
			return iResult, err_msg
		end

		devIndex = devIndex + 1
	end

	return iResult, err_msg
end

-- ATTENTION function only reads the activated RGBC channels
-- starts the measurements on each opened color controller device
-- having read and checked all raw color data, these will be converted into the needed color spaces and stored in a color table
function Color_control:startMeasurements()
	local devIndex
	local iResult
	local tLog = self.tLog
	local err_msg = nil
	local bit = self.bit
	local auiError_msg = self.auiError_msg
	local fConversion
	local uiConversion_count 
	local tDeviceConversion

	-- be optimistic
	iResult = 0

	local tStrSerials = self.color_conversions:astring2table(self.asSerials, self.numberOfDevices)

	uiConversion_count = 0
	fConversion = 0

	tDeviceConversion = {}
	for i=0,self.numberOfDevices-1 do
		tDeviceConversion[i] = 1
	end

	repeat
		devIndex = 0
		self.led_analyzer.wait4Conversion(200)

		while (devIndex < self.numberOfDevices) do
			-- Get Colours --

			if tDeviceConversion[devIndex] == 1 then
			iResult =
				self.led_analyzer.read_colors(
				self.apHandles,
				devIndex,
				self.ausClear,
				self.ausRed,
				self.ausGreen,
				self.ausBlue,
				self.aucIntTimes,
				self.aucGains
			)
		end

			if iResult == 0 then
				fConversion = 0
				tDeviceConversion[devIndex] = 0
			elseif iResult ~= 0 then
				if bit.band(iResult, auiError_msg["INCOMPLETE_CONVERSION_ERROR"]) ~= 0 and uiConversion_count <= 5 then
					fConversion = 1
					uiConversion_count = uiConversion_count + 1
				else
					err_msg =
						string.format(
						"read colors failed! Device: %d - Serial: %s - Error Code: %d",
						devIndex,
						tStrSerials[devIndex + 1],
						iResult
					)
					tLog.error(err_msg)
					return iResult, err_msg
				end
			end

			self.tColorTable[tStrSerials[devIndex + 1]] =
				self.color_conversions:aus2colorTable(
				self.ausClear,
				self.ausRed,
				self.ausGreen,
				self.ausBlue,
				self.aucIntTimes,
				self.aucGains,
				self.MAXSENSORS
			)
			-- self.pl.pretty.dump(self.tColorTable[tStrSerials[devIndex+1]])

			devIndex = devIndex + 1
		end
	until (fConversion == 0)

	return iResult, err_msg
end

function Color_control:swapUp(sCurSerial)
	self.led_analyzer.swap_up(self.asSerials, sCurSerial)
	self.tStrSerials = self.color_conversions:astring2table(self.asSerials, self.numberOfDevices)
end

function Color_control:swapDown(sCurSerial)
	self.led_analyzer.swap_down(self.asSerials, sCurSerial)
	self.tStrSerials = self.color_conversions:astring2table(self.asSerials, self.numberOfDevices)
end

function Color_control:setGainX(iDeviceIndex, iSensorIndex, gain)
	return self.led_analyzer.set_gain_x(self.apHandles, iDeviceIndex, iSensorIndex, gain)
end

function Color_control:setIntTimeX(iDeviceIndex, iSensorIndex, intTime)
	return self.led_analyzer.set_intTime_x(self.apHandles, iDeviceIndex, iSensorIndex, intTime)
end

function Color_control:setSettings(intTime, gain)
	local devIndex = 0
	local ret
	while (devIndex < self.numberOfDevices) do
		if gain ~= nil then
			ret = self.led_analyzer.set_gain(self.apHandles, devIndex, gain)
		end
		if intTime ~= nil then
			ret = self.led_analyzer.set_intTime(self.apHandles, devIndex, intTime)
		end
		devIndex = devIndex + 1
	end
	return ret
end

-- don't forget to clean up after every test --
function Color_control:free()
	-- CLEAN UP --
	self.led_analyzer.free_devices(self.apHandles)
	self.led_analyzer.delete_ushort(self.ausClear)
	self.led_analyzer.delete_ushort(self.ausRed)
	self.led_analyzer.delete_ushort(self.ausGreen)
	self.led_analyzer.delete_ushort(self.ausBlue)
	self.led_analyzer.delete_ushort(self.ausCCT)
	self.led_analyzer.delete_afloat(self.afLUX)
	self.led_analyzer.delete_puchar(self.aucGains)
	self.led_analyzer.delete_puchar(self.aucIntTimes)
	self.led_analyzer.delete_apvoid(self.apHandles)
	self.led_analyzer.delete_astring(self.asSerials)

	self.ausClear = nil
	self.ausRed = nil
	self.ausGreen = nil
	self.ausBlue = nil
	self.ausCCT = nil
	self.afLUX = nil
	-- system settings from tcs3472 will be stored in following arrays --
	self.aucGains = nil
	self.aucIntTimes = nil
	-- serial numbers of connected color controller(s) will be stored in asSerials --
	self.asSerials = nil
	self.tStrSerials = {}
	-- handle to all connected color controller(s) will be stored in apHandles (note 2 handles per device)
	self.apHandles = nil
	-- global table contains all color and light related data / global for easy access by C
	self.tColorTable = {}
	-- number of 1 scanned / 2 connected devices
	self.numberOfDevices = 0
end

function Color_control:test(tData)
	local tLog = self.tLog
	local err_msg = nil

	-- be pessimistic
	local iResult = -1

	if tData == nil then
		err_msg = "No data of CoCo settings (and optional data of serials) available."
		tLog.error(err_msg)
		return iResult, err_msg
	end

	local atSettings = tData.atSettings
	if atSettings == nil then
		err_msg = "No data of CoCo settings available."
		tLog.error(err_msg)
		return iResult, err_msg
	end

	local uiConversationTime = tData.uiConversationTime
	if uiConversationTime == nil then
		err_msg = "No data of CoCo conversion time available."
		tLog.error(err_msg)
		return iResult, err_msg
	end

	-- optional
	local asSerials = tData.asSerials
	if atSettings == nil then
		tLog.info("No opional data of CoCo serials available.")
	end

	tLog.info("start to scan CoCo devices")
	iResult, err_msg = self:scanDevices()

	if iResult <= 0 then
		self:free()
		return iResult, err_msg
	end
	tLog.info("scan CoCo devices finished")
	tLog.info(
		"detected number of devices: %d - with serials of: %s",
		self.numberOfDevices,
		table.concat(self.tStrSerials, ",")
	)

	tLog.info("initialize connection to devices: ")
	iResult, err_msg = self:connectDevices(asSerials)

	if iResult <= 0 then
		self:free()
		return iResult, err_msg
	end
	tLog.info("established connection to devices: ")

	tLog.info("start of the initialization")
	iResult, err_msg = self:initDevices(atSettings)
	if iResult < 0 then
		self:free()
		return iResult, err_msg
	end
	tLog.info("initialization finished")

	-- tLog.info("conversion time: %d", uiConversationTime)
	-- self.led_analyzer.wait4Conversion(uiConversationTime)

	tLog.info("start of the measurement")
	iResult, err_msg = self:startMeasurements()
	if iResult ~= 0 then
		self:free()
		return iResult, err_msg
	end

	tLog.info("measurement of CoCo finished")

	return iResult, err_msg
end

return Color_control
