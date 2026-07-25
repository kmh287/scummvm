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

#ifndef BURIED_DIALOGS_H
#define BURIED_DIALOGS_H

#include "gui/widget.h"

namespace GUI {
class PopUpWidget;
class StaticTextWidget;
}

namespace Buried {

/**
 * Engine-specific options widget shown in the Audio tab of the ScummVM
 * in-game options dialog (Ctrl+F5 → Options → Audio).
 *
 * Currently exposes:
 *   - Subtitle Font Size  (Small = 12, Medium = 14, Large = 18, X-Large = 22)
 *     stored as "subtitle_font_size" in scummvm.ini.
 */
class BuriedOptionsWidget : public GUI::OptionsContainerWidget {
public:
	BuriedOptionsWidget(GUI::GuiObject *boss, const Common::String &name, const Common::String &domain);

	// OptionsContainerWidget interface
	void defineLayout(GUI::ThemeEval &layouts, const Common::String &layoutName, const Common::String &overlayedLayout) const override;
	void load() override;
	bool save() override;

private:
	GUI::PopUpWidget      *_fontSizePopUp;
	GUI::StaticTextWidget *_fontSizeDesc;
};

} // End of namespace Buried

#endif // BURIED_DIALOGS_H
