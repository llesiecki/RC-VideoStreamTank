# RC-VideoStreamTank
Remote Controlled (RC) tank fitted with a live camera.  
![RC-VideoStreamTank-gif](https://user-images.githubusercontent.com/37122127/128909247-972e796e-6ff7-4d2b-91a5-fe6f40e31f98.gif)  
<img src="https://user-images.githubusercontent.com/37122127/119276177-ac8c1d00-bc19-11eb-803a-0365740db262.jpg" width="854" height="480">  
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

# Color recognition and tracking
The ```color_recognition.cpp``` C++ program provides some simple, but effective color recognition and tracking.  
The RGB camera output is converted into HSV color space, so the target color can be easily identified.  
After color filtering, the output is eroded to dismiss some smaller objects and noise. The final step  
is to calculate the position of the target, basing on the pixels left. These three pictures illustrate the  
above steps in practise:

* Raw RGB data:
![raw](https://user-images.githubusercontent.com/37122127/146478195-3b1622f2-94b3-4d29-af68-66000e959b1c.png)

* Filtered red color:
![red](https://user-images.githubusercontent.com/37122127/146478307-39b1ac0c-a65a-4110-b3e2-f78d9bc24474.png)

* Eroded red and the calculated target position marked as a green square:
![target](https://user-images.githubusercontent.com/37122127/146478496-50136aa5-e221-4f52-bf69-afc09130f5a7.png)


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
## The Python server
* Language: [Python](https://www.python.org/) (3)
* Camera API: [picamera](https://pypi.org/project/picamera/) Python module
* GPIO Control: [RPi.GPIO](https://pypi.org/project/RPi.GPIO/) Python module
## The C++ clolor recognition
* Language: [C++](https://www.cplusplus.com/) 11
* Camera API: [raspicam](https://github.com/cedricve/raspicam) C++ lib
* GPIO Control: [wiringPi](http://wiringpi.com/) C++ lib
