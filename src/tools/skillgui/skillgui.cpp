
/***************************************************************************
 *  skillgui.cpp - Skill GUI
 *
 *  Created: Mon Nov 03 13:37:33 2008
 *  Copyright  2008  Tim Niemueller [www.niemueller.de]
 *
 *  $Id$
 *
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

#include "skillgui.h"
#ifdef USE_PAPYRUS
#  include "graph_viewport.h"
#else
#  include "graph_drawing_area.h"
#endif

#include <utils/system/argparser.h>
#include <blackboard/remote.h>
#include <interfaces/SkillerInterface.h>
#include <interfaces/SkillerDebugInterface.h>
#include <netcomm/fawkes/client.h>

#include <gui_utils/logview.h>
#include <gui_utils/throbber.h>
#include <gui_utils/service_chooser_dialog.h>
#include <gui_utils/interface_dispatcher.h>
#include <gui_utils/plugin_tree_view.h>

#include <cstring>

#include <gvc.h>

using namespace fawkes;

#define ACTIVE_SKILL "Active Skill"

/** @class SkillGuiGtkWindow "skillgui.h"
 * Skill GUI main window.
 * The Skill GUI provides shows Skiller log messages and allows for
 * executing skills.
 * @author Tim Niemueller
 */

/** Constructor.
 * @param cobject C base object
 * @param refxml Glade XML
 */
