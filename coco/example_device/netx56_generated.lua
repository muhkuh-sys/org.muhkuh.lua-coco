require("color_control")
require("muhkuh_cli_init")
require("interface")
require("io_matrix")


-- INSERT INTERFACE NUMBER
local strInterface = "romloader_uart_COM4"
-- be pessimistic 
local iRetval = TEST_RESULT_FAIL

local strXmlResult
local lux_check_enable = nil
 ---------------------- ColorControl LEDs under test ---------------------- 
-- LED Testset # 0 --

local tTestSet0 = {

 [ 0] = {
     [ 1] = {name = "eth0_g", nm = 571, tol_nm = 10, sat = 100, tol_sat = 10, lux =    16, tol_lux =   50},
     [ 2] = {name = "eth0_y", nm = 589, tol_nm = 10, sat = 100, tol_sat = 10, lux =    10, tol_lux =   50},
     [ 3] = {name = "eth1_g", nm = 569, tol_nm = 10, sat =  98, tol_sat = 10, lux =    16, tol_lux =   50},
     [ 4] = {name = "eth1_y", nm = 589, tol_nm = 10, sat = 100, tol_sat = 10, lux =    15, tol_lux =   50},
     [ 5] = {name = "LEDSY1", nm = 589, tol_nm = 10, sat = 100, tol_sat = 10, lux =   450, tol_lux =   50},
     [ 6] = {name = "LEDSY2", nm = 590, tol_nm = 10, sat = 100, tol_sat = 10, lux =   464, tol_lux =   50},
     [ 7] = {name = "LEDSY3", nm = 588, tol_nm = 10, sat = 100, tol_sat = 10, lux =   402, tol_lux =   50},
     [ 8] = {name = "LEDSY4", nm = 588, tol_nm = 10, sat = 100, tol_sat = 10, lux =   456, tol_lux =   50},
     [ 9] = {name = "duo1_g", nm = 571, tol_nm = 10, sat =  99, tol_sat = 10, lux =    47, tol_lux =   50},
     [10] = {name = "duo2_g", nm = 571, tol_nm = 10, sat =  99, tol_sat = 10, lux =    66, tol_lux =   50},
     [11] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [12] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [13] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [14] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [15] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [16] =  nil   -- NO TEST ! NO TEST ! NO TEST ! -- 
   }

}

-- LED Testset # 1 --

local tTestSet1 = {

 [ 0] = {
     [ 1] = {name = "eth0_g", nm =   0, tol_nm = 10, sat =   0, tol_sat = 10, lux =     0, tol_lux =   50},
     [ 2] = {name = "eth0_y", nm =   0, tol_nm = 10, sat =   0, tol_sat = 10, lux =     1, tol_lux =   50},
     [ 3] = {name = "eth1_g", nm =   0, tol_nm = 10, sat =   0, tol_sat = 10, lux =     1, tol_lux =   50},
     [ 4] = {name = "eth1_y", nm =   0, tol_nm = 10, sat =   0, tol_sat = 10, lux =     0, tol_lux =   50},
     [ 5] = {name = "LEDSY1", nm =   0, tol_nm = 10, sat =   0, tol_sat = 10, lux =     1, tol_lux =   50},
     [ 6] = {name = "LEDSY2", nm =   0, tol_nm = 10, sat =   0, tol_sat = 10, lux =     1, tol_lux =   50},
     [ 7] = {name = "LEDSY3", nm =   0, tol_nm = 10, sat =   0, tol_sat = 10, lux =     1, tol_lux =   50},
     [ 8] = {name = "LEDSY4", nm =   0, tol_nm = 10, sat =   0, tol_sat = 10, lux =     1, tol_lux =   50},
     [ 9] = {name = "duo1_g", nm =   0, tol_nm = 10, sat =   0, tol_sat = 10, lux =     1, tol_lux =   50},
     [10] = {name = "duo2_g", nm =   0, tol_nm = 10, sat =   0, tol_sat = 10, lux =     3, tol_lux =   50},
     [11] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [12] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [13] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [14] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [15] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [16] =  nil   -- NO TEST ! NO TEST ! NO TEST ! -- 
   }

}

-- LED Testset # 2 --

local tTestSet2 = {

 [ 0] = {
     [ 1] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [ 2] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [ 3] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [ 4] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [ 5] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [ 6] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [ 7] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [ 8] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [ 9] = {name = "duo1_r", nm = 628, tol_nm = 10, sat = 100, tol_sat = 10, lux =    29, tol_lux =   50},
     [10] = {name = "duo2_r", nm = 628, tol_nm = 10, sat = 100, tol_sat = 10, lux =    28, tol_lux =   50},
     [11] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [12] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [13] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [14] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [15] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [16] =  nil   -- NO TEST ! NO TEST ! NO TEST ! -- 
   }

}

-- LED Testset # 3 --

