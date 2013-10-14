
/***************************************************************************
 *  topological_map_graph.cpp - Topological graph
 *
 *  Created: Fri Sep 21 15:55:49 2012
 *  Copyright  2012  Tim Niemueller [www.niemueller.de]
 ****************************************************************************/

/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version. A runtime exception applies to
 *  this software (see LICENSE.GPL_WRE file mentioned below for details).
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  Read the full text in the LICENSE.GPL_WRE file in the doc directory.
 */

#include <utils/graph/topological_map_graph.h>
#include <core/exception.h>

#include <algorithm>
#include <list>
#include <set>
#include <queue>
#include <cmath>
#include <cstdio>

namespace fawkes {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

/** @class TopologicalMapGraph <utils/graph/topological_map_graph.h>
 * Topological map graph.
 * This class represents a topological graph using 2D map coordinates
 * with nodes and edges. Both can be annotated with certain free-form
 * properties which can be used at run-time for example to instruct
 * the robot behavior.
 *
 * The class supports change listeners. These are called whenever the graph
 * is changed, that is if a node or edge is added or if the graph is assigned
 * from another one (i.e. graph := new_graph).
 *
 * This class is based on KBSG RCSoft's MapGraph but has been
 * abstracted and improved.
 * @author Tim Niemueller
 */

/** Constructor.
 * @param graph_name Name of the graph, for example to handle multiple
 * graphs, e.g. for multiple levels of a building.
 */
TopologicalMapGraph::TopologicalMapGraph(std::string graph_name)
{
  graph_name_ = graph_name;
}


/** Virtual empty destructor. */
TopologicalMapGraph::~TopologicalMapGraph()
{
}


/** Assign/copy structures from another graph.
 * This method will remove internal data like root node, nodes, and edges
 * and copy the data from the passed instance. The change listeners will
 * not be copied. The assignment operator will trigger all registered
 * change listeners to be called.
 * @param g graph from which to copy the data
 * @return reference to this instance
 */
TopologicalMapGraph &
TopologicalMapGraph::operator=(const TopologicalMapGraph &g)
{
  root_node_  = g.root_node_;
  graph_name_ = g.graph_name_;
  nodes_.clear();
  nodes_      = g.nodes_;
  edges_.clear();
  edges_      = g.edges_;

  notify_of_change();

  return *this;
}

/** Get graph name.
 * @return graph name
 */
std::string
TopologicalMapGraph::name() const
{
  return graph_name_;
}


/** Get nodes of the graph.
 * @return const reference to vector of nodes of this graph
 */
const std::vector<TopologicalMapNode> &
TopologicalMapGraph::nodes() const
{
  return nodes_;
}


/** Get edges of the graph.
 * @return const reference to vector of edges of this graph
 */
const std::vector<TopologicalMapEdge> &
TopologicalMapGraph::edges() const
{
  return edges_;
}


/** Get a specified node.
 * @param name name of the node to get
 * @return the node representation of the searched node, if not
 * found returns an invalid node.
 */
TopologicalMapNode
TopologicalMapGraph::node(std::string name) const
{
  std::vector<TopologicalMapNode>::const_iterator i;
  for (i = nodes_.begin(); i != nodes_.end(); ++i) {
    if (i->name() == name)  return *i;
  }
  return TopologicalMapNode();
}


/** Get the root node of the graph.
 * @return root node
 */
TopologicalMapNode
TopologicalMapGraph::root_node() const
{
  return root_node_;
}


/** Get node closest to a specified point with a certain property.
 * This search does *NOT* consider unconnected nodes.
 * @param pos_x X coordinate in global (map) frame
 * @param pos_y X coordinate in global (map) frame
 * @param property property the node must have to be considered,
 * empty string to not check for any property
 * @return node closest to the given point in the global frame, or an
 * invalid node if such a node cannot be found
 */
TopologicalMapNode
TopologicalMapGraph::closest_node(float pos_x, float pos_y, std::string property)
{
  return closest_node(pos_x, pos_y, false, property);
}


/** Get node closest to a specified point with a certain property.
 * This search *does* consider unconnected nodes.
 * @param pos_x X coordinate in global (map) frame
 * @param pos_y X coordinate in global (map) frame
 * @param property property the node must have to be considered,
 * empty string to not check for any property
 * @return node closest to the given point in the global frame, or an
 * invalid node if such a node cannot be found
 */
TopologicalMapNode
TopologicalMapGraph::closest_node_with_unconnected(float pos_x, float pos_y,
						   std::string property)
{
  return closest_node(pos_x, pos_y, true, property);
}

/** Get node closest to another node with a certain property.
 * This search does *NOT* consider unconnected nodes.
 * @param node_name the name of the node from which to start
 * @param property property the node must have to be considered,
 * empty string to not check for any property
 * @return node closest to the given point in the global frame, or an
 * invalid node if such a node cannot be found. The node will obviously
 * not be the node with the name @p node_name.
 */
TopologicalMapNode
TopologicalMapGraph::closest_node_to(std::string node_name,
						      std::string property)
{
  return closest_node_to(node_name, false, property);
}

/** Get node closest to another node with a certain property.
 * This search *does* consider unconnected nodes.
 * @param node_name the name of the node from which to start
 * @param property property the node must have to be considered,
 * empty string to not check for any property
 * @return node closest to the given point in the global frame, or an
 * invalid node if such a node cannot be found. The node will obviously
 * not be the node with the name @p node_name.
 */
TopologicalMapNode
TopologicalMapGraph::closest_node_to_with_unconnected(std::string node_name,
						      std::string property)
{
  return closest_node_to(node_name, true, property);
}

/** Get node closest to a specified point with a certain property.
 * @param pos_x X coordinate in global (map) frame
 * @param pos_y X coordinate in global (map) frame
 * @param consider_unconnected consider unconnected node for the search
 * of the closest node
 * @param property property the node must have to be considered,
 * empty string to not check for any property
 * @return node closest to the given point in the global frame, or an
 * invalid node if such a node cannot be found
 */
TopologicalMapNode
TopologicalMapGraph::closest_node(float pos_x, float pos_y, bool consider_unconnected,
                                  std::string property)
{
  std::vector<TopologicalMapNode> nodes = search_nodes(property);

  float min_dist = HUGE;

  std::vector<TopologicalMapNode>::iterator i;
  std::vector<TopologicalMapNode>::iterator elem = nodes.begin();
  for (i = nodes.begin(); i != nodes.end(); ++i) {
    if (! consider_unconnected && i->unconnected())  continue;

    float dx   = i->x() - pos_x;
    float dy   = i->y() - pos_y;
    float dist = sqrtf(dx * dx + dy * dy);
    if (sqrtf(dx * dx + dy * dy) < min_dist) {
      min_dist = dist;
      elem = i;
    }
  }

  if (elem == nodes.end()) {
    return TopologicalMapNode();
  } else {
    return *elem;
  }
}

/** Get node closest to another node with a certain property.
 * @param node_name the name of the node from which to start
 * @param consider_unconnected consider unconnected node for the search
 * of the closest node
 * @param property property the node must have to be considered,
 * empty string to not check for any property
 * @return node closest to the given point in the global frame, or an
 * invalid node if such a node cannot be found. The node will obviously
 * not be the node with the name @p node_name.
 */
TopologicalMapNode
TopologicalMapGraph::closest_node_to(std::string node_name, bool consider_unconnected,
				     std::string property)
{
  TopologicalMapNode n = node(node_name);
  std::vector<TopologicalMapNode> nodes = search_nodes(property);

  float min_dist = HUGE;

  std::vector<TopologicalMapNode>::iterator i;
  std::vector<TopologicalMapNode>::iterator elem = nodes.begin();
  for (i = nodes.begin(); i != nodes.end(); ++i) {
    if (! consider_unconnected && i->unconnected())  continue;

    float dx   = i->x() - n.x();
    float dy   = i->y() - n.y();
    float dist = sqrtf(dx * dx + dy * dy);
    if ((sqrtf(dx * dx + dy * dy) < min_dist) && (i->name() != node_name)) {
      min_dist = dist;
      elem = i;
    }
  }

  if (elem == nodes.end()) {
    return TopologicalMapNode();
  } else {
    return *elem;
  }
}


/** Check if a certain node exists.
 * @param name name of the node to look for
 * @return true if a node with the given name exists, false otherwise
 */
bool
TopologicalMapGraph::node_exists(std::string name) const
{
  std::vector<TopologicalMapNode>::const_iterator i;
  for (i = nodes_.begin(); i != nodes_.end(); ++i) {
    if (i->name() == name)  return true;
  }
  return false;
}

/** Search nodes for given property.
 * @param property property name to look for
 * @return vector of nodes having the specified property
 */
std::vector<TopologicalMapNode>
TopologicalMapGraph::search_nodes(std::string property)
{
  if (property == "") {
    return nodes();
  } else {
    std::vector<TopologicalMapNode> rv;

    std::vector<TopologicalMapNode>::iterator i;
    for (i = nodes_.begin(); i != nodes_.end(); ++i) {
      if ( i->has_property(property) )  rv.push_back(*i);
    }

    return rv;
  }
}


/** Set root node name.
 * @param node_id name of the root node
 */
void
TopologicalMapGraph::set_root(std::string node_id)
{
  root_node_ = node(node_id);
  notify_of_change();
}


/** Add a node.
 * @param node node to add
 */
void
TopologicalMapGraph::add_node(TopologicalMapNode node)
{
  nodes_.push_back(node);
  notify_of_change();
}

/** Add an edge
 * @param edge edge to add
 */
void
TopologicalMapGraph::add_edge(TopologicalMapEdge edge)
{
  edges_.push_back(edge);
  notify_of_change();
}


/** Get nodes reachable from specified nodes.
 * @param node_name name of the node to get reachable nodes for
 * @return vector of names of nodes reachable from the specified node
 */
std::vector<std::string>
TopologicalMapGraph::reachable_nodes(std::string node_name) const
{
  std::vector<std::string> rv;

  TopologicalMapNode n = node(node_name);
  if (! n.is_valid())  return rv;

  std::vector<TopologicalMapEdge>::const_iterator i;
  for (i = edges_.begin(); i != edges_.end(); ++i) {
    if (i->is_directed()) {
      if (i->from() == node_name) {
        rv.push_back(i->to());
      }
    } else {
      if (i->from() == node_name) {
        rv.push_back(i->to());
      } else if (i->to() == node_name) {
        rv.push_back(i->from());
      }
    }
  }

  std::sort(rv.begin(), rv.end());
  std::unique(rv.begin(), rv.end());
  return rv;
}


/** Make sure each node exists only once. */
void
TopologicalMapGraph::assert_unique_nodes()
{
  std::list<std::string> names;
  std::vector<TopologicalMapNode>::iterator i;
  for (i = nodes_.begin(); i != nodes_.end(); ++i) {
    names.push_back(i->name());
  }
  names.sort();
  std::list<std::string>::iterator n;
  std::string last_name = "";
  for (n = names.begin(); n != names.end(); ++n) {
    if (*n == last_name) {
      throw Exception("Node '%s' exists at least twice", last_name.c_str());
    }
    last_name = *n;
  }
}

/** Make sure each edge exists only once. */
void
TopologicalMapGraph::assert_unique_edges()
{
  for (size_t i = 0; i < edges_.size(); ++i) {
    for (size_t j = i+1; j < edges_.size(); ++j) {
      if (edges_[i].from() == edges_[j].from() &&
          edges_[i].to() == edges_[j].to())
      {
        throw Exception("Edge '%s - %s' is defined twice",
                        edges_[i].from().c_str(), edges_[i].to().c_str());
      }
      if (edges_[i].from() == edges_[j].to() &&
          edges_[i].to() == edges_[j].from() &&
          (!edges_[i].is_directed() || !edges_[j].is_directed()))
      {
        throw Exception("Edge '%s - %s' and '%s - %s' both exist "
                        "and at least one is not directed",
                        edges_[i].from().c_str(), edges_[i].to().c_str(),
                        edges_[j].from().c_str(), edges_[j].to().c_str());
      }
    }
  }
}

/** Make sure each node in the edges exists. */
void
TopologicalMapGraph::assert_valid_edges()
{
  for (size_t i = 0; i < edges_.size(); ++i) {
    if (! node_exists(edges_[i].from())) {
      throw Exception("Node '%s' for edge '%s' -> '%s' does not exist",
                      edges_[i].from().c_str(), edges_[i].from().c_str(),
                      edges_[i].to().c_str());
    }

    if (! node_exists(edges_[i].to())) {
      throw Exception("Node '%s' for edge '%s' -> '%s' does not exist",
                      edges_[i].to().c_str(), edges_[i].from().c_str(),
                      edges_[i].to().c_str());
    }
  }
}



void
TopologicalMapGraph::assert_connected()
{
  std::set<std::string> traversed;
  std::set<std::string> nodeset;
  std::queue<TopologicalMapNode> q;
  q.push(nodes_.front());

  while (! q.empty()) {
    TopologicalMapNode &n = q.front();
    traversed.insert(n.name());

    const std::vector<std::string> & reachable = n.reachable_nodes();

    if (n.unconnected() && !reachable.empty()) {
      throw Exception("Node %s is marked unconnected but nodes are reachable from it",
		      n.name().c_str());
    }
    std::vector<std::string>::const_iterator r;
    for (r = reachable.begin(); r != reachable.end(); ++r) {
      TopologicalMapNode target(node(*r));
      if (target.unconnected()) {
	throw Exception("Node %s is marked unconnected but is reachable from node %s\n",
			target.name().c_str(), n.name().c_str());
      }
      if (traversed.find(*r) == traversed.end()) q.push(node(*r));
    }
    q.pop();
  }

  std::vector<TopologicalMapNode>::iterator n;
  for (n = nodes_.begin(); n != nodes_.end(); ++n) {
    nodeset.insert(n->name());
  }

  if (traversed.size() != nodeset.size()) {
    std::set<std::string> nodediff;
    std::set_difference(nodeset.begin(), nodeset.end(),
			traversed.begin(), traversed.end(),
			std::inserter(nodediff, nodediff.begin()));

    // the nodes might be unconnected, in which case it is not
    // an error that they were mentioned. But it might still be
    // a problem if there was a *directed* outgoing edge from the
    // unconnected node, which we couldn't have spotted earlier
    std::set<std::string>::const_iterator ud = nodediff.begin();
    while (ud != nodediff.end()) {
      TopologicalMapNode udnode(node(*ud));
      if (udnode.unconnected()) {
	// it's ok to be in the disconnected set, but check if it has any
	// reachable nodes which is forbidden
	if (! udnode.reachable_nodes().empty()) {
	  throw Exception("Node %s is marked unconnected but nodes are reachable from it",
			  ud->c_str());
	}
#if __cplusplus > 201100L || defined(__GXX_EXPERIMENTAL_CXX0X__)
	ud = nodediff.erase(ud);
#else
	std::set<std::string>::const_iterator delit = ud;
	++ud;
	nodediff.erase(delit);
#endif
      } else {
	++ud;
      }
    }

    if (! nodediff.empty()) {
      std::set<std::string>::iterator d = nodediff.begin();
      std::string disconnected = *d;
      for (++d; d != nodediff.end(); ++d) {
	disconnected += ", " + *d;
      }
      throw Exception("The graph is not fully connected, "
		      "cannot reach (%s) from '%s' for example",
		      disconnected.c_str(), nodes_[0].name().c_str());
    }
  }
}



/** Calculate eachability relations.
 * This will set the directly reachable nodes on each
 * of the graph nodes. 
 */
void
TopologicalMapGraph::calc_reachability()
{
  assert_unique_nodes();
  assert_unique_edges();
  assert_valid_edges();
  std::vector<TopologicalMapNode>::iterator i;
  for (i = nodes_.begin(); i != nodes_.end(); ++i) {
    i->set_reachable_nodes(reachable_nodes(i->name()));
  }
  assert_connected();
}


/** Add a change listener.
 * @param listener listener to add
 */
void
TopologicalMapGraph::add_change_listener(ChangeListener *listener)
{
  change_listeners_.push_back(listener);
}

/** Remove a change listener.
 * @param listener listener to remove
 */
void
TopologicalMapGraph::remove_change_listener(ChangeListener *listener)
{
  change_listeners_.remove(listener);
}

/** Notify all listeners of a change. */
void
TopologicalMapGraph::notify_of_change() throw()
{
  std::list<ChangeListener *> tmp_listeners = change_listeners_;

  std::list<ChangeListener *>::iterator l;
  for (l = tmp_listeners.begin(); l != tmp_listeners.end(); ++l) {
    (*l)->graph_changed();
  }
}

/** @class TopologicalMapGraph::ChangeListener <utils/graph/topological_map_graph.h>
 * Topological graph change listener.
 * @author Tim Niemueller
 *
 * @fn void TopologicalMapGraph::ChangeListener::graph_changed() throw() = 0
 * Function called if the graph has been changed.
 */

/** Virtual empty destructor. */
TopologicalMapGraph::ChangeListener::~ChangeListener()
{
}


} // end of namespace fawkes