SkillGuiGtkWindow::SkillGuiGtkWindow(BaseObjectType* cobject,
				 const Glib::RefPtr<Gnome::Glade::Xml> &refxml)
  : Gtk::Window(cobject)
{
  bb = NULL;
  __skiller_if = NULL;
  __skdbg_if = NULL;
  __agdbg_if = NULL;

  __gconf = Gnome::Conf::Client::get_default_client();
  __gconf->add_dir(GCONF_PREFIX);

  refxml->get_widget_derived("trv_log", __logview);
  refxml->get_widget("tb_connection", tb_connection);
  refxml->get_widget("tb_continuous", tb_continuous);
  refxml->get_widget("tb_exit", tb_exit);
  refxml->get_widget("cbe_skillstring", cbe_skillstring);
  refxml->get_widget("but_exec", but_exec);
  refxml->get_widget("but_stop", but_stop);
  refxml->get_widget("lab_status", lab_status);
  refxml->get_widget("lab_alive", lab_alive);
  refxml->get_widget("lab_continuous", lab_continuous);
  refxml->get_widget("lab_skillstring", lab_skillstring);
  refxml->get_widget("lab_error", lab_error);
  refxml->get_widget("scw_graph", scw_graph);
  //refxml->get_widget("drw_graph", drw_graph);
  refxml->get_widget("ntb_tabs", ntb_tabs);
  refxml->get_widget("tb_skiller", tb_skiller);
  refxml->get_widget("tb_agent", tb_agent);
  refxml->get_widget("tb_graphlist", tb_graphlist);
  refxml->get_widget("tb_controller", tb_controller);
  refxml->get_widget("tb_graphsave", tb_graphsave);
  refxml->get_widget("tb_graphopen", tb_graphopen);
  refxml->get_widget("tb_graphupd", tb_graphupd);
  refxml->get_widget("tb_graphrecord", tb_graphrecord);
  refxml->get_widget("tb_zoomin", tb_zoomin);
  refxml->get_widget("tb_zoomout", tb_zoomout);
  refxml->get_widget("tb_zoomfit", tb_zoomfit);
  refxml->get_widget("tb_zoomreset", tb_zoomreset);

  refxml->get_widget_derived("img_throbber", __throbber);
  refxml->get_widget_derived("trv_plugins",  __trv_plugins);
  
  Gtk::SeparatorToolItem *spacesep;
  refxml->get_widget("tb_spacesep", spacesep);
  spacesep->set_expand();

  __sks_list = Gtk::ListStore::create(__sks_record);
  cbe_skillstring->set_model(__sks_list);
  cbe_skillstring->set_text_column(__sks_record.skillstring);

  __trv_plugins->set_network_client(connection_dispatcher.get_client());
  __trv_plugins->set_gconf_prefix(GCONF_PREFIX);

#ifdef USE_PAPYRUS
  pvp_graph = Gtk::manage(new SkillGuiGraphViewport());
  scw_graph->add(*pvp_graph);
  pvp_graph->show();
#else
  gda = Gtk::manage(new SkillGuiGraphDrawingArea());
  scw_graph->add(*gda);
  gda->show();
#endif

  cb_graphlist = Gtk::manage(new Gtk::ComboBoxText());
  cb_graphlist->append_text(ACTIVE_SKILL);
  cb_graphlist->set_active_text(ACTIVE_SKILL);
  tb_graphlist->add(*cb_graphlist);
  cb_graphlist->show();

  //ntb_tabs->set_current_page(1);

  connection_dispatcher.signal_connected().connect(sigc::mem_fun(*this, &SkillGuiGtkWindow::on_connect));
  connection_dispatcher.signal_disconnected().connect(sigc::mem_fun(*this, &SkillGuiGtkWindow::on_disconnect));

  tb_connection->signal_clicked().connect(sigc::mem_fun(*this, &SkillGuiGtkWindow::on_connection_clicked));
  but_exec->signal_clicked().connect(sigc::mem_fun(*this, &SkillGuiGtkWindow::on_exec_clicked));
  tb_controller->signal_clicked().connect(sigc::mem_fun(*this, &SkillGuiGtkWindow::on_controller_clicked));
  tb_exit->signal_clicked().connect(sigc::mem_fun(*this, &SkillGuiGtkWindow::on_exit_clicked));
  but_stop->signal_clicked().connect(sigc::mem_fun(*this, &SkillGuiGtkWindow::on_stop_clicked));
  tb_continuous->signal_toggled().connect(sigc::mem_fun(*this, &SkillGuiGtkWindow::on_contexec_toggled));
  tb_skiller->signal_toggled().connect(sigc::mem_fun(*this, &SkillGuiGtkWindow::on_skdbg_data_changed));
  tb_skiller->signal_toggled().connect(sigc::bind(sigc::mem_fun(*cb_graphlist, &Gtk::ComboBoxText::set_sensitive),true));
  tb_agent->signal_toggled().connect(sigc::mem_fun(*this, &SkillGuiGtkWindow::on_agdbg_data_changed));
  tb_agent->signal_toggled().connect(sigc::bind(sigc::mem_fun(*cb_graphlist, &Gtk::ComboBoxText::set_sensitive),false));
  cb_graphlist->signal_changed().connect(sigc::mem_fun(*this, &SkillGuiGtkWindow::on_skill_changed));
  tb_graphupd->signal_clicked().connect(sigc::mem_fun(*this, &SkillGuiGtkWindow::on_graphupd_clicked));
#ifdef USE_PAPYRUS
  tb_graphsave->signal_clicked().connect(sigc::mem_fun(*pvp_graph, &SkillGuiGraphViewport::save));
  tb_zoomin->signal_clicked().connect(sigc::mem_fun(*pvp_graph, &SkillGuiGraphViewport::zoom_in));
  tb_zoomout->signal_clicked().connect(sigc::mem_fun(*pvp_graph, &SkillGuiGraphViewport::zoom_out));
  tb_zoomfit->signal_clicked().connect(sigc::mem_fun(*pvp_graph, &SkillGuiGraphViewport::zoom_fit));
  tb_zoomreset->signal_clicked().connect(sigc::mem_fun(*pvp_graph, &SkillGuiGraphViewport::zoom_reset));
#else
  tb_graphsave->signal_clicked().connect(sigc::mem_fun(*gda, &SkillGuiGraphDrawingArea::save));
  tb_graphopen->signal_clicked().connect(sigc::mem_fun(*gda, &SkillGuiGraphDrawingArea::open));
  tb_zoomin->signal_clicked().connect(sigc::mem_fun(*gda, &SkillGuiGraphDrawingArea::zoom_in));
  tb_zoomout->signal_clicked().connect(sigc::mem_fun(*gda, &SkillGuiGraphDrawingArea::zoom_out));
  tb_zoomfit->signal_clicked().connect(sigc::mem_fun(*gda, &SkillGuiGraphDrawingArea::zoom_fit));
  tb_zoomreset->signal_clicked().connect(sigc::mem_fun(*gda, &SkillGuiGraphDrawingArea::zoom_reset));
  tb_graphrecord->signal_clicked().connect(sigc::mem_fun(*this, &SkillGuiGtkWindow::on_recording_toggled));
  gda->signal_update_disabled().connect(sigc::mem_fun(*this, &SkillGuiGtkWindow::on_update_disabled));
#endif

  __gconf->signal_value_changed().connect(sigc::hide(sigc::hide(sigc::mem_fun(*this, &SkillGuiGtkWindow::on_config_changed))));
  on_config_changed();
}