local tTestSet3 = {

 [ 0] = {
     [ 1] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [ 2] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [ 3] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [ 4] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [ 5] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [ 6] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [ 7] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [ 8] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [ 9] = {name = "duo1_r", nm =   0, tol_nm = 10, sat =   0, tol_sat = 10, lux =     3, tol_lux =   50},
     [10] = {name = "duo2_r", nm =   0, tol_nm = 10, sat =   0, tol_sat = 10, lux =     4, tol_lux =   50},
     [11] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [12] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [13] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [14] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [15] =  nil , -- NO TEST ! NO TEST ! NO TEST ! -- 
     [16] =  nil   -- NO TEST ! NO TEST ! NO TEST ! -- 
   }

}

local atTestSets = { tTestSet0, tTestSet1, tTestSet2, tTestSet3 }

-- Settings Table contains gain and integration time
atSettings = {
 [ 0] = {
   [ 1] = { gain =   0, integration =   0 },
   [ 2] = { gain =   0, integration =   0 },
   [ 3] = { gain =   0, integration =   0 },
   [ 4] = { gain =   0, integration =   0 },
   [ 5] = { gain =   0, integration =   0 },
   [ 6] = { gain =   0, integration =   0 },
   [ 7] = { gain =   0, integration =   0 },
   [ 8] = { gain =   0, integration =   0 },
   [ 9] = { gain =   0, integration =   0 },
   [10] = { gain =   0, integration =   0 },
   [11] = { gain =   0, integration =   0 },
   [12] = { gain =   0, integration =   0 },
   [13] = { gain =   0, integration =   0 },
   [14] = { gain =   0, integration =   0 },
   [15] = { gain =   0, integration =   0 },
   [16] = { gain =   0, integration =   0 }
   }

}
 --------------------------- netX I/O Table ---------------------------
 
 local atPinsUnderTest = {

  --- Sensor  1 ---
  { "eth0_g", io_matrix.PINTYPE_MMIO,  12,  0, io_matrix.PINFLAG_IOZ },

  --- Sensor  2 ---
  { "eth0_y", io_matrix.PINTYPE_MMIO,  13,  0, io_matrix.PINFLAG_IOZ },

  --- Sensor  3 ---
  { "eth1_g", io_matrix.PINTYPE_MMIO,  14,  0, io_matrix.PINFLAG_IOZ },

  --- Sensor  4 ---
  { "eth1_y", io_matrix.PINTYPE_MMIO,  15,  0, io_matrix.PINFLAG_IOZ },

  --- Sensor  5 ---
  { "LEDSY1", io_matrix.PINTYPE_MMIO,  24,  0, io_matrix.PINFLAG_IOZ },

  --- Sensor  6 ---
  { "LEDSY2", io_matrix.PINTYPE_MMIO,  25,  0, io_matrix.PINFLAG_IOZ },

  --- Sensor  7 ---
  { "LEDSY3", io_matrix.PINTYPE_MMIO,  26,  0, io_matrix.PINFLAG_IOZ },

  --- Sensor  8 ---
  { "LEDSY4", io_matrix.PINTYPE_MMIO,  27,  0, io_matrix.PINFLAG_IOZ },

  --- Sensor  9 ---
  { "duo1_g", io_matrix.PINTYPE_MMIO,  28,  0, io_matrix.PINFLAG_IOZ },
  { "duo1_r", io_matrix.PINTYPE_MMIO,  29,  0, io_matrix.PINFLAG_IOZ },

  --- Sensor 10 ---
  { "duo2_g", io_matrix.PINTYPE_MMIO,  30,  0, io_matrix.PINFLAG_IOZ },
  { "duo2_r", io_matrix.PINTYPE_MMIO,  31,  0, io_matrix.PINFLAG_IOZ }

}

-- Functions triggers the desired pin state for a testset
-- apply pin states for test set 0
local function applyPinState0(aAttr)
   io_matrix.set_pin(aAttr, "eth0_g", 1)
   io_matrix.set_pin(aAttr, "eth0_y", 1)
   io_matrix.set_pin(aAttr, "eth1_g", 1)
   io_matrix.set_pin(aAttr, "eth1_y", 1)
   io_matrix.set_pin(aAttr, "LEDSY1", 1)
   io_matrix.set_pin(aAttr, "LEDSY2", 1)
   io_matrix.set_pin(aAttr, "LEDSY3", 1)
   io_matrix.set_pin(aAttr, "LEDSY4", 1)
   io_matrix.set_pin(aAttr, "duo1_g", 1)
   io_matrix.set_pin(aAttr, "duo2_g", 1)
end
-- apply pin states for test set 1
local function applyPinState1(aAttr)
   io_matrix.set_pin(aAttr, "eth0_g", 2)
   io_matrix.set_pin(aAttr, "eth0_y", 2)
   io_matrix.set_pin(aAttr, "eth1_g", 2)
   io_matrix.set_pin(aAttr, "eth1_y", 2)
   io_matrix.set_pin(aAttr, "LEDSY1", 2)
   io_matrix.set_pin(aAttr, "LEDSY2", 2)
   io_matrix.set_pin(aAttr, "LEDSY3", 2)
   io_matrix.set_pin(aAttr, "LEDSY4", 2)
   io_matrix.set_pin(aAttr, "duo1_g", 2)
   io_matrix.set_pin(aAttr, "duo2_g", 2)
