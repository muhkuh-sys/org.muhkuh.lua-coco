-- Create the color_control class.
local class = require "pl.class"

---
-- @type color_control
local Color_conversions = class()

function Color_conversions:_init()
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
		require "log.writer.prefix".new("[COCO COLOR_CONVERSION] ", tLogWriter),
		-- formatter
		require "log.formatter.format".new()
	)

	self.tLog = tLog
	
	local led_analyzer = require("led_analyzer")
	self.led_analyzer = led_analyzer

	local pl = require "pl.import_into"()
	self.pl = pl

	local tcs_chromaTable = require("tcs_chromaTable")
	self.tcs_chromaTable = tcs_chromaTable

	local MIN_LUX = 4.0
	local MIN_CLEAR = 0.0008 -- Minimum Clear Level as percentage of maximum clear

	self.MIN_LUX = MIN_LUX
	self.MIN_CLEAR = MIN_CLEAR

	-- tcs3472 specific settings for integration time
	local auiIntegration_timeINTEGRATION = {
		TCS3472_INTEGRATION_2_4ms = 0xFF,
		TCS3472_INTEGRATION_24ms = 0xF6,
		TCS3472_INTEGRATION_100ms = 0xD6,
		TCS3472_INTEGRATION_154ms = 0xC0,
		TCS3472_INTEGRATION_200ms = 0xAD,
		TCS3472_INTEGRATION_700ms = 0x00
	}

	self.auiIntegration_timeINTEGRATION = auiIntegration_timeINTEGRATION

	-- tcs3472 specific settings for gain
	local auiTCS3472_GAIN = {
		TCS3472_GAIN_1X = 0x00,
		TCS3472_GAIN_4X = 0x01,
		TCS3472_GAIN_16X = 0x02,
		TCS3472_GAIN_60X = 0x03
	}
	self.auiTCS3472_GAIN = auiTCS3472_GAIN

end

-- Gets the maximum clear level corresponding to a given integration time of the tcs3472 sensor
-- Datasheet TCS3472 RGBC Timing Register(0x01) 
function Color_conversions:maxClear(integrationTime)
	local tLog = self.tLog

	if integrationTime == self.auiIntegration_timeINTEGRATION["TCS3472_INTEGRATION_2_4ms"] then
		return 1024
	elseif integrationTime == self.auiIntegration_timeINTEGRATION["TCS3472_INTEGRATION_24ms"] then
		return 10240
	elseif integrationTime == self.auiIntegration_timeINTEGRATION["TCS3472_INTEGRATION_100ms"] then
		return 43008
	elseif integrationTime == self.auiIntegration_timeINTEGRATION["TCS3472_INTEGRATION_154ms"] then
		return 65535
	elseif integrationTime == self.auiIntegration_timeINTEGRATION["TCS3472_INTEGRATION_200ms"] then
		return 65535
	elseif integrationTime == self.auiIntegration_timeINTEGRATION["TCS3472_INTEGRATION_700ms"] then
		return 65535
	else
		tLog.info("Unknown Integration Time - we take default 65535")
		return 65535
	end
end

