#include <iostream> 
#include "speechapi_cxx.h"
#include <cpr/cpr.h>
#include <argparse/argparse.hpp>
#include <filesystem>
#include <alsa/asoundlib.h>
#include <nlohmann/json.hpp>

using namespace Microsoft::CognitiveServices::Speech;
using namespace Microsoft::CognitiveServices::Speech::Audio;
using namespace Microsoft::CognitiveServices::Speech::Transcription;
using namespace std;

const string RED = "\033[0;31m";
const string GREEN = "\033[0;32m";
const string YELLOW = "\033[0;33m";

std::string GetEnvironmentVariable(const char* name);

string create_voice_signature(const string& key, const string& region, const string& wav_file)
{
    string endpoint = "https://signature." + region + ".cts.speech.microsoft.com/api/v1/Signature/GenerateVoiceSignatureFromByteArray";
    // Post the audio file to the endpoint
    cpr::Response r = cpr::Post(cpr::Url{endpoint},
                                cpr::Header{{"Ocp-Apim-Subscription-Key", key}, {"Content-Type", "application/octet-stream"}},
                                cpr::Body{cpr::File(wav_file)},
                                cpr::VerifySsl{false});
    if (r.status_code != 200) {
        throw std::runtime_error("Failed to create voice signature");
    }

    // Return the signature 
    string signature = nlohmann::json::parse(r.text)["Signature"].dump();
    std::cout << signature << std::endl;
    return signature;
}

// string find_azure_kinect()
// {
//     // Loop through all available alsa inputs
//     // print the number of channels for each device
//     // and return the first device with 8 channels
    
// }

int main(int argc, char* argv[])
{
    argparse::ArgumentParser program("multi_user_asr");
    program.add_argument("--speaker-samples")
        .help("WAV files for each speaker")
        .nargs(argparse::nargs_pattern::any);

    try
    {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err)
    {
        std::cout << err.what() << std::endl;
        std::cout << program;
        exit(0);
    }
    
    // This example requires environment variables named "SPEECH_KEY" and "SPEECH_REGION"
    auto speech_key = GetEnvironmentVariable("SPEECH_KEY");
    auto speech_region = GetEnvironmentVariable("SPEECH_REGION");

    if ((std::size(speech_key) == 0) || (size(speech_region) == 0)) {
        std::cout << "Please set both SPEECH_KEY and SPEECH_REGION environment variables." << std::endl;
        return -1;
    }

    std::map<std::string, std::string> speaker_signatures;
    for (auto wav_file : program.get<std::vector<std::string>>("--speaker-samples")) {
        std::cout << "Generating signature for " << wav_file << std::endl;
        string signature = create_voice_signature(speech_key, speech_region, wav_file);
        string stem = std::filesystem::path(wav_file).stem().string();
        speaker_signatures[signature] = stem;
    }

    auto speech_config = SpeechConfig::FromSubscription(speech_key, speech_region);
    speech_config->SetSpeechRecognitionLanguage("en-US");
    speech_config->SetProperty("ConversationTranscriptionInRoomAndOnline", "true");
    speech_config->SetProperty("DifferentiateGuestSpeakers", "true");
    auto audioProcessingOptions = AudioProcessingOptions::Create(
        AUDIO_INPUT_PROCESSING_ENABLE_DEFAULT,
        PresetMicrophoneArrayGeometry::Circular7,
        SpeakerReferenceChannel::None);
    auto audio_config = AudioConfig::FromMicrophoneInput("hw:1,0", audioProcessingOptions);
    auto transcriber = Transcription::ConversationTranscriber::FromConfig(audio_config);
    auto conversation = Transcription::Conversation::CreateConversationAsync(speech_config, "conversation1").get();

    promise<void> recognition_end;
    transcriber->Transcribing.Connect([](const ConversationTranscriptionEventArgs& e)
    {
        cout << YELLOW << "Recognizing: " << e.Result->Text << "\r" << flush;
    });
    transcriber->Transcribed.Connect([](const ConversationTranscriptionEventArgs& e)
    {
        if (e.Result->Reason == ResultReason::RecognizedSpeech)
        {
            // cout << "Transcribed: Text=" << e.Result->Text << std::endl
            //     << "  Offset=" << e.Result->Offset() << std::endl
            //     << "  Duration=" << e.Result->Duration() << std::endl
            //     << "  UserId=" << e.Result->UserId << std::endl;
            if (e.Result->Text.size() == 0) {
                return;
            }
            cout << GREEN << "\033[K" << e.Result->UserId << ": " << e.Result->Text << endl;
        }
        else if (e.Result->Reason == ResultReason::NoMatch)
        {
            cout << RED << "\033[K" << "NOMATCH: Speech could not be recognized." << endl;
        }
    });
    transcriber->Canceled.Connect([&recognition_end](const ConversationTranscriptionCanceledEventArgs& e)
    {
        cout << RED << "\033[K" << "CANCELED: Reason=" << (int)e.Reason << endl;
        if (e.Reason == CancellationReason::Error)
        {
            cout << "CANCELED: ErrorCode=" << (int)e.ErrorCode << "\n"
                 << "CANCELED: ErrorDetails=" << e.ErrorDetails << "\n"
                 << "CANCELED: Did you set the speech resource key and region values?" << endl;

            recognition_end.set_value(); // Notify to stop recognition.
        }
    });
    transcriber->SessionStopped.Connect([&recognition_end](const SessionEventArgs& e)
    {
        cout << "Session stopped.";
        recognition_end.set_value(); // Notify to stop recognition.
    });

    // Add speaker_signatures to the conversation
    for (auto& [signature, name] : speaker_signatures) {
        conversation->AddParticipantAsync(Participant::From(name, "en-us", signature)).get();
    }

    transcriber->JoinConversationAsync(conversation).get();
    cout << "Conversation joined." << std::endl;
    transcriber->StartTranscribingAsync().wait();

    recognition_end.get_future().wait();

    transcriber->StopTranscribingAsync().wait();
}

std::string GetEnvironmentVariable(const char* name)
{
    auto value = getenv(name);
    return value ? value : "";
}