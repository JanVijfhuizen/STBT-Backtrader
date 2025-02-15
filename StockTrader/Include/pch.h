#ifndef PCH_H
#define PCH_H

#define _CRT_SECURE_NO_WARNINGS
#include <framework.h>
#include <cstdint>
#include <cassert>
#include <exception>
#include <new.h>
#include <iostream>
#include <fstream>
#include <string>
#include "gnuplot-iostream.h"
#include <filesystem>
#include <format>

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include "Ext/ImGuiDatePicker.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glad/glad.h> 
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

const int32_t DAY_MUL = 60 * 60 * 24;
const glm::ivec2 RESOLUTION{ 600, 400 };
const uint32_t MENU_WIDTH = 200;
const glm::vec2 MENU_RESOLUTION_SMALL{ MENU_WIDTH, 100 };
const glm::vec2 MENU_RESOLUTION = MENU_RESOLUTION_SMALL * glm::vec2(1, 2);
const glm::vec2 MENU_RESOLUTION_LARGE = MENU_RESOLUTION_SMALL * glm::vec2(1, 4);

const auto WIN_FLAGS = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
const uint32_t DAYS_DEFAULT = 28;

#include <Jlib/Arena.h>
#include <JLib/Iterator.h>
#include <JLib/Array.h>
#include <JLib/Menu.h>

#include <TimeSeries.h>
#include <Tracker.h>
#include <STBT.h>

#endif //PCH_H