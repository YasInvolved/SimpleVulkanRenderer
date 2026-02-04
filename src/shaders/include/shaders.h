#pragma once

#include <stdint.h>

extern "C" char vert_spv_start;
extern "C" char vert_spv_end;

extern "C" char frag_spv_start;
extern "C" char frag_spv_end;

namespace shaders
{
	const uint32_t* vert_code_start = reinterpret_cast<const uint32_t*>(&vert_spv_start);
	const uint32_t* vert_code_end = reinterpret_cast<const uint32_t*>(&vert_spv_end);
	const size_t vert_code_size = (vert_code_end - vert_code_start) * sizeof(uint32_t);

	const uint32_t* frag_code_start = reinterpret_cast<const uint32_t*>(&frag_spv_start);
	const uint32_t* frag_code_end = reinterpret_cast<const uint32_t*>(&frag_spv_end);
	const size_t frag_code_size = (frag_spv_end - frag_spv_start) * sizeof(uint32_t);
}