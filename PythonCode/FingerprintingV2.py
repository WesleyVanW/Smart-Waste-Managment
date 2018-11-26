import sys
sys.path.insert(0, 'C:\Users\wesley\Documents\UAntwerpen\Master\Semester1\IOT-Low Power Embedded Communication\Practicum\pyd7a')

import paho.mqtt.client as mqtt #import the client1 --> done by typing "pip install paho-mqtt" in terminal
import time
import json

from bitstring import ConstBitStream

from d7a.alp.parser import Parser as AlpParser

from pymongo import MongoClient

gatewayList = []
strengthList = []
clearList = []
counters = 0
level = ""
############
def on_message(client, userdata, message):
    #print("message topic=", message.topic)
    #print("message received ", str(message.payload.decode("utf-8")))

    #parsing dash7 message using functions from pyd7a/tools/parse_hexstring
    hexstring = str(message.payload.decode("utf-8"))
    data = bytearray(hexstring.decode("hex"))
    parsedCommando = AlpParser().parse(ConstBitStream(data), len(data)) #debug this if you need information on parameters of d7a parsed commando
    #print parsedCommando


    if parsedCommando.actions[0].operand.offset.id == 86:
        global counters
        counters += 1
        print '\nCounter: ', counters
        a, b, c, gateway_id = message.topic.split("/")
        print 'Gateway id: ', gateway_id[-4:]
        sLevel = parsedCommando.interface_status.operand.interface_status.rx_level
        print 'Level: ', sLevel
        if gateway_id  not in gatewayList:
            print 'Adding gateway and RSS to correct sets...'
            gatewayList.append(gateway_id)
            strengthList.append(sLevel)
        if len(gatewayList) == 4:
            print '\nSet is full, sending data to database...'
            positionX = input("Geef de x coordinaat: ")
            positionY = input('Geef de y coordinaat: ')
            i = 0
            while i < len(gatewayList):
                gateway_id = gatewayList[i]
                sLevel = strengthList[i]
                i=i+1
                print i
                if database.RSS.find({"$and": [{"x": positionX}, {"y": positionY}]}).count() == 1:
                    if database.RSS.find({"$and": [{"x": positionX}, {"y": positionY},
                                                   {"gateways.gateway_id": {'$exists': True}}]}).count() == 0:
                        print "new gateway"
                        database.RSS.update({"$and": [{"x": positionX}, {"y": positionY}]},
                                            {"$push": {'gateways': {str(gateway_id): [sLevel]}}})
                        print "test"
                    else:
                        print "update gateway"
                        database.RSS.update({"$and": [{"x": positionX}, {"y": positionY},
                                                      {'gateways.gateway_id': [sLevel]}]})
                else:
                    jsonData = {
                        'gateways': {
                            str(gateway_id): [sLevel]
                        },
                        'x': positionX,
                        'y': positionY}
                    # Insert data object directly into MongoDB via insert_one
                    database.RSS.insert_one(jsonData)  # Collection name
            gatewayList[:] = []
            strengthList[:] = []
            print 'Sets are cleared'
            i=0




########################################

broker_address="backend.idlab.uantwerpen.be"

print("creating new instance")
client = mqtt.Client("W10") #create new instance, standard was P1 --> changed
client.on_message=on_message #attach function to callback

print("creating database")
databaseClient = MongoClient();
database = databaseClient["FingerprintingDB"]
collection = database["RSS"]

print("connecting to broker")
client.connect(broker_address) #connect to broker

client.loop_start() #start the loop
print("Subscribing to topic","/d7/#")
client.subscribe("/d7/#")

time.sleep(6000) # wait long enough to read data from board
client.loop_stop() #stop the loop