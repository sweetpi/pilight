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
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <regex.h>
#include <sys/stat.h>
#include <time.h>
#include <libgen.h>

#include "../../pilight.h"
#include "common.h"
#include "json.h"
#include "settings.h"
#include "http_lib.h"
#include "log.h"

/* Add a string value to the settings struct */
void settings_add_string(const char *name, char *value) {
	struct settings_t *snode = malloc(sizeof(struct settings_t));
	if(!snode) {
		logprintf(LOG_ERR, "out of memory");
		exit(EXIT_FAILURE);
	}
	snode->name = malloc(strlen(name)+1);
	if(!snode->name) {
		logprintf(LOG_ERR, "out of memory");
		exit(EXIT_FAILURE);
	}
	strcpy(snode->name, name);
	snode->value = malloc(strlen(value)+1);
	if(!snode->value) {
		logprintf(LOG_ERR, "out of memory");
		exit(EXIT_FAILURE);
	}
	strcpy(snode->value, value);
	snode->type = 2;
	snode->next = settings;
	settings = snode;
}

/* Add an int value to the settings struct */
void settings_add_number(const char *name, int value) {
	struct settings_t *snode = malloc(sizeof(struct settings_t));
	if(!snode) {
		logprintf(LOG_ERR, "out of memory");
		exit(EXIT_FAILURE);
	}
	char ctmp[256];
	snode->name = malloc(strlen(name)+1);
	if(!snode->name) {
		logprintf(LOG_ERR, "out of memory");
		exit(EXIT_FAILURE);
	}
	strcpy(snode->name, name);
	sprintf(ctmp, "%d", value);
	snode->value = malloc(strlen(ctmp)+1);
	if(!snode->value) {
		logprintf(LOG_ERR, "out of memory");
		exit(EXIT_FAILURE);
	}
	strcpy(snode->value, ctmp);
	snode->type = 1;
	snode->next = settings;
	settings = snode;
}

/* Retrieve a numeric value from the settings struct */
int settings_find_number(const char *name, int *out) {
	struct settings_t *tmp_settings = settings;

	while(tmp_settings) {
		if(strcmp(tmp_settings->name, name) == 0 && tmp_settings->type == 1) {
			*out = atoi(tmp_settings->value);
			return EXIT_SUCCESS;
		}
		tmp_settings = tmp_settings->next;
	}
	sfree((void *)&tmp_settings);
	return EXIT_FAILURE;
}

/* Retrieve a string value from the settings struct */
int settings_find_string(const char *name, char **out) {
	struct settings_t *tmp_settings = settings;

	while(tmp_settings) {
		if(strcmp(tmp_settings->name, name) == 0 && tmp_settings->type == 2) {
			*out = tmp_settings->value;
			return EXIT_SUCCESS;
		}
		tmp_settings = tmp_settings->next;
	}
	sfree((void *)&tmp_settings);
	return EXIT_FAILURE;
}

/* Check if a given file exists */
int settings_file_exists(char *filename) {
	struct stat sb;   
	return stat(filename, &sb);
}

