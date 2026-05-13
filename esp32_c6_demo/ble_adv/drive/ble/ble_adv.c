#include "ble_adv.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#if CONFIG_BT_ENABLED
#include "nvs_flash.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#endif

/*
 * BLE 广播流程（新手版）
 *
 * app_main()
 *   -> ble_adv_start_task()
 *      -> 创建 ble_adv_task 任务
 *         -> 初始化 NVS
 *         -> 初始化 NimBLE Host
 *         -> 初始化 GAP/GATT 基础服务
 *         -> 设置设备名为 "Kiana"
 *         -> 注册 ble_on_sync 回调
 *         -> 启动 NimBLE 主循环任务 nimble_port_freertos_init()
 *            -> 触发 ble_on_sync()
 *               -> 获取本机地址类型
 *               -> ble_start_advertising() 开始广播
 *
 * 你可以把它理解成两层：
 * 1) 初始化层：把 BLE 运行环境搭好。
 * 2) 广播层：把“我是 Kiana”通过广播包发出去。
 */
static const char *TAG = "ble_adv";

#if CONFIG_BT_ENABLED
// 本变量保存当前设备使用的本地地址类型（public / random）。
// NimBLE 在开始广播前需要这个类型来配置 GAP 广播。
static uint8_t s_ble_addr_type;

typedef enum {
    BLE_STATE_READY = 0,
    BLE_STATE_INITIATING,
    BLE_STATE_ADVERTISING,
    BLE_STATE_SCANNING,
    BLE_STATE_CONNECTED,
} ble_state_t;

static volatile ble_state_t s_ble_state = BLE_STATE_READY;

static const char *ble_state_to_str(ble_state_t state)
{
    switch (state)
    {
    case BLE_STATE_READY:
        return "就绪态";
    case BLE_STATE_INITIATING:
        return "发起态";
    case BLE_STATE_ADVERTISING:
        return "广播态";
    case BLE_STATE_SCANNING:
        return "扫描态";
    case BLE_STATE_CONNECTED:
        return "连接态";
    default:
        return "未知状态";
    }
}

static void ble_start_advertising(void);

static int ble_gap_event_cb(struct ble_gap_event *event, void *arg)
{
    (void)arg;

    switch (event->type)
    {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0)
        {
            s_ble_state = BLE_STATE_CONNECTED;
            ESP_LOGI(TAG, "BLE GAP event: connected, state=%s", ble_state_to_str(s_ble_state));
        }
        else
        {
            s_ble_state = BLE_STATE_READY;
            ESP_LOGW(TAG, "BLE GAP event: connect failed(status=%d), fallback to %s",
                     event->connect.status, ble_state_to_str(s_ble_state));
            ble_start_advertising();
        }
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        s_ble_state = BLE_STATE_READY;
        ESP_LOGI(TAG, "BLE GAP event: disconnected(reason=%d), state=%s",
                 event->disconnect.reason, ble_state_to_str(s_ble_state));
        ble_start_advertising();
        return 0;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        s_ble_state = BLE_STATE_READY;
        ESP_LOGI(TAG, "BLE GAP event: adv complete(reason=%d), state=%s",
                 event->adv_complete.reason, ble_state_to_str(s_ble_state));
        ble_start_advertising();
        return 0;

    default:
        return 0;
    }
}