-- Convert RGB to XYZ -- only with sRGB due to the tranformation of the sensor measurement values with the sRGB transformation matrix
-- r from 0.0 to 1.0
-- g from 0.0 to 1.0
-- b from 0.0 to 1.0
function Color_conversions:RGB2XYZ(r, g, b, rgb_workingspace)
	local r_n = r
	local g_n = g
	local b_n = b


	-- gamma decoding of SRGB space
	if r_n>0.04045 then
		r_n = math.pow((r_n + 0.055) / 1.055, 2.4)
	else
		r_n = r_n / 12.92
	end

	if g_n > 0.04045 then
		g_n = math.pow((g_n + 0.055) / 1.055, 2.4)
	else
		g_n = g_n / 12.92
	end

	if b_n > 0.04045 then
		b_n = math.pow((b_n + 0.055) / 1.055, 2.4)
	else
		b_n = b_n / 12.92
	end


	local tXYZ = {}

	-- CIE Standard Observer 2 °, Daylight
	if rgb_workingspace == "sRGB" then
		-- CIE RGB, Observer = 2°, Illuminant = E
		--Source White in XYZ units not Yxy units !
		self.tRefWhite = {x = 0.312727, y = 0.329023}
		tXYZ = {
			x = r_n * 0.4124564 + g_n * 0.3575761 + b_n * 0.1804375,
			y = r_n * 0.2126729 + g_n * 0.7151522 + b_n * 0.0721750,
			z = r_n * 0.0193339 + g_n * 0.1191920 + b_n * 0.9503041
		}
	end

	-- elseif rgb_workingspace == "CIE_RGB" then
	-- 	-- Apple RGB, Observer = 2°, Illuminant = D65
	-- 	self.tRefWhite = {x = 0.3333, y = 0.3333}
	-- 	tXYZ = {
	-- 		x = r_n * 0.4887180 + g_n * 0.3106803 + b_n * 0.2006017,
	-- 		y = r_n * 0.1762044 + g_n * 0.8129847 + b_n * 0.0108109,
	-- 		z = r_n * 0.0000000 + g_n * 0.0102048 + b_n * 0.989795
	-- 	}
	-- elseif rgb_workingspace == "Apple_RGB" then
	-- 	-- Best RGB, Observer = 2°, Illuminant = D50
	-- 	self.tRefWhite = {x = 0.312727, y = 0.329023}
	-- 	tXYZ = {
	-- 		x = r_n * 0.4497288 + g_n * 0.3162486 + b_n * 0.1844926,
	-- 		y = r_n * 0.2446525 + g_n * 0.6720283 + b_n * 0.0833192,
	-- 		z = r_n * 0.0251848 + g_n * 0.1411824 + b_n * 0.9224628
	-- 	}
	-- elseif rgb_workingspace == "Best_RGB" then
	-- 	-- Beta RGB , Observere 2°, Illuminant D50
	-- 	self.tRefWhite = {x = 0.34567, y = 0.35850}
	-- 	tXYZ = {
	-- 		x = r_n * 0.6326696 + g_n * 0.2045558 + b_n * 0.1269946,
	-- 		y = r_n * 0.2284569 + g_n * 0.7373523 + b_n * 0.0341908,
	-- 		z = r_n * 0.0000000 + g_n * 0.0095142 + b_n * 0.8156958
	-- 	}
	-- elseif rgb_workingspace == "Beta_RGB" then
	-- 	-- Bruce RGB, Observer 2°, Illuminant D65
	-- 	self.tRefWhite = {x = 0.34567, y = 0.35850}
	-- 	tXYZ = {
	-- 		x = r_n * 0.6712537 + g_n * 0.1745834 + b_n * 0.1183829,
	-- 		y = r_n * 0.3032726 + g_n * 0.6637861 + b_n * 0.0329413,
	-- 		z = r_n * 0.0000000 + g_n * 0.0407010 + b_n * 0.7845090
	-- 	}
	-- elseif rgb_workingspace == "Bruce_RGB" then
	-- 	-- Color Match RGB, Observer = 2°, Illuminant = D50
	-- 	self.tRefWhite = {x = 0.312727, y = 0.329023}
	-- 	tXYZ = {
	-- 		x = r_n * 0.4674162 + g_n * 0.2944512 + b_n * 0.1886026,
	-- 		y = r_n * 0.2410115 + g_n * 0.6835475 + b_n * 0.0754410,
	-- 		z = r_n * 0.0219101 + g_n * 0.0736128 + b_n * 0.9933071
	-- 	}
	-- elseif rgb_workingspace == "D50" then
	-- 	-- NTSCRGB, Observer = 2°, Illuminant = C
	-- 	self.tRefWhite = {x = 0.34567, y = 0.35850}
	-- 	tXYZ = {
	-- 		x = r_n * 0.5093439 + g_n * 0.3209071 + b_n * 0.1339691,
	-- 		y = r_n * 0.2748840 + g_n * 0.6581315 + b_n * 0.0669845,
	-- 		z = r_n * 0.0242545 + g_n * 0.1087821 + b_n * 0.6921735
	-- 	}
	-- elseif rgb_workingspace == "NTSC_RGB" then
	-- 	-- NTSCRGB, Observer = 2°, Illuminant = C
	-- 	self.tRefWhite = {x = 0.31006, y = 0.31616}
	-- 	tXYZ = {
	-- 		x = r_n * 0.6068909 + g_n * 0.1735011 + b_n * 0.2003480,
	-- 		y = r_n * 0.2989164 + g_n * 0.5865990 + b_n * 0.1144845,
	-- 		z = r_n * 0.0000000 + g_n * 0.0660957 + b_n * 1.1162243
	-- 	}
	-- elseif rgb_workingspace == "PAL_RGB" then
	-- 	-- Adobe RGB using D65 as reference white --
	-- 	self.tRefWhite = {x = 0.31273, y = 0.32902}
	-- 	tXYZ = {
	-- 		x = r_n * 0.4306190 + g_n * 0.3415419 + b_n * 0.1783091,
	-- 		y = r_n * 0.2220379 + g_n * 0.7066384 + b_n * 0.0713236,
	-- 		z = r_n * 0.0201853 + g_n * 0.1295504 + b_n * 0.9390944
	-- 	}
	-- elseif rgb_workingspace == "Adobe" then
	-- 	-- Default Reference white and sRGB if wrong or unknown "rgb_workingspace"
	-- 	self.tRefWhite = {x = 0.312727, y = 0.329023}
	-- 	tXYZ = {
	-- 		x = r_n * 0.5767309 + g_n * 0.1855540 + b_n * 0.1881852,
	-- 		y = r_n * 0.2973769 + g_n * 0.6273491 + b_n * 0.0752741,
	-- 		z = r_n * 0.0270343 + g_n * 0.0706872 + b_n * 0.9911085
	-- 	}
	-- else -- sRGB ??
	-- 	self.tRefWhite = {x = 0.312727, y = 0.329023}
	-- 	tXYZ = {
	-- 		x = r_n * 0.4124564 + g_n * 0.3575761 + b_n * 0.1804375,
	-- 		y = r_n * 0.2126729 + g_n * 0.7151522 + b_n * 0.0721750,
	-- 		z = r_n * 0.0193339 + g_n * 0.1191920 + b_n * 0.9503041
	-- 	}
	-- end

	return tXYZ.x, tXYZ.y, tXYZ.z
