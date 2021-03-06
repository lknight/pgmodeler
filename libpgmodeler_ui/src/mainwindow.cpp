/*
# PostgreSQL Database Modeler (pgModeler)
#
# Copyright 2006-2014 - Raphael Araújo e Silva <raphael@pgmodeler.com.br>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation version 3.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# The complete text of GPLv3 is at LICENSE file on source code root directory.
# Also, you can get the complete GNU General Public License at <http://www.gnu.org/licenses/>
*/

#include "mainwindow.h"
#include "configurationform.h"

ConfigurationForm *configuration_form=nullptr;

MainWindow::MainWindow(QWidget *parent, Qt::WindowFlags flags) : QMainWindow(parent, flags)
{
	map<QString, attribs_map >confs;
	map<QString, attribs_map >::iterator itr, itr_end;
	attribs_map attribs;
	BaseConfigWidget *conf_wgt=nullptr;
	PluginsConfigWidget *plugins_conf_wgt=nullptr;

	central_wgt=nullptr;
	setupUi(this);
	models_tbw->tabBar()->setVisible(false);
	print_dlg=new QPrintDialog(this);

	central_wgt=new CentralWidget(models_tbw_parent);
	general_tb->layout()->setContentsMargins(0,0,0,0);

	try
	{
		configuration_form=new ConfigurationForm(nullptr, Qt::WindowTitleHint | Qt::WindowSystemMenuHint);
		configuration_form->loadConfiguration();

    plugins_conf_wgt=dynamic_cast<PluginsConfigWidget *>(configuration_form->getConfigurationWidget(ConfigurationForm::PLUGINS_CONF_WGT));
		plugins_conf_wgt->installPluginsActions(nullptr, plugins_menu, this, SLOT(executePlugin(void)));
		plugins_menu->setEnabled(!plugins_menu->isEmpty());
		action_plugins->setEnabled(!plugins_menu->isEmpty());
		action_plugins->setMenu(plugins_menu);
    dynamic_cast<QToolButton *>(general_tb->widgetForAction(action_plugins))->setPopupMode(QToolButton::InstantPopup);

		conf_wgt=configuration_form->getConfigurationWidget(ConfigurationForm::GENERAL_CONF_WGT);
		confs=conf_wgt->getConfigurationParams();

		itr=confs.begin();
		itr_end=confs.end();

		//Configuring the widget visibility according to the configurations
		while(itr!=itr_end)
		{
			attribs=itr->second;
			if(attribs.count(ParsersAttributes::PATH)!=0)
			{
				try
				{
					//Storing the file of a previous session
					if(itr->first.contains(ParsersAttributes::_FILE_) &&
						 !attribs[ParsersAttributes::PATH].isEmpty())
						prev_session_files.push_back(attribs[ParsersAttributes::PATH]);

					//Creating the recent models menu
					else if(itr->first.contains(ParsersAttributes::RECENT) &&
									!attribs[ParsersAttributes::PATH].isEmpty())
						recent_models.push_back(attribs[ParsersAttributes::PATH]);
				}
				catch(Exception &e)
				{
					Messagebox msg_box;
					msg_box.show(e);
				}
			}

			itr++;
		}

		//Enables the action to restore session when there are registered session files
		action_restore_session->setEnabled(!prev_session_files.isEmpty());
		central_wgt->last_session_tb->setEnabled(action_restore_session->isEnabled());
	}
	catch(Exception &e)
	{
		Messagebox msg_box;
		msg_box.show(e);
	}

	try
	{
		QDir dir;

		this->setFocusPolicy(Qt::WheelFocus);

		//Check if the temporary dir exists, if not, creates it.
		if(!dir.exists(GlobalAttributes::TEMPORARY_DIR))
			dir.mkdir(GlobalAttributes::TEMPORARY_DIR);

		model_nav_wgt=new ModelNavigationWidget(this);

    control_tb->addWidget(model_nav_wgt);
    control_tb->addSeparator();
    control_tb->addAction(action_about);
    control_tb->addAction(action_update_found);

		about_wgt=new AboutWidget(this);
		update_notifier_wgt=new UpdateNotifierWidget(this);
		restoration_form=new ModelRestorationForm(nullptr, Qt::Dialog | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);

		oper_list_wgt=new OperationListWidget;
		model_objs_wgt=new ModelObjectsWidget;
		overview_wgt=new ModelOverviewWidget;
		model_valid_wgt=new ModelValidationWidget;
		sql_tool_wgt=new SQLToolWidget;
		obj_finder_wgt=new ObjectFinderWidget;
	}
	catch(Exception &e)
	{
		throw Exception(e.getErrorMessage(), e.getErrorType(),__PRETTY_FUNCTION__,__FILE__,__LINE__, &e);
	}

	connect(central_wgt->new_tb, SIGNAL(clicked()), this, SLOT(addModel()));
	connect(central_wgt->open_tb, SIGNAL(clicked()), this, SLOT(loadModel()));
	connect(central_wgt->last_session_tb, SIGNAL(clicked()), this, SLOT(restoreLastSession()));

	connect(action_show_main_menu, SIGNAL(triggered()), this, SLOT(showMainMenu()));
	connect(action_hide_main_menu, SIGNAL(triggered()), this, SLOT(showMainMenu()));

	connect(update_notifier_wgt, SIGNAL(s_updateAvailable(bool)), action_update_found, SLOT(setVisible(bool)));
	connect(update_notifier_wgt, SIGNAL(s_updateAvailable(bool)), action_update_found, SLOT(setChecked(bool)));
	connect(update_notifier_wgt, SIGNAL(s_visibilityChanged(bool)), action_update_found, SLOT(setChecked(bool)));
	connect(action_update_found,SIGNAL(toggled(bool)),this,SLOT(toggleUpdateNotifier(bool)));
	connect(action_check_update,SIGNAL(triggered()), update_notifier_wgt, SLOT(checkForUpdate()));

	connect(action_about,SIGNAL(toggled(bool)),this,SLOT(toggleAboutWidget(bool)));
	connect(about_wgt, SIGNAL(s_visibilityChanged(bool)), action_about, SLOT(setChecked(bool)));

	connect(action_restore_session,SIGNAL(triggered(bool)),this,SLOT(restoreLastSession()));
	connect(action_exit,SIGNAL(triggered(bool)),this,SLOT(close()));
	connect(action_new_model,SIGNAL(triggered(bool)),this,SLOT(addModel()));
	connect(action_close_model,SIGNAL(triggered(bool)),this,SLOT(closeModel()));
	connect(action_fix_model, SIGNAL(triggered(bool)), this, SLOT(fixModel()));

	connect(action_next,SIGNAL(triggered(bool)),this,SLOT(setCurrentModel()));
	connect(action_previous,SIGNAL(triggered(bool)),this,SLOT(setCurrentModel()));
	connect(action_wiki,SIGNAL(triggered(bool)),this,SLOT(openWiki()));

	connect(action_inc_zoom,SIGNAL(triggered(bool)),this,SLOT(applyZoom()));
	connect(action_dec_zoom,SIGNAL(triggered(bool)),this,SLOT(applyZoom()));
	connect(action_normal_zoom,SIGNAL(triggered(bool)),this,SLOT(applyZoom()));

	connect(action_load_model,SIGNAL(triggered(bool)),this,SLOT(loadModel()));
	connect(action_save_model,SIGNAL(triggered(bool)),this,SLOT(saveModel()));
	connect(action_save_as,SIGNAL(triggered(bool)),this,SLOT(saveModel()));
	connect(action_save_all,SIGNAL(triggered(bool)),this,SLOT(saveAllModels()));

	connect(oper_list_wgt, SIGNAL(s_operationExecuted(void)), this, SLOT(updateDockWidgets(void)));
	connect(oper_list_wgt, SIGNAL(s_operationListUpdated(void)), this, SLOT(__updateToolsState(void)));
	connect(action_undo,SIGNAL(triggered(bool)),oper_list_wgt,SLOT(undoOperation(void)));
	connect(action_redo,SIGNAL(triggered(bool)),oper_list_wgt,SLOT(redoOperation(void)));

	connect(model_nav_wgt, SIGNAL(s_modelCloseRequested(int)), this, SLOT(closeModel(int)));
	connect(model_nav_wgt, SIGNAL(s_currentModelChanged(int)), this, SLOT(setCurrentModel()));

	connect(action_print, SIGNAL(triggered(bool)), this, SLOT(printModel(void)));
	connect(action_configuration, SIGNAL(triggered(bool)), configuration_form, SLOT(show(void)));

	connect(oper_list_wgt, SIGNAL(s_operationExecuted(void)), overview_wgt, SLOT(updateOverview(void)));
	connect(configuration_form, SIGNAL(finished(int)), this, SLOT(applyConfigurations(void)));
	connect(configuration_form, SIGNAL(rejected()), this, SLOT(updateConnections(void)));
	connect(&model_save_timer, SIGNAL(timeout(void)), this, SLOT(saveAllModels(void)));

	connect(action_export, SIGNAL(triggered(bool)), this, SLOT(exportModel(void)));
	connect(action_import, SIGNAL(triggered(bool)), this, SLOT(importDatabase(void)));

	window_title=this->windowTitle() + " " + GlobalAttributes::PGMODELER_VERSION;
	this->setWindowTitle(window_title);

	current_model=nullptr;
	models_tbw->setVisible(false);
	models_tbw->installEventFilter(this);

	model_objs_parent->setVisible(false);
	oper_list_parent->setVisible(false);
	obj_finder_parent->setVisible(false);
	model_valid_parent->setVisible(false);
	sql_tool_parent->setVisible(false);
	bg_saving_wgt->setVisible(false);
	update_notifier_wgt->setVisible(false);
	about_wgt->setVisible(false);

	models_tbw_parent->lower();
	central_wgt->lower();
	v_splitter1->lower();

	QVBoxLayout *vlayout=new QVBoxLayout;
	vlayout->setContentsMargins(0,0,0,0);
	vlayout->addWidget(model_objs_wgt);
	model_objs_parent->setLayout(vlayout);

	vlayout=new QVBoxLayout;
	vlayout->setContentsMargins(0,0,0,0);
	vlayout->addWidget(oper_list_wgt);
	oper_list_parent->setLayout(vlayout);

	QHBoxLayout * hlayout=new QHBoxLayout;
	hlayout->setContentsMargins(0,0,0,0);
	hlayout->addWidget(model_valid_wgt);
	model_valid_parent->setLayout(hlayout);
	model_valid_parent->installEventFilter(this);

	hlayout=new QHBoxLayout;
	hlayout->setContentsMargins(0,0,0,0);
	hlayout->addWidget(obj_finder_wgt);
	obj_finder_parent->setLayout(hlayout);
	obj_finder_parent->installEventFilter(this);

	vlayout=new QVBoxLayout;
	vlayout->setContentsMargins(0,0,0,0);
	vlayout->addWidget(sql_tool_wgt);
	sql_tool_parent->setLayout(vlayout);
	sql_tool_parent->installEventFilter(this);

	connect(objects_btn, SIGNAL(toggled(bool)), model_objs_parent, SLOT(setVisible(bool)));
	connect(objects_btn, SIGNAL(toggled(bool)), model_objs_wgt, SLOT(setVisible(bool)));
	connect(objects_btn, SIGNAL(toggled(bool)), this, SLOT(showRightWidgetsBar(void)));
	connect(model_objs_wgt, SIGNAL(s_visibilityChanged(bool)), objects_btn, SLOT(setChecked(bool)));
	connect(model_objs_wgt, SIGNAL(s_visibilityChanged(bool)), this, SLOT(showRightWidgetsBar()));

	connect(operations_btn, SIGNAL(toggled(bool)), oper_list_parent, SLOT(setVisible(bool)));
	connect(operations_btn, SIGNAL(toggled(bool)), oper_list_wgt, SLOT(setVisible(bool)));
	connect(operations_btn, SIGNAL(toggled(bool)), this, SLOT(showRightWidgetsBar(void)));
	connect(oper_list_wgt, SIGNAL(s_visibilityChanged(bool)), operations_btn, SLOT(setChecked(bool)));
	connect(oper_list_wgt, SIGNAL(s_visibilityChanged(bool)), this, SLOT(showRightWidgetsBar()));

	connect(validation_btn, SIGNAL(toggled(bool)), model_valid_parent, SLOT(setVisible(bool)));
	connect(validation_btn, SIGNAL(toggled(bool)), model_valid_wgt, SLOT(setVisible(bool)));
	connect(validation_btn, SIGNAL(toggled(bool)), this, SLOT(showBottomWidgetsBar(void)));
	connect(model_valid_wgt, SIGNAL(s_visibilityChanged(bool)), validation_btn, SLOT(setChecked(bool)));
	connect(model_valid_wgt, SIGNAL(s_visibilityChanged(bool)), this, SLOT(showBottomWidgetsBar()));

	connect(find_obj_btn, SIGNAL(toggled(bool)), obj_finder_parent, SLOT(setVisible(bool)));
	connect(find_obj_btn, SIGNAL(toggled(bool)), obj_finder_wgt, SLOT(setVisible(bool)));
	connect(find_obj_btn, SIGNAL(toggled(bool)), this, SLOT(showBottomWidgetsBar(void)));
	connect(obj_finder_wgt, SIGNAL(s_visibilityChanged(bool)), find_obj_btn, SLOT(setChecked(bool)));
	connect(obj_finder_wgt, SIGNAL(s_visibilityChanged(bool)), this, SLOT(showBottomWidgetsBar()));

	connect(sql_tool_btn, SIGNAL(toggled(bool)), sql_tool_parent, SLOT(setVisible(bool)));
	connect(sql_tool_btn, SIGNAL(toggled(bool)), sql_tool_wgt, SLOT(setVisible(bool)));
	connect(sql_tool_wgt, SIGNAL(s_visibilityChanged(bool)), sql_tool_btn, SLOT(setChecked(bool)));

	connect(model_valid_wgt, SIGNAL(s_validationInProgress(bool)), this->main_menu_mb, SLOT(setDisabled(bool)));
	connect(model_valid_wgt, SIGNAL(s_validationInProgress(bool)), control_tb, SLOT(setDisabled(bool)));
	connect(model_valid_wgt, SIGNAL(s_validationInProgress(bool)), general_tb, SLOT(setDisabled(bool)));
	connect(model_valid_wgt, SIGNAL(s_validationInProgress(bool)), models_tbw, SLOT(setDisabled(bool)));
	connect(model_valid_wgt, SIGNAL(s_validationInProgress(bool)), oper_list_wgt, SLOT(setDisabled(bool)));
	connect(model_valid_wgt, SIGNAL(s_validationInProgress(bool)), model_objs_wgt, SLOT(setDisabled(bool)));
	connect(model_valid_wgt, SIGNAL(s_validationInProgress(bool)), obj_finder_wgt, SLOT(setDisabled(bool)));
	connect(model_valid_wgt, SIGNAL(s_validationInProgress(bool)), models_tbw, SLOT(setDisabled(bool)));
	connect(model_valid_wgt, SIGNAL(s_validationInProgress(bool)), this, SLOT(stopTimers(bool)));

	connect(&tmpmodel_save_timer, SIGNAL(timeout()), &tmpmodel_thread, SLOT(start()));
	connect(&tmpmodel_thread, SIGNAL(started()), this, SLOT(saveTemporaryModels()));
	connect(&tmpmodel_thread, &QThread::started, [=](){ tmpmodel_thread.setPriority(QThread::HighPriority); });

	models_tbw_parent->resize(QSize(models_tbw_parent->maximumWidth(), models_tbw_parent->height()));

	//Forcing the splitter that handles the bottom widgets to resize its children to their minimum size
	v_splitter1->setSizes({500, 250, 500});

	showRightWidgetsBar();
	showBottomWidgetsBar();

	//Restore temporary models (if exists)
	if(restoration_form->hasTemporaryModels())
	{
		restoration_form->exec();

		if(restoration_form->result()==QDialog::Accepted)
		{
			ModelWidget *model=nullptr;
			QString model_file;
			QStringList tmp_models=restoration_form->getSelectedModels();

			while(!tmp_models.isEmpty())
			{
				try
				{
					model_file=tmp_models.front();
					tmp_models.pop_front();
					this->addModel(model_file);

					//Get the model widget generated from file
					model=dynamic_cast<ModelWidget *>(models_tbw->widget(models_tbw->count()-1));

					//Set the model as modified forcing the user to save when the autosave timer ends
					model->modified=true;
					model->filename.clear();
					restoration_form->removeTemporaryModel(model_file);
				}
				catch(Exception &e)
				{
					//Destroy the temp file if the "keep  models" isn't checked
					if(!restoration_form->keep_models_chk->isChecked())
						restoration_form->removeTemporaryModel(model_file);

					Messagebox msg_box;
					msg_box.show(e);
				}
			}
		}
	}

	//If a previous session was restored save the temp models
	saveTemporaryModels(true);
	updateConnections();
	updateRecentModelsMenu();
	configureSamplesMenu();
	applyConfigurations();

	//Temporary models are saved every two minutes
	tmpmodel_save_timer.setInterval(120000);

  QList<QAction *> actions=general_tb->actions();
  QToolButton *btn=nullptr;

  for(auto act : actions)
  {
    btn=qobject_cast<QToolButton *>(general_tb->widgetForAction(act));
		btn->setGraphicsEffect(createDropShadow(btn));
  }

  #ifdef Q_OS_MAC
    control_tb->removeAction(action_main_menu);
    action_main_menu->setEnabled(false);
  #else
		plugins_menu->menuAction()->setIconVisibleInMenu(false);
    main_menu.addMenu(file_menu);
    main_menu.addMenu(edit_menu);
    main_menu.addMenu(show_menu);
    main_menu.addMenu(plugins_menu);
    main_menu.addMenu(about_menu);
    main_menu.addSeparator();
    main_menu.addAction(action_show_main_menu);
    action_main_menu->setMenu(&main_menu);
    dynamic_cast<QToolButton *>(control_tb->widgetForAction(action_main_menu))->setPopupMode(QToolButton::InstantPopup);
  #endif
}

