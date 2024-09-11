#include <iotc_demo_thread.h>
#include "da16k_comm/da16k_comm.h"
#include "common_init.h"
#include "iotc_demo.h"
#include "usb_console_main.h"

/* Telemetry grabber & command handler code
 */

static inline bool string_starts_with(const char *full_string, const char *to_check) {
    return strncmp(to_check, full_string, strlen(to_check)) == 0;
}

static inline bool command_has_params(const da16k_cmd_t *cmd) {
    return cmd->parameters != NULL;
}

static void iotc_demo_handle_command(const da16k_cmd_t *cmd) {
    /* All commands we know need parameters. */

    if (!command_has_params(cmd)) {
        DA16K_PRINT("ERROR: Command '%s' needs a parameter!\r\n", cmd->command);
        return;
    }

    if        (string_starts_with(cmd->command, "set_led_frequency")) {
        set_led_frequency((uint16_t) atoi(cmd->parameters));

    } else if (string_starts_with(cmd->command, "set_red_led")) {

        if        (string_starts_with(cmd->parameters, "on")) {
            TURN_RED_ON
        } else if (string_starts_with(cmd->parameters, "off")) {
            TURN_RED_OFF
        } else {
            DA16K_PRINT("ERROR: unknown parameter for %s\r\n", cmd->command);
        }

    } else {
        DA16K_PRINT("ERROR: Unknown command received: %s\r\n", cmd->command);
        return;

    }
}

/* Custom IoTConnect configuration parameters - define DA16K_IOTC_CONFIG_USED to use them */
#if defined (DA16K_IOTC_CONFIG_USED)

#define IOTC_CONNECTION_TYPE    DA16K_IOTC_AWS
#define IOTC_CPID               "<INSERT CPID HERE>"
#define IOTC_DUID               "<INSERT DUID HERE>"
#define IOTC_ENV                "<INSERT ENV HERE>"


/*  Custom device cert/key - place pem files in ../cert/ to use them. 
    WARNING: Transmitting certificates via AT commands is INSECURE and the functionality is only provided for testing purposes! */
#include "../cert/iotc_demo_dev_cert.h"

#if defined (__DEVICE_CERT__)
#warning "WARNING: Transmitting certificates via AT commands is INSECURE and the functionality is only provided for testing purposes!"
static const char device_cert[] = __DEVICE_CERT__;
static const char device_key[] = __DEVICE_KEY__;
#else
static const char *device_cert = NULL;
static const char *device_key = NULL;
#endif

static da16k_iotc_cfg_t iotc_config = { IOTC_CONNECTION_TYPE, IOTC_CPID, IOTC_DUID, IOTC_ENV, 0, device_cert, device_key };
#define IOTC_CONFIG_PTR &iotc_config
#else
#define IOTC_CONFIG_PTR NULL
#endif

/* Custom WiFi configuration parameters - define DA16K_WIFI_CONFIG_USED to use them */
#if defined (DA16K_WIFI_CONFIG_USED)
#define IOTC_SSID               "<INSERT SSID HERE>"
#define IOTC_PASSPHRASE         "<INSERT PASSPHRASE HERE>"

static da16k_wifi_cfg_t wifi_config = { IOTC_SSID, IOTC_PASSPHRASE, false, 0 };
#define WIFI_CONFIG_PTR &wifi_config
#else
#define WIFI_CONFIG_PTR NULL
#endif

/* IoTCDemo entry function */
/* pvParameters contains TaskHandle_t */
void iotc_demo_thread_entry(void *pvParameters) {
    FSP_PARAMETER_NOT_USED (pvParameters);

    da16k_cfg_t da16k_config = { IOTC_CONFIG_PTR, WIFI_CONFIG_PTR, 0 };
    da16k_err_t err = da16k_init(&da16k_config);

    assert(err == DA16K_SUCCESS);

    while (1) {
        da16k_cmd_t current_cmd = {0};
        float cpuTemp = 0.0;

        err = da16k_get_cmd(&current_cmd);

        if (err == DA16K_SUCCESS) {
            iotc_demo_handle_command(&current_cmd);
            da16k_destroy_cmd(current_cmd);
        }

        if (err == DA16K_SUCCESS) {
            DA16K_PRINT("Command received: %s, parameters: %s\r\n", current_cmd.command, current_cmd.parameters ? current_cmd.parameters : "<none>" );
        }

        /* obtain sensor data */

        cpuTemp = get_cpu_temperature();

        err = da16k_send_msg_direct_num("cpu_temperature", cpuTemp);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
