/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include <fstream>

#include "ns3/parameter.h"
#include "ns3/fat-tree-helper.h"
#include "ns3/global-content-manager.h"


#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("CCDN");

void CreateContent(uint64_t content, unsigned numHost, unsigned *hosts);
void UpdateContent(uint64_t content);
void RequireFile(unsigned host, uint64_t content);
void Review();
void ParseSetup(Parameter *para, char* filename, double timescale);

double scanscap = 0.1;
GlobalContentManager *manager = 0;

int
main (int argc, char *argv[])
{

    //The attributes:
    //1.port; 2.fib_size; 3.cache_size; 4.enable_cache, 5.timescale;
    //6.input; 7.output;

    if (argc < 8)
    {
        return 0;
    }

    Parameter *para = new Parameter;
    para->port = std::atoi(argv[1]);
    para->fib_size = std::atoi(argv[2]);
    para->cache_size = std::atoi(argv[3]);
    para->enable_cache = argv[4][0] == '1';
    para->filename = argv[7];

    manager = new GlobalContentManager();
    ParseSetup(para, argv[6], std::atoi(argv[5]));
    Simulator::Run ();
    Simulator::Destroy ();
    return 0;
}


void CreateContent(uint64_t content, unsigned numHost, unsigned *hosts)
{
    NS_LOG_LOGIC("Create chunk "<<content<<" on "<<numHost<<" hosts: first is "<<hosts[0]);
    manager->CreateContent(content, numHost, hosts);
}
void UpdateContent(uint64_t content)
{
    NS_LOG_LOGIC("Update chunk "<<content);
    manager->UpdateContent(content);
}
void RequireFile(unsigned host, uint64_t content)
{
    NS_LOG_LOGIC("Access chunk "<<content<<" on "<<host);
    manager->RequireFile(host, content);
}
void Review()
{
    manager->ReviewFile();
}


void ParseSetup(Parameter *para, char* filename, double timescale)
{

    std::ifstream is;
    is.open(filename);

    for (double cur_time = scanscap; cur_time <= timescale; cur_time += scanscap)
    {
        //Simulator::Schedule(Seconds(cur_time), &Review);
    }

    manager->SetPara(para);
    manager->Create();

    while (is.good())
    {
        char command;
        double time;
        char content_in_char[16];
        uint64_t content = 0;
        is >> command >> time >> content_in_char;
        for (int i=0; i<16; i++)
        {
            content = (content << 4) | (content_in_char[i] >= 97 ? content_in_char[i] - 87 : content_in_char[i] - 48);
        }

        if (command == 'c')
        {
            unsigned host;
            is >> host;
            unsigned *hosts = new unsigned[host];
            for (unsigned i=0; i<host; i++)
            {
                is>>hosts[i];
            }

            NS_LOG_LOGIC("Schedule create "<<content<<" on "<<host<<" hosts: first is "<<hosts[0]);
            Simulator::Schedule(Seconds(time), &CreateContent, content, host, hosts);
        }
        else if (command == 'u')
        {
            NS_LOG_LOGIC("Schedule update "<<content);
            Simulator::Schedule(Seconds(time), &UpdateContent, content);
        }
        else if (command == 'a')
        {
            unsigned host;
            is >> host;
            NS_LOG_LOGIC("Schedule access "<<content<<" on "<<host);
            Simulator::Schedule(Seconds(time), &RequireFile, host, content);
        }
    }

    is.close();
}
