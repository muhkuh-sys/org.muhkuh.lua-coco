module("color_conversions", package.seeall)

require("tcs_chromaTable")

local MIN_LUX   = 4.0
local MIN_CLEAR = 0.0008 -- Minimum Clear Level as percentage of maximum clear

--[[
-- a helper to print a colortable which contains values in RGB, XYZ, HSV, Yxy and wavelength color space 
-- parameter space determines which space should be printed out 
function print_color(devIndex, colortable, length, space)
	print(string.format("------------- Colors - Device %2d --------------- ", devIndex))
	
	
	if space == "RGB" then 
		print("     Clear   Red     Green    Blue")
		for i=1,length do
		   print(string.format("%2d - 0x%04x 0x%04x 0x%04x 0x%04x", i, colortable[devIndex][2][i].clear,
													colortable[devIndex][2][i].red, colortable[devIndex][2][i].green,
													colortable[devIndex][2][i].blue))
		end
		
	
	elseif space == "RGB_n" then
		print("     Clear   Red     Green    Blue")
		for i=1,length do
			-- Avoid division by zero 
		    if(colortable[devIndex][2][i].clear == 0) then 
				print(string.format("%2d - 0x%04x %3.5f %3.5f %3.5f", i, 0, 0, 0, 0))
		    else 
				print(string.format("%2d - 0x%04x %.5f %.5f %.5f", i, colortable[devIndex][2][i].clear,
													(colortable[devIndex][2][i].red/colortable[devIndex][2][i].clear)*255, 
													(colortable[devIndex][2][i].green/colortable[devIndex][2][i].clear)*255,
													(colortable[devIndex][2][i].blue/colortable[devIndex][2][i].clear)*255))
		    end 
		end
		
	elseif space == "XYZ" then
		print("     X        Y	       Z    ")
		for i=1, length do
			print(string.format("%2d - %.5f %.5f %.5f", i, colortable[devIndex][3][i].X,
											colortable[devIndex][3][i].Y, colortable[devIndex][3][i].Z))
		end
		
	elseif space == "Yxy" then
		print("     Y        x	       y    ")
		for i=1, length do
			print(string.format("%2d - %.7f %.7f %.7f", i, colortable[devIndex][4][i].Y,
											colortable[devIndex][4][i].x, colortable[devIndex][4][i].y))
		end
		
	-- if no colorspace is given, we will print out wavelengths as default
	elseif space == "wavelength" or space == nil then
	    print(" dominant wavelength	 sat         LUX  ")
		for i=1, length do
			print(string.format("%3d)   %3d nm		%3.2f	   %4.3f	", i, colortable[devIndex][1][i].nm, colortable[devIndex][1][i].sat, colortable[devIndex][1][i].lux))
		end								
	
	elseif space == "HSV" then
	print("     H        S	       V    ")
		for i=1,length do
		   print(string.format("%2d -  %3.2f    %3.2f    %3.2f", i, 
													colortable[devIndex][5][i].H,
													colortable[devIndex][5][i].S,
													colortable[devIndex][5][i].V))
		end	
	end
	print("\n")
end
--]]



local function round(num, idp)
	local mult = 10^(idp or 0)
	return math.floor(num * mult + 0.5) / mult
end



-- Gets the maximum clear level corresponding to a given integration time of the tcs3472 sensor 
local function maxClear(integrationTime)
	if     integrationTime == TCS3472_INTEGRATION_2_4ms then
		return 1024

	elseif integrationTime == TCS3472_INTEGRATION_24ms then
		return 10240

	elseif integrationTime == TCS3472_INTEGRATION_100ms then
		return 43008

	elseif integrationTime == TCS3472_INTEGRATION_154ms then
		return 65535

	elseif integrationTime == TCS3472_INTEGRATION_200ms then
		return 65535

	elseif integrationTime == TCS3472_INTEGRATION_700ms then
		return 65535

	else
		print(string.format("Unknown Integration Time - we take default 65535"))
		return 65535
	end
end



