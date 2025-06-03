#pragma once

#include "screen.h"
#include "resources.h"

namespace esphome
{

    namespace wmbus_gateway_gui
    {

        class SplashScreen : public Screen
        {
        protected:
            void render_fcn(display::Display &it) override
            {
                it.print(it.get_width() / 2, 0, font_small, display::TextAlign::TOP_CENTER, std::strchr(ESPHOME_PROJECT_NAME, '.') + 1);
                it.image(it.get_width() / 2, it.get_height() / 2, logo, display::ImageAlign::CENTER);
                it.print(it.get_width() / 2, it.get_height(), font_small, display::TextAlign::BOTTOM_CENTER, "v" ESPHOME_PROJECT_VERSION);
            }
        };
    }
}