# insomniad

insomniad is a daemon implementing Linux's autosleep mechanism in user space.
This allows additional policy to factor into the decision to suspend the system
beyond the kernel's built-in wakeup sources.

This is useful for AC powered devices where the overhead of entering suspend is
more influential than the power saved by going to sleep as soon as possible.

## Motivation

Linux's `/sys/power/autosleep` feature takes into account wakeup sources which
come from drivers in the kernel and user space wake locks. When no wakeup
sources are active, the system will immediately suspend.

It may be useful to take into account more factors than explicit locks and
events. For example, accounting time since the last wakeup source was active
(hysteresis) can help reduce thrashing on systems that are slow to suspend.
Without the ability to globally adjust the timeout, every wakeup source's
timeout would need to be modified. This isn't feasible as the wake locks and
wakeup sources are spread across various applications and drivers,
respectively.

## Features

* Configurable time since last event before initiating sleep
* systemd integration

## Mechanism

The mechanism to implement the policy in user space while still racelessly
accounting for kernel wakeup sources is provided by the
`/sys/power/wakeup_count` API. Read more at
[Documentation/ABI/testing/sysfs-power](https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-power)
