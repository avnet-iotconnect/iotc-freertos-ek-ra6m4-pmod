/**********************************************************************************************************************
 * File Name    : board_mon_thread_entry.c
 * Version      : .
 * Description  : Board status monitor thread.
 *********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright [2020] Renesas Electronics Corporation and/or its affiliates.  All Rights Reserved.
 *
 * This software and documentation are supplied by Renesas Electronics America Inc. and may only be used with products
 * of Renesas Electronics Corp. and its affiliates ("Renesas").  No other uses are authorized.  Renesas products are
 * sold pursuant to Renesas terms and conditions of sale.  Purchasers are solely responsible for the selection and use
 * of Renesas products and Renesas assumes no liability.  No license, express or implied, to any intellectual property
 * right is granted by Renesas. This software is protected under all applicable laws, including copyright laws. Renesas
 * reserves the right to change or discontinue this software and/or this documentation. THE SOFTWARE AND DOCUMENTATION
 * IS DELIVERED TO YOU "AS IS," AND RENESAS MAKES NO REPRESENTATIONS OR WARRANTIES, AND TO THE FULLEST EXTENT
 * PERMISSIBLE UNDER APPLICABLE LAW, DISCLAIMS ALL WARRANTIES, WHETHER EXPLICITLY OR IMPLICITLY, INCLUDING WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NONINFRINGEMENT, WITH RESPECT TO THE SOFTWARE OR
 * DOCUMENTATION.  RENESAS SHALL HAVE NO LIABILITY ARISING OUT OF ANY SECURITY VULNERABILITY OR BREACH.  TO THE MAXIMUM
 * EXTENT PERMITTED BY LAW, IN NO EVENT WILL RENESAS BE LIABLE TO YOU IN CONNECTION WITH THE SOFTWARE OR DOCUMENTATION
 * (OR ANY PERSON OR ENTITY CLAIMING RIGHTS DERIVED FROM YOU) FOR ANY LOSS, DAMAGES, OR CLAIMS WHATSOEVER, INCLUDING,
 * WITHOUT LIMITATION, ANY DIRECT, CONSEQUENTIAL, SPECIAL, INDIRECT, PUNITIVE, OR INCIDENTAL DAMAGES; ANY LOST PROFITS,
 * OTHER ECONOMIC DAMAGE, PROPERTY DAMAGE, OR PERSONAL INJURY; AND EVEN IF RENESAS HAS BEEN ADVISED OF THE POSSIBILITY
 * OF SUCH LOSS, DAMAGES, CLAIMS OR COSTS.
 **********************************************************************************************************************/

#include "board_mon_thread.h"
#include "common_init.h"
#include "common_data.h"
#include "common_utils.h"
#include "usb_console_main.h"
#include "board_cfg.h"
#include "iotc_demo.h"

#define BUTTON_DEBOUNCE_RATE (500)

extern bool b_usb_configured;

st_board_status_t g_board_status = {};
static const TickType_t xTicksToWait = 50 / portTICK_PERIOD_MS;

static uint16_t adc_data        = 0;
static uint16_t old_adc_data    = 0;

static char print_buffer [128] = {};

static const char * const sp_cursor_store     = "\x1b[s";
static const char * const sp_cursor_restore   = "\x1b[u";
static const char * const sp_cursor_temp      = "\x1b[8;41H\x1b[K";
static const char * const sp_cursor_frequency = "\x1b[9;41H\x1b[K";
static const char * const sp_cursor_intensity = "\x1b[10;41H\x1b[K";

static const char * const sp_green_fg   = "\x1b[32m";
static const char * const sp_white_fg   = "\x1b[37m";

static double mcu_temp_f       = 0.00;
static double mcu_temp_c       = 0.00;
static double tmp              = 0.00;
static uint16_t wn_mcu_temp_f  = 0;
static uint16_t fr_mcu_temp_f  = 0;
static uint16_t wn_mcu_temp_c  = 0;
static uint16_t fr_mcu_temp_c  = 0;
static uint16_t d              = 0;
static uint16_t r              = 0;

uint32_t pwm_dcs[3] = {LED_INTENSITY_10, LED_INTENSITY_50, LED_INTENSITY_90};
uint32_t pwm_rates[3] = {BLINK_FREQ_1HZ, BLINK_FREQ_5HZ, BLINK_FREQ_10HZ};

char g_pwm_dcs_data []   = {10, 50, 90};
char g_pwm_rates_data [] = {1, 5, 10};

EventBits_t uxBits;
uint16_t new_value      = 0;
uint32_t temperature_read_interval = 10;

extern adc_info_t  g_adc_info_rtn;

/**********************************************************************************************************************
 * Function Name: test_temperature_change
 * Description  : Read the Temperature and if it is different form the previous reading, trigger an update.
 * Argument     : None
 * Return Value : None
 *********************************************************************************************************************/
