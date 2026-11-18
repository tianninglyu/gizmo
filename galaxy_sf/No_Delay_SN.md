# Prompt supernovae during initial evolution

Enable `No_Delay_SN` alongside `FIRE_PHYSICS_DEFAULTS=2` or `3` in Config.sh
for an isolated run. `NoDelaySNDurationMyr` defaults to 250 and measures
the prompt window from `TimeBegin`. Set it to zero for ordinary feedback.
Star formation remains active. The macro is incompatible with `Pre_SN`.

Stars born in the window deliver their core-collapse budget on their first
active mechanical feedback step. With the FIRE-3 IMF sampling, this is the
number of massive stars drawn at birth from the new particle's mass.
For FIRE-2, the budget is sampled by rounding the expectation stochastically,
using the integrated native core-collapse rate of 0.0106022912 SN per solar
mass. Delayed core-collapse events are suppressed through 37.53 Myr for
FIRE-2 and 44 Myr for FIRE-3, with Type Ia events retained afterward.
The selected FIRE version's mechanical coupling uses core-collapse ejecta
properties evaluated at a representative age of 10 Myr. For very small
star particles, prompt ejecta mass is capped at 90% of the stellar mass,
with the SN energy budget retained.

Stars born after the window use the normal age-dependent rates of the selected FIRE version.
Prompt populations do not draw a second core-collapse budget. Type Ia
events retain their usual delay distribution. The experiment in
[Hu et al.](https://arxiv.org/abs/2208.10528) instead used a fixed 10 Myr
delay after the switch and sampled one progenitor per 100 solar masses
on average.

Use native restart files for a full continuation. Snapshots are written in
HDF5 (`SnapFormat 3`) and store the prompt state as
`PartType4/PromptSNState`: 0 for ordinary populations, 1 for a pending
prompt event, and 3 for a population whose prompt budget has been sampled.

For merger initial conditions, concatenate this field in the same order
as the Type 4 particles, along with `StellarFormationTime` and
`OStarNumber`. Keep stellar ages consistent with the merger clock. If
resetting time, add `merger_TimeBegin - source_snapshot_Time` to each
source's formation times. Keep the resulting stars as Type 4, use HDF5
(`ICFormat 3`), and keep `No_Delay_SN` compiled in with
`NoDelaySNDurationMyr 0` for normal feedback on new stars. Pending prompt
events still run and spent populations do not draw again. Type 4 particles
with prompt history are excluded from numerical particle merging.

Older snapshots without the field load with state 0. The code cannot
recover prompt feedback history that those files did not save.

For a FIRE-2 trial, specify `CritPhysDensity` explicitly in the parameter file.
The local FIRE-3 implementation also applies this density threshold; its
default is 1 cm^-3, compared with 100 cm^-3 for FIRE-2. Switching FIRE
versions also changes the star formation criteria and feedback prescriptions.
