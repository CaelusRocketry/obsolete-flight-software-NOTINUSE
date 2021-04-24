#ifndef FLIGHT_LOG_HPP
#define FLIGHT_LOG_HPP

#include <string>
#include <vector>
#include <ArduinoJson.h>
#include <flight/modules/lib/Util.hpp>
#include <flight/modules/lib/logger_util.hpp>
#include <SD.h>

using namespace std;
using ArduinoJson::StaticJsonDocument;

// class Log;

// Log class stores messages to be sent to and from ground and flight station
class Log {
private:
    string header;
    string message;
    long double timestamp;

public:
    Log() = default;

    Log(const string& header, const string& message, long double timestamp, bool save = true)
        : header(header),
          message(message),
          timestamp(timestamp) {
        // print("Constructor Log details:");
        // print(getHeader());
        // print(getMessage());
        // string msg;
        // ArduinoJson::serializeJson(getMessage(), msg);
        // print(msg);
        // print(Util::to_string(getTimestamp()));
        if (save) {
            this->save();
        }

    }

    static void to_string(string &output, const Log& log);
    static void from_json(const JsonObject& j, Log& log);
    void save(const string& filename = "black_box.txt") const;
    Log copy();
    string getHeader() const;
    string getMessage() const;
    long double getTimestamp() const;
};


#endif //FLIGHT_LOG_HPP