/** Destructor. */
SkillGuiGtkWindow::~SkillGuiGtkWindow()
{
  __gconf->remove_dir(GCONF_PREFIX);
  __logview->set_client(NULL);
  __trv_plugins->set_network_client(NULL);
}


void
SkillGuiGtkWindow::on_config_changed()
{
  Gnome::Conf::SListHandle_ValueString l(__gconf->get_string_list(GCONF_PREFIX"/command_history"));

  __sks_list->clear();
  for (Gnome::Conf::SListHandle_ValueString::const_iterator i = l.begin(); i != l.end(); ++i) {
    Gtk::TreeModel::Row row  = *__sks_list->append();
    row[__sks_record.skillstring] = *i;    
  }

#ifdef GLIBMM_EXCEPTIONS_ENABLED
  bool continuous = __gconf->get_bool(GCONF_PREFIX"/continuous_exec");
#else
  std::auto_ptr<Glib::Error> error;
  bool continuous = __gconf->get_bool(GCONF_PREFIX"/continuous_exec", error);
#endif
  tb_continuous->set_active(continuous);
}


void
SkillGuiGtkWindow::on_skill_changed()
{
  Glib::ustring skill = cb_graphlist->get_active_text();
  if ( skill == ACTIVE_SKILL ) {
    skill = "ACTIVE";
  }
  SkillerDebugInterface::SetGraphMessage *sgm = new SkillerDebugInterface::SetGraphMessage(skill.c_str());
  __skdbg_if->msgq_enqueue(sgm);
}

void
SkillGuiGtkWindow::on_contexec_toggled()
{
  __gconf->set(GCONF_PREFIX"/continuous_exec", tb_continuous->get_active());
}

/** Event handler for connection button. */
void
SkillGuiGtkWindow::on_connection_clicked()
{
  if ( ! connection_dispatcher.get_client()->connected() ) {
    ServiceChooserDialog ssd(*this, connection_dispatcher.get_client());
    ssd.run_and_connect();
  } else {
    connection_dispatcher.get_client()->disconnect();
  } 
}


void
SkillGuiGtkWindow::on_exit_clicked()
{
  Gtk::Main::quit();
}


void
SkillGuiGtkWindow::on_controller_clicked()
{
  if (__skiller_if && __skiller_if->is_valid() && __skiller_if->has_writer() &&
      __skiller_if->exclusive_controller() == __skiller_if->serial()) {
    // we are exclusive controller, release control
    SkillerInterface::ReleaseControlMessage *rcm = new SkillerInterface::ReleaseControlMessage();
    __skiller_if->msgq_enqueue(rcm);
  } else if (__skiller_if && __skiller_if->is_valid() && __skiller_if->has_writer() &&
	     __skiller_if->exclusive_controller() == 0) {
    // there is no exclusive controller, try to acquire control
    SkillerInterface::AcquireControlMessage *acm = new SkillerInterface::AcquireControlMessage();
    __skiller_if->msgq_enqueue(acm);
  } else {
    Gtk::MessageDialog md(*this,
			  "Another component already acquired the exclusive "
			  "control for the Skiller; not acquiring exclusive control.",
			  /* markup */ false,
			  Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK,
			  /* modal */ true);
    md.set_title("Control Acquisition Failed");
    md.run();
  }
}

void
SkillGuiGtkWindow::on_stop_clicked()
{
  if ( bb && __skiller_if && __skiller_if->is_valid() && __skiller_if->has_writer() ) {
    SkillerInterface::StopExecMessage *sem = new SkillerInterface::StopExecMessage();
    __skiller_if->msgq_enqueue(sem);
  }
}

