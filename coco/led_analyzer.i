%module led_analyzer

%include <carrays.i>
// this declares a batch of function for manipulating C integer arrays
%array_functions(unsigned short, ushort)
%array_functions(unsigned long, ulong)
%array_functions(int, integer)
%array_functions(void*, apvoid)
%array_functions(char*, astring)
%array_functions(unsigned char, puchar)
%array_functions(float, afloat)

%include <cpointer.i>

//%pointer_functions(void, pvoid)

// Usable functions with carrays:
// new_ushort(elements)
// delete_ushort(what array?)
// ushort_getitem(whatarray?, index)
// name_setitem(whatarary?, index, Writevalue)



%include "led_analyzer.h"

%{
	#include "led_analyzer.h"
%}

%include <typemaps.i>
//%apply void **INOUT {(void** apHandles)};
%apply (unsigned short *INOUT) {unsigned short* ausClear, unsigned short* ausRed, unsigned short* ausGreen, unsigned short* ausBlue, unsigned short* ausCCT};
%apply (float* INOUT) {float* afLUX}
%apply (unsigned short *INOUT) {unsigned short* ausGains}
%apply (unsigned short *INOUT) {unsigned short* ausIntTimeSettings}
//%apply (char **INOUT) {(char** asSerial)}
//%apply (char *INOUT) {char* aucGains}
//%apply (char *INOUT) {char* aucIntegrationtime}



// 3 possibilities to work with arrays:
// 1) Use Standard C function with carrays
// 2) use c function which you can pass a table and with %luacode% in the .i file 
//    you internally create new c arrays
// 3) Use typemaps and just pass a table