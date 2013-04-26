// synthv1widget_jack.h
//
/****************************************************************************
   Copyright (C) 2012-2013, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __synthv1widget_jack_h
#define __synthv1widget_jack_h

#include "synthv1widget.h"


// Forward decls.
class synthv1_jack;

#ifdef CONFIG_NSM
class synthv1_nsm;
#endif


//-------------------------------------------------------------------------
// synthv1widget_jack - decl.
//

class synthv1widget_jack : public synthv1widget
{
	Q_OBJECT

public:

	// Constructor.
	synthv1widget_jack(synthv1_jack *pSynth);

	// Destructor.
	~synthv1widget_jack();

#ifdef CONFIG_JACK_SESSION

	// JACK session self-notification.
	void notifySessionEvent(void *pvSessionArg);

signals:

	// JACK session notify event.
	void sessionNotify(void *);

protected slots:

	// JACK session event handler.
	void sessionEvent(void *pvSessionArg);

#endif	// CONFIG_JACK_SESSION

#ifdef CONFIG_NSM

protected slots:

	// NSM callback slots.
	void openSession();
	void saveSession();
	
#endif	// CONFIG_NSM

protected:

	// Param methods.
	void updateParam(synthv1::ParamIndex index, float fValue) const;

	// Application close.
	void closeEvent(QCloseEvent *pCloseEvent);

private:

	// Instance variables.
	synthv1_jack *m_pSynth;

#ifdef CONFIG_NSM
	synthv1_nsm *m_nsm;
#endif
};


#endif	// __synthv1widget_jack_h

// end of synthv1widget_jack.h
