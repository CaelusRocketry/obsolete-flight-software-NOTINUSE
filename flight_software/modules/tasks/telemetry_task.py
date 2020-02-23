from modules.tasks.task import Task
from modules.mcl.registry import Registry
from modules.mcl.flag import Flag
from modules.drivers.telemetry_driver import Telemetry
from abc import ABC, abstractmethod

class TelemetryTask(Task):
    def __init__(self):
        self.telemetry = Telemetry()
        super().__init__("Telemetry", self.telemetry)

    def begin(self, config: dict):
        gs_ip = config['GS_IP']
        gs_port = config['GS_PORT']
        delay_listen = config['DELAY_LISTEN']
        self.telemetry.reset(gs_ip, gs_port, delay_listen)

    def read(self, state_field_registry: Registry, flag: Flag) -> Registry:
        telemetry_packets = self.telemetry.read(-1)
        telemetry_queue
#        print("Packets", telemetry_packets)
        for pack in telemetry_packets:
            err = state_field_registry.put("telemetry_queue", pack)
            if err is not None:
                print("REE GOT AN ERROR :(")
        return state_field_registry
    
    def actuate(self, state_field_registry, flag: Flag) -> (bool, Flag):
        return True, flag
