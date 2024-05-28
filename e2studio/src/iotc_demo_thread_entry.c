#include <iotc_demo_thread.h>
#include "da16k_comm/da16k_comm.h"
#include "common_init.h"
#include "iotc_demo.h"
#include "usb_console_main.h"

/* Telemetry grabber & command handler code
 */

static inline bool stringStartsWith(const char *fullString, const char *toCheck) {
    return strncmp(toCheck, fullString, strlen(toCheck)) == 0;
}

static inline bool commandHasParams(const da16k_cmd_t *cmd) {
    return cmd->parameters != NULL;
}

static void iotcDemoHandleCommand(const da16k_cmd_t *cmd) {
    /* All commands we know need parameters. */

    if (!commandHasParams(cmd)) {
        DA16K_PRINT("ERROR: Command '%s' needs a parameter!\r\n");
        return;
    }

    if        (stringStartsWith(cmd->command, "set_led_frequency")) {
        set_led_frequency((uint16_t) atoi(cmd->parameters));

    } else if (stringStartsWith(cmd->command, "set_red_led")) {

        if        (stringStartsWith(cmd->parameters, "on")) {
            TURN_RED_ON
        } else if (stringStartsWith(cmd->parameters, "off")) {
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

    da16k_cfg_t da16kConfig;
    da16k_err_t err = da16k_init(&da16kConfig);

    assert(err == DA16K_SUCCESS);

    while (1) {
        da16k_cmd_t currentCmd = {0};
        float cpuTemp = 0.0;

        err = da16k_getCmd(&currentCmd);

        if (currentCmd.command) {
            iotcDemoHandleCommand(&currentCmd);
        }

        if (err == DA16K_SUCCESS) {
            DA16K_PRINT("Command received: %s, parameters: %s\r\n", currentCmd.command, currentCmd.parameters ? currentCmd.parameters : "<none>" );
        }

        /* obtain sensor data */

        cpuTemp = get_cpu_temperature();

        da16k_send_float("cpu_temperature", cpuTemp);


        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