end

-- X,Y,Z in the nominal range [0.0, 1.0]
local function XYZ2Yxy(X, Y, Z)
	-- avoid division by zero --
	if ((X == 0) and (Y == 0) and (Z == 0)) then
		return 0, 0, 0
	end

	local Y = Y
	local x = X / (X + Y + Z)
	local y = Y / (X + Y + Z)

	return Y, x, y
end

-- Returns HSV value of RGB inputs --
-- @param r,g,b: Red, Green and Blue values from 0 to 1
-- @return H,S,V --> Hue, Saturation and Value from 0 to 360, 0 to 100, and 0 to 100
local function RGB2HSV(r, g, b)
	local r = r
	local g = g
	local b = b

	if r > 1.0 then
		r = 1.0
	end

	if g > 1.0 then
		g = 1.0
	end

	if b > 1.0 then
		b = 1.0
	end

	local max = math.max(r, g, b)
	local min = math.min(r, g, b)
	local v = max
	local d = max - min
	local s
	if max == 0 then
		s = 0
	else
		s = d / max
	end

	local h = 0
	if max ~= min then
		local _exp_0 = max
		if r == _exp_0 then
			h = (g - b) / d + ((function()
					if g < b then
						return 6
					else
						return 0
					end
				end)())
		elseif g == _exp_0 then
			h = (b - r) / d + 2
		elseif b == _exp_0 then
			h = (r - g) / d + 4
		end
		h = h / 6
	end

	return h * 360, s * 100, v * 100
end

