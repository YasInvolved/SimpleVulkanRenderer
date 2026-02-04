#pragma once

extern "C" char* vert_spv_start;
extern "C" char* vert_spv_end;

extern "C" char* frag_spv_start;
extern "C" char* frag_spv_end;

namespace shaders
{
	const uint32_t* vert_code = reinterpret_cast<const uint32_t*>(&vert_spv_start);
	const size_t vert_code_size = ((&vert_spv_end) - (&vert_spv_start)) / sizeof(uint32_t);

	const uint32_t* frag_code = reinterpret_cast<const uint32_t*>(&frag_spv_start);
	const size_t frag_code_size = ((&frag_spv_end) - (&frag_spv_start)) / sizeof(uint32_t);
}