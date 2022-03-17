# -------- standard -------------
import time
from time import sleep
import datetime
import pytz
tz = pytz.timezone('Europe/Sofia')

import os
# --------- camera --------------
from picamera import PiCamera
#camera = PiCamera(resolution='1920x1080')
camera = PiCamera()
camera.rotation = 180
rec_i = 0
security_events = 0

# ----------- GPIO --------------
import RPi.GPIO as GPIO
GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)
GPIO.setup(21, GPIO.OUT)
GPIO.setup(20, GPIO.OUT)
GPIO.setup(16, GPIO.OUT)

GPIO.output(21, GPIO.HIGH)
GPIO.output(20, GPIO.LOW)
GPIO.output(16, GPIO.LOW)

# -------- sensors --------------
import bme680
import mh_z19
import glob
import math
import json

base_dir = '/sys/bus/w1/devices/'
device_folder = glob.glob(base_dir + '28*')[0]
device_file = device_folder + '/w1_slave'

sensor = bme680.BME680(0x77)
sensor.set_humidity_oversample(bme680.OS_2X)
sensor.set_pressure_oversample(bme680.OS_4X)
sensor.set_temperature_oversample(bme680.OS_8X)
sensor.set_filter(bme680.FILTER_SIZE_3)
sensor.set_gas_status(bme680.ENABLE_GAS_MEAS)
sensor.set_gas_heater_temperature(320)
sensor.set_gas_heater_duration(150)
sensor.select_gas_heater_profile(0)

def readCO2():
    try:
        x = mh_z19.read()
        sleep(0.2)
        return x
    except Exception as e:
        print(e)
        x = {"co2": 0}
        return x

def readBme():
    t = 0
    h = 0
    p = 0
    g = 0
    if sensor.get_sensor_data():
        t = round(sensor.data.temperature, 1)
        h = round(sensor.data.humidity)
        p = round(sensor.data.pressure)
        if sensor.data.heat_stable:
            #print(sensor.data.gas_resistance)
            #g = math.log(sensor.data.gas_resistance) + 0.04 * h
            g = round(sensor.data.gas_resistance)
        #print(f"t={t} | h={h} | p={p} | g={g}")
    else:
        print("bme can't obtain data.")
    return t, h, p, g

def read_temp_raw():
    f = open(device_file, 'r')
    lines = f.readlines()
    f.close()
    return lines
def readTout():
    lines = read_temp_raw()
    while lines == [] or lines[0].strip()[-3:] != 'YES':
        time.sleep(0.2)
        lines = read_temp_raw()
    equals_pos = lines[1].find('t=')
    if equals_pos != -1:
        temp_string = lines[1][equals_pos + 2:]
        temp_c = float(temp_string) / 1000.0
        temp_c = temp_c - 2.0
        temp_f = temp_c * 9.0 / 5.0 + 32.0
        return round(temp_c, 1)
    else:
        return 0

def updateReadings(logit=False):
    dtnow = datetime.datetime.now(tz).strftime("%d-%b-%Y %H:%M:%S")
    #all val combines all the sensor readings.
    #on the end of this function I send the allval_string to EMMA (without timestamp).
    # EMMA collets the other sensor value too and save it with its own stamp
    #and save it to a txt file, adding the dtnow in the beginning.
    tw, hw, pw, gw = readBme() #reading the bme sensor
    sleep(0.2)
    to = readTout() #reading the outside temp sensor.
    sleep(0.1)
    co2w = readCO2()
    try:
        pubstr = json.dumps({"time": dtnow, "to": to, "tw": tw, "hw": hw, "pw": pw, "gw": gw, "co2w": co2w["co2"]}) #preparing data str for export to EMMA
        client.publish("sensors/tblog", pubstr)  # publishing the table sensor values to EMMA
        # topic is tblog (for table log), to be devided from lablog and "grdlog" (laboratory sensors and garden sensors)
        # print(f"Published: {pubstr}")
        if logit:
            file_name = "/home/pi/workshop/snzlog/" + datetime.datetime.now(tz).strftime("%d-%b-%Y") + ".txt"
            # save to local file:
            logfile = open(file_name, "a")
            logfile.write(pubstr + "\n")
            logfile.close()
            # print(f"Log file: {file_name}")
            # print(f"Saved: {pubstr}")
    except Exception as e:
        print("an error ocured while structuring the json, publish it and save it...")
        print(e)

# ------------- MQTT -------------
import paho.mqtt.client as mqtt
broker_adr = "192.168.0.173"
table_status = 0

