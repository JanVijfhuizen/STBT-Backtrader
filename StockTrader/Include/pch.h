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

constexpr int32_t DAY_MUL = 60 * 60 * 24;

#include "Ext/ImGuiDatePicker.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glad/glad.h> 
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <BackTrader.h>
#include <FPFNTester.h>
#include <GeneticAlgorithm.h>
#include <NNet.h>
#include <NNetUtils.h>
#include <TimeSeries.h>
#include <Tracker.h>

#endif //PCH_H