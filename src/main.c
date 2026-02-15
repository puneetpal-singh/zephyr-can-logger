#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/display.h>
#include <lvgl.h>
#include <stdio.h>

K_MSGQ_DEFINE(can_msgq, sizeof(struct can_frame), 64, 4);

static struct can_frame latest_frame;
static struct k_mutex latest_lock;
static bool latest_valid;
static volatile uint32_t rx_count;

static lv_obj_t *frame_label;
static lv_obj_t *stats_label;

#define CAN_RX_STACK 2048
#define UI_STACK 4096

K_THREAD_STACK_DEFINE(can_rx_stack, CAN_RX_STACK);
K_THREAD_STACK_DEFINE(ui_stack, UI_STACK);

static struct k_thread can_rx_thread;
static struct k_thread ui_thread;

static void format_can_frame(char *out, size_t out_size, const struct can_frame *frame)
{
    int offset = snprintf(out, out_size, "ID:0x%03X DLC:%d\nDATA:", frame->id, frame->dlc);

    for (int i = 0; i < frame->dlc && (size_t)offset < out_size; i++) {
        offset += snprintf(out + offset, out_size - (size_t)offset, " %02X", frame->data[i]);
    }
}

static void can_rx_worker(void *a, void *b, void *c)
{
    ARG_UNUSED(a);
    ARG_UNUSED(b);
    ARG_UNUSED(c);

    while (true) {
        struct can_frame frame;

        if (k_msgq_get(&can_msgq, &frame, K_FOREVER) == 0) {
            rx_count++;
            k_mutex_lock(&latest_lock, K_FOREVER);
            latest_frame = frame;
            latest_valid = true;
            k_mutex_unlock(&latest_lock);
            printk("RX id=0x%X dlc=%d\n", frame.id, frame.dlc);
        }
    }
}

static void ui_worker(void *a, void *b, void *c)
{
    ARG_UNUSED(a);
    ARG_UNUSED(b);
    ARG_UNUSED(c);

    const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (!device_is_ready(display)) {
        printk("ERROR: display not ready\n");
        return;
    }

    k_msleep(200);

    lv_obj_t *title_label = lv_label_create(lv_scr_act());
    lv_label_set_text(title_label, "Zephyr CAN Logger");
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 2);

    frame_label = lv_label_create(lv_scr_act());
    lv_label_set_text(frame_label, "Waiting for CAN frames...");
    lv_obj_align(frame_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_width(frame_label, 155);
    lv_label_set_long_mode(frame_label, LV_LABEL_LONG_WRAP);

    stats_label = lv_label_create(lv_scr_act());
    lv_label_set_text(stats_label, "RX:0 FPS:0");
    lv_obj_align(stats_label, LV_ALIGN_BOTTOM_MID, 0, 0);

    display_blanking_off(display);

    uint32_t last_ms = k_uptime_get_32();
    uint32_t last_rx = 0;

    while (true) {
        struct can_frame frame;
        bool valid = false;

        k_mutex_lock(&latest_lock, K_FOREVER);
        if (latest_valid) {
            frame = latest_frame;
            valid = true;
        }
        k_mutex_unlock(&latest_lock);

        if (valid) {
            char line[128];
            format_can_frame(line, sizeof(line), &frame);
            lv_label_set_text(frame_label, line);
        }

        uint32_t now = k_uptime_get_32();
        if (now - last_ms >= 1000U) {
            uint32_t fps = rx_count - last_rx;
            last_rx = rx_count;
            last_ms = now;

            char stats[64];
            snprintf(stats, sizeof(stats), "RX:%u FPS:%u", (unsigned)rx_count, (unsigned)fps);
            lv_label_set_text(stats_label, stats);
        }

        lv_timer_handler();
        k_msleep(100);
    }
}

int main(void)
{
    k_mutex_init(&latest_lock);

    const struct device *can_dev = DEVICE_DT_GET(DT_ALIAS(can0));
    if (!device_is_ready(can_dev)) {
        printk("ERROR: CAN device not ready\n");
        return 0;
    }

    int err = can_start(can_dev);
    if (err) {
        printk("ERROR: can_start failed (%d)\n", err);
        return 0;
    }

    struct can_filter filter = {.flags = 0, .id = 0, .mask = 0};
    int filter_id = can_add_rx_filter_msgq(can_dev, &can_msgq, &filter);
    if (filter_id < 0) {
        printk("ERROR: can_add_rx_filter_msgq failed (%d)\n", filter_id);
        return 0;
    }

    k_thread_create(&can_rx_thread, can_rx_stack, CAN_RX_STACK, can_rx_worker, NULL, NULL, NULL, 2, 0, K_NO_WAIT);
    k_thread_create(&ui_thread, ui_stack, UI_STACK, ui_worker, NULL, NULL, NULL, 7, 0, K_NO_WAIT);

    while (true) {
        k_sleep(K_SECONDS(5));
    }

    return 0;
}
