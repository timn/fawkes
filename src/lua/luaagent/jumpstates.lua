
------------------------------------------------------------------------
--  jumpstates.lua - LuaAgent JumpState
--
--  Created: Fri Jan 02 15:31:45 2009
--  Copyright  2008-2009  Tim Niemueller [www.niemueller.de]
--
--  $Id$
--
------------------------------------------------------------------------

--  This program is free software; you can redistribute it and/or modify
--  it under the terms of the GNU General Public License as published by
--  the Free Software Foundation; either version 2 of the License, or
--  (at your option) any later version.
--
--  This program is distributed in the hope that it will be useful,
--  but WITHOUT ANY WARRANTY; without even the implied warranty of
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--  GNU Library General Public License for more details.
--
--  Read the full text in the LICENSE.GPL file in the doc directory.

require("fawkes.modinit")

--- Jump states to build Hybrid State Machines (HSM) for skills.
-- @author Tim Niemueller
module(..., fawkes.modinit.module_init)

require("fawkes.fsm.jumpstate")
require("luaagent.skillqueue")
local skillstati = require("skiller.skillstati")

-- Convenience shortcuts
local JumpState     = fawkes.fsm.jumpstate.JumpState
local SkillQueue    = luaagent.skillqueue.SkillQueue


--- @class SkillJumpState
-- Skill jump states to build up Hybrid State Machines (HSM) specifically for
-- the use in skills.
-- Note: Other than for FSM states, the HSM states ignore the return value of the
-- loop() function. You need to add transitions with jump conditions for state
-- transitions and cannot return a state to switch to after the loop.
-- @author Tim Niemueller
AgentSkillExecJumpState = { add_transition     = JumpState.add_transition,
			    add_precondition   = JumpState.add_precondition,
			    add_precond_trans  = JumpState.add_precond_trans,
			    get_transitions    = JumpState.get_transitions,
			    clear_transitions  = JumpState.clear_transitions,
			    try_transitions    = JumpState.try_transitions,
			    last_transition    = JumpState.last_transition,
			    init               = JumpState.init,
			    loop               = JumpState.loop,
			    exit               = JumpState.exit,
			    reset              = JumpState.reset
			  }


--- Fires if skill(s) finished successfully.
function AgentSkillExecJumpState:jumpcond_final()
   return self.skill_status == skillstati.S_FINAL
end


--- Fires if any called skill failed.
function AgentSkillExecJumpState:jumpcond_failure()
   return self.skill_status == skillstati.S_FAILED
end


--- Create new state.
-- @param o table with initializations for the object.
-- @return Initialized FSM state
function AgentSkillExecJumpState:new(o)
   assert(o, "AgentSkillExecJumpState requires a table as argument")
   assert(o.name, "AgentSkillExecJumpState requires a name")
   assert(o.final_state, "AgentSkillExecJumpState requires state to go to on success for state " .. o.name)
   assert(o.failure_state, "AgentSkillExecJumpState requires state to go to on failure for state " .. o.name)
   assert(not getmetatable(o), "Meta table already set for object")
   setmetatable(o, self)
   self.__index = self

   o.transitions = o.transitions or {}
   assert(type(o.transitions) == "table", "Transitions for " .. o.name .. " not a table")

   local skills = o.skills or {}
   o.skill_queue = SkillQueue:new{name=o.name, skills=skills}

   o.skill_status = skillstati.S_RUNNING

   o.final_transition   = JumpState.add_transition(o, o.final_state, o.jumpcond_final, "Skill(s) succeeded")
   o.failure_transition = JumpState.add_transition(o, o.failure_state, o.jumpcond_failure, "Skill(s) failed")

   o:set_transition_labels()

   return o
end


--- Add a skill.
-- @param skillname name of the skill to add
-- @param params skill parameters, an array with elements generated via fixp() and
-- fsmp() functions.
-- @see SkillQueue:add_skill()
function AgentSkillExecJumpState:add_skill(skillname, params)
   self.skill_queue:add_skill(skillname, params)
end


function AgentSkillExecJumpState:set_transition_labels()
   if #self.skills > 0 then
      local snames = {}
      for _,s in ipairs(self.skills) do
	 table.insert(snames, s[1])
      end
      self.failure_transition.description = table.concat(snames, " or ") .. " failed"
      self.final_transition.description   = table.concat(snames, " and ") .. " succeeded"
   else
      self.failure_transition.description = "S_FAILED"
      self.final_transition.description   = "S_FINAL"
   end
   self.fsm:mark_changed()
end

--- Init state.
-- Calls init(). if params were passed (table containing parameters
-- suitable for SkillQueue:set_params()) they are passed to the internal
-- skill queue. The skill queue is executed and intermediate skill status
-- is S_RUNNING.
function AgentSkillExecJumpState:do_init(params)
   local rv = { self:try_transitions(self.preconditions) }
   if next(rv) then return unpack(rv) end

   self:init()
   if params and type(params) == "table" then
      self.skill_queue:set_params(params)
   end
   self.skill_queue:execute(self.skiller)
   self.skill_status = skillstati.S_RUNNING
   self:set_transition_labels()
end

--- On leaving state.
-- Stops executed skills and resets the skill queue. Calls state's exit()
-- method.
function AgentSkillExecJumpState:do_exit()
   self.skill_queue:stop()
   self.skill_queue:reset()
   self:exit()
end

--- Execute loop.
function AgentSkillExecJumpState:do_loop()
   self.skill_status = self.skill_queue:status()

   self:loop()

   return self:try_transitions()
end