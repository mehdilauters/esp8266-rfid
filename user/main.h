#ifndef USER_MAIN_H
#define USER_MAIN_H
bool get_button_pressed();
bool is_connected();
bool load_server(char * _server, int *_port);
bool load_network(struct sdk_station_config* _config);
bool save_network(char * _essid, char *_password);
bool save_server(char * _server, int _port);
#endif