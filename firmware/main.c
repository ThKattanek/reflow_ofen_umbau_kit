/* Name: main.c
 * Projekt: reflow_ofen_steuerung
 * Author: Thorsten Kattanek
 * Geändert am: 27.09.2020
 * Copyright: Thorsten Kattanek
 * License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)
 *
 */

#define F_CPU   4000000UL

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>   /* benötigt von usbdrv.h */

#include <stdio.h>

#include "./config.h"
#include "./version.h"
#include "./lcd.h"
#include "./kty84-130_teperature_table.h"

#include "./gui_constans.h"
#include "./menu.h"
#include "./settings.h"

void show_start_message(void);
void init_adc(void);
uint16_t get_adc(void);
void init_pwm(void);
void set_fan_speed(uint8_t speed);
void init_keys(void);
void init_timer2(void);
uint8_t get_key_from_buffer();
void update_gui(void);
void check_menu_events(uint16_t menu_event);
void audio_signal_final();
void audio_beep();

volatile uint8_t adc_counter = 0;
volatile uint16_t current_adc_value;
uint16_t old_adc_value;
int16_t current_temperature = 0;

// Optimal für 85°
uint16_t tempern_time = 240;                    // Tempern Zeit in minuten
uint8_t tempern_temperatur_max_start = 70;      // Tempern nach Start hier Heizung abschalten, muss dann aber min. bis tempern_temperatur kommen
uint8_t pre_heating;
uint8_t tempern_temperatur = 85;                // Tempern Max Temperatur

volatile uint16_t counter_soldering;
volatile uint32_t counter_tempern;
volatile uint32_t old_counter_tempern;
volatile uint32_t counter_tempern_max;

// gui

enum {MODE_SOLDERING, MODE_TEMPERN, MODE_MENU};
uint8_t mode = MODE_MENU;

#define PHASE_1A	(PINC & (1<<IMPULS_1A_PIN))
#define PHASE_1B	(PINC & (1<<IMPULS_1B_PIN))
const unsigned char drehimp_tab[16]PROGMEM = {0,0,2,0,0,0,0,0,1,0,0,0,0,0,0,0};

volatile uint8_t key_buffer[16];
volatile uint8_t key_buffer_r_pos;
volatile uint8_t key_buffer_w_pos;

// MENU
enum  MENU_IDS{M_START_SOLDERING, M_START_TEMPERN, M_HEATER_TOP, M_HEATER_BOTTOM, M_SETTINGS, \
              M_BACK_SETTINGS, M_TEST1, M_TEST2};

/*
enum  MENU_IDS{M_BACK, M_IMAGE, M_SETTINGS, M_INFO, \
               M_BACK_IMAGE, M_INSERT_IMAGE, M_REMOVE_IMAGE, M_WP_IMAGE, M_NEW_IMAGE, M_SAVE_IMAGE, \
               M_BACK_SETTINGS, M_PIN_PB2, M_PIN_PB3, M_SAVE_EEPROM, M_RESTART, \
               M_BACK_INFO, M_VERSION_INFO, M_SDCARD_INFO};
               */

MENU_STRUCT main_menu;
MENU_STRUCT settings_menu;

