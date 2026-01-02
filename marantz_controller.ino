#include <Arduino.h>
#include <map>

// ---------- AMP COMMAND ----------
struct AmpCommand {
  uint8_t address;
  uint8_t command;
  uint8_t repeats;
  uint8_t extension;  // 0=RC5, >0=RC5Marantz

  AmpCommand(uint8_t a, uint8_t c, uint8_t r) 
    : address(a), command(c), repeats(r), extension(0) {}

  AmpCommand(uint8_t a, uint8_t c, uint8_t e, uint8_t r) 
    : address(a), command(c), repeats(r), extension(e) {}
};

// ---------- BUTTONS ----------
std::map<String, AmpCommand> ampButtons = {
  {"PowerToggle",         AmpCommand(16, 12, 1)},        
  {"VolumeUp",            AmpCommand(16, 16, 1)},
  {"VolumeDown",          AmpCommand(16, 17, 1)},
  {"MuteToggle",          AmpCommand(16, 13, 1)},
  {"InputPhono",          AmpCommand(21, 63, 1)},         
  {"InputCD",             AmpCommand(20, 63, 1)},
  {"InputTuner",          AmpCommand(17, 63, 1)},
  {"InputNetwork",        AmpCommand(25, 63, 10, 1)},
  {"InputRecorder",       AmpCommand(18, 63, 1)}, // not working
  {"InputCoaxial",        AmpCommand(16, 1, 42, 1)}, // not working
  {"InputOptical",        AmpCommand(16, 1, 40, 1)},         
  {"SourceDirectToggle",  AmpCommand(23, 63, 1)}, // not working
  {"PowerOn",             AmpCommand(16, 12, 1, 0)},  
  {"PowerOff",            AmpCommand(16, 12, 2, 0)}   
};

void sendAmpButton(const String& btnName) {
  auto it = ampButtons.find(btnName);
  if (it == ampButtons.end()) {
    Serial.printf("Unknown: %s\n", btnName.c_str());
    return;
  }

  const auto& cmd = it->second;
  if (cmd.extension == 0) {
    sendRC5(cmd.address, cmd.command, cmd.repeats);
    Serial.printf("Sent RC5: %s (addr=%d,cmd=%d,rep=%d)\n",
      btnName.c_str(), cmd.address, cmd.command, cmd.repeats);
  } else {
    sendRC5_X(cmd.address, cmd.command, cmd.extension, cmd.repeats);
    Serial.printf("Sent RC5Marantz: %s (addr=%d,cmd=%d,ext=%d,rep=%d)\n",
      btnName.c_str(), cmd.address, cmd.command, cmd.extension, cmd.repeats);
  }
}
