#ifndef serToMQTT_serToMQTT_H
#define serToMQTT_serToMQTT_H
json_object *json_object_new_dateTime(void);
struct json_object *json_division(double value,char *description, char *units);
int setDateTimeFromGPS(char *gps_date,char *gps_time ); 
extern int no_meta;
#endif
