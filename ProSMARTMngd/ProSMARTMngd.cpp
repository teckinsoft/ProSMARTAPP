// ProSMARTMngd.cpp : Defines the exported functions for the DLL.
//

#include "pch.h"
#include "framework.h"
#include "ProSMARTMngd.h"


// This is an example of an exported variable
PROSMARTMNGD_API int nProSMARTMngd = 0;

// This is an example of an exported function.
PROSMARTMNGD_API int fnProSMARTMngd(void)
{
   return 0;
}

// This is the constructor of a class that has been exported.
CProSMARTMngd::CProSMARTMngd()
{
   return;
}