end
-- apply pin states for test set 2
local function applyPinState2(aAttr)
   io_matrix.set_pin(aAttr, "duo1_r", 1)
   io_matrix.set_pin(aAttr, "duo2_r", 1)
end
-- apply pin states for test set 3
local function applyPinState3(aAttr)
   io_matrix.set_pin(aAttr, "duo1_r", 2)
   io_matrix.set_pin(aAttr, "duo2_r", 2)
end
-- Open a netx connection without user input
local function open_netx_connection(strInterfacePattern)

    -- Open the connection to the netX.
    if string.upper(strInterfacePattern)~="ASK" then
        -- No interface detected yet.
        local tPlugin = nil

        -- Detect all interfaces.
        local aDetectedInterfaces = {}
        for iCnt,tPlugin in ipairs(__MUHKUH_PLUGINS) do
            tPlugin:DetectInterfaces(aDetectedInterfaces)
        end

        -- Search all detected interfaces for the pattern.
        for iInterfaceIdx,tInterface in ipairs(aDetectedInterfaces) do
            local strName = tInterface:GetName()
            if string.match(strName, strInterfacePattern)~=nil then
                tPlugin = aDetectedInterfaces[iInterfaceIdx]:Create()

                -- Connect the plugin.
                tPlugin:Connect()
                break
            end
        end

        -- Found the interface?
        if tPlugin==nil then
            error(string.format("No interface matched the pattern '%s'!", strInterfacePattern))
        end

        tester.setCommonPlugin(tPlugin)

    end

end



-- Device connection ----------------------
-- netX
open_netx_connection(strInterface)
tPlugin = tester.getCommonPlugin()
if tPlugin==nil then
    error("No plugin selected, nothing to do!")
end

-- Color Controller --> Scan
numberOfDevices, strXmlResult = scanDevices()
if numberOfDevices <= 0 then
    free()
    tPlugin:Disconnect()
    return TEST_RESULT_DEVICE_ERROR, strXmlResult
end

-- Color Controller --> Connect
numberOfDevices, strXmlResult = connectDevices()
if numberOfDevices <= 0 then
    free()
    tPlugin:Disconnect()
    return TEST_RESULT_DEVICE_ERROR, strXmlResult
end

-- Device initialization -------------------
-- netX
local aAttr = io_matrix.initialize(tPlugin, "netx/iomatrix_netx%d.bin")
io_matrix.parse_pin_description(aAttr, atPinsUnderTest, ulVerbose)
-- Turn off all LEDs
local uiCounter = 1
while(atPinsUnderTest[uiCounter] ~= nil) do
    io_matrix.set_pin(aAttr, atPinsUnderTest[uiCounter][1],2)
    uiCounter = uiCounter + 1
end

-- Color Controller --> Init
initDevices(atSettings)

-- Actual Test -----------------------------
-- Testset 1
applyPinState0(aAttr)
led_analyzer.wait4Conversion(200)
startMeasurements()
iRetval, strXmlResult = validateLEDs(atTestSets[1], lux_check_enable)

if iRetval ~= TEST_RESULT_OK then
    free()
    ---- APPLY DEFAULT PINSTATES ---- 
    tPlugin:Disconnect()
    return  TEST_RESULT_FAIL, strXmlResult
end

-- Testset 2
applyPinState1(aAttr)
led_analyzer.wait4Conversion(200)
startMeasurements()
iRetval, strXmlResult = validateLEDs(atTestSets[2], lux_check_enable)

if iRetval ~= TEST_RESULT_OK then
    free()
    ---- APPLY DEFAULT PINSTATES ---- 
    tPlugin:Disconnect()
    return  TEST_RESULT_FAIL, strXmlResult
end

-- Testset 3
applyPinState2(aAttr)
led_analyzer.wait4Conversion(200)
startMeasurements()
iRetval, strXmlResult = validateLEDs(atTestSets[3], lux_check_enable)

if iRetval ~= TEST_RESULT_OK then
    free()
    ---- APPLY DEFAULT PINSTATES ---- 
    tPlugin:Disconnect()
    return  TEST_RESULT_FAIL, strXmlResult
end

-- Testset 4
applyPinState3(aAttr)
led_analyzer.wait4Conversion(200)
startMeasurements()
iRetval, strXmlResult = validateLEDs(atTestSets[4], lux_check_enable)

if iRetval ~= TEST_RESULT_OK then
    free()
    ---- APPLY DEFAULT PINSTATES ---- 
    tPlugin:Disconnect()
    return  TEST_RESULT_FAIL, strXmlResult
end

free()
tPlugin:Disconnect()
return iRetval, strXmlResult
