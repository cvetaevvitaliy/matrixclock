#include <avr/interrupt.h>
#include <util/delay.h>

#include "matrix.h"
#include "fonts.h"
#include "mtimer.h"
#include "display.h"
#include "ds18x20.h"
#include "rtc.h"
#include "alarm.h"
#include "bmp180.h"
#include "dht22.h"
#include "eeprom.h"

void hwInit(void)
{
    _delay_ms(250);

    ds18x20SearchDevices();
    bmp180Init();
    dht22Init();

    displayInit();

    mTimerInit();

    matrixScrollAndADCInit();

    alarmInit();

    rtc.etm = RTC_NOEDIT;

    sei();

    return;
}

int main(void)
{
    uint8_t cmd;
    uint8_t dispMode = MODE_MAIN;

    static int8_t direction = PARAM_UP;

    hwInit();

    showTimeMasked();

    ds18x20Process();
    sensTimer = TEMP_MEASURE_TIME;

    while (1) {
        // Update sensors with SENSOR_POLL_INTERVAL period
        if (!sensTimer == 0) {
            sensTimer = SENSOR_POLL_INTERVAL;
            ds18x20Process();
            if (bmp180HaveSensor())
                bmp180Convert();
            dht22Read();
        }

        // Check alarm
        checkAlarm();

        // Update brightness only when not in brightness setup
        if (dispMode != MODE_BRIGHTNESS)
            calcBrightness();

        // Get command from buttons
        cmd = getBtnCmd();

        // Beep on button pressed
        if (cmd != BTN_STATE_0) {
            if (cmd < BTN_0_LONG)
                startBeeper(BEEP_SHORT);
            else
                startBeeper(BEEP_LONG);
        }

        // Stop scrolling on any button pressed
        if (cmd != BTN_STATE_0)
            matrixHwScroll(MATRIX_SCROLL_STOP);

        // Handle command
        switch (cmd) {
        case BTN_0:
            direction = PARAM_UP;
            switch (dispMode) {
            case MODE_EDIT_TIME:
                rtcNextEditParam();
                break;
            case MODE_EDIT_ALARM:
                alarmNextEditParam();
                break;
            }
            break;
        case BTN_1:
            direction = PARAM_UP;
        case BTN_2:
            if (cmd == BTN_2)
                direction = PARAM_DOWN;
            switch (dispMode) {
            case MODE_MAIN:
                startScroll(cmd - BTN_1);
                break;
            case MODE_EDIT_TIME:
                rtcChangeTime(direction);
                break;
            case MODE_EDIT_ALARM:
                alarmChange(direction);
                break;
            case MODE_BRIGHTNESS:
                changeBrightness(direction);
                break;
            case MODE_CORRECTION:
                changeCorrection(direction);
                break;
            case MODE_TEST:
                displayChangeRotate(direction);
                break;
            }
            break;
        case BTN_0_LONG:
            if (dispMode == MODE_MAIN) {
                dispMode = MODE_EDIT_TIME;
                rtcNextEditParam();
            } else {
                rtc.etm = RTC_NOEDIT;
                alarmSave();
                alarm.eam = ALARM_NOEDIT;
                dispMode = MODE_MAIN;
                showTimeMasked();
            }
            break;
        case BTN_1_LONG:
            if (dispMode == MODE_MAIN) {
                dispMode = MODE_EDIT_ALARM;
                alarmNextEditParam();
            }
            break;
        case BTN_2_LONG:
            if (dispMode == MODE_MAIN || dispMode == MODE_CORRECTION) {
                dispMode = MODE_BRIGHTNESS;
                showBrightness(direction, MASK_ALL);
            } else if (dispMode == MODE_BRIGHTNESS) {
                dispMode = MODE_CORRECTION;
                showCorrection(direction, MASK_ALL);
            }
            break;
        case BTN_0_LONG | BTN_1_LONG:
            displaySwitchHourZero();
            dispMode = MODE_MAIN;
            showTimeMasked();
            break;
        case BTN_1_LONG | BTN_2_LONG:
            displaySwitchBigNum();
            dispMode = MODE_MAIN;
            showTimeMasked();
            break;
        case BTN_0_LONG | BTN_2_LONG:
            displaySwitchHourSignal();
            dispMode = MODE_MAIN;
            showTimeMasked();
            break;
        case BTN_0_LONG | BTN_1_LONG | BTN_2_LONG:
            dispMode = MODE_TEST;
            break;
        }

        // Show things
        switch (dispMode) {
        case MODE_MAIN:
            showMainScreen();
            break;
        case MODE_EDIT_TIME:
            showTimeEdit(direction);
            break;
        case MODE_EDIT_ALARM:
            showAlarmEdit(direction);
            break;
        case MODE_BRIGHTNESS:
            showBrightness(direction, MASK_NONE);
            break;
        case MODE_CORRECTION:
            showCorrection(direction, MASK_NONE);
            break;
        case MODE_TEST:
            showTest();
            break;
        }
    }

    return 0;
}
