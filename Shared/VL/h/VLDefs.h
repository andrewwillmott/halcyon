// Zero-dependency definitions, for use with VL_NO_CL

#include <stdint.h>

typedef float Real;

#define CL_ASSERT(M_X)
#define CL_ASSERT_MSG(M_X, M_MSG)
#define CL_RANGE_MSG(M_X, M_0, M_1, M_MSG)
#define CL_ERROR(M_X)
#define CL_WARNING(M_X)
#define CL_RANGE(I, A, B)
#define CL_INDEX_MSG(I, N, MSG)

#define VL_NEW new
#define VL_DELETE delete
#define VL_RESTRICT
