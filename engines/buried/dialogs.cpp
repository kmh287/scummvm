/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "common/config-manager.h"
#include "common/translation.h"

#include "gui/ThemeEval.h"
#include "gui/widget.h"
#include "gui/widgets/popup.h"

#include "buried/dialogs.h"
#include "buried/subtitle_manager.h"

namespace Buried {

// Font size preset values (pixel height)
enum {
	kFontSizeSmall   = 12,
	kFontSizeMedium  = 14,
	kFontSizeLarge   = 18,
	kFontSizeXLarge  = 22
};

BuriedOptionsWidget::BuriedOptionsWidget(GUI::GuiObject *boss, const Common::String &name, const Common::String &domain)
	: OptionsContainerWidget(boss, name, "BuriedGameOptionsDialog", domain) {

	// "Subtitle font size:" label
	_fontSizeDesc = new GUI::StaticTextWidget(widgetsBoss(), "BuriedGameOptionsDialog.FontSizeDesc", _("Subtitle font size:"));
	_fontSizeDesc->setAlign(Graphics::kTextAlignRight);

	// Dropdown populated with named presets
	_fontSizePopUp = new GUI::PopUpWidget(widgetsBoss(), "BuriedGameOptionsDialog.FontSize");
	_fontSizePopUp->appendEntry(_("Small"),         kFontSizeSmall);
	_fontSizePopUp->appendEntry(_("Medium"),        kFontSizeMedium);
	_fontSizePopUp->appendEntry(_("Large"),         kFontSizeLarge);
	_fontSizePopUp->appendEntry(_("Extra Large"),   kFontSizeXLarge);
}

void BuriedOptionsWidget::defineLayout(GUI::ThemeEval &layouts, const Common::String &layoutName, const Common::String &overlayedLayout) const {
	layouts.addDialog(layoutName, overlayedLayout);
	layouts.addLayout(GUI::ThemeLayout::kLayoutVertical).addPadding(0, 0, 0, 0);

	// Single row: label + popup
	layouts.addLayout(GUI::ThemeLayout::kLayoutHorizontal).addPadding(0, 0, 2, 0);
	layouts.addWidget("FontSizeDesc", "OptionsLabel");
	layouts.addWidget("FontSize", "PopUp");
	layouts.closeLayout(); // close horizontal row

	layouts.closeLayout(); // close vertical
	layouts.closeDialog();
}

// Reads the subtitle font size setting from the active ScummVM configuration domain ("subtitle_font_size").
static int getSavedSubtitleFontSize(const Common::String &domain) {
	if (ConfMan.hasKey("subtitle_font_size", domain))
		return ConfMan.getInt("subtitle_font_size", domain);
	return kDefaultSubtitleFontSize;
}

// Writes the subtitle font size setting to the active ScummVM configuration domain ("subtitle_font_size").
static void saveSubtitleFontSize(const Common::String &domain, int fontSize) {
	ConfMan.setInt("subtitle_font_size", fontSize, domain);
}

void BuriedOptionsWidget::load() {
	int fontSize = getSavedSubtitleFontSize(_domain);
	_fontSizePopUp->setSelectedTag(fontSize);

	// If the saved value doesn't match any preset, fall back to Medium
	if (_fontSizePopUp->getSelectedTag() == (uint32)-1)
		_fontSizePopUp->setSelectedTag(kFontSizeMedium);
}

bool BuriedOptionsWidget::save() {
	uint32 selectedTag = _fontSizePopUp->getSelectedTag();
	if (selectedTag == (uint32)-1)
		selectedTag = kDefaultSubtitleFontSize;

	saveSubtitleFontSize(_domain, (int)selectedTag);
	return true;
}

} // End of namespace Buried
