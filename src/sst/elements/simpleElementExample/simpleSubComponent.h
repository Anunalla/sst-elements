// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _SIMPLESUBCOMPONENT_H
#define _SIMPLESUBCOMPONENT_H

#include<vector>

#include <sst/core/component.h>
#include <sst/core/elementinfo.h>
#include <sst/core/subcomponent.h>
#include <sst/core/link.h>

namespace SST {
namespace SimpleSubComponent {

class SubCompInterface : public SST::SubComponent
{
public:
    SubCompInterface(Component *owningComponent) :
        SubComponent(owningComponent)
    { }
    virtual ~SubCompInterface() {}
    virtual void clock(SST::Cycle_t) = 0;
};

/* Our trivial component */
class SubComponentLoader : public Component
{
public:
    SubComponentLoader(ComponentId_t id, SST::Params& params);

private:

    bool tick(SST::Cycle_t);
    std::vector<SubCompInterface*> subComps;

    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(SubComponentLoader,
                               "simpleElementExample",
                               "SubComponentLoader",
                               "Demonstrates subcomponents",
                               COMPONENT_CATEGORY_UNCATEGORIZED
    )

    SST_ELI_DOCUMENT_VERSION(1,0,0)

    SST_ELI_DOCUMENT_PARAMS(
        {"clock", "Clock Rate", "1GHz"},
    )

    SST_ELI_DOCUMENT_STATISTICS(
    )

    SST_ELI_DOCUMENT_PORTS(
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"mySubComp", "Test slot", "SST::SimpleSubComponent::SubCompInterface" }
    )
};


/* Our example subcomponents */
class SubCompSender : public SubCompInterface
{
    Statistic<uint32_t> *nMsgSent;
    uint32_t nToSend;
    SST::Link *link;
public:
    SubCompSender(Component *owningComponent, Params &params);
    ~SubCompSender() {}
    void clock(Cycle_t);

    // REGISTER THIS SUB-COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_SUBCOMPONENT(
        SubCompSender,
        "simpleElementExample",
        "SubCompSender",
        "Sending Subcomponent",
        "SST::simpleSubComponent::SubCompInterface"
    )
    
    SST_ELI_DOCUMENT_VERSION(1,0,0)

    SST_ELI_DOCUMENT_PARAMS(
        {"sendCount", "Number of Messages to Send", "10"},
    )

    SST_ELI_DOCUMENT_STATISTICS(
        {"numSent", "# of msgs sent", "", 1},
    )

    SST_ELI_DOCUMENT_PORTS(
        {"sendPort", "Sending Port", { "simpleMessageGeneratorComponent.simpleMessage", "" } }
    )
    
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
    )
};


class SubCompReceiver : public SubCompInterface
{
    Statistic<uint32_t> *nMsgReceived;
    SST::Link *link;

    void handleEvent(SST::Event *ev);

public:
    SubCompReceiver(Component *owningComponent, Params &params);
    ~SubCompReceiver() {}
    void clock(Cycle_t);

    // REGISTER THIS SUB-COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_SUBCOMPONENT(
        SubCompReceiver,
        "simpleElementExample",
        "SubCompReceiver",
        "Receiving Subcomponent",
        "SST::simpleSubComponent::SubCompInterface"
    )
    
    SST_ELI_DOCUMENT_VERSION(1,0,0)

    SST_ELI_DOCUMENT_PARAMS(
    )

    SST_ELI_DOCUMENT_STATISTICS(
        {"numRecv", "# of msgs sent", "", 1},
    )

    SST_ELI_DOCUMENT_PORTS(
        {"recvPort", "Receiving Port", { "simpleMessageGeneratorComponent.simpleMessage", "" } }
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
    )
};

} // namespace SimpleSubComponent
} // namespace SST




#endif /* _SIMPLESUBCOMPONENT_H */
