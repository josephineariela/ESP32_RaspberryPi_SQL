import paho.mqtt.client as mqtt , os, urlparse
import time
import subprocess

mqtt_username = str('hhaqsitb')
mqtt_server = str('tailor.cloudmqtt.com')
mqtt_port = str('14131')
mqtt_password = str('MP7TFv0i040Q')

ActTemp = float(27.45)
ActLight = float(600.00)
ActHum = float(60.25)
ActMoist = float(72.35)

OptTemp = float()
OptHum = float()
OptMoist = float()
heater_status = str()
cooler_status = str()
humidifier_status = str()
dehumidifier_status = str()
LED_status = str()
pump_status = str()

lOptTemp = []
lOptHum = []
lOptMoist = []
lHeater = []
lCooler = []
lHumidifier = []
lDehumidifier = []
lPump = []
lLED = []

# Define event callbacks
    
def on_connect(mosq, obj, rc):
    #print ("on_connect:: Connected with result code "+ str ( rc ) )
    #print("rc: " + str(rc))
    print("" )

# Subscribing a Topic
def on_message(mosq, obj, msg):
    # print ("New Message Subscribed : ")
    if msg.topic == "OptTemp":
        lOptTemp.append(msg.payload)
    elif msg.topic == "OptHum":
        lOptHum.append(msg.payload)
    elif msg.topic == "OptMoist":
        lOptMoist.append(msg.payload)
    elif msg.topic == "heater":
        lHeater.append(msg.payload)
    elif msg.topic == "cooler":
        lCooler.append(msg.payload)
    elif msg.topic == "humidifier":
        lHumidifier.append(msg.payload)
    elif msg.topic == "dehumidifier":
        lDehumidifier.append(msg.payload)
    elif msg.topic == "light":
        lLED.append(msg.payload)
    elif msg.topic == "moisture":
        lPump.append(msg.payload)
    


def on_publish(mosq, obj, mid):
    #print("mid: " + str(mid))
    # print ("")
    pass

def on_subscribe(mosq, obj, mid, granted_qos):
    #print("This means broker has acknowledged my subscribe request")
    # print("Subscribed: " + str(mid) + " " + str(granted_qos))
    pass

def on_log(mosq, obj, level, string):
    # print(  string)
    pass



client = mqtt.Client()
# Assign event callbacks
client.on_message = on_message
client.on_connect = on_connect
client.on_publish = on_publish
client.on_subscribe = on_subscribe

# Uncomment to enable debug messages
client.on_log = on_log


# user name has to be called before connect - my notes.
client.username_pw_set(mqtt_username, mqtt_password)
client.connect(mqtt_server, mqtt_port, 60)

client.loop_start()

client.subscribe ("OptTemp" ,0 )

client.subscribe ("OptHum" ,0 )
client.subscribe ("OptMoist" ,0 )

client.subscribe ("heater" ,0 )
client.subscribe ("cooler" ,0 )
client.subscribe ("humidifier" ,0 )
client.subscribe ("dehumidifier" ,0 )
client.subscribe ("light" ,0 )
client.subscribe ("moisture" ,0 )


run = True
while run:
    #client.on_message = on_message
    
    client.publish  ( "ActTemp", ActTemp )
    client.publish ( "ActHum", ActHum)
    client.publish ( "ActMoist", ActMoist)
    client.publish ( "ActLight", ActLight)
    time.sleep(2)


    result = subprocess.Popen(["php /var/www/html/SQL_toESP-A.php"], shell=True, stdout=subprocess.PIPE)

    script_response = result.stdout.read()
    print(script_response)
    
    
    # print("All Status = ")
    # print ("OptTemp = ")
    for i in [len(lOptTemp)-1]:
        OptTemp = lOptTemp[i]
        print(OptTemp + "x")
    # print ("OptHum = ") 
    for i in [len(lOptHum)-1]:
        OptHum = lOptHum[i]
        print(OptHum + "y")
    # print ("OptMoist = ") 
    for i in [len(lOptMoist)-1]:
        OptMoist = lOptMoist[i]
        print(OptMoist + "z")
    time.sleep(20)

    '''
    print ("ActTemp = ") 
    print (ActTemp)
    print ("ActHum = ") 
    print (ActHum)
    print ("ActMoist = ") 
    print (ActMoist)
    print ("ActLight = ") 
    print (ActLight)
    '''
    # print ("Heater Status = ")
    # for i in [len(lHeater)-1]:
    #     heater_status = lHeater[i]
    #     print(heater_status)
    # print ("Cooler Status = ")
    # for i in [len(lCooler)-1]:
    #     cooler_status = lCooler[i]
    #     print(cooler_status)
    # print ("Humidifier Status = ")
    # for i in [len(lHumidifier)-1]:
    #     humidifier_status = lHumidifier[i]
    #     print(humidifier_status)
    # print ("Dehumidifier Status = ")
    # for i in [len(lDehumidifier)-1]:
    #     dehumidifier_status = lDehumidifier[i]
    #     print(dehumidifier_status)
    # print ("Light Status = ")
    # for i in [len(lLED)-1]:
    #     LED_status = lLED[i]
    #     print(LED_status)
    # print ("Pump Status = ")
    # for i in [len(lPump)-1]:
    #     pump_status = lPump[i]
    #     print(pump_status)
    
    # print (" ")