MainWindow::~MainWindow(void)
{
	delete(restoration_form);
	delete(overview_wgt);
	delete(configuration_form);
}

void MainWindow::showRightWidgetsBar(void)
{
	right_wgt_bar->setVisible(objects_btn->isChecked() || operations_btn->isChecked());
}

void MainWindow::showBottomWidgetsBar(void)
{
	bottom_wgt_bar->setVisible(validation_btn->isChecked() || find_obj_btn->isChecked());
}

void MainWindow::restoreLastSession(void)
{
	/* Loading the files from the previous session. The session will be restored only
	if pgModeler is not on model restore mode or pgModeler is not opening a model clicked by user
	o the file manager */
	if(QApplication::arguments().size() <= 1 &&
		 !prev_session_files.isEmpty() && restoration_form->result()==QDialog::Rejected)
	{
		try
		{
			while(!prev_session_files.isEmpty())
			{
				this->addModel(prev_session_files.front());
				prev_session_files.pop_front();
			}

			saveTemporaryModels(true);
			action_restore_session->setEnabled(false);
			central_wgt->last_session_tb->setEnabled(false);
		}
		catch(Exception &e)
		{
			Messagebox msg_box;
			msg_box.show(e);
		}
	}
}

void MainWindow::stopTimers(bool value)
{
	if(value)
	{
		tmpmodel_save_timer.stop();
		model_save_timer.stop();
		tmpmodel_thread.quit();
	}
	else
	{
		tmpmodel_save_timer.start();
		model_save_timer.start();
	}
}

