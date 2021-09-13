import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import math
import os.path
from scipy.interpolate import CubicSpline


def normList(list_unnormed, normalizer):
    list_normed = []
    for i in range(len(list_unnormed)):
        if (normalizer[i] == 0):
            list_normed.append(0)
        else:
            list_normed.append(list_unnormed[i] / normalizer[i])
    return list_normed


def visibleLight(list, wavelength):
    list_380_780 = []
    for i in range(len(wavelength)):
        if (wavelength[i] >= 380 and wavelength[i] <= 780):
            list_380_780.append(list[i])
    return list_380_780


def gammaDecoding(list_undecoded, strRGB):
    list_decoded = []
    if strRGB == "sRGB":
        for i in range(len(list_undecoded)):
            if (list_undecoded[i] <= 0.04045):
                list_decoded.append(list_undecoded[i]/12.92)
            else:
                list_decoded.append(
                    math.pow((list_undecoded[i] + 0.055)/1.055, 2.4))
    elif strRGB == "adobeRGB":
        for i in range(len(list_undecoded)):
            list_decoded.append(
                math.pow(list_undecoded[i], (2*256+51)/256))
    return list_decoded

# http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
# sRGB to XYZ
#  0.4124564  0.3575761  0.1804375
#  0.2126729  0.7151522  0.0721750
#  0.0193339  0.1191920  0.9503041
#
# adobeRGB to XYZ
#  0.5767309  0.1855540  0.1881852
#  0.2973769  0.6273491  0.0752741
#  0.0270343  0.0706872  0.9911085


def RGBtoXYZ(RGB_values, strRGB):
    XYZ_values = []
    if strRGB == "sRGB":
        sRGB = np.array([[0.4124564,  0.3575761,  0.1804375],
                         [0.2126729,  0.7151522,  0.0721750],
                         [0.0193339,  0.1191920,  0.9503041]])

        XYZ_values = np.dot(sRGB, RGB_values)
    elif strRGB == "adobeRGB":
        adobeRGB = np.array([[0.5767309,  0.1855540,  0.1881852],
                             [0.2973769,  0.6273491,  0.0752741],
                             [0.0270343,  0.0706872,  0.9911085]])
        XYZ_values = np.dot(adobeRGB, RGB_values)

    return XYZ_values


def XYZtoYxy(XYZ_values):
    Y = XYZ_values[1]
    x = XYZ_values[0] / (XYZ_values[0] + XYZ_values[1] + XYZ_values[2])
    y = XYZ_values[1] / (XYZ_values[0] + XYZ_values[1] + XYZ_values[2])

    return Y, x, y