static void ble_status_task(void *pvParameters)
{
    (void)pvParameters;

    while (1)
    {
        ESP_LOGI(TAG, "BLE state machine: %s, addr_type=%u",
                 ble_state_to_str((ble_state_t)s_ble_state),
                 (unsigned int)s_ble_addr_type);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void ble_start_advertising(void)
{
    // GAP 广播参数：控制是否可连接、是否通用可发现等行为。
    struct ble_gap_adv_params adv_params = {0};
    // 广播负载字段：真正会被手机扫描到的内容（例如设备名、标志位）。
    struct ble_hs_adv_fields fields = {0};

    // BLE 广播常见标志位：
    // - BLE_HS_ADV_F_DISC_GEN: 通用可发现模式（手机更容易扫描到）
    // - BLE_HS_ADV_F_BREDR_UNSUP: 仅 BLE，不支持经典蓝牙 BR/EDR
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    // 设备名放进广播包，手机扫描列表中就会显示 Kiana。
    fields.name = (const uint8_t *)"Kiana";
    fields.name_len = 5;
    fields.name_is_complete = 1;

    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0)
    {
        ESP_LOGE(TAG, "ble_gap_adv_set_fields failed, rc=%d", rc);
        return;
    }

    // 连接模式和发现模式都使用通用配置：
    // - BLE_GAP_CONN_MODE_UND: 允许连接（undirected connectable advertising）
    // - BLE_GAP_DISC_MODE_GEN: 通用可发现
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    // BLE_HS_FOREVER 表示持续广播，直到你主动停止或系统重启。
    rc = ble_gap_adv_start(s_ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event_cb, NULL);
    if (rc != 0)
    {
        ESP_LOGE(TAG, "ble_gap_adv_start failed, rc=%d", rc);
        s_ble_state = BLE_STATE_READY;
        return;
    }

    s_ble_state = BLE_STATE_ADVERTISING;
    ESP_LOGI(TAG, "BLE advertising started. Device name: Kiana, state=%s", ble_state_to_str((ble_state_t)s_ble_state));
}

static void ble_on_sync(void)
{
    // sync 回调表示：NimBLE Host 已经与 Controller 完成同步，
    // 这时才能安全地推断地址类型并真正启动广播。
    int rc = ble_hs_id_infer_auto(0, &s_ble_addr_type);
    if (rc != 0)
    {
        ESP_LOGE(TAG, "ble_hs_id_infer_auto failed, rc=%d", rc);
        return;
    }

    ble_start_advertising();
}

static void ble_host_task(void *param)
{
    (void)param;
    // NimBLE 协议栈主循环：处理 BLE 事件（连接、广播、GATT 等）。
    // 该函数通常会一直运行，直到协议栈停止。
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static void ble_adv_task(void *pvParameters)
{
    (void)pvParameters;

    // NVS 用于保存蓝牙/PHY等运行参数，是 BLE 启动的基础依赖。
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // 当 NVS 分区版本不匹配或空间异常时，擦除后重建。
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 初始化 NimBLE Host 侧栈。
    ESP_ERROR_CHECK(nimble_port_init());
    // 初始化 GAP/GATT 基础服务。
    // GAP 负责连接、广播等“链路管理”；GATT 负责“数据服务模型”。
    ble_svc_gap_init();
    ble_svc_gatt_init();

    // 设置 GAP 设备名。很多扫描工具会读取并显示这个名称。
    int rc = ble_svc_gap_device_name_set("Kiana");
    if (rc != 0)
    {
        ESP_LOGE(TAG, "ble_svc_gap_device_name_set failed, rc=%d", rc);
    }

    // 注册同步回调，等 Host 与 Controller 完成同步后再启动广播。
    ble_hs_cfg.sync_cb = ble_on_sync;
    // 启动 NimBLE FreeRTOS 任务，内部会执行 ble_host_task。
    nimble_port_freertos_init(ble_host_task);

    // 状态任务单独打印 BLE 运行状态，方便你在串口里观察系统是否正常。
    s_ble_state = BLE_STATE_READY;
    BaseType_t status_task_created = xTaskCreate(ble_status_task, "ble_status_task", 3072, NULL, 4, NULL);
    if (status_task_created != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create ble_status_task");
    }

    // 当前初始化任务职责完成，主动删除自身以节省资源。
    vTaskDelete(NULL);
}
#endif

esp_err_t ble_adv_start_task(void)
{
#if CONFIG_BT_ENABLED
    // 对外提供的入口：由 app_main 调用，启动 BLE 初始化任务。
    BaseType_t created = xTaskCreate(ble_adv_task, "ble_adv_task", 6144, NULL, 5, NULL);
    if (created != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create ble_adv_task");
        return ESP_FAIL;
    }
    return ESP_OK;
#else
    // 若未开启蓝牙配置，返回不支持并打印提示，避免静默失败。
    ESP_LOGW(TAG, "Bluetooth is disabled in sdkconfig. Enable CONFIG_BT_ENABLED to advertise as Kiana.");
    return ESP_ERR_NOT_SUPPORTED;
#endif
}