void MainWindow::fixModel(const QString &filename)
{
	ModelFixForm model_fix_form(nullptr, Qt::Dialog | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);

	connect(&model_fix_form, SIGNAL(s_modelLoadRequested(QString)), this, SLOT(loadModel(QString)));

	if(!filename.isEmpty())
	{
		QFileInfo fi(filename);
		model_fix_form.input_file_edt->setText(fi.absoluteFilePath());
		model_fix_form.output_file_edt->setText(fi.absolutePath() + GlobalAttributes::DIR_SEPARATOR + fi.baseName() + "_fixed." + fi.suffix());
	}

	model_fix_form.exec();
	disconnect(&model_fix_form, nullptr, this, nullptr);
}

void MainWindow::showEvent(QShowEvent *)
{
	GeneralConfigWidget *conf_wgt=dynamic_cast<GeneralConfigWidget *>(configuration_form->getConfigurationWidget(ConfigurationForm::GENERAL_CONF_WGT));
	map<QString, attribs_map> confs=conf_wgt->getConfigurationParams();

  #ifndef Q_OS_MAC
    QTimer::singleShot(1000, conf_wgt, SLOT(updateFileAssociation()));

    //Hiding/showing the main menu bar depending on the retrieved conf
    main_menu_mb->setVisible(confs[ParsersAttributes::CONFIGURATION][ParsersAttributes::SHOW_MAIN_MENU]==ParsersAttributes::_TRUE_);

    if(main_menu_mb->isVisible())
      file_menu->addAction(action_hide_main_menu);

    action_main_menu->setVisible(!main_menu_mb->isVisible());
  #endif

	//Positioning the update notifier widget before showing it (if there is an update)
	setFloatingWidgetPos(update_notifier_wgt, action_update_found, control_tb, false);
	action_update_found->setVisible(false);

	//Enabling update check at startup
	if(confs[ParsersAttributes::CONFIGURATION][ParsersAttributes::CHECK_UPDATE]==ParsersAttributes::_TRUE_)
		QTimer::singleShot(2000, update_notifier_wgt, SLOT(checkForUpdate()));
}