int main(void)
{
    char string0[100];

    // LCD initiallisieren

    lcd_init();

    init_keys();

    // Key Puffer leeren
    key_buffer_r_pos = 0;
    key_buffer_w_pos = 0;
    
    status_led_init();

    // Buzzer initialisieren
    buzzer_init();
    motor_power_init();
    init_pwm();

    // Startmeldung ausgeben

    show_start_message();

    uint8_t counter1 = 8;

    while (counter1 > 0) {
        buzzer_on();
         status_led_on();
        _delay_ms(50);
        buzzer_off();
         status_led_off();
        _delay_ms(50);
        counter1--;
    }

    // Fan Selfcheck
    motor_power_on();
    uint8_t fan_speed = 0;

    while(fan_speed < 255)
    {
        set_fan_speed(fan_speed);
        fan_speed++;
        _delay_ms(10);
    }

    while(fan_speed > 0)
    {
        set_fan_speed(fan_speed);
        fan_speed--;
        _delay_ms(10);
    }
    motor_power_off();

    // Heizung initialisieren
    heating_bottom_init();
    heating_top_init();

    // Init ADC
    init_adc();

    set_fan_speed(255);

    // Timer2 initialisieren und Interrupts freigeben
    init_timer2();
    sei();

    /// Menüs einrichten
    /// Hauptmenü
    MENU_ENTRY main_menu_entrys[] = {{"Starte Loetvorgang",M_START_SOLDERING,ENTRY_NORMAL,0,0},{"Starte Tempern",M_START_TEMPERN,ENTRY_NORMAL,0,0},{"Heizung oben :",M_HEATER_TOP,ENTRY_ONOFF,0,0},{"Heizung unten:",M_HEATER_BOTTOM,ENTRY_ONOFF,0,0},{"Settings",M_SETTINGS,ENTRY_MENU,0,&settings_menu}};
    MENU_ENTRY settings_menu_entrys[] = {{"TEST1",M_START_SOLDERING,ENTRY_NORMAL,0,0}, {"TEST2",M_START_SOLDERING,ENTRY_NORMAL,0,0}};

    main_menu.lcd_cursor_char = 2;  // 126 Standard Pfeil
    menu_init(&main_menu, main_menu_entrys, 5,LCD_LINE_COUNT);

    settings_menu.lcd_cursor_char = 2;  // 126 Standard Pfeil
    menu_init(&settings_menu, settings_menu_entrys, 2,LCD_LINE_COUNT);

    menu_set_root(&main_menu);

    // Zeichen für Menü More Top setzen
    uint8_t char00[] = {4,14,31,0,0,0,0,0};
    lcd_generatechar(0, char00);

    // Zeichen für Menü More Down setzen
    uint8_t char01[] = {0,0,0,0,31,14,4,0};
    lcd_generatechar(1, char01);

    // Zeichen für Menü Position setzen
    uint8_t arrow_char[] = {0,4,6,31,6,4,0,0};
    lcd_generatechar(2, arrow_char);

    uint8_t start_soldering = 0;

    while(1)
    {
        switch(mode)
        {
        case MODE_MENU:
             update_gui();
            break;
        case MODE_SOLDERING:
            if(old_adc_value != current_adc_value)
            {
                old_adc_value = current_adc_value;
                lcd_clear();

                if(current_adc_value >= 156 && current_adc_value <= 581)
                {
                    current_temperature = pgm_read_word_near(temperature_table + current_adc_value - 156);
                    sprintf(string0,"Temperatur: %d%cC", current_temperature, 0xdf);
                    lcd_string(string0);

                    if(current_temperature > 180)
                    {
                        heating_top_off();
                        heating_bottom_off();
                        if(start_soldering == 0)
                        {
                            start_soldering = 1;
                            counter_soldering = 0;
                        }
                    }
                    if(current_temperature < 180)
                    {
                        heating_top_on();
                        heating_bottom_on();
                    }

                }
                else
                {
                    lcd_string("Temperatur: Error");
                }
            }

            if(counter_soldering == 600 && start_soldering == 1)
            {
                start_soldering = 0;
                mode = MODE_MENU;
                heating_top_off();
                heating_bottom_off();

                audio_signal_final();

                menu_refresh();
            }

            if(get_key_from_buffer() == KEY2_UP)
            {
                mode = MODE_MENU;
                heating_top_off();
                heating_bottom_off();
                menu_refresh();
            }
            break;
        case MODE_TEMPERN:
            if(old_adc_value != current_adc_value)
            {
                old_adc_value = current_adc_value;

                if(current_adc_value >= 156 && current_adc_value <= 581)
                {
                    current_temperature = pgm_read_word_near(temperature_table + current_adc_value - 156);
                    lcd_setcursor(0,2);
                    sprintf(string0,"Temperatur: %d%cC   ", current_temperature, 0xdf);
                    lcd_string(string0);

                    if(current_temperature > tempern_temperatur_max_start && pre_heating)
                    {
                        heating_top_off();
                    }

                    if(current_temperature >= tempern_temperatur)
                    {
                        heating_top_off();
                        pre_heating = 0;
                    }

                    if(current_temperature < tempern_temperatur && !pre_heating)
                    {
                        heating_top_on();
                    }
                }
                else
                {
                    lcd_string("Temperatur: Error");
                }
            }

            if(old_counter_tempern != counter_tempern)
            {
                old_counter_tempern = counter_tempern;

                uint16_t time_seconds = (counter_tempern_max - old_counter_tempern) / 20;
                uint16_t minutes = time_seconds / 60;
                uint16_t seconds = time_seconds % 60;
                lcd_setcursor(0,3);
                sprintf(string0,"Restzeit: %3.3d:%2.2d", minutes, seconds);
                //sprintf(string0,"CounterMax: %d", counter_tempern_max);
                lcd_string(string0);

                char string1[21];
                string1[20] = 0;
                uint16_t t = (old_counter_tempern * 20) / counter_tempern_max;
                lcd_setcursor(0,4);
                for(uint8_t i=0; i <t; i++)
                    lcd_data('*');
            }

            if(counter_tempern >= counter_tempern_max)
            {
                mode = MODE_MENU;
                heating_top_off();
                heating_bottom_off();
                audio_signal_final();
                menu_refresh();
            }

            if(get_key_from_buffer() == KEY2_UP)
            {
                mode = MODE_MENU;
                heating_top_off();
                heating_bottom_off();
                menu_refresh();
            }
            break;
         }
    }
}