if __name__ == "__main__":

    if os.path.isfile("TCS3400_RGBC.json") == False:
        data_TCS3400_RGBC_wavelength = pd.read_excel('TCS3400_RGBC.xlsx')

        # https://pandas.pydata.org/pandas-docs/stable/user_guide/io.html#json
        data_TCS3400_RGBC_wavelength.to_json("TCS3400_RGBC.json")

        data_TCS3400_RGBC_wavelength = pd.read_json("TCS3400_RGBC.json")
    else:
        data_TCS3400_RGBC_wavelength = pd.read_json("TCS3400_RGBC.json")

    # https://pandas.pydata.org/docs/reference/api/pandas.DataFrame.to_numpy.html
    list_wavelength = data_TCS3400_RGBC_wavelength['wavelength (nm)'].to_numpy(
    )
    list_R = data_TCS3400_RGBC_wavelength['R'].to_numpy()
    list_G = data_TCS3400_RGBC_wavelength['G'].to_numpy()
    list_B = data_TCS3400_RGBC_wavelength['B'].to_numpy()
    list_C = data_TCS3400_RGBC_wavelength['C'].to_numpy()

    # eventuell nochmal UnivariateSpline (https://docs.scipy.org/doc/scipy/reference/generated/scipy.interpolate.UnivariateSpline.html) betrachten
    # CubicSpline nochmal anschauen

    # spline interpolation with stepsize of 0,3125 or 16 struts - using of splines
    # CubicSpline: https://docs.scipy.org/doc/scipy/reference/generated/scipy.interpolate.CubicSpline.html
    # or with bc_type='natural'
    interpolation_R = CubicSpline(list_wavelength, list_R, bc_type='natural')
    interpolation_G = CubicSpline(list_wavelength, list_G, bc_type='natural')
    interpolation_B = CubicSpline(list_wavelength, list_B, bc_type='natural')
    interpolation_C = CubicSpline(list_wavelength, list_C, bc_type='natural')
    # or with default bc_type = ‘not-a-knot’ (
    # interpolation_R = CubicSpline(list_wavelength, list_R)
    # interpolation_G = CubicSpline(list_wavelength, list_G)
    # interpolation_B = CubicSpline(list_wavelength, list_B)
    # interpolation_C = CubicSpline(list_wavelength, list_C)

    interpolation_wavelength = np.arange(
        list_wavelength[0], list_wavelength[-1] + 0.3125, 0.3125)

    # Spectral Responsivity of RGBC
    fig, (ax1, ax2) = plt.subplots(2)
    fig.suptitle('Spectral Responsivity of RGBC')
    ax1.plot(list_wavelength, list_R, "r*-", markersize=6,
             linewidth=1, color='k', label='R')
    ax1.plot(interpolation_wavelength, interpolation_R(interpolation_wavelength),
             "r*-", markersize=1, linewidth=1, color='r', label='R')
    ax1.grid(True)

    ax2.plot(list_wavelength, list_G, "r*-", markersize=6,
             linewidth=1, color='k', label='G')
    ax2.plot(interpolation_wavelength, interpolation_G(interpolation_wavelength),
             "r*-", markersize=1, linewidth=1, color='g', label='G')
    ax2.grid(True)

    fig, (ax1, ax2) = plt.subplots(2)
    fig.suptitle('Spectral Responsivity of RGBC')
    ax1.plot(list_wavelength, list_B, "r*-", markersize=6,
             linewidth=1, color='k', label='B')
    ax1.plot(interpolation_wavelength, interpolation_B(interpolation_wavelength),
             "r*-", markersize=1, linewidth=1, color='b', label='B')
    ax1.grid(True)
    ax2.plot(list_wavelength, list_C, "r*-", markersize=6,
             linewidth=1, color='k', label='C')
    ax2.plot(interpolation_wavelength, interpolation_C(interpolation_wavelength),
             "r*-", markersize=1, linewidth=1, color='k', label='C')
    ax2.grid(True)

    list_wavelength = interpolation_wavelength
    list_R = interpolation_R(interpolation_wavelength)
    list_G = interpolation_G(interpolation_wavelength)
    list_B = interpolation_B(interpolation_wavelength)
    list_C = interpolation_C(interpolation_wavelength)

    list_R_normed = normList(list_R, list_C)
    list_G_normed = normList(list_G, list_C)
    list_B_normed = normList(list_B, list_C)

    # normalization with max value of RGB
    maximum_value_RGB = np.max(
        np.array([list_R_normed, list_G_normed, list_B_normed]))

    list_R_normed /= maximum_value_RGB
    list_G_normed /= maximum_value_RGB
    list_B_normed /= maximum_value_RGB

    list_wavelength_380_780 = visibleLight(list_wavelength, list_wavelength)

    list_R_normed_380_780 = visibleLight(list_R_normed, list_wavelength)
    list_G_normed_380_780 = visibleLight(list_G_normed, list_wavelength)
    list_B_normed_380_780 = visibleLight(list_B_normed, list_wavelength)

    list_R_380_780 = visibleLight(list_R, list_wavelength)
    list_G_380_780 = visibleLight(list_G, list_wavelength)
    list_B_380_780 = visibleLight(list_B, list_wavelength)
    list_C_380_780 = visibleLight(list_C, list_wavelength)

    list_R_normed_380_780_decoded = gammaDecoding(
        list_R_normed_380_780, 'sRGB')
    list_G_normed_380_780_decoded = gammaDecoding(
        list_G_normed_380_780, 'sRGB')
    list_B_normed_380_780_decoded = gammaDecoding(
        list_B_normed_380_780, 'sRGB')

    # with gamma decoding
    RGB_normed_380_780_decoded = np.array([list_R_normed_380_780_decoded,
                                           list_G_normed_380_780_decoded,
                                           list_B_normed_380_780_decoded])

    XYZ_normed_380_780_decoded = RGBtoXYZ(RGB_normed_380_780_decoded, "sRGB")

    Y_normed_380_780_decoded, x_normed_380_780_decoded, y_normed_380_780_decoded = XYZtoYxy(
        XYZ_normed_380_780_decoded)

    # without gamma
    RGB_normed_380_780 = np.array([list_R_normed_380_780,
                                   list_G_normed_380_780,
                                   list_B_normed_380_780])

    XYZ_normed_380_780 = RGBtoXYZ(RGB_normed_380_780, "sRGB")

    Y_normed_380_780, x_normed_380_780, y_normed_380_780 = XYZtoYxy(
        XYZ_normed_380_780)

    # with gamma decoding and adobeRGB transformation
    list_R_normed_380_780_decoded_adobeRGB = gammaDecoding(
        list_R_normed_380_780, 'adobeRGB')
    list_G_normed_380_780_decoded_adobeRGB = gammaDecoding(
        list_G_normed_380_780, 'adobeRGB')
    list_B_normed_380_780_decoded_adobeRGB = gammaDecoding(
        list_B_normed_380_780, 'adobeRGB')

    RGB_normed_380_780_decoded_adobeRGB = np.array([list_R_normed_380_780_decoded_adobeRGB,
                                                    list_G_normed_380_780_decoded_adobeRGB,
                                                    list_B_normed_380_780_decoded_adobeRGB])

    XYZ_normed_380_780_decoded_adobeRGB = RGBtoXYZ(
        RGB_normed_380_780_decoded_adobeRGB, "adobeRGB")

    Y_normed_380_780_decoded_adobeRGB, x_normed_380_780_decoded_adobeRGB, y_normed_380_780_decoded_adobeRGB = XYZtoYxy(
        XYZ_normed_380_780_decoded_adobeRGB)

    data = {'nm': list_wavelength_380_780,
            'x': x_normed_380_780_decoded_adobeRGB,
            'y': y_normed_380_780_decoded_adobeRGB}
    df = pd.DataFrame(data, columns=['nm', 'x', 'y'])

    df.to_json('tTCS_Chromaticity.json', orient='index')

    # Normalized RGB to C channel
    fig, (ax1, ax2) = plt.subplots(2)
    fig.suptitle('Normalized RGB to C channel')

    ax1.set_title('wavelength: 300-1100 nm')
    ax1.plot(list_wavelength, list_R_normed, "r*-",
             markersize=6, linewidth=1, color='r', label='R')
    ax1.plot(list_wavelength, list_G_normed, "r*-",
             markersize=6, linewidth=1, color='g', label='G')
    ax1.plot(list_wavelength, list_B_normed, "r*-",
             markersize=6, linewidth=1, color='b', label='B')
    ax1.grid(True)
    ax1.legend(loc='best')

    ax2.set_title('wavelength: 380-780 nm (visible light)')
    ax2.plot(list_wavelength_380_780, list_R_normed_380_780,
             "r*-", markersize=6, linewidth=1, color='r', label='R')
    ax2.plot(list_wavelength_380_780, list_G_normed_380_780,
             "r*-", markersize=6, linewidth=1, color='g', label='G')
    ax2.plot(list_wavelength_380_780, list_B_normed_380_780,
             "r*-", markersize=6, linewidth=1, color='b', label='B')
    ax2.grid(True)
    ax2.legend(loc='best')

    # Spectral Responsivity of RGBC
    fig, (ax1, ax2) = plt.subplots(2)
    fig.suptitle('Spectral Responsivity of RGBC')
    ax1.set_title('wavelength: 300-1100 nm')
    ax1.plot(list_wavelength, list_R, "r*-", markersize=6,
             linewidth=1, color='r', label='R')
    ax1.plot(list_wavelength, list_G, "r*-", markersize=6,
             linewidth=1, color='g', label='G')
    ax1.plot(list_wavelength, list_B, "r*-", markersize=6,
             linewidth=1, color='b', label='B')
    ax1.plot(list_wavelength, list_C, "r*-", markersize=6,
             linewidth=1, color='k', label='C')
    ax1.grid(True)
    ax1.legend(loc='best')

    ax2.set_title('wavelength: 380-780 nm (visible light)')
    ax2.plot(list_wavelength_380_780, list_R_380_780,
             "r*-", markersize=6, linewidth=1, color='r', label='R')
    ax2.plot(list_wavelength_380_780, list_G_380_780,
             "r*-", markersize=6, linewidth=1, color='g', label='G')
    ax2.plot(list_wavelength_380_780, list_B_380_780,
             "r*-", markersize=6, linewidth=1, color='b', label='B')
    ax2.plot(list_wavelength_380_780, list_C_380_780,
             "r*-", markersize=6, linewidth=1, color='k', label='C')
    ax2.grid(True)
    ax2.legend(loc='best')

    # gamma decoded
    plt.figure()
    plt.suptitle('Gamma Decoding')
    plt.plot(list_wavelength_380_780,
             list_R_normed_380_780_decoded, "r*-", markersize=6, linewidth=1, color='r', label='R')
    plt.plot(list_wavelength_380_780,
             list_G_normed_380_780_decoded, "r*-", markersize=6, linewidth=1, color='g', label='G')
    plt.plot(list_wavelength_380_780,
             list_B_normed_380_780_decoded, "r*-", markersize=6, linewidth=1, color='b', label='B')
    plt.grid(True)
    plt.legend(loc='best')

    # XYZ color space
    plt.figure()
    plt.suptitle('XYZ color space')
    plt.plot(list_wavelength_380_780,
             XYZ_normed_380_780_decoded[0], "r*-", markersize=6, linewidth=1, color='r', label='R')
    plt.plot(list_wavelength_380_780,
             XYZ_normed_380_780_decoded[1], "r*-", markersize=6, linewidth=1, color='g', label='G')
    plt.plot(list_wavelength_380_780,
             XYZ_normed_380_780_decoded[2], "r*-", markersize=6, linewidth=1, color='b', label='B')
    plt.grid(True)
    plt.legend(loc='best')

    # Yxy color space (sRGB)
    plt.figure()
    plt.suptitle('Yxy color space of sRGB')
    plt.plot(x_normed_380_780_decoded, y_normed_380_780_decoded,
             "r*-", markersize=6, linewidth=1, color='r')
    plt.grid(True)
    plt.legend(loc='best')

    # fig, (ax1, ax2) = plt.subplots(2)
    # fig.suptitle('Yxy color space of sRGB')
    # ax1.set_title('with gamma decoding')
    # ax1.plot(x_normed_380_780_decoded, y_normed_380_780_decoded,
    #          "r*-", markersize=6, linewidth=1, color='r')
    # ax1.grid(True)
    # ax1.legend(loc='best')

    # # plt.figure()
    # ax2.set_title('without gamma decoding')
    # ax2.plot(x_normed_380_780, y_normed_380_780, "r*-",
    #          markersize=6, linewidth=1, color='r')
    # ax2.grid(True)
    # ax2.legend(loc='best')

    # Yxy color space (adobeRGB)
    plt.figure()
    plt.suptitle('Yxy color space of adobeRGB')
    plt.plot(x_normed_380_780_decoded_adobeRGB, y_normed_380_780_decoded_adobeRGB,
             "r*-", markersize=6, linewidth=1, color='r')
    plt.grid(True)
    plt.legend(loc='best')

    plt.show()

    print()


# data.to_csv('TCS3400_RGBC.csv', sep=",")

# print()
# data.head()

# fileXLSX = openpyxl.load_workbook('TCS3400_Spectral_Response_3-lots.xlsx')

# sheet = fileXLSX["Vornamen 2019 mit Angabe der Ra"]

# print(sheet['A2'].value)
