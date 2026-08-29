# smart-irrigation-field-monitoring-system  
An ESP32 based smart irrigation system that continuously monitors soil moisture, air temperature, humidity, and harmful gas levels, determines whether a field is currently suitable for cropping, and automatically switches a water pump ON/OFF through a relay based on real-time soil dryness with no manual intervention required

#Components  
ESP32 Dev Board  
Soil Moisture Sensor (analog)  
Potentiometer (10k)  
DHT11 Sensor  
MQ2 Gas Sensor  
5V Relay Module  
Water Pump   
External Power Supply  
Jumper Wires,Breadboard, USB Cable  

#Working  
Stage 1 : Startup (first 30 seconds)    
1.  Board powers on;Serial Monitor starts at 115200 baud;the relay is forced OFF for safety   
2.  Sensors are given time to stabilize  
3.  A countdown is shown on the Serial Monitor    
4.  After 30 seconds, the system prints "SYSTEM READY" and begins normal monitoring    

Stage 2 : Normal Monitoring (pump OFF)  
1.  Read soil moisture,potentiometer,temperature,humidity and gas level  
2.  The potentiometer can be used to change the threshold of turning the pump-ON. Map the potentiometer reading (0-4095)  onto 2400-4000 raw range and update the pump-ON threshold  
3.  Check land suitability in order: temperature 15–35°C ; humidity 30–85% ; soil at least 20% wet ; gas below 700  
4.  If the land is APT for cropping and the soil raw value is above 3000 (too dry) : turn the pump ON  
5.  If the land is NOT APT : keep the pump OFF and display the reason  
6.  Print the full field status to the Serial Monitor and repeat after 10 seconds  

Stage 3 : Pumping  
1.  Switch to continuous monitoring; only soil moisture is read (DHT11, MQ2, and potentiometer are skipped to save power and respond faster)  
2.  Print the live soil raw value and percentage every 1 second  
3.  When the soil raw value reaches 2000 or below, turn the pump OFF immediately and increment the watering counter, resume Stage 2  

#Demonstration video  

https://github.com/user-attachments/assets/1d4418bb-738f-492d-b07c-1767032dcaf0


