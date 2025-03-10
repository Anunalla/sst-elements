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

#ifndef _SUMI_POLL_H_
#define _SUMI_POLL_H_

#ifdef __cplusplus
extern "C" {
#endif

int sumi_poll_poll(struct fid_poll *pollset, void **context, int count);

int sumi_poll_add(struct fid_poll *pollset, struct fid *event_fid,
		  uint64_t flags);

int sumi_poll_del(struct fid_poll *pollset, struct fid *event_fid,
		  uint64_t flags);

#ifdef __cplusplus
}
#endif

#endif
