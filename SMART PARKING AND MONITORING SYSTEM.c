#include <Arduino.h>
#include <avr/io.h>
#include <stdio.h>
#include <string.h>

#include "gpio.h"
#include "timer.h"
#include "ultra.h"
#include "two_segment.h"
#include "pwm.h"
#include "keypad.h"
#include "lcd.h"

/* ========== SETTINGS ========== */
#define IR1_PIN 2
#define IR2_PIN 3
#define IR_DETECTED LOW
#define IR_CONFIRM_MS 50

#define DISTANCE_UPDATE_MS 100
#define MAX_DISPLAY_DISTANCE 99

#define LED_DIM 20
#define LED_BRIGHT 255
#define LED_STEP_MS 10

#define LCD_UPDATE_MS 300

/* ========== IR SENSOR VARIABLES ========== */
volatile uint8_t ir1_raw = 0;
volatile uint8_t ir2_raw = 0;

uint8_t ir1_state = 0;
uint8_t ir2_state = 0;
uint8_t ir1_candidate = 0;
uint8_t ir2_candidate = 0;

unsigned long ir1_change_time = 0;
unsigned long ir2_change_time = 0;

/* ========== SYSTEM VARIABLES ========== */
uint16_t distance = 0;

unsigned long last_distance_update = 0;
unsigned long last_serial = 0;
unsigned long last_led_update = 0;
unsigned long last_lcd_update = 0;

/* Green LED breathing */
unsigned long last_green_update = 0;
uint8_t green_brightness = LED_DIM;
int8_t green_direction = 1;

/* ========== KEYPAD SETTINGS ========== */
uint8_t red_alert_enabled = 1;
uint8_t buzzer_muted = 0;
uint8_t auto_led_mode = 1;
uint8_t red_test_mode = 0;

unsigned long red_test_started = 0;

/* IR1 red alert: three breathing cycles */
uint8_t ir1_previous_state = 0;
uint8_t ir1_alert_running = 0;
uint8_t ir1_alert_cycles = 0;
uint8_t red_alert_brightness = LED_DIM;
int8_t red_alert_direction = 1;

/* ========== IR INTERRUPTS ========== */
void ir1_ISR(void)
{
    ir1_raw = (digitalRead(IR1_PIN) == IR_DETECTED);
}

void ir2_ISR(void)
{
    ir2_raw = (digitalRead(IR2_PIN) == IR_DETECTED);
}

/* ========== STATUS LED PWM ========== */
/* Red D6, Yellow D7, Green D8 */

void status_led_pwm_init(void)
{
    DDRH |= (1 << PH3) |
            (1 << PH4) |
            (1 << PH5);

    TCCR4A = 0;
    TCCR4B = 0;
    TCCR4C = 0;
    TCNT4 = 0;

    TCCR4A = (1 << COM4A1) |
             (1 << COM4B1) |
             (1 << COM4C1) |
             (1 << WGM40);

    TCCR4B = (1 << WGM42) |
             (1 << CS41);

    OCR4A = 0;
    OCR4B = 0;
    OCR4C = 0;
}

void status_led_write(uint8_t red,
                      uint8_t yellow,
                      uint8_t green)
{
    OCR4A = red;
    OCR4B = yellow;
    OCR4C = green;
}

/* ========== IR SENSOR DEBOUNCE ========== */
void update_ir_states(void)
{
    uint8_t raw1;
    uint8_t raw2;

    noInterrupts();
    raw1 = ir1_raw;
    raw2 = ir2_raw;
    interrupts();

    unsigned long now = millis();

    if (raw1 != ir1_candidate)
    {
        ir1_candidate = raw1;
        ir1_change_time = now;
    }

    if (raw2 != ir2_candidate)
    {
        ir2_candidate = raw2;
        ir2_change_time = now;
    }

    if (now - ir1_change_time >= IR_CONFIRM_MS)
        ir1_state = ir1_candidate;

    if (now - ir2_change_time >= IR_CONFIRM_MS)
        ir2_state = ir2_candidate;
}

/* ========== RED/YELLOW LED BREATHING ========== */
void update_status_led(uint8_t slot_status)
{
    static uint8_t brightness = LED_DIM;
    static int8_t direction = 1;

    unsigned long now = millis();

    if (now - last_led_update < LED_STEP_MS)
        return;

    last_led_update = now;

    int16_t next = (int16_t)brightness +
                   (direction * 5);

    if (next >= LED_BRIGHT)
    {
        brightness = LED_BRIGHT;
        direction = -1;
    }
    else if (next <= LED_DIM)
    {
        brightness = LED_DIM;
        direction = 1;
    }
    else
    {
        brightness = (uint8_t)next;
    }

    if (slot_status == 0)
        status_led_write(brightness, 0, 0);
    else if (slot_status == 1)
        status_led_write(0, brightness, 0);
    else
        status_led_write(0, 0, brightness);
}

