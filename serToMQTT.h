#ifndef serToMQTT_serToMQTT_H
#define serToMQTT_serToMQTT_H
json_object *json_object_new_dateTime(void);
struct json_object *json_division(double value,char *description, char *units);
struct json_object *json_int_division(int value,char *description, char *units);
struct json_object *json_string_division(char * value,char *description, char *units);
struct json_object *nullValue(void);
int setDateTimeFromGPS(char *aprsworld_date_time); 
extern int no_meta;
extern   void set_blocking (int fd, int vmin, int vtime) ;
extern int serToMQTT_pub(const char *message ,const char *topic );
extern char mqtt_topic[];
extern char mqtt_meta_topic[];
extern int retainedFlag;
extern const char * new_topic(const char *packet,const char *topic );
#endif
