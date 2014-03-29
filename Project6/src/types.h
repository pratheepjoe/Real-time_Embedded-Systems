/******************************************************************************
 * Define common types
 *
 * Description:
 *
 * Different microprocessors and compilers use different length
 * intrinsic values. The values defined here are universal.
 * 
 * Author:
 *  Tom Bullinger (09/07/2011)	Initial creation
 *
 *****************************************************************************/

// Signed 8-bit Type
#ifndef INT8
typedef signed char         INT8;
#endif

// Signed 16-bit Type
#ifndef INT16
typedef signed int          INT16;
#endif

// Signed 32-bit Type
#ifndef INT32
typedef signed long int     INT32;
#endif

// Unsigned 8-bit Type
#ifndef UINT8
typedef unsigned char       UINT8;
#endif

// Unsigned 16-bit Type
#ifndef UINT16
typedef unsigned int        UINT16;
#endif

// Unsigned 32-bit Type
#ifndef UINT32
typedef unsigned long int   UINT32;
#endif