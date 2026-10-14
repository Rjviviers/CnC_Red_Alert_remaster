// S09: the game's sound API (audio.h, WIN32LIB/AUDIO/SOUNDIO.CPP) on the null backend.
//
// Calls every function through the unchanged audio.h, so a signature that drifted from the
// header would fail to link. Synthetic AUD data and an in-memory file system stand in for
// game assets. The null backend is pumped by hand, so the tests don't depend on the clock.
#include "ra_test.h"

#include "audio_compat.h"
#include "audio.h"
#include "soundint.h"
#include "audio_mixer.h"
#include "audio_backend.h"

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------------------------------
// In-memory files: the game defines these in CODE/CCFILE.CPP.
// ---------------------------------------------------------------------------------------------
namespace {

struct MemFile {
    std::string name;
    std::vector<unsigned char> data;
};
std::vector<MemFile> Files;

struct OpenMemFile {
    bool open;
    size_t file;
    size_t pos;
};
OpenMemFile Handles[8];

int Open_Count()
{
    int n = 0;
    for (int i = 0; i < 8; i++) if (Handles[i].open) n++;
    return n;
}

int Find(char const *name)
{
    for (size_t i = 0; i < Files.size(); i++) if (Files[i].name == name) return (int)i;
    return -1;
}

} // namespace

int __cdecl Open_File(char const *file_name, int)
{
    int f = Find(file_name);
    if (f < 0) return WW_ERROR;
    for (int i = 0; i < 8; i++) {
        if (!Handles[i].open) {
            Handles[i].open = true;
            Handles[i].file = (size_t)f;
            Handles[i].pos = 0;
            return i;
        }
    }
    return WW_ERROR;
}

void __cdecl Close_File(int handle)
{
    if (handle >= 0 && handle < 8) Handles[handle].open = false;
}

long __cdecl Read_File(int handle, void *buf, unsigned long bytes)
{
    if (handle < 0 || handle >= 8 || !Handles[handle].open) return 0;
    OpenMemFile &h = Handles[handle];
    std::vector<unsigned char> const &d = Files[h.file].data;
    unsigned long left = (unsigned long)(d.size() - h.pos);
    if (bytes > left) bytes = left;
    if (bytes) std::memcpy(buf, &d[h.pos], bytes);
    h.pos += bytes;
    return (long)bytes;
}

int __cdecl Find_File(char const *file_name)
{
    return Find(file_name) >= 0;
}

unsigned long __cdecl File_Size(int handle)
{
    if (handle < 0 || handle >= 8 || !Handles[handle].open) return 0;
    return (unsigned long)Files[Handles[handle].file].data.size();
}

