#pragma once
namespace SamplerKit::Bootloader
{

enum class Animation { WAITING, WRITING, RECEIVING, SYNC, DONE, SUCCESS, FAIL_ERR, FAIL_SYNC, FAIL_CRC, RESET };

void animate(Animation animation_type);
} // namespace SamplerKit::Bootloader