void
SkillGuiGtkWindow::close_bb()
{
  if ( bb ) {
    bb->unregister_listener(__skiller_ifd);
    bb->unregister_listener(__skdbg_ifd);
    bb->unregister_listener(__agdbg_ifd);
    delete __skiller_ifd;
    delete __skdbg_ifd;
    delete __agdbg_ifd;
    if ( __skiller_if && __skiller_if->is_valid() && __skiller_if->has_writer() &&
	 (__skiller_if->exclusive_controller() == __skiller_if->serial()) ) {
      SkillerInterface::ReleaseControlMessage *rcm = new SkillerInterface::ReleaseControlMessage();
      __skiller_if->msgq_enqueue(rcm);
    }
    bb->close(__skiller_if);
    bb->close(__skdbg_if);
    bb->close(__agdbg_if);
    delete bb;
    __skiller_if = NULL;
    __skdbg_if = NULL;
    __agdbg_if = NULL;
    bb = NULL;
  }
}

/** Event handler for connected event. */
void
SkillGuiGtkWindow::on_connect()
{
  try {
    if ( ! bb ) {
      bb           = new RemoteBlackBoard(connection_dispatcher.get_client());
      __skiller_if = bb->open_for_reading<SkillerInterface>("Skiller");
      __skdbg_if   = bb->open_for_reading<SkillerDebugInterface>("Skiller");
      __agdbg_if   = bb->open_for_reading<SkillerDebugInterface>("LuaAgent");
      on_skiller_data_changed();
      on_skdbg_data_changed();
      on_agdbg_data_changed();

      __skiller_ifd = new InterfaceDispatcher("Skiller IFD", __skiller_if);
      __skdbg_ifd   = new InterfaceDispatcher("SkillerDebug IFD", __skdbg_if);
      __agdbg_ifd   = new InterfaceDispatcher("LuaAgent SkillerDebug IFD", __agdbg_if);
      bb->register_listener(__skiller_ifd, BlackBoard::BBIL_FLAG_DATA);
      bb->register_listener(__skdbg_ifd, BlackBoard::BBIL_FLAG_DATA);
      bb->register_listener(__agdbg_ifd, BlackBoard::BBIL_FLAG_DATA);
      __skiller_ifd->signal_data_changed().connect(sigc::hide(sigc::mem_fun(*this, &SkillGuiGtkWindow::on_skiller_data_changed)));
      __skdbg_ifd->signal_data_changed().connect(sigc::hide(sigc::mem_fun(*this, &SkillGuiGtkWindow::on_skdbg_data_changed)));
      __agdbg_ifd->signal_data_changed().connect(sigc::hide(sigc::mem_fun(*this, &SkillGuiGtkWindow::on_agdbg_data_changed)));

      // always try to acquire control on connect, this may well fail, for
      // example if agent is running, but we don't care
      __skiller_if->read();
      if (__skiller_if->has_writer() && __skiller_if->exclusive_controller() == 0) {
	SkillerInterface::AcquireControlMessage *aqm = new SkillerInterface::AcquireControlMessage();
	__skiller_if->msgq_enqueue(aqm);
      }
      if (__skdbg_if->has_writer()) {
	SkillerDebugInterface::SetGraphMessage *sgm = new SkillerDebugInterface::SetGraphMessage("LIST");
	__skdbg_if->msgq_enqueue(sgm);
      }
    }
    tb_connection->set_stock_id(Gtk::Stock::DISCONNECT);
    __logview->set_client(connection_dispatcher.get_client());

    tb_continuous->set_sensitive(true);
    tb_controller->set_sensitive(true);
    cbe_skillstring->set_sensitive(true);

  } catch (Exception &e) {
    Glib::ustring message = *(e.begin());
    Gtk::MessageDialog md(*this, message, /* markup */ false,
			  Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK,
			  /* modal */ true);
    md.set_title("BlackBoard connection failed");
    md.run();

    close_bb();
    connection_dispatcher.get_client()->disconnect();
  }
}

