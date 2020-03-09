/*******************************************************************************
 * Copyright (c) 2014, 2015 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *    Ian Craggs - make sure QoS2 processing works, and add device headers
 *******************************************************************************/

 /**
  This is a sample program to illustrate the use of the MQTT Client library
  on the mbed platform.  The Client class requires two classes which mediate
  access to system interfaces for networking and timing.  As long as these two
  classes provide the required public programming interfaces, it does not matter
  what facilities they use underneath. In this program, they use the mbed
  system libraries.

 */

 // change this to 1 to output messages to LCD instead of serial
#define USE_LCD 0

#if USE_LCD
#include "C12832.h"

// the actual pins are defined in mbed_app.json and can be overridden per target
C12832 lcd(LCD_MOSI, LCD_SCK, LCD_MISO, LCD_A0, LCD_NCS);

#define logMessage lcd.cls();lcd.printf

#else

#define logMessage printf

#endif

#define MQTTCLIENT_QOS2 1

#include "easy-connect.h"
#include "MQTTNetwork.h"
#include "MQTTmbed.h"
#include "MQTTClient.h"
#include "LM75B.h" //temperature sensor library need: 'mbed add https://os.mbed.com/users/chris/code/LM75B/'

LM75B temp_sensor(D14,D15); //set up the sensor and name it temp_sensor

int arrivedcount = 0;

void messageArrived(MQTT::MessageData& md);

int main(int argc, char* argv[])
{
    float version = 0.6;
    char* topic = "mbed-sample";

    logMessage("HelloMQTT: version is %.2f\r\n", version);

    NetworkInterface* network = easy_connect(true);
    if (!network) {
        return -1;
    }

    MQTTNetwork mqttNetwork(network);

    MQTT::Client<MQTTNetwork, Countdown> client(mqttNetwork);

    const char* hostname = "192.168.1.89";
    int port = 1883;
    logMessage("Connecting to %s:%d\r\n", hostname, port);
    int rc = mqttNetwork.connect(hostname, port);
    if (rc != 0)
        logMessage("rc from TCP connect is %d\r\n", rc);

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = "mbed-sample";
    //data.username.cstring = "testuser";
    //data.password.cstring = "testpassword";
    if ((rc = client.connect(data)) != 0)
        logMessage("rc from MQTT connect is %d\r\n", rc);

    if ((rc = client.subscribe(topic, MQTT::QOS2, messageArrived)) != 0)
        logMessage("rc from MQTT subscribe is %d\r\n", rc);

    MQTT::Message message;

    //the buffer to store the data
    char buf[100];							
    sprintf(buf, "Hello World!  QoS 0 message from app version %f\r\n", version);
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf)+1;
    rc = client.publish(topic, message);
    while (arrivedcount < 1)
        client.yield(100);

    // QoS 1
    sprintf(buf, "Hello World!  QoS 1 message from app version %f\r\n", version);
    message.qos = MQTT::QOS1;
    message.payloadlen = strlen(buf)+1;
    rc = client.publish(topic, message);
    while (arrivedcount < 2)
        client.yield(100);

    // QoS 2
    sprintf(buf, "Hello World!  QoS 2 message from app version %f\r\n", version);
    message.qos = MQTT::QOS2;
    message.payloadlen = strlen(buf)+1;
    rc = client.publish(topic, message); 
    while (arrivedcount < 3)
        client.yield(100);

    //create a variable for the temperature data
    float temp = 0.0;
    
    //publish the value of the temperature sensor to the sensors/mw_board1/temperature topic
    char* topic2 = "sensors/mw_board1/temperature";
    
    
    //set up a loop to send the sensor data
    while(true) {
        temp = temp_sensor.read(); 			//get the sensor reading
        printf("pub: %.2f to %s\r\n", temp, topic2);  	//print something to sanity check
        sprintf(buf, "%.2f\n", temp);			//write the reading to the buffer
        message.payloadlen = strlen(buf)+1;		//set the payload length
        rc = client.publish(topic2, message);        	//publish the message
        wait(5); 					//publish every 5 seconds
    }

    if ((rc = client.unsubscribe(topic)) != 0)
        logMessage("rc from unsubscribe was %d\r\n", rc);

    if ((rc = client.unsubscribe(topic2)) != 0)
        logMessage("rc from unsubscribe was %d\r\n", rc);

    if ((rc = client.disconnect()) != 0)
        logMessage("rc from disconnect was %d\r\n", rc);

    mqttNetwork.disconnect();

    logMessage("Version %.2f: finish %d msgs\r\n", version, arrivedcount);

    return 0;
}

void messageArrived(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;
    logMessage("Message arrived: qos %d, retained %d, dup %d, packetid %d\r\n", message.qos, message.retained, message.dup, message.id);
    logMessage("Payload %.*s\r\n", message.payloadlen, (char*)message.payload);
    ++arrivedcount;
}

