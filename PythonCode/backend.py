from SmartWasteManagment import smartwastemanagment
import time
import threading

# ------------------------------
# Create devices
# ------------------------------
device_list = []
device_list.append(threading.Thread(target=smartwastemanagment, args=[ 'SmartWasteManagement',  '49333234002e0020', 0, [0,0] ]).start())

# ------------------------------
# Loop
# ------------------------------
while True:
    time.sleep(60)
