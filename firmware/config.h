/* Name: config.h
 * Projekt: reflow_ofen_steuerung
 * Author: Thorsten Kattanek
 * Geändert am: 27.09.2020
 * Copyright: Thorsten Kattanek
 * License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)
 *
 */ 

// Zeit in ms wie lange die Version nach dem Start angezeigt wird
#define START_MESSAGE_TIME 1500

// Status LED Cofiguration --> PortB Pin0
#define STATUS_LED_PORT     PORTB
#define STATUS_LED_DDR      DDRB
#define STATUS_LED_PIN      PIN0

// Heizungsteuerung
// Heizung unten
#define HEATING_BOTTOM_PORT   PORTB
#define HEATING_BOTTOM_DDR    DDRB
#define HEATING_BOTTOM_PIN    PIN6

// Heizung oben
#define HEATING_TOP_PORT   PORTD
#define HEATING_TOP_DDR    DDRD
#define HEATING_TOP_PIN    PIN6

/////////////////////////////////////////////////////////////////////////////////////////////

#define status_led_init() STATUS_LED_DDR |= 1 << STATUS_LED_PIN; STATUS_LED_PORT |= 1 << STATUS_LED_PIN
#define status_led_on() STATUS_LED_PORT &= ~(1 << STATUS_LED_PIN)
#define status_led_off() STATUS_LED_PORT |= 1 << STATUS_LED_PIN

#define heating_bottom_init() HEATING_BOTTOM_DDR |= 0x03 << HEATING_BOTTOM_PIN; HEATING_BOTTOM_PORT |= 0x03 << HEATING_BOTTOM_PIN
#define heating_top_init() HEATING_TOP_DDR |= 0x03 << HEATING_TOP_PIN; HEATING_TOP_PORT |= 0x03 << HEATING_TOP_PIN

#define heating_bottom_on() HEATING_BOTTOM_PORT &= ~(0x03 << HEATING_BOTTOM_PIN)
#define heating_top_on() HEATING_TOP_PORT &= ~(0x03 << HEATING_TOP_PIN)

#define heating_bottom_off() HEATING_BOTTOM_PORT |= 0x03 << HEATING_BOTTOM_PIN
#define heating_top_off() HEATING_TOP_PORT |= 0x03 << HEATING_TOP_PIN