/* ========== GREEN LED BREATHING ========== */
void update_green_breathing(void)
{
    unsigned long now = millis();

    if (now - last_green_update < LED_STEP_MS)
        return;

    last_green_update = now;

    int16_t next = (int16_t)green_brightness +
                   (green_direction * 5);

    if (next >= LED_BRIGHT)
    {
        green_brightness = LED_BRIGHT;
        green_direction = -1;
    }
    else if (next <= LED_DIM)
    {
        green_brightness = LED_DIM;
        green_direction = 1;
    }
    else
    {
        green_brightness = (uint8_t)next;
    }

    OCR4C = green_brightness;
}

/* ========== IR1 RED ALERT: THREE CYCLES ========== */
void update_ir1_red_alert(void)
{
    if (!ir1_alert_running)
    {
        OCR4A = 0;
        return;
    }

    unsigned long now = millis();

    if (now - last_led_update < LED_STEP_MS)
        return;

    last_led_update = now;

    int16_t next = (int16_t)red_alert_brightness +
                   (red_alert_direction * 5);

    if (next >= LED_BRIGHT)
    {
        red_alert_brightness = LED_BRIGHT;
        red_alert_direction = -1;
    }
    else if (next <= LED_DIM)
    {
        red_alert_brightness = LED_DIM;
        ir1_alert_cycles++;

        if (ir1_alert_cycles >= 3)
        {
            ir1_alert_running = 0;
            OCR4A = 0;
            return;
        }

        red_alert_direction = 1;
    }
    else
    {
        red_alert_brightness = (uint8_t)next;
    }

    OCR4A = red_alert_brightness;
}

/* ========== DISTANCE-BASED SIREN ========== */
struct MusicNote
{
    uint16_t frequency;
    uint16_t duration;
};

const MusicNote parking_bgm[] =
{
    {900, 150},
    {1400, 150},
    {900, 150},
    {1400, 150},
    {900, 150},
    {1400, 150},
    {900, 150},
    {1400, 150},
    {0, 50}
};

#define BGM_NOTE_COUNT \
    (sizeof(parking_bgm) / sizeof(parking_bgm[0]))

uint8_t music_index = 0;
unsigned long music_note_started = 0;
uint8_t music_playing = 0;

uint16_t siren_tone_duration = 150;
uint16_t siren_gap_duration = 50;

void bgm_start(void)
{
    music_index = 0;
    music_note_started = millis();
    music_playing = 1;
}

void bgm_stop(void)
{
    pwm_off();
    music_playing = 0;
    music_index = 0;
}

void bgm_update(void)
{
    if (!music_playing)
        return;

    unsigned long now = millis();
    uint16_t duration;

    if (parking_bgm[music_index].frequency == 0)
        duration = siren_gap_duration;
    else
        duration = siren_tone_duration;

    if (now - music_note_started >= duration)
    {
        music_index++;

        if (music_index >= BGM_NOTE_COUNT)
            music_index = 0;

        music_note_started = now;
    }

    uint16_t frequency =
        parking_bgm[music_index].frequency;

    if (frequency == 0)
    {
        pwm_off();
    }
    else
    {
        pwm_set_frequency(frequency);
        pwm_set_duty(50);
        pwm_on();
    }
}

/* ========== KEYPAD CONTROL ========== */
/*
   1 = Enable red alert
   2 = Disable red alert
   3 = Mute buzzer
   B = Unmute buzzer
   7 = Automatic LED mode
   C = Toggle automatic/manual LED mode
   8 = Start red LED test
   9 or # = Stop red LED test
   * = Reset settings
   0 = Print status
   D = Restore default LED mode
*/