// ---------------------------------------------------------------------------------------------
// AUD builders
// ---------------------------------------------------------------------------------------------
namespace {

void Put16(std::vector<unsigned char> &v, unsigned value)
{
    v.push_back((unsigned char)value);
    v.push_back((unsigned char)(value >> 8));
}

void Put32(std::vector<unsigned char> &v, unsigned long value)
{
    for (int i = 0; i < 4; i++) v.push_back((unsigned char)(value >> (8 * i)));
}

std::vector<unsigned char> Aud_Header(unsigned rate, unsigned long size, unsigned long uncomp, int flags, int comp)
{
    std::vector<unsigned char> h;
    Put16(h, rate);
    Put32(h, size);
    Put32(h, uncomp);
    h.push_back((unsigned char)flags);
    h.push_back((unsigned char)comp);
    return h;
}

// Uncompressed 16-bit mono AUD with the given samples.
std::vector<unsigned char> Raw_Aud(std::vector<short> const &pcm, unsigned rate = 22050)
{
    std::vector<unsigned char> a = Aud_Header(rate, pcm.size() * 2, pcm.size() * 2, AUD_FLAG_16BIT, SCOMP_NONE);
    for (size_t i = 0; i < pcm.size(); i++) Put16(a, (unsigned short)pcm[i]);
    return a;
}

std::vector<short> Ramp(size_t count, int start = -20000, int step = 7)
{
    std::vector<short> v(count);
    int value = start;
    for (size_t i = 0; i < count; i++) {
        v[i] = (short)value;
        value += step;
        if (value > 20000) value = -20000;
    }
    return v;
}

// IMA ADPCM 16-bit mono AUD: 'frames' frames of 512 random code bytes (2048 output bytes
// each). 'expected' receives the PCM, decoded with the (separately tested) decoder.
std::vector<unsigned char> Ima_Aud(int frames, std::vector<short> &expected, unsigned seed)
{
    std::vector<unsigned char> body;
    expected.clear();
    _SOS_COMPRESS_INFO info;
    std::memset(&info, 0, sizeof(info));
    info.wBitSize = 16;
    info.wChannels = 1;
    sosCODECInitStream(&info);
    for (int f = 0; f < frames; f++) {
        unsigned char code[512];
        for (int i = 0; i < 512; i++) {
            seed = seed * 1103515245u + 12345u;
            code[i] = (unsigned char)(seed >> 16);
        }
        Put16(body, 512);
        Put16(body, 2048);
        Put32(body, 0x0000DEAFUL);
        body.insert(body.end(), code, code + 512);

        short pcm[1024];
        info.lpSource = (char *)code;
        info.lpDest = (char *)pcm;
        sosCODECDecompressData(&info, 2048);
        expected.insert(expected.end(), pcm, pcm + 1024);
    }
    std::vector<unsigned char> a = Aud_Header(22050, body.size(), (unsigned long)frames * 2048, AUD_FLAG_16BIT, SCOMP_SOS);
    a.insert(a.end(), body.begin(), body.end());
    return a;
}

NullAudioBackendClass Null;

// Init on the null backend, mono 22050 Hz like the game (CODE/STARTUP.CPP).
bool Start_Audio()
{
    for (int i = 0; i < 8; i++) Handles[i].open = false;
    Null.Set_Real_Time(false);
    Audio_Set_Backend(&Null);
    return Audio_Init(0, 16, FALSE, 22050, 0) == TRUE;
}

void Stop_Audio()
{
    Sound_End();
    Audio_Set_Backend(0);
}

std::vector<short> Pump(int frames)
{
    std::vector<short> out((size_t)frames);
    Null.Pump(frames, &out[0]);
    return out;
}

} // namespace


RA_TEST(api_is_inert_before_init)
{
    RA_CHECK_EQ(-1, Get_Digi_Handle());
    std::vector<unsigned char> aud = Raw_Aud(Ramp(100));
    RA_CHECK_EQ(-1, Play_Sample(&aud[0], 255, 255, 0));
    RA_CHECK(!Sample_Status(0));
    RA_CHECK(!Is_Sample_Playing(&aud[0]));
    RA_CHECK_EQ(-1, File_Stream_Sample("NONE.AUD"));
    RA_CHECK(Load_Sample("NONE.AUD") == 0);
    Sound_Callback();   // must not crash
}

RA_TEST(api_init_uses_selected_backend)
{
    RA_CHECK(Start_Audio());
    RA_CHECK_EQ(1, Get_Digi_Handle());
    RA_CHECK_EQ(1, (int)SampleType);
    RA_CHECK(Null.Is_Open());
    RA_CHECK(!Null.Is_Paused());
    RA_CHECK(Audio_Init(0, 16, FALSE, 22050, 0) == TRUE);    // second call is harmless
    RA_CHECK(Set_Primary_Buffer_Format() == TRUE);
    Stop_Audio();
    RA_CHECK(!Null.Is_Open());
    RA_CHECK_EQ(-1, Get_Digi_Handle());
}

RA_TEST(api_play_uncompressed_sample_bit_exact)
{
    RA_CHECK(Start_Audio());
    std::vector<short> pcm = Ramp(3000);
    std::vector<unsigned char> aud = Raw_Aud(pcm);

    int handle = Play_Sample(&aud[0], 255, 255, 0);
    RA_CHECK_EQ(MAX_SFX - 1, handle);        // free slots are taken from the top
    RA_CHECK(Sample_Status(handle) == TRUE);
    RA_CHECK(Is_Sample_Playing(&aud[0]) == TRUE);

    // Volume 255 with sound volume 255 is unity gain (DirectSound volume 0), so the
    // output equals the input.
    std::vector<short> out = Pump(3100);
    RA_CHECK_MEM_EQ(&pcm[0], &out[0], pcm.size() * sizeof(short));
    RA_CHECK_EQ(0, out[3050]);
    RA_CHECK(Sample_Status(handle) == FALSE);
    RA_CHECK(Is_Sample_Playing(&aud[0]) == FALSE);
    Stop_Audio();
}

