#include <Logger/logger_util.h>
#include <flight/modules/control_tasks/PressureControl.hpp>
#include <vector>
#include <flight/modules/mcl/Config.hpp>

PressureControl::PressureControl(){
    cout << "Pressure Control Initialized";
}

void PressureControl::begin() {
    log("Pressure control: Beginning");

    global_flag.log_info("response", {
            {"header", "info"},
            {"Description", "Pressure Control started"}
    });

    this->activate_stages = global_config.pressure_control.active_stages;
    this->valves = global_config.valves.list["solenoid"];
    this->sensors = global_config.sensors.list["pressure"];

    // if we're using the PT-2 sensor in this test

    if(global_registry.sensors["pressure"].find("PT-2") != global_registry.sensors["pressure"].end()) {
        this->matchups.push_back(make_pair("PT-2", "PRESSURE_RELIEF"));
    }

    for (pair<string , string> matched : this->matchups) {
        if(global_registry.sensors["pressure"].find(matched.first) == global_registry.sensors["pressure"].end()) {
            log("sensor at" + matched.first + "not registered");
        }

        if(global_registry.sensors["solenoid"].find(matched.first) == global_registry.sensors["solenoid"].end()) {
            log("pressure_relief_valve not registered");
        }
    }
}

void PressureControl::execute() {
    log("Pressure control: Controlling");
    check_pressure();
}

void PressureControl::check_pressure() {
    for(pair<string, string> matched : this->matchups) {

        if (global_registry.sensors["pressure"][matched.first].normalized_value >
                global_config.sensors.list["pressure"][matched.first].boundaries.pressurization.safe.upper) {
            if (global_registry.valves["solenoid"][matched.second].state == SolenoidState::CLOSED) {
                global_registry.valves["solenoid"][matched.second].actuation_type = ActuationType::OPEN_VENT;
                global_registry.valves["solenoid"][matched.second].actuation_priority = ValvePriority::PI_PRIORITY;
            }
        } else if (global_registry.sensors["pressure"][matched.first].status == SensorStatus::SAFE) {
            if (global_registry.valves["solenoid"][matched.second].state == SolenoidState::OPEN) {
               global_registry.valves["solenoid"][matched.second].actuation_type = ActuationType::OPEN_VENT;
               global_registry.valves["solenoid"][matched.second].actuation_priority = ValvePriority::PI_PRIORITY;
            }
        }
    }
}
