#include <cassert>
#include <stdexcept>
#include <span>
#include <vector>
#include <array>
#include <algorithm>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <volk.h>
#include <fmt/format.h>
#include <fmt/color.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>