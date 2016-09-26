#ifndef __MIN_MAX_ABS_SQR_H__
#define __MIN_MAX_ABS_SQR_H__

#ifndef MIN
#define MIN(x,y) ((x) > (y) ? (y) : (x))
#endif
#ifndef MAX
#define MAX(x,y) ((x) < (y) ? (y) : (x))
#endif
#ifndef ABS
#define ABS(x) (((x) < 0) ? -(x) : (x))
#endif
#ifndef SQR
#define SQR(x) ((x) * (x))
#endif
#ifndef LIMIT
#define LIMIT(d, th) MAX(MIN(d, th), -(th))
#endif
#ifndef CLIP10
#define CLIP10(x) MAX(MIN(x, 1023), 0)
#endif
#ifndef CLIP
#define CLIP(x,a,b)           MIN(b,MAX(a,x))
#endif

#endif

