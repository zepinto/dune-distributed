//***************************************************************************
// Copyright 2017 OceanScan - Marine Systems & Technology, Lda.             *
//***************************************************************************
// This file is subject to the terms and conditions defined in file         *
// 'LICENCE.md', which is part of this source code package.                 *
//***************************************************************************
// Author: Ricardo Martins                                                  *
//***************************************************************************

#ifndef TRANSPORTS_BROADLINK_DRIVER_HPP_INCLUDED_
#define TRANSPORTS_BROADLINK_DRIVER_HPP_INCLUDED_

// ISO C++ 98 headers.
#include <cstddef>
#include <set>
#include <string>

// DUNE headers.
#include "NodeMap.hpp"
#include <DUNE/DUNE.hpp>

namespace Transports
{
  namespace Broadlink
  {
    class Driver: public DUNE::Hardware::BasicModem
    {
    public:
      //! Constructor.
      //!
      //! @param[in] task parent task.
      //! @param[in] handle I/O handle.
      Driver(DUNE::Tasks::Task* task, DUNE::IO::Handle* handle):
        DUNE::Hardware::BasicModem(task, handle)
      {
        m_task = task;
        m_handle = handle;
        m_seq = 1;
      }

      void
      setSoundSpeed(double value)
      {
        // Do something with the sound speed value.
        m_sound_speed = value;
        //sendCommand("set-sound-speed " + std::to_string(value), "OK");
      }

      void
      setActiveChannel(int chan)
      {
        std::string command = String::str("tx-chan %02d\n", chan);
        sendCommand(command, "OK");
      }

      void
      setTrasmissionMode(std::string mode)
      {
        if (mode == m_tx_mode)
          return;
        sendCommand("tx-mode " + mode, "OK");
        m_tx_mode = mode;
      }

      void
      setLpiTxMode()
      {
        if (m_tx_mode == "lpi")
          return;
        setTrasmissionMode("lpi");        
      }

      void
      setOfdmTxMode()
      {
        if (m_tx_mode == "ofdm")
          return;
        setTrasmissionMode("ofdm");
      }

      void
      sendCommand(std::string command, std::string expected)
      {
        m_task->debug("Sending command: %s", command.c_str());
        m_handle->writeString(command.c_str());
        expect(expected);
      }

      void
      sendFrame(const DUNE::IMC::UamTxFrame* frame)
      {
        m_task->debug("Sending frame");

        // find the address of the destination node
        unsigned addr = 0;
        m_node_map.lookupAddress(frame->sys_dst, addr);

        // command to send the frame
        int size = frame->data.size();
        std::string cmd = String::str("tx-data %d %d\n", addr, size);
        sendCommand(cmd, "OK");

        // send the frame
        m_handle->write(frame->data.data(), size);
        expect("OK");
      }

      void
      sendRange(const DUNE::IMC::UamTxRange* range)
      {
        // find the address of the destination node
        unsigned addr;
        m_node_map.lookupAddress(range->sys_dst, addr);
        
        // command to send the range
        std::string cmd = String::str("range-request");
      }

      std::string
      poll(double timeout)
      {
        setTimeout(timeout);
        try {
          return readLine();
        }
        catch (...)
        {
          return "";
        }        
      }

      std::string
      getModemVersion()
      {
        m_handle->writeString("sys-sw-version\n");
        try
        {
          std::string version = poll(1.0);
          m_task->inf("Modem version: %s", version.c_str());
          return version;
        }
        catch (...)
        {
          m_task->err("Failed to get modem version");
        }
        return "unknown";
      }

      std::string
      getSystemStatus()
      {
        m_handle->writeString("system-status\n");
        try
        {
          std::string status = poll(1.0);
          m_task->inf("System status: %s", status.c_str());
          return status;
        }
        catch (...)
        {
          m_task->err("Failed to get system status");
        }
        return "unknown";
      }

      void
      setTime() {
        BrokenDown time;
        std::string cmd = String::str("sys-rtc-clock %04u-%02u-%02u %02u:%02u:%02u\n",
                                      time.year, time.month, time.day,
                                      time.hour, time.minutes, time.seconds);
        sendCommand(cmd, "OK");
      }

      void
      initModem(void)
      {
        // Initialize modem.
        getModemVersion();
        getSystemStatus();
      }

      void
      setNodeMap(const NodeMap& map)
      {
        // Do something with the node map.
        m_node_map = map;
      }

    private:
      //! Node map.
      NodeMap m_node_map;
      //! Last sound speed value.
      double m_sound_speed;
      //! parent task.
      DUNE::Tasks::Task* m_task;
      //! handle
      DUNE::IO::Handle* m_handle;
      //! Packet sequence number.
      unsigned m_seq;
      //! current transmission mode
      std::string m_tx_mode;
    };
  }
}

#endif