void update_keypad(void)
{
    char key = keypad_getkey();

    if (key == '\0')
        return;

    switch (key)
    {
        case '1':
            red_alert_enabled = 1;
            Serial.println("Red alert enabled");
            break;

        case '2':
            red_alert_enabled = 0;
            red_test_mode = 0;
            OCR4A = 0;
            Serial.println("Red alert disabled");
            break;

        case '3':
            buzzer_muted = 1;
            bgm_stop();
            pwm_off();
            Serial.println("Buzzer muted");
            break;

        case 'B':
            buzzer_muted = 0;
            Serial.println("Buzzer unmuted");
            break;

        case '7':
            auto_led_mode = 1;
            red_test_mode = 0;
            Serial.println("Automatic LED mode");
            break;

        case 'C':
            auto_led_mode = !auto_led_mode;
            red_test_mode = 0;

            Serial.println(
                auto_led_mode
                    ? "Automatic LED mode"
                    : "Manual LED mode"
            );
            break;

        case '8':
            red_test_mode = 1;
            red_test_started = millis();
            Serial.println("Red LED test started");
            break;

        case '9':
        case '#':
            red_test_mode = 0;
            OCR4A = 0;
            Serial.println("Red LED test stopped");
            break;

        case '*':
            red_alert_enabled = 1;
            buzzer_muted = 0;
            auto_led_mode = 1;
            red_test_mode = 0;
            OCR4A = 0;
            Serial.println("Settings reset");
            break;

        case '0':
            Serial.print("Distance: ");
            Serial.print(distance);
            Serial.print(" cm | IR1: ");
            Serial.print(ir1_state ? "DETECTED" : "CLEAR");
            Serial.print(" | IR2: ");
            Serial.print(ir2_state ? "DETECTED" : "CLEAR");
            Serial.print(" | Red alert: ");
            Serial.print(red_alert_enabled ? "ON" : "OFF");
            Serial.print(" | Buzzer: ");
            Serial.println(buzzer_muted ? "MUTED" : "ACTIVE");
            break;

        case 'D':
            auto_led_mode = 1;
            red_test_mode = 0;
            Serial.println("Default LED mode restored");
            break;

        default:
            break;
    }
}

/* ========== LCD STATUS DISPLAY ========== */
void update_lcd(void)
{
    char line1[17];
    char padded_line1[17];
    char line2[17];
    const char *message;

    snprintf(line1, sizeof(line1), "IR1:%s IR2:%s",
             ir1_state ? "DET" : "CLR",
             ir2_state ? "DET" : "CLR");

    snprintf(padded_line1, sizeof(padded_line1),
             "%-16s", line1);

    if (ir1_state)
        message = "IR1 OBJECT ALERT";
    else if (distance == 0)
        message = "NO ECHO";
    else if (distance <= 10)
        message = "STOP! TOO CLOSE";
    else if (distance <= 15)
        message = "VERY CLOSE!";
    else if (distance <= 30)
        message = "CAUTION: SLOW";
    else if (distance < 50)
        message = "SLOW DOWN";
    else
        message = "SAFE DISTANCE";

    snprintf(line2, sizeof(line2), "%-16s", message);

    lcd_set_cursor(0, 0);
    lcd_print(padded_line1);

    lcd_set_cursor(1, 0);
    lcd_print(line2);
}

/* ========== SETUP ========== */
void setup(void)
{
    Serial.begin(9600);

    pinMode(IR1_PIN, INPUT);
    pinMode(IR2_PIN, INPUT);

    attachInterrupt(
        digitalPinToInterrupt(IR1_PIN),
        ir1_ISR,
        CHANGE
    );

    attachInterrupt(
        digitalPinToInterrupt(IR2_PIN),
        ir2_ISR,
        CHANGE
    );

    ir1_raw = (digitalRead(IR1_PIN) == IR_DETECTED);
    ir2_raw = (digitalRead(IR2_PIN) == IR_DETECTED);

    ir1_candidate = ir1_state = ir1_raw;
    ir2_candidate = ir2_state = ir2_raw;

    ir1_change_time = millis();
    ir2_change_time = millis();

    ultra_init();
    two_segment_init(PORT_A, PORT_C, 0, 1);
    keypad_init();

    pwm_init();
    pwm_off();

    status_led_pwm_init();

    /* Initialize LCD */
    lcd_init();
    lcd_clear();

    lcd_set_cursor(0, 0);
    lcd_print("PARKING SYSTEM");

    lcd_set_cursor(1, 0);
    lcd_print("INITIALIZING");

    two_segment_display(100);

    Serial.println("Parking system started");
    Serial.println("Distance display: 1-99 cm; no echo or >99 cm displays 99");
    Serial.println("IR1 detection: display 00 and red LED breathes three times");
    Serial.println("1-10 cm: continuous emergency tone");
    Serial.println("11-15 cm: very fast siren");
    Serial.println("16-30 cm: fast siren");
    Serial.println("31-49 cm: warning siren");

    Serial.println("KEYPAD CONTROLS:");
    Serial.println("1=Red ON, 2=Red OFF");
    Serial.println("3=Mute buzzer, B=Unmute buzzer");
    Serial.println("7=Auto LED, C=Toggle LED mode");
    Serial.println("8=LED test, 9/#=Stop test");
    Serial.println("*=Reset, 0=Status, D=Default");
}

