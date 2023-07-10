/*
 * Copyright (c) 2019-2020, University of Southampton and Contributors.
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libs/make_unique.hpp"
#include "ps/ConstantCurrentState.hpp"
#include "ps/ConstantEnergyEvent.hpp"
#include "ps/PowerModelChannel.hpp"
#include "ps/PowerModelChannelIf.hpp"
#include "ps/PowerModelBridge.hpp"

#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <systemc>

using namespace sc_core;

SC_MODULE(Memory) {
  PowerModelEventOutPort powerModelPort{"outport"};

  SC_CTOR(Memory){}

  virtual void end_of_elaboration() override {
    // Register events for power model
    //
    // These are very simple constant current events. They have a name, e.g.
    // "write", and a constant energy consumption per occurrence (in Joules).
    //
    // In this example, we're explicitly setting the energy/power numbers, but
    // in a more realistic scenario, you'd probably want to load that from 
    // some sort of power/configuration database/dictonary, as is done in 
    // Fused (and all other simulators).
    //
    // Events & states are registered with the power modelling channel to get
    // a unique ID, which you use later to report when the events occur and
    // when the state of the module changes (i.e. off->on).
    //
    // These events are wrapped in make_unique to move the ownership of the
    // events to the channel. You could also use make_shared if you needed to
    // retain shared ownership of the event (e.g. in cases where the energy
    // changes depending on some other variable, and you need to input that
    // other variable somehow).

    m_writeEventId = powerModelPort->registerEvent(
        this->name(),
        std::make_unique<ConstantEnergyEvent>("write", 0.001));

    m_readEventId = powerModelPort->registerEvent(
        this->name(),
        std::make_unique<ConstantEnergyEvent>("read", 0.00005));

    // Register states
    m_offStateId = powerModelPort->registerState(
        this->name(),
        std::make_unique<ConstantCurrentState>("off", 0.0));
    m_onStateId = powerModelPort->registerState(
        this->name(), std::make_unique<ConstantCurrentState>("on", 0.0001));

    // Register methods with SC kernel
    SC_THREAD(process);
  }

  void process () {
    // Obviously, this is just reporting events & states, but not actually
    // doing anything interesting. See fused for more realistic usage.
    // E.g. fused/mcu/GenericMemory.cpp is a simple example
    wait(SC_ZERO_TIME);
    while (1) {
      wait(400, SC_NS);
      // Power on
      powerModelPort->reportState(m_onStateId);

      // Do some reads
      for (int i = 0; i < 45; ++i) {
        powerModelPort->reportEvent(m_readEventId);
        wait(1, SC_NS);
      }

      // Do some writes
      for (int i = 0; i < 45; ++i) {
        powerModelPort->reportEvent(m_readEventId);
        wait(1, SC_NS);
      }

      // Do some writes & writes
      for (int i = 0; i < 45; ++i) {
        powerModelPort->reportEvent(m_readEventId);
        wait(1, SC_NS);
        powerModelPort->reportEvent(m_writeEventId);
        wait(1, SC_NS);
      }

      // Power off
      powerModelPort->reportState(m_offStateId);
    }
  }

  int m_writeEventId{-1};
  int m_readEventId{-1};
  int m_offStateId{-1};
  int m_onStateId{-1};
};

int sc_main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
  sc_signal<double> current("current");
  sc_signal<double> voltage("voltage", /*voltage[V]=*/0.8);

  // The channel. It does the job of counting how many events have occurred in
  // the current timestep (since the last time the event energy was read from
  // the channel), and when a consumer calls getDynamicEnergy, the channel
  // multiplies each event type's energy consumption with the aggregated
  // event count.
  // It also maintains an array which keeps track of total event counts. This
  // array is dumped to a csv file once in a while, and at the end of
  // simulation.
  // Static power works similarly, and also produces a csv trace.
  PowerModelChannel ch("ch", ".", /*csvTimeStep=*/sc_time(1, SC_US));

  // The bridge reads event energy and state power from the channel and
  // converts it to a current
  PowerModelBridge bridge("bridge", /*timestep=*/sc_time(1, SC_US));

  Memory memory("memory");

  // Hook up
  bridge.v_in.bind(voltage);
  bridge.i_out.bind(current);
  bridge.powerModelPort.bind(ch);

  memory.powerModelPort.bind(ch);

  // The built in systemc tracing functions are handy for showing currents and
  // voltages (along with digital signals, not shown here).
  auto vcdfile = sc_create_vcd_trace_file("trace");
  vcdfile->set_time_unit(1, SC_NS);
  sc_trace(vcdfile, current, "current");

  // Simulate for a millisecond
  sc_start(1, SC_MS);

  sc_close_vcd_trace_file(vcdfile);

  return false;
}
