#ifndef _COMM_TYPE_H_
#define _COMM_TYPE_H_


#ifdef __cplusplus
extern "C" {
#endif



typedef signed int      INT32, *PINT32;
typedef signed short    INT16, *PINT16;
typedef signed char     INT8, *PINT8;
typedef unsigned int    UINT32, *PUINT32;
typedef unsigned short  UINT16, *PUINT16;
typedef unsigned char   UINT8, *PUINT8;
typedef int             BOOL, *PBOOL;
typedef unsigned char   BYTE, *PBYTE;

typedef float           FLOAT32, *PFLOAT32;
typedef double          FLOAT64, *PFLOAT64;


#ifndef TRUE
#define TRUE     1
#endif

#ifndef FALSE
#define FALSE    0


#define POW_2_POS32       (4294967296.0)
#define POW_2_POS17       (131072.0)


#endif


#ifdef __cplusplus
}
#endif

#endif//_COMM_TYPE_H_