-- Convert RGB to XYZ
-- r from 0.0 to 1.0
-- g from 0.0 to 1.0
-- b from 0.0 to 1.0
local function RGB2XYZ(r, g, b, rgb_workingspace)
	local r_n = r
	local g_n = g
	local b_n = b


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
	if rgb_workingspace == 'sRGB' then
		--Source White in XYZ units not Yxy units !
		tRefWhite = {x = 0.312727, y = 0.329023}
		tXYZ = { x = r_n * 0.4124564 + g_n * 0.3575761 + b_n * 0.1804375,
		         y = r_n * 0.2126729 + g_n * 0.7151522 + b_n * 0.0721750,
		         z = r_n * 0.0193339 + g_n * 0.1191920 + b_n * 0.9503041 }

	-- CIE RGB, Observer = 2°, Illuminant = E
	elseif rgb_workingspace == 'CIE_RGB' then
		tRefWhite = {x = 0.3333, y = 0.3333}
		tXYZ = { x = r_n * 0.4887180 + g_n * 0.3106803  + b_n * 0.2006017,
		         y = r_n * 0.1762044 + g_n * 0.8129847  + b_n * 0.0108109,
		         z = r_n * 0.0000000 + g_n * 0.0102048  + b_n * 0.989795 }

	-- Apple RGB, Observer = 2°, Illuminant = D65
	elseif rgb_workingspace == 'Apple_RGB' then
		tRefWhite = {x = 0.312727, y = 0.329023}
		tXYZ = { x = r_n *0.4497288 + g_n * 0.3162486  + b_n *0.1844926,
		         y = r_n *0.2446525 + g_n * 0.6720283  + b_n *0.0833192,
		         z = r_n *0.0251848 + g_n * 0.1411824  + b_n *0.9224628 }

	-- Best RGB, Observer = 2°, Illuminant = D50
	elseif rgb_workingspace == 'Best_RGB' then
		tRefWhite = {x = 0.34567, y = 0.35850}
		tXYZ = { x = r_n * 0.6326696  + g_n * 0.2045558  + b_n * 0.1269946,
		         y = r_n * 0.2284569  + g_n * 0.7373523  + b_n * 0.0341908,
		         z = r_n * 0.0000000  + g_n * 0.0095142  + b_n * 0.8156958 }

	-- Beta RGB , Observere 2°, Illuminant D50
	elseif rgb_workingspace == 'Beta_RGB' then
		tRefWhite = {x = 0.34567, y = 0.35850}
		tXYZ = { x = r_n * 0.6712537  + g_n * 0.1745834  + b_n * 0.1183829,
		         y = r_n * 0.3032726  + g_n * 0.6637861  + b_n * 0.0329413,
		         z = r_n * 0.0000000  + g_n * 0.0407010  + b_n * 0.7845090 }

	-- Bruce RGB, Observer 2°, Illuminant D65
	elseif rgb_workingspace == 'Bruce_RGB' then
		tRefWhite = {x = 0.312727, y = 0.329023}
		tXYZ = { x = r_n *  0.4674162  + g_n * 0.2944512  + b_n * 0.1886026,
		         y = r_n *  0.2410115  + g_n * 0.6835475  + b_n * 0.0754410,
		         z = r_n *  0.0219101  + g_n * 0.0736128  + b_n * 0.9933071 }

	-- Color Match RGB, Observer = 2°, Illuminant = D50
	elseif rgb_workingspace == 'D50' then
		tRefWhite = {x = 0.34567, y = 0.35850}
		tXYZ = { x = r_n * 0.5093439  + g_n * 0.3209071  + b_n * 0.1339691,
		         y = r_n * 0.2748840  + g_n * 0.6581315  + b_n * 0.0669845,
		         z = r_n * 0.0242545  + g_n * 0.1087821  + b_n * 0.6921735 }

	-- NTSCRGB, Observer = 2°, Illuminant = C
	elseif rgb_workingspace == 'NTSC_RGB' then
		tRefWhite = {x = 0.31006, y = 0.31616}
		tXYZ = { x = r_n * 0.6068909  + g_n * 0.1735011  + b_n * 0.2003480,
		         y = r_n * 0.2989164  + g_n * 0.5865990  + b_n * 0.1144845,
		         z = r_n * 0.0000000  + g_n * 0.0660957  + b_n * 1.1162243 }

	-- NTSCRGB, Observer = 2°, Illuminant = C
	elseif rgb_workingspace == 'PAL_RGB' then
		tRefWhite = {x = 0.31273, y = 0.32902}
		tXYZ = { x = r_n * 0.4306190   + g_n * 0.3415419  + b_n * 0.1783091,
		         y = r_n * 0.2220379   + g_n * 0.7066384  + b_n * 0.0713236,
		         z = r_n * 0.0201853   + g_n * 0.1295504  + b_n * 0.9390944 }

	-- Adobe RGB using D65 as reference white --
	elseif rgb_workingspace == "Adobe" then
		tRefWhite = {x = 0.312727, y=0.329023}
		tXYZ = { x = r_n * 0.5767309 + g_n * 0.1855540  + b_n *  0.1881852,
		         y = r_n * 0.2973769 + g_n * 0.6273491  + b_n *  0.0752741,
		         z = r_n * 0.0270343 + g_n * 0.0706872  + b_n *  0.9911085 }

	-- Default Reference white and sRGB if wrong or unknown "rgb_workingspace"
	else
		tRefWhite = {x = 0.312727, y = 0.329023}
		tXYZ = { x = r_n * 0.4124564 + g_n * 0.3575761 + b_n * 0.1804375,
		         y = r_n * 0.2126729 + g_n * 0.7151522 + b_n * 0.0721750,
		         z = r_n * 0.0193339 + g_n * 0.1191920 + b_n * 0.9503041 }
	end

	return tXYZ.x, tXYZ.y, tXYZ.z
