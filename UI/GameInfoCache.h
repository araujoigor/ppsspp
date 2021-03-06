﻿// Copyright (c) 2013- PPSSPP Project.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0 or later versions.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/

// Official git repository and contact information can be found at
// https://github.com/hrydgard/ppsspp and http://www.ppsspp.org/.

#pragma once

#include <string>
#include <map>
#include <mutex>
#include <atomic>

#include "file/file_util.h"
#include "Core/ELF/ParamSFO.h"
#include "UI/TextureUtil.h"

namespace Draw {
	class DrawContext;
	class Texture;
}
class PrioritizedWorkQueue;

// A GameInfo holds information about a game, and also lets you do things that the VSH
// does on the PSP, namely checking for and deleting savedata, and similar things.
// Only cares about games that are installed on the current device.

// A GameInfo object can also represent a piece of savedata.

// Guessed from GameID, not necessarily accurate
enum GameRegion {
	GAMEREGION_JAPAN,
	GAMEREGION_USA,
	GAMEREGION_EUROPE,
	GAMEREGION_HONGKONG,
	GAMEREGION_ASIA,
	GAMEREGION_OTHER,
	GAMEREGION_MAX,
};

enum GameInfoWantFlags {
	GAMEINFO_WANTBG = 0x01,
	GAMEINFO_WANTSIZE = 0x02,
	GAMEINFO_WANTSND = 0x04,
};

class FileLoader;
enum class IdentifiedFileType;

class GameInfo {
public:
	GameInfo();
	~GameInfo();

	bool Delete();  // Better be sure what you're doing when calling this.
	bool DeleteAllSaveData();
	bool LoadFromPath(const std::string &gamePath);

	FileLoader *GetFileLoader();
	void DisposeFileLoader();

	u64 GetGameSizeInBytes();
	u64 GetSaveDataSizeInBytes();
	u64 GetInstallDataSizeInBytes();

	void ParseParamSFO();

	std::vector<std::string> GetSaveDataDirectories();

	std::string GetTitle();
	void SetTitle(const std::string &newTitle);

	bool IsPending();
	bool IsWorking();

	// Hold this when reading or writing from the GameInfo.
	// Don't need to hold it when just passing around the pointer,
	// and obviously also not when creating it and holding the only pointer
	// to it.
	std::mutex lock;

	std::string id;
	std::string id_version;
	int disc_total = 0;
	int disc_number = 0;
	int region = -1;
	IdentifiedFileType fileType;
	ParamSFOData paramSFO;
	bool paramSFOLoaded;
	bool hasConfig;

	// Pre read the data, create a texture the next time (GL thread..)
	std::string iconTextureData;
	ManagedTexture *iconTexture;
	std::string pic0TextureData;
	ManagedTexture *pic0Texture;
	std::string pic1TextureData;
	ManagedTexture *pic1Texture;

	std::string sndFileData;

	int wantFlags;

	double lastAccessedTime;

	// The time at which the Icon and the BG were loaded.
	// Can be useful to fade them in smoothly once they appear.
	double timeIconWasLoaded;
	double timePic0WasLoaded;
	double timePic1WasLoaded;

	std::atomic<bool> iconDataLoaded;
	std::atomic<bool> pic0DataLoaded;
	std::atomic<bool> pic1DataLoaded;
	std::atomic<bool> sndDataLoaded;

	u64 gameSize;
	u64 saveDataSize;
	u64 installDataSize;
	bool pending;
	bool working;

protected:
	// Note: this can change while loading, use GetTitle().
	std::string title;

	FileLoader *fileLoader;
	std::string filePath_;
};

class GameInfoCache {
public:
	GameInfoCache();
	~GameInfoCache();

	// This creates a background worker thread!
	void Clear();
	void PurgeType(IdentifiedFileType fileType);

	// All data in GameInfo including iconTexture may be zero the first time you call this
	// but filled in later asynchronously in the background. So keep calling this,
	// redrawing the UI often. Only set flags to GAMEINFO_WANTBG or WANTSND if you really want them 
	// because they're big. bgTextures and sound may be discarded over time as well.
	GameInfo *GetInfo(Draw::DrawContext *draw, const std::string &gamePath, int wantFlags);
	void FlushBGs();  // Gets rid of all BG textures. Also gets rid of bg sounds.

	PrioritizedWorkQueue *WorkQueue() { return gameInfoWQ_; }

	void WaitUntilDone(GameInfo *info);

private:
	void Init();
	void Shutdown();
	void SetupTexture(GameInfo *info, std::string &textureData, Draw::DrawContext *draw, ManagedTexture *&tex, double &loadTime);

	// Maps ISO path to info.
	std::map<std::string, GameInfo *> info_;

	// Work queue and management
	PrioritizedWorkQueue *gameInfoWQ_;
};

// This one can be global, no good reason not to.
extern GameInfoCache *g_gameInfoCache;