-- Returns the angle between two vectors --
local function get_angle(v1, v2)
	local absv1 = math.sqrt(math.pow(v1.x, 2) + math.pow(v1.y, 2))
	local absv2 = math.sqrt(math.pow(v2.x, 2) + math.pow(v2.y, 2))
	-- local absv1 =  math.pow(v1.x, 2) + math.pow(v1.y, 2)) --math.sqrt(v1.x^2 + v1.y^2)
	-- local absv2 =  math.pow(v2.x, 2) + math.pow(v2.y, 2)) --math.sqrt(v2.x^2 + v2.y^2)
	local scalarprodukt = (v1.x * v2.x) + (v1.y * v2.y)

	return math.acos((scalarprodukt / (absv1 * absv2)))
end

-- calculate the length of a direction vector
-- length means absolute distance from .x .y coordinates to whitePoint.x .y
local function get_length(directionVector)
	if (directionVector.x == nil or directionVector.y == nil) then
		directionVector.x = 0
		directionVector.y = 0
	end

	return math.sqrt(math.pow(directionVector.x,2) + math.pow(directionVector.y,2))
	-- return math.sqrt(directionVector.x^2 + directionVector.y^2)   -- math.pow(directionVector.x, 2) + math.pow(directionVector.y, 2))
end

--Returns the dominant wavelength of input parameters x,y
--instead of using the idealized CIE1931 2° Observer Curver we use the spectral sensitivity data
--of our sensor and thus achieve a much better accuracy
function Color_conversions:Yxy2wavelength(x, y)
	-- if too dark return zeros --
	if ((x == 0) and (y == 0)) then
		return 0, 0
	end

	local t_curDirVector = {}

	-- use global tRefWhite table to get ur refwhite values which depend on the rgb work space
	local refWhitex = self.tRefWhite.x
	local refWhitey = self.tRefWhite.y


	-- Construct direction vector from current x and y input values
	t_curDirVector.x = (x - refWhitex)
	t_curDirVector.y = (y - refWhitey)

	-- Algorithm determines which direction vector in tTCS_dirVector is closest to the direction vector
	-- given by the current x,y pair by calculating their absolute angle variance

	-- Get smallest angle variance
	local min_angle = 2 * math.pi -- Set the initial min angle to a max value
	local cur_angle = 2 * math.pi -- Set the initial current angle to a max value
	local min_index = 0 -- Set the initial index to an invalid value

	for i = 1, 817 do
		cur_angle = math.abs(get_angle(t_curDirVector, self.tcs_chromaTable.tTCS_dirVector[i]))
		if cur_angle < min_angle then
			min_index = i
			min_angle = cur_angle
		end
	end

	local saturation = get_length(t_curDirVector) / get_length(self.tcs_chromaTable.tTCS_dirVector[min_index])
	-- As 100 % should be the max saturation possible, cap your saturation in case it gets over 1.0 ( == 100 % )
	if saturation >= 1.0 then
		saturation = 1.0
	end

	return self.tcs_chromaTable.tTCS_Chromaticity[min_index].nm, saturation
end

-- returns the divisor for the value in the gain register for tcs3472
-- a register value of 0 means gain_divisor = 1
-- a register value of 1 means gain_divisor = 4 .. and so on
function Color_conversions:getGainDivisor(gain)
	if gain == self.auiTCS3472_GAIN["TCS3472_GAIN_1X"] then
		return 1
	elseif gain == self.auiTCS3472_GAIN["TCS3472_GAIN_4X"] then
		return 4
	elseif gain == self.auiTCS3472_GAIN["TCS3472_GAIN_16X"] then
		return 16
	elseif gain == self.auiTCS3472_GAIN["TCS3472_GAIN_60X"] then
		return 60
	else
		-- unknown register value ??
		return 1
	end
end

