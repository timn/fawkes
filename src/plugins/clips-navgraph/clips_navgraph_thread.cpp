
/***************************************************************************
 *  clips_navgraph_thread.cpp -  NavGraph feature for CLIPS
 *
 *  Created: Wed Oct 09 19:27:41 2013
 *  Copyright  2006-2013  Tim Niemueller [www.niemueller.de]
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

#include "clips_navgraph_thread.h"

#include <utils/graph/topological_map_graph.h>

#include <clipsmm.h>

using namespace fawkes;

/** @class ClipsNavGraphThread "clips-protobuf-thread.h"
 * Provide protobuf functionality to CLIPS environment.
 * @author Tim Niemueller
 */

/** Constructor. */
ClipsNavGraphThread::ClipsNavGraphThread()
  : Thread("ClipsNavGraphThread", Thread::OPMODE_WAITFORWAKEUP),
    CLIPSFeature("navgraph"), CLIPSFeatureAspect(this)
{
}


/** Destructor. */
ClipsNavGraphThread::~ClipsNavGraphThread()
{
}


void
ClipsNavGraphThread::init()
{
}


void
ClipsNavGraphThread::finalize()
{
}


void
ClipsNavGraphThread::clips_context_init(const std::string &env_name,
					LockPtr<CLIPS::Environment> &clips)
{
  logger->log_info(name(), "Called to initialize environment %s", env_name.c_str());

  clips.lock();
  clips->batch_evaluate(SRCDIR"/clips/navgraph.clp");
  clips_navgraph_load(clips);
  clips.unlock();
}

void
ClipsNavGraphThread::clips_context_destroyed(const std::string &env_name)
{
  logger->log_info(name(), "Removing environment %s", env_name.c_str());
}

void
ClipsNavGraphThread::clips_navgraph_load(LockPtr<CLIPS::Environment> &clips)
{
  try {
    TopologicalMapNode root_node                 = navgraph->root_node();
    const std::vector<TopologicalMapNode> &nodes = navgraph->nodes();
    const std::vector<TopologicalMapEdge> &edges = navgraph->edges();

    clips->assert_fact_f("(navgraph (name \"%s\") (root \"%s\"))",
			 navgraph->name().c_str(), root_node.name().c_str());

    for (auto n : nodes) {
      std::string props_string;
      const std::map<std::string, std::string> &properties = n.properties();
      for (auto p : properties) {
	props_string += " \"" + p.first + "\" \"" + p.second + "\"";
      }
      clips->assert_fact_f("(navgraph-node (name \"%s\") (pos %f %f) (properties %s))",
			   n.name().c_str(), n.x(), n.y(), props_string.c_str());
    }

    for (auto e : edges) {
      std::string props_string;
      const std::map<std::string, std::string> &properties = e.properties();
      for (auto p : properties) {
	props_string += " \"" + p.first + "\" \"" + p.second + "\"";
      }
      clips->assert_fact_f("(navgraph-edge (from \"%s\") (to \"%s\") (directed %s) "
			   "(properties \"%s\"))",
			   e.from().c_str(), e.to().c_str(),
			   e.is_directed() ? "TRUE" : "FALSE", props_string.c_str());
    }

  } catch (Exception &e) {
    logger->log_warn(name(), "Failed to assert navgraph, exception follows");
    logger->log_warn(name(), e);
    clips->assert_fact_f("(navgraph-load-fail %s)", *(e.begin()));
  }
}

void
ClipsNavGraphThread::loop()
{
}