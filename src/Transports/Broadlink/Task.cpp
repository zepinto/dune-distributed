//***************************************************************************
// Copyright 2024 OceanScan - Marine Systems & Technology, Lda.             *
//***************************************************************************
// This file is subject to the terms and conditions defined in file         *
// 'LICENCE.md', which is part of this source code package.                 *
//***************************************************************************
// Author: José Pinto                                                       *
//***************************************************************************

// DUNE headers.
#include <DUNE/DUNE.hpp>

// Include the driver header file
#include "Driver.hpp"   
#include "NodeMap.hpp"

namespace Transports
{
  //! Device driver for the Broadlink family of underwater acoustic modems.
  namespace Broadlink
  {
    using DUNE_NAMESPACES;

    struct Arguments
    {
      //! Device URI.
      std::string dev;
      //! Power channel name.
      std::string power_channel;
      //! Maximum transmission unit.
      unsigned mtu;
      //! Name of the section with modem addresses.
      std::string addr_section;
      //! Default Sound speed on water.
      double sound_speed_def;
      //! Entity label of sound speed provider.
      std::string sound_speed_elabel;
    };

    struct Task: public Hardware::BasicDeviceDriver
    {
      //! I/O handle.
      IO::Handle* m_io;
      //! Driver.
      Driver* m_driver;
      //! Local modem address.
      unsigned m_local_address;
      //! Node map.
      NodeMap m_node_map;
      //! Last sound speed value.
      double m_sound_speed;
      //! Sound speed entity id.
      int m_sound_speed_eid;
      //! Task arguments.
      Arguments m_args;

      //! Constructor.
      //! @param[in] name task name.
      //! @param[in] ctx context.
      Task(const std::string& name, Tasks::Context& ctx):
        Hardware::BasicDeviceDriver(name, ctx),
        m_io(nullptr),
        m_driver(nullptr),
        m_local_address(0),
        m_sound_speed_eid(-1)
      {
        paramActive(Tasks::Parameter::SCOPE_GLOBAL, Tasks::Parameter::VISIBILITY_USER);

        param("Device", m_args.dev).description("Device URI");

        param("Power Channel - Name", m_args.power_channel)
          .defaultValue("")
          .description("Name of the modem's power channel");

        param("Maximum Transmission Unit", m_args.mtu)
          .units(Units::Byte)
          .defaultValue("32")
          .description("Name of the configuration section with modem addresses");

        param("Address Section", m_args.addr_section)
          .defaultValue("")
          .description("Name of the configuration section with modem addresses");

        param("Sound Speed - Default Value", m_args.sound_speed_def)
          .defaultValue("1500")
          .minimumValue("1375")
          .maximumValue("1900")
          .units(Units::MeterPerSecond)
          .description("Water sound speed");

        param("Sound Speed - Entity Label", m_args.sound_speed_elabel)
          .description("Entity label of sound speed provider");

        bind<IMC::UamTxFrame>(this);
        bind<IMC::UamTxRange>(this);
      }

      ~Task() override
      {
        onDisconnect();
      }


      template <typename T>
      void
      sendTxStatus( const T &request, IMC::UamTxStatus::ValueEnum value, const std::string &error = "" )
      {
        IMC::UamTxStatus status;
        status.seq = request.seq;
        status.value = value;
        status.error = error;
        dispatch( status );
      }

      //! Update internal state with new parameter values.
      void
      onUpdateParameters() override
      {
        // Power channel changed.
        if (paramChanged(m_args.power_channel))
        {
          if (!m_args.power_channel.empty())
          {
            clearPowerChannelNames();
            addPowerChannelName(m_args.power_channel);
          }
        }

        // Address section changed.
        if (paramChanged(m_args.addr_section))
        {
          m_node_map.readConfigSection(m_ctx.config, m_args.addr_section);
          if (!m_node_map.lookupAddress(getSystemName(), m_local_address))
            err(DTR("local modem address is not configured"));
        }

        // Sound speed.
        if (paramChanged(m_args.sound_speed_def))
          m_sound_speed = m_args.sound_speed_def;
      }

      void
      onResourceRelease() override
      {
        onDisconnect();
      }

      void
      onEntityResolution() override
      {
        try
        {
          m_sound_speed_eid = resolveEntity( m_args.sound_speed_elabel );
        }
        catch ( ... )
        {
          m_sound_speed = m_args.sound_speed_def;
        }
      }

      void
      onSoundSpeed( double value ) override
      {
        m_sound_speed = value;

        if ( m_driver != nullptr )
          m_driver->setSoundSpeed( m_sound_speed );
      }

      void
      consume(const IMC::UamTxFrame* msg)
      {
        if (msg->getDestination() != getSystemId())
          return;

        if (m_driver == nullptr)
        {
          err(DTR("driver not initialized"));
          return;
        }

        if ( m_driver->isBusy() )
        {
          spew( "modem is busy" );
          sendTxStatus( *msg, IMC::UamTxStatus::UTS_BUSY, DTR( "modem is busy" ) );
          return;
        }

        m_driver->sendFrame(msg);
      }

      void
      consume(const IMC::UamTxRange* msg)
      {
        if (msg->getDestination() != getSystemId())
          return;

        if (m_driver == nullptr)
        {
          err(DTR("driver not initialized"));
          return;
        }

        if ( m_driver->isBusy() )
        {
          spew( "modem is busy" );
          sendTxStatus( *msg, IMC::UamTxStatus::UTS_BUSY, DTR( "modem is busy" ) );
          return;
        }

        m_driver->sendRange(msg);
      }

      bool
      onConnect() override
      {
        try
        {
          m_io = openDeviceHandle( m_args.dev );
          m_driver = new Driver( this, m_io );
          m_driver->start();
          spew( "onConnect" );
          return true;
        }
        catch ( std::runtime_error &e )
        {
          if (errno == EINPROGRESS)
            throw RestartNeeded(e.what(), 5.0, false);
          else
          {
            Memory::clear(m_io);
            err("failed to connect: %s", e.what());
            return false;
          }
        }
      }

      void
      onDisconnect() override
      {
        Memory::clear(m_driver);
        Memory::clear(m_io);
      }

      bool
      onReadData() override
      {
        if (m_driver != nullptr)
          m_driver->poll();
        
        return true;
      }

      void
      onInitializeDevice() override
      {
        if (m_driver != nullptr)
        {
          m_driver->setNodeMap(m_node_map);
          m_driver->setSoundSpeed(m_sound_speed);
        }
      }

    };
  }
}

DUNE_TASK