int settings_parse(JsonNode *root) {
	int have_error = 0;

#ifdef WEBSERVER
	int web_port = 0;
	int own_port = 0;

	char *webgui_tpl = malloc(strlen(WEBGUI_TEMPLATE)+1);
	if(!webgui_tpl) {
		logprintf(LOG_ERR, "out of memory");
		exit(EXIT_FAILURE);
	}
	strcpy(webgui_tpl, WEBGUI_TEMPLATE);
	char *webgui_root = malloc(strlen(WEBSERVER_ROOT)+1);
	if(!webgui_root) {
		logprintf(LOG_ERR, "out of memory");
		exit(EXIT_FAILURE);
	}
	strcpy(webgui_root, WEBSERVER_ROOT);
#endif

#ifndef __FreeBSD__	
	regex_t regex;
	int reti;
#endif	
	
	JsonNode *jsettings = json_first_child(root);
	
	while(jsettings) {
		if(strcmp(jsettings->key, "port") == 0 
		   || strcmp(jsettings->key, "send-repeats") == 0
		   || strcmp(jsettings->key, "receive-repeats") == 0) {
			if((int)jsettings->number_ == 0) {
				logprintf(LOG_ERR, "setting \"%s\" must contain a number larger than 0", jsettings->key);
				have_error = 1;
				goto clear;
			} else {
#ifdef WEBSERVER			
				if(strcmp(jsettings->key, "port") == 0) {
					own_port = (int)jsettings->number_;
				}
#endif
				settings_add_number(jsettings->key, (int)jsettings->number_);
			}
		} else if(strcmp(jsettings->key, "standalone") == 0) {
			if(jsettings->number_ < 0 || jsettings->number_ > 1) {
				logprintf(LOG_ERR, "setting \"%s\" must be either 0 or 1", jsettings->key);
				have_error = 1;
				goto clear;
			} else {
				settings_add_number(jsettings->key, (int)jsettings->number_);
			}
		}  else if(strcmp(jsettings->key, "firmware-update") == 0) {
			if(jsettings->number_ < 0 || jsettings->number_ > 1) {
				logprintf(LOG_ERR, "setting \"%s\" must be either 0 or 1", jsettings->key);
				have_error = 1;
				goto clear;
			} else {
				settings_add_number(jsettings->key, (int)jsettings->number_);
			}
		} else if(strcmp(jsettings->key, "log-level") == 0) {
			if((int)jsettings->number_ < 0 || (int)jsettings->number_ > 5) {
				logprintf(LOG_ERR, "setting \"%s\" must contain a number from 0 till 5", jsettings->key);
				have_error = 1;
				goto clear;
			} else {
				settings_add_number(jsettings->key, (int)jsettings->number_);
			}
		} else if(strcmp(jsettings->key, "pid-file") == 0 || strcmp(jsettings->key, "log-file") == 0) {
			if(!jsettings->string_) {
				logprintf(LOG_ERR, "setting \"%s\" must contain an existing file path", jsettings->key);
				have_error = 1;
				goto clear;
			} else {
				if(path_exists(jsettings->string_) != EXIT_SUCCESS) {
					logprintf(LOG_ERR, "setting \"%s\" must point to an existing folder", jsettings->key);
					have_error = 1;
					goto clear;				
				} else {
					settings_add_string(jsettings->key, jsettings->string_);
				}
			}
		} else if(strcmp(jsettings->key, "config-file") == 0 || strcmp(jsettings->key, "hardware-file") == 0) {
			if(!jsettings->string_) {
				logprintf(LOG_ERR, "setting \"%s\" must contain an existing file path", jsettings->key);
				have_error = 1;
				goto clear;
			} else if(strlen(jsettings->string_) > 0) {
#ifndef NODEJS_MODULE
				if(settings_file_exists(jsettings->string_) == EXIT_SUCCESS) {
					settings_add_string(jsettings->key, jsettings->string_);
				} else {
					logprintf(LOG_ERR, "setting \"%s\" must point to an existing file", jsettings->key);
					have_error = 1;
					goto clear;
				}
#else
				settings_add_string(jsettings->key, jsettings->string_);
#endif				

			}
		} else if(strcmp(jsettings->key, "whitelist") == 0) {
			if(!jsettings->string_) {
				logprintf(LOG_ERR, "setting \"%s\" must contain valid ip addresses", jsettings->key);
				have_error = 1;
				goto clear;
			} else if(strlen(jsettings->string_) > 0) {
#ifndef __FreeBSD__			
				char validate[] = "^((\\*|[0-9]|[1-9][0-9]|1[0-9][0-9]|2([0-4][0-9]|5[0-5]))\\.(\\*|[0-9]|[1-9][0-9]|1[0-9][0-9]|2([0-4][0-9]|5[0-5]))\\.(\\*|[0-9]|[1-9][0-9]|1[0-9][0-9]|2([0-4][0-9]|5[0-5]))\\.(\\*|[0-9]|[1-9][0-9]|1[0-9][0-9]|2([0-4][0-9]|5[0-5]))(,[\\ ]|,|$))+$";
				reti = regcomp(&regex, validate, REG_EXTENDED);
				if(reti) {
					logprintf(LOG_ERR, "could not compile regex");
					have_error = 1;
					goto clear;
				}
				reti = regexec(&regex, jsettings->string_, 0, NULL, 0);
				if(reti == REG_NOMATCH || reti != 0) {
					logprintf(LOG_ERR, "setting \"%s\" must contain valid ip addresses", jsettings->key);
					have_error = 1;
					regfree(&regex);
					goto clear;
				}
				regfree(&regex);
#endif
				int l = (int)strlen(jsettings->string_)-1;
				if(jsettings->string_[l] == ' ' || jsettings->string_[l] == ',') {
					logprintf(LOG_ERR, "setting \"%s\" must contain valid ip addresses", jsettings->key);
					have_error = 1;
					goto clear;
				}
				settings_add_string(jsettings->key, jsettings->string_);
			}
#ifdef WEBSERVER
		} else if(strcmp(jsettings->key, "webserver-port") == 0) {
			if(jsettings->number_ < 0) {
				logprintf(LOG_ERR, "setting \"%s\" must contain a number larger than 0", jsettings->key);
				have_error = 1;
				goto clear;
			} else {
				web_port = (int)jsettings->number_;
				settings_add_number(jsettings->key, (int)jsettings->number_);
			}
		} else if(strcmp(jsettings->key, "webserver-root") == 0) {
			if(!jsettings->string_ || path_exists(jsettings->string_) != 0) {
				logprintf(LOG_ERR, "setting \"%s\" must contain a valid path", jsettings->key);
				have_error = 1;
				goto clear;
			} else {
				webgui_root = realloc(webgui_root, strlen(jsettings->string_)+1);
				if(!webgui_root) {
					logprintf(LOG_ERR, "out of memory");
					exit(EXIT_FAILURE);
				}
				strcpy(webgui_root, jsettings->string_);
				settings_add_string(jsettings->key, jsettings->string_);
			}
		} else if(strcmp(jsettings->key, "webserver-enable") == 0) {
			if(jsettings->number_ < 0 || jsettings->number_ > 1) {
				logprintf(LOG_ERR, "setting \"%s\" must be either 0 or 1", jsettings->key);
				have_error = 1;
				goto clear;
			} else {
				settings_add_number(jsettings->key, (int)jsettings->number_);
			}
		} else if(strcmp(jsettings->key, "webserver-cache") == 0) {
			if(jsettings->number_ < 0 || jsettings->number_ > 1) {
				logprintf(LOG_ERR, "setting \"%s\" must be either 0 or 1", jsettings->key);
				have_error = 1;
				goto clear;
			} else {
				settings_add_number(jsettings->key, (int)jsettings->number_);
			} 
		} else if(strcmp(jsettings->key, "webserver-user") == 0) {
			if(jsettings->string_ || strlen(jsettings->string_) > 0) {
				if(name2uid(jsettings->string_) == -1) {
					logprintf(LOG_ERR, "setting \"%s\" must contain a valid system user", jsettings->key);
					have_error = 1;
					goto clear;
				} else {
					settings_add_string(jsettings->key, jsettings->string_);
				}
			}
		} else if(strcmp(jsettings->key, "webserver-authentication") == 0 && jsettings->tag == JSON_ARRAY) {
				JsonNode *jtmp = json_first_child(jsettings);
				unsigned short i = 0;
				while(jtmp) {
					i++;
					if(jtmp->tag == JSON_STRING) {
						if(i == 1) {
							settings_add_string("webserver-authentication-username", jtmp->string_);
						} else if(i == 2) {
							settings_add_string("webserver-authentication-password", jtmp->string_);
						}
					} else {
						have_error = 1;
						break;
					}
					if(i > 2) {
						have_error = 1;
						break;
					}
					jtmp = jtmp->next;
				}
				if(i != 2 || have_error == 1) {
					logprintf(LOG_ERR, "setting \"%s\" must be in the format of [ \"username\", \"password\" ]", jsettings->key);
					have_error = 1;
					goto clear;
				}
		}  else if(strcmp(jsettings->key, "webgui-template") == 0) {
			if(!jsettings->string_) {
				logprintf(LOG_ERR, "setting \"%s\" must be a valid template", jsettings->key);
				have_error = 1;
				goto clear;
			} else {
				webgui_tpl = realloc(webgui_tpl, strlen(jsettings->string_)+1);
				if(!webgui_tpl) {
					logprintf(LOG_ERR, "out of memory");
					exit(EXIT_FAILURE);
				}
				strcpy(webgui_tpl, jsettings->string_);
				settings_add_string(jsettings->key, jsettings->string_);
			}
#endif
#ifdef UPDATE
		} else if(strcmp(jsettings->key, "update-check") == 0) {
			if(jsettings->number_ < 0 || jsettings->number_ > 1) {
				logprintf(LOG_ERR, "setting \"%s\" must be either 0 or 1", jsettings->key);
				have_error = 1;
				goto clear;
			} else {
				settings_add_number(jsettings->key, (int)jsettings->number_);
			} 
		} else if(strcmp(jsettings->key, "update-development") == 0) {
			if(jsettings->number_ < 0 || jsettings->number_ > 1) {
				logprintf(LOG_ERR, "setting \"%s\" must be either 0 or 1", jsettings->key);
				have_error = 1;
				goto clear;
			} else {
				settings_add_number(jsettings->key, (int)jsettings->number_);
			}
		} else if(strcmp(jsettings->key, "update-mirror") == 0) {
			char *filename = NULL;
			char *url = NULL;
			char *data = NULL;
			int lg = 0;
			char typebuf[70];

			if(jsettings->string_) {
				url = malloc(strlen(jsettings->string_)+1);
				if(!url) {
					logprintf(LOG_ERR, "out of memory");
					exit(EXIT_FAILURE);
				}
				strcpy(url, jsettings->string_);
				http_parse_url(url, &filename);
			}
			if(!jsettings->string_ || http_get(filename, &data, &lg, typebuf) != 200) {
				logprintf(LOG_ERR, "setting \"%s\" must be point to a valid (online) file", jsettings->key);
				/* clean-up http_lib global */
				if(http_server) sfree((void *)&http_server);
				if(filename) sfree((void *)&filename);
				if(url) sfree((void *)&url);
				if(data) sfree((void *)&data);
				have_error = 1;
				goto clear;
			} else {
				settings_add_string(jsettings->key, jsettings->string_);
				/* clean-up http_lib global */
				if(http_server) sfree((void *)&http_server);
				if(filename) sfree((void *)&filename);
				if(url) sfree((void *)&url);
				if(data) sfree((void *)&data);
			}
#endif		
		} else {
			logprintf(LOG_ERR, "setting \"%s\" is invalid", jsettings->key);
			have_error = 1;
			goto clear;
		}
		jsettings = jsettings->next;
	}
	json_delete(jsettings);
#ifdef WEBSERVER
	if(webgui_tpl) {
		char *tmp = malloc(strlen(webgui_root)+strlen(webgui_tpl)+13);
		if(!tmp) {
			logprintf(LOG_ERR, "out of memory");
			exit(EXIT_FAILURE);
		}
		sprintf(tmp, "%s/%s/index.html", webgui_root, webgui_tpl);
		if(path_exists(tmp) != EXIT_SUCCESS) {
			logprintf(LOG_ERR, "setting \"webgui-template\", template does not exists");
			have_error = 1;
			sfree((void *)&tmp);
			goto clear;		
		}
		sfree((void *)&tmp);
	}

	if(web_port == own_port) {
		logprintf(LOG_ERR, "setting \"port\" and \"webserver-port\" cannot be the same");
		have_error = 1;
		goto clear;
	}
#endif
clear:
#ifdef WEBSERVER
	if(webgui_tpl) {
		sfree((void *)&webgui_tpl);
	}
	if(webgui_root) {
		sfree((void *)&webgui_root);
	}
#endif
	return have_error;
}


