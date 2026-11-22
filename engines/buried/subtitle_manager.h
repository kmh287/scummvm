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

#include "common/path.h"
#include "common/ptr.h"
#include "common/scummsys.h"
#include "common/ustr.h"

namespace Graphics {
struct Surface;
}

namespace Buried {

class BuriedEngine;

class SubtitleManager {
public:
	// Speaker and dialogue text for a single active subtitle card.
	struct Subtitle {
		// The character who speaks the subtitled line of dialog (decoded from UTF-8).
		Common::U32String speaker;
		// The text spoken for the subtitled line of dialog (decoded from UTF-8).
		Common::U32String text;
	};

	explicit SubtitleManager(BuriedEngine *vm);
	~SubtitleManager();

	// Returns true if subtitles are enabled in configuration and false otherwise.
	static bool areSubtitlesEnabled();

	// Returns true if a subtitle track exists for the specified media file and false otherwise.
	bool hasSubtitleTrack(const Common::Path &filePath) const;

	// Returns the Subtitle for the specified media file and playback time, or nullptr if none matches.
	const Subtitle *getSubtitleForTime(const Common::Path &filePath, uint32 currentMs) const;

	// Returns true if sound is unpaused and at least one currently playing audio channel has a
	// subtitle track; returns false if no audio is playing, sound is paused, or playing audio
	// has no subtitle track.
	bool isSubtitledAudioPlaying() const;

	// Resolves the highest-priority subtitled audio channel currently playing and renders its
	// active subtitle card (if any at the current playback time) onto destSurface.
	bool renderActiveAudio(Graphics::Surface *destSurface) const;

private:
	struct TrackMap;
	BuriedEngine *_vm;
	const Common::ScopedPtr<const TrackMap> _trackMap;
};

} // End of namespace Buried

#endif
