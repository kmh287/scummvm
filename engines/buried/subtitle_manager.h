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

#ifndef BURIED_SUBTITLE_MANAGER_H
#define BURIED_SUBTITLE_MANAGER_H

#include "common/array.h"
#include "common/hashmap.h"
#include "common/hash-str.h"
#include "common/str.h"
#include "common/rect.h"

namespace Graphics {
class Font;
struct Surface;
}

namespace Buried {

class BuriedEngine;
class Window;

struct SubtitleEntry {
	uint32 startMs;
	uint32 endMs;
	Common::String speaker;
	Common::String text;
};

struct SubtitleTrack {
	Common::String mediaId;
	Common::Array<SubtitleEntry> entries;
};

// Subtitle Box Layout Constants
// Controls position, width, and height of the subtitle overlay box.
// Extended horizontal placement spans across the suit viewport bezel lips (X=58..502, Top Y=317).
static const int kSubtitleBoxX = 48;          // X position (expanded to cover left bezel lip)
static const int kSubtitleViewportTop = 319;  // Y position flush with bottom of viewport
static const int kSubtitleBoxWidth = 460;     // Width of subtitle box (expanded to cover right bezel lip)
static const float kSubtitleBoxOpacity = 0.75f; // Opacity: 0.0f (fully transparent) to 1.0f (fully opaque)
static const int kDefaultSubtitleFontSize = 14; // Default font pixel height

class SubtitleManager {
public:
	SubtitleManager(BuriedEngine *vm);
	~SubtitleManager();

	bool areSubtitlesEnabled() const;
	void invalidateSubtitles(Window *targetWindow = nullptr);

	bool loadSubtitlesDat();
	const SubtitleEntry *getSubtitleForTime(const Common::String &mediaId, uint32 currentMs);

	void renderSubtitle(Graphics::Surface *destSurface, const SubtitleEntry &entry);
	void renderSubtitle(Graphics::Surface *destSurface, const Common::Rect &boxRect, const SubtitleEntry &entry);

	bool renderSubtitleForMedia(Graphics::Surface *destSurface, const Common::String &mediaId, uint32 currentMs);
	bool renderSubtitleForMedia(Graphics::Surface *destSurface, const Common::Rect &boxRect, const Common::String &mediaId, uint32 currentMs);
	
	int getFontHeight();

private:
	BuriedEngine *_vm;
	Common::HashMap<Common::String, SubtitleTrack> _loadedTracks;
	Graphics::Font *_font;
	Graphics::Font *_fontBold;
	int _fontSize;

	void updateFont();
	Common::Array<Common::String> wrapText(const Common::String &text, int line1AvailW, int line2AvailW);
};

} // End of namespace Buried

#endif
