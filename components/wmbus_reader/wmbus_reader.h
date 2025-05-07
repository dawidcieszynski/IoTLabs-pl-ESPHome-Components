#pragma once

#include <functional>
#include <vector>
#include <string>

#include "esphome/core/base_automation.h"
#include "esphome/components/display/display.h"

#ifdef USE_WMBUS_METER_SENSOR
#include "esphome/components/wmbus_meter/sensor.h"
#endif

namespace esphome
{

    namespace wmbus_reader
    {

        class DisplayScreen
        {
        public:
            DisplayScreen(int update_interval = UINT32_MAX);
            display::DisplayPage *get_page();
            void reset_frame_counter() { this->frame_counter = 0; };
            int get_update_interval() { return this->has_string_overflows ? 100 : this->update_interval; };

        protected:
            display::DisplayPage page_;
            virtual void render_fcn(display::Display &it) = 0;

            int update_interval;
            int frame_counter = 0;
            bool has_string_overflows = false;

            void render_string(display::Display &it,
                               int x,
                               int y,
                               display::TextAlign align,
                               const char *text,
                               int max_width = 0);
        };

        class IntroDisplayScreen : public DisplayScreen
        {
        protected:
            void render_fcn(display::Display &it) override;
        };

        class DeviceInfoDisplayScreen : public DisplayScreen
        {
        public:
            DeviceInfoDisplayScreen();

        protected:
            virtual std::vector<std::pair<std::string, std::string>> get_device_info() = 0;
            void render_fcn(display::Display &it) override;
        };

        class ConnectionInfoDisplayScreen : public DeviceInfoDisplayScreen
        {
        protected:
            std::vector<std::pair<std::string, std::string>> get_device_info() override;
        };

        class RuntimeInfoDisplayScreen : public DeviceInfoDisplayScreen
        {
        protected:
            std::vector<std::pair<std::string, std::string>> get_device_info() override;
        };

        class MoreInfoScreen : public DisplayScreen
        {
        protected:
            void render_fcn(display::Display &it) override;
        };

#ifdef USE_WMBUS_METER_SENSOR
        class SensorDisplayScreen : public DisplayScreen
        {
        public:
            SensorDisplayScreen(wmbus_meter::Sensor *sensor);

        protected:
            wmbus_meter::Sensor *sensor;

            uint32_t last_update = 0;

            void render_fcn(display::Display &it) override;

            void render_value(display::Display &it);
            void render_time_ago(display::Display &it);
        };
#endif

        class DisplayScreenManager
        {
        public:
            DisplayScreenManager(display::Display *it) : it(it)
            {
                auto trigger = new display::DisplayOnPageChangeTrigger(it);
                auto skip_automation = new Automation(trigger);
                skip_automation->add_action(new LambdaAction<
                                            display::DisplayPage *,
                                            display::DisplayPage *>([this](
                                                                        display::DisplayPage *from,
                                                                        display::DisplayPage *to)
                                                                    { this->on_page_change(from, to); }));
            }

            void sync()
            {
                std::vector<display::DisplayPage *> pages;

                for (auto screen : this->all_screens())
                    pages.push_back(screen->get_page());

                this->it->set_pages(pages);
            }

#ifdef USE_WMBUS_METER_SENSOR
        public:
            void add_sensor(wmbus_meter::Sensor *sensor)
            {
                auto screen = new SensorDisplayScreen{sensor};
                this->main_screens.push_back(screen);
                sensor->add_on_state_callback(
                    [this, screen](float state)
                    {
                        if (this->it->get_active_page() == screen->get_page())
                            this->it->update();
                    });
            }
#endif

        private:
            std::vector<DisplayScreen *> init_screens{new IntroDisplayScreen()};
            std::vector<DisplayScreen *> main_screens;
            std::vector<DisplayScreen *> status_screens{new ConnectionInfoDisplayScreen(),
                                                        new RuntimeInfoDisplayScreen(),
                                                        new MoreInfoScreen()};

            display::Display *it;

            std::vector<DisplayScreen *> all_screens() const
            {
                std::vector<DisplayScreen *> all_screens;

                for (auto &screen : this->init_screens)
                    all_screens.push_back(screen);

                for (auto &screen : this->main_screens)
                    all_screens.push_back(screen);

                for (auto &screen : this->status_screens)
                    all_screens.push_back(screen);

                return all_screens;
            }

            void on_page_change(display::DisplayPage *from, display::DisplayPage *to)
            {
                if (this->init_screens.front()->get_page() == from)
                {
                    delete this->init_screens[0];
                    this->init_screens.erase(this->init_screens.begin());
                    this->sync();
                }

                for (auto screen : this->all_screens())
                    screen->reset_frame_counter();
            }
        };
    }
}
