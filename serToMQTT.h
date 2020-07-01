#ifndef serToMQTT_serToMQTT_H
#define serToMQTT_serToMQTT_H
json_object *json_object_new_dateTime(void);
struct json_object *json_division(double value,char *description, char *units);
struct json_object *json_int_division(int value,char *description, char *units);
struct json_object *json_string_division(char * value,char *description, char *units);
struct json_object *nullValue(void);
int setDateTimeFromGPS(char *aprsworld_date_time); 
extern uint64_t microtime(void);
extern void sleep_half_the_time(void);
extern int no_meta;
extern   void set_blocking (int fd, int vmin, int vtime) ;
extern int serToMQTT_pub(const char *message ,const char *topic );
extern char mqtt_topic[];
extern char mqtt_meta_topic[];
extern int retainedFlag;
extern int outputDebug;
extern int alarmSeconds;
extern int milliseconds_timeout;
extern const char * new_topic(const char *packet,const char *topic );
extern char *strsave(const char *s );
extern int do_station(const char *cmd );
extern int do_anemometer(const char *cmd );
extern int jsonToAPCALC(json_object ** jobjP, char *commands );

#endif
