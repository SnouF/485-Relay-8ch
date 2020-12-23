/*
 * relaySTM32.ino
 *
 * Copyright 2020 Jonas <jonas@mythtv-fr.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 *
 */


//~ #define DEBUG

// https://github.com/SnouF/modbus-arduino
#include <Modbus.h>
#include <ModbusSerial.h>
ModbusSerial mb;

//GENERAL
#define SLAVE_ID 52
#define BAUDERATE 9600
#define DE_PIN PB6

#define REFRESH_STATE_TIMER 5 //ms
#define NB_CHANELS 8

//OUTPUT SWITCH
const uint8_t PINS_RELAY[NB_CHANELS]    { PB3, PB4, PA12, PA15, PA11, PA8, PB1, PB0};
const uint8_t ADDRS_RELAY[NB_CHANELS]   { 1, 2, 3, 4, 5, 6, 7, 8};

//INPUT SWITCH
const uint8_t PINS_BUTTON[NB_CHANELS]   { PA0, PA1, PA2 , PA3 , PA4 , PA_5 , PA6 , PA7 };
const uint8_t BUTTONS_MODS[NB_CHANELS]  { 1, 1, 1, 1, 1, 1, 1, 1}; //Mode (0: interrupteur à bascule, 1: interrupteur à pression, 2: interrupteur à relachement, autre : spéciale)
const uint8_t ADDRS_BUTTON[NB_CHANELS]  { 1, 2, 3, 4, 5, 6, 7, 8};
uint8_t buttons_read[NB_CHANELS];
bool buttons_state[NB_CHANELS];

uint16_t time = 0;
uint16_t timer = 0;


void setup()
{
    mb.config(&Serial1, BAUDERATE, SERIAL_8N1,DE_PIN);
    mb.setSlaveId(SLAVE_ID);

    #if defined(DEBUG)
        Serial1.println("Serial1 Begin");
    #endif

    //SWITCH
    for (uint8_t i = 0; i < NB_CHANELS; i++)
    {
        //SWITCH
        mb.addIsts(ADDRS_BUTTON[i], 0);
        pinMode(PINS_BUTTON[i], INPUT);
        buttons_state[i] = digitalRead(PINS_BUTTON[i]);


        //RELAY
        mb.addCoil(ADDRS_RELAY[i], 0);
        pinMode(PINS_RELAY[i], OUTPUT);
        digitalWrite(PINS_RELAY[i],HIGH);
    }
}


void loop()
{
    mb.task();
    time = millis();

    if ( (uint16_t) ( time - timer ) > (uint16_t) REFRESH_STATE_TIMER )
    {
        timer = time;
        for (uint8_t i = 0; i < NB_CHANELS; i++)
        {
            buttons_read[i] = digitalRead(PINS_BUTTON[i]) * 20 +  buttons_read[i] * 235 / 256;
            if ( ((buttons_state[i] == false ) && (buttons_read[i] > (128+40)))
              || ((buttons_state[i] == true  ) && (buttons_read[i] < (128-40))) )
            {
                #if defined(DEBUG)
                if ((buttons_read[i] > 128) != buttons_state[i])
                {
                    Serial1.print("button ");
                    Serial1.print(i);
                    Serial1.print(" read ");
                    Serial1.print(buttons_read[i]);
                    Serial1.print(" state ");
                    Serial1.println(buttons_read[i] > 128);
                }
                #endif

                buttons_state[i] = buttons_read[i] > 128;
                mb.Ists(ADDRS_BUTTON[i], buttons_state[i]);

                if ( ( BUTTONS_MODS[i] == 0 )
                  || ( BUTTONS_MODS[i] == 1 && buttons_state[i] == HIGH)
                  || ( BUTTONS_MODS[i] == 2 && buttons_state[i] == LOW))
                {
                    mb.Coil(ADDRS_RELAY[i], mb.Coil(ADDRS_RELAY[i]) == 0 ? 0xff : 0x00);
                    #if defined(DEBUG)
                        Serial1.print("relay ");
                        Serial1.print(i);
                        Serial1.print(" state ");
                        Serial1.println( (mb.Coil(ADDRS_RELAY[i]) == 0) ? "HIGH" : "LOW");
                    #endif
                }
            }
            //allume/éteint (en dehors du if pour si changer via le serveur)
            digitalWrite(PINS_RELAY[i], mb.Coil(ADDRS_RELAY[i]) == 0 ? HIGH : LOW);
        }
    }
}