def on_connect(client, userdata, flags, rc):
    print(f"Connected with result code {rc}")

    #client.publish("mainer", "workshop/snz")
    client.subscribe("table/check") #used only for check if table is online.

    client.subscribe("security/event")
    #table and EMMA listen for events. Table runs the camera to record. Emma only inform the user.
    #if workshop is locked, but emma is interacted, emma sends to security/event "2" telling the camera to take a shot for logging.
    #they are saved in folder "security/lockedEmma

    client.subscribe("table/cmd")
    #1: all on | 0: all off
    #lbon: lightboard ON | lboff : lightboard OFF
    #redon: redlight ON | redoff: redlight OFF | redfade: redlight fade in-out
    #tlon: table under light ON | tloff: table under light OFF
    #recs: give me the records from the last time asked. . Rewrite the last time asked (session). // I count the number of events for the session
    client.subscribe("sensors/cmd")
    #tw: give me the workshop temperature | to: give me the outside temp
    #hw: give me the workshop humidity | pw: give me the workshop air pressure

    #!!== ALL sensors are logged in a file every 60 seconds. They are also sent over "sensors/data" topic in one line,
    #and logged in every machine that currently listen. EMMA will use this info to detect events like storm income.
    #client.subscribe("id/check")
    #client.subscribe("sensors/tblog")

    client.publish("table/check", "0")
    #client.publish("id/check", "-") #ping to see if ID is unlocked and turn underlights on.

def on_disconnect(client, userdata, rc):
    print("Disconnect, reason: " + str(rc))
    #print("Disconnect, reason: " + str(client))

def on_message(client, userdata, msg):
    global table_status
    global security_events
    #print(msg.topic + " " + str(msg.payload, "utf-8"))
    t = msg.topic
    x = str(msg.payload, "utf-8")
    #print(f"Received message --> {x}")
    if msg.topic == "table/check" and x == "-":
        client.publish("table/check", str(table_status))

        lb_status = GPIO.input(16)
        tl_status = GPIO.input(20)
        rl_status = GPIO.input(21)
        client.publish("table/lb_state", str(lb_status))
        client.publish("table/tl_state", str(tl_status))
        client.publish("table/rl_state", str(rl_status))

    elif msg.topic == "table/cmd":
        if x == "0":
            print("The Table is going OFF.")
            GPIO.output(21, GPIO.HIGH)
            GPIO.output(20, GPIO.LOW)
            sleep(0.3)
            GPIO.output(16, GPIO.LOW)
            table_status = 0
        elif x == "1":
            print("The Table is going ON.")
            GPIO.output(21, GPIO.LOW)

            GPIO.output(16, GPIO.HIGH)
            sleep(0.02)
            GPIO.output(16, GPIO.LOW)
            sleep(0.05)
            GPIO.output(16, GPIO.HIGH)
            sleep(0.05)
            GPIO.output(16, GPIO.LOW)
            sleep(0.1)
            GPIO.output(20, GPIO.HIGH)
            sleep(0.2)
            GPIO.output(16, GPIO.HIGH)

            table_status = 1
        elif x == "lbon":
            GPIO.output(16, GPIO.HIGH)
        elif x == "lboff":
            GPIO.output(16, GPIO.LOW)
        elif x == "tlon":
            GPIO.output(20, GPIO.HIGH)
        elif x == "tloff":
            GPIO.output(20, GPIO.LOW)
        elif x == "redon":
            GPIO.output(21, GPIO.HIGH)
        elif x == "redoff":
            GPIO.output(21, GPIO.LOW)

        elif x == "recs": #how many security events was from the last time I asked? and zero the counter.
            client.publish("security/recs", str(security_events))
            security_events = 0

    elif msg.topic == "sensors/cmd":
        if x == "reload":
            updateReadings()

    elif msg.topic == "security/event":
        if x == "1":
            print("Warning! Unauthorized event registered!")
            now = datetime.datetime.now(tz)

            date1 = str(now.year) + "-" + str(now.month) + "-" + str(now.day)
            tm1 = str(now.hour) + "-" + str(now.minute) + "-" + str(now.second)
            #print(date1)
            #print(tm1)
            path = "/home/pi/workshop/security/"+date1
            isExist = os.path.exists(path)
            if not isExist:
                os.makedirs(path)

            fl1 = path+"/"+tm1+".jpg"
            fl2 = path+"/"+tm1+".h264"

            #camera.awb_mode = 'tungsten'
            #camera.exposure_mode = 'beach'

            camera.capture(fl1)
            camera.start_recording(fl2)
            #camera.capture("/home/pi/workshop/security/shoot%s.jpg" % rec_i)
            #camera.start_recording("/home/pi/workshop/security/event%s.h264" % rec_i)
            for i in range(15):
                GPIO.output(21, GPIO.LOW)
                sleep(0.25)
                GPIO.output(21, GPIO.HIGH)
                sleep(0.25)
            #sleep(5)
            camera.stop_recording()
            #rec_i+=1
            security_events+=1

client = mqtt.Client("mainer")
client.on_connect = on_connect
client.on_disconnect = on_disconnect
client.on_message = on_message
client.connect(broker_adr, 1883)
client.loop_start()
#client.loop_forever()

while True:
    updateReadings(logit=True)
    sleep(60)

