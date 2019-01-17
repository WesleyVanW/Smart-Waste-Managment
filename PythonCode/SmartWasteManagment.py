import sys
sys.path.insert(0, 'C:\Users\wesley\Documents\UAntwerpen\Master\Semester1\IOT-Low Power Embedded Communication\Practicum\pyd7a')

import paho.mqtt.client as mqtt #import the client1 --> done by typing "pip install paho-mqtt" in terminal
import time
import json
import logging

from thingsboard import Thingsboard

from bitstring import ConstBitStream

from d7a.alp.parser import Parser as AlpParser

############
def on_message_D7(client, userdata, message):
    print("onmessage D7")
    #print("message topic=", message.topic)
    #print("message received ", str(message.payload.decode("utf-8")))

    #parsing dash7 message using functions from pyd7a/tools/parse_hexstring
    hexstring = str(message.payload.decode("utf-8"))
    data = bytearray(hexstring.decode("hex"))
    parsedCommando = AlpParser().parse(ConstBitStream(data), len(data)) #debug this if you need information on parameters of d7a parsed commando
    print parsedCommando

    current_ts_ms = int(round(time.time() * 1000))  # current timestamp in milliseconds, needed for Thingsboard

    if parsedCommando.actions[0].operand.offset.id == 86:
        print("----adjusting payload----")
        payload = {'temperatuur' : parsedCommando.actions[0].operand.data[0] + parsedCommando.actions[0].operand.data[1]/100,
                   'x-axis' : parsedCommando.actions[2],
                   'y-axis' : parsedCommando.actions[3],
                    'z-axis' : parsedCommando.actions[4],
                   'distance' : parsedCommando.actions[5],
                   'device': 'SmartWasteManagement',
                   'metadata': {
                       'frequency': 868.1,
                       'data_rate': "SF7125",
                   }}
        print("-----Payload adjusted-----")
        device_id = payload['device']

        # send non-numeric data ('attributes') to Thingsboard as JSON. Example:
        tb_attributes = {'last_data_rate': str(payload['metadata']['data_rate'])}
        tb.sendDeviceAttributes(device_id, tb_attributes)

        # send numeric data ('telemetry') to Thingsboard as JSON (only floats or integers!!!). Example:
        tb_telemetry = {'temperatuur': float(payload['temperatuur']),
                        'x-axis': float(payload['x-axis']),
                        'y-axis': float(payload['y-axis']),
                        'z-axis': float(payload['z-axis']),
                        'distance': float(payload['distance']),
                        'frequency': float(payload['metadata']['frequency'])}
        tb.sendDeviceTelemetry(device_id, current_ts_ms, tb_telemetry)

########################################
############
def on_message_LoRa(client, userdata, message):
    print("onmessage LORA")
    #print("message topic=", message.topic)
    #print("message received ", str(message.payload.decode("utf-8")))

    msg = json.loads(message.payload.decode("utf-8"))
    #print("Temp: ", str(msg["payload_fields"]['temperature']))

    current_ts_ms = int(round(time.time() * 1000))  # current timestamp in milliseconds, needed for Thingsboard

    print("----adjusting payload----")
    payload = {'temperatuur': str(msg["payload_fields"]['temperature']
               }
    print("-----Payload adjusted-----")
    device_id = payload['device']

    # send non-numeric data ('attributes') to Thingsboard as JSON. Example:
    tb_attributes = {'last_data_rate': str(payload['metadata']['data_rate'])}
    tb.sendDeviceAttributes(device_id, tb_attributes)

    # send numeric data ('telemetry') to Thingsboard as JSON (only floats or integers!!!). Example:
    tb_telemetry = {'temperatuur': float(payload['temperatuur']),
                    'x-axis': float(payload['x-axis']),
                    'y-axis': float(payload['y-axis']),
                    'z-axis': float(payload['z-axis']),
                    'distance': float(payload['distance']),
                    'frequency': float(payload['metadata']['frequency'])}
    tb.sendDeviceTelemetry(device_id, current_ts_ms, tb_telemetry)

########################################
# Create global logger
logger = logging.getLogger('tb_example')
formatstring = "%(asctime)s - %(name)s:%(funcName)s:%(lineno)i - %(levelname)s - %(message)s"
logging.basicConfig(format=formatstring, level=logging.DEBUG)
tb = Thingsboard("thingsboard.idlab.uantwerpen.be", 1883, "ODvYloOuCa514CgP2ZaZ") #access token

broker_address="backend.idlab.uantwerpen.be"
broker_address2="eu.thethings.network"

print("creating new instance")
clientD7 = mqtt.Client("W10") #create new instance, standard was P1 --> changed
clientLoRa = mqtt.Client("W11")
clientD7.on_message=on_message_D7 #attach function to callback
clientLoRa.on_message=on_message_LoRa

print("connecting to broker")
clientD7.connect(broker_address) #connect to broker
clientLoRa.username_pw_set("smartwastemanagment", password="ttn-account-v2.2ROmCDs8z--YiaooGq2DrRcsd6s-WCibvDUNvfTNkqo")
clientLoRa.connect(broker_address2) #connect to broker

clientD7.loop_start()
clientLoRa.loop_start()
print("Subscribing to topic","/d7/#")
clientD7.subscribe("/d7/#")
print("Subscribing to topic", "+/devices/+/up")
clientLoRa.subscribe("+/devices/+/up")
time.sleep(600) # wait long enough to read data from board
clientLoRa.loop_stop() #stop the loop
clientD7.loop_stop() #stop the loop