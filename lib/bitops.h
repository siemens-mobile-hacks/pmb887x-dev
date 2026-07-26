#pragma once

#define BITS_PER_LONG		32
#define BITS_PER_LONG_LONG	64

#define BIT(n)				(1 << (n))

#define __AC(X,Y)			(X##Y)
#define _AC(X,Y)			__AC(X,Y)
#define _AT(T,X)			((T)(X))
#define UL(x)				(_AC(x, UL))
#define ULL(x)				(_AC(x, ULL))

#define GENMASK(h, l) \
(((~UL(0)) - (UL(1) << (l)) + 1) & \
(~UL(0) >> (BITS_PER_LONG - 1 - (h))))

#define GENMASK_ULL(h, l) \
(((~ULL(0)) - (ULL(1) << (l)) + 1) & \
(~ULL(0) >> (BITS_PER_LONG_LONG - 1 - (h))))
