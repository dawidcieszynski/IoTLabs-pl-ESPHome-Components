#pragma once

#include "esphome/components/display/display.h"
#include "esphome/components/font/font.h"

namespace esphome
{

    namespace wmbus_gateway_gui
    {

        class Screen
        {
        public:
            Screen(uint32_t update_interval = UINT32_MAX);

            uint32_t get_update_interval();

            void render(display::Display &it);
            void reinit();

            Screen *next = nullptr;

        protected:
            bool need_animations = false;
            uint32_t frame_counter = 0;
            uint32_t update_interval;

            virtual void render_fcn(display::Display &it) = 0;

            void render_string(display::Display &it,
                               int x,
                               int y,
                               display::TextAlign align,
                               font::Font *font,
                               const char *text,
                               int max_width = 0);

            static const char *NO_DATA;
        };
    }
}