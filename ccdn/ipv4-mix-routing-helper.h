/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 New York University
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Adrian S Tam <adrian.sw.tam@gmail.com>
 */

#ifndef IPV4_MIX_ROUTING_HELPER_H
#define IPV4_MIX_ROUTING_HELPER_H

#include "ns3/mix-routing.h"
#include "ns3/ipv4.h"
#include "ns3/ptr.h"
#include "ns3/node.h"
#include "ns3/ipv4-routing-helper.h"

namespace ns3 {

/**
 * \brief Helper class that adds ns3::Ipv4MixRouting objects
 *
 * This class is expected to be used in conjunction with
 * ns3::InternetStackHelper::SetRoutingHelper
 */
class Ipv4MixRoutingHelper : public Ipv4RoutingHelper
{
public:
  /*
   * Construct an Ipv4MixRoutingHelper object, used to make configuration
   * of Hash routing easier.
   */
  Ipv4MixRoutingHelper () {};

  /**
   * \brief Construct an Ipv4MixRoutingHelper from another previously
   * initialized instance (Copy Constructor).
   */
  Ipv4MixRoutingHelper (const Ipv4MixRoutingHelper &) {};

  /**
   * \internal
   * \returns pointer to clone of this Ipv4MixRoutingHelper
   *
   * This method is mainly for internal use by the other helpers;
   * clients are expected to free the dynamic memory allocated by this method
   */
  Ipv4MixRoutingHelper* Copy (void) const;

  /**
   * \param node the node on which the routing protocol will run
   * \returns a newly-created routing protocol
   *
   * This method will be called by ns3::InternetStackHelper::Install
   */
  virtual Ptr<Ipv4RoutingProtocol> Create (Ptr<Node> node) const;

  /**
   * Try and find the Hash routing protocol as either the main routing
   * protocol or in the list of routing protocols associated with the
   * Ipv4 provided.
   *
   * \param ipv4 the Ptr<Ipv4> to search for the Hash routing protocol
   */
  Ptr<MixRouting> GetMixRouting (Ptr<Ipv4> ipv4) const;

private:
  /**
   * \internal
   * \brief Assignment operator declared private and not implemented to disallow
   * assignment and prevent the compiler from happily inserting its own.
   */
  Ipv4MixRoutingHelper &operator = (const Ipv4MixRoutingHelper &o);
};

} // namespace ns3

#endif /* IPV4_HASH_ROUTING_HELPER_H */
