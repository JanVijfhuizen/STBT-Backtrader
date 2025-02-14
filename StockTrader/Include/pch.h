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

constexpr int32_t DAY_MUL = 60 * 60 * 24;

#include "Ext/ImGuiDatePicker.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glad/glad.h> 
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <Jlib/Arena.h>
#include <JLib/Iterator.h>
#include <JLib/Array.h>
#include <JLib/Menu.h>

#include <TimeSeries.h>
#include <Tracker.h>
#include <STBT.h>

#endif //PCH_H