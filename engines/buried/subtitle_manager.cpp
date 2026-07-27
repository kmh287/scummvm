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
#include "buried/window.h"

#include "common/config-manager.h"
#include "common/file.h"
#include "common/textconsole.h"
#include "graphics/font.h"
#include "graphics/surface.h"

namespace Buried {

SubtitleManager::SubtitleManager(BuriedEngine *vm) : _vm(vm), _font(nullptr), _fontBold(nullptr), _fontSize(0) {
	updateFont();
	loadSubtitlesDat();
}

SubtitleManager::~SubtitleManager() {
	delete _font;
	delete _fontBold;
}

bool SubtitleManager::areSubtitlesEnabled() const {
	return ConfMan.getBool("subtitles");
}

void SubtitleManager::invalidateSubtitles(Window *targetWindow) {
	if (!areSubtitlesEnabled()) {
		return;
	}

	// Invalidate target window (or main window) to trigger onPaint() repaints
	if (targetWindow) {
		targetWindow->invalidateWindow(false);
	} else if (_vm->_mainWindow) {
		_vm->_mainWindow->invalidateWindow(false);
	}

	// Mark the subtitle box region dirty on the GraphicsManager screen renderer.
	// This is required because the subtitle overlay sits below the jumpsuit visor window bounds,
	// so we explicitly add its screen bounding rect to the hardware dirty rect list for display blitting.
	_vm->_gfx->invalidateRect(getDefaultBoxBounds(), false);
}

void SubtitleManager::updateSubtitles(Window *targetWindow) {
	if (!areSubtitlesEnabled()) {
		return;
	}

	Window *windowToPaint = targetWindow ? targetWindow : _vm->_mainWindow;
	if (!windowToPaint) {
		return;
	}

	invalidateSubtitles(windowToPaint);
	windowToPaint->onPaint();
}

void SubtitleManager::updateFont() {
	int requestedSize = kDefaultSubtitleFontSize;
	if (ConfMan.hasKey("subtitle_font_size")) {
		requestedSize = ConfMan.getInt("subtitle_font_size");
	}

	// Clamp font pixel height between 10px and 24px to ensure text remains legible while fitting inside the subtitle box
	if (requestedSize < 10) {
		requestedSize = 10;
	}
	if (requestedSize > 24) {
		requestedSize = 24;
	}

	if (!_font || _fontSize != requestedSize) {
		delete _font;
		delete _fontBold;
		_fontSize = requestedSize;
		_font = _vm->_gfx->createFont(_fontSize, /* bold= */ false);
		_fontBold = _vm->_gfx->createFont(_fontSize, /* bold= */ true);
	}
}

static inline byte blendColorComponent(byte srcComp, byte targetComp, float alpha, float invAlpha) {
	return (byte)(srcComp * invAlpha + targetComp * alpha);
}

static inline void drawPixel(Graphics::Surface *destSurface, int x, int y, uint32 color) {
	if (destSurface->format.bytesPerPixel == 2) {
		uint16 *ptr = (uint16 *)destSurface->getBasePtr(x, y);
		*ptr = (uint16)color;
	} else if (destSurface->format.bytesPerPixel == 4) {
		uint32 *ptr = (uint32 *)destSurface->getBasePtr(x, y);
		*ptr = color;
	} else {
		byte *ptr = (byte *)destSurface->getBasePtr(x, y);
		*ptr = (byte)color;
	}
}

// ---------------------------------------------------------------------------
// Loads binary subtitle package (buried_subtitles.dat).
//
// File Format Specification (Big-Endian):
// 1. Magic Signature (4 bytes): 'BURS' (0x42555253)
// 2. File Version (uint16): 1
// 3. Track Count (uint16): N tracks
// 4. TOC Table (N x 22 bytes):
//    - Media ID (16 bytes): Fixed ASCII string (null-padded)
//    - Payload Offset (uint32): Byte offset to track data
//    - Card Count (uint16): M subtitle cards in track
// 5. Track Payloads:
//    - Start Time (uint32): Start time in milliseconds
//    - End Time (uint32): End time in milliseconds
//    - Speaker Length (uint16): Speaker string length
//    - Speaker String: UTF-8 speaker name
//    - Text Length (uint16): Dialogue string length
//    - Text String: UTF-8 dialogue text
// ---------------------------------------------------------------------------
bool SubtitleManager::loadSubtitlesDat() {
	Common::File file;
	if (!file.open(Common::Path("buried_subtitles.dat"))) {
		warning("[SubtitleManager] Could not open buried_subtitles.dat");
		return false;
	}

	uint32 magic = file.readUint32BE();
	if (magic != MKTAG('B', 'U', 'R', 'S')) {
		warning("[SubtitleManager] Invalid magic in subtitles.dat: 0x%08X", magic);
		return false;
	}

	uint16 version = file.readUint16BE();
	if (version != 1) {
		warning("[SubtitleManager] Unsupported subtitles.dat version: %d", version);
		return false;
	}

	uint16 numTracks = file.readUint16BE();

	struct TocEntry {
		Common::String mediaId;
		uint32 offset;
		uint16 cardCount;
	};

	Common::Array<TocEntry> toc;
	toc.reserve(numTracks);

	for (uint16 i = 0; i < numTracks; ++i) {
		char mediaIdBuf[17];
		file.read(mediaIdBuf, 16);
		mediaIdBuf[16] = '\0';

		TocEntry entry;
		entry.mediaId = mediaIdBuf;
		entry.offset = file.readUint32BE();
		entry.cardCount = file.readUint16BE();
		toc.push_back(entry);
	}

	// Load track payload data
	for (const auto &tocEntry : toc) {
		if (file.seek(tocEntry.offset, SEEK_SET)) {
			SubtitleTrack track;
			track.mediaId = tocEntry.mediaId;

			for (uint16 c = 0; c < tocEntry.cardCount; ++c) {
				SubtitleEntry card;
				card.startMs = file.readUint32BE();
				card.endMs = file.readUint32BE();

				uint16 spkLen = file.readUint16BE();
				if (spkLen > 0) {
					card.speaker = file.readString(0, spkLen);
				}

				uint16 txtLen = file.readUint16BE();
				if (txtLen > 0) {
					card.text = file.readString(0, txtLen);
				}

				track.entries.push_back(card);
			}

			_loadedTracks[track.mediaId] = track;
		}
	}

	debug(1, "[SubtitleManager] Successfully loaded %u subtitle tracks from subtitles.dat", (uint)_loadedTracks.size());
	return true;
}

static Common::String sanitizeMediaId(const Common::String &mediaId) {
	Common::String sanitized = mediaId;
	if (sanitized.contains(".")) {
		size_t dotPos = sanitized.findLastOf('.');
		sanitized = sanitized.substr(0, dotPos);
	}
	sanitized.toUppercase();
	return sanitized;
}

const SubtitleEntry *SubtitleManager::getSubtitleForTime(const Common::String &mediaId, uint32 currentMs) {
	Common::String sanitized = sanitizeMediaId(mediaId);

	if (!_loadedTracks.contains(sanitized)) {
		return nullptr;
	}

	// Linear search is O(M) over a small array (each track contains at most <= 20 subtitle entries)
	const SubtitleTrack &track = _loadedTracks[sanitized];
	for (size_t i = 0; i < track.entries.size(); ++i) {
		const SubtitleEntry &entry = track.entries[i];
		if (currentMs >= entry.startMs && currentMs <= entry.endMs) {
			return &entry;
		}
	}

	return nullptr;
}

bool SubtitleManager::renderSubtitleForMedia(Graphics::Surface *destSurface, const Common::String &mediaId, uint32 currentMs) {
	if (!areSubtitlesEnabled()) {
		return false;
	}

	const SubtitleEntry *sub = getSubtitleForTime(mediaId, currentMs);
	if (!sub) {
		return false;
	}
	renderSubtitle(destSurface, *sub);
	return true;
}

bool SubtitleManager::renderSubtitleForMedia(Graphics::Surface *destSurface, const Common::Rect &boxRect, const Common::String &mediaId, uint32 currentMs) {
	if (!areSubtitlesEnabled()) {
		return false;
	}

	const SubtitleEntry *sub = getSubtitleForTime(mediaId, currentMs);
	if (!sub) {
		return false;
	}
	renderSubtitle(destSurface, boxRect, *sub);
	return true;
}

void SubtitleManager::renderSubtitle(Graphics::Surface *destSurface, const SubtitleEntry &entry) {
	renderSubtitle(destSurface, getDefaultBoxBounds(), entry);
}

void SubtitleManager::renderSubtitle(Graphics::Surface *destSurface, const Common::Rect &boxRect, const SubtitleEntry &entry) {
	updateFont();
	if (!destSurface || !_font || entry.text.empty()) {
		return;
	}

	int fontHeight = _font->getFontHeight();

	// Dark semi-transparent scrim background color
	float alpha = kSubtitleBoxOpacity;
	if (alpha < 0.0f) { alpha = 0.0f; }
	if (alpha > 1.0f) { alpha = 1.0f; }

	// -----------------------------------------------------------------------
	// Render subtitle box scrim & bottom chamfered border.
	//
	// 1. Chamfer Cutouts: For the bottom 6 rows (distFromBottom < 6), inset the left and right
	//    bounds by (6 - distFromBottom) * 2 pixels. This creates a 30-degree bevel angle
	//    (2:1 horizontal-to-vertical slope ratio) matching the suit HUD.
	// 2. CRT Scanline Interlacing: Alternate row opacity (1.15x alpha on even rows,
	//    0.85x alpha on odd rows) to simulate an interlaced glass CRT monitor.
	// -----------------------------------------------------------------------
	for (int y = boxRect.top; y < boxRect.bottom; ++y) {
		if (y < 0 || y >= destSurface->h) { continue; }

		int dy = y - boxRect.top;
		int distFromBottom = (boxRect.bottom - 1) - y;
		int inset = 0;
		if (distFromBottom < 6) {
			inset = (6 - distFromBottom) * 2; // 30-degree chamfer slope (2:1 horizontal-to-vertical ratio)
		}

		int startX = boxRect.left + inset;
		int endX   = boxRect.right - inset;

		// CRT Scanline Raster: alternate line alpha for subtle interlaced CRT glass effect
		float curAlpha = (dy % 2 == 0) ? (alpha * 1.15f) : (alpha * 0.85f);
		if (curAlpha > 0.95f) { curAlpha = 0.95f; }
		if (curAlpha < 0.20f) { curAlpha = 0.20f; }
		float curInvAlpha = 1.0f - curAlpha;

		for (int x = startX; x < endX; ++x) {
			if (x < 0 || x >= destSurface->w) { continue; }

			// Border pixels: top line (muted copper-orange), bottom line (bright orange), side/diagonal edges (subtle dark red)
			bool isTopBorder    = (y == boxRect.top);
			bool isBottomBorder = (y == boxRect.bottom - 1);
			bool isSideBorder   = (x == startX || x == endX - 1);
			bool isBorderPixel  = isTopBorder || isBottomBorder || isSideBorder;

			if (isBorderPixel) {
				uint32 curBorderColor = isTopBorder ? _vm->_gfx->getColor(140, 60, 35) : (isBottomBorder ? _vm->_gfx->getColor(237, 109, 66) : _vm->_gfx->getColor(105, 36, 28));
				drawPixel(destSurface, x, y, curBorderColor);
			} else {
				if (destSurface->format.bytesPerPixel == 2) {
					uint16 *ptr = (uint16 *)destSurface->getBasePtr(x, y);
					byte r, g, b;
					destSurface->format.colorToRGB(*ptr, r, g, b);
					byte blendedR = blendColorComponent(r, 38, curAlpha, curInvAlpha);
					byte blendedG = blendColorComponent(g, 12, curAlpha, curInvAlpha);
					byte blendedB = blendColorComponent(b, 12, curAlpha, curInvAlpha);
					*ptr = destSurface->format.RGBToColor(blendedR, blendedG, blendedB);
				} else if (destSurface->format.bytesPerPixel == 4) {
					uint32 *ptr = (uint32 *)destSurface->getBasePtr(x, y);
					byte r, g, b;
					destSurface->format.colorToRGB(*ptr, r, g, b);
					byte blendedR = blendColorComponent(r, 38, curAlpha, curInvAlpha);
					byte blendedG = blendColorComponent(g, 12, curAlpha, curInvAlpha);
					byte blendedB = blendColorComponent(b, 12, curAlpha, curInvAlpha);
					*ptr = destSurface->format.RGBToColor(blendedR, blendedG, blendedB);
				} else {
					drawPixel(destSurface, x, y, _vm->_gfx->getColor(38, 12, 12));
				}
			}
		}
	}

	// Glowing HUD Neon Orange/Amber color for speaker name, Cream-Amber color for dialogue text
	uint32 orangeColor = _vm->_gfx->getColor(237, 109, 66);
	uint32 dialogueColor = _vm->_gfx->getColor(255, 230, 180);

	const int kPadX = 8;  // horizontal inner padding
	const int kPadY = kSubtitlePadY;  // vertical inner padding
	const int innerW  = boxRect.width() - kPadX * 2;
	const int innerX  = boxRect.left + kPadX;
	int curY = boxRect.top + kPadY;

	// Build speaker prefix (e.g. "Arthur: ")
	Common::String speakerPrefix = entry.speaker.empty() ? "" : (entry.speaker + ": ");
	int speakerW = speakerPrefix.empty() ? 0 : _fontBold->getStringWidth(speakerPrefix);

	// Word-wrap text across up to 2 lines
	int line1AvailW = innerW - speakerW;
	Common::Array<Common::String> lines = wrapText(entry.text, line1AvailW, innerW);

	// Draw line 1: speaker prefix (bold orange) + first line of dialogue (cream-amber)
	if (!lines.empty()) {
		int drawX = innerX;
		if (!speakerPrefix.empty()) {
			_fontBold->drawString(destSurface, speakerPrefix, drawX, curY, innerW, orangeColor, Graphics::kTextAlignLeft);
			drawX += speakerW;
		}
		int line1AvailWActual = boxRect.right - kPadX - drawX;
		if (line1AvailWActual > 0)
			_font->drawString(destSurface, lines[0], drawX, curY, line1AvailWActual, dialogueColor, Graphics::kTextAlignLeft);
		curY += fontHeight;
	}

	// Draw line 2 (if present): dialogue continues (cream-amber)
	if (lines.size() >= 2 && curY + fontHeight <= boxRect.bottom) {
		_font->drawString(destSurface, lines[1], innerX, curY, innerW, dialogueColor, Graphics::kTextAlignLeft);
	}

	_vm->_gfx->invalidateRect(boxRect, false);
}

Common::Array<Common::String> SubtitleManager::wrapText(const Common::String &text, int line1AvailW, int line2AvailW) {
	Common::Array<Common::String> lines;
	Common::String remaining = text;
	bool firstLine = true;

	while (!remaining.empty() && lines.size() < 2) {
		int availW = firstLine ? line1AvailW : line2AvailW;
		if (availW <= 0) {
			firstLine = false;
			availW = line2AvailW;
		}

		Common::String fittingLine;
		Common::String rest = remaining;

		while (!rest.empty()) {
			uint nextSpace = 0;
			while (nextSpace < rest.size() && rest[nextSpace] != ' ')
				++nextSpace;
			Common::String word = rest.substr(0, nextSpace);

			Common::String candidate = fittingLine.empty() ? word : (fittingLine + " " + word);

			if (_font->getStringWidth(candidate) <= availW) {
				fittingLine = candidate;
				rest = (nextSpace < rest.size()) ? rest.substr(nextSpace + 1) : "";
			} else {
				if (fittingLine.empty()) {
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

	return lines;
}

int SubtitleManager::getFontHeight() {
	updateFont();
	return _font ? _font->getFontHeight() : 14;
}

int SubtitleManager::getBoxHeight() {
	return (getFontHeight() * 2) + (kSubtitlePadY * 2) + 10;
}

Common::Rect SubtitleManager::getDefaultBoxBounds() {
	int boxHeight = getBoxHeight();
	return Common::Rect(kSubtitleBoxX, kSubtitleViewportTop, kSubtitleBoxX + kSubtitleBoxWidth, kSubtitleViewportTop + boxHeight);
}

Common::Rect SubtitleManager::calculateBoxBoundsForVideo(const Common::Rect &videoFrameRect) {
	int boxHeight = getBoxHeight();
	return Common::Rect(
		videoFrameRect.left - 16,
		videoFrameRect.bottom,
		videoFrameRect.right + 12,
		videoFrameRect.bottom + boxHeight
	);
}

} // End of namespace Buried