end



-- X,Y,Z in the nominal range [0.0, 1.0]
local function XYZ2Yxy(X, Y, Z)
	-- avoid division by zero --
	if ((X == 0) and (Y == 0) and (Z == 0)) then
		return 0, 0, 0
	end

	local Y = Y
	local x = X / ( X + Y + Z )
	local y = Y / ( X + Y + Z )

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

	return h*360, s*100, v*100
end



local function MultMatVec(mat, vec)
	local v = {x = 0,y=0,z=0}

	v.x = mat[1][1]*vec.x + mat[1][2]*vec.y + mat[1][3]*vec.z
	v.y = mat[2][1]*vec.x + mat[2][2]*vec.y + mat[2][3]*vec.z
	v.z = mat[3][1]*vec.x + mat[3][2]*vec.y + mat[3][3]*vec.z

	return v
end



local function MultMatMat(mat1, mat2)
	local M  = {[1] = {}, [2] = {}, [3] = {}}

	M[1] ={[1] = mat1[1][1]*mat2[1][1] + mat1[1][2]*mat2[2][1] + mat1[1][3]*mat2[3][1],
	       [2] = mat1[1][1]*mat2[1][2] + mat1[1][2]*mat2[2][2] + mat1[1][3]*mat2[3][2],
	       [3] = mat1[1][1]*mat2[1][3] + mat1[1][2]*mat2[2][3] + mat1[1][3]*mat2[3][3] }

	M[2] ={[1] = mat1[2][1]*mat2[1][1] + mat1[2][2]*mat2[2][1] + mat1[2][3]*mat2[3][1],
	       [2] = mat1[2][1]*mat2[1][2] + mat1[2][2]*mat2[2][2] + mat1[2][3]*mat2[3][2],
	       [3] = mat1[2][1]*mat2[1][3] + mat1[2][2]*mat2[2][3] + mat1[2][3]*mat2[3][3] }

	M[3] ={[1] = mat1[3][1]*mat2[1][1] + mat1[3][2]*mat2[2][1] + mat1[3][3]*mat2[3][1],
	       [2] = mat1[3][1]*mat2[1][2] + mat1[3][2]*mat2[2][2] + mat1[3][3]*mat2[3][2],
	       [3] = mat1[3][1]*mat2[1][3] + mat1[3][2]*mat2[2][3] + mat1[3][3]*mat2[3][3] }

	return M
