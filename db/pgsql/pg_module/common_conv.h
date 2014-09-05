#ifndef _COMMON_CONV_H
#define _COMMON_CONV_H 1

#include <math.h>

/**
 * Round to nearest integer
 *
 * @param      v  Input (double)
 * @return     Rounded integer value
 */
static int round_d2i( double v )
{
  if ( v < 0 ) return (int)(v-0.5);
  else return (int)(v+0.5);
}

#define CC_DEG_TO_RAD 0.017453292519943295
#define CC_RAD_TO_DEG 57.295779513082323
#define CC_ARCMIN_TO_RAD 0.0002908882086657216
#define CC_RAD_TO_ARCMIN 3437.7467707849396

#define RA2RAI(v) round_d2i(((v)-180.0)*1e7)
#define DEC2DECI(v) round_d2i((v)*1e7)
#define RAI2RA(v)  ((double)((long long)(v) + 1800000000LL)/1e7)
#define DECI2DEC(v) ((double)(v)/1e7)

/* prec = about 9 arcmin */
#define SC_XYZ_TO_XYZI16 32400.0
#define GET_I16_UVEC(v) ((int)(round_d2i((v) * (SC_XYZ_TO_XYZI16))))
#define GET_I16_UVEC_FLOOR(v) ((int)(floor((v) * (SC_XYZ_TO_XYZI16))))
#define GET_I16_UVEC_CEIL(v) ((int)(ceil((v) * (SC_XYZ_TO_XYZI16))))

#endif
