// synthv1widget_lv2.cpp
//
/****************************************************************************
   Copyright (C) 2012-2015, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "synthv1widget_lv2.h"

#include "synthv1_lv2.h"

#include "lv2/lv2plug.in/ns/ext/instance-access/instance-access.h"

#include <QApplication>
#include <QCloseEvent>


//-------------------------------------------------------------------------
// synthv1widget_lv2 - impl.
//

synthv1widget_lv2::synthv1widget_lv2 ( synthv1_lv2 *pSynthUi,
	LV2UI_Controller controller, LV2UI_Write_Function write_function )
	: synthv1widget(), m_pSynthUi(pSynthUi)
{
	m_controller = controller;
	m_write_function = write_function;

#ifdef CONFIG_LV2_EXTERNAL_UI
	m_external_host = NULL;
#endif
#ifdef CONFIG_LV2_UI_IDLE
	m_bIdleClosed = false;
#endif

	for (uint32_t i = 0; i < synthv1::NUM_PARAMS; ++i)
		m_params_def[i] = true;

	clearPreset();
}


// Synth engine accessor.
synthv1_ui *synthv1widget_lv2::ui_instance (void) const
{
	return m_pSynthUi;
}


#ifdef CONFIG_LV2_EXTERNAL_UI

void synthv1widget_lv2::setExternalHost ( LV2_External_UI_Host *external_host )
{
	m_external_host = external_host;

	if (m_external_host && m_external_host->plugin_human_id)
		synthv1widget::setWindowTitle(m_external_host->plugin_human_id);
}

const LV2_External_UI_Host *synthv1widget_lv2::externalHost (void) const
{
	return m_external_host;
}

#endif	// CONFIG_LV2_EXTERNAL_UI


#ifdef CONFIG_LV2_UI_IDLE

bool synthv1widget_lv2::isIdleClosed (void) const
{
	return m_bIdleClosed;
}

#endif	// CONFIG_LV2_UI_IDLE


// Close event handler.
void synthv1widget_lv2::closeEvent ( QCloseEvent *pCloseEvent )
{
	synthv1widget::closeEvent(pCloseEvent);

#ifdef CONFIG_LV2_UI_IDLE
	if (pCloseEvent->isAccepted())
		m_bIdleClosed = true;
#endif
#ifdef CONFIG_LV2_EXTERNAL_UI
	if (m_external_host && m_external_host->ui_closed) {
		if (pCloseEvent->isAccepted())
			m_external_host->ui_closed(m_controller);
	}
#endif
}


void synthv1widget_lv2::port_event ( uint32_t port_index,
	uint32_t buffer_size, uint32_t format, const void *buffer )
{
	if (format == 0 && buffer_size == sizeof(float)) {
		synthv1::ParamIndex index
			= synthv1::ParamIndex(port_index - synthv1_lv2::ParamBase);
		float fValue = *(float *) buffer;
	//--legacy support < 0.3.0.4 -- begin
		if (index == synthv1::DEL1_BPM && fValue < 3.6f)
			fValue *= 100.0f;
	//--legacy support < 0.3.0.4 -- end.
		setParamValue(index, fValue, m_params_def[index]);
		m_params_def[index] = false;
	}
}


// Param method.
void synthv1widget_lv2::updateParam (
	synthv1::ParamIndex index, float fValue ) const
{
	m_write_function(m_controller,
		synthv1_lv2::ParamBase + index, sizeof(float), 0, &fValue);
}


//-------------------------------------------------------------------------
// synthv1widget_lv2 - LV2 UI desc.
//

static QApplication *synthv1_lv2ui_qapp_instance = NULL;
static unsigned int  synthv1_lv2ui_qapp_refcount = 0;


static LV2UI_Handle synthv1_lv2ui_instantiate (
	const LV2UI_Descriptor *, const char *, const char *,
	LV2UI_Write_Function write_function,
	LV2UI_Controller controller, LV2UI_Widget *widget,
	const LV2_Feature *const *features )
{
	synthv1_lv2 *pSynth = NULL;

	for (int i = 0; features && features[i]; ++i) {
		if (::strcmp(features[i]->URI, LV2_INSTANCE_ACCESS_URI) == 0) {
			pSynth = static_cast<synthv1_lv2 *> (features[i]->data);
			break;
		}
	}

	if (pSynth == NULL)
		return NULL;

	if (qApp == NULL && synthv1_lv2ui_qapp_instance == NULL) {
		static int s_argc = 1;
		static const char *s_argv[] = { __func__, NULL };
		synthv1_lv2ui_qapp_instance = new QApplication(s_argc, (char **) s_argv);
	}
	synthv1_lv2ui_qapp_refcount++;

	synthv1widget_lv2 *pWidget
		= new synthv1widget_lv2(pSynth, controller, write_function);
	*widget = pWidget;
	return pWidget;
}

static void synthv1_lv2ui_cleanup ( LV2UI_Handle ui )
{
	synthv1widget_lv2 *pWidget = static_cast<synthv1widget_lv2 *> (ui);
	if (pWidget) {
		delete pWidget;
		if (--synthv1_lv2ui_qapp_refcount == 0 && synthv1_lv2ui_qapp_instance) {
			delete synthv1_lv2ui_qapp_instance;
			synthv1_lv2ui_qapp_instance = NULL;
		}
	}
}

static void synthv1_lv2ui_port_event (
	LV2UI_Handle ui, uint32_t port_index,
	uint32_t buffer_size, uint32_t format, const void *buffer )
{
	synthv1widget_lv2 *pWidget = static_cast<synthv1widget_lv2 *> (ui);
	if (pWidget)
		pWidget->port_event(port_index, buffer_size, format, buffer);
}


#ifdef CONFIG_LV2_UI_IDLE

int synthv1_lv2ui_idle ( LV2UI_Handle ui )
{
	synthv1widget_lv2 *pWidget = static_cast<synthv1widget_lv2 *> (ui);
	if  (pWidget && !pWidget->isIdleClosed()) {
		QApplication::processEvents();
		return 0;
	} else {
		return 1;
	}
}

static const LV2UI_Idle_Interface synthv1_lv2ui_idle_interface =
{
	synthv1_lv2ui_idle
};

#endif	// CONFIG_LV2_UI_IDLE


#ifdef CONFIG_LV2_UI_SHOW

int synthv1_lv2ui_show ( LV2UI_Handle ui )
{
	synthv1widget_lv2 *pWidget = static_cast<synthv1widget_lv2 *> (ui);
	if (pWidget) {
		pWidget->show();
		pWidget->raise();
		pWidget->activateWindow();
		return 0;
	} else {
		return 1;
	}
}

int synthv1_lv2ui_hide ( LV2UI_Handle ui )
{
	synthv1widget_lv2 *pWidget = static_cast<synthv1widget_lv2 *> (ui);
	if (pWidget) {
		pWidget->hide();
		return 0;
	} else {
		return 1;
	}
}

static const LV2UI_Show_Interface synthv1_lv2ui_show_interface =
{
	synthv1_lv2ui_show,
	synthv1_lv2ui_hide
};

#endif	// CONFIG_LV2_UI_IDLE

static const void *synthv1_lv2ui_extension_data ( const char *uri )
{
#ifdef CONFIG_LV2_UI_IDLE
	if (::strcmp(uri, LV2_UI__idleInterface) == 0)
		return (void *) &synthv1_lv2ui_idle_interface;
	else
#endif
#ifdef CONFIG_LV2_UI_SHOW
	if (::strcmp(uri, LV2_UI__showInterface) == 0)
		return (void *) &synthv1_lv2ui_show_interface;
	else
#endif
	return NULL;
}


#ifdef CONFIG_LV2_EXTERNAL_UI

struct synthv1_lv2ui_external_widget
{
	LV2_External_UI_Widget external;
	synthv1widget_lv2     *widget;
};

static void synthv1_lv2ui_external_run ( LV2_External_UI_Widget *ui_external )
{
	synthv1_lv2ui_external_widget *pExtWidget
		= (synthv1_lv2ui_external_widget *) (ui_external);
	if (pExtWidget)
		QApplication::processEvents();
}

static void synthv1_lv2ui_external_show ( LV2_External_UI_Widget *ui_external )
{
	synthv1_lv2ui_external_widget *pExtWidget
		= (synthv1_lv2ui_external_widget *) (ui_external);
	if (pExtWidget) {
		synthv1widget_lv2 *widget = pExtWidget->widget;
		if (widget) {
			widget->show();
			widget->raise();
			widget->activateWindow();
		}
	}
}

static void synthv1_lv2ui_external_hide ( LV2_External_UI_Widget *ui_external )
{
	synthv1_lv2ui_external_widget *pExtWidget
		= (synthv1_lv2ui_external_widget *) (ui_external);
	if (pExtWidget && pExtWidget->widget)
		pExtWidget->widget->hide();
}

static LV2UI_Handle synthv1_lv2ui_external_instantiate (
	const LV2UI_Descriptor *, const char *, const char *,
	LV2UI_Write_Function write_function,
	LV2UI_Controller controller, LV2UI_Widget *widget,
	const LV2_Feature *const *ui_features )
{
	synthv1_lv2 *pSynth = NULL;
	LV2_External_UI_Host *external_host = NULL;

	for (int i = 0; ui_features[i] && !external_host; ++i) {
		if (::strcmp(ui_features[i]->URI, LV2_INSTANCE_ACCESS_URI) == 0)
			pSynth = static_cast<synthv1_lv2 *> (ui_features[i]->data);
		else
		if (::strcmp(ui_features[i]->URI, LV2_EXTERNAL_UI__Host) == 0 ||
			::strcmp(ui_features[i]->URI, LV2_EXTERNAL_UI_DEPRECATED_URI) == 0) {
			external_host = (LV2_External_UI_Host *) ui_features[i]->data;
		}
	}

	if (qApp == NULL && synthv1_lv2ui_qapp_instance == NULL) {
		static int s_argc = 1;
		static const char *s_argv[] = { __func__, NULL };
		synthv1_lv2ui_qapp_instance = new QApplication(s_argc, (char **) s_argv);
	}
	synthv1_lv2ui_qapp_refcount++;

	synthv1_lv2ui_external_widget *pExtWidget = new synthv1_lv2ui_external_widget;
	pExtWidget->external.run  = synthv1_lv2ui_external_run;
	pExtWidget->external.show = synthv1_lv2ui_external_show;
	pExtWidget->external.hide = synthv1_lv2ui_external_hide;
	pExtWidget->widget = new synthv1widget_lv2(pSynth, controller, write_function);
	if (external_host)
		pExtWidget->widget->setExternalHost(external_host);
	*widget = pExtWidget;
	return pExtWidget;
}

static void synthv1_lv2ui_external_cleanup ( LV2UI_Handle ui )
{
	synthv1_lv2ui_external_widget *pExtWidget
		= static_cast<synthv1_lv2ui_external_widget *> (ui);
	if (pExtWidget) {
		if (pExtWidget->widget)
			delete pExtWidget->widget;
		delete pExtWidget;
		if (--synthv1_lv2ui_qapp_refcount == 0 && synthv1_lv2ui_qapp_instance) {
			delete synthv1_lv2ui_qapp_instance;
			synthv1_lv2ui_qapp_instance = NULL;
		}
	}
}

static void synthv1_lv2ui_external_port_event (
	LV2UI_Handle ui, uint32_t port_index,
	uint32_t buffer_size, uint32_t format, const void *buffer )
{
	synthv1_lv2ui_external_widget *pExtWidget
		= static_cast<synthv1_lv2ui_external_widget *> (ui);
	if (pExtWidget && pExtWidget->widget)
		pExtWidget->widget->port_event(port_index, buffer_size, format, buffer);
}

static const void *synthv1_lv2ui_external_extension_data ( const char * )
{
	return NULL;
}

#endif	// CONFIG_LV2_EXTERNAL_UI


static const LV2UI_Descriptor synthv1_lv2ui_descriptor =
{
	SYNTHV1_LV2UI_URI,
	synthv1_lv2ui_instantiate,
	synthv1_lv2ui_cleanup,
	synthv1_lv2ui_port_event,
	synthv1_lv2ui_extension_data
};

#ifdef CONFIG_LV2_EXTERNAL_UI
static const LV2UI_Descriptor synthv1_lv2ui_external_descriptor =
{
	SYNTHV1_LV2UI_EXTERNAL_URI,
	synthv1_lv2ui_external_instantiate,
	synthv1_lv2ui_external_cleanup,
	synthv1_lv2ui_external_port_event,
	synthv1_lv2ui_external_extension_data
};
#endif	// CONFIG_LV2_EXTERNAL_UI


LV2_SYMBOL_EXPORT const LV2UI_Descriptor *lv2ui_descriptor ( uint32_t index )
{
	if (index == 0)
		return &synthv1_lv2ui_descriptor;
	else
#ifdef CONFIG_LV2_EXTERNAL_UI
	if (index == 1)
		return &synthv1_lv2ui_external_descriptor;
	else
#endif
	return NULL;
}


// end of synthv1widget_lv2.cpp