void MainWindow::resizeEvent(QResizeEvent *)
{
	if(central_wgt)
	{
		central_wgt->move(bg_widget->width()/2 - central_wgt->width()/2 ,
											bg_widget->height()/2 - central_wgt->height()/2);
	}
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	//pgModeler will not close when the validation thread is still running
	if(model_valid_wgt->validation_thread->isRunning())
		event->ignore();
	else
	{
		GeneralConfigWidget *conf_wgt=nullptr;
		map<QString, attribs_map > confs;
		bool modified=false;
		int i=0;

		//Stops the saving timers as well the temp. model saving thread before close pgmodeler
		model_save_timer.stop();
		tmpmodel_save_timer.stop();
		tmpmodel_thread.quit();
		plugins_menu->clear();

		//Checking if there is modified models and ask the user to save them before close the application
		if(models_tbw->count() > 0)
		{
			i=0;
			while(i < models_tbw->count() && !modified)
				modified=dynamic_cast<ModelWidget *>(models_tbw->widget(i++))->isModified();

			if(modified)
			{
				Messagebox msg_box;

				msg_box.show(trUtf8("Save all models"),
										 trUtf8("Some models were modified! Do you really want to quit pgModeler without save them?"),
										 Messagebox::CONFIRM_ICON,Messagebox::YES_NO_BUTTONS);

				/* If the user rejects the message box the close event will be aborted
			causing pgModeler not to be finished */
				if(msg_box.result()==QDialog::Rejected)
					event->ignore();
			}
		}

		if(event->isAccepted())
		{
			int i, count;
			ModelWidget *model=nullptr;
			QString param_id;
			attribs_map attribs;

			this->overview_wgt->close();
			conf_wgt=dynamic_cast<GeneralConfigWidget *>(configuration_form->getConfigurationWidget(ConfigurationForm::GENERAL_CONF_WGT));
			confs=conf_wgt->getConfigurationParams();
			conf_wgt->removeConfigurationParams();

			attribs[ParsersAttributes::SHOW_MAIN_MENU]=main_menu_mb->isVisible() ? "1" : "";
			conf_wgt->addConfigurationParam(ParsersAttributes::CONFIGURATION, attribs);
			attribs.clear();

			//Saving the session
			count=models_tbw->count();
			for(i=0; i < count; i++)
			{
				model=dynamic_cast<ModelWidget *>(models_tbw->widget(i));

				if(!model->getFilename().isEmpty())
				{
					param_id=QString("%1%2").arg(ParsersAttributes::_FILE_).arg(i);
					attribs[ParsersAttributes::ID]=param_id;
					attribs[ParsersAttributes::PATH]=model->getFilename();
					conf_wgt->addConfigurationParam(param_id, attribs);
					attribs.clear();
				}
			}

			//Saving recent models list
			if(!recent_models.isEmpty())
			{
				int i=0;
				QString param_id;
				attribs_map attribs;

				while(!recent_models.isEmpty())
				{
					param_id=QString("%1%2").arg(ParsersAttributes::RECENT).arg(i++);
					attribs[ParsersAttributes::ID]=param_id;
					attribs[ParsersAttributes::PATH]=recent_models.front();
					conf_wgt->addConfigurationParam(param_id, attribs);
					attribs.clear();
					recent_models.pop_front();
				}

				recent_mdls_menu.clear();
			}

			conf_wgt->saveConfiguration();
			restoration_form->removeTemporaryModels();

			//Remove import log files
			QDir dir(GlobalAttributes::TEMPORARY_DIR);
			QStringList log_files;

			dir.setNameFilters({"*.log"});
			log_files=dir.entryList(QDir::Files);

			while(!log_files.isEmpty())
			{
				dir.remove(log_files.front());
				log_files.pop_front();
			}
		}

		/* Close all top level windows to avoid close the main window and left sub windows behind
			 preventing the application to be finished */
		qApp->closeAllWindows();
	}
}

void MainWindow::updateConnections(void)
{
	ConnectionsConfigWidget *conn_cfg_wgt=nullptr;
	map<QString, Connection *> connections;

	conn_cfg_wgt=dynamic_cast<ConnectionsConfigWidget *>(configuration_form->getConfigurationWidget(ConfigurationForm::CONNECTIONS_CONF_WGT));
	conn_cfg_wgt->getConnections(connections);
	model_valid_wgt->updateConnections(connections);
	sql_tool_wgt->updateConnections(connections);
}

void MainWindow::saveTemporaryModels(bool force)
{
	try
	{
		ModelWidget *model=nullptr;
		int count=models_tbw->count();

		if(count > 0 && (force || this->isActiveWindow()))
		{
			bg_saving_wgt->setVisible(true);
			bg_saving_pb->setValue(0);
			bg_saving_lbl->setText(trUtf8("Saving temp. models"));
			bg_saving_wgt->repaint();

			for(int i=0; i < count; i++)
			{
				model=dynamic_cast<ModelWidget *>(models_tbw->widget(i));
				bg_saving_pb->setValue(((i+1)/static_cast<float>(count)) * 100);

				if(model->isModified())
					model->getDatabaseModel()->saveModel(model->getTempFilename(), SchemaParser::XML_DEFINITION);

				QThread::msleep(200);
			}

			bg_saving_pb->setValue(100);
			bg_saving_wgt->setVisible(false);
		}

		tmpmodel_thread.quit();
	}
	catch(Exception &e)
	{
		Messagebox msg_box;

		tmpmodel_thread.quit();
		msg_box.show(e);
	}
}

void MainWindow::updateRecentModelsMenu(void)
{
	QAction *act=nullptr;
	recent_mdls_menu.clear();
	recent_models.removeDuplicates();

	for(int i=0; i < recent_models.size() && i < MAX_RECENT_MODELS; i++)
	{
		act=recent_mdls_menu.addAction(QFileInfo(recent_models[i]).fileName(),this,SLOT(loadModelFromAction(void)));
		act->setToolTip(recent_models[i]);
		act->setData(recent_models[i]);
	}

	if(!recent_mdls_menu.isEmpty())
	{
		recent_mdls_menu.addSeparator();
		recent_mdls_menu.addAction(trUtf8("Clear Menu"), this, SLOT(clearRecentModelsMenu(void)));
		action_recent_models->setMenu(&recent_mdls_menu);
		dynamic_cast<QToolButton *>(control_tb->widgetForAction(action_recent_models))->setPopupMode(QToolButton::InstantPopup);
	}

	action_recent_models->setEnabled(!recent_mdls_menu.isEmpty());
	central_wgt->recent_tb->setEnabled(action_recent_models->isEnabled());
	central_wgt->recent_tb->setMenu(recent_mdls_menu.isEmpty() ? nullptr : &recent_mdls_menu);
}

void MainWindow::loadModelFromAction(void)
{
	QAction *act=dynamic_cast<QAction *>(sender());

	if(act)
	{
		addModel(act->data().toString());
		recent_models.push_back(act->data().toString());
		updateRecentModelsMenu();
		saveTemporaryModels(true);
	}
}

