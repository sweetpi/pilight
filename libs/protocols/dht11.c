/*
	Copyright (C) 2013 CurlyMo

	This file is part of pilight.

	pilight is free software: you can redistribute it and/or modify it under the 
	terms of the GNU General Public License as published by the Free Software 
	Foundation, either version 3 of the License, or (at your option) any later 
	version.

	pilight is distributed in the hope that it will be useful, but WITHOUT ANY 
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR 
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with pilight. If not, see	<http://www.gnu.org/licenses/>
*/

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdint.h>

#include "../../pilight.h"
#include "common.h"
#include "log.h"
#include "threads.h"
#include "protocol.h"
#include "hardware.h"
#include "binary.h"
#include "gc.h"
#include "json.h"
#include "dht11.h"
#include "../pilight/wiringPi.h"
	
#define MAXTIMINGS 85

unsigned short dht11_loop = 1;

void *dht11Parse(void *param) {

	struct JsonNode *json = (struct JsonNode *)param;
	struct JsonNode *jsettings = NULL;
	int interval = 5;
	int dht_pin = 7;

	json_find_number(json, "gpio", &dht_pin);
	if((jsettings = json_find_member(json, "settings"))) {
		json_find_number(jsettings, "interval", &interval);
	}
	json_delete(json);	
	
	while(dht11_loop) {

		int tries = 5;
		unsigned short got_correct_date = 0;

		while(tries && !got_correct_date) {

			int laststate = HIGH;
			int counter = 0;
			int j = 0, i = 0;

			int dht11_dat[5] = {0,0,0,0,0};

			// pull pin down for 18 milliseconds
			pinMode(dht_pin, OUTPUT);
			digitalWrite(dht_pin, LOW);
			delay(18);
			// then pull it up for 40 microseconds
			digitalWrite(dht_pin, HIGH);
			delayMicroseconds(40);
			// prepare to read the pin
			pinMode(dht_pin, INPUT);

			// detect change and read data
			for(i=0; i<MAXTIMINGS; i++) {
				counter = 0;
				while(digitalRead(dht_pin) == laststate) {
					counter++;
					delayMicroseconds(1);
					if (counter == 255) {
						break;
					}
				}
				laststate = digitalRead(dht_pin);

				if(counter == 255) 
					break;

				// ignore first 3 transitions
				if((i >= 4) && (i%2 == 0)) {
					// shove each bit into the storage bytes
					dht11_dat[j/8] <<= 1;
					if (counter > 16)
						dht11_dat[j/8] |= 1;
						j++;
				  	}
				}

			// check we read 40 bits (8bit x 5 ) + verify checksum in the last byte
			// print it out if data is good
			if((j >= 40) && (dht11_dat[4] == ((dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xFF))) {

				got_correct_date = 1;

				int h = dht11_dat[0] * 100 + dht11_dat[1];
				int t = dht11_dat[2] * 100 + dht11_dat[3];
				
				dht11->message = json_mkobject();
				JsonNode *code = json_mkobject();
				json_append_member(code, "id", json_mkstring(dht11->id));
				json_append_member(code, "temperature", json_mknumber(t));
				json_append_member(code, "humidity", json_mknumber(h));

				json_append_member(dht11->message, "code", code);
				json_append_member(dht11->message, "origin", json_mkstring("receiver"));
				json_append_member(dht11->message, "protocol", json_mkstring(dht11->id));
				pilight.broadcast(dht11->id, dht11->message);
				json_delete(dht11->message);
				dht11->message = NULL;
			} else {
				logprintf(LOG_DEBUG, "dht11 data checksum was wrong");
				tries--;
				sleep(1);
			}
		}
		sleep((unsigned int)interval);
	}

	return (void *)NULL;
}

void dht11InitDev(JsonNode *jdevice) {
	char *output = json_stringify(jdevice, NULL);
	JsonNode *json = json_decode(output);
	threads_register("dht11", &dht11Parse, (void *)json);
	sfree((void *)&output);
}

int dht11GC(void) {
	dht11_loop = 0;
	return 1;
}

void dht11Init(void) {
	gc_attach(dht11GC);

	protocol_register(&dht11);
	protocol_set_id(dht11, "dht11");
	protocol_device_add(dht11, "dht11", "1-wire temperature and humidity sensor");
	dht11->devtype = WEATHER;
	dht11->hwtype = SENSOR;

	options_add(&dht11->options, 't', "temperature", has_value, config_value, "^[0-9]{1,3}$");
	options_add(&dht11->options, 'h', "humidity", has_value, config_value, "^[0-9]{1,3}$");
	options_add(&dht11->options, 'i', "id", has_value, config_id, ".+");
	options_add(&dht11->options, 'g', "gpio", has_value, config_value, "^[0-9]{1,2}$");

	protocol_setting_add_number(dht11, "decimals", 1);
	protocol_setting_add_number(dht11, "humidity", 1);
	protocol_setting_add_number(dht11, "temperature", 1);
	protocol_setting_add_number(dht11, "battery", 0);
	protocol_setting_add_number(dht11, "interval", 5);

	dht11->initDev=&dht11InitDev;
	
	wiringPiSetup();
}