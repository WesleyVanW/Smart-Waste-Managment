# SmartWasteManagment

SmartWasteManagment is a device which tracks a regular garbage bin indoor aswell as outdoor. The device is triggered when opening the lid and will send valuable data such as inside temperature, capacity and location to either LoRa or Dash7 gateways. This data will be able to be processed by a backend python script to further implement the visualization in ThingsBoard. 

## Information

Project name: SmartWasteManagment 

Course: I-IoT Low Power Embedded Communication 

Professor + Assistents: Maarten Weyn, Michiel Aernouts, Mats De Meyer 

Contributors: Gregory Gonzalez Lopez, Mouhcine Oulad Ali, Wesley Van Wijnsberghe

### What you need

Vagrant 

Riot OS : https://github.com/RIOT-OS/RIOT

Dash7 Python Support: https://github.com/MOSAIC-LoPoW/pyd7a

Murata Modem Support: https://github.com/MOSAIC-LoPoW/riot-oss7-modem

Nucleo L496ZG 

Octa connect shield 

Octa connect Murata modem shield 

Octa connnect GPS shield 

Python 2.7

MongoDB Compass 

The Things Network Account 

ThingsBoard Account

### Visualization of project

--- add picture ---

## Implementation

1. Copy contents to the correct location in the RIOT directory: 

  - Vagrantfile --> RIOT/
  
  - boards/octa --> RIOT/boards 
  
  - drivers/include --> RIOT/drivers/include 
  
  - drivers/ --> RIOT/drivers 
  
  - src/SmartWasteManagment --> RIOT/tests // other directories are tests for the drivers 
  
  - sys/auto_init/saul --> RIOT/sys/auto_init/saul
  
  - sys/auto_init --> RIOT/sys/auto_init
  
  - Copy the following scripts from PythonCode to a directory of your choosing: backend, localization, SmartWasteManagment

2. Import the contents of full_dataset.json in a MongoDB database called FingerprintingDB with a collection no_test_ack .

3. Open Command Prompt and navigate to the RIOT directory. 

4. Start vagrant in this directory (command: vagrant up) and connect to it (command: vagrant ssh).

5. Connect the ultrasonic sensor to the nucleo (see further).

6. Navigate to RIOT/test/SmartWasteManagment and program it on the octa board (command: make BOARD=octa flash) 

7. Open Python program of your choosing (we used PyCharm) and make sure you're using Python 2.7 

8. Install the Pymongo and numpy plugins. 

9. Run the backend.py script and make sure both localization.py, SmartWasteManagment.py and thingsboard.py are in the same directory

10. The script should now connect to both an D7 and LoRa broker assuming you are in range. 

11. The actions are monitorable in a serial monitor (Putty) when connected to the Octa board.


  

### Break down into end to end tests

Explain what these tests test and why

```
Give an example
```

### And coding style tests

Explain what these tests test and why

```
Give an example
```

## Deployment

Add additional notes about how to deploy this on a live system

## Built With

* [Dropwizard](http://www.dropwizard.io/1.0.2/docs/) - The web framework used
* [Maven](https://maven.apache.org/) - Dependency Management
* [ROME](https://rometools.github.io/rome/) - Used to generate RSS Feeds

## Contributing

Please read [CONTRIBUTING.md](https://gist.github.com/PurpleBooth/b24679402957c63ec426) for details on our code of conduct, and the process for submitting pull requests to us.

## Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://github.com/your/project/tags). 

## Authors

* **Billie Thompson** - *Initial work* - [PurpleBooth](https://github.com/PurpleBooth)

See also the list of [contributors](https://github.com/your/project/contributors) who participated in this project.

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

* Hat tip to anyone whose code was used
* Inspiration
* etc