-- function calculates the lux levels and cct (correleated colour temperature)
-- Illumination: --> unit: LUX      CCT: --> unit: degrees Kelvin)
-- input parameters are tables which contain 16 values each (as 16 sensors exist per device)
-- the calculation itself is specific to the spectral responsitivity of tcs3472, the calculation sheet
-- can be acquired from ams (DN40 - Lux and CCT Calculations)
function Color_conversions:calculate_CCT_LUX(red, green, blue, clear, integrationTime, gain)
	-- color temperature and LUX
	local CCT
	local LUX
	-- some magic numbers from the Design Note:
	local R_Coef = 0.136
	local G_Coef = 1.0000000
	local B_Coef = -0.444

	local CT_Coef = 3810
	local CT_Offset = 1391

	local IR_Content
	local CPL = 0 -- counts per lux
	local GA = 1.0 -- device attenuation
	local device_factor = 310
	local DGF = device_factor * GA -- combined device factor and glass attenuation

	-- remove the IR content from the rgb values
	IR_Content = (red + green + blue - clear) / 2
	red = red - IR_Content -- R' (removed IR)
	if red < 0 then
		red = 0
	end
	green = green - IR_Content -- G' (removed IR)
	if green < 0 then
		green = 0
	end
	blue = blue - IR_Content -- B' (removed IR)
	if blue < 0 then
		blue = 0
	end

	-- Color temperature calculation (avoid division by zero):
	if red > 0 then
		CCT = CT_Coef * (blue / red) + CT_Offset
	else
		CCT = 0
	end

	-- LUX calculation:
	CPL = ((256 - integrationTime) * 2.4) * self:getGainDivisor(gain) / (DGF)
	LUX = ((R_Coef * red) + (G_Coef * green) + (B_Coef * blue)) / CPL

	-- blue leds produce a negative lux ... so turn it around
	if LUX < 0 then
		LUX = LUX * (-1)
	end

	return LUX, CCT
end


-- Saves content of a string array into a lua table
function Color_conversions:astring2table(astring, numbOfSerials)
	local tSerialnumbers = {}

	for i = 0, numbOfSerials do
		-- print('astring_getitem: ',self.led_analyzer.astring_getitem(astring, i))
		if self.led_analyzer.astring_getitem(astring, i) ~= nil then
			tSerialnumbers[i + 1] = self.led_analyzer.astring_getitem(astring, i)
		end
	end

	return tSerialnumbers
end

-- Saves content of a string table into a string array
function Color_conversions:table2astring(tString, aString, MAXSERIALS)
	local pl = self.pl
	local iResult
	local numberOfEntries
	local tLog = self.tLog

	numberOfEntries = pl.tablex.size(tString)

	-- be optimistic
	iResult = 0

	if numberOfEntries > MAXSERIALS then
		tLog.error("Number of elements in tString exceeds maximum amounts of possible serial numbers!\n")
		iResult = -1
		return iResult
	end

	for i = 1, numberOfEntries do
		self.led_analyzer.astring_setitem(aString, i - 1, tString[i])
	end


	return iResult
end