RA_TEST(api_play_ima_compressed_sample)
{
    RA_CHECK(Start_Audio());
    std::vector<short> expected;
    std::vector<unsigned char> aud = Ima_Aud(6, expected, 42);
    int handle = Play_Sample(&aud[0], 255, 255, 0);
    RA_CHECK(handle >= 0);
    std::vector<short> out = Pump((int)expected.size() + 100);
    RA_CHECK_MEM_EQ(&expected[0], &out[0], expected.size() * sizeof(short));
    RA_CHECK(!Sample_Status(handle));
    Stop_Audio();
}

RA_TEST(api_play_westwood_compressed_8bit_sample)
{
    RA_CHECK(Start_Audio());
    // One frame: the vector from s09_decoder_test (13 code bytes -> 16 unsigned samples).
    unsigned char const code[13] = {0x40, 0x9F, 0x00, 0xE4, 0xA3, 0xBF, 0x81, 0x10, 0xF0, 0xC2, 0x40, 0xFF, 0xAF};
    unsigned char const pcm8[16] = {
        0x88, 0x89, 0x87, 0x86, 0x86, 0x87, 0x8A, 0x89, 0x10, 0xF0, 0xF0, 0xF0, 0xF0, 0xF8, 0xFF, 0x0E
    };
    std::vector<unsigned char> aud = Aud_Header(22050, 8 + 13, 16, 0, SCOMP_WESTWOOD);
    Put16(aud, 13);
    Put16(aud, 16);
    Put32(aud, 0x0000DEAFUL);
    aud.insert(aud.end(), code, code + 13);

    RA_CHECK(Play_Sample(&aud[0], 255, 255, 0) >= 0);
    std::vector<short> out = Pump(20);
    for (int i = 0; i < 16; i++) RA_CHECK_EQ(((int)pcm8[i] - 128) * 256, out[i]);
    RA_CHECK_EQ(0, out[17]);
    Stop_Audio();
}

RA_TEST(api_resamples_to_device_rate_and_fudges_22k)
{
    RA_CHECK(Start_Audio());
    // 11025 Hz source on the 22050 Hz device: twice as many output frames.
    std::vector<unsigned char> aud = Raw_Aud(std::vector<short>(100, 1000), 11025);
    int handle = Play_Sample(&aud[0], 255, 255, 0);
    Pump(150);
    RA_CHECK(Sample_Status(handle));
    Pump(60);
    RA_CHECK(!Sample_Status(handle));

    // Rates between 20000 and 24000 play at exactly 22050 (as the original did).
    std::vector<unsigned char> aud2 = Raw_Aud(std::vector<short>(100, 1000), 22000);
    RA_CHECK(Play_Sample(&aud2[0], 255, 255, 0) >= 0);
    RA_CHECK_EQ(22050, LockedData.SampleTracker[MAX_SFX - 1].PlaybackRate);
    Stop_Audio();
}

RA_TEST(api_priority_and_handles)
{
    RA_CHECK(Start_Audio());
    std::vector<unsigned char> aud = Raw_Aud(Ramp(50000));
    int handles[MAX_SFX];
    for (int i = 0; i < MAX_SFX; i++) {
        handles[i] = Play_Sample(&aud[0], 100, 255, 0);
        RA_CHECK_EQ(MAX_SFX - 1 - i, handles[i]);
    }
    // All slots busy with priority 100: a lower priority sound can't play...
    RA_CHECK_EQ(-1, Play_Sample(&aud[0], 50, 255, 0));
    // ...an equal or higher one takes the first slot whose priority is <= its own.
    RA_CHECK_EQ(0, Play_Sample(&aud[0], 100, 255, 0));
    RA_CHECK_EQ(100, LockedData.SampleTracker[0].Priority);
    RA_CHECK_EQ(0, Play_Sample(&aud[0], 200, 255, 0));      // slot 0 again (100 <= 200)
    // A finished sound frees its slot.
    Stop_Sample(3);
    RA_CHECK_EQ(3, Play_Sample(&aud[0], 10, 255, 0));
    Stop_Sample_Playing(&aud[0]);            // stops the first tracker playing this sample
    Stop_Audio();
}

