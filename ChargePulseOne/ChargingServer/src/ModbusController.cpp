#include "ModbusController.h"
#include "Logger.h"

void ModbusController::init() {
    Logger::instance().log(INFO, "ModbusController initialized (simulated)");
}
