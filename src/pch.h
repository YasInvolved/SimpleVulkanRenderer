#include <cassert>
#include <stdexcept>
#include <span>
#include <vector>
#include <array>
#include <algorithm>
#include <chrono>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <volk.h>
#include <fmt/format.h>
#include <fmt/color.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <argparse/argparse.hpp>
#include <tiny_obj_loader.h>