/* ========== MAIN LOOP ========== */
void loop(void)
{
    update_keypad();
    update_ir_states();

    uint8_t ir1 = ir1_state;
    uint8_t ir2 = ir2_state;

    /* Start three red LED breathing cycles on new IR1 detection */
    if (ir1 && !ir1_previous_state &&
        red_alert_enabled && auto_led_mode)
    {
        ir1_alert_running = 1;
        ir1_alert_cycles = 0;
        red_alert_brightness = LED_DIM;
        red_alert_direction = 1;
    }

    /* Reset alert when IR1 becomes clear */
    if (!ir1)
    {
        ir1_alert_running = 0;
        ir1_alert_cycles = 0;
        red_alert_brightness = LED_DIM;
        red_alert_direction = 1;
    }

    ir1_previous_state = ir1;

    uint8_t slot_unavailable = (ir1 && ir2);
    uint8_t slot_status;

    if (slot_unavailable)
        slot_status = 0;
    else if (ir1 || ir2)
        slot_status = 1;
    else
        slot_status = 2;

    unsigned long now = millis();

    /* ========== ULTRASONIC DISTANCE ========== */
    if (ir1)
    {
        distance = 0;
        two_segment_display(0);
    }
    else if (now - last_distance_update >= DISTANCE_UPDATE_MS)
    {
        last_distance_update = now;

        uint16_t measured = ultra_getDistance();

        if (measured == 0)
        {
            distance = 0;
            two_segment_display(99);
        }
        else
        {
            if (measured > MAX_DISPLAY_DISTANCE)
                distance = MAX_DISPLAY_DISTANCE;
            else
                distance = measured;

            two_segment_display((uint8_t)distance);
        }
    }

    /* ========== LCD REFRESH ========== */
    if (now - last_lcd_update >= LCD_UPDATE_MS)
    {
        last_lcd_update = now;
        update_lcd();
    }

    /* ========== DISTANCE-BASED BUZZER ========== */
    if (buzzer_muted)
    {
        if (music_playing)
            bgm_stop();
        else
            pwm_off();
    }
    else if (!ir1 && !slot_unavailable &&
             distance >= 1 && distance <= 10)
    {
        if (music_playing)
            bgm_stop();

        pwm_set_frequency(2500);
        pwm_set_duty(50);
        pwm_on();
    }
    else if (!ir1 && !slot_unavailable &&
             distance >= 11 && distance <= 15)
    {
        siren_tone_duration = 60;
        siren_gap_duration = 20;

        if (!music_playing)
            bgm_start();

        bgm_update();
    }
    else if (!ir1 && !slot_unavailable &&
             distance >= 16 && distance <= 30)
    {
        siren_tone_duration = 100;
        siren_gap_duration = 30;

        if (!music_playing)
            bgm_start();

        bgm_update();
    }
    else if (!ir1 && !slot_unavailable &&
             distance >= 31 && distance < 50)
    {
        siren_tone_duration = 150;
        siren_gap_duration = 50;

        if (!music_playing)
            bgm_start();

        bgm_update();
    }
    else
    {
        if (music_playing)
            bgm_stop();
        else
            pwm_off();
    }

    /* ========== LED CONTROL ========== */
    if (red_test_mode)
    {
        update_status_led(0);
        OCR4B = 0;
    }
    else if (ir1 && red_alert_enabled && auto_led_mode)
    {
        update_ir1_red_alert();
        OCR4B = 0;
    }
    else
    {
        OCR4A = 0;

        if (slot_status == 1)
            update_status_led(1);
        else
            OCR4B = 0;
    }

    /* Green LED remains OFF while IR1 detects an object */
    if (ir1 || OCR4A > 0)
    {
        OCR4C = 0;
    }
    else
    {
        update_green_breathing();
    }

    /* Refresh multiplexed seven-segment display */
    two_segment_refresh();

    /* ========== SERIAL MONITOR ========== */
    if (now - last_serial >= 500)
    {
        last_serial = now;

        Serial.print("IR1: ");
        Serial.print(ir1 ? "DETECTED" : "CLEAR");

        Serial.print(" | IR2: ");
        Serial.print(ir2 ? "DETECTED" : "CLEAR");

        Serial.print(" | Slots available: ");

        if (slot_status == 0)
            Serial.println("0 - RED");
        else if (slot_status == 1)
            Serial.println("1 - YELLOW");
        else
            Serial.println("2 - GREEN");

        if (ir1)
        {
            Serial.println(
                "Display: 00 | Red LED: three PWM breathing cycles | Green LED: OFF"
            );
        }
        else
        {
            Serial.print("Distance: ");

            if (distance == 0)
            {
                Serial.println("No echo (display shows 99)");
            }
            else
            {
                Serial.print(distance);
                Serial.println(" cm");
            }
        }
    }
}