/** Event handler for disconnected event. */
void
SkillGuiGtkWindow::on_disconnect()
{
  tb_continuous->set_sensitive(false);
  tb_controller->set_sensitive(false);
  cbe_skillstring->set_sensitive(false);
  but_exec->set_sensitive(false);
  but_stop->set_sensitive(false);

  close_bb();

  tb_connection->set_stock_id(Gtk::Stock::CONNECT);
#ifdef USE_PAPYRUS
  pvp_graph->queue_draw();
#endif
  __logview->set_client(NULL);
}


void
SkillGuiGtkWindow::on_exec_clicked()
{
  Glib::ustring sks = "";
  if ( cbe_skillstring->get_active_row_number() == -1 ) {
    Gtk::Entry *entry = cbe_skillstring->get_entry();
    sks = entry->get_text();
  } else {
    Gtk::TreeModel::Row row = *cbe_skillstring->get_active();
    row.get_value(cbe_skillstring->get_text_column(), sks);
  }

  if ( sks != "" ) {
    __throbber->set_timeout(80);

    if (__skiller_if && __skiller_if->is_valid() && __skiller_if->has_writer() &&
	__skiller_if->exclusive_controller() == __skiller_if->serial()) {

      if ( tb_continuous->get_active() ) {
	SkillerInterface::ExecSkillContinuousMessage *escm = new SkillerInterface::ExecSkillContinuousMessage(sks.c_str());
	__skiller_if->msgq_enqueue(escm);
      } else {
	SkillerInterface::ExecSkillMessage *esm = new SkillerInterface::ExecSkillMessage(sks.c_str());
	__skiller_if->msgq_enqueue(esm);
      }

      Gtk::TreeModel::Children children = __sks_list->children();
      bool ok = true;
      if ( ! children.empty() ) {
	size_t num = 0;
	Gtk::TreeIter i = children.begin();
	while (ok && (i != children.end())) {
	  if ( num >= 9 ) {
	    i = __sks_list->erase(i);
	} else {
	    Gtk::TreeModel::Row row = *i;
	    ok = (row[__sks_record.skillstring] != sks);
	    ++num;
	    ++i;
	  }
	}
      }
      if (ok) {
	Gtk::TreeModel::Row row  = *__sks_list->prepend();
	row[__sks_record.skillstring] = sks;
	
	std::list<Glib::ustring> l;
	for (Gtk::TreeIter i = children.begin(); i != children.end(); ++i) {
	  Gtk::TreeModel::Row row = *i;
	  l.push_back(row[__sks_record.skillstring]);
	}

	__gconf->set_string_list(GCONF_PREFIX"/command_history", l);
      }
    } else {
      Gtk::MessageDialog md(*this, "The exclusive control over the skiller has "
			    "not been acquired yet and skills cannot be executed",
			    /* markup */ false,
			    Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK,
			    /* modal */ true);
      md.set_title("Skill Execution Failure");
      md.run();
    }
  }
}


void
SkillGuiGtkWindow::on_skiller_data_changed()
{
  try {
    __skiller_if->read();

    switch (__skiller_if->status()) {
    case SkillerInterface::S_INACTIVE:
      __throbber->stop_anim();
      lab_status->set_text("S_INACTIVE");
      break;
    case SkillerInterface::S_FINAL:
      __throbber->stop_anim();
      __throbber->set_stock(Gtk::Stock::APPLY);
      lab_status->set_text("S_FINAL");
      break;
    case SkillerInterface::S_RUNNING:
      __throbber->start_anim();
      lab_status->set_text("S_RUNNING");
      break;
    case SkillerInterface::S_FAILED:
      __throbber->stop_anim();
      __throbber->set_stock(Gtk::Stock::DIALOG_WARNING);
      lab_status->set_text("S_FAILED");
      break;
    }

    lab_skillstring->set_text(__skiller_if->skill_string());
    lab_skillstring->set_tooltip_text(__skiller_if->skill_string());
    lab_error->set_text(__skiller_if->error());
    lab_error->set_tooltip_text(__skiller_if->error());
    lab_continuous->set_text(__skiller_if->is_continuous() ? "Yes" : "No");
    lab_alive->set_text(__skiller_if->has_writer() ? "Yes" : "No");

    if ( __skiller_if->exclusive_controller() == __skiller_if->serial() ) {
      if ( tb_controller->get_stock_id() == Gtk::Stock::NO.id ) {
	tb_controller->set_stock_id(Gtk::Stock::YES);
      }
      but_exec->set_sensitive(true);
      but_stop->set_sensitive(true);
    } else {
      if ( tb_controller->get_stock_id() == Gtk::Stock::YES.id ) {
	tb_controller->set_stock_id(Gtk::Stock::NO);
      }
      but_exec->set_sensitive(false);
      but_stop->set_sensitive(false);
    }


  } catch (Exception &e) {
    __throbber->stop_anim();
  }
}


