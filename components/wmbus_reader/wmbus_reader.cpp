#include "wmbus_reader.h"

#include "esphome/core/hal.h"
#include "esphome/components/network/ip_address.h"
#include <ctime>
#include <esp_netif.h>
#include <esp_heap_caps.h>
#include <esp_wifi.h>

#include "resources.h"

namespace esphome
{

    namespace wmbus_reader
    {

        static const char *NO_DATA = "---";
        DisplayScreen::DisplayScreen(int update_interval) : update_interval{update_interval},
                                                            page_{
                                                                [this](display::Display &it)
                                                                {
                                                                    this->frame_counter++;
                                                                    this->has_string_overflows = false;

                                                                    this->render_fcn(it);

                                                                    if (it.get_update_interval() != this->get_update_interval())
                                                                    {
                                                                        it.set_update_interval(this->get_update_interval());
                                                                        it.start_poller();
                                                                    }
                                                                }}
        {
        }

        display::DisplayPage *DisplayScreen::get_page()
        {
            return &this->page_;
        }

        void DisplayScreen::render_string(display::Display &it,
                                          int x,
                                          int y,
                                          display::TextAlign align,
                                          const char *text,
                                          int max_width)
        {
            int width, height, x0, y0;
            it.get_text_bounds(x, y, text, font_small, align, &x0, &y0, &width, &height);

            if (max_width == 0)
                max_width = it.get_width();
            auto overflow = std::max(width - max_width, 0);

            if (max_width && overflow)
            {
                auto clip = display::Rect(x0, 0, max_width, it.get_height());

                auto x_align = display::TextAlign(int(align) & 0x18);

                switch (x_align)
                {
                case display::TextAlign::RIGHT:
                    clip.x += overflow;
                    break;
                case display::TextAlign::CENTER_HORIZONTAL:
                    clip.x += overflow / 2;
                    break;
                // display::TextAlign::LEFT:
                default:
                    break;
                }

                it.start_clipping(clip);

                static const int dead_frames = 15;
                this->has_string_overflows = true;

                x0 = clip.x;
                auto frame_idx = this->frame_counter % (overflow + 2 * dead_frames);
                if (frame_idx > overflow + dead_frames)
                    x0 += -overflow;
                else if (frame_idx > dead_frames)
                    x0 += -frame_idx + dead_frames;
            }

            it.print(x0, y0, font_small, text);
            if (it.is_clipping())
                it.end_clipping();
        };

        void IntroDisplayScreen::render_fcn(display::Display &it)
        {
            it.print(it.get_width() / 2, 0, font_small, display::TextAlign::TOP_CENTER, std::strchr(ESPHOME_PROJECT_NAME, '.') + 1);
            it.image(it.get_width() / 2, it.get_height() / 2, logo, display::ImageAlign::CENTER);
            it.print(it.get_width() / 2, it.get_height(), font_small, display::TextAlign::BOTTOM_CENTER, "v" ESPHOME_PROJECT_VERSION);
        }

        DeviceInfoDisplayScreen::DeviceInfoDisplayScreen() : DisplayScreen{1000} {}

        void DeviceInfoDisplayScreen::render_fcn(display::Display &it)
        {
            int y = 0;
            int occupied, dummy;
            auto di = this->get_device_info();

            for (const auto &info : di)
            {
                it.print(0, y, font_small, info.first.c_str());
                font_small->measure((info.first + ' ').c_str(), &occupied, &dummy, &dummy, &dummy);
                this->render_string(it,
                                    it.get_width(),
                                    y,
                                    display::TextAlign::TOP_RIGHT,
                                    info.second.c_str(),
                                    it.get_width() - occupied);
                y += it.get_height() / di.size();
            }
        }

