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
 * Author: Adrian S. Tam <adrian.sw.tam@gmail.com>
 */

#include "ipv4-mix-routing-helper.h"
#include "ns3/log.h"
#include "ns3/ipv4-list-routing.h"

NS_LOG_COMPONENT_DEFINE("Ipv4MixRoutingHelper");

namespace ns3 {

Ipv4MixRoutingHelper*
Ipv4MixRoutingHelper::Copy (void) const
{
  return new Ipv4MixRoutingHelper (*this);
}

Ptr<Ipv4RoutingProtocol>
Ipv4MixRoutingHelper::Create (Ptr<Node> node) const
{
  Ptr<MixRouting> r = CreateObject<MixRouting> ();
  return r;
}

Ptr<MixRouting>
Ipv4MixRoutingHelper::GetMixRouting (Ptr<Ipv4> ipv4) const
{
  NS_LOG_FUNCTION (this);
  Ptr<Ipv4RoutingProtocol> ipv4rp = ipv4->GetRoutingProtocol ();
  NS_ASSERT_MSG (ipv4rp, "No routing protocol associated with Ipv4");
  if (DynamicCast<MixRouting> (ipv4rp))
    {
      NS_LOG_LOGIC ("Hash routing found as the main IPv4 routing protocol.");
      return DynamicCast<MixRouting> (ipv4rp);
    }
  if (DynamicCast<Ipv4ListRouting> (ipv4rp))
    {
      Ptr<Ipv4ListRouting> lrp = DynamicCast<Ipv4ListRouting> (ipv4rp);
      int16_t priority;
      for (uint32_t i = 0; i < lrp->GetNRoutingProtocols ();  i++)
        {
          NS_LOG_LOGIC ("Searching for Hash routing in list");
          Ptr<Ipv4RoutingProtocol> temp = lrp->GetRoutingProtocol (i, priority);
          if (DynamicCast<MixRouting> (temp))
            {
              NS_LOG_LOGIC ("Found Hash routing in list");
              return DynamicCast<MixRouting> (temp);
            }
        }
    }
  NS_LOG_LOGIC ("Hash routing not found");
  return 0;
}

}; // namespace ns3
