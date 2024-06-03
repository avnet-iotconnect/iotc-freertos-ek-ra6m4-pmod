#include <iotc_demo_thread.h>
#include "da16k_comm/da16k_comm.h"
#include "common_init.h"
#include "iotc_demo.h"
#include "usb_console_main.h"

/* Telemetry grabber & command handler code
 */

static inline bool string_starts_with(const char *fullString, const char *toCheck) {
    return strncmp(toCheck, fullString, strlen(toCheck)) == 0;
}

static inline bool command_has_params(const da16k_cmd_t *cmd) {
    return cmd->parameters != NULL;
}

static void iotc_demo_handle_command(const da16k_cmd_t *cmd) {
    /* All commands we know need parameters. */

    if (!command_has_params(cmd)) {
        DA16K_PRINT("ERROR: Command '%s' needs a parameter!\r\n");
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


/* IoTCDemo entry function */
/* pvParameters contains TaskHandle_t */

void iotc_demo_thread_entry(void *pvParameters) {
    FSP_PARAMETER_NOT_USED (pvParameters);

    da16k_cfg_t da16k_config;
    da16k_err_t err = da16k_init(&da16k_config);

    assert(err == DA16K_SUCCESS);

    while (1) {
        da16k_cmd_t current_cmd = {0};
        float cpuTemp = 0.0;

        err = da16k_get_cmd(&current_cmd);

        if (current_cmd.command) {
            iotc_demo_handle_command(&current_cmd);
            da16k_destroy_cmd(current_cmd);
        }

        if (err == DA16K_SUCCESS) {
            DA16K_PRINT("Command received: %s, parameters: %s\r\n", current_cmd.command, current_cmd.parameters ? current_cmd.parameters : "<none>" );
        }

        /* obtain sensor data */

        cpuTemp = get_cpu_temperature();

        da16k_send_float("cpu_temperature", cpuTemp);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
