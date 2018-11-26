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
def on_message(client, userdata, message):
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
        payload = {'hexstring' : hexstring,
                   'parsedCommando' : parsedCommando,
                   'temperatuur' : parsedCommando.actions[0].operand.data[0] + parsedCommando.actions[0].operand.data[1]/100,
                   'device': 'test_device_Wesley_D7',
                   'payload': "RoyalAntwerp1880",
                   'latitude': 51.508213,
                   'longitude': -0.075917,
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
        tb_telemetry = {'hexstring': str(payload['hexstring']),
                        'parsedCommando': str(payload['parsedCommando']),
                        'temperatuur': float(payload['temperatuur']),
                        'frequency': float(payload['metadata']['frequency']),
                        'latitude': float(payload['latitude']),
                        'longitude': float(payload['longitude'])}
        tb.sendDeviceTelemetry(device_id, current_ts_ms, tb_telemetry)

########################################
# Create global logger
logger = logging.getLogger('tb_example')
formatstring = "%(asctime)s - %(name)s:%(funcName)s:%(lineno)i - %(levelname)s - %(message)s"
logging.basicConfig(format=formatstring, level=logging.DEBUG)
tb = Thingsboard("thingsboard.idlab.uantwerpen.be", 1883, "ODvYloOuCa514CgP2ZaZ") #access token

broker_address="backend.idlab.uantwerpen.be"

print("creating new instance")
client = mqtt.Client("W10") #create new instance, standard was P1 --> changed
client.on_message=on_message #attach function to callback
print("connecting to broker")

client.connect(broker_address) #connect to broker
client.loop_start() #start the loop
print("Subscribing to topic","/d7/#")
client.subscribe("/d7/#")

time.sleep(600) # wait long enough to read data from board
client.loop_stop() #stop the loop