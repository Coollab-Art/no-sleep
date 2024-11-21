#include <imgui.h>
#include <no_sleep/no_sleep.hpp>
#include <optional>
#include <quick_imgui/quick_imgui.hpp>

// Learn how to use Dear ImGui: https://coollibs.github.io/contribute/Programming/dear-imgui

auto main(int argc, char* argv[]) -> int
{
    bool const should_run_imgui_tests = argc < 2 || strcmp(argv[1], "-nogpu") != 0; // NOLINT(*pointer-arithmetic)
    if (!should_run_imgui_tests)
        return 0;

    std::optional<no_sleep::Scoped> scope1{};
    std::optional<no_sleep::Scoped> scope2{};

    quick_imgui::loop("no_sleep tests", [&]() { // Open a window and run all the ImGui-related code
        ImGui::Begin("no_sleep tests");
        bool sleep_not_allowed = scope1.has_value();
        if (ImGui::Checkbox("Sleep not allowed", &sleep_not_allowed))
        {
            if (sleep_not_allowed)
                scope1.emplace();
            else
                scope1.reset();
        }
        bool sleep_not_allowed2 = scope2.has_value();
        if (ImGui::Checkbox("Sleep not allowed2", &sleep_not_allowed2))
        {
            if (sleep_not_allowed2)
                scope2.emplace();
            else
                scope2.reset();
        }
        ImGui::End();
        ImGui::ShowDemoWindow();
    });

    return 0;
}