/////////////////////////////////////////////////////////////////////

void show_start_message(void)
{
    lcd_clear();
    lcd_setcursor( 1, 1);
    lcd_string("- Reflow Ofen Kit -");
    lcd_setcursor( 2,2);
    lcd_string("Firmware:  ");
    lcd_string(VERSION);
    lcd_setcursor( 0,4);
    lcd_string("by thorsten kattanek");
    _delay_ms(START_MESSAGE_TIME);
    lcd_clear();
}

/////////////////////////////////////////////////////////////////////

void init_adc(void)
{
    ADMUX = 1 << REFS0 | 1 << REFS1;                // using external reference 5.0V,  result is right adjusted
    ADCSRA = 1 << ADEN | 1 << ADPS1 | 1 << ADPS2;   // enable adc // ADC Prescaler set of 64 (62,5kHz ADC Clock)
}

uint16_t get_adc(void)
{
    ADCSRA |= (1 << ADSC);          // start conversion
    while(ADCSRA & (1 << ADSC));    // wait of finish conversion
    return ADC;                     // return ADC0
}

void init_pwm(void)
{
    DDRB |= 1 << PB1;                                   // OC1A auf Ausgang setzen
    TCCR1A |= 1 << COM1A1 | /*1 << WGM13 |*/ 1 << WGM12 | /*1 << WGM11 |*/ 1 << WGM10;     // OC0A non-invertmode - FAST PWM Mode 10Bit
    TCCR1B |= 1 << CS10;                                 // Prescaler auf 8 --> 8MHz / 8 / 256 = 3,9 KHz
    OCR1A = 0;   // 0 - 1023
}

void set_fan_speed(uint8_t speed)
{
    OCR1A = speed;
}

void init_keys(void)
{
    // Entsprechende Ports auf Eingangschalten
    KEY0_DDR &= ~(1<<KEY0);
    KEY1_DDR &= ~(1<<KEY1);
    KEY2_DDR &= ~(1<<KEY2);

    // Interne Pullup Setzen
    KEY0_PORT |= 1<<KEY0;
    KEY1_PORT |= 1<<KEY1;
    KEY2_PORT |= 1<<KEY2;
}


void init_timer2(void)
{
    // Interrunpt auf 500Hz
    TCCR2 |= (1 << WGM21) | (1 << CS20) | (1 << CS21);  // CTC Mode / Normal port operation, OC2 disconnected / Prescaler 32
    OCR2 = 250; // Timer Top = 250
    TIMSK |= (1 << OCIE2); // Output Compare Match Interrupt Enable
}

ISR(TIMER2_COMP_vect)
{
    volatile static uint8_t counter0 = 0;
    volatile static uint16_t counter1 = 0;
    volatile static uint8_t key2_is_pressed = 0;
    volatile static uint8_t key2_next_up_is_invalid = 0;

    volatile static uint8_t old_drehgeber = 0;
    uint8_t pos_change;	// 0=keine Änderung 1=hoch 2=runter

    volatile static uint8_t old_key2 = 0;

    // Wird alle 2ms aufgerufen

    // Encoder
    old_drehgeber = (old_drehgeber << 2)  & 0x0F;
    if (PHASE_1A) old_drehgeber |=2;
    if (PHASE_1B) old_drehgeber |=1;
    pos_change = (char)pgm_read_byte(&drehimp_tab[old_drehgeber]);

    switch(pos_change)
    {
    case 1:
        key_buffer[key_buffer_w_pos++] = KEY0_DOWN;
        break;
    case 2:
        key_buffer[key_buffer_w_pos++] = KEY1_DOWN;
        break;
    default:
        break;
    }
    key_buffer_w_pos &= 0x0f;

    if(counter0 < 25)
    {
        counter0++;
        return;
    }

    counter0 = 0;

    // Ab hier alle 50ms

    counter1++;
    counter_soldering++;
    counter_tempern++;

    current_adc_value = get_adc();
    uint8_t key2 = get_key2();



    if(key2 != old_key2)
    {
        if(key2)
        {
            key_buffer[key_buffer_w_pos++] = KEY2_DOWN;
            counter1 = 0;
            key2_is_pressed = 1;
        }
        else
        {
            if(!key2_next_up_is_invalid)
                key_buffer[key_buffer_w_pos++] = KEY2_UP;
            key2_is_pressed = 0;
            key2_next_up_is_invalid = 0;
        }
    }

    if(key2_is_pressed && counter1 == TIMEOUT1_KEY2)
    {
        key_buffer[key_buffer_w_pos++] = KEY2_TIMEOUT1;
        key2_next_up_is_invalid = 1;
    }

    if(key2_is_pressed && counter1 == TIMEOUT2_KEY2)
    {
        key_buffer[key_buffer_w_pos++] = KEY2_TIMEOUT2;
        key2_next_up_is_invalid = 1;
    }

    key_buffer_w_pos &= 0x0f;
    old_key2 = key2;

}

