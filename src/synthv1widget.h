// synthv1widget.h
//
/****************************************************************************
   Copyright (C) 2012, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*****************************************************************************/

#ifndef __synthv1widget_h
#define __synthv1widget_h

#include "ui_synthv1widget.h"

#include "synthv1widget_config.h"

#include "synthv1.h"

#include <QHash>


//-------------------------------------------------------------------------
// synthv1widget - decl.
//

class synthv1widget : public QWidget
{
	Q_OBJECT

public:

	// Constructor
	synthv1widget(QWidget *pParent = 0, Qt::WindowFlags wflags = 0);

	// Param port accessors.
	void setParamValue(synthv1::ParamIndex index, float fValue);
	float paramValue(synthv1::ParamIndex index) const;

	// Param kbob (widget) mapper.
	void setParamKnob(synthv1::ParamIndex index, synthv1widget_knob *pKnob);
	synthv1widget_knob *paramKnob(synthv1::ParamIndex index) const;

public slots:

	// Preset file I/O.
	void loadPreset(const QString& sFilename);
	void savePreset(const QString& sFilename);

protected slots:

	// Preset clear.
	void newPreset();

	// Param knob (widget) slots.
	void paramChanged(float fValue);

	// Menu actions.
	void helpAbout();
	void helpAboutQt();

protected:

	// Application close.
	void closeEvent(QCloseEvent *pCloseEvent);

	// Preset init.
	void initPreset();

	// Reset all param/knob default values.
	void resetParamValues();
	void resetParamKnobs();

	// Param port methods.
	virtual void updateParam(synthv1::ParamIndex index, float fValue) const = 0;

private:

	// Instance variables.
	Ui::synthv1widget m_ui;

	synthv1widget_config m_config;

	QHash<synthv1::ParamIndex, synthv1widget_knob *> m_paramKnobs;
	QHash<synthv1widget_knob *, synthv1::ParamIndex> m_knobParams;

	int m_iUpdate;
};


#endif	// __synthv1widget_h

// end of synthv1widget.h
