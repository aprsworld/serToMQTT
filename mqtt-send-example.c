/* compile with: gcc mqtt-send-example.c -o mqtt-send-example -lmosquitto */
#define _GNU_SOURCE

#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

#include <mosquitto.h>

#define mqtt_host "localhost"
#define mqtt_port 1883


static int run = 1;


void connect_callback(struct mosquitto *mosq, void *obj, int result) {
	printf("# connect_callback, rc=%d\n", result);
}

#if 0
void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message) {
	char data[4];
	bool match = 0;
	uint16_t value = 0;



	printf("got message '%.*s' for topic '%s'\n", message->payloadlen, (char*) message->payload, message->topic);

	/* local copy of our message */
	strncpy(data, (char *) message->payload,3);
	data[3]='\0';

	

	/* light above big door */
	mosquitto_topic_matches_sub("refarm/shop/exteriorLightSouth", message->topic, &match);
	if (match) {
		printf("got message for exterior light south\n");

		/* connect to modbus gateway */
		if ( NULL == mb ) {
			fprintf(stderr,"# error creating modbus instance: %s\n", modbus_strerror(errno));
			return;

		}

		if ( -1 == modbus_connect(mb) ) {
			fprintf(stderr,"# modbus connection failed: %s\n", modbus_strerror(errno));
			modbus_free(mb);
			return;
		}


	}

	

}
#endif

int main(int argc, char **argv) {
	uint8_t reconnect = true;
	char clientid[24];
	struct mosquitto *mosq;
	int rc = 0;

	char testMessage[256];
	int messageID;

	fprintf(stderr,"# mqtt-send-example start-up\n");


	fprintf(stderr,"# initializing mosquitto MQTT library\n");
	mosquitto_lib_init();

	memset(clientid, 0, 24);
	snprintf(clientid, 23, "mqtt-send-example_%d", getpid());
	mosq = mosquitto_new(clientid, true, 0);

	if (mosq) {
		mosquitto_connect_callback_set(mosq, connect_callback);
//		mosquitto_message_callback_set(mosq, message_callback);

		fprintf(stderr,"# connecting to MQTT server %s:%d\n",mqtt_host,mqtt_port);
		rc = mosquitto_connect(mosq, mqtt_host, mqtt_port, 60);


		fprintf(stderr,"# sending test message\n");
		sprintf(testMessage,"hello world!");

		/* instance, message ID pointer, topic, data length, data, qos, retain */
		rc = mosquitto_publish(mosq, &messageID, "test", strlen(testMessage), testMessage, 0, 0); 

		fprintf(stderr,"# mosquitto_publish provided messageID=%d and return code=%d\n",messageID,rc);

		/* check return status of mosquitto_publish */ 
		if ( MOSQ_ERR_SUCCESS == rc ) {
			/* successful send */
		} else if ( MOSQ_ERR_INVAL == rc ) {
			fprintf(stderr,"# mosquitto error invalid parameters\n");
		} else if ( MOSQ_ERR_NOMEM == rc ) {
			fprintf(stderr,"# mosquitto error out of memory\n");
		} else if ( MOSQ_ERR_NO_CONN == rc ) {
			fprintf(stderr,"# mosquitto error no connection\n");
		} else if ( MOSQ_ERR_PROTOCOL == rc ) {
			fprintf(stderr,"# mosquitto error protocol\n");
		} else if ( MOSQ_ERR_PAYLOAD_SIZE == rc ) {
			fprintf(stderr,"# mosquitto error payload too large\n");
		} else if ( MOSQ_ERR_MALFORMED_UTF8 == rc ) {
			fprintf(stderr,"# mosquitto error topic is not valid UTF-8\n");
		} else {
			fprintf(stderr,"# mosquitto unknown error = %d\n",rc);
		}


		/* give async mosquitto_publish a chance to finish */
		sleep(1);


		mosquitto_destroy(mosq);
	}

	fprintf(stderr,"# mosquitto_lib_cleanup()\n");
	mosquitto_lib_cleanup();

	return rc;
}

