// MIT License
// 
// Copyright(c) 2026 Jakub Bączyk
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <stdint.h>

extern "C" char vert_spv_start;
extern "C" char vert_spv_end;

extern "C" char frag_spv_start;
extern "C" char frag_spv_end;

namespace shaders
{
	const char* vert_code_start = &vert_spv_start;
	const char* vert_code_end = &vert_spv_end;
	const size_t vert_code_size = vert_code_end - vert_code_start;

	const char* frag_code_start = &frag_spv_start;
	const char* frag_code_end = &frag_spv_end;
	const size_t frag_code_size = frag_code_end - frag_code_start;
}