end



-- Returns the angle between two vectors --
local function get_angle(v1, v2)
	local absv1 = math.sqrt(math.pow(v1.x, 2) + math.pow(v1.y, 2))
	local absv2 = math.sqrt(math.pow(v2.x, 2) + math.pow(v2.y, 2))
	local scalarprodukt = (v1.x * v2.x) + (v1.y * v2.y)

	return  math.acos( ( scalarprodukt / (absv1 * absv2)))
end



-- calculate the length of a direction vector
-- length means absolute distance from .x .y coordinates to whitePoint.x .y
local function get_length(directionVector)
	if (directionVector.x == nil or directionVector.y == nil) then
		directionVector.x = 0
		directionVector.y = 0
	end

	return math.sqrt(math.pow(directionVector.x,2) + math.pow(directionVector.y,2))
end



--Returns the dominant wavelength of input parameters x,y
--instead of using the idealized CIE1931 2° Observer Curver we use the spectral sensitivity data
--of our sensor and thus achieve a much better accuracy
local function Yxy2wavelength(x,y)
	-- if too dark return zeros --
	if ((x == 0) and (y == 0)) then
		return 0, 0
	end

	local t_curDirVector = {}

	-- use global tRefWhite table to get ur refwhite values which depend on the rgb work space
	local refWhitex = tRefWhite.x
	local refWhitey = tRefWhite.y

	-- Construct direction vector from current x and y input values
	t_curDirVector.x = (x-refWhitex)
	t_curDirVector.y = (y-refWhitey)


	-- Algorithm determines which direction vector in tTCS_dirVector is closest to the direction vector
	-- given by the current x,y pair by calculating their absolute angle variance

	-- Get smalest angle variance
	local min_angle = 2*math.pi -- Set the initial min angle to a max value
	local cur_angle = 2*math.pi -- Set the initial current angle to a max value
	local min_index = 0         -- Set the initial index to an invalid value

	for i=1,817 do
		cur_angle = math.abs(get_angle(t_curDirVector, tcs_chromaTable.tTCS_dirVector[i]))
		if cur_angle < min_angle then
			min_index = i
			min_angle = cur_angle
		end
	end


	local saturation = get_length(t_curDirVector) / get_length(tcs_chromaTable.tTCS_dirVector[min_index])
	-- As 100 % should be the max saturation possible, cap your saturation in case it gets over 1.0 ( == 100 % )
	if saturation >= 1.0 then
		saturation = 1.0
	end

	return  tcs_chromaTable.tTCS_Chromaticity[min_index].nm, saturation
end



-- returns the divisor for the value in the gain register for tcs3472
-- a register value of 0 means gain_divisor = 1
-- a register value of 1 means gain_divisor = 4 .. and so on
local function getGainDivisor(gain)
	if gain == TCS3472_GAIN_1X then
		return 1
	elseif gain == TCS3472_GAIN_4X then
		return 4
	elseif gain == TCS3472_GAIN_16X then
		return 16
	elseif gain == TCS3472_GAIN_60X then
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
local function calculate_CCT_LUX(red, green, blue, clear, integrationTime, gain)
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
	local CPL = 0                   -- counts per lux
	local GA  = 1.0                 -- device attenuation
	local device_factor = 310
	local DGF = device_factor * GA  -- combined device factor and glass attenuation

	-- remove the IR content from the rgb values
	IR_Content = (red + green + blue - clear) / 2
	red   = red   - IR_Content              -- R' (removed IR)
	if red < 0 then
		red = 0
	end
	green = green - IR_Content              -- G' (removed IR)
	if green < 0 then
		green = 0
	end
	blue  = blue  - IR_Content              -- B' (removed IR)
	if blue < 0 then
		blue = 0
	end

	-- Color temperature calculation (avoid division by zero):
	if red > 0 then
		CCT = CT_Coef * ( blue / red ) + CT_Offset
	else
		CCT = 0
	end

	-- LUX calculation:
	CPL = ((256 - integrationTime) * 2.4) * getGainDivisor(gain) / (DGF)
	LUX = ((R_Coef * red) + (G_Coef * green) + (B_Coef * blue)) / CPL

	-- blue leds produce a negative lux ... so turn it around
	if LUX < 0 then
		LUX = LUX * (-1)
	end

	return LUX, CCT