int settings_write(char *content) {
#ifndef NODEJS_MODULE
	FILE *fp;

	/* Overwrite config file with proper format */
	if(!(fp = fopen(settingsfile, "w+"))) {
		logprintf(LOG_ERR, "cannot write settings file: %s", settingsfile);
		return EXIT_FAILURE;
	}
	fseek(fp, 0L, SEEK_SET);
 	fwrite(content, sizeof(char), strlen(content), fp);
	fclose(fp);
#endif
	return EXIT_SUCCESS;
}



int settings_gc(void) {
	struct settings_t *tmp;

	while(settings) {
		tmp = settings;
		sfree((void *)&tmp->name);
		sfree((void *)&tmp->value);
		settings = settings->next;
		sfree((void *)&tmp);
	}
	sfree((void *)&settings);
	
	sfree((void *)&settingsfile);
	logprintf(LOG_DEBUG, "garbage collected settings library");
	return 1;
}

int settings_read(void) {
	FILE *fp;
	char *content;
	size_t bytes;
	JsonNode *root;
	struct stat st;

	/* Read JSON config file */
	if(!(fp = fopen(settingsfile, "rb"))) {
		logprintf(LOG_ERR, "cannot read settings file: %s", settingsfile);
		return EXIT_FAILURE;
	}

	fstat(fileno(fp), &st);
	bytes = (size_t)st.st_size;

	if(!(content = calloc(bytes+1, sizeof(char)))) {
		logprintf(LOG_ERR, "out of memory");
		fclose(fp);
		return EXIT_FAILURE;
	}

	if(fread(content, sizeof(char), bytes, fp) == -1) {
		logprintf(LOG_ERR, "cannot read settings file: %s", settingsfile);
	}
	fclose(fp);

	/* Validate JSON and turn into JSON object */
	if(json_validate(content) == false) {
		logprintf(LOG_ERR, "settings are not in a valid json format", content);
		sfree((void *)&content);
		return EXIT_FAILURE;
	}

	root = json_decode(content);

	if(settings_parse(root) != 0) {
		sfree((void *)&content);
		return EXIT_FAILURE;
	}
	char *output = json_stringify(root, "\t");
	settings_write(output);
	json_delete(root);
	sfree((void *)&output);	
	sfree((void *)&content);
	return EXIT_SUCCESS;
}

#ifdef NODEJS_MODULE
int settings_set_from_string(char *content) {
	JsonNode *root;

	/* Validate JSON and turn into JSON object */
	if(json_validate(content) == false) {
		logprintf(LOG_ERR, "settings are not in a valid json format", content);
		return EXIT_FAILURE;
	}
	root = json_decode(content);
	if(settings_parse(root) != 0) {
		json_delete(root);
		return EXIT_FAILURE;
	}
	json_delete(root);
	return EXIT_SUCCESS;
}
#endif


int settings_set_file(char *settfile) {
	if(access(settfile, R_OK | W_OK) != -1) {
		settingsfile = realloc(settingsfile, strlen(settfile)+1);
		if(!settingsfile) {
			logprintf(LOG_ERR, "out of memory");
			exit(EXIT_FAILURE);
		}
		strcpy(settingsfile, settfile);
	} else {
		fprintf(stderr, "%s: the settings file %s does not exists\n", progname, settfile);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}