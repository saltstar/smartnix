
#pragma once

#include <stdint.h>

#include <kernel/timer.h>
#include <zircon/syscalls/policy.h>
#include <zircon/types.h>

typedef uint64_t pol_cookie_t;

// JobPolicy is a value type that provides a space-efficient encoding of the policies defined in the
// policy.h public header.
//
// JobPolicy encodes two kinds of policy, basic and timer slack.
//
// Basic policy is logically an array of zx_policy_basic elements. For example:
//
//   zx_policy_basic policy[] = {
//      { ZX_POL_BAD_HANDLE, ZX_POL_ACTION_KILL },
//      { ZX_POL_NEW_CHANNEL, ZX_POL_ACTION_ALLOW },
//      { ZX_POL_NEW_FIFO, ZX_POL_ACTION_ALLOW | ZX_POL_ACTION_EXCEPTION },
//      { ZX_POL_VMAR_WX, ZX_POL_ACTION_DENY | ZX_POL_ACTION_KILL }}
//
// Timer slack policy defines the type and minimium amount of slack that will be applied to timer
// and deadline events.
class JobPolicy {
public:
    // Merge array |policy| of length |count| into this object.
    //
    // |mode| controls what happens when the policies in |policy| and this object intersect. |mode|
    // must be one of:
    //
    // ZX_JOB_POL_RELATIVE - Conflicting policies are ignored and will not cause the call to fail.
    //
    // ZX_JOB_POL_ABSOLUTE - If any of the policies in |policy| conflict with those in this object,
    //   the call will fail with an error and this object will not be modified.
    //
    zx_status_t AddBasicPolicy(uint32_t mode, const zx_policy_basic_t* policy, size_t count);

    // Returns the set of actions for the specified |condition|.
    //
    // If the |condition| is allowed, returns ZX_POL_ACTION_ALLOW, optionally ORed with
    // ZX_POL_ACTION_EXCEPTION.
    //
    // If the condition is not allowed, returns ZX_POL_ACTION_DENY, optionally ORed with zero or
    // more of ZX_POL_ACTION_EXCEPTION and ZX_POL_ACTION_KILL.
    //
    // This method asserts if |policy| is invalid, and returns ZX_POL_ACTION_DENY for all other
    // failure modes.
    uint32_t QueryBasicPolicy(uint32_t condition) const;

    // Sets the timer slack policy.
    //
    // |slack.amount| must be >= 0.
    void SetTimerSlack(TimerSlack slack);

    // Returns the timer slack policy.
    TimerSlack GetTimerSlack() const;

    bool operator==(const JobPolicy& rhs) const;
    bool operator!=(const JobPolicy& rhs) const;

private:
    // Remember, JobPolicy is a value type so think carefully before increasing its size.
    //
    // Const instances of JobPolicy must be immutable to ensure thread-safety.
    pol_cookie_t cookie_{};
    TimerSlack slack_{TimerSlack::none()};
};
