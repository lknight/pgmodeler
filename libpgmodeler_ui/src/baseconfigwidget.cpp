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

#include "baseconfigwidget.h"

void BaseConfigWidget::addConfigurationParam(const QString &param, const attribs_map &attribs)
{
	if(!param.isEmpty() && !attribs.empty())
		config_params[param]=attribs;
}

map<QString, attribs_map > BaseConfigWidget::getConfigurationParams(void)
{
	return(config_params);
}

void BaseConfigWidget::removeConfigurationParam(const QString &param)
{
	config_params.erase(param);
}

void BaseConfigWidget::removeConfigurationParams(void)
{
	config_params.clear();
}

void BaseConfigWidget::saveConfiguration(const QString &conf_id)
{
	QByteArray buf;

						//Configures the schema filename for the configuration
	QString	sch_filename=GlobalAttributes::CONFIGURATIONS_DIR +
									 GlobalAttributes::DIR_SEPARATOR +
									 GlobalAttributes::SCHEMAS_DIR +
									 GlobalAttributes::DIR_SEPARATOR +
									 conf_id +
									 GlobalAttributes::SCHEMA_EXT,

			//Cofnigures the filename for the configuration file
			cfg_filename=GlobalAttributes::CONFIGURATIONS_DIR +
									 GlobalAttributes::DIR_SEPARATOR +
									 conf_id +
									 GlobalAttributes::CONFIGURATION_EXT;
	QFile output(cfg_filename);
	attribs_map attribs;
	map<QString, attribs_map >::iterator itr, itr_end;

	try
	{
		itr=config_params.begin();
		itr_end=config_params.end();

		while(itr!=itr_end)
		{
			attribs.insert((itr->second).begin(), (itr->second).end());
			itr++;
		}

		//Generates the configuration from the schema file
		schparser.setIgnoreEmptyAttributes(true);
		buf.append(schparser.getCodeDefinition(sch_filename, attribs));
		output.open(QFile::WriteOnly);

		if(!output.isOpen())
			throw Exception(Exception::getErrorMessage(ERR_FILE_NOT_WRITTEN).arg(Utf8String::create(cfg_filename)),
											ERR_FILE_NOT_WRITTEN,__PRETTY_FUNCTION__,__FILE__,__LINE__);

		//Writes the generated configuration to the output file
		output.write(buf.data(), buf.size());
		output.close();
	}
	catch(Exception &e)
	{
		if(output.isOpen()) output.close();
		throw Exception(Exception::getErrorMessage(ERR_FILE_NOT_WRITTER_INV_DEF).arg(Utf8String::create(cfg_filename)),
										ERR_FILE_NOT_WRITTER_INV_DEF,__PRETTY_FUNCTION__,__FILE__,__LINE__, &e);
	}
}

void BaseConfigWidget::restoreDefaults(const QString &conf_id)
{
	QString current_file, default_file;

	//Build the path to the current configuration (conf/[conf_id].conf
	current_file=GlobalAttributes::CONFIGURATIONS_DIR +
							 GlobalAttributes::DIR_SEPARATOR +
							 conf_id +
							 GlobalAttributes::CONFIGURATION_EXT;

	//Build the path to the default configuration file (conf/defaults/[conf_id].conf
	default_file=GlobalAttributes::CONFIGURATIONS_DIR +
							 GlobalAttributes::DIR_SEPARATOR +
							 GlobalAttributes::DEFAULT_CONFS_DIR+
							 GlobalAttributes::DIR_SEPARATOR +
							 conf_id +
							 GlobalAttributes::CONFIGURATION_EXT;

	//Raises an error if the default file doesn't exists
	if(!QFile::exists(default_file))
		throw Exception(Exception::getErrorMessage(ERR_DEFAULT_CONFIG_NOT_REST).arg(Utf8String::create(default_file)),
										ERR_DEFAULT_CONFIG_NOT_REST,__PRETTY_FUNCTION__,__FILE__,__LINE__);
	else
	{
		//Overwrites the current file with the default
		QFile::remove(current_file);
		QFile::copy(default_file, current_file);
	}
}

void BaseConfigWidget::loadConfiguration(const QString &conf_id, const vector<QString> &key_attribs)
{
	try
	{
		config_params.clear();
		xmlparser.restartParser();
		xmlparser.setDTDFile(GlobalAttributes::CONFIGURATIONS_DIR +
													GlobalAttributes::DIR_SEPARATOR +
													GlobalAttributes::OBJECT_DTD_DIR +
													GlobalAttributes::DIR_SEPARATOR +
													conf_id +
													GlobalAttributes::OBJECT_DTD_EXT,
													conf_id);

		xmlparser.loadXMLFile(GlobalAttributes::CONFIGURATIONS_DIR +
													 GlobalAttributes::DIR_SEPARATOR +
													 conf_id +
													 GlobalAttributes::CONFIGURATION_EXT);

		if(xmlparser.accessElement(XMLParser::CHILD_ELEMENT))
		{
			do
			{
				if(xmlparser.getElementType()==XML_ELEMENT_NODE)
				{
					this->getConfigurationParams(key_attribs);

					if(xmlparser.hasElement(XMLParser::CHILD_ELEMENT))
					{
						xmlparser.savePosition();
						xmlparser.accessElement(XMLParser::CHILD_ELEMENT);

						do
						{
							this->getConfigurationParams(key_attribs);
						}
						while(xmlparser.accessElement(XMLParser::NEXT_ELEMENT));

						xmlparser.restorePosition();
					}
				}
			}
			while(xmlparser.accessElement(XMLParser::NEXT_ELEMENT));
		}
	}
	catch(Exception &e)
	{
		throw Exception(e.getErrorMessage(),e.getErrorType(),__PRETTY_FUNCTION__,__FILE__,__LINE__, &e);
	}
}

void BaseConfigWidget::getConfigurationParams(const vector<QString> &key_attribs)
{
	attribs_map aux_attribs;
	attribs_map::iterator itr, itr_end;
	QString key;

	xmlparser.getElementAttributes(aux_attribs);

	itr=aux_attribs.begin();
	itr_end=aux_attribs.end();

	while(itr!=itr_end && key.isEmpty())
	{
		if(key.isEmpty() && find(key_attribs.begin(), key_attribs.end(), itr->first)!=key_attribs.end())
			key=itr->second;

		itr++;
	}

	if(key.isEmpty())
		key=xmlparser.getElementName();

	if(!aux_attribs.empty())
		config_params[key]=aux_attribs;
}

