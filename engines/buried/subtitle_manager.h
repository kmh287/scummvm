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

// A single subtitle entry or "card".
struct SubtitleEntry {
	// The start time of this subtitle in milliseconds since the beginning of the file.
	uint32 startMs;
	// The end time of this subtitle in milliseconds since the beginning of the file.
	uint32 endMs;
	// The character who speaks the subtitled line of dialog.
	Common::String speaker;
	// The text spoken for the subtitled line of dialog.
	Common::String text;
};

// A subtitle track for an audio or video file, containing multiple entries or "cards".
struct SubtitleTrack {
	// The identifier for the audio or video file to which this track is associated.
	Common::String mediaId;
	// The subtitle entries, sorted by their start times.
	Common::Array<SubtitleEntry> entries;
};

// Subtitle Box Layout Constants
// Controls position, width, and height of the subtitle overlay box.
// The subtitle box is designed to appear below the jumpsuit's viewport, spanning from one end of the window to the other
// and including the viewport bezels.
//
// This design deliberately obstructs the display of the current date while subtitles are showing. This was deemed preferable
// to showing the subtitles on top of the viewport and obstructing the already-small jumpsuit viewport.
//
// The X coordinate of the subtitle box's top-left edge.
static constexpr int kSubtitleBoxX = 46;
// The Y coordinate of the subtitle box's top-left edge.
static constexpr int kSubtitleViewportTop = 319;
// The width of the subtitle box.
static constexpr int kSubtitleBoxWidth = 462;
// The opacity of the subtitle box. 0f would be fully transparent. 1f would be fully opaque.
static constexpr float kSubtitleBoxOpacity = 0.75f;
// Additional vertical padding of the interior of the subtitle box. This is added additionally to space reserved
// for font ascenders, descenders, and the angled chamfer section at the bottom of the subtitle box.
static constexpr int  kSubtitlePadY = 2;

class SubtitleManager {
public:
	SubtitleManager(BuriedEngine *vm);
	~SubtitleManager();

	bool areSubtitlesEnabled() const;
	void invalidateSubtitles(Window *targetWindow = nullptr);
	void updateSubtitles(Window *targetWindow = nullptr);

	bool loadSubtitlesDat();
	const SubtitleEntry *getSubtitleForTime(const Common::String &mediaId, uint32 currentMs);

	void renderSubtitle(Graphics::Surface *destSurface, const SubtitleEntry &entry);
	void renderSubtitle(Graphics::Surface *destSurface, const Common::Rect &boxRect, const SubtitleEntry &entry);

	bool renderSubtitleForMedia(Graphics::Surface *destSurface, const Common::String &mediaId, uint32 currentMs);
	bool renderSubtitleForMedia(Graphics::Surface *destSurface, const Common::Rect &boxRect, const Common::String &mediaId, uint32 currentMs);
	
	int getFontHeight();
	int getBoxHeight();
	Common::Rect getDefaultBoxBounds();
	Common::Rect calculateBoxBoundsForVideo(const Common::Rect &videoFrameRect);

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