void MainWindow::clearRecentModelsMenu(void)
{
	recent_models.clear();
	updateRecentModelsMenu();
}

void MainWindow::addModel(const QString &filename)
{
	ModelWidget *model_tab=nullptr;
	QString obj_name, tab_name, str_aux;
	Schema *public_sch=nullptr;
	bool start_timers=(models_tbw->count() == 0);

	//Set a name for the tab widget
	str_aux=QString("%1").arg(models_tbw->count());
	obj_name="model_";
	obj_name+=str_aux;
	tab_name=obj_name;

	model_tab=new ModelWidget;
	model_tab->setObjectName(Utf8String::create(obj_name));

	//Add the tab to the tab widget
	obj_name=model_tab->db_model->getName();

	models_tbw->blockSignals(true);
	models_tbw->addTab(model_tab, Utf8String::create(obj_name));
	models_tbw->setCurrentIndex(models_tbw->count()-1);
	models_tbw->blockSignals(false);
	models_tbw->currentWidget()->layout()->setContentsMargins(3,3,0,3);

	//Creating the system objects (public schema and languages C, SQL and pgpgsql)
	model_tab->db_model->createSystemObjects(filename.isEmpty());
	model_tab->db_model->setInvalidated(false);

	if(!filename.isEmpty())
	{
		try
		{
			model_tab->loadModel(filename);
			models_tbw->setTabToolTip(models_tbw->currentIndex(), filename);
			//Get the "public" schema and set as system object
			public_sch=dynamic_cast<Schema *>(model_tab->db_model->getObject("public", OBJ_SCHEMA));
			if(public_sch)	public_sch->setSystemObject(true);

			models_tbw->setVisible(true);
			model_tab->restoreLastCanvasPosition();
		}
		catch(Exception &e)
		{
			central_wgt->update();
			model_tab->setParent(nullptr);

			//Destroy the temp file generated by allocating a new model widget
			restoration_form->removeTemporaryModel(model_tab->getTempFilename());

			delete(model_tab);
			updateToolsState(true);

			throw Exception(e.getErrorMessage(),e.getErrorType(),__PRETTY_FUNCTION__,__FILE__,__LINE__, &e);
		}
	}

	model_nav_wgt->addModel(model_tab);
	setCurrentModel();

	if(start_timers)
	{
		if(model_save_timer.interval() > 0)
			model_save_timer.start();

		tmpmodel_save_timer.start();
	}

	model_tab->setModified(false);
}

void MainWindow::addModel(ModelWidget *model_wgt)
{
	if(!model_wgt)
		throw Exception(ERR_ASG_NOT_ALOC_OBJECT,__PRETTY_FUNCTION__,__FILE__,__LINE__);
	else if(model_wgt->parent())
		throw Exception(ERR_ASG_WGT_ALREADY_HAS_PARENT,__PRETTY_FUNCTION__,__FILE__,__LINE__);

	model_nav_wgt->addModel(model_wgt);

	models_tbw->blockSignals(true);
	models_tbw->addTab(model_wgt, Utf8String::create(model_wgt->getDatabaseModel()->getName()));
	models_tbw->setCurrentIndex(models_tbw->count()-1);
	models_tbw->blockSignals(false);
	setCurrentModel();
}

int MainWindow::getModelCount(void)
{
	return(models_tbw->count());
}

ModelWidget *MainWindow::getModel(int idx)
{
	if(idx < 0 || idx > models_tbw->count())
		throw Exception(ERR_REF_OBJ_INV_INDEX,__PRETTY_FUNCTION__,__FILE__,__LINE__);

	return(dynamic_cast<ModelWidget *>(models_tbw->widget(idx)));
}

void MainWindow::showMainMenu(void)
{
	action_main_menu->setVisible(sender()!=action_show_main_menu);
	main_menu_mb->setVisible(sender()==action_show_main_menu);

	if(sender()!=action_show_main_menu)
		file_menu->removeAction(action_hide_main_menu);
	else
		file_menu->addAction(action_hide_main_menu);
}

void MainWindow::setCurrentModel(void)
{
	QObject *object=nullptr;
	QToolButton *tool_btn=nullptr;

	object=sender();
	models_tbw->setVisible(models_tbw->count() > 0);
	removeModelActions();

	edit_menu->clear();
	edit_menu->addAction(action_undo);
	edit_menu->addAction(action_redo);
	edit_menu->addSeparator();

	if(object==action_next || object==action_previous)
	{
		models_tbw->blockSignals(true);

		if(object==action_next)
			models_tbw->setCurrentIndex(models_tbw->currentIndex()+1);
		else if(object==action_previous)
			models_tbw->setCurrentIndex(models_tbw->currentIndex()-1);

		models_tbw->blockSignals(false);
	}

	//Avoids the tree state saving in order to restore the current model tree state
	model_objs_wgt->saveTreeState(false);

	//Restore the tree state
	if(current_model)
		model_objs_wgt->saveTreeState(model_tree_states[current_model]);

	models_tbw->setCurrentIndex(model_nav_wgt->getCurrentIndex());
	current_model=dynamic_cast<ModelWidget *>(models_tbw->currentWidget());

	if(current_model)
	{
		current_model->setFocus(Qt::OtherFocusReason);
		current_model->cancelObjectAddition();

		general_tb->addAction(current_model->action_new_object);
    tool_btn=qobject_cast<QToolButton *>(general_tb->widgetForAction(current_model->action_new_object));
		tool_btn->setPopupMode(QToolButton::InstantPopup);
		tool_btn->setGraphicsEffect(createDropShadow(tool_btn));

		general_tb->addAction(current_model->action_quick_actions);
    tool_btn=qobject_cast<QToolButton *>(general_tb->widgetForAction(current_model->action_quick_actions));
		tool_btn->setPopupMode(QToolButton::InstantPopup);
		tool_btn->setGraphicsEffect(createDropShadow(tool_btn));

		general_tb->addAction(current_model->action_edit);
		tool_btn=qobject_cast<QToolButton *>(general_tb->widgetForAction(current_model->action_edit));
		tool_btn->setGraphicsEffect(createDropShadow(tool_btn));

		general_tb->addAction(current_model->action_source_code);
		tool_btn=qobject_cast<QToolButton *>(general_tb->widgetForAction(current_model->action_source_code));//->installEventFilter(this);
		tool_btn->setGraphicsEffect(createDropShadow(tool_btn));

		general_tb->addAction(current_model->action_select_all);
		tool_btn=qobject_cast<QToolButton *>(general_tb->widgetForAction(current_model->action_select_all));//->installEventFilter(this);
		tool_btn->setGraphicsEffect(createDropShadow(tool_btn));

		edit_menu->addAction(current_model->action_copy);
		edit_menu->addAction(current_model->action_cut);
		edit_menu->addAction(current_model->action_paste);
		edit_menu->addAction(current_model->action_remove);

		if(current_model->getFilename().isEmpty())
			this->setWindowTitle(window_title);
		else
			this->setWindowTitle(window_title + " - " + QDir::toNativeSeparators(current_model->getFilename()));

		connect(current_model, SIGNAL(s_objectsMoved(void)),oper_list_wgt, SLOT(updateOperationList(void)));
		connect(current_model, SIGNAL(s_objectModified(void)),this, SLOT(updateDockWidgets(void)));
		connect(current_model, SIGNAL(s_objectCreated(void)),this, SLOT(updateDockWidgets(void)));
		connect(current_model, SIGNAL(s_objectRemoved(void)),this, SLOT(updateDockWidgets(void)));
		connect(current_model, SIGNAL(s_objectManipulated(void)),this, SLOT(updateDockWidgets(void)));
		connect(current_model, SIGNAL(s_objectManipulated(void)), this, SLOT(updateModelTabName(void)));

		connect(current_model, SIGNAL(s_zoomModified(float)), this, SLOT(updateToolsState(void)));
		connect(current_model, SIGNAL(s_objectModified(void)), this, SLOT(updateModelTabName(void)));


		connect(action_alin_objs_grade, SIGNAL(triggered(bool)), this, SLOT(setGridOptions(void)));
		connect(action_show_grid, SIGNAL(triggered(bool)), this, SLOT(setGridOptions(void)));
		connect(action_show_delimiters, SIGNAL(triggered(bool)), this, SLOT(setGridOptions(void)));

		connect(action_overview, SIGNAL(toggled(bool)), this, SLOT(showOverview(bool)));
		connect(overview_wgt, SIGNAL(s_overviewVisible(bool)), action_overview, SLOT(setChecked(bool)));

		if(action_overview->isChecked())
			overview_wgt->show(current_model);
	}
	else
		this->setWindowTitle(window_title);

	edit_menu->addSeparator();
	edit_menu->addAction(action_configuration);

	updateToolsState();

	oper_list_wgt->setModel(current_model);
	model_objs_wgt->setModel(current_model);
	model_valid_wgt->setModel(current_model);
	obj_finder_wgt->setModel(current_model);

	if(current_model)
		model_objs_wgt->restoreTreeState(model_tree_states[current_model]);

	model_objs_wgt->saveTreeState(true);
}

