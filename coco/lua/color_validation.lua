module("color_validation", package.seeall)

-- compare rows will compare the current measured colors with the row set provided in a test table
-- it returns a status information, an information text table with string values, and the values for
-- wavelength ... illumination, tolerances for these, and set point wavelength ... illumination
	-- 0: PASS ALL FITS
	-- 1: There's no test entry for the row, thus we do not need a test
	-- 2: Wrong color as saturation fits but the wavelength does not
	-- 3: Wavelength fits but saturation does not
	-- 4: Neither wavelength nor saturation fit (environmental lightning ?)
	-- if You can optionally pass a lux_check_enable parameter, it is not nil then
	-- the test will also check for the brightness (illumination) of the LED
	-- 0: color ok, sat ok, brightness ok
	-- 5: color ok, sat ok, brightness too low
	-- 6: color ok, sat ok, brightness too high

local function compareRows(tDUTrow, tCurColorSensor, lux_check_enable)
	-- The row in testboard exists, thus a test for this LED is desired
	if tDUTrow ~= nil then
		-- current values
		local nm  = tCurColorSensor.nm
		local sat = tCurColorSensor.sat
		local lux = tCurColorSensor.lux
		--tolerances
		local tol_nm = tDUTrow.tol_nm
		local tol_sat = tDUTrow.tol_sat
		local tol_lux = tDUTrow.tol_lux
		-- set points --
		local sp_nm = tDUTrow.nm
		local sp_sat = tDUTrow.sat
		local sp_lux = tDUTrow.lux

		-- Saturation and wavelength fit
		if  (nm    >= (sp_nm  - tol_nm)
		and  nm    <= (sp_nm  + tol_nm)
		and  sat   >= (sp_sat - tol_sat)
		and  sat   <= (sp_sat + tol_sat)) then
			-- Brightness check is enabled
			if lux_check_enable ~= nil then
					-- Brightness is OK --
					if (lux > (sp_lux - tol_lux)
					and lux < (sp_lux + tol_lux)) then
						return 0, {"Wavelength   is in range",
						           "Saturation   is in range",
						           "Illumination is in range"}, nm, sat, lux, tol_nm, tol_sat, tol_lux, sp_nm, sp_sat, sp_lux

					-- Brightness falls below min_brightness
					elseif lux < (sp_lux - tol_lux) then
						return 5, {"Wavelength is in range",
						           "Saturation is in range",
						           "Illumination is too low"}, nm, sat, lux, tol_nm, tol_sat, tol_lux, sp_nm, sp_sat, sp_lux
					-- Brightness exceeds max_brightness
					elseif lux > (sp_lux + tol_lux) then
						return 6, {"Wavelength is in range",
						           "Saturation is in range",
						           "Illumination is too high"}, nm, sat, lux, tol_nm, tol_sat, tol_lux, sp_nm, sp_sat, sp_lux
					end
			--Brightness check is disabled
			else
				-- As wavelength and saturations are OK and there's no need for a lux check we can return OK here
				return 0, {"Wavelength is in range",
				           "Saturation is in range",
				           "Illumination was not tested"}, nm, sat, lux, tol_nm, tol_sat, tol_lux, sp_nm, sp_sat, sp_lux
			end
		-- Saturation fits but wavelength doesn't (wrong color)
		elseif ((nm  <= (sp_nm  - tol_nm)
		or       nm  >= (sp_nm  + tol_nm))
		and      sat >= (sp_sat - tol_sat)
		and      sat <= (sp_sat + tol_sat)) then
			return 2, {"Wavelength is not in range",
			           "Saturation is in range",
			           "Illumination was not tested"}, nm, sat, lux, tol_nm, tol_sat, tol_lux, sp_nm, sp_sat, sp_lux
		-- Wavelength fits but saturation doesn't
		elseif   (nm <= (sp_nm + tol_nm)
		and       nm >= (sp_nm - tol_nm)
		and      (sat <= sp_sat - tol_sat
		or        sat >= sp_sat + tol_sast)) then
			return 3, {"Wavelength is in range",
			           "Saturation is not in range",
			           "Illumination was not tested"}, nm, sat, lux, tol_nm, tol_sat, tol_lux, sp_nm, sp_sat, sp_lux
		-- Neither saturation nor wavelength fit --
		else
			return 4, {"Wavelength is not in range",
			           "Saturation is not in range",
			           "Illumination was not tested"}, nm, sat, lux, tol_nm, tol_sat, tol_lux, sp_nm, sp_sat, sp_lux
		end

	-- the row field is nil thus we do not need a test
	else
		return 1, {"NO TEST ENTRY",
		           "NO TEST ENTRY",
		           "NO TEST ENTRY"}, -1, -1, -1, -1, -1, -1, -1, -1, -1
	end
end



-- function prints all errored leds
local function reportErrors(tTestSummary)
	for iDevIndex, tTestSummaryDevice in pairs(tTestSummary) do
		for iSensorIndex, tTestrowDevice in pairs(tTestSummaryDevice) do
			-- Status was not okay (!0) and Test Entry existed (!1) --
			if tTestrowDevice.status ~= 0 and tTestrowDevice.status ~= 1 then
				print("Sensor -------- Status --------- nm ----------- sat ------------- lux")
				print(string.format("%2d              %2d               %3d            %3d               %5d", (iDevIndex*16 + iSensorIndex ), tTestrowDevice.status, tTestrowDevice.nm,
				tTestrowDevice.sat, tTestrowDevice.lux))

				for iIndex, strText in ipairs(tTestrowDevice.infotext) do
					print(strText)
				end
			end
		end
	end
