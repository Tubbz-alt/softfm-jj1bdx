#ifndef SOFTFM_AUDIOOUTPUT_H
#define SOFTFM_AUDIOOUTPUT_H

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "SoftFM.h"

// Base class for writing audio data to file or playback.
class AudioOutput {
public:
  // Destructor.
  virtual ~AudioOutput() {}

  // Write audio data.
  // Return true on success.
  // Return false if an error occurs.
  virtual bool write(const SampleVector &samples) = 0;

  // Return the last error, or return an empty string if there is no error.
  std::string error() {
    std::string ret(m_error);
    m_error.clear();
    return ret;
  }

  // Return true if the stream is OK, return false if there is an error.
  operator bool() const { return (!m_zombie) && m_error.empty(); }

protected:
  // Constructor.
  AudioOutput() : m_zombie(false) {}

  // Encode a list of samples as signed 16-bit little-endian integers.
  static void samplesToInt16(const SampleVector &samples,
                             std::vector<std::uint8_t> &bytes);

  std::string m_error;
  bool m_zombie;

private:
  AudioOutput(const AudioOutput &);            // no copy constructor
  AudioOutput &operator=(const AudioOutput &); // no assignment operator
};

// Write audio data as raw signed 16-bit little-endian data.
class RawAudioOutput : public AudioOutput {
public:
  // Construct raw audio writer.
  // filename :: file name (including path) or "-" to write to stdout
  RawAudioOutput(const std::string &filename);

  ~RawAudioOutput();
  bool write(const SampleVector &samples);

private:
  int m_fd;
  std::vector<std::uint8_t> m_bytebuf;
};

// Write audio data as .WAV file.
class WavAudioOutput : public AudioOutput {
public:
  // Construct .WAV writer.
  // Stereo output always.
  // filename     :: file name (including path) or "-" to write to stdout
  // samplerate   :: audio sample rate in Hz
  WavAudioOutput(const std::string &filename, unsigned int samplerate);

  ~WavAudioOutput();
  bool write(const SampleVector &samples);

private:
  // (Re-)Write .WAV header.
  bool write_header(unsigned int nsamples);

  static void encode_chunk_id(std::uint8_t *ptr, const char *chunkname);

  template <typename T> static void set_value(std::uint8_t *ptr, T value);

  const unsigned numberOfChannels;
  const unsigned sampleRate;
  std::FILE *m_stream;
  std::vector<std::uint8_t> m_bytebuf;
};

#endif
