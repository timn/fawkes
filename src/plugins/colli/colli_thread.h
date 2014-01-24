
/***************************************************************************
 *  colli_thread.h - Fawkes Colli Thread
 *
 *  Created: Wed Oct 16 18:00:00 2013
 *  Copyright  2013  Bahram Maleki-Fard
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

#ifndef __PLUGINS_COLLI_COLLI_THREAD_H_
#define __PLUGINS_COLLI_COLLI_THREAD_H_

#include <core/threading/thread.h>
#include <aspect/blocked_timing.h>
#include <aspect/clock.h>
#include <aspect/configurable.h>
#include <aspect/logging.h>
#include <aspect/blackboard.h>
#include <aspect/tf.h>

#include <utils/time/clock.h>
#include <utils/math/types.h>
#include <utils/math/angle.h>

// Colli States
enum ColliState
{
  NothingToDo,          // Indicating that nothing is to do
  OrientAtTarget,       // Indicating that the robot is at target and has to orient
  DriveToOrientPoint,   // Drive to the orientation point
  DriveToTarget,        // Drive to the target
};

namespace fawkes
{
  class MotorInterface;
  class Laser360Interface;
  class NavigatorInterface;

  class Laser;
  class CLaserOccupancyGrid;
  class CSearch;

  class CSelectDriveMode;
  class CBaseMotorInstruct;

}

class ColliVisualizationThread;

class ColliThread
: public fawkes::Thread,
  public fawkes::BlockedTimingAspect,
  public fawkes::ClockAspect,
  public fawkes::LoggingAspect,
  public fawkes::ConfigurableAspect,
  public fawkes::TransformAspect,
  public fawkes::BlackBoardAspect
{
 public:
  ColliThread();
  virtual ~ColliThread();

  virtual void init();
  virtual void loop();
  virtual void finalize();

  virtual void set_vis_thread(ColliVisualizationThread* vis_thread);
 private:
  /* ************************************************************************ */
  /* PRIVATE OBJECT VARIABLES                                                 */
  /* ************************************************************************ */
  /*
   * This is a short list of types that have been transformed from RCSoftX->fawkes:
   *
   * Mopo_Client          ->  MotorInterface      (motor data)
   * Laser_Client         ->  Laser360Interface   (laser data)
   * Colli_Target_Client  ->  NavigatorInterface  (colli target)
   * Colli_Data_Server    ->  NavigatorInterface  (colli data)
   *
   * Point                ->  cart_coord_2d_t     (point with 2 floats)
   */
  fawkes::MotorInterface*         m_pMopoObj;          // MotorObject
  fawkes::Laser360Interface*      m_pLaserScannerObj;  // LaserScannerObject
  fawkes::NavigatorInterface*     m_pColliTargetObj;   // TargetObject
  fawkes::NavigatorInterface*     m_pColliDataObj;     // Colli Data Object

  fawkes::Laser*                  m_pLaser;            // laser interface for easy use
  fawkes::CLaserOccupancyGrid*    m_pLaserOccGrid;     // the grid to drive on
  fawkes::CSearch*                m_pSearch;           // our plan module which calculates the info

  fawkes::CSelectDriveMode*       m_pSelectDriveMode;  // the drive mode selection module
  fawkes::CBaseMotorInstruct*     m_pMotorInstruct;    // the motor instructor module

  ColliVisualizationThread*       vis_thread_;         // the VisualizationThread

  /* ************************************************************************ */
  /* PRIVATE VARIABLES THAT HAVE TO BE HANDLED ALL OVER THE MODULE            */
  /* ************************************************************************ */
  fawkes::point_t  m_RoboGridPos;           // the robots position in the grid
  fawkes::point_t  m_LaserGridPos;          // the laser its position in the grid ( not equal to robopos!!! )
  fawkes::point_t  m_TargetGridPos;         // the targets position in the grid

  fawkes::point_t m_LocalGridTarget;        // the local target in grid
  fawkes::point_t m_LocalGridTrajec;        // the local trajec in grid

  fawkes::cart_coord_2d_t  m_LocalTarget;   // the local target (relative)
  fawkes::cart_coord_2d_t  m_LocalTrajec;   // the local trajec (relative)

  float m_ProposedTranslation;  // the proposed translation that should be realized in MotorInstruct
  float m_ProposedRotation;     // the proposed rotation that should be realized in MotorInstruct

  ColliState m_ColliStatus;     // representing current colli status

  float m_oldTargetX, m_oldTargetY, m_oldTargetOri;   // for init problems
  float m_TargetPointX, m_TargetPointY;               // for Update

  float m_OldX, m_OldY, m_OldOri;  // for updating occgrid and performing pipe compensation

  int escape_count;                // count escaping behaviour

  // Config file constants that are read at the beginning
  int m_ColliFrequency;                          // frequency of the colli
  float m_OccGridHeight, m_OccGridWidth;         // occgrid field sizes
  int m_OccGridCellHeight, m_OccGridCellWidth;   // occgrid cell sizes
  float m_MaximumRoboIncrease;                   // maximum increasement of the robots size
  bool cfg_obstacle_inc_;                        // indicator if obstacles should be increased or not

  bool  cfg_visualize_idle_;      /**< Defines if visualization should run when robot is idle without a target. */

  float cfg_min_rot_;             /**< The minimum rotation speed. */
  float cfg_min_drive_dist_;      /**< The minimum distance to drive straight to target */
  float cfg_min_long_dist_drive_; /**< The minimum distance to drive straight to target in a long distance */
  float cfg_min_long_dist_prepos_;/**< The minimum distance to drive to a pre-positino of a target in long distance */
  float cfg_min_rot_dist_;        /**< The minimum rotation distance to rotate, when at target */
  float cfg_target_pre_pos_;      /**< Distance to target pre-position (only if ColliState == DriveToOrientPoint) */

  std::string cfg_frame_base_;    /**< The frame of the robot's base */
  std::string cfg_frame_laser_;   /**< The frame of the laser */

  std::string cfg_iface_motor_;   /**< The ID of the MotorInterface */
  std::string cfg_iface_laser_;   /**< The ID of the LaserInterface */

  // stop on target stuff
  std::vector< float > m_oldAnglesToTarget;      // the old angles to the target

  fawkes::cart_coord_2d_t laser_to_base_; /**< The distance from laser to base */
  bool laser_to_base_valid_;              /**< Do we have a valid distance from laser to base? */

  // manually adjust the frequency on how often the loop should be processed
  unsigned int loop_count_;
  unsigned int loop_count_trigger_;


  /* ************************************************************************ */
  /* PRIVATE METHODS                                                          */
  /* ************************************************************************ */

  /// Register all BB-Interfaces at the Blackboard.
  void RegisterAtBlackboard();

  /// Initialize all modules used by the Colli
  void InitializeModules();

  /// Get the newest values from the blackboard
  void UpdateBB();

  /// Check, in what state the colli is, and what to do
  void UpdateColliStateMachine();

  /// Calculate all information out of the updated blackboard data
  void UpdateOwnModules();

  /// Check, if we have to do escape mode, or if we have to drive the ordinary way ;-)
  bool CheckEscape();

};

#endif

