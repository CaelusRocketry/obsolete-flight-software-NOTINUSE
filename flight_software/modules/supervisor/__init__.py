# /modules/supervisor

from threading import Thread
import time
import heapq
from ..telemetry.telemetry import Telemetry
from ..telemetry.logging import Packet, Log
from ..telemetry.encryption import decrypt
from ..sensors.thermocouple import Thermocouple
from ..sensors.imu import IMU
from ..sensors.force import Load
from ..valve import ValveType, Valve
from . import ingest

GS_IP = '192.168.1.75' 
GS_PORT = 5005
SENSOR_DELAY = 1.5

# Initialize all sensors
sensors = [
    Thermocouple(0, 0, "chamber"),
    IMU("nose"),
    Load(6, 5, "fuel")
]

# Initialize all valves
valves = [
    Valve(0, ValveType.Ball, 4, 17)
]

# Initializes anything in the telemetry object and ingests it
def handle_telem(telem):
    telem.begin()  # Starts a sending and recieving thread, with associated queue_ingest and queue_send methods
    while True: 
        if len(telem.queue_ingest) > 0:  # if there is a message, begin a thread to interperate the message
            data = telem.queue_ingest.popleft()
            ingest_thread = Thread(target=interpret, args=(telem, data))
            ingest_thread.daemon = True
            ingest_thread.start()

# Handles the information returned by ingest
def interpret(telem, data):
    pck_str = decrypt(data)
    pck = Packet.from_string(pck_str)  # Deserialize the packet
    pack = Packet(header='RESPONSE')
    for log in pck.logs:
        response = ingest(log, sensors, valves)  # Performs the action requested and returns a response
        if response.header == "Error":
            print("Error", response.message)
        pack.add(response)

    telem.enqueue(pack)
    return

#  The main supervisor loop. An infinite loop that collects data from all sensors and sends it to ground station.
def start():
    # Initalize telemetry object
    telem = Telemetry(GS_IP, GS_PORT)
    telem_thread = Thread(target=handle_telem, args=(telem,))
    telem_thread.daemon = True
    telem_thread.start()

    # Begin the checking method for all sensors
    for sensor in sensors:
        sensor_thread = Thread(target=sensor.check)
        sensor_thread.daemon = True
        sensor_thread.start()     

    # Begin the loop that collects sensor data, synthesizes it into a packet, and sends it to ground stations
    while True:
        # The packet stores data from all sensors for this iteration
        packet = Packet(
            header='DATA',
            logs=[],
            timestamp=time.time()
            )

        for sensor in sensors:
            # TODO: Handle sensor status by doing something
            status = sensor.status()

            # Log is the current sensor's data
            sensor_data = {"raw":sensor.data, "normalized":sensor.normalized}
            log = Log(
                message=sensor_data,
                level=status,
                timestamp=sensor.timestamp,
                sender=sensor.name())

            packet.add(log)
    

        telem.enqueue(packet)
        time.sleep(SENSOR_DELAY)
