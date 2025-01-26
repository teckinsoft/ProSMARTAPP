// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the PROSMARTMNGD_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// PROSMARTMNGD_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef PROSMARTMNGD_EXPORTS
#define PROSMARTMNGD_API __declspec(dllexport)
#else
#define PROSMARTMNGD_API __declspec(dllimport)
#endif

// This class is exported from the dll
class PROSMARTMNGD_API CProSMARTMngd {
public:
	CProSMARTMngd(void);
	// TODO: add your methods here.
};

extern PROSMARTMNGD_API int nProSMARTMngd;

PROSMARTMNGD_API int fnProSMARTMngd(void);