end



function getDeviceSummary(tDUT, tCurColors, lux_check_enable)
	
	local tTestSummary_device = {}
	local status, infotext, nm, sat, lux, tol_nm, tol_sat, tol_lux, sp_nm, sp_sat, sp_lux  
	

	if tDUT ~= nil then 
		for sensor = 1, MAXSENSORS do 
				tTestSummary_device[sensor] = {}
				status, infotext, nm, sat, lux, tol_nm, tol_sat, tol_lux, sp_nm, sp_sat, sp_lux 
													 = compareRows(tDUT[sensor], tCurColors[sensor], lux_check_enable)
				tTestSummary_device[sensor].status 	 = status
				tTestSummary_device[sensor].infotext = infotext
				tTestSummary_device[sensor].nm  	 = nm 
				tTestSummary_device[sensor].sat 	 = sat  
				tTestSummary_device[sensor].lux 	 = lux  
				tTestSummary_device[sensor].tol_nm   = tol_nm  
				tTestSummary_device[sensor].tol_sat  = tol_sat   
				tTestSummary_device[sensor].tol_lux  = tol_lux  
				tTestSummary_device[sensor].sp_nm    = sp_nm   
				tTestSummary_device[sensor].sp_sat   = sp_sat   
				tTestSummary_device[sensor].sp_lux   = sp_lux 
		end 
	else 
		for sensor = 1, MAXSENSORS do 
				tTestSummary_device[sensor] = {}
				tTestSummary_device[sensor].status 	 = 1 -- means no test entry 
				tTestSummary_device[sensor].infotext = {"NO TEST ENTRY","NO TEST ENTRY","NO TEST ENTRY"} 
				tTestSummary_device[sensor].nm  	 = -1  
				tTestSummary_device[sensor].sat 	 = -1   
				tTestSummary_device[sensor].lux 	 = -1   
				tTestSummary_device[sensor].tol_nm   = -1   
				tTestSummary_device[sensor].tol_sat  = -1    
				tTestSummary_device[sensor].tol_lux  = -1 
				tTestSummary_device[sensor].sp_nm    = -1    
				tTestSummary_device[sensor].sp_sat   = -1    
				tTestSummary_device[sensor].sp_lux   = -1				
		end 
	end  
	
	return tTestSummary_device 
end 

--[[
-- prints a test summary for a device containing all elements (failed and succeeded leds)
function printDeviceSummary(tTestSummary_device, info_enable)
	for iSensorIndex, tTestrowDevice in pairs(tTestSummary_device) do 
		print("Sensor -------- Status --------- nm ----------- sat ------------- lux")
		if tTestrowDevice ~= nil then
			print(string.format("%2d	 	 %2d		%3d		%3d		%5d", iSensorIndex, tTestrowDevice.status, tTestrowDevice.nm,
			tTestrowDevice.sat, tTestrowDevice.lux))
			if info_enable >= 1 then
				for iIndex, strText in ipairs(tTestrowDevice.infotext) do
					print(strText)
				end
			end
		end
	end
end
--]]



-- returns nil if no error occured
-- returns iDevIndex (starts with 0) iSensorIndex ( starts with 1) and the infotext of the first sensor that failed
function errorsOccured(tTestSummary)
	local atErrors = {}
	local error_occured = false


	for iDevIndex, tTestSummaryDevice in pairs(tTestSummary) do
		for iSensorIndex, tTestrowDevice in pairs(tTestSummaryDevice) do
			-- Status was not okay (!0) and Test Entry existed (!1) --
			if tTestrowDevice.status ~= 0 and tTestrowDevice.status ~= 1 then
				--return iDevIndex, iSensorIndex, tTestrowDevice.infotext
				table.insert(atErrors, string.format("%2d", iDevIndex*16+iSensorIndex))
				error_occured = true
			end
		end
	end

	return error_occured, table.concat(atErrors, ",")
end



function validateTestSummary(numberOfDevices, tTestSummary)
	-- Iterate over all device summaries
	local devIndex
	local strErrored 
	local error_occured = false 
	
	errors_occured, strErrored = errorsOccured(tTestSummary)
	if errors_occured then 
		print(string.format("Errors with LED(s): %s", strErrored ))
		reportErrors(tTestSummary)
	end 
	
	
	if errors_occured then 
		print("")
		print("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
		print("!!!!!!!!!!!!!			!!!!!!!!!!!!!")
		print("!!!!!!!!!!!!!       ERROR       !!!!!!!!!!!!!")
		print("!!!!!!!!!!!!!			!!!!!!!!!!!!!")
		print("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
		print("")
		return TEST_RESULT_FAIL

	-- else the test was not successfull --
	else 
		print("")
		print(" #######  ##    ## ")
		print("##     ## ##   ##  ")
		print("##     ## ##  ##   ")
		print("##     ## #####    ")
		print("##     ## ##  ##   ")
		print("##     ## ##   ##  ")
		print(" #######  ##    ## ")
		print("")		
		return TEST_RESULT_OK 
	end 
end  



