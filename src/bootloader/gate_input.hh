#pragma once
#include <cstdint>
#include <functional>

void init_gate_in();
bool gate_in_read(uint32_t threshold);
void start_reception(uint32_t sample_rate, std::function<void()> &&callback);
