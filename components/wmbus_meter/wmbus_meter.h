#pragma once
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

#include "esphome/components/wmbus_radio/component.h"
#include "esphome/components/wmbus_common/meters.h"

namespace esphome
{
    namespace wmbus_meter
    {
        class Meter : public Component
        {
        public:
            void set_meter_params(std::string id, std::string driver, std::string key);
            void set_radio(wmbus_radio::Radio *radio);

            void dump_config() override;

            void on_telegram(std::function<void()> &&callback);

            std::string get_id() const;

            std::string as_json(bool pretty_print = false);
            optional<std::string> get_string_field(std::string field_name);
            optional<float> get_numeric_field(std::string field_name);
            
            std::unique_ptr<Telegram> last_telegram;

        protected:
            wmbus_radio::Radio *radio;

            std::string id_;
            std::string driver_;
            std::string key_;

            std::shared_ptr<::Meter> meter;

            CallbackManager<void()> on_telegram_callback_manager;

            void create_meter(std::string driver);

            virtual void handle_frame(wmbus_radio::Frame *frame);
        };
    }
}