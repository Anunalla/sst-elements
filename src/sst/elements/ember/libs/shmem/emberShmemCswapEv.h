// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_SHMEM_CSWAP_EVENT
#define _H_EMBER_SHMEM_CSWAP_EVENT

#include "emberShmemEvent.h"

namespace SST {
namespace Ember {

class EmberCswapShmemEvent : public EmberShmemEvent {

public:
	EmberCswapShmemEvent( Shmem::Interface& api, Output* output,
            Hermes::Value result, Hermes::Vaddr dest, Hermes::Value cond, Hermes::Value value, int pe,
            EmberEventTimeStatistic* stat = NULL ) :
            EmberShmemEvent( api, output, stat ),
            m_result(result), m_dest(dest), m_cond(cond), m_value(value), m_pe(pe) {}
	~EmberCswapShmemEvent() {}

    std::string getName() { return "Cswap"; }

    void issue( uint64_t time, Shmem::Callback callback ) {

        EmberEvent::issue( time );
        m_api.cswap( m_result, m_dest, m_cond, m_value, m_pe, callback );
    }

private:
    Hermes::Vaddr m_dest;

    Hermes::Value m_result;
    Hermes::Value m_value;
    Hermes::Value m_cond;
    int m_pe;
};

}
}

#endif
