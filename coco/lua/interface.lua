
function getStrInterfaces()
	-- Detect all interfaces.
	local aDetectedInterfaces = {}
	for iCnt,tPlugin in ipairs(__MUHKUH_PLUGINS) do
		tPlugin:DetectInterfaces(aDetectedInterfaces)
	end

	-- Search all detected interfaces for the pattern.
	local atInterfaceNames = {}
	for iInterfaceIdx,tInterface in ipairs(aDetectedInterfaces) do
		table.insert(atInterfaceNames, tInterface:GetName())
	end

	-- Join all interface names to one big string.
	return table.concat(atInterfaceNames, ',')
end 