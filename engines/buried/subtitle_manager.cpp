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

#include "buried/buried.h"
#include "buried/graphics.h"
#include "buried/subtitle_manager.h"

#include "common/config-manager.h"
#include "common/file.h"
#include "common/formats/json.h"
#include "graphics/font.h"
#include "graphics/surface.h"

namespace Buried {

SubtitleManager::SubtitleManager(BuriedEngine *vm) : _vm(vm), _font(nullptr), _fontBold(nullptr), _fontSize(0) {
	updateFont();
}

SubtitleManager::~SubtitleManager() {
	delete _font;
	delete _fontBold;
}

void SubtitleManager::updateFont() {
	int requestedSize = kDefaultSubtitleFontSize;
	if (ConfMan.hasKey("subtitle_font_size")) {
		requestedSize = ConfMan.getInt("subtitle_font_size");
	} else if (ConfMan.hasKey("talkspeed")) {
		int talkspeed = ConfMan.getInt("talkspeed");
		requestedSize = 12 + (talkspeed * 8 / 255); // Maps talkspeed 0..255 to font sizes 12px..20px
	}

	if (requestedSize < 10) requestedSize = 10;
	if (requestedSize > 24) requestedSize = 24;

	if (!_font || _fontSize != requestedSize) {
		delete _font;
		delete _fontBold;
		_fontSize = requestedSize;
		_font = _vm->_gfx->createFont(_fontSize, /* bold= */ false);
		_fontBold = _vm->_gfx->createFont(_fontSize, /* bold= */ true);
	}
}

Common::String SubtitleManager::getSubtitlePath(const Common::String &mediaId) const {
	Common::String cleanId = mediaId;
	if (cleanId.contains(".")) {
		size_t dotPos = cleanId.findLastOf('.');
		cleanId = cleanId.substr(0, dotPos);
	}
	return "subtitles/" + cleanId + ".json";
}

bool SubtitleManager::loadSubtitles(const Common::String &mediaId) {
	Common::String cleanId = mediaId;
	if (cleanId.contains(".")) {
		size_t dotPos = cleanId.findLastOf('.');
		cleanId = cleanId.substr(0, dotPos);
	}
	cleanId.toUppercase();

	if (_loadedTracks.contains(cleanId))
		return true;

	Common::String jsonPath = getSubtitlePath(cleanId);
	Common::File file;
	if (!file.open(Common::Path(jsonPath))) {
		// Also try without folder prefix or lowercase
		if (!file.open(Common::Path(cleanId + ".json"))) {
			// Cache as an empty track so we don't check disk again
			SubtitleTrack emptyTrack;
			emptyTrack.mediaId = cleanId;
			_loadedTracks[cleanId] = emptyTrack;
			return true;
		}
	}

	uint32 size = file.size();
	if (size == 0) {
		SubtitleTrack emptyTrack;
		emptyTrack.mediaId = cleanId;
		_loadedTracks[cleanId] = emptyTrack;
		return true;
	}

	char *buffer = new char[size + 1];
	file.read(buffer, size);
	buffer[size] = '\0';

	Common::JSONValue *root = Common::JSON::parse(buffer);
	delete[] buffer;

	if (!root || !root->isObject()) {
		warning("[SubtitleManager] Failed to parse JSON object in subtitle file for media ID: %s", cleanId.c_str());
		delete root;
		// Cache as empty to prevent infinite parse retries
		SubtitleTrack emptyTrack;
		emptyTrack.mediaId = cleanId;
		_loadedTracks[cleanId] = emptyTrack;
		return true;
	}

	SubtitleTrack track;
	track.mediaId = cleanId;

	Common::JSONValue *subsValue = root->child("subtitles");
	if (subsValue && subsValue->isArray()) {
		const Common::JSONArray &array = subsValue->asArray();
		for (size_t i = 0; i < array.size(); ++i) {
			Common::JSONValue *item = array[i];
			if (!item || !item->isObject())
				continue;

			SubtitleEntry entry;
			if (item->hasChild("start_ms"))
				entry.startMs = (uint32)item->child("start_ms")->asIntegerNumber();
			else
				entry.startMs = 0;

			if (item->hasChild("end_ms"))
				entry.endMs = (uint32)item->child("end_ms")->asIntegerNumber();
			else
				entry.endMs = 0;

			if (item->hasChild("speaker"))
				entry.speaker = item->child("speaker")->asString();

			if (item->hasChild("text"))
				entry.text = item->child("text")->asString();

			track.entries.push_back(entry);
		}
	}

	delete root;

	debug(5, "[SubtitleManager] SUCCESS: Loaded subtitle file for '%s' (%u entries)", cleanId.c_str(), (uint)track.entries.size());
	_loadedTracks[cleanId] = track;
	return true;
}

const SubtitleEntry *SubtitleManager::getSubtitleForTime(const Common::String &mediaId, uint32 currentMs) {
	Common::String cleanId = mediaId;
	if (cleanId.contains(".")) {
		size_t dotPos = cleanId.findLastOf('.');
		cleanId = cleanId.substr(0, dotPos);
	}
	cleanId.toUppercase();

	if (!_loadedTracks.contains(cleanId)) {
		if (!loadSubtitles(cleanId))
			return nullptr;
	}

	const SubtitleTrack &track = _loadedTracks[cleanId];
	for (size_t i = 0; i < track.entries.size(); ++i) {
		const SubtitleEntry &entry = track.entries[i];
		if (currentMs >= entry.startMs && currentMs <= entry.endMs) {
			return &entry;
		}
	}

	return nullptr;
}

void SubtitleManager::renderSubtitle(Graphics::Surface *destSurface, const SubtitleEntry &entry) {
	updateFont();
	int fontHeight = _font ? _font->getFontHeight() : 14;
	int calculatedBoxHeight = (fontHeight * 2) + 8;
	Common::Rect defaultBox(kSubtitleBoxX, kSubtitleViewportBottom - calculatedBoxHeight, kSubtitleBoxX + kSubtitleBoxWidth, kSubtitleViewportBottom);
	renderSubtitle(destSurface, defaultBox, entry);
}

void SubtitleManager::renderSubtitle(Graphics::Surface *destSurface, const Common::Rect &boxRect, const SubtitleEntry &entry) {
	updateFont();
	if (!destSurface || !_font || entry.text.empty())
		return;

	if (_lastLoggedText != entry.text) {
		_lastLoggedText = entry.text;
		warning("[Subtitle Text] %s: %s", entry.speaker.empty() ? "Dialogue" : entry.speaker.c_str(), entry.text.c_str());
	}

	if (boxRect.width() <= 20 || boxRect.height() <= 10)
		return;

	int fontHeight = _font->getFontHeight();

	// Dark semi-transparent scrim background color
	float alpha = kSubtitleBoxOpacity;
	if (alpha < 0.0f) alpha = 0.0f;
	if (alpha > 1.0f) alpha = 1.0f;
	float invAlpha = 1.0f - alpha;

	byte targetR = 38;
	byte targetG = 12;
	byte targetB = 12;
	uint32 scrimColor = _vm->_gfx->getColor(targetR, targetG, targetB);
	
	// Glowing orange/copper border color matching HUD
	uint32 borderColor = _vm->_gfx->getColor(237, 109, 66);

	// Draw scrim background box and 1-pixel top border
	for (int y = boxRect.top; y < boxRect.bottom; ++y) {
		if (y < 0 || y >= destSurface->h) continue;
		
		bool isBorderLine = (y == boxRect.top);

		for (int x = boxRect.left; x < boxRect.right; ++x) {
			if (x < 0 || x >= destSurface->w) continue;
			
			if (isBorderLine) {
				if (destSurface->format.bytesPerPixel == 2) {
					uint16 *ptr = (uint16 *)destSurface->getBasePtr(x, y);
					*ptr = (uint16)borderColor;
				} else if (destSurface->format.bytesPerPixel == 4) {
					uint32 *ptr = (uint32 *)destSurface->getBasePtr(x, y);
					*ptr = borderColor;
				} else {
					byte *ptr = (byte *)destSurface->getBasePtr(x, y);
					*ptr = (byte)borderColor;
				}
			} else {
				if (destSurface->format.bytesPerPixel == 2) {
					uint16 *ptr = (uint16 *)destSurface->getBasePtr(x, y);
					byte r, g, b;
					destSurface->format.colorToRGB(*ptr, r, g, b);
					byte blendedR = (byte)(r * invAlpha + targetR * alpha);
					byte blendedG = (byte)(g * invAlpha + targetG * alpha);
					byte blendedB = (byte)(b * invAlpha + targetB * alpha);
					*ptr = destSurface->format.RGBToColor(blendedR, blendedG, blendedB);
				} else if (destSurface->format.bytesPerPixel == 4) {
					uint32 *ptr = (uint32 *)destSurface->getBasePtr(x, y);
					byte r, g, b;
					destSurface->format.colorToRGB(*ptr, r, g, b);
					byte blendedR = (byte)(r * invAlpha + targetR * alpha);
					byte blendedG = (byte)(g * invAlpha + targetG * alpha);
					byte blendedB = (byte)(b * invAlpha + targetB * alpha);
					*ptr = destSurface->format.RGBToColor(blendedR, blendedG, blendedB);
				} else {
					byte *ptr = (byte *)destSurface->getBasePtr(x, y);
					*ptr = (byte)scrimColor;
				}
			}
		}
	}

	// Glowing HUD Neon Orange/Amber color for speaker name, Cream-Amber color for dialogue text
	uint32 orangeColor = _vm->_gfx->getColor(237, 109, 66);
	uint32 dialogueColor = _vm->_gfx->getColor(255, 230, 180);

	const int kPadX = 8;  // horizontal inner padding
	const int kPadY = 4;  // vertical inner padding
	const int innerW  = boxRect.width() - kPadX * 2;
	const int innerX  = boxRect.left + kPadX;
	int curY = boxRect.top + kPadY;

	// Build speaker prefix (e.g. "Arthur: ")
	Common::String speakerPrefix = entry.speaker.empty() ? "" : (entry.speaker + ": ");
	int speakerW = speakerPrefix.empty() ? 0 : _fontBold->getStringWidth(speakerPrefix);

	// -----------------------------------------------------------------------
	// Word-wrap the dialogue text across up to 2 lines.
	// Line 1 has a narrower available width because the speaker prefix
	// occupies the left side of it.
	// -----------------------------------------------------------------------
	Common::Array<Common::String> lines;

	Common::String remaining = entry.text;
	bool firstLine = true;

	while (!remaining.empty() && lines.size() < 2) {
		int availW = firstLine ? (innerW - speakerW) : innerW;
		if (availW <= 0) {
			// Speaker prefix alone fills the row; push text to next line.
			firstLine = false;
			availW = innerW;
		}

		// Find the maximum prefix of 'remaining' that fits in availW.
		// We scan word by word and commit the longest fitting run.
		Common::String fittingLine;
		Common::String rest = remaining;

		while (!rest.empty()) {
			// Pull the next word (up to the next space or end of string)
			uint nextSpace = 0;
			while (nextSpace < rest.size() && rest[nextSpace] != ' ')
				++nextSpace;
			Common::String word = rest.substr(0, nextSpace);

			// Candidate: what would the line look like if we add this word?
			Common::String candidate = fittingLine.empty() ? word : (fittingLine + " " + word);

			if (_font->getStringWidth(candidate) <= availW) {
				fittingLine = candidate;
				rest = (nextSpace < rest.size()) ? rest.substr(nextSpace + 1) : "";
			} else {
				// Word doesn't fit.
				if (fittingLine.empty()) {
					// Single word is already too long — force it onto the line anyway.
					fittingLine = word;
					rest = (nextSpace < rest.size()) ? rest.substr(nextSpace + 1) : "";
				}
				break;
			}
			}


		lines.push_back(fittingLine);
		remaining = rest;
		firstLine = false;
	}

	// -----------------------------------------------------------------------
	// Draw line 1: speaker prefix (bold orange) + first line of dialogue (cream-amber)
	// -----------------------------------------------------------------------
	if (!lines.empty()) {
		int drawX = innerX;
		if (!speakerPrefix.empty()) {
			_fontBold->drawString(destSurface, speakerPrefix, drawX, curY, innerW, orangeColor, Graphics::kTextAlignLeft);
			drawX += speakerW;
		}
		int line1AvailW = boxRect.right - kPadX - drawX;
		if (line1AvailW > 0)
			_font->drawString(destSurface, lines[0], drawX, curY, line1AvailW, dialogueColor, Graphics::kTextAlignLeft);
		curY += fontHeight;
	}

	// -----------------------------------------------------------------------
	// Draw line 2 (if present): dialogue continues (cream-amber), no speaker prefix
	// -----------------------------------------------------------------------
	if (lines.size() >= 2 && curY + fontHeight <= boxRect.bottom) {
		_font->drawString(destSurface, lines[1], innerX, curY, innerW, dialogueColor, Graphics::kTextAlignLeft);
	}
}

int SubtitleManager::getFontHeight() {
	updateFont();
	return _font ? _font->getFontHeight() : 14;
}

} // End of namespace Buried

