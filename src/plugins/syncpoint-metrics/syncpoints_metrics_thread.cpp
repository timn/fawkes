/***************************************************************************
 *  metrics_syncpoints_thread.cpp - collect metrics for syncpoints
 *
 *  Created: Sat Jul 29 22:53:42 2017 (Aonami Line from Kinjo Futo to Nagoya)
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

#include "syncpoints_metrics_thread.h"

#include <core/threading/mutex_locker.h>

using namespace fawkes;

/** @class SyncPointsMetricsThread "metrics_syncpoints_thread.h"
 * Thread to collect sync points metrics.
 * @author Tim Niemueller
 */

/** Constructor. */
SyncPointsMetricsThread::SyncPointsMetricsThread()
    : Thread("SyncPointsSyncPointsMetricsThread", Thread::OPMODE_WAITFORWAKEUP), MetricsAspect(this)
{
}

/** Destructor. */
SyncPointsMetricsThread::~SyncPointsMetricsThread() {}

void
SyncPointsMetricsThread::init()
{
}

void
SyncPointsMetricsThread::finalize()
{
}

void
SyncPointsMetricsThread::loop()
{
}

std::list<io::prometheus::client::MetricFamily>
SyncPointsMetricsThread::metrics()
{
	std::list<io::prometheus::client::MetricFamily> rv;

	return rv;
}