void MainWindow::setGridOptions(void)
{
	//Configures the global settings for the scene grid
	ObjectsScene::setGridOptions(action_show_grid->isChecked(),
															 action_alin_objs_grade->isChecked(),
															 action_show_delimiters->isChecked());

	if(current_model)
	{
		//Align the object to grid is the option is checked
		if(action_alin_objs_grade->isChecked())
			current_model->scene->alignObjectsToGrid();

		//Redraw the scene to apply the new grid options
		current_model->scene->update();
	}
}

void MainWindow::applyZoom(void)
{
	if(current_model)
	{
		float zoom=current_model->getCurrentZoom();

		if(sender()==action_normal_zoom)
			zoom=1;
		else if(sender()==action_inc_zoom && zoom < ModelWidget::MAXIMUM_ZOOM)
			zoom+=ModelWidget::ZOOM_INCREMENT;
		else if(sender()==action_dec_zoom && zoom > ModelWidget::MINIMUM_ZOOM)
			zoom-=ModelWidget::ZOOM_INCREMENT;

		current_model->applyZoom(zoom);
	}
}

void MainWindow::removeModelActions(void)
{
	QList<QAction *> act_list;
	act_list=general_tb->actions();

	while(act_list.size() > 5)
	{
    general_tb->removeAction(act_list.back());
		act_list.pop_back();
	}
}

void MainWindow::closeModel(int model_id)
{
	QWidget *tab=nullptr;

	overview_wgt->close();

	if(model_id >= 0)
		tab=models_tbw->widget(model_id);
	else
		tab=models_tbw->currentWidget();

	if(tab)
	{
		ModelWidget *model=dynamic_cast<ModelWidget *>(tab);
		Messagebox msg_box;

		//Ask the user to save the model if its modified
		if(model->isModified())
		{
			msg_box.show(trUtf8("Save model"),
									 trUtf8("The model was modified! Do you really want to close without save it?"),
									 Messagebox::CONFIRM_ICON, Messagebox::YES_NO_BUTTONS);
		}

		if(!model->isModified() ||
			 (model->isModified() && msg_box.result()==QDialog::Accepted))
		{
			model_nav_wgt->removeModel(model_id);
			model_tree_states.erase(model);

			disconnect(tab, nullptr, oper_list_wgt, nullptr);
			disconnect(tab, nullptr, model_objs_wgt, nullptr);
			disconnect(tab, nullptr, this, nullptr);
			disconnect(action_alin_objs_grade, nullptr, this, nullptr);
			disconnect(action_show_grid, nullptr, this, nullptr);
			disconnect(action_show_delimiters, nullptr, this, nullptr);

			//Remove the temporary file related to the closed model
			QDir arq_tmp;
			arq_tmp.remove(model->getTempFilename());

			//Removing model specific actions from general toolbar
			removeModelActions();

			if(model_id >= 0)
				models_tbw->removeTab(model_id);
			else
				models_tbw->removeTab(models_tbw->currentIndex());

			delete(model);
		}
	}

	if(models_tbw->count()==0)
	{
		current_model=nullptr;
		setCurrentModel();
		model_save_timer.stop();
		tmpmodel_save_timer.stop();
		models_tbw->setVisible(false);
	}
	else
	{
		setCurrentModel();
	}
}

void MainWindow::updateModelTabName(void)
{
	if(current_model && current_model->db_model->getName()!=models_tbw->tabText(models_tbw->currentIndex()))
		model_nav_wgt->updateModelText(models_tbw->currentIndex(), Utf8String::create(current_model->db_model->getName()), current_model->getFilename());
}

void MainWindow::applyConfigurations(void)
{
	if(!sender() ||
		 (sender()==configuration_form && configuration_form->result()==QDialog::Accepted))
	{
		GeneralConfigWidget *conf_wgt=nullptr;
		int count, i;
		ModelWidget *model=nullptr;

		conf_wgt=dynamic_cast<GeneralConfigWidget *>(configuration_form->getConfigurationWidget(ConfigurationForm::GENERAL_CONF_WGT));

		//Disable the auto save if the option is not checked
		if(!conf_wgt->autosave_interv_chk->isChecked())
		{
			//Stop the save timer
			model_save_timer.stop();
			model_save_timer.setInterval(0);
		}
		else
		{
			model_save_timer.setInterval(conf_wgt->autosave_interv_spb->value() * 60000);
			model_save_timer.start();
		}

		//Force the update of all opened models
		count=models_tbw->count();
		for(i=0; i < count; i++)
		{
			model=dynamic_cast<ModelWidget *>(models_tbw->widget(i));
			model->db_model->setObjectsModified();
			model->update();
		}

		updateConnections();
	}
}


void MainWindow::saveAllModels(void)
{
	if(models_tbw->count() > 0 &&
		 ((sender()==action_save_all) ||
			(sender()==&model_save_timer &&	this->isActiveWindow())))

	{
		int i, count;

		count=models_tbw->count();
		for(i=0; i < count; i++)
			this->saveModel(dynamic_cast<ModelWidget *>(models_tbw->widget(i)));
	}
}

