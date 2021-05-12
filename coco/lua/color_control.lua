---- importing ----

require("led_analyzer")       -- api for color controller device 
require("color_conversions")  -- handles conversion between color spaces and array to table (or vice versa) handling
require("color_validation")   -- Validate your colors, contains helper to print your colors and store your colors in adequate arrays
require("generate_xml")


-- LED Test retvals
TEST_RESULT_OK            = 0
TEST_RESULT_FAIL          = 1
TEST_RESULT_DEVICE_ERROR  = 2

-- Return values if the device or sensors on the device failed -- dont change !
IDENTIFICATION_ERROR        = 0x40000000
INCOMPLETE_CONVERSION_ERROR = 0x20000000
EXCEEDED_CLEAR_ERROR        = 0x10000000
DEVICE_ERROR_FATAL          = 0x08000000
USB_ERROR                   = 0x04000000

MAXDEVICES = 50
MAXSENSORS = 16
MAXHANDLES = MAXDEVICES * 2
MAXSERIALS = MAXDEVICES

-- tcs3472 specific settings for gain
TCS3472_GAIN_1X  = 0x00
TCS3472_GAIN_4X  = 0x01
TCS3472_GAIN_16X = 0x02
TCS3472_GAIN_60X = 0x03

-- tcs3472 specific settings for integration time
TCS3472_INTEGRATION_2_4ms       = 0xFF
TCS3472_INTEGRATION_24ms        = 0xF6
TCS3472_INTEGRATION_100ms       = 0xD6
TCS3472_INTEGRATION_154ms       = 0xC0
TCS3472_INTEGRATION_200ms       = 0xAD
TCS3472_INTEGRATION_700ms       = 0x00

-- indexes for entries in tColorTable
ENTRY_WAVELENGTH = 1
ENTRY_RGB        = 2
ENTRY_XYZ        = 3
ENTRY_Yxy        = 4
ENTRY_HSV        = 5
ENTRY_SETTINGS   = 6
ENTRY_ERRORCODE  = 7


local fArraysCreated    = false

-- Color and light related data from the TCS3472 will be stored in following arrays --
local ausClear          = nil
local ausRed            = nil
local ausGreen          = nil
local ausBlue           = nil
local ausCCT            = nil
local afLUX             = nil
-- system settings from tcs3472 will be stored in following arrays --
local aucGains          = nil
local aucIntTimes       = nil
-- serial numbers of connected color controller(s) will be stored in asSerials --
local asSerials         = nil
local tStrSerials       = {}
-- handle to all connected color controller(s) will be stored in apHandles (note 2 handles per device)
local apHandles         = nil
-- global table contains all color and light related data / global for easy access by C
tColorTable             = {}
-- number of 1 scanned / 2 connected devices
local numberOfDevices   = 0
-- table that contains a test summary for each device --
local tTestSummary      = nil




local function create_arrays()
	if fArraysCreated~=true then
		-- Color and light related data from the TCS3472 will be stored in following arrays --
		ausClear          = led_analyzer.new_ushort(MAXSENSORS)
		ausRed            = led_analyzer.new_ushort(MAXSENSORS)
		ausGreen          = led_analyzer.new_ushort(MAXSENSORS)
		ausBlue           = led_analyzer.new_ushort(MAXSENSORS)
		ausCCT            = led_analyzer.new_ushort(MAXSENSORS)
		afLUX             = led_analyzer.new_afloat(MAXSENSORS)
		-- system settings from tcs3472 will be stored in following arrays --
		aucGains          = led_analyzer.new_puchar(MAXSENSORS)
		aucIntTimes       = led_analyzer.new_puchar(MAXSENSORS)
		-- serial numbers of connected color controller(s) will be stored in asSerials --
		asSerials         = led_analyzer.new_astring(MAXSERIALS)
		tStrSerials       = {}
		-- handle to all connected color controller(s) will be stored in apHandles (note 2 handles per device)
		apHandles         = led_analyzer.new_apvoid(MAXHANDLES)
		-- global table contains all color and light related data / global for easy access by C
		tColorTable       = {}
		-- number of 1 scanned / 2 connected devices
		numberOfDevices   = 0
		-- table that contains a test summary for each device --
		tTestSummary      = nil

		fArraysCreated = true
	end
end



function scanDevices()
	create_arrays()

	numberOfDevices = led_analyzer.scan_devices(asSerials, MAXSERIALS)
	tStrSerials = color_conversions.astring2table(asSerials, numberOfDevices)

	return numberOfDevices, generate_xml.generate_xml_exception(numberOfDevices), tStrSerials
end



-- connects to color controller devices with serial numbers given in table tStrSerials
-- if tOptionalSerials doesn't exist, function will connect to all color controller devices
-- taking the order of their serial numbers into account (serial number 20000 will have a smaller index than 20004)
function connectDevices(tOptionalSerials)
	if(tOptionalSerials == nil) then
		numberOfDevices = led_analyzer.connect_to_devices(apHandles, MAXHANDLES, color_conversions.table2astring(tStrSerials, asSerials))
	else
		numberOfDevices = led_analyzer.connect_to_devices(apHandles, MAXHANDLES, color_conversions.table2astring(tOptionalSerials, asSerials))
	end

	return numberOfDevices, generate_xml.generate_xml_exception(numberOfDevices)
end



