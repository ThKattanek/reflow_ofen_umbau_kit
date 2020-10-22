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

void show_start_message(void);
void init_adc(void);
uint16_t get_adc(void);

int main(void)
{
    char string0[100];

    // LCD initiallisieren

    lcd_init();
    
    status_led_init();

    // Board Diagnose Start
    // Signalisiert durch blinken das der µC
    // angefangen hat zu arbeiten
    for(int i=0; i<5; i++)
    {
        status_led_on();
        _delay_ms(50);

        status_led_off();
        _delay_ms(50);
    }
    
    // Buzzer initialisieren
    buzzer_init();

    // Startmeldung ausgeben
    buzzer_on();
    show_start_message();
    buzzer_on();
    
    // Heizung initialisieren
    heating_bottom_init();
    heating_top_init();

    // Init ADC
    init_adc();

    uint16_t current_adc = 0;
    uint16_t old_adc = 0;
    int16_t current_temerature = 0;

    while(1)
    {
        status_led_on();
        current_adc = get_adc();
        status_led_off();

        if(current_adc != old_adc)
        {
            lcd_clear();

            if(current_adc >= 156 && current_adc <= 581)
            {
                current_temerature = pgm_read_word_near(temperature_table + current_adc - 156);
                sprintf(string0,"Temperatur: %d%cC", current_temerature, 0xdf);
                lcd_string(string0);
            }
            else
            {
                lcd_string("Temperatur: Error");
            }

            old_adc = current_adc;
        }

        if(current_temerature > 30)
        {
            heating_top_on();
            heating_bottom_on();
            buzzer_on();
        }
        if(current_temerature < 27)
        {
            heating_top_off();
            heating_bottom_off();
            buzzer_off();
        }

        _delay_ms(40);
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