static void test_temperature_change()
{
    fsp_err_t fsp_err = FSP_SUCCESS;

    /* Read die temperature */
    fsp_err = R_ADC_Read (&g_adc_ctrl, ADC_CHANNEL_TEMPERATURE, &adc_data);

    /* Handle error */
    if (FSP_SUCCESS != fsp_err)
    {
        print_to_console ("** R_ADC_Read API failed ** \r\n");

        /* Fatal error */
        SYSTEM_ERROR
    }

    g_board_status.adc_temperature_data = adc_data;

    /* Read TSN cal data (value written at manufacture, does not change at runtime) */
    if (0xFFFFFFFF == g_adc_info_rtn.calibration_data)
    {
        /* Unable to read TSN cal value - not supported */
        fsp_err = FSP_ERR_UNSUPPORTED;
    }

    if (FSP_SUCCESS == fsp_err)
    {
        mcu_temp_c = (double)( (adc_data - (((double)g_adc_info_rtn.calibration_data) -
                TSN_CAL_OFFEST_COUNTS_AT_127DEG_TO_0DEG_C)) / TSN_ADC_COVERSION_SLOPE_COUNTS_PER_DEG_C);

        mcu_temp_f = (double)( (adc_data - (((double)g_adc_info_rtn.calibration_data) -
                TSN_CAL_OFFEST_COUNTS_AT_260_6DEG_TO_0DEG_F)) / TSN_ADC_COVERSION_SLOPE_COUNTS_PER_DEG_F);
    }
    else
    {
        mcu_temp_f       = 0.00;
        mcu_temp_c       = 0.00;
    }

    /* As sprintf does not support floating point convert result to d.r */
    tmp = mcu_temp_f * 100.0;
    d = (uint16_t)(mcu_temp_f / 1.00);
    r = (uint16_t)(tmp / 1.00);
    r = r % (uint16_t)(d * 100);

    g_board_status.temperature_f.whole_number = d;
    g_board_status.temperature_f.mantissa = r;

    tmp = mcu_temp_c * 100.0;
    d = (uint16_t)(mcu_temp_c / 1.00);
    r = (uint16_t)(tmp / 1.00);
    r = r % (uint16_t)(d * 100);

    g_board_status.temperature_c.whole_number = d;
    g_board_status.temperature_c.mantissa = r;

    if (old_adc_data != adc_data)
    {
        old_adc_data = adc_data;
        xEventGroupSetBits(g_update_console_event, STATUS_UPDATE_TEMP_INFO);
    }
}
/**********************************************************************************************************************
 End of function test_temperature_change
 *********************************************************************************************************************/
/**********************************************************************************************************************
 * Function Name: board_mon_thread_entry
 * Description  : .
 * Argument     : pvParameters
 * Return Value : .
 *********************************************************************************************************************/
void board_mon_thread_entry(void * pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

    while (1)
    {
        uxBits = xEventGroupWaitBits(g_update_console_event,  STATUS_UPDATE_KIS_INFO, pdFALSE, pdTRUE, 1);

        if (true == b_usb_configured)
        {
            if (0 == ( --temperature_read_interval))
            {
                temperature_read_interval = 10;

                /* Check for change in core temperature */
                /* Event will be set is a change is detected */
                test_temperature_change();
            }

            if ((uxBits & (STATUS_DISPLAY_MENU_KIS | STATUS_UPDATE_TEMP_INFO)) ==
                    (STATUS_DISPLAY_MENU_KIS | STATUS_UPDATE_TEMP_INFO))
            {
                wn_mcu_temp_f = g_board_status.temperature_f.whole_number;
                fr_mcu_temp_f = g_board_status.temperature_f.mantissa;
                wn_mcu_temp_c = g_board_status.temperature_c.whole_number;
                fr_mcu_temp_c = g_board_status.temperature_c.mantissa;

                /* Update temperature to display */
                char * temp_buffer = pvPortMalloc(128);
                sprintf(temp_buffer, "%s%s%s%d.%d/%d.%d%s\r\n%s",
                        sp_cursor_store, sp_cursor_temp, sp_green_fg,
                        wn_mcu_temp_f, fr_mcu_temp_f, wn_mcu_temp_c, fr_mcu_temp_c,
                        sp_green_fg, sp_cursor_restore );
                print_to_console(temp_buffer);
                vPortFree(temp_buffer);
            }

            if ((uxBits & (STATUS_DISPLAY_MENU_KIS | STATUS_UPDATE_INTENSE_INFO)) ==
                    (STATUS_DISPLAY_MENU_KIS | STATUS_UPDATE_INTENSE_INFO))
            {
                /* Update Switch SW1 */
                new_value = g_board_status.led_intensity;

                sprintf(print_buffer, "%s%s%s%d%s\r\n%s",
                        sp_cursor_store, sp_cursor_intensity, sp_green_fg,
                        g_pwm_dcs_data[new_value],
                        sp_white_fg, sp_cursor_restore );
                print_to_console(print_buffer);
            }

            if ((uxBits & (STATUS_DISPLAY_MENU_KIS | STATUS_UPDATE_FREQ_INFO)) ==
                    (STATUS_DISPLAY_MENU_KIS | STATUS_UPDATE_FREQ_INFO))
            {
                /* Update Switch SW2 */
                new_value = g_board_status.led_frequency;

                sprintf(print_buffer, "%s%s%s%d%s\r\n%s",
                        sp_cursor_store, sp_cursor_frequency, sp_green_fg,
                        g_pwm_rates_data[new_value],
                        sp_white_fg, sp_cursor_restore );
                print_to_console(print_buffer);

            }
        }

        if ((uxBits & (STATUS_UPDATE_TEMP_INFO)) == (STATUS_UPDATE_TEMP_INFO))
        {
            xEventGroupClearBits(g_update_console_event, STATUS_UPDATE_TEMP_INFO);
        }

        if ((uxBits & (STATUS_UPDATE_INTENSE_INFO)) == (STATUS_UPDATE_INTENSE_INFO))
        {
            led_duty_cycle_update();

            /* Clear Event */
            xEventGroupClearBits(g_update_console_event, (STATUS_UPDATE_INTENSE_INFO));
        }

        if ((uxBits & (STATUS_UPDATE_FREQ_INFO)) == (STATUS_UPDATE_FREQ_INFO))
        {
            R_GPT_PeriodSet(g_blinker.p_ctrl, pwm_rates[g_board_status.led_frequency]);

            /* Clear Event */
            xEventGroupClearBits(g_update_console_event, (STATUS_UPDATE_FREQ_INFO));
        }

        vTaskDelay(xTicksToWait);
    }
}
/**********************************************************************************************************************
 End of function board_mon_thread_entry
 *********************************************************************************************************************/


