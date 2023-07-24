/*
 * Copyright (c) 2020, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ps/PowerModelChannel.hpp"
#include "ps/PowerModelEventBase.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <numeric>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <systemc>
#include <vector>

using namespace sc_core;

PowerModelChannel::PowerModelChannel(const sc_module_name name,
                                     const std::string logFilePath,
                                     sc_time logTimestep)
    : sc_module(name),
      m_eventLogFileName(logFilePath == "none"
                             ? "none"
                             : logFilePath + "/" + std::string(name) +
                                   "_eventlog.csv"),
      m_stateLogFileName(logFilePath == "none"
                             ? "none"
                             : logFilePath + "/" + std::string(name) +
                                   "_statelog.csv"),
      m_staticPowerLogFileName(logFilePath == "none"
                                   ? "none"
                                   : logFilePath + "/" + std::string(name) +
                                         "_static_power_log.csv"),
      m_eventPowerLogFileName(logFilePath == "none"
                                   ? "none"
                                   : logFilePath + "/" + std::string(name) +
                                         "_event_power_log.csv"),
      m_logTimestep(logTimestep) {
  if (logFilePath != "none") {
    // Make directory if it doesn't exist
    std::string makedir = "mkdir -p " + logFilePath;
    spdlog::info(logFilePath);
    system(makedir.c_str());
    // Create/overwrite log files
    std::ofstream f(m_eventLogFileName, std::ios::out | std::ios::trunc);
    if (!f.good()) {
      SC_REPORT_FATAL(
          this->name(),
          fmt::format("Can't open eventlog file at {}", m_eventLogFileName)
              .c_str());
    }

    std::ofstream fstate(m_stateLogFileName, std::ios::out | std::ios::trunc);
    if (!fstate.good()) {
      SC_REPORT_FATAL(
          this->name(),
          fmt::format("Can't open statelog file at {}", m_stateLogFileName)
              .c_str());
    }

    std::ofstream fs(m_staticPowerLogFileName, std::ios::out | std::ios::trunc);
    if (!fs.good()) {
      SC_REPORT_FATAL(this->name(),
                      fmt::format("Can't open staticPowerLog file at {}",
                                  m_staticPowerLogFileName)
                          .c_str());
    }

    std::ofstream fe(m_eventPowerLogFileName, std::ios::out | std::ios::trunc);
    if (!fe.good()) {
      SC_REPORT_FATAL(this->name(),
                      fmt::format("Can't open staticPowerLog file at {}",
                                  m_eventPowerLogFileName)
                          .c_str());
    }
  }
  SC_HAS_PROCESS(PowerModelChannel);
  SC_THREAD(logLoop);
}

PowerModelChannel::~PowerModelChannel() {
  dumpEventCsv();
  dumpStateCsv();
  dumpStaticPowerCsv();
  dumpEventPowerCsv();
}

int PowerModelChannel::registerEvent(
    const std::string moduleName,
    std::shared_ptr<PowerModelEventBase> eventPtr) {
  // Check if already running
  if (sc_is_running()) {
    throw std::runtime_error(
        "PowerModelEvent::registerEvent events can not be registered after "
        "simulation has started. Events shall only be registered during "
        "construction/elaboration.");
  }

  const auto it =
      std::find(m_moduleNames.begin(), m_moduleNames.end(), moduleName);
  unsigned int moduleId = -1;
  if (it == m_moduleNames.end()) {
    // This is the first registration for this module
    moduleId = m_moduleNames.size();
    m_moduleNames.push_back(moduleName);
    m_stateLog.back().push_back(-1);
  } else {
    // This is *not* the first event registration for this module
    // Check if event name already registered for the specified module name
    const auto name = eventPtr->name;
    moduleId = it - m_moduleNames.begin();
    if (std::any_of(m_events.begin(), m_events.end(),
                    [name, moduleId](const ModuleEventEntry &s) {
                      return s.event->name == name && s.moduleId == moduleId;
                    })) {
      throw std::invalid_argument(fmt::format(
          FMT_STRING("PowerModelChannel::registerEvent event '{:s}' already "
                     "registered with module '{:s}'"),
          name, moduleName));
    }
  }

  // Add event to m_events
  const unsigned int id = m_events.size();
  m_events.emplace_back(std::move(eventPtr), moduleId);
  m_eventRates.push_back(0);
  sc_assert(m_events.size() == m_eventRates.size());
  return id;
}

int PowerModelChannel::registerState(
    const std::string moduleName,
    std::shared_ptr<PowerModelStateBase> statePtr) {
  // Check if already running
  if (sc_is_running()) {
    throw std::runtime_error(
        "PowerModelEvent::registerState states can not be registered after "
        "simulation has started. States shall only be registered during "
        "construction/elaboration.");
  }

  const auto it =
      std::find(m_moduleNames.begin(), m_moduleNames.end(), moduleName);
  unsigned int moduleId = -1;
  if (it == m_moduleNames.end()) {
    // This is the first state registration for this module
    moduleId = m_moduleNames.size();
    m_moduleNames.push_back(moduleName);
    m_stateLog.back().push_back(-1);
  } else {
    // This is *not* the first state registration for this module
    // Check if state name already registered for the specified module name
    const auto name = statePtr->name;
    moduleId = it - m_moduleNames.begin();
    if (std::any_of(m_states.begin(), m_states.end(),
                    [name, moduleId](const ModuleStateEntry &s) {
                      return s.state->name == name && s.moduleId == moduleId;
                    })) {
      throw std::invalid_argument(fmt::format(
          FMT_STRING("PowerModelChannel::registerState state '{:s}' already "
                     "registered with module '{:s}'"),
          name, moduleName));
    }
  }

  // Add state to m_states
  const unsigned int id = m_states.size();
  m_states.emplace_back(std::move(statePtr), moduleId);

  // Set default state to first state registered for this module
  if (m_stateLog.back()[moduleId] == -1) {
    m_stateLog.back()[moduleId] = id;
  }

  return id;
}

void PowerModelChannel::reportEvent(const unsigned int eventId, const unsigned int n) {
  if (!sc_is_running()) {
    throw std::runtime_error(
        "PowerModelEvent::reportEvent events can not be reported before"
        "simulation has started. Events shall only be reported during "
        "simulation");
  }
  sc_assert(eventId >= 0 && eventId < m_eventLog.back().size());
  m_eventRates[eventId] += n;
  m_eventLog.back()[eventId] += n;
}

void PowerModelChannel::reportState(const unsigned int stateId) {
  if (!sc_is_running()) {
    throw std::runtime_error(
        "PowerModelState::reportState states can not be reported before"
        "simulation has started. States shall only be reported during "
        "simulation");
  }
  sc_assert(stateId >= 0 && stateId < m_states.size());
  // FUTURE: Calculate and record fraction of timestep spent in the previous
  // state
  const auto mid = m_states[stateId].moduleId;
  m_stateLog.back()[mid] = stateId;
}

int PowerModelChannel::popEventCount(const unsigned int eventId) {
  sc_assert(eventId >= 0 && eventId < m_eventLog.back().size());
  const auto tmp = m_eventRates[eventId];
  m_eventRates[eventId] = 0;
  return tmp;
}

double PowerModelChannel::popEventEnergy(const unsigned int eventId) {
  sc_assert(eventId >= 0 && eventId < m_eventLog.back().size());
  return m_events[eventId].event->calculateEnergy(m_supplyVoltage) *
         popEventCount(eventId);
}

double PowerModelChannel::popDynamicEnergy() {
  double result = 0.0;
  for (unsigned int i = 0; i < m_events.size(); ++i) {
    result += popEventEnergy(i);
  }
  return result;
}

double PowerModelChannel::getStaticCurrent() {
  // TEST
  // Log current for each module
  m_staticPowerLog.emplace_back(m_stateLog.back().size() + 1, 0.0);
  m_staticPowerLog.back().back() = sc_time_stamp().to_seconds();
  for (unsigned int i = 0; i < m_stateLog.back().size() - 1; ++i) {
    const auto &stateId = m_stateLog.back()[i];
    m_staticPowerLog.back()[i] =
        stateId >= 0
            ? m_supplyVoltage *
                  m_states[stateId].state->calculateCurrent(m_supplyVoltage)
            : 0.0;
  }

  if (m_staticPowerLog.size() > m_logDumpThreshold) {
    dumpStaticPowerCsv();
    m_staticPowerLog.clear();
  }

  // TEST
  return std::accumulate(
      m_stateLog.back().begin(), m_stateLog.back().end() - 1, 0.0,
      [=](const double &sum, const unsigned int &stateId) {
        // Ignore invalid (uninitialized) states
        return stateId >= 0 ? sum + m_states[stateId].state->calculateCurrent(
                                        m_supplyVoltage)
                            : sum;
      });
}

//___________ADDITION_________________________________

void PowerModelChannel::getDynamicPower() {
  // TEST
  // Log power for each module
  m_eventPowerLog.emplace_back(m_eventLog.back().size() + 1, 0.0);
  spdlog::info(m_eventLog.back().size());
  m_eventPowerLog.back().back() = sc_time_stamp().to_seconds();

  for (int i = 0; i < m_eventLog.back().size() - 1; ++i) {
      const auto &eventId = i; // increment ID = 0, reset ID = 1
      // Second to last eventLog entry
      int numberOfEvents = m_eventLog[m_eventLog.size()-1][i];
      // P = (n_event*E_event)/t_log
      const double dynamicPower = numberOfEvents * m_events[i].event->calculateEnergy(m_supplyVoltage) / m_logTimestep.to_seconds();
      m_eventPowerLog.back()[eventId] = dynamicPower;
    }

    // Total dynamic power in column past event dynamic powers
    for (int i = 0; i < m_events.size(); ++i) {
      m_eventPowerLog.back()[m_events.size()] += m_eventPowerLog.back()[i];
    }

    if (m_eventPowerLog.size() > m_logDumpThreshold) {
      dumpEventPowerCsv();
      m_eventPowerLog.clear();
    }

    // // TEST
    // return std::accumulate(
    //     m_eventLog.back().begin(), m_eventLog.back().end() - 1, 0.0,
    //     [=](const double &sum, const int &eventId) {
    //       // Ignore invalid (uninitialized) events
    //       return eventId >= 0 ? sum + m_events[eventId].event->calculateEnergy(
    //                                       m_supplyVoltage)
    //                           : sum;
    //     });
}
//_________________________________________________________________________


void PowerModelChannel::start_of_simulation() {
  // Initialize event and state log
  m_eventLog.emplace_back(m_events.size() + 1, 0);

  // First entry is at t = timestep
  m_eventLog.back().back() =
      static_cast<int>(m_logTimestep.to_seconds() * 1.0e6);

  m_stateLog.back().push_back(
      static_cast<int>(m_logTimestep.to_seconds() * 1.0e6));

  // Print list of events & states
  spdlog::info("-- PowerModelChannel Registered Events & States ------");
  for (unsigned int i = 0; i < m_moduleNames.size(); ++i) {
    spdlog::info("\t<module> {:s}:", m_moduleNames[i]);
    for (const auto &e : m_events) {
      if (e.moduleId == i) {
        spdlog::info("\t\t{:s}", e.event->toString());
      }
    }
    for (const auto &s : m_states) {
      if (s.moduleId == i) {
        spdlog::info("\t\t{:s}", s.state->toString());
      }
    }
  }
  spdlog::info("----------------------------------------------");
}

void PowerModelChannel::logLoop() {
  if (m_eventLogFileName == "none" || m_logTimestep == SC_ZERO_TIME) {
    SC_REPORT_INFO(this->name(), "Logging disabled.");
    return;
  }

  while (1) {
    // Wait for a timestep
    wait(m_logTimestep);
    // Copy current state (excluding time stamp)
    const auto currentState = std::vector<int>(m_stateLog.back());
    // Dump file when log exceeds threshold
    if (m_eventLog.size() > m_logDumpThreshold) {
      dumpEventCsv();
      dumpStateCsv();
      m_eventLog.clear();
      m_stateLog.clear();
    }

    // Push new timestep
    m_eventLog.emplace_back(m_events.size() + 1, 0); // New row of all 0s
    m_stateLog.emplace_back(currentState);           // New row of all 0s

    // Last column in the new row is the current time step
    m_eventLog.back().back() = static_cast<int>(
        (m_logTimestep + sc_time_stamp()).to_seconds() * 1.0e6);
    m_stateLog.back().back() = static_cast<int>(
        (m_logTimestep + sc_time_stamp()).to_seconds() * 1.0e6);
  }
}

void PowerModelChannel::dumpEventCsv() {
  std::ofstream f(m_eventLogFileName, std::ios::out | std::ios::app);
  if (f.tellp() == 0) {
    // Header
    for (unsigned int i = 0; i < m_events.size() + 1; ++i) {
      if (i < m_events.size()) {
        f << m_moduleNames[m_events[i].moduleId] << " "
          << m_events[i].event->name << ',';
      } else {
        f << "time(us)\n";
      }
    }
  }

  // Values
  for (const auto &row : m_eventLog) {
    for (const auto &val : row) {
      f << val << (&val == &row.back() ? '\n' : ',');
    }
  }
}

void PowerModelChannel::dumpStateCsv() {
  std::ofstream f(m_stateLogFileName, std::ios::out | std::ios::app);
  if (f.tellp() == 0) {
    // State ID mapping
    f << "module,state,id\n";
    for (unsigned int i = 0; i < m_states.size(); ++i) {
      f << m_moduleNames[m_states[i].moduleId] << "," << m_states[i].state->name
        << ',' << i << "\n";
    }

    f << "\n\n";

    // Column header (module names)
    for (const auto &nm : m_moduleNames) {
      f << nm << ",";
    }
    f << "time(us)\n";
  }

  // Values
  for (const auto &row : m_stateLog) {
    for (const auto &val : row) {
      f << val << (&val == &row.back() ? '\n' : ',');
    }
  }
}

void PowerModelChannel::dumpStaticPowerCsv() {
  std::ofstream f(m_staticPowerLogFileName, std::ios::out | std::ios::app);
  if (f.tellp() == 0) {
    // Header
    for (const auto &nm : m_moduleNames) {
      f << nm << ",";
    }
    f << "time(s)\n";
  }
  
  // Values
  // Calculate average
  auto res = std::vector<double>(m_stateLog.back().size() + 1, 0.0);

  for (unsigned int i = 0; i < m_staticPowerLog.size(); ++i) {
    // Average values
    for (unsigned int j = 0; j < m_staticPowerLog[i].size() - 1; ++j) {
      res[j] += m_staticPowerLog[i][j] / m_staticPowerAveragingFactor;
    }

    // Dump
    if ((i > 0 && (i % m_staticPowerAveragingFactor == 0)) ||
        i == m_staticPowerLog.size() - 1) {
      if (i != 10){
        spdlog::info("Statelog: {}", i);
      }
      for (auto &val : res) {
        f << val << (&val == &res.back() ? '\n' : ',');
        val = 0.0;
      }
    }

    // Time stamp
    if (i % m_staticPowerAveragingFactor == 0) {
      res.back() = m_staticPowerLog[i].back();
    }
  }
}

//_____________ADDITION________________________________
void PowerModelChannel::dumpEventPowerCsv() {
  std::ofstream f(m_eventPowerLogFileName, std::ios::out | std::ios::app);
  if (f.tellp() == 0) {
    // Header
    for (const auto &nm : m_events) {
      f << nm.event->name << ",";
    }
    f << "time(s)\n";
  }
  // Values
  // Calculate average
  auto res = std::vector<double>(m_eventLog.back().size() + 1, 0.0);
  for (int i = 0; i < m_eventPowerLog.size(); ++i) {
    // Average values
    for (int j = 0; j < m_eventPowerLog[i].size() - 1; ++j) {
      res[j] += m_eventPowerLog[i][j] / m_eventPowerAveragingFactor;
    }

    // Dump
    if ((i > 0 && (i % m_eventPowerAveragingFactor == 0)) ||
        i == m_eventPowerLog.size() - 1) {
        if(i != 10){
          spdlog::info("Eventlog: {}", i);
          spdlog::info("First condition: {}", i > 0 && (i % m_eventPowerAveragingFactor == 0));
          spdlog::info("Second condition: {}", i == m_eventPowerLog.size() - 1);
          spdlog::info(m_eventPowerLog[i].back());
        }
        for (auto &val : res) {
          f << val << (&val == &res.back() ? '\n' : ',');
          val = 0.0;
      }
    }

    // Time stamp
    if (i % m_eventPowerAveragingFactor == 0) {
      res.back() = m_eventPowerLog[i].back();
    }
  }
}

void PowerModelChannel::setSupplyVoltage(const double val) {
  if (m_supplyVoltage != val) {
    m_supplyVoltage = val;
    m_supplyVoltageChangedEvent.notify(SC_ZERO_TIME);
  }
}