-- Convert the Colors given as parameters into various color spaces (RGB, HSV, XYZ, Yxy, Wavelength)
-- and save the values of the color spaces into tables
function Color_conversions:aus2colorTable(clear, red, green, blue, intTimes, gain, length) 
	-- tables containing colors in different color spaces
	local tRGB = {}
	local tXYZ = {}
	local tYxy = {}
	local tWavelength = {}
	local tHSV = {}
	local tSettings = {}
	local tColorTable = {}

	-- local values for clear, red, green and blue channel
	local lClear, lRed, lGreen, lBlue
	local r_n, g_n, b_n
	local lGain, lIntTime
	local lCCT, lLUX
	local lClearRatio

	for i = 0, length - 1 do
		-- Get your current colors and save them into tables
		lClear = self.led_analyzer.ushort_getitem(clear, i)
		lRed = self.led_analyzer.ushort_getitem(red, i)
		lGreen = self.led_analyzer.ushort_getitem(green, i)
		lBlue = self.led_analyzer.ushort_getitem(blue, i)

		-- Settings like Gain and Integration Time
		lGain = self.led_analyzer.puchar_getitem(gain, i)
		lIntTime = self.led_analyzer.puchar_getitem(intTimes, i)

		-- ratio of measured clear channel count to max clear channel count (dependent on gain and integration time)
		lClearRatio = lClear / self:maxClear(lIntTime)

		-- get lux and cct, cct is not used/needed yet
		lLUX, lCCT = self:calculate_CCT_LUX(lRed, lGreen, lBlue, lClear, lIntTime, lGain)

		-- to avoid a later division by zero and to have more stable readings and no unneccessary
		-- outputs from channels that are not reading any LEDs we set a minum lux level

		-- if measured brightness (lux) falls beneath required MINIMUM LUX or our clear channel gives zero
		-- fill all color tables with zero values, beside the lux value.
		if (lClearRatio < self.MIN_CLEAR or lLUX < self.MIN_LUX) then
			-- RGB table
			tRGB[i + 1] = {
				clear_tsc = 0,
				red_tsc = 0,
				green_tsc = 0,
				blue_tsc = 0
			}

			-- RGB normalized
			tXYZ[i + 1] = {
				X = 0,
				Y = 0,
				Z = 0
			}

			-- Yxy space
			tYxy[i + 1] = {
				Y = 0,
				x = 0,
				y = 0
			}

			-- Wavelength and saturation
			tWavelength[i + 1] = {
				nm = 0,
				sat = 0,
				lux = lLUX,
				cct = 0,
				r_estimate = 0,
				g_estimate = 0,
				b_estimate = 0,
				clear_ratio = 100 * lClearRatio
			}

			tHSV[i + 1] = {
				H = 0,
				S = 0,
				V = 0
			}

			tSettings[i + 1] = {
				gain = lGain,
				intTime = lIntTime
			}
		else
			r_n = lRed / lClear
			g_n = lGreen / lClear
			b_n = lBlue / lClear

			-- RGB table
			tRGB[i + 1] = {
				clear = lClear,
				red = lRed,
				green = lGreen,
				blue = lBlue,
				R_n = r_n,
				G_n = g_n,
				B_n = b_n
			}

			local X, Y, Z = self:RGB2XYZ(r_n, g_n, b_n, "sRGB")

			-- XYZ table
			tXYZ[i + 1] = {
				X = X,
				Y = Y,
				Z = Z
			}

			local Y, x, y = XYZ2Yxy(X, Y, Z)

			-- Yxy table
			tYxy[i + 1] = {
				Y = Y,
				x = x,
				y = y
			}

			local wavelength, saturation = self:Yxy2wavelength(x, y)
			-- local wR, wG, wB = wavelength2RGB(wavelength)

			-- Wavelength Saturation Brightness table
			tWavelength[i + 1] = {
				nm = math.floor(wavelength + 0.5),
				sat = saturation * 100,
				lux = lLUX,
				cct = lCCT,
				-- r_estimate = wR,
				-- g_estimate = wG,
				-- b_estimate = wB,
				clear_ratio = 100 * lClearRatio
			}

			local H, S, V = RGB2HSV(r_n, g_n, b_n)

			-- HSV table
			tHSV[i + 1] = {
				H = H,
				S = S,
				V = V
			}

			-- Settings --
			tSettings[i + 1] = {
				gain = lGain,
				intTime = lIntTime
			}
		end

		tColorTable[i + 1] = {
			Wavelength = tWavelength[i + 1],
			RGB_tsc = tRGB[i + 1],
			XYZ = tXYZ[i + 1],
			Yxy = tYxy[i + 1],
			HSV = tHSV[i + 1],
			Settings = tSettings[i + 1]
		}
	end

	return tColorTable
end

-- Gets the gain and integration time settings and stores them into a settings table with form
-- table[sensor] = {gain = ..., intTime = ...}
function Color_conversions:auc2settingsTable(aucIntegrationtimes, aucGains, length)
	local tEntry = {}
	local lGain
	local lIntTime

	for i = 0, length - 1 do
		lIntTime = self.led_analyzer.puchar_getitem(aucIntegrationtimes, i)
		lGain = self.led_analyzer.puchar_getitem(aucGains, i)

		-- print('index:',i,' - aucIntegrationtimes: ',lIntTime)
		-- print('index:',i,'- aucGains: ',i,lGain)

		tEntry[i + 1] = {
			gain = lGain,
			intTime = lIntTime
		}
	end

	return tEntry
