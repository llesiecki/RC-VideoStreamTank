# RC-VideoStreamTank
Remote Controlled (RC) tank fitted with a live camera.
![RC-VideoStreamTank](https://user-images.githubusercontent.com/37122127/119276177-ac8c1d00-bc19-11eb-803a-0365740db262.jpg)
The whole thing is controlled by a RasperryPi Zero W (Wi-Fi)  
It connects to the home network and exposes an HTTP server.  
The number of clients is unlimited.  
> **_NOTE:_** It actually is by the WiFi bandwidth - it's getting laggy with > 5 clients

# Usage
After connecting to **http://\<RaspberryPi address\>:8000/index.html** we can see a livestream from the onboard camera.  
This webpage also allows us to control the tank:  
* Movement: W, S, A, D  
* Speed control:  
  * Increase: +, =  
  * Decrease: -  
* Camera angle control: Arrow-up, Arrow-down  

# Elements used
* Computing unit: RasperryPi Zero W  
* Caterpillar chassis: RP6 Robot System  
* Motor control: RPi GPIO in PWM mode + H-Bridge  
* Power supply:  
  * Main: Laptop Battery  
  * 5V for RPi: Step-Down Converter  
  * H-Bridge powering: Directly from the battery  
  * Battery charging: CC/CV Step-Down Converter + laptop charger  

# Software side
* OS: [DietPi](https://dietpi.com/)
* Language: [Python](https://www.python.org/) (3)
* Camera API: [picamera](https://pypi.org/project/picamera/) Python module
* GPIO Control: [RPi.GPIO](https://pypi.org/project/RPi.GPIO/) Python module
