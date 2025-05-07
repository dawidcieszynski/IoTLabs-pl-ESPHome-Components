#include "component.h"

#include "freertos/task.h"
#include "freertos/queue.h"

#define ASSERT(expr, expected, before_exit)                       \
  {                                                               \
    auto result = (expr);                                         \
    if (!!result != expected)                                     \
    {                                                             \
      ESP_LOGE(TAG, "Assertion failed: %s -> %d", #expr, result); \
      before_exit;                                                \
      return;                                                     \
    }                                                             \
  }

#define ASSERT_SETUP(expr) ASSERT(expr, 1, this->mark_failed())

namespace esphome
{
  namespace wmbus_radio
  {
    static const char *TAG = "wmbus";

    void Radio::setup()
    {
      ASSERT_SETUP(this->packet_queue_ = xQueueCreate(3, sizeof(Packet *)));

      ASSERT_SETUP(xTaskCreate(
          (TaskFunction_t)this->receiver_task,
          "radio_recv",
          3 * 1024,
          this,
          2,
          &(this->receiver_task_handle_)));

      ESP_LOGI(TAG, "Receiver task created [%p]", this->receiver_task_handle_);

      this->radio->attach_data_interrupt(Radio::wakeup_receiver_task_from_isr, &(this->receiver_task_handle_));
    }

    void Radio::loop()
    {
      Packet *p;
      if (xQueueReceive(this->packet_queue_, &p, 0) != pdPASS)
        return;

      auto frame = p->convert_to_frame();

      // std::optional<wmbus_radio::Frame> frame = {};

      // static int counter = 0;

      // if (counter++ % 100==0)
      // {
      //   const uint8_t data[] = {
      //       0x4e,
      //       0x44,
      //       0x1,
      //       0x6,
      //       0x38,
      //       0x98,
      //       0x65,
      //       0x0,
      //       0x5,
      //       0x7,
      //       0x7a,
      //       0x58,
      //       0x0,
      //       0x40,
      //       0x85,
      //       0xb0,
      //       0xc4,
      //       0x67,
      //       0x8e,
      //       0x19,
      //       0x89,
      //       0xd0,
      //       0x3b,
      //       0x4a,
      //       0xb3,
      //       0x13,
      //       0xa7,
      //       0x22,
      //       0xee,
      //       0x73,
      //       0x8a,
      //       0x19,
      //       0x6c,
      //       0x12,
      //       0xda,
      //       0x69,
      //       0xf,
      //       0x23,
      //       0x19,
      //       0x4d,
      //       0x46,
      //       0xd1,
      //       0xd2,
      //       0xbb,
      //       0x76,
      //       0xee,
      //       0xbe,
      //       0x5d,
      //       0x21,
      //       0xb8,
      //       0xa6,
      //       0x82,
      //       0xe0,
      //       0xe6,
      //       0x93,
      //       0x79,
      //       0xc2,
      //       0x89,
      //       0x9a,
      //       0x7a,
      //       0xa7,
      //       0x53,
      //       0xde,
      //       0x35,
      //       0xc2,
      //       0x14,
      //       0xa1,
      //       0x34,
      //       0x57,
      //       0x67,
      //       0x17,
      //       0x11,
      //       0xea,
      //       0xa8,
      //       0xfe,
      //       0x80,
      //       0x64,
      //       0xfb,
      //       0xf2,
      //   };

      //   std::vector<uint8_t> data_vec(data, data + sizeof(data));

      //   frame = wmbus_radio::Frame{data_vec, LinkMode::T1, -100};
      // }

      if (!frame)
        return;

      ESP_LOGI(TAG, "Have data from radio (%zu bytes) [RSSI: %d, mode:%s]", frame->data().size(), frame->rssi(), toString(frame->link_mode()));

      uint8_t packet_handled = 0;
      for (auto &handler : this->handlers_)
        handler(&frame.value());

      ESP_LOGI(TAG, "Telegram handled by %d handlers", frame->handlers_count());
    }

    void Radio::wakeup_receiver_task_from_isr(TaskHandle_t *arg)
    {
      BaseType_t xHigherPriorityTaskWoken;
      vTaskNotifyGiveFromISR(*arg, &xHigherPriorityTaskWoken);
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

    void Radio::receive_frame()
    {
      this->radio->restart_rx();

      if (!ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(60000)))
      {
        ESP_LOGD(TAG, "Radio interrupt timeout");
        return;
      }
      auto packet = std::make_unique<Packet>();

      if (!this->radio->read_in_task(packet->rx_data_ptr(), packet->rx_capacity()))
      {
        ESP_LOGV(TAG, "Failed to read preamble");
        return;
      }

      if (!packet->calculate_payload_size())
      {
        ESP_LOGD(TAG, "Cannot calculate payload size");
        return;
      }

      if (!this->radio->read_in_task(packet->rx_data_ptr(), packet->rx_capacity()))
      {
        ESP_LOGW(TAG, "Failed to read data");
        return;
      }

      packet->set_rssi(this->radio->get_rssi());
      auto packet_ptr = packet.get();

      if (xQueueSend(this->packet_queue_, &packet_ptr, 0) == pdTRUE)
      {
        ESP_LOGV(TAG, "Queue items: %zu", uxQueueMessagesWaiting(this->packet_queue_));
        ESP_LOGV(TAG, "Queue send success");
        packet.release();
      }
      else
        ESP_LOGW(TAG, "Queue send failed");
    }

    void Radio::receiver_task(Radio *arg)
    {
      ESP_LOGE(TAG, "Hello from radio task!");
      int counter = 0;
      while (true)
        arg->receive_frame();
    }

    void Radio::add_frame_handler(std::function<void(Frame *)> &&callback)
    {
      this->handlers_.push_back(std::move(callback));
    }

  } // namespace wmbus
} // namespace esphome
