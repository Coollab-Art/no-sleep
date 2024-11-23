#define _CRT_SECURE_NO_WARNINGS // Disable warning about std::localtime
#include <imgui.h>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <no_sleep/no_sleep.hpp>
#include <optional>
#include <quick_imgui/quick_imgui.hpp>

auto main(int argc, char* argv[]) -> int
{
    {
        no_sleep::Scoped scoped{"MyApp", "MyApp is exporting a video", no_sleep::Mode::KeepScreenOnAndKeepComputing};
    }
    {
        no_sleep::Scoped scoped{"MyApp", "MyApp is doing some long computation", no_sleep::Mode::ScreenCanTurnOffButKeepComputing};
    }
    bool const should_run_imgui_tests = argc < 2 || strcmp(argv[1], "-nogpu") != 0; // NOLINT(*pointer-arithmetic)
    if (!should_run_imgui_tests)
        return 0;

    std::optional<no_sleep::Scoped> scope1{};
    std::optional<no_sleep::Scoped> scope2{};

    quick_imgui::AverageTime average_time{};

    int prev_sec{};

    quick_imgui::loop("no_sleep tests", [&]() {
        ImGui::Begin("no_sleep tests");
        average_time.start();

        {
            bool sleep_not_allowed = scope1.has_value();
            if (ImGui::Checkbox("Keep screen on and keep computing", &sleep_not_allowed))
            {
                if (sleep_not_allowed)
                    scope1.emplace("MyApp", "MyApp is exporting a video", no_sleep::Mode::KeepScreenOnAndKeepComputing);
                else
                    scope1.reset();
            }
        }

        {
            bool sleep_not_allowed = scope2.has_value();
            if (ImGui::Checkbox("Screen can turn off but keep computing", &sleep_not_allowed))
            {
                if (sleep_not_allowed)
                    scope2.emplace("MyApp", "MyApp is doing some long computation", no_sleep::Mode::ScreenCanTurnOffButKeepComputing);
                else
                    scope2.reset();
            }
        }

        average_time.stop();
        average_time.imgui_plot();
        ImGui::End();

        // Output the current hour, minute, and second
        std::time_t const    current_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm const* const local_time   = std::localtime(&current_time);
        if (prev_sec != local_time->tm_sec)
        {
            prev_sec = local_time->tm_sec;
            std::cout << "Current time: "
                      << std::setfill('0') << std::setw(2) << local_time->tm_hour << ":"
                      << std::setfill('0') << std::setw(2) << local_time->tm_min << ":"
                      << std::setfill('0') << std::setw(2) << local_time->tm_sec
                      << '\n';
        }
    });

    return 0;
}