void MainWindow::saveModel(ModelWidget *model)
{
	try
	{
		if(!model) model=current_model;

		if(model)
		{
			Messagebox msg_box;

			if(model->getDatabaseModel()->isInvalidated())
			{
				msg_box.show(trUtf8("Confirmation"),
										 trUtf8(" <strong>WARNING:</strong> The model <strong>%1</strong> is invalidated and it's extremely recommended that it be validated before save. Ignoring this situation can generate a broken model that will need manual fixes to be loadable again!").arg(model->getDatabaseModel()->getName()),
										 Messagebox::ALERT_ICON, Messagebox::ALL_BUTTONS,
										 trUtf8("Save anyway"), trUtf8("Validate"), "",
										 ":/icones/icones/salvar.png", ":/icones/icones/validation.png");

				//If the user cancel the saving force the stopping of autosave timer to give user the chance to validate the model
				if(msg_box.isCancelled())
				{
					model_save_timer.stop();

					//The autosave timer will be reactivated in 5 minutes
					QTimer::singleShot(300000, &model_save_timer, SLOT(start()));
				}
			}

			if((!model->getDatabaseModel()->isInvalidated() ||
					(model->getDatabaseModel()->isInvalidated() && msg_box.result()==QDialog::Accepted))
				 && (model->isModified() || sender()==action_save_as))
			{
				//If the action that calls the slot were the 'save as' or the model filename isn't set
				if(sender()==action_save_as || model->filename.isEmpty())
				{
					QFileDialog file_dlg;

					file_dlg.setDefaultSuffix("dbm");
					file_dlg.setWindowTitle(trUtf8("Save '%1' as...").arg(model->db_model->getName()));
					file_dlg.setNameFilter(tr("Database model (*.dbm);;All files (*.*)"));
					file_dlg.setFileMode(QFileDialog::AnyFile);
					file_dlg.setAcceptMode(QFileDialog::AcceptSave);
					file_dlg.setModal(true);

					if(file_dlg.exec()==QFileDialog::Accepted && !file_dlg.selectedFiles().isEmpty())
					{
						model->saveModel(file_dlg.selectedFiles().at(0));
						recent_models.push_front(file_dlg.selectedFiles().at(0));
						updateRecentModelsMenu();

						//models_tbw->setTabToolTip(models_tbw->indexOf(model), file_dlg.selectedFiles().at(0));
						model_nav_wgt->updateModelText(models_tbw->indexOf(model), model->getDatabaseModel()->getName(), file_dlg.selectedFiles().at(0));
					}
				}
				else
					model->saveModel();

				this->setWindowTitle(window_title + " - " + QDir::toNativeSeparators(model->getFilename()));
				model_valid_wgt->clearOutput();
			}
			//When the user click "Validate" on the message box the validation will be executed
			else if(model->getDatabaseModel()->isInvalidated() &&
							msg_box.result()==QDialog::Rejected && !msg_box.isCancelled())
			{
				validation_btn->setChecked(true);
				model_valid_wgt->validate_btn->click();
			}
		}
	}
	catch(Exception &e)
	{
		throw Exception(e.getErrorMessage(),e.getErrorType(),__PRETTY_FUNCTION__,__FILE__,__LINE__, &e);
	}
}

