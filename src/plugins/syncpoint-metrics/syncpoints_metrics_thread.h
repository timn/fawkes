/***************************************************************************
 *  metrics_syncpoints_thread.h - collect metrics for syncpoints
 *
 *  Created: Sat Jul 29 22:51:41 2017 (Aonami Line from Kinjo Futo to Nagoya)
 *  Copyright  2017  Tim Niemueller [www.niemueller.de]
 ****************************************************************************/

/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  Read the full text in the LICENSE.GPL file in the doc directory.
 */

#ifndef __PLUGINS_METRICS_SYNCPOINTS_METRICS_SYNCPOINTS_THREAD_H_
#define __PLUGINS_METRICS_SYNCPOINTS_METRICS_SYNCPOINTS_THREAD_H_

#include <aspect/blackboard.h>
#include <aspect/configurable.h>
#include <aspect/logging.h>
#include <aspect/syncpoint_manager.h>
#include <core/threading/thread.h>
#include <core/utils/lock_map.h>
#include <plugins/metrics/aspect/metrics.h>

#include <plugins/metrics/aspect/metrics_supplier.h>

class SyncPointsMetricsThread : public fawkes::Thread,
                                public fawkes::LoggingAspect,
                                public fawkes::ConfigurableAspect,
                                public fawkes::BlackBoardAspect,
                                public fawkes::MetricsAspect,
                                public fawkes::SyncPointManagerAspect,
                                public fawkes::MetricsSupplier
{
 public:
	SyncPointsMetricsThread();
	virtual ~SyncPointsMetricsThread();

	virtual void init();
	virtual void loop();
	virtual void finalize();

	/** Stub to see name in backtrace for easier debugging. @see Thread::run() */
 protected:
	virtual void
	run()
	{
		Thread::run();
	}

 private:
	// for MetricsSupplier
	virtual std::list<io::prometheus::client::MetricFamily> metrics();
};

#endif