        std::vector<std::pair<std::string, std::string>> ConnectionInfoDisplayScreen::get_device_info()
        {
            std::string ssid = NO_DATA, ip = NO_DATA, rssi_ch = NO_DATA;

            wifi_mode_t mode;
            if (esp_wifi_get_mode(&mode) == ESP_OK)
            {
                switch (mode)
                {
                case WIFI_MODE_STA:
                    wifi_ap_record_t info;
                    if (esp_wifi_sta_get_ap_info(&info) == ESP_OK)
                    {
                        if (strlen(reinterpret_cast<const char *>(info.ssid)))
                            ssid = reinterpret_cast<const char *>(info.ssid);

                        rssi_ch = std::to_string(info.rssi) + "dBm CH" + std::to_string(info.primary);
                    }
                    break;
                case WIFI_MODE_AP:
                case WIFI_MODE_APSTA:
                    wifi_config_t config;
                    if (esp_wifi_get_config(WIFI_IF_AP, &config) == ESP_OK)
                    {
                        ssid = reinterpret_cast<const char *>(config.ap.ssid);

                        wifi_sta_list_t sta_list;
                        if (esp_wifi_ap_get_sta_list(&sta_list) == ESP_OK && sta_list.num > 0)
                            rssi_ch = std::to_string(sta_list.sta[0].rssi) + "dBm";
                        rssi_ch += " CH" + to_string(config.ap.channel);
                    }

                    break;
                }

                auto *default_netif = esp_netif_get_default_netif();
                esp_netif_ip_info_t ip_info;

                if (default_netif && esp_netif_get_ip_info(default_netif, &ip_info) == ESP_OK)
                    ip = network::IPAddress(&ip_info.ip).str();
            }

            return {
                {"AP", ssid},
                {"IP", ip},
                {"RSSI", rssi_ch},
                {"MAC", get_mac_address()},
            };
        }

        std::vector<std::pair<std::string, std::string>> RuntimeInfoDisplayScreen::get_device_info()
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

        void MoreInfoScreen::render_fcn(display::Display &it)
        {
            it.print(0, 0, font_small, "More");
            it.print(0, font_small->get_height(), font_small, "info");
            it.print(0, font_small->get_height() * 2, font_small, "at:");

            const uint8_t scale = 3;
            const uint8_t x_offset = 65;
            const uint8_t y_offset = 1;

            for (uint8_t x = 0; x < qr_code->get_width(); x++)
                for (uint8_t y = 0; y < qr_code->get_height(); y++)
                    if (qr_code->get_pixel(x, y).is_on())
                        it.filled_rectangle(x * scale + x_offset, y * scale + y_offset, scale, scale);
        }

#ifdef USE_WMBUS_METER_SENSOR
        SensorDisplayScreen::SensorDisplayScreen(wmbus_meter::Sensor *sensor) : DisplayScreen{60000}, sensor{sensor}
        {
            this->sensor->add_on_state_callback([this](float state)
                                                { this->last_update = millis(); });
        }

        void SensorDisplayScreen::render_fcn(display::Display &it)
        {
            this->render_string(it, 0, 0, display::TextAlign::TOP_LEFT, this->sensor->get_name().c_str());
            this->render_value(it);
            this->render_time_ago(it);
        }

        void SensorDisplayScreen::render_value(display::Display &it)
        {
            std::string data = NO_DATA;
            if (!std::isnan(sensor->get_state()))
            {
                data = std::to_string(sensor->get_state());
                size_t pos = data.find('.');
                if (pos != std::string::npos)
                {
                    if (sensor->get_accuracy_decimals() > 0)
                        pos += 1 + sensor->get_accuracy_decimals();
                    data = str_truncate(data, std::min(pos, 11 - sensor->get_unit_of_measurement().length()));
                }
                data += sensor->get_unit_of_measurement();
            }
            it.print(it.get_width(),
                     it.get_height() / 2,
                     font_large,
                     display::TextAlign::CENTER_RIGHT,
                     data.c_str());
        }

        void SensorDisplayScreen::render_time_ago(display::Display &it)
        {
            std::string data = NO_DATA;
            if (!std::isnan(sensor->get_state()))
                data = std::to_string((millis() - this->last_update) / 1000 / 60) + " minutes ago";

            it.print(it.get_width(),
                     it.get_height(),
                     font_small,
                     display::TextAlign::BOTTOM_RIGHT,
                     data.c_str());
        }
#endif
    }
}
