// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "../common/config.h"
#include <string>
namespace bubbsy {
// Owns the managed headless Everything child (or validates the user's existing one).
class EverythingManager {
public:
    explicit EverythingManager(const Settings& s) : cfg_(s) {}
    // Write our tuned ini (managed mode, first run only) and start the instance.
    // No-op start in "Existing" mode beyond verifying reachability.
    bool Start();
    // True if our target instance answers on the IPC pipe.
    bool Healthy() const;
    // Restart the managed child if it has died. Returns true if running afterwards.
    bool EnsureAlive();
    void Stop();  // stops only our managed child, never the user's own instance
private:
    void WriteTunedIni() const;   // 10GB cache, folder-size index, USN monitor
    Settings cfg_;
    PROCESS_INFORMATION child_{};
};
}  // namespace bubbsy
