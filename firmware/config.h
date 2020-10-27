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

// Buzzer (Dauerton)
#define BUZZER_PORT     PORTC
#define BUZZER_DDR      DDRC
#define BUZZER_PIN      PIN5

// Motor Power
#define MOTOR_POWER_PORT     PORTB
#define MOTOR_POWER_DDR      DDRB
#define MOTOR_POWER_PIN      PIN2

// Temperatur Sensor 1
#define SENSOR1_PORT    PORTC
#define SENSOR1_DDR     DDRC
#define SENSOR1_PIN     PIN0

// Temperatur Sensor 2 (Optional)
#define SENSOR2_PORT    PORTC
#define SENSOR2_DDR     DDRC
#define SENSOR2_PIN     PIN1

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

#define buzzer_init() BUZZER_DDR |= 0x03 << BUZZER_PIN; BUZZER_PORT |= 0x03 << BUZZER_PIN
#define buzzer_on() BUZZER_PORT &= ~(0x03 << BUZZER_PIN)
#define buzzer_off() BUZZER_PORT |= 0x03 << BUZZER_PIN

#define motor_power_init() MOTOR_POWER_DDR |= 1 << MOTOR_POWER_PIN
#define motor_power_on() MOTOR_POWER_PORT |= 1 << MOTOR_POWER_PIN
#define motor_power_off() MOTOR_POWER_PORT &= ~(1 << MOTOR_POWER_PIN)
