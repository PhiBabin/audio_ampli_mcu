#ifndef FLASH_STATE_GUARD_H_
#define FLASH_STATE_GUARD_H_

#include <optional>

struct State
{
  bool is_muted{false};
  uint32_t volume_db{0};
  AudioInput current_audio_input_;
};

class PersistentSate
{
public:
  std::optional<State> maybe_load_state();
  void update(const State curr_state);
  void force_save_state();

private:
  std::optional<unsigned long> maybe_millis_when_state_has_last_changed_;
  std::optional<State> maybe_state_;
};

#endif  // FLASH_STATE_GUARD_H_