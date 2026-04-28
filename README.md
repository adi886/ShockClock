# ShockClock
How V1 works. (Without data, website and set up alarm features.)
- Read in needed libraries 
- Setup Wifi Connection
- Setup LCD 
- Setup pins
- Start an alarm when light sensor detects low enough light levels (can test by covering) and the time alarm set goes off
- The display will output a random 3 digit value which the user must match by turning the pentiometer to increase or decrease a shown value on the screen which the user must match to the random 3 digit given value
- When the user has matched the value to the given random value the display will show that the alarm is disabled
- Setup wifi connectivity 


How V2 Works and how it changes:
- Once you turn on the arduino, launch the website and connect the arduino tho the internet and ip address. 
- On the website you can set the alarm clock for when it goes off and test the timer.
- The arduino is connected to the live time.
- Once the alarm goes off, you can see the brightness level, the password generated and the password inputed.
- You can also turn off the alarm from here just incase there's an error or something breaks.


How V3 Works and what it includes:
-
ActionURL Arm the alarmhttp://192.168.1.45/arm
Disarm the alarmhttp://192.168.1.45/disarm
Manually trigger alarmhttp://192.168.1.45/triggerForce 
disable alarmhttp://192.168.1.45/disableCheck 
current statushttp://192.168.1.45/status