/**********************************************************************************************************************
 * Function Name: button_irq10_callback
 * Description  : SW1 Interrupt handler.
 * Argument     : p_args
 * Return Value : None
 *********************************************************************************************************************/
void button_irq10_callback(external_irq_callback_args_t *p_args)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t xResult = pdFAIL;
    EventBits_t uxBits;

    FSP_PARAMETER_NOT_USED(p_args);

    uxBits = xEventGroupGetBitsFromISR(g_update_console_event);

    if ((uxBits & (STATUS_UPDATE_INTENSE_INFO)) != (STATUS_UPDATE_INTENSE_INFO))
    {
        g_board_status.led_intensity = (uint16_t)((g_board_status.led_intensity + 1) % 3);
        xResult = xEventGroupSetBitsFromISR(g_update_console_event, STATUS_UPDATE_INTENSE_INFO,
                                            &xHigherPriorityTaskWoken);

        /* Was the message posted successfully? */
        if (pdFAIL != xResult)
        {
            /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
            switch should be requested.  The macro used is port specific and will
            be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
            the documentation page for the port being used. */
            portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
        }
    }
}
/**********************************************************************************************************************
 End of function button_irq10_callback
 *********************************************************************************************************************/

/* SW 2 */
/**********************************************************************************************************************
 * Function Name: button_irq11_callback
 * Description  : SW2 interrupt handler.
 * Argument     : p_args
 * Return Value : None
 *********************************************************************************************************************/
void button_irq11_callback(external_irq_callback_args_t *p_args)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t xResult = pdFAIL;
    EventBits_t uxBits;

    FSP_PARAMETER_NOT_USED(p_args);

    uxBits = xEventGroupGetBitsFromISR(g_update_console_event);

    if ( ( uxBits & ( STATUS_UPDATE_FREQ_INFO ) ) != ( STATUS_UPDATE_FREQ_INFO ) )
    {
        g_board_status.led_frequency = (uint16_t)((g_board_status.led_frequency + 1) % 3);
        xResult = xEventGroupSetBitsFromISR(g_update_console_event, STATUS_UPDATE_FREQ_INFO,
                                            &xHigherPriorityTaskWoken);

        /* Was the message posted successfully? */
        if ( pdFAIL != xResult)
        {
            /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
            switch should be requested.  The macro used is port specific and will
            be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
            the documentation page for the port being used. */
            portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
        }
    }
}
/**********************************************************************************************************************
 End of function button_irq11_callback
 *********************************************************************************************************************/


/**********************************************************************************************************************
 * Function Name: set_led_frequency
 * Description  : Sets LED frequency externally
 * Argument     : freq: Value from 0 to 3
 * Return Value : None
 *********************************************************************************************************************/

void set_led_frequency(uint16_t freq)
{
    if (freq > 3)
    {
        print_to_console("** Frequency index out of range! ** \r\n");
        return;
    }

    g_board_status.led_frequency = freq;
    xEventGroupSetBits(g_update_console_event, STATUS_UPDATE_FREQ_INFO);
}

/**********************************************************************************************************************
 End of function set_led_frequency
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Function Name: get_cpu_temperature
 * Description  : Gets the CPU Temperature externally
 * Argument     : None
 * Return Value : float, CPU temperature in degrees celsius
 *********************************************************************************************************************/

float get_cpu_temperature(void)
{
    return ((float) g_board_status.temperature_c.whole_number) + ((float) (g_board_status.temperature_c.mantissa) / 100.0f);
}
