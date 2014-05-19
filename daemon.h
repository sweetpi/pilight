#ifndef NODEJS_MODUL
extern "C" {
	int start_daemon(const char* settingsContent);
	void log_setcallback(void (*logcallback)(int, char*));
	void broadcast_setcallback(void (*cb)(char*));
	void send_message(char* message);
}
#endif