end



local function adjustColor(color, factor)
	local Gamma = 0.8
	local IntensityMax = 255


	if color == 0.0 then
		return 0
	else
		return round(IntensityMax * math.pow(color * factor, Gamma))
	end
end



--Discussion
--The WaveLengthToRGB function is based on Dan Bruton's work (www.physics.sfasu.edu/astro/color.html)
local function wavelength2RGB(wavelength)
	local r = 0
	local g = 0
	local b = 0
	local factor


	-- round wavelength
	wavelength = math.floor(wavelength + 0.5)
	-- 380 ... 439
	if ((wavelength >= 380) and (wavelength <= 439)) then
		r = -(wavelength - 440) / (440 - 380)
		g = 0.0
		b = 1.0

	-- 440 ... 489
	elseif ((wavelength >= 440) and (wavelength <= 489)) then
		r = 0.0;
		g = (wavelength -440) / (490 - 440)
		b = 1.0

	-- 490 ... 509
	elseif ((wavelength >= 490) and (wavelength <= 509)) then
		r = 0.0
		g = 1.0
		b = -(wavelength - 510) / (510 - 490)

	-- 510 ... 579
	elseif ((wavelength >= 510) and (wavelength <= 579)) then
		r = (wavelength - 510) / (580 - 510)
		g = 1.0
		b = 0.0000000

	-- 580 ... 644
	elseif ((wavelength >= 580) and (wavelength <= 644)) then
		r = 1.0
		g = -(wavelength - 645) / (645 - 580)
		b = 0.0

	-- 645 ... 780
	elseif ((wavelength >= 645) and (wavelength <= 780)) then
		r = 1.0
		g = 0.0
		b = 0.0

	elseif (wavelength == 0) then
		r = 0.0
		g = 0.0
		b = 0.0
	end

	-- let the intensitiy fall off near the vision limits
	if ((wavelength >= 380) and (wavelength <= 419)) then
		factor = 0.3 + 0.7*(wavelength - 380) / (420 - 380)

	elseif ((wavelength >= 420) and (wavelength <= 700)) then
		factor = 1.0

	elseif ((wavelength >= 701) and (wavelength <= 780)) then
		factor = 0.3 + 0.7*(780 - wavelength) / (780 - 700)

	else
		factor = 0.0
	end

	r = adjustColor(r, factor)
	g = adjustColor(g, factor)
	b = adjustColor(b, factor)

	return r, g, b
end



-- Saves content of a string array into a lua table
function astring2table(astring, numbOfSerials)
	local tSerialnumbers = {}


	for i = 0, numbOfSerials do
		if led_analyzer.astring_getitem(astring, i) ~= NULL then
			tSerialnumbers[i+1] = led_analyzer.astring_getitem(astring, i)
		end
	end

	return tSerialnumbers
end



-- Saves content of a string table into a string array
function table2astring(tString, aString)
	local numberOfEntries = table.getn(tString)


	if numberOfEntries > MAXSERIALS then
		print("Number of elements in tString exceeds maximum amounts of possible serial numbers!\n")
		return -1
	end

	for i = 1, numberOfEntries do
		led_analyzer.astring_setitem(aString, i-1, tString[i])
	end

	return aString
end



