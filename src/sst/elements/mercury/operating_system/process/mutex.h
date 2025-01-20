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

#pragma once

#include <mercury/operating_system/process/thread.h>

#include <list>
#include <map>

namespace SST {
namespace Hg {

class mutex_t  {
 public:
  /** Blocking keys for those threads waiting on the mutex */
  std::list<Thread*> waiters;
  std::list<Thread*> conditionals;
  bool locked;

  mutex_t() : locked(false)
  {
  }
};

typedef std::map<long, mutex_t*> condition_t;

} // end namespace Hg
} // end namespace SST
