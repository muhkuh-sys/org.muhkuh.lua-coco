import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import math

def dictToList(dict):
	list = []
	for x,y in dict.items():
		list.append(y)
	return list

def normList(list_unnormed,normalizer):
	list_normed = []
	for i in range(len(list_unnormed)):
		if (normalizer[i] == 0):
			list_normed.append(0)
		else:
			list_normed.append(list_unnormed[i] / normalizer[i])
	return list_normed


def visibleLight(list,wavelength):
	list_380_780 = []
	for i in range(len(wavelength)):
		if (wavelength[i] >= 380 and wavelength[i] <= 780):
			list_380_780.append(list[i])
	return list_380_780


def gammaDecoding(list_undecoded):
	list_decoded = []

	for i in range(len(list_undecoded)):
		if (list_undecoded[i] <= 0.04045):
			list_decoded.append(list_undecoded[i]/12.92)
		else:
			list_decoded.append(math.pow((list_undecoded[i] + 0.055)/1.055,2.4))
	
	return list_decoded 


	#  0.4124564  0.3575761  0.1804375
	#  0.2126729  0.7151522  0.0721750
	#  0.0193339  0.1191920  0.9503041
def sRGBtoXYZ(RGB_values):
	sRGB = np.array([[0.4124564,  0.3575761,  0.1804375],
					[0.2126729,  0.7151522,  0.0721750],
					[0.0193339,  0.1191920,  0.9503041]])

	XYZ_values = np.dot(sRGB, RGB_values)
	return XYZ_values


def XYZtoYxy(XYZ_values):
	Y = XYZ_values[1]
	x = XYZ_values[0] / (XYZ_values[0] + XYZ_values[1] + XYZ_values[2])
	y = XYZ_values[1] / (XYZ_values[0] + XYZ_values[1] + XYZ_values[2])

	return Y,x,y




data_TCS3400_RGBC_wavelength = pd.read_excel('TCS3400_RGBC.xlsx')

dict_TCS3400_RGBC_wavelength = data_TCS3400_RGBC_wavelength.to_dict()

dict_wavelength = dict_TCS3400_RGBC_wavelength["wavelength (nm)"]
dict_R = dict_TCS3400_RGBC_wavelength["R"]
dict_G = dict_TCS3400_RGBC_wavelength["G"]
dict_B = dict_TCS3400_RGBC_wavelength["B"]
dict_C = dict_TCS3400_RGBC_wavelength["C"]

list_wavelength = dictToList(dict_wavelength) # list(dict_wavelength.items())
list_R = dictToList(dict_R) # list(dict_R.items())
list_G = dictToList(dict_G) # list(dict_G.items())
list_B = dictToList(dict_B) # dict_B.values() # list(dict_B.items())
list_C = dictToList(dict_C) # list(dict_C.items())

list_R_normed = normList(list_R,list_C)
list_G_normed = normList(list_G,list_C)
list_B_normed = normList(list_B,list_C)

list_wavelength_380_780 = visibleLight(list_wavelength,list_wavelength)

list_R_normed_380_780 = visibleLight(list_R_normed,list_wavelength)
list_G_normed_380_780 = visibleLight(list_G_normed,list_wavelength)
list_B_normed_380_780 = visibleLight(list_B_normed,list_wavelength)

list_R_380_780 = visibleLight(list_R,list_wavelength)
list_G_380_780 = visibleLight(list_G,list_wavelength)
list_B_380_780 = visibleLight(list_B,list_wavelength)
list_C_380_780 = visibleLight(list_C,list_wavelength)

list_R_normed_380_780_decoded = gammaDecoding(list_R_normed_380_780)
list_G_normed_380_780_decoded = gammaDecoding(list_G_normed_380_780) 
list_B_normed_380_780_decoded = gammaDecoding(list_B_normed_380_780)

#with gamma decoding
RGB_normed_380_780_decoded = np.array([list_R_normed_380_780_decoded,
				list_G_normed_380_780_decoded,
				list_B_normed_380_780_decoded])

XYZ_normed_380_780_decoded = sRGBtoXYZ(RGB_normed_380_780_decoded)

Y_normed_380_780_decoded,x_normed_380_780_decoded,y_normed_380_780_decoded = XYZtoYxy(XYZ_normed_380_780_decoded)

# without gamma
RGB_normed_380_780 = np.array([list_R_normed_380_780,
				list_G_normed_380_780,
				list_B_normed_380_780])

XYZ_normed_380_780 = sRGBtoXYZ(RGB_normed_380_780)

Y_normed_380_780,x_normed_380_780,y_normed_380_780 = XYZtoYxy(XYZ_normed_380_780)



#
plt.figure()
plt.plot(list_wavelength,list_R_normed, "r*-", markersize=6, linewidth=1, color='r')
plt.plot(list_wavelength,list_G_normed, "r*-", markersize=6, linewidth=1, color='g')
plt.plot(list_wavelength,list_B_normed, "r*-", markersize=6, linewidth=1, color='b')
plt.grid(True)
# plt.show()

plt.figure()
plt.plot(list_wavelength_380_780,list_R_normed_380_780, "r*-", markersize=6, linewidth=1, color='r')
plt.plot(list_wavelength_380_780,list_G_normed_380_780, "r*-", markersize=6, linewidth=1, color='g')
plt.plot(list_wavelength_380_780,list_B_normed_380_780, "r*-", markersize=6, linewidth=1, color='b')
plt.grid(True)

#
plt.figure()
plt.plot(list_wavelength,list_R, "r*-", markersize=6, linewidth=1, color='r')
plt.plot(list_wavelength,list_G, "r*-", markersize=6, linewidth=1, color='g')
plt.plot(list_wavelength,list_B, "r*-", markersize=6, linewidth=1, color='b')
plt.plot(list_wavelength,list_C, "r*-", markersize=6, linewidth=1, color='k')
plt.grid(True)

plt.figure()
plt.plot(list_wavelength_380_780,list_R_380_780, "r*-", markersize=6, linewidth=1, color='r')
plt.plot(list_wavelength_380_780,list_G_380_780, "r*-", markersize=6, linewidth=1, color='g')
plt.plot(list_wavelength_380_780,list_B_380_780, "r*-", markersize=6, linewidth=1, color='b')
plt.plot(list_wavelength_380_780,list_C_380_780, "r*-", markersize=6, linewidth=1, color='k')
plt.grid(True)

#
plt.figure()
plt.plot(list_wavelength_380_780,XYZ_normed_380_780_decoded[0], "r*-", markersize=6, linewidth=1, color='r')
plt.plot(list_wavelength_380_780,XYZ_normed_380_780_decoded[1], "r*-", markersize=6, linewidth=1, color='g')
plt.plot(list_wavelength_380_780,XYZ_normed_380_780_decoded[2], "r*-", markersize=6, linewidth=1, color='b')
plt.grid(True)

#
plt.figure()
plt.plot(x_normed_380_780_decoded,y_normed_380_780_decoded, "r*-", markersize=6, linewidth=1, color='r')
plt.grid(True)


plt.figure()
plt.plot(x_normed_380_780,y_normed_380_780, "r*-", markersize=6, linewidth=1, color='r')
plt.grid(True)


plt.show()




# data.to_csv('TCS3400_RGBC.csv', sep=",")

# print()
# data.head()

# fileXLSX = openpyxl.load_workbook('TCS3400_Spectral_Response_3-lots.xlsx')

# sheet = fileXLSX["Vornamen 2019 mit Angabe der Ra"]

# print(sheet['A2'].value)

