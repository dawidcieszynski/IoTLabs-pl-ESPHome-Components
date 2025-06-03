#include "screen.h"

namespace esphome
{

    namespace wmbus_gateway_gui
    {

        const char *Screen::NO_DATA = "---";

        Screen::Screen(uint32_t update_interval) : update_interval{update_interval}
        {
        }

        uint32_t Screen::get_update_interval()
        {
            if (this->need_animations)
                return 100;

            return this->update_interval;
        }

        void Screen::render(display::Display &it)
        {
            this->frame_counter++;

            it.clear();
            this->render_fcn(it);
        }

        void Screen::reinit()
        {
            this->need_animations = false;
            this->frame_counter = 0;
        }

        void Screen::render_string(display::Display &it,
                                   int x,
                                   int y,
                                   display::TextAlign align,
                                   font::Font *font,
                                   const char *text,
                                   int max_width)
        {
            int width, height, x0, y0;
            it.get_text_bounds(x, y, text, font, align, &x0, &y0, &width, &height);

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

                static const uint8_t dead_frames = 15;
                this->need_animations = true;

                x0 = clip.x;
                auto frame_idx = this->frame_counter % (overflow + 2 * dead_frames);
                if (frame_idx > overflow + dead_frames)
                    x0 += -overflow;
                else if (frame_idx > dead_frames)
                    x0 += -frame_idx + dead_frames;
            }

            it.print(x0, y0, font, text);
            if (it.is_clipping())
                it.end_clipping();
        };
    }
}