#ifndef FX_SUPPORT_H
#define FX_SUPPORT_H

#include <ArduboyFX.h>

#if __has_include("fxdata/fxdata.h")
#include "fxdata/fxdata.h"
#else
constexpr uint16_t FX_DATA_PAGE = 0xFFFF;
#endif

inline void initFxChip()
{
    FX::begin(FX_DATA_PAGE);
}

#endif
