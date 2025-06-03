#pragma once

#include "screen_info.h"

namespace esphome
{
    namespace wmbus_gateway_gui
    {
        class RuntimeInfoScreen : public InfoScreen
        {
        protected:
            std::vector<std::pair<std::string, std::string>> collect_data() override
            {
                int uptime = millis() / 1000;
                int seconds = uptime % 60;
                uptime /= 60;
                int minutes = uptime % 60;
                uptime /= 60;
                int hours = uptime % 24;
                uptime /= 24;
                int days = uptime;

                std::vector<std::pair<int, std::string>> time_parts = {
                    {days, "d"},
                    {hours, "h"},
                    {minutes, "m"},
                    {seconds, "s"}};

                std::string uptime_str;

                for (auto it = time_parts.begin(); it != time_parts.end(); ++it)
                {
                    if (it->first)
                    {
                        uptime_str = std::to_string(it->first) + it->second;
                        ++it;
                        if (it != time_parts.end())
                            uptime_str += std::to_string(it->first) + it->second;
                        break;
                    }
                }

                std::time_t epoch = std::time(NULL);
                std::tm *calendar_time = std::localtime(&epoch);

                std::string date{NO_DATA, 10 + 1}, time{NO_DATA, 8 + 1};

                if (calendar_time && calendar_time->tm_year >= 125)
                {
                    std::strftime(date.data(), date.size(), "%Y-%m-%d", calendar_time);
                    std::strftime(time.data(), time.size(), "%H:%M:%S", calendar_time);
                }

                return {
                    {"Date", date},
                    {"Time", time},
                    {"Uptime", uptime_str},
                    {"FreeMem", std::to_string(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)) + 'B'},
                };
            }
        };
    }
}