-- Convert the Colors given as parameters into various color spaces (RGB, HSV, XYZ, Yxy, Wavelength)
-- and save the values of the color spaces into tables
function aus2colorTable(clear, red, green, blue, intTimes, gain, errorcode, length)
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
		lClear = led_analyzer.ushort_getitem(clear, i)
		lRed   = led_analyzer.ushort_getitem(red,   i)
		lGreen = led_analyzer.ushort_getitem(green, i)
		lBlue  = led_analyzer.ushort_getitem(blue,  i)

		-- Settings like Gain and Integration Time
		lGain    = led_analyzer.puchar_getitem(gain, i)
		lIntTime = led_analyzer.puchar_getitem(intTimes, i)

		-- ratio of measured clear channel count to max clear channel count (dependent on gain and integration time)
		lClearRatio = lClear/maxClear(lIntTime)

		-- get lux and cct, cct is not used/needed yet
		lLUX, lCCT = calculate_CCT_LUX(lRed, lGreen, lBlue, lClear, lIntTime, lGain)

		-- to avoid a later division by zero and to have more stable readings and no unneccessary
		-- outputs from channels that are not reading any LEDs we set a minum lux level

		-- if measured brightness (lux) falls beneath required MINIMUM LUX or our clear channel gives zero
		-- fill all color tables with zero values, beside the lux value.
		if(lClearRatio < MIN_CLEAR or lLUX < MIN_LUX ) then
			-- RGB table
			tRGB[i+1] = {clear = 0,
			             red   = 0,
			             green = 0,
			             blue  = 0 }

			-- RGB normalized
			tXYZ[i+1] = { X = 0,
			              Y = 0,
			              Z = 0 }

			-- Yxy space
			tYxy[i+1] = { Y = 0,
			              x = 0,
			              y = 0 }

			-- Wavelength and saturation
			tWavelength[i+1] = { nm          = 0,
			                     sat         = 0,
			                     lux         = lLUX,
			                     r           = 0,
			                     g           = 0,
			                     b           = 0,
			                     clear_ratio = 100*lClearRatio }

			tHSV[i+1] = { H = 0,
			              S = 0,
			              V = 0 }


			tSettings[i+1] = { gain      = lGain,
			                   intTime   = lIntTime }


		else
			r_n = lRed   / lClear
			g_n = lGreen / lClear
			b_n = lBlue  / lClear


			-- RGB table
			tRGB[i+1] = {clear = lClear,
			             red   = lRed,
			             green = lGreen,
			             blue  = lBlue }

			local X, Y, Z = RGB2XYZ(r_n, g_n, b_n, "sRGB")

			-- XYZ table
			tXYZ[i+1] = { X = X,
			              Y = Y,
			              Z = Z }

			local Y, x, y = XYZ2Yxy(X, Y, Z)

			-- Yxy table
			tYxy[i+1] = { Y = Y,
			              x = x,
			              y = y }

			local wavelength, saturation = Yxy2wavelength(x, y)
			local wR, wG, wB = wavelength2RGB(wavelength)

			-- Wavelength Saturation Brightness table
			tWavelength[i+1] = { nm         = math.floor(wavelength+0.5),
			                    sat         = saturation * 100,
			                    lux         = lLUX,
			                    r           = wR,
			                    g           = wG,
			                    b           = wB,
			                    clear_ratio = 100*lClearRatio }

			local H, S, V = RGB2HSV(r_n, g_n, b_n)

			-- HSV table
			tHSV[i+1] = { H = H,
			              S = S,
			              V = V }

			-- Settings --
			tSettings[i+1] = { gain      = lGain,
			                   intTime   = lIntTime }
		end
	end

	tColorTable [ENTRY_WAVELENGTH] = tWavelength
	tColorTable [ENTRY_RGB] = tRGB
	tColorTable [ENTRY_XYZ] = tXYZ
	tColorTable [ENTRY_Yxy] = tYxy
	tColorTable [ENTRY_HSV] = tHSV
	tColorTable [ENTRY_SETTINGS] = tSettings
	tColorTable [ENTRY_ERRORCODE] = errorcode

	return tColorTable
end



-- Gets the gain and integration time settings and stores them into a settings table with form
-- table[sensor] = {gain = ..., intTime = ...}
function auc2settingsTable(aucIntegrationtimes, aucGains, length)
	local tEntry = {}
	local lGain
	local lIntTime


	for i = 0, length - 1 do
		lIntTime = led_analyzer.puchar_getitem(aucIntegrationtimes, i)
		lGain    = led_analyzer.puchar_getitem(aucGains, i)

		tEntry[i+1] = { gain    = lGain,
		                intTime = lIntTime }
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