uint8_t get_key_from_buffer()
{
    if(key_buffer_r_pos != key_buffer_w_pos)
    {
        uint8_t val = key_buffer[key_buffer_r_pos++];
        key_buffer_r_pos &= 0x0f;
        return  val;
    }
    else
        return NO_KEY;
}

void update_gui(void)
{
    uint8_t key_code = get_key_from_buffer();
    check_menu_events(menu_update(key_code));
}

void check_menu_events(uint16_t menu_event)
{
    uint8_t command = menu_event >> 8;
    uint8_t value = menu_event & 0xff;

    switch(command)
    {
    case MC_EXIT_MENU:
        audio_signal_final();
        break;

    case MC_SELECT_ENTRY:
        switch(value)
        {
        case M_START_SOLDERING:
            audio_beep();
            heating_top_on();
            heating_bottom_on();
            mode = MODE_SOLDERING;
            break;

        case M_START_TEMPERN:
            audio_beep();
            lcd_clear();
            lcd_string("Tempern...");
            heating_top_on();
            pre_heating = 1;
            counter_tempern_max = (uint32_t)tempern_time * 60 * 20;
            counter_tempern = old_counter_tempern = 0;
            mode = MODE_TEMPERN;
            break;

        case M_HEATER_TOP:
            audio_beep();
            if(menu_get_entry_var1(&main_menu, M_HEATER_TOP))
            {
                heating_top_on();
            }
            else
            {
                heating_top_off();
            }
            menu_refresh();
            break;

        case M_HEATER_BOTTOM:
            audio_beep();
            if(menu_get_entry_var1(&main_menu, M_HEATER_BOTTOM))
            {
                heating_bottom_on();
            }
            else
            {
                heating_bottom_off();
            }
            menu_refresh();
            break;
        }

    /*
    case MC_EXIT_MENU:
        set_gui_mode(GUI_INFO_MODE);
        break;

    case MC_SELECT_ENTRY:
        switch(value)
        {
        /// Main Menü

        /// Image Menü
        case M_INSERT_IMAGE:
            set_gui_mode(GUI_FILE_BROWSER);
            break;
        case M_REMOVE_IMAGE:
            remove_image();
            set_gui_mode(GUI_INFO_MODE);
            break;
        case M_WP_IMAGE:
            if(akt_image_type != G64_IMAGE)
                menu_set_entry_var1(&image_menu, M_WP_IMAGE, 1);

            if(menu_get_entry_var1(&image_menu, M_WP_IMAGE))
                set_write_protection(1);
            else
                set_write_protection(0);
                menu_refresh();
            break;

        /// Settings Menü
        case M_PIN_PB2:
            if(menu_get_entry_var1(&settings_menu, M_PIN_PB2))
            {
                DDRB |= 1<<PB2;
            }
            else
            {
                DDRB &= ~(1<<PB2);
            }
            menu_refresh();
            break;

        case M_PIN_PB3:
            if(menu_get_entry_var1(&settings_menu, M_PIN_PB3))
            {
                DDRB |= 1<<PB3;
            }
            else
            {
                DDRB &= ~(1<<PB3);
            }
            menu_refresh();
            break;

        case M_SAVE_EEPROM:
            break;

        case M_RESTART:
            exit_main = 0;
            break;

        /// Info Menü
        case M_VERSION_INFO:
            show_start_message();
            menu_refresh();
            break;

        case M_SDCARD_INFO:
            show_sdcard_info_message();
            menu_refresh();
            break;
        }
        break;
        */
    }
}

void audio_signal_final()
{
    buzzer_on();
    _delay_ms(1000);
    buzzer_off();
    _delay_ms(500);
    buzzer_on();
    _delay_ms(1000);
    buzzer_off();
    _delay_ms(500);
    buzzer_on();
    _delay_ms(1000);
    buzzer_off();
}

void audio_beep()
{
    buzzer_on();
    _delay_ms(30);
    buzzer_off();
}
