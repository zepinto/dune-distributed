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
#include <DUNE/DUNE.hpp>
#include "NodeMap.hpp"

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
      }

      void
      setSoundSpeed(double value)
      {
        // Do something with the sound speed value.
        m_sound_speed = value;        
      }

      void
      sendFrame(const DUNE::IMC::UamTxFrame* frame)
      {
        // Do something with the frame.
        m_task->inf("Sending frame");
      }

      void
      sendRange(const DUNE::IMC::UamTxRange* range)
      {
        // Do something with the range.
        m_task->inf("Sending range");
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
    }
    ;
  }
}

#endif