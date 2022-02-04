#ifndef _NXSYS_PRODUCT_NAME_H__
#define _NXSYS_PRODUCT_NAME_H__

#ifdef NXV2

 extern long RTExpireTime;

 #ifdef RT_PRODUCT
   #define PRODUCT_NAME "RT-Designer"
   #define EXE_NAME "rtd"
   #define COMPANY "Basis Technology"
 #else
  #ifndef PRODUCT_NAME
   #define PRODUCT_NAME "NXSYS"
  #endif
   #define EXE_NAME "nxv2"
   #define COMPANY "B.Greenberg"
 #endif
 #define V_PRODUCT_NAME "V2 " PRODUCT_NAME
 #define PRODUCT_NAME_V PRODUCT_NAME "V2"
#else
 #define PRODUCT_NAME "NXSYS"
 #define V_PRODUCT_NAME PRODUCT_NAME
 #define PRODUCT_NAME_V PRODUCT_NAME
 #define EXE_NAME "nxsys32"
#define COMPANY "B.Greenberg"
#endif

#endif