-- todo: return a strxml
-- Initializes the devices, by turning them on, clearing flags and identifying them
function initDevices(atSettings)
-- iterate over all devices and perform initialization --
	local devIndex = 0
	local error_counter = 0
	local ret = 0


	while(devIndex < numberOfDevices) do
		--if atsettings is provided --
		if atSettings ~= nil then
			for i = 1, MAXSENSORS do
				if atSettings[devIndex] ~= nil then 
					led_analyzer.set_intTime_x(apHandles, devIndex, i-1, atSettings[devIndex][i].integration)
					led_analyzer.set_gain_x(apHandles, devIndex, i-1, atSettings[devIndex][i].gain)
				end
			end
		end

		ret = led_analyzer.init_sensors(apHandles, devIndex)
		led_analyzer.get_intTime(apHandles, devIndex, aucIntTimes)
		led_analyzer.get_gain(apHandles, devIndex, aucGains)

		tColorTable[devIndex] = {}
		tColorTable[devIndex][ENTRY_ERRORCODE] = ret
		tColorTable[devIndex][ENTRY_SETTINGS]  = color_conversions.auc2settingsTable(aucIntTimes, aucGains, MAXSENSORS)

		devIndex = devIndex + 1
	end

	return ret
end



-- todo: return a strxml
-- starts the measurements on each opened color controller device
-- having read and checked all raw color data, these will be converted into the needed color spaces and stored in a color table
function startMeasurements()
	local devIndex = 0
	local error_counter = 0
	local ret = 0
	local fatal_error_occured


	tColorTable = {}

	while(devIndex < numberOfDevices) do
		-- Get Colours --

		ret = led_analyzer.read_colors(apHandles, devIndex, ausClear, ausRed, ausGreen, ausBlue, aucIntTimes, aucGains)
		if ret ~= 0 then
			if ret == DEVICE_ERROR_FATAL then -- return code for fatal errors
				fatal_error_occured = 1
			end
		end

		tColorTable[devIndex] = color_conversions.aus2colorTable(ausClear, ausRed, ausGreen, ausBlue, aucIntTimes, aucGains, ret, MAXSENSORS)
		devIndex = devIndex + 1
	end

	if fatal_error_occured then
		return DEVICE_ERROR_FATAL
	end

	-- otherwise return 0 as no fatal error occured, the actualy ret code for smaller errors is stored in tColorTable
	return 0
end



-- function compares the color sets read from the devices to the testtable given in tDUT
-- the LEDs under test must be on, this means we test if the right LEDs (correct wavelength, sat, ...) are mounted on the baord
-- a table tTestSummary will be filled according to the test results (led on, led off, wrong led detected and so on)
-- returns: an integer as return value (ret) indicating the result of the led test (0 if ok)
--          a string as second parameter. string contains the test result with traces as xml
function validateLEDs(tDUT, lux_check_enable)
	local devIndex = 0
	local ret = 0
	local strXml


	-- empty test summary --
	tTestSummary = {}

	while(devIndex < numberOfDevices) do
		tTestSummary[devIndex] = color_validation.getDeviceSummary(tDUT[devIndex], tColorTable[devIndex][ENTRY_WAVELENGTH], lux_check_enable)
		devIndex = devIndex + 1
	end

	ret = color_validation.validateTestSummary(numberOfDevices, tTestSummary)
	strXml = generate_xml.generate_xml(tTestSummary)

	return ret, strXml
end



function swapUp(sCurSerial)
	led_analyzer.swap_up(asSerials, sCurSerial)
	tStrSerials = color_conversions.astring2table(asSerials, numberOfDevices)
	return tStrSerials
end



function swapDown(sCurSerial)
	led_analyzer.swap_down(asSerials, sCurSerial)
	tStrSerials = color_conversions.astring2table(asSerials, numberOfDevices)
	return tStrSerials
end



function setGainX(iDeviceIndex, iSensorIndex, gain)
	return led_analyzer.set_gain_x(apHandles, iDeviceIndex, iSensorIndex, gain)
end



function setIntTimeX(iDeviceIndex, iSensorIndex, intTime)
	return led_analyzer.set_intTime_x(apHandles, iDeviceIndex, iSensorIndex, intTime)
end



function setSettings(intTime, gain)
	local devIndex = 0
	local ret
	while(devIndex < numberOfDevices) do
		if gain ~= nil then
			ret = led_analyzer.set_gain(apHandles, devIndex, gain)
		end
		if intTime ~= nil then
			ret = led_analyzer.set_intTime(apHandles, devIndex, intTime)
		end
		devIndex = devIndex + 1
	end
	return ret
end



-- don't forget to clean up after every test --
function free()
	if fArraysCreated==true then
		-- CLEAN UP --
		led_analyzer.free_devices(apHandles)
		led_analyzer.delete_ushort(ausClear)
		led_analyzer.delete_ushort(ausRed)
		led_analyzer.delete_ushort(ausGreen)
		led_analyzer.delete_ushort(ausBlue)
		led_analyzer.delete_ushort(ausCCT)
		led_analyzer.delete_afloat(afLUX)
		led_analyzer.delete_puchar(aucGains)
		led_analyzer.delete_puchar(aucIntTimes)
		led_analyzer.delete_apvoid(apHandles)
		led_analyzer.delete_astring(asSerials)

		ausClear          = nil
		ausRed            = nil
		ausGreen          = nil
		ausBlue           = nil
		ausCCT            = nil
		afLUX             = nil
		-- system settings from tcs3472 will be stored in following arrays --
		aucGains          = nil
		aucIntTimes       = nil
		-- serial numbers of connected color controller(s) will be stored in asSerials --
		asSerials         = nil
		tStrSerials       = {}
		-- handle to all connected color controller(s) will be stored in apHandles (note 2 handles per device)
		apHandles         = nil
		-- global table contains all color and light related data / global for easy access by C
		tColorTable             = {}
		-- number of 1 scanned / 2 connected devices
		numberOfDevices   = 0
		-- table that contains a test summary for each device --
		tTestSummary      = nil

		fArraysCreated = false
	end
end

