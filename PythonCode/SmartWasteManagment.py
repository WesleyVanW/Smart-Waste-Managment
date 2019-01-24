# -*- coding: utf-8 -*-
from __future__ import print_function

import sys
sys.path.insert(0, 'C:\Users\wesley\Documents\UAntwerpen\Master\Semester1\IOT-Low Power Embedded Communication\Practicum\pyd7a')

import paho.mqtt.client as mqtt #import the client1 --> done by typing "pip install paho-mqtt" in terminal
import time
import json
import logging
import base64
import os
import threading

from thingsboard import Thingsboard

from bitstring import ConstBitStream

from d7a.alp.parser import Parser as AlpParser

from localization import Localization

logger = logging.getLogger(__name__)
############

class smartwastemanagment:

    # ------------------------------
    # Broker addresses
    # ------------------------------
    broker_address_lora = 'eu.thethings.network'
    broker_address_d7 = 'backend.idlab.uantwerpen.be'

    thingsboard = Thingsboard("thingsboard.idlab.uantwerpen.be", 1883, "ODvYloOuCa514CgP2ZaZ")

    def __init__(self, device_name, device_id, training_mode, location ):
        self.device_name = device_name
        self.device_id = device_id
        self.training_mode = training_mode
        self.x_training_location = location[0]
        self.y_training_location = location[1]
        self.localization = Localization( 'mongodb://localhost:27017/', 'FingerprintingDB', 'no_ack_test' )     # ( host, db, collection )
        self.queue_d7 = {}                      # empty queue for dash-7 deduplication and rssi values
        self.processor = threading.Thread()     # empty thread object
        # ------------------------------
        # Subscribe to Dash-7
        # ------------------------------
        try:
            self.client_d7 = mqtt.Client() #create new instance
            self.client_d7.on_message=self.on_message_D7 #attach function to callback
            # self.client_d7.on_connect=self.on_connect
            print('connecting to broker '+self.broker_address_d7)
            self.client_d7.connect(self.broker_address_d7) #connect to broker
            self.client_d7.loop_start()
            print("Subscribing to topic","/d7/#")
            self.client_d7.subscribe('/d7/#')
        except:
            print('connecting to Dash-7 backend failed')
        # ------------------------------
        # Subscribe to LoRaWAN
        # ------------------------------
        try:
            self.client_lora = mqtt.Client() #create new instance
            self.client_lora.username_pw_set("smartwastemanagment", password="ttn-account-v2.2ROmCDs8z--YiaooGq2DrRcsd6s-WCibvDUNvfTNkqo")
            self.client_lora.on_message=self.on_message_LoRa #attach function to callback
            print('connecting to broker '+self.broker_address_lora)
            self.client_lora.connect(self.broker_address_lora) #connect to broker
            self.client_lora.loop_start()
            print('Subscribing to topic", "+/devices/+/up')
            self.client_lora.subscribe("+/devices/+/up")
        except:

            print('connecting to The Things Network failed')

    def __del__(self):
        self.client_d7.loop_stop()
        self.client_lora.loop_stop()

    def on_message_D7(self,client, userdata, message):
        #parsing dash7 message using functions from pyd7a/tools/parse_hexstring
        hexstring = str(message.payload.decode("utf-8"))
        data = bytearray(hexstring.decode("hex"))
        parsedCommando = AlpParser().parse(ConstBitStream(data), len(data)) #debug this if you need information on parameters of d7a parsed commando

        if parsedCommando.actions[0].operand.offset.id == 86:
            hardware_id = message.topic.split('/')[2]
            gateway_id = message.topic.split('/')[3]
            self.queue_d7[gateway_id] = parsedCommando.interface_status.operand.interface_status.rx_level  # save rx_level for every receiving gateway
            print('queue', self.queue_d7)
            print('thread', self.processor.is_alive())
            if not self.processor.is_alive():
                print('Thread started')
                self.processor = threading.Thread(target=self.process_data_counter, args=[parsedCommando.actions[0], hardware_id])
                print('Thread created')
                self.processor.start()
                print('Thread started')
            print('-----------------------------------------------')
    ########################################

    def process_data_counter(self, data, device_id):
        time.sleep(1)
        print('-------------------- Dash-7 Process --------------------')
        print('queue',self.queue_d7)
        if self.training_mode:
            # -------------------------
            # Add Fingerprint to Database
            # -------------------------
            # x_training_location = input('X position >')
            # y_training_location = input('Y position >')
            self.localization.train( self.x_training_location, self.y_training_location, self.queue_d7 ) # add to training database ( x_value, y_value, rx_values )
            print('Entry added to database')

        # -------------------------
        # Localize
        # -------------------------
        location = self.localization.localize( self.queue_d7, 25 )  # get location based on fingerprinting (rx_values, k-nearest neighbors)
        print('Location is approximately x:'+str(location['x'])+' y:'+str(location['y']))


        print('')
        for y in range(0,3):
            print(str(y), end='')
            for x in range(0,6):
                if y == round(location['y']) and x == round(location['x']):
                    print(str(' X'), end='')
                else:
                    print(str(' •'), end='')
            print('')
        print('  0 1 2 3 4 5')
        print('')

        # -------------------------
        # Done
        # -------------------------
        self.queue_d7 = {}                   # clear queue
        if not self.training_mode:
            self.process_data(data, device_id, location)   # process data of first received packet
        print('--------------------------------------------------------')
    ############
    def on_message_LoRa(self,client, userdata, message):

        msg = json.loads(message.payload.decode("utf-8"))
        payloadmsg = msg["payload_fields"]


        self.process_data(payloadmsg, 'SmartWasteManagement', None)
        print('---------------------------------------------')

    ########################################

    def process_data(self, data, device_id, location):
        if location == None:
            payload = {'temperatuur': str(data['temperatuur']),
                       'x_axis': str(data['x_axis']),
                       'y_axis': str(data['y_axis']),
                       'z_axis': str(data['z_axis']),
                       'distance': str(data['distance']),
                       'latitude': str(data['latitude']),
                       'longitude': str(data['longitude']),
                       'level' : str(data['level']),
                       'device': 'SmartWasteManagement',
                       'metadata': {
                           'frequency': 868.1,
                           'data_rate': "SF7125",
                       }}

            print("-----Payload adjusted LoRa-----")
            device_id = payload['device']

            # send non-numeric data ('attributes') to Thingsboard as JSON. Example:
            tb_attributes = {'last_data_rate': str(payload['metadata']['data_rate'])}

            # send numeric data ('telemetry') to Thingsboard as JSON (only floats or integers!!!). Example:
            #tb_telemetry = {'temperatuur': float(payload['temperatuur']),
            #                'x_axis': float(payload['x_axis']),
            #                'y_axis': float(payload['y_axis']),
            #                'z_axis': float(payload['z_axis']),
            #                'distance': float(payload['distance']),
            #                'latitude': float(payload['latitude']),
            #                'longitude': float(payload['longitude']),
            #                'level' : int(payload['level']),
            #                'frequency': float(payload['metadata']['frequency'])
            #                }
            # send numeric data ('telemetry') to Thingsboard as JSON (only floats or integers!!!). Example:
            tb_telemetry = {'temperatuur': float(payload['temperatuur']),
                            'x_axis': float(payload['x_axis']),
                            'y_axis': float(payload['y_axis']),
                            'z_axis': float(payload['z_axis']),
                            'distance': float(payload['distance']),
                            'latitude': float(payload['latitude']),
                            'longitude': float(payload['longitude']),
                            'level': float(payload['level']),
                            'frequency': float(payload['metadata']['frequency'])}
        else :
            payload = {'temperatuur': data.operand.data[0] + data.operand.data[1] / 100.0,
                        'x_axis': data.operand.data[2],
                        'y_axis':data.operand.data[3],
                        'z_axis': data.operand.data[4],
                        'distance': data.operand.data[5],
                        'x': location['x'],
                        'y': location['y'],
                        'level' : 100-(data.operand.data[5]*10)/53,
                        'device': 'SmartWasteManagement',
                        'metadata': {
                            'frequency': 868.1,
                            'data_rate': "SF7125",
                        }}
            print("-----Payload adjusted D7-----")
            device_id = payload['device']
            # send non-numeric data ('attributes') to Thingsboard as JSON. Example:
            tb_attributes = {'last_data_rate': str(payload['metadata']['data_rate'])}

            # send numeric data ('telemetry') to Thingsboard as JSON (only floats or integers!!!). Example:
            tb_telemetry = {'temperatuur': float(payload['temperatuur']),
                            'x_axis': float(payload['x_axis']),
                            'y_axis': float(payload['y_axis']),
                            'z_axis': float(payload['z_axis']),
                            'distance': float(payload['distance']),
                            'x': float(payload['x']),
                            'y': float(payload['y']),
                            'level' : int(payload['level']),
                            'frequency': float(payload['metadata']['frequency'])
                            }

        current_ts_ms = int(round(time.time() * 1000))   # current timestamp in milliseconds, needed for Thingsboard
        self.thingsboard.sendDeviceAttributes(device_id, tb_attributes)
        self.thingsboard.sendDeviceTelemetry(device_id, current_ts_ms, tb_telemetry)
        print("-----Data sent to TB-----")
