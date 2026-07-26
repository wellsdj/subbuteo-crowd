#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "../core/BendCurve.h"
#include "../core/BendEmitter.h"

/** SnapBend — a MIDI FX plugin that draws pitch bend curves which snap to
    whole semitones, and tells the receiving synth how far it is allowed to
    bend so that those semitones are actually the semitones you asked for.
*/
class SnapBendProcessor : public juce::AudioProcessor
{
public:
    SnapBendProcessor();
    ~SnapBendProcessor() override = default;

    // ---- AudioProcessor ------------------------------------------------

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    // Keeps the double-precision overload visible; we only implement the float
    // one, and hiding the other trips a warning in some hosts' builds.
    using juce::AudioProcessor::processBlock;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SnapBend"; }

    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- curve access --------------------------------------------------
    //
    // The editor lives on the message thread and the emitter runs on the audio
    // thread, so the curve is only ever handed over as a copy under a lock. The
    // curves are a handful of nodes, so this is cheap.

    snapbend::BendCurve getCurve() const;
    void setCurve (const snapbend::BendCurve& newCurve);

    /** Zoom is view state rather than something you would automate, so it is
        kept here and saved with the project, but is not a parameter. */
    void setZoom (double horizontal, double vertical) noexcept;
    double getHorizontalZoom() const noexcept { return horizontalZoom.load(); }
    double getVerticalZoom()   const noexcept { return verticalZoom.load(); }

    /** Where the host's playhead is, for drawing the position line. */
    double getCurrentBeat() const noexcept { return currentBeat.load(); }
    bool   isPlaying()      const noexcept { return transportRunning.load(); }

    /** The host's time signature, for the bar ruler. */
    int getTimeSignatureNumerator()   const noexcept { return timeSigNumerator.load(); }
    int getTimeSignatureDenominator() const noexcept { return timeSigDenominator.load(); }

    /** Sends a centred bend on the next block. Used by the panic button, and
        whenever the plugin is bypassed, so nothing is left detuned. */
    void requestSafetyReset() noexcept { safetyResetPending.store (true); }

    /** Plays the alternating reference/bent note pair used to set FINE by ear. */
    void setCalibrating (bool shouldCalibrate) noexcept { calibrating.store (shouldCalibrate); }
    bool isCalibrating() const noexcept { return calibrating.load(); }

    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    snapbend::BendEmitter emitter;

    mutable juce::CriticalSection curveLock;
    snapbend::BendCurve           curve;

    std::atomic<double> horizontalZoom     { 0.5 };
    std::atomic<double> verticalZoom       { 0.5 };

    std::atomic<int>    timeSigNumerator   { 4 };
    std::atomic<int>    timeSigDenominator { 4 };

    std::atomic<double> currentBeat        { 0.0 };
    std::atomic<bool>   transportRunning   { false };
    std::atomic<bool>   safetyResetPending { false };
    std::atomic<bool>   calibrating        { false };
    bool                wasCalibrating     = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SnapBendProcessor)
};
