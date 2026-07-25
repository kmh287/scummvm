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
// Default placement is anchored to the bottom of the jumpsuit viewport (Y=317).
static const int kSubtitleBoxX = 64;          // X position (left edge)
static const int kSubtitleViewportBottom = 317; // Y position of the bottom of the viewport
static const int kSubtitleBoxWidth = 432;     // Width of subtitle box
static const float kSubtitleBoxOpacity = 0.75f; // Opacity: 0.0f (fully transparent) to 1.0f (fully opaque)
static const int kDefaultSubtitleFontSize = 14; // Default font pixel height

class SubtitleManager {
public:
	SubtitleManager(BuriedEngine *vm);
	~SubtitleManager();

	bool loadSubtitles(const Common::String &mediaId);
	const SubtitleEntry *getSubtitleForTime(const Common::String &mediaId, uint32 currentMs);

	void renderSubtitle(Graphics::Surface *destSurface, const SubtitleEntry &entry);
	void renderSubtitle(Graphics::Surface *destSurface, const Common::Rect &boxRect, const SubtitleEntry &entry);
	
	int getFontHeight();

private:
	BuriedEngine *_vm;
	Common::HashMap<Common::String, SubtitleTrack> _loadedTracks;
	Graphics::Font *_font;
	Graphics::Font *_fontBold;
	int _fontSize;

	Common::String getSubtitlePath(const Common::String &mediaId) const;
	void updateFont();
	mutable Common::String _lastLoggedText;
};

} // End of namespace Buried

#endif
