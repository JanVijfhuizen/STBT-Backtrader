#include "pch.h"
#include "Ext/ImGuiTextUtils.h"

namespace ImGui
{
    void TextCenter(std::string text)
    {
        float font_size = ImGui::GetFontSize() * text.size() / 2;
        ImGui::SameLine(
            ImGui::GetWindowSize().x / 2 -
            font_size + (font_size / 2)
        );

        ImGui::Text(text.c_str());
    }

    bool ButtonCenter(std::string text)
    {
        float font_size = ImGui::GetFontSize() * text.size() / 2;
        ImGui::SameLine(
            ImGui::GetWindowSize().x / 2 -
            font_size + (font_size / 2)
        );

        return ImGui::Button(text.c_str());
    }
}