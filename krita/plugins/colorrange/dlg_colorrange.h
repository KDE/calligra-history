/*
 *  dlg_colorrange.h -- part of KimageShop^WKrayon^WKrita
 *
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 */
#ifndef DLG_COLORRANGE
#define DLG_COLORRANGE

#include <qpixmap.h>

#include <kdialogbase.h>

#include "kis_types.h"
#include "wdg_colorrange.h"


/**
 * This dialog allows the user to create a selection mask based
 * on a (range of) colors.
 */
class DlgColorRange: public KDialogBase {
	typedef KDialogBase super;
	Q_OBJECT

public:

	DlgColorRange(KisLayerSP layer, QWidget * parent = 0, const char* name = 0);
	~DlgColorRange();

private slots:

	void okClicked();

	void slotPickerPlusClicked();
	void slotPickerClicked();
	void slotLoad();
	void slotPickerMinusClicked();
	void slotSave();
	void slotInvertClicked();
	void slotFuzzinessChanged(int value);
	void slotSliderMoved(int value);
	void slotSelectionTypeChanged(int index);
	void slotPreviewTypeChanged(int index);
	void updatePreview();


private:
	QImage createMask(KisSelectionSP selection, KisLayerSP layer);
	
private:

	WdgColorRange * m_page;
	KisSelectionSP m_selection;
	KisLayerSP m_layer;
};

#endif // DLG_COLORRANGE
