/***************************************************************************
 *  print_action_executor.cpp - A simple action executor for printing
 *
 *  Created: Tue 05 Nov 2019 14:38:48 CET 14:38
 *  Copyright  2019  Till Hofmann <hofmann@kbsg.rwth-aachen.de>
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

#include "print_action_executor.h"

#include "utils.h"

#include <golog++/model/action.h>
#include <golog++/model/effect_axiom.h>
#include <golog++/model/fluent.h>
#include <logging/logger.h>

namespace fawkes {
namespace gpp {
/** @class PrintActionExecutor
   * A Golog++ action executor that just prints a message.
   * @author Till Hofmann
   */

/** Constructor.
 * Initialize the executor to print with the given logger.
 * @param logger The logger to send messages to
 */
PrintActionExecutor::PrintActionExecutor(Logger *logger) : ActionExecutor(logger)
{
	auto action_scope = new gologpp::Scope(gologpp::global_scope());
	auto message_var  = action_scope->get_var(gologpp::VarDefinitionMode::FORCE,
                                           gologpp::StringType::name(),
                                           "message");
	auto level_var    = action_scope->get_var(gologpp::VarDefinitionMode::FORCE,
                                         gologpp::SymbolType::name(),
                                         "log_level");
	std::vector<std::shared_ptr<gologpp::Variable>> params = {level_var, message_var};
	gologpp::global_scope().define_global<gologpp::Action>(
	  action_scope, std::string{}, "msg", params, boost::none, boost::none, boost::none, boost::none);
	gologpp::global_scope().define_domain("log_level",
	                                      gologpp::StringType::name(),
	                                      gologpp::Domain());
	level_var->set_domain("log_level");
}

/** Destructor. */
PrintActionExecutor::~PrintActionExecutor()
{
}

bool
PrintActionExecutor::can_execute_activity(std::shared_ptr<gologpp::Activity> activity) const
{
	return activity->mapped_name() == "msg";
}

void
PrintActionExecutor::start(std::shared_ptr<gologpp::Activity> activity)
{
	logger_->log_info("Golog++",
	                  "%s",
	                  static_cast<std::string>(activity->mapped_arg_value("message")).c_str());
}

void
PrintActionExecutor::stop(std::shared_ptr<gologpp::Grounding<gologpp::Action>> activity)
{
	logger_->log_error("PrintActionExecutor", "Cannot stop printing a message!");
}
} // namespace gpp
} // namespace fawkes
