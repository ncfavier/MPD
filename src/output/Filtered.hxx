/*
 * Copyright 2003-2017 The Music Player Daemon Project
 * http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef MPD_FILTERED_AUDIO_OUTPUT_HXX
#define MPD_FILTERED_AUDIO_OUTPUT_HXX

#include "AudioFormat.hxx"
#include "filter/Observer.hxx"

#include <memory>
#include <string>
#include <chrono>

class PreparedFilter;
class MusicPipe;
class EventLoop;
class Mixer;
class MixerListener;
struct MixerPlugin;
struct MusicChunk;
struct ConfigBlock;
class AudioOutput;
struct ReplayGainConfig;
struct Tag;

struct FilteredAudioOutput {
	const char *const plugin_name;

	/**
	 * The device's configured display name.
	 */
	const char *name;

private:
	/**
	 * A string describing this devicee in log messages.  It is
	 * usually in the form "NAME (PLUGIN)".
	 */
	std::string log_name;

public:
	/**
	 * The plugin which implements this output device.
	 */
	std::unique_ptr<AudioOutput> output;

	/**
	 * The #mixer object associated with this audio output device.
	 * May be nullptr if none is available, or if software volume is
	 * configured.
	 */
	Mixer *mixer = nullptr;

	/**
	 * The configured audio format.
	 */
	AudioFormat config_audio_format;

	/**
	 * The #AudioFormat which is emitted by the #Filter, with
	 * #config_audio_format already applied.  This is used to
	 * decide whether this object needs to be closed and reopened
	 * upon #AudioFormat changes.
	 */
	AudioFormat filter_audio_format;

	/**
	 * The audio_format which is really sent to the device.  This
	 * is basically config_audio_format (if configured) or
	 * in_audio_format, but may have been modified by
	 * plugin->open().
	 */
	AudioFormat out_audio_format;

	/**
	 * The filter object of this audio output.  This is an
	 * instance of chain_filter_plugin.
	 */
	PreparedFilter *prepared_filter = nullptr;

	/**
	 * The #VolumeFilter instance of this audio output.  It is
	 * used by the #SoftwareMixer.
	 */
	FilterObserver volume_filter;

	/**
	 * The replay_gain_filter_plugin instance of this audio
	 * output.
	 */
	PreparedFilter *prepared_replay_gain_filter = nullptr;

	/**
	 * The replay_gain_filter_plugin instance of this audio
	 * output, to be applied to the second chunk during
	 * cross-fading.
	 */
	PreparedFilter *prepared_other_replay_gain_filter = nullptr;

	/**
	 * The convert_filter_plugin instance of this audio output.
	 * It is the last item in the filter chain, and is responsible
	 * for converting the input data into the appropriate format
	 * for this audio output.
	 */
	FilterObserver convert_filter;

	/**
	 * Throws #std::runtime_error on error.
	 */
	FilteredAudioOutput(const char *_plugin_name,
			    std::unique_ptr<AudioOutput> &&_output,
			    const ConfigBlock &block);

	~FilteredAudioOutput();

private:
	void Configure(const ConfigBlock &block);

public:
	void Setup(EventLoop &event_loop,
		   const ReplayGainConfig &replay_gain_config,
		   const MixerPlugin *mixer_plugin,
		   MixerListener &mixer_listener,
		   const ConfigBlock &block);

	void BeginDestroy() noexcept;
	void FinishDestroy() noexcept;

	const char *GetName() const {
		return name;
	}

	const char *GetPluginName() const noexcept {
		return plugin_name;
	}

	const char *GetLogName() const noexcept {
		return log_name.c_str();
	}

	/**
	 * Does the plugin support enabling/disabling a device?
	 */
	gcc_pure
	bool SupportsEnableDisable() const noexcept;

	/**
	 * Does the plugin support pausing a device?
	 */
	gcc_pure
	bool SupportsPause() const noexcept;

	/**
	 * Throws #std::runtime_error on error.
	 */
	void Enable();

	void Disable() noexcept;

	/**
	 * Invoke OutputPlugin::close().
	 *
	 * Caller must not lock the mutex.
	 */
	void Close(bool drain) noexcept;

	void ConfigureConvertFilter();

	/**
	 * Invoke OutputPlugin::open() and configure the
	 * #ConvertFilter.
	 *
	 * Throws #std::runtime_error on error.
	 *
	 * Caller must not lock the mutex.
	 */
	void OpenOutputAndConvert(AudioFormat audio_format);

	/**
	 * Close the output plugin.
	 *
	 * Mutex must not be locked.
	 */
	void CloseOutput(bool drain) noexcept;

	/**
	 * Mutex must not be locked.
	 */
	void OpenSoftwareMixer() noexcept;

	/**
	 * Mutex must not be locked.
	 */
	void CloseSoftwareMixer() noexcept;

	gcc_pure
	std::chrono::steady_clock::duration Delay() noexcept;

	void SendTag(const Tag &tag);

	size_t Play(const void *data, size_t size);

	void Drain();
	void Cancel() noexcept;

	void BeginPause() noexcept;
	bool IteratePause() noexcept;

	void EndPause() noexcept{
	}
};

/**
 * Notify object used by the thread's client, i.e. we will send a
 * notify signal to this object, expecting the caller to wait on it.
 */
extern struct notify audio_output_client_notify;

/**
 * Throws #std::runtime_error on error.
 */
FilteredAudioOutput *
audio_output_new(EventLoop &event_loop,
		 const ReplayGainConfig &replay_gain_config,
		 const ConfigBlock &block,
		 MixerListener &mixer_listener);

#endif
