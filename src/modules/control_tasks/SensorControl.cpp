//
// Created by adiv413 on 4/24/2020.
//

#include <Logger/logger_util.h>
#include <flight/modules/control_tasks/SensorControl.hpp>
#include <flight/modules/lib/Enums.hpp>
#include <flight/modules/mcl/Config.hpp>
#include <chrono>

SensorControl::SensorControl() {
    this->last_send_time = 0;

    // config gives it in seconds, convert to milliseconds
    this->send_interval = global_config.sensors.send_interval * 1000;
    global_flag.log_info("response", {
        {"header", "info"},
        {"Description", "Sensor control started"}
    });
}

void SensorControl::begin() {

    log("Sensor control: Beginning");

    // Initialize the Kalman filters

    /* Pair of <string, <string, SensorInfo>> */
    for (const auto& type_ : global_config.sensors.list) {
        /* Pair of <string, SensorInfo> */
        for (const auto& location_ : type_.second) {
            ConfigSensorInfo sensor = location_.second;
            auto kalman = sensor.kalman_args;
            // Use brackets for the first because we want to create a new map
            // We use emplace for the second because Kalman has no default constructor
            kalman_filters[type_.first].emplace(location_.first, Kalman(
                kalman.process_variance,
                kalman.measurement_variance,
                kalman.kalman_value
            ));
        }
    }
}

void SensorControl::execute() {
    boundary_check();

    long now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    if(last_send_time == 0 || now > last_send_time + send_interval) {
        send_sensor_data();
        last_send_time = now;
    }
}

bool between(double a, double b, double x) {
    return a <= x && x <= b;
}

void SensorControl::boundary_check() {
    vector<pair<string, string>> critical_sensors;

    for (const auto& type_ : global_config.sensors.list) {
        string type = type_.first;
        for (const auto& location_ : type_.second) {
            string location = location_.first;
            ConfigSensorInfo conf = location_.second;

            RegistrySensorInfo &sensor_registry = global_registry.sensors[type][location];
            double value = sensor_registry.measured_value;
            // We use .at() here because Kalman has no default constructor
            double kalman_value = kalman_filters.at(type).at(location).update_kalman(value);
            sensor_registry.normalized_value = kalman_value; // reference
            Stage curr_stage = global_registry.general.stage;
            if(curr_stage == Stage::WAITING)
            {
                if (between(conf.boundaries.waiting.safe.lower, conf.boundaries.waiting.safe.upper, kalman_value)) {
                    sensor_registry.status = SensorStatus::SAFE;
                } else if (between(conf.boundaries.waiting.warn.lower, conf.boundaries.waiting.warn.upper, kalman_value)) {
                    sensor_registry.status = SensorStatus::WARNING;
                } else {
                    sensor_registry.status = SensorStatus::CRITICAL;
                    critical_sensors.push_back({type, location});
                }
            }
            else if(curr_stage == Stage::PRESSURIZATION)
            {
                if (between(conf.boundaries.pressurization.safe.lower, conf.boundaries.pressurization.safe.upper, kalman_value)) {
                    sensor_registry.status = SensorStatus::SAFE;
                } else if (between(conf.boundaries.pressurization.warn.lower, conf.boundaries.pressurization.warn.upper, kalman_value)) {
                    sensor_registry.status = SensorStatus::WARNING;
                } else {
                    sensor_registry.status = SensorStatus::CRITICAL;
                    critical_sensors.push_back({type, location});
                }
            }
            else if(curr_stage == Stage::AUTOSEQUENCE)
            {
                if (between(conf.boundaries.autosequence.safe.lower, conf.boundaries.autosequence.safe.upper, kalman_value)) {
                    sensor_registry.status = SensorStatus::SAFE;
                } else if (between(conf.boundaries.autosequence.warn.lower, conf.boundaries.autosequence.warn.upper, kalman_value)) {
                    sensor_registry.status = SensorStatus::WARNING;
                } else {
                    sensor_registry.status = SensorStatus::CRITICAL;
                    critical_sensors.push_back({type, location});
                }
            }
            else if(curr_stage == Stage::POSTBURN)
            {
                if (between(conf.boundaries.postburn.safe.lower, conf.boundaries.postburn.safe.upper, kalman_value)) {
                    sensor_registry.status = SensorStatus::SAFE;
                } else if (between(conf.boundaries.postburn.warn.lower, conf.boundaries.postburn.warn.upper, kalman_value)) {
                    sensor_registry.status = SensorStatus::WARNING;
                } else {
                    sensor_registry.status = SensorStatus::CRITICAL;
                    critical_sensors.push_back({type, location});
                }
            }
        }
    }

    if (!global_registry.general.soft_abort and critical_sensors.empty()) { // one or more of the sensors are critical, soft abort
        global_registry.general.soft_abort = true;

        string message = "Soft aborting because the following sensors have reached critical levels- ";
        for(const pair<string, string>& sensor_location : critical_sensors) {
            message += sensor_location.first + "." + sensor_location.second + ", ";
        }
        message = message.substr(0, message.length() - 2);

        global_flag.log_critical("response", {
            {"header", "info"},
            {"Description", message}
        });
    }
}

void SensorControl::send_sensor_data() {
    json sensor_data_json = json::object();

    for (const auto& type_pair : global_config.sensors.list) {
        string type = type_pair.first;
        for (const auto &location_pair : type_pair.second) {
            string location = location_pair.first;
            RegistrySensorInfo sensor = global_registry.sensors[type][location];

            // "type.location": {
            sensor_data_json[type][location] = json{
                {"measured", sensor.measured_value},
                {"kalman", sensor.normalized_value},
                {"status", int(sensor.status)}
            };
        }
    }

    global_flag.log_info("sensor_data", json{{"sensors", sensor_data_json}});
}

