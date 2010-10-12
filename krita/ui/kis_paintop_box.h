/*
 *  kis_paintop_box.h - part of KImageShop/Krayon/Krita
 *
 *  Copyright (c) 2004-2008 Boudewijn Rempt (boud@valdyas.org)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef KIS_PAINTOP_BOX_H_
#define KIS_PAINTOP_BOX_H_

#include <QHash>
#include <QWidget>
#include <QList>
#include <QPixmap>

#include <kcombobox.h>

#include <KoInputDevice.h>

#include <kis_types.h>
#include <kis_paintop_settings.h>

class QPushButton;
class QString;
class QHBoxLayout;

class KoID;
class KoColorSpace;
class KoResourceSelector;
class KoResource;
class KoCompositeOp;

class KisView2;
class KisCanvasResourceProvider;
class KisPopupButton;
class KisPaintOpPresetsPopup;
class KisPaintOpPresetsChooserPopup;
class KisPaintOpSettingsWidget;
class KisCmbPaintop;
class KisCmbComposite;
class KisBrushEngineSelector;

/**
 * This widget presents all paintops that a user can paint with.
 * Paintops represent real-world tools or the well-known Shoup
 * computer equivalents that do nothing but change color.
 *
 * XXX: When we have a lot of paintops, replace the listbox
 * with a table, and for every category a combobox.
 *
 * XXX: instead of text, use pretty pictures.
 */
class KisPaintopBox : public QWidget
{

    Q_OBJECT

public:
    KisPaintopBox(KisView2 * view,  QWidget * parent, const char * name);
    KisPaintOpPresetSP paintOpPresetSP(KoID * = 0);
    const KoID & currentPaintop();
    void setCurrentPaintop(const KoID & paintop);
    QPixmap paintopPixmap(const KoID & paintop);
    ~KisPaintopBox();

signals:
    void signalPaintopChanged(KisPaintOpPresetSP paintop);

public slots:

    void colorSpaceChanged(const KoColorSpace *cs);
    void slotInputDeviceChanged(const KoInputDevice & inputDevice);
    void slotCurrentNodeChanged(KisNodeSP node);
    void slotSaveActivePreset();
    void slotUpdatePreset();
    void slotSetupDefaultPreset();
    void resourceSelected( KoResource * resource );

private:

    KoID defaultPaintop(const KoInputDevice & inputDevice);
    KisPaintOpPresetSP activePreset(const KoID & paintop, const KoInputDevice & inputDevice);

    ///Sets the current composite op in the canvas resource provide
    ///Composite op will be set to eraser if the erase mode of the input device is active
    void compositeOpChanged();

    ///Sets the internal composite op, without emitting
    /// @param id id of the composite op, when empty COMPOSITE_OVER will be used
    void setCompositeOpInternal(const QString & id);

    void setEnabledInternal(bool value);

private slots:

    void updatePaintops();
    void nodeChanged(const KisNodeSP node);
    void eraseModeToggled(bool checked);
    void updateCompositeOpComboBox();
    void slotSetCompositeMode(const QString& compositeOp);
    void slotSetPaintop(const QString& paintOpId);
    void slotSaveToFavouriteBrushes();

private:

    const KoColorSpace* m_colorspace;

    KisCanvasResourceProvider *m_resourceProvider;
    KisCmbPaintop* m_cmbPaintops;

    QHBoxLayout* m_layout;
    KisPaintOpSettingsWidget* m_optionWidget;
    KisPopupButton* m_settingsWidget;
    KisPopupButton* m_presetWidget;
    KisPopupButton* m_brushChooser;
    KisCmbComposite* m_cmbComposite;
    QPushButton* m_eraseModeButton;
    KisPaintOpPresetsPopup* m_presetsPopup;
    KisPaintOpPresetsChooserPopup* m_presetsChooserPopup;
    KisBrushEngineSelector* m_brushEngineSelector;
    KisView2* m_view;
    QPushButton* m_paletteButton;

    QMap<KoID, KisPaintOpSettingsWidget*> m_paintopOptionWidgets;
    KisPaintOpPresetSP m_activePreset;
    const KoCompositeOp* m_compositeOp;
    KisNodeSP m_previousNode;

    typedef QHash<KoInputDevice, KoID> InputDevicePaintopMap;
    InputDevicePaintopMap m_currentID;

    typedef QHash<QString, KisPaintOpPresetSP> PresetMap;
    typedef QHash<KoInputDevice, PresetMap > InputDevicePresetsMap;
    InputDevicePresetsMap m_inputDevicePresets;

    QHash<KoInputDevice, bool> m_inputDeviceEraseModes;
    QHash<KoInputDevice, QString> m_inputDeviceCompositeModes;
    bool m_eraserUsed;
};



#endif //KIS_PAINTOP_BOX_H_