RA_TEST(api_volume_curve)
{
    // The DirectSound mapping: 255 -> 0 dB, 0 -> silent, monotonic in between.
    RA_CHECK_EQ(65536, Audio_Volume_To_Gain(255));
    RA_CHECK_EQ(65536, Audio_Volume_To_Gain(254));         // -0.0368 dB truncates to 0
    RA_CHECK_EQ(0, Audio_Volume_To_Gain(0));
    RA_CHECK_EQ(0, Audio_Volume_To_Gain(-5));
    RA_CHECK_EQ(65536, Audio_Volume_To_Gain(1000));
    int previous = 0;
    bool monotonic = true;
    for (int v = 0; v < 256; v++) {
        if (Audio_Volume_To_Gain(v) < previous) monotonic = false;
        previous = Audio_Volume_To_Gain(v);
    }
    RA_CHECK(monotonic);
    // Volume 128: exp(127/27.686) - 1 = 97 hundredths of a dB -> 10^(-97/2000) = 0.894.
    RA_CHECK_EQ(-97, Convert_HMI_To_Direct_Sound_Volume(128));
    RA_CHECK(Audio_Volume_To_Gain(128) > 58500 && Audio_Volume_To_Gain(128) < 58700);
}

RA_TEST(api_sound_and_score_volume)
{
    RA_CHECK(Start_Audio());
    RA_CHECK_EQ(255, Set_Sound_Vol(128));
    std::vector<unsigned char> aud = Raw_Aud(Ramp(50000));
    int handle = Play_Sample(&aud[0], 255, 255, 0);
    // Gain = curve(sound volume * volume / 256) = curve(127).
    RA_CHECK_EQ(Audio_Volume_To_Gain(127), Audio_Mixer().Voice_Gain(handle));
    // Changing the sound volume doesn't touch playing sounds (as in the original).
    RA_CHECK_EQ(128, Set_Sound_Vol(255));
    RA_CHECK_EQ(Audio_Volume_To_Gain(127), Audio_Mixer().Voice_Gain(handle));
    RA_CHECK_EQ(255, Set_Score_Vol(100));
    RA_CHECK_EQ(100, Set_Score_Vol(255));
    Stop_Audio();
}

RA_TEST(api_fade_sample)
{
    RA_CHECK(Start_Audio());
    std::vector<unsigned char> aud = Raw_Aud(std::vector<short>(100000, 10000));
    int handle = Play_Sample(&aud[0], 255, 255, 0);
    Pump(100);
    // Volume 255<<7 = 32640; Reducer = 32640/10 + 1 = 3265 per 40 Hz tick:
    // silent after the 10th tick (3255 left after 9 ticks).
    Fade_Sample(handle, 10);
    RA_CHECK_EQ(3265, LockedData.SampleTracker[handle].Reducer);
    Pump(22050 / 40 * 9);
    RA_CHECK(LockedData.SampleTracker[handle].Volume > 0);
    Pump(22050 / 40 * 2);
    RA_CHECK_EQ(0, LockedData.SampleTracker[handle].Volume);
    std::vector<short> out = Pump(10);
    RA_CHECK_EQ(0, out[5]);
    RA_CHECK(Sample_Status(handle));         // still "playing" until Sound_Callback sees it
    Sound_Callback();
    RA_CHECK(!Sample_Status(handle));

    // A zero-tick fade stops at once.
    handle = Play_Sample(&aud[0], 255, 255, 0);
    Fade_Sample(handle, 0);
    RA_CHECK(!Sample_Status(handle));
    Stop_Audio();
}

RA_TEST(api_sample_length)
{
    // 44100 bytes of 16-bit mono at 22050 Hz = 1 s = 60 ticks (integer maths: 22050 / 367).
    std::vector<unsigned char> h = Aud_Header(22050, 0, 44100, AUD_FLAG_16BIT, 0);
    RA_CHECK_EQ(60, Sample_Length(&h[0]));
    std::vector<unsigned char> s = Aud_Header(22050, 0, 88200, AUD_FLAG_16BIT | AUD_FLAG_STEREO, 0);
    RA_CHECK_EQ(60, Sample_Length(&s[0]));
    RA_CHECK_EQ(0, Sample_Length(0));
}