void
SkillGuiGtkWindow::on_skdbg_data_changed()
{
  if (tb_skiller->get_active() && __skdbg_if) {
    try {
      __skdbg_if->read();

      if (strcmp(__skdbg_if->graph_fsm(), "LIST") == 0) {
	Glib::ustring list = __skdbg_if->graph();
	Glib::RefPtr<Glib::Regex> regex = Glib::Regex::create("\n");
	std::list<std::string> skills = regex->split(list);
	cb_graphlist->clear_items();
	cb_graphlist->append_text(ACTIVE_SKILL);
	cb_graphlist->set_active_text(ACTIVE_SKILL);
	for (std::list<std::string>::iterator i = skills.begin(); i != skills.end(); ++i) {
	  if (*i != "")  cb_graphlist->append_text(*i);
	}
	if (__skdbg_if->has_writer()) {
	  SkillerDebugInterface::SetGraphMessage *sgm = new SkillerDebugInterface::SetGraphMessage("ACTIVE");
	  __skdbg_if->msgq_enqueue(sgm);
	}
      } else {
#ifdef USE_PAPYRUS
	pvp_graph->set_graph_fsm(__skdbg_if->graph_fsm());
	pvp_graph->set_graph(__skdbg_if->graph());
	pvp_graph->render();
#else
	gda->set_graph_fsm(__skdbg_if->graph_fsm());
	gda->set_graph(__skdbg_if->graph());
#endif
      }
    } catch (Exception &e) {
      // ignored
    }
  }
}


void
SkillGuiGtkWindow::on_agdbg_data_changed()
{
  if (tb_agent->get_active() && __agdbg_if) {
    try {
      __agdbg_if->read();
#ifdef USE_PAPYRUS
      pvp_graph->set_graph_fsm(__agdbg_if->graph_fsm());
      pvp_graph->set_graph(__agdbg_if->graph());
      pvp_graph->render();
#else
      gda->set_graph_fsm(__agdbg_if->graph_fsm());
      gda->set_graph(__agdbg_if->graph());
#endif
    } catch (Exception &e) {
      // ignored
    }
  }
}


void
SkillGuiGtkWindow::on_graphupd_clicked()
{
#ifdef USE_PAPYRUS
  if ( pvp_graph->get_update_graph() ) {
    pvp_graph->set_update_graph(false);
    tb_graphupd->set_stock_id(Gtk::Stock::MEDIA_STOP);
  } else {
    pvp_graph->set_update_graph(true);
    tb_graphupd->set_stock_id(Gtk::Stock::MEDIA_PLAY);
    pvp_graph->render();
  }
#else
  if ( gda->get_update_graph() ) {
    gda->set_update_graph(false);
    tb_graphupd->set_stock_id(Gtk::Stock::MEDIA_STOP);
  } else {
    gda->set_update_graph(true);
    tb_graphupd->set_stock_id(Gtk::Stock::MEDIA_PLAY);
  }
#endif
}


SkillGuiGtkWindow::SkillStringRecord::SkillStringRecord()
{
  add(skillstring);
}


void
SkillGuiGtkWindow::on_update_disabled()
{
#ifdef USE_PAPYRUS
#else
  tb_graphupd->set_stock_id(Gtk::Stock::MEDIA_STOP);
#endif
}


void
SkillGuiGtkWindow::on_recording_toggled()
{
#ifdef USE_PAPYRUS
#else
  bool active = tb_graphrecord->get_active();
  if (gda->set_recording(active) != active) {
    tb_graphrecord->set_active(!active);
  }
#endif
}
