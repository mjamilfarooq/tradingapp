// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the AVICENNA_FLEX_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// AVICENNA_FLEX_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.

#ifndef AVICENNASUPERPLUS_H
#define	AVICENNASUPERPLUS_H

#include "Definition.h"

class CSMQAvicennaSuperPlusImpl;
#ifdef _WINDOWSDLL

	#ifdef _AVICENNASUPERPLUS_EXPORTS
	#define AVICENNASUPERPLUS_API __declspec(dllexport)
	#else
	#define AVICENNASUPERPLUS_API CAvicenna_SuperPlus {
        #endif
#else
    class CAvicenna_SuperPlus {
#endif
public:
	CAvicenna_SuperPlus();
        void SetIP(char *r_ip);
	void SetVariableConfig(char *r_Param1,  char *r_Param3, char *r_Param4,
                               char *r_Decimal ); //= NULL);	
	char * CalculateAvicennaSuperPlus(double r_Input);
        double GetExportValue4();

	~CAvicenna_SuperPlus();
               
private:
	CSMQAvicennaSuperPlusImpl *const m_ptrImpl;	// TODO: add your methods here.
        
};

#ifdef _WINDOWSDLL
	extern AVICENNASUPERPLUS_API int nSuperPlus;
	AVICENNASUPERPLUS_API int fnSuperPlus(void);
#endif

#endif