RA_TEST(api_focus_stop_and_start)
{
    RA_CHECK(Start_Audio());
    std::vector<unsigned char> aud = Raw_Aud(Ramp(50000));
    int handle = Play_Sample(&aud[0], 255, 255, 0);
    Stop_Primary_Sound_Buffer();              // focus lost
    RA_CHECK(Null.Is_Paused());
    RA_CHECK(!Sample_Status(handle));
    RA_CHECK_EQ(-1, Play_Sample(&aud[0], 255, 255, 0));
    RA_CHECK(Start_Primary_Sound_Buffer(FALSE) == FALSE);
    RA_CHECK(Start_Primary_Sound_Buffer(TRUE) == TRUE);    // focus back
    RA_CHECK(!Null.Is_Paused());
    RA_CHECK(Play_Sample(&aud[0], 255, 255, 0) >= 0);
    Restore_Sound_Buffers();
    Stop_Audio();
}

RA_TEST(api_load_sample_from_file)
{
    RA_CHECK(Start_Audio());
    std::vector<short> pcm = Ramp(500);
    MemFile f;
    f.name = "BEEP.AUD";
    f.data = Raw_Aud(pcm);
    Files.push_back(f);

    void *sample = Load_Sample("BEEP.AUD");
    RA_CHECK(sample != 0);
    RA_CHECK_EQ((long)f.data.size() + AUD_HEADER_SIZE, (long)Misc);
    RA_CHECK_MEM_EQ(&f.data[0], sample, f.data.size());
    RA_CHECK_EQ(0, Open_Count());

    std::vector<unsigned char> buffer(f.data.size());
    RA_CHECK_EQ((long)f.data.size(), Load_Sample_Into_Buffer("BEEP.AUD", &buffer[0], (long)buffer.size()));
    RA_CHECK_MEM_EQ(&f.data[0], &buffer[0], buffer.size());

    int handle = Play_Sample(sample, 255, 255, 0);
    std::vector<short> out = Pump(600);
    RA_CHECK_MEM_EQ(&pcm[0], &out[0], pcm.size() * sizeof(short));
    RA_CHECK(!Sample_Status(handle));
    Free_Sample(sample);
    Files.clear();
    Stop_Audio();
}

// Music: File_Stream_Sample_Vol reads the file in 2176-byte blocks into a 16-block ring,
// Sound_Callback (game thread) keeps it filled, and the mixer pulls frames across block
// boundaries. 60 IMA frames = 31,212 bytes = 15 blocks, more than the 12-block preload.
RA_TEST(api_stream_music_from_file)
{
    RA_CHECK(Start_Audio());
    std::vector<short> expected;
    MemFile f;
    f.name = "SCORE.AUD";
    f.data = Ima_Aud(60, expected, 7);
    Files.push_back(f);

    int handle = File_Stream_Sample_Vol("SCORE.AUD", 255, FALSE);
    RA_CHECK_EQ(MAX_SFX - 1, handle);
    RA_CHECK(LockedData.SampleTracker[handle].IsScore);
    RA_CHECK(Sample_Status(handle));

    std::vector<short> out;
    for (int i = 0; i < 70 && Sample_Status(handle); i++) {
        std::vector<short> chunk = Pump(1024);
        out.insert(out.end(), chunk.begin(), chunk.end());
        Sound_Callback();
    }
    RA_CHECK(!Sample_Status(handle));
    RA_CHECK(out.size() >= expected.size());
    if (out.size() >= expected.size()) {
        int mismatches = 0;
        for (size_t i = 0; i < expected.size(); i++) if (out[i] != expected[i]) mismatches++;
        RA_CHECK_EQ(0, mismatches);
    }
    RA_CHECK_EQ(0, Open_Count());             // the stream closed its file

    // Score volume applies to the playing score.
    handle = File_Stream_Sample("SCORE.AUD");
    RA_CHECK(handle >= 0);
    Set_Score_Vol(128);
    RA_CHECK_EQ(Audio_Volume_To_Gain(127), Audio_Mixer().Voice_Gain(handle));
    Set_Score_Vol(255);
    Stop_Sample(handle);
    RA_CHECK_EQ(0, Open_Count());
    Files.clear();
    Stop_Audio();
}