void MainWindow::exportModel(void)
{
	ModelExportForm model_export_form(nullptr, Qt::Dialog | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
	model_export_form.exec(current_model);
}

void MainWindow::importDatabase(void)
{
	DatabaseImportForm db_import_form(nullptr, Qt::Dialog | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
	db_import_form.exec();

	if(db_import_form.result()==QDialog::Accepted && db_import_form.getModelWidget())
		this->addModel(db_import_form.getModelWidget());
}

void MainWindow::printModel(void)
{
	if(current_model)
	{
		QPrinter *printer=nullptr;
		QPrinter::PageSize paper_size, curr_paper_size;
		QPrinter::Orientation orientation, curr_orientation;
		QRectF margins;
		QSizeF custom_size;
		qreal ml,mt,mr,mb, ml1, mt1, mr1, mb1;
		GeneralConfigWidget *conf_wgt=dynamic_cast<GeneralConfigWidget *>(configuration_form->getConfigurationWidget(ConfigurationForm::GENERAL_CONF_WGT));

		print_dlg->setOption(QAbstractPrintDialog::PrintCurrentPage, false);
		print_dlg->setWindowTitle(trUtf8("Database model printing"));

		//Get the page configuration of the scene
		ObjectsScene::getPaperConfiguration(paper_size, orientation, margins, custom_size);

		//Get a reference to the printer
		printer=print_dlg->printer();

		//Sets the printer options based upon the configurations from the scene
		ObjectsScene::configurePrinter(printer);

		printer->getPageMargins(&mt,&ml,&mb,&mr,QPrinter::Millimeter);

		print_dlg->exec();

		//If the user confirms the printing
		if(print_dlg->result() == QDialog::Accepted)
		{
			Messagebox msg_box;

			//Checking If the user modified the default settings overriding the scene configurations
			printer->getPageMargins(&mt1,&ml1,&mb1,&mr1,QPrinter::Millimeter);
			curr_orientation=print_dlg->printer()->orientation();
			curr_paper_size=print_dlg->printer()->paperSize();

			if(ml!=ml1 || mr!=mr1 || mt!=mt1 || mb!=mb1 ||
				 orientation!=curr_orientation || curr_paper_size!=paper_size)
			{
				msg_box.show(trUtf8("Confirmation"),
										 trUtf8("Changes were detected in the definitions of paper/margin of the model which may cause the incorrect print of the objects. Do you want to continue printing using the new settings? To use the default settings click 'No' or 'Cancel' to abort printing."),
										 Messagebox::ALERT_ICON, Messagebox::ALL_BUTTONS);
			}

			if(!msg_box.isCancelled())
			{
				if(msg_box.result()==QDialog::Rejected)
					ObjectsScene::configurePrinter(printer);

				current_model->printModel(printer, conf_wgt->print_grid_chk->isChecked(), conf_wgt->print_pg_num_chk->isChecked());
			}
		}
	}
}

void MainWindow::loadModel(void)
{
	QFileDialog file_dlg;

	try
	{
		file_dlg.setNameFilter(trUtf8("Database model (*.dbm);;All files (*.*)"));
		file_dlg.setWindowIcon(QPixmap(QString(":/icones/icones/pgsqlModeler48x48.png")));
		file_dlg.setWindowTitle(trUtf8("Load model"));
		file_dlg.setFileMode(QFileDialog::ExistingFiles);
		file_dlg.setAcceptMode(QFileDialog::AcceptOpen);

		if(file_dlg.exec()==QFileDialog::Accepted)
			loadModels(file_dlg.selectedFiles());
	}
	catch(Exception &e)
	{
		Messagebox msg_box;
		msg_box.show(e);
	}
}

void MainWindow::loadModel(const QString &filename)
{
	loadModels({ filename });
}

void MainWindow::loadModels(const QStringList &list)
{
	int i=0;

	try
	{
		for(i=0; i < list.count(); i++)
		{
			addModel(list[i]);
			recent_models.push_front(list[i]);
		}

		updateRecentModelsMenu();
		saveTemporaryModels(true);
	}
	catch(Exception &e)
	{	
		Messagebox msg_box;

		msg_box.show(Exception(Exception::getErrorMessage(ERR_MODEL_FILE_NOT_LOADED).arg(list[i]),
													 ERR_MODEL_FILE_NOT_LOADED ,__PRETTY_FUNCTION__,__FILE__,__LINE__, &e),
								 trUtf8("Could not load the database model file `%1'. Check the error stack to see details. You can try to fix it in order to make it loadable again.").arg(list[i]),
								 Messagebox::ERROR_ICON, Messagebox::YES_NO_BUTTONS,
								 trUtf8("Fix model"), trUtf8("Cancel"), "",
								 ":/icones/icones/fixobject.png", ":/icones/icones/msgbox_erro.png");

		if(msg_box.result()==QDialog::Accepted)
			fixModel(list[i]);
	}
}

void MainWindow::__updateToolsState(void)
{
	updateToolsState(false);
}

void MainWindow::updateToolsState(bool model_closed)
{
	bool enabled=(!model_closed && current_model);

	action_print->setEnabled(enabled);
	action_save_as->setEnabled(enabled);
	action_save_model->setEnabled(enabled);
	action_save_all->setEnabled(enabled);
	action_export->setEnabled(enabled);
	action_close_model->setEnabled(enabled);
	action_show_grid->setEnabled(enabled);
	action_show_delimiters->setEnabled(enabled);
	action_overview->setEnabled(enabled);

	action_normal_zoom->setEnabled(enabled);
	action_inc_zoom->setEnabled(enabled);
	action_dec_zoom->setEnabled(enabled);
	action_alin_objs_grade->setEnabled(enabled);
	action_undo->setEnabled(enabled);
	action_redo->setEnabled(enabled);

	if(!model_closed && current_model && models_tbw->count() > 0)
	{
		action_undo->setEnabled(current_model->op_list->isUndoAvailable());
		action_redo->setEnabled(current_model->op_list->isRedoAvailable());

		action_inc_zoom->setEnabled(current_model->getCurrentZoom() <= ModelWidget::MAXIMUM_ZOOM - ModelWidget::ZOOM_INCREMENT);
		action_normal_zoom->setEnabled(current_model->getCurrentZoom()!=0);
		action_dec_zoom->setEnabled(current_model->getCurrentZoom() >= ModelWidget::MINIMUM_ZOOM + ModelWidget::ZOOM_INCREMENT);
	}
}

void MainWindow::updateDockWidgets(void)
{
	oper_list_wgt->updateOperationList();
	model_objs_wgt->updateObjectsView();

	/* Any operation executed over the model will reset the validation and
	the finder will execute the search again */
	model_valid_wgt->setModel(current_model);

	if(current_model && obj_finder_wgt->result_tbw->rowCount() > 0)
		obj_finder_wgt->findObjects();
}

void MainWindow::executePlugin(void)
{
	QAction *action=dynamic_cast<QAction *>(sender());

	if(action)
	{
		PgModelerPlugin *plugin=reinterpret_cast<PgModelerPlugin *>(action->data().value<void *>());

		if(plugin)
			plugin->executePlugin(current_model);
	}
}

void MainWindow::showOverview(bool show)
{
	if(show && current_model && !overview_wgt->isVisible())
		overview_wgt->show(current_model);
	else if(!show)
		overview_wgt->close();
}

void MainWindow::openWiki(void)
{
	Messagebox msg_box;

	msg_box.show(trUtf8("Open Wiki pages"),
							 trUtf8("This action will open a web browser window! Want to proceed?"),
							 Messagebox::CONFIRM_ICON,Messagebox::YES_NO_BUTTONS);

	if(msg_box.result()==QDialog::Accepted)
		QDesktopServices::openUrl(QUrl(GlobalAttributes::PGMODELER_WIKI));
}

void MainWindow::toggleUpdateNotifier(bool show)
{
	if(show)
	{
		setFloatingWidgetPos(update_notifier_wgt, qobject_cast<QAction *>(sender()), control_tb, false);
		action_about->setChecked(false);
	}

	update_notifier_wgt->setVisible(show);
}

void MainWindow::toggleAboutWidget(bool show)
{
	if(show)
	{
		setFloatingWidgetPos(about_wgt, qobject_cast<QAction *>(sender()), control_tb, false);
		action_update_found->setChecked(false);
	}

	about_wgt->setVisible(show);
}

bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
	QWidget *wgt=qobject_cast<QWidget *>(object);

	if((event->type()==QEvent::Resize || event->type()==QEvent::Show || event->type()==QEvent::Hide) &&
		 (wgt==model_valid_parent || wgt==obj_finder_parent || wgt==sql_tool_parent || wgt==models_tbw))
	{
		QWidgetList wgt_list={ model_valid_parent, obj_finder_parent, sql_tool_parent, models_tbw };
		QPoint pos;
		QRect ret;
		bool intersects=false;

		for(auto wgt_aux : wgt_list)
		{
			pos=(wgt_aux==sql_tool_parent ? wgt_aux->pos() : wgt_aux->mapTo(models_tbw_parent, wgt_aux->pos()));
			ret=QRect(QPoint(0, pos.y()), wgt_aux->size());

			if(!intersects && wgt_aux->isVisible() && ret.intersects(central_wgt->geometry()))
			{
				intersects=true;
				break;
			}
		}

		if(intersects)
			central_wgt->lower();
		else if(!models_tbw->isVisible())
			central_wgt->raise();
	}

	return(QMainWindow::eventFilter(object, event));
}

void MainWindow::setFloatingWidgetPos(QWidget *widget, QAction *act, QToolBar *toolbar, bool map_to_window)
{
	if(widget && act && toolbar)
	{
		QWidget *wgt=toolbar->widgetForAction(act);
		QPoint pos=(wgt ? wgt->pos() : QPoint(0,0));

		if(map_to_window) pos=wgt->mapTo(this, pos);
		pos.setX(pos.x() - 9);
		pos.setY(toolbar->pos().y() + toolbar->height() - 9);
		widget->move(pos);
	}
}

QGraphicsDropShadowEffect *MainWindow::createDropShadow(QToolButton *btn)
{
	QGraphicsDropShadowEffect *shadow=nullptr;

	shadow=new QGraphicsDropShadowEffect(btn);
	shadow->setXOffset(2);
	shadow->setYOffset(2);
	shadow->setBlurRadius(5);
	shadow->setColor(QColor(0,0,0, 100));

	return(shadow);
}

void MainWindow::configureSamplesMenu(void)
{
	QDir dir(GlobalAttributes::SAMPLES_DIR);
	QStringList files=dir.entryList({"*.dbm"});
	QAction *act=nullptr;
	QString path;

	while(!files.isEmpty())
	{
		act=sample_mdls_menu.addAction(files.front(),this,SLOT(loadModelFromAction(void)));
		path=QFileInfo(GlobalAttributes::SAMPLES_DIR + GlobalAttributes::DIR_SEPARATOR + files.front()).absoluteFilePath();
		act->setToolTip(path);
		act->setData(path);
		files.pop_front();
	}

	if(sample_mdls_menu.isEmpty())
	{
		act=sample_mdls_menu.addAction(trUtf8("(no samples found)"));
		act->setEnabled(false);
	}

	central_wgt->sample_tb->setMenu(&sample_mdls_menu);
}