end

--[[
-- Convert XYZ to LAB --
function XYZToLAB(x, y, z)
  local whiteX = 95.047
  local whiteY = 100.0
  local whiteZ = 108.883
  local x = xyz.x / whiteX
  local y = xyz.y / whiteY
  local z = xyz.z / whiteZ
  if x > 0.008856451679 then
    x = math.pow(x, 0.3333333333)
  else
    x = (7.787037037 * x) + 0.1379310345
  end
  if y > 0.008856451679 then
    y = math.pow(y, 0.3333333333)
  else
    y = (7.787037037 * y) + 0.1379310345
  end
  if z > 0.008856451679 then
    z = math.pow(z, 0.3333333333)
  else
    z = (7.787037037 * z) + 0.1379310345
  end
  local l = 116 * y - 16
  local a = 500 * (x - y)
  local b = 200 * (y - z)
  return {
    l = l,
    a = a,
    b = b
  }
end
--]]

--[[
-- function chromatic addaption --
function chromatic_adaption(tSourceWhite, tDestWhite, tXYZ, mode)

	local M  = {[1] = {}, [2] = {}, [3] = {}}
	local Mi = {[1] = {}, [2] = {}, [3] = {}}
	local Mcone = {[1] = {}, [2] = {}, [3] = {}}
	
	if mode == "XYZ_Scaling" then
	
		M[1] ={[1] = 1.0000000, [2] = 0.0000000, [3] = 0.0000000}
		M[2] ={[1] = 0.0000000, [2] = 1.0000000, [3] = 0.0000000}
		M[3] ={[1] = 0.0000000, [2] = 0.0000000, [3] = 1.0000000}
		
		Mi[1] ={[1] = 1.0000000, [2] = 0.0000000, [3] = 0.0000000}
		Mi[2] ={[1] = 0.0000000, [2] = 1.0000000, [3] = 0.0000000}
		Mi[3] ={[1] = 0.0000000, [2] = 0.0000000, [3] = 1.0000000}
	
	elseif mode == "Bradford" then
	
		M[1] ={[1] =  0.8951000, [2] =  0.2664000, [3] =  -0.1614000}
		M[2] ={[1] = -0.7502000, [2] =  1.7135000, [3] =   0.0367000}
		M[3] ={[1] =  0.0389000, [2] = -0.0685000, [3] =   1.0296000}	
		                              
		Mi[1] ={[1] = 0.9869929,  [2] =  -0.1470543 ,[3] =  0.1599627}
		Mi[2] ={[1] = 0.4323053,  [2] =  0.5183603  , [3] =  0.0492912}
		Mi[3] ={[1] = -0.0085287, [2] =  0.0400428  , [3] =  0.9684867}			
	
	
	elseif mode == "Von_Kries" then 
		--could implement von kries
		--not really needed as we dont use chromatic adaption 
	
	end 
	
	-- 3x1 vectors 
	local cone_source = MultMatVec(M, tSourceWhite)
	local cone_dest   = MultMatVec(M, tDestWhite)
	
	-- 3x3 matrice 
 	Mcone[1] ={[1] = cone_dest.x / cone_source.x, [2] = 0.0000000, [3] = 0.0000000}
	Mcone[2] ={[1] = 0.0000000, [2] = cone_dest.y / cone_source.y, [3] = 0.0000000}
	Mcone[3] ={[1] = 0.0000000, [2] = 0.0000000, [3] = cone_dest.z / cone_source.z}
	
	local Mtemp = MultMatMat(Mi, Mcone)
	
	local M_Transformation = MultMatMat(Mtemp, M)
	
	local xyz_dest = MultMatVec(M_Transformation, tXYZ)
	
	return xyz_dest.x, xyz_dest.y, xyz_dest.z
	

end 
--]]


return Color_conversions