// The SDL backend mixes on its own thread. Pump from a second thread (about 10x real time)
// while the game thread streams a score with real-time start, plays, fades and stops sound
// effects and changes volumes. Nothing may crash or deadlock, and the score must come out
// bit-exact: the effects play at volume 0, so the output is the score alone.
RA_TEST(api_mixing_on_another_thread)
{
    RA_CHECK(Start_Audio());
    std::vector<short> expected;
    MemFile f;
    f.name = "SCORE.AUD";
    f.data = Ima_Aud(40, expected, 11);
    Files.push_back(f);
    std::vector<unsigned char> aud = Raw_Aud(Ramp(4000));

    std::atomic<bool> stop(false);
    std::vector<short> out;
    std::thread audio_thread([&stop, &out]() {
        short buffer[256];
        while (!stop.load()) {
            Null.Pump(256, buffer);
            out.insert(out.end(), buffer, buffer + 256);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    int score = File_Stream_Sample_Vol("SCORE.AUD", 255, TRUE);   // real-time start: 2 blocks per call
    RA_CHECK(score >= 0);
    RA_CHECK(Sample_Status(score));
    int iterations = 0;
    for (int i = 0; i < 20000 && Sample_Status(score); i++) {
        iterations++;
        int h = Play_Sample(&aud[0], i & 0xFF, 0, 0);
        if (i % 7 == 0) Fade_Sample(h, 3);
        if (i % 11 == 0) Stop_Sample(h);
        Set_Score_Vol(255);
        Set_Sound_Vol(255 - (i & 0x3F));
        Sound_Callback();
        std::this_thread::sleep_for(std::chrono::microseconds(200));
    }
    stop.store(true);
    audio_thread.join();
    RA_CHECK(!Sample_Status(score));         // the score played to the end
    RA_CHECK(iterations < 20000);

    // Align on the first non-zero sample (the score starts after a few preload calls).
    size_t first_out = 0, first_expected = 0;
    while (first_out < out.size() && out[first_out] == 0) first_out++;
    while (first_expected < expected.size() && expected[first_expected] == 0) first_expected++;
    size_t needed = expected.size() - first_expected;
    RA_CHECK(out.size() - first_out >= needed);
    if (out.size() - first_out >= needed) {
        int mismatches = 0;
        for (size_t i = 0; i < needed; i++) {
            if (out[first_out + i] != expected[first_expected + i]) mismatches++;
        }
        RA_CHECK_EQ(0, mismatches);
    }
    std::printf("  (%d game-thread iterations, %lu frames mixed, score starts at frame %lu)\n", iterations, (unsigned long)out.size(), (unsigned long)(first_out - first_expected));
    Files.clear();
    Stop_Audio();
    RA_CHECK_EQ(0, Open_Count());
}

// With the null backend in real-time mode (how the game runs with audio disabled), a sample
// ends after its duration even if the caller only polls its status, like the game's
// "while (Is_Speaking()) {};" busy-waits in CODE/SCENARIO.CPP.
RA_TEST(api_null_backend_real_time_busy_wait_ends)
{
    RA_CHECK(Start_Audio());
    Null.Set_Real_Time(true);
    std::vector<unsigned char> aud = Raw_Aud(std::vector<short>(2205, 100));   // 0.1 s
    int handle = Play_Sample(&aud[0], 255, 255, 0);
    RA_CHECK(handle >= 0);
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    double seconds = 0.0;
    while (Is_Sample_Playing(&aud[0]) && seconds < 2.0) {
        seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    }
    RA_CHECK(!Sample_Status(handle));
    RA_CHECK(seconds >= 0.05 && seconds < 2.0);
    std::printf("  (sample of 0.1 s ended after %.3f s)\n", seconds);
    Null.Set_Real_Time(false);
    Stop_Audio();
}

RA_TEST(api_after_sound_end)
{
    RA_CHECK(Start_Audio());
    Stop_Audio();
    std::vector<unsigned char> aud = Raw_Aud(Ramp(100));
    RA_CHECK_EQ(0, Play_Sample(&aud[0], 255, 255, 0));   // the original returned 0 here too
    RA_CHECK(!Sample_Status(0));
    RA_CHECK(Start_Audio());                               // and can start again
    RA_CHECK(Play_Sample(&aud[0], 255, 255, 0) >= 0);
    Stop_Audio();
}

RA_TEST(api_audio_init_fails_without_device)
{
    struct NoDevice : public AudioBackendClass {
        char const *Name(void) const { return "none"; }
        bool Open(AudioDeviceFormat const &, AudioMixCallback, void *) { return false; }
        void Close(void) {}
        void Pause(bool) {}
        void Set_Gain(float) {}
    } none;
    Audio_Set_Backend(&none);
    RA_CHECK(Audio_Init(0, 16, FALSE, 22050, 0) == FALSE);
    RA_CHECK_EQ(-1, Get_Digi_Handle());
    Audio_Set_Backend(0);
}

RA_TEST_MAIN()
