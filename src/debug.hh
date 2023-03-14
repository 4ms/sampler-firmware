#pragma once
#include "brain_conf.hh"

namespace Debug = Brain::Debug; // NOLINT

#define GCC_OPTIMIZE_OFF __attribute__((optimize("-O0")))
