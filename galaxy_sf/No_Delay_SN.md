# Prompt supernovae during initial evolution

Enable `No_Delay_SN` alongside `FIRE_PHYSICS_DEFAULTS=3` in Config.sh
for an isolated run. `NoDelaySNDurationMyr` defaults to 250 and measures
the prompt window from `TimeBegin`. Set it to zero for ordinary feedback.
Star formation remains active. The macro is incompatible with `Pre_SN`.

Stars born in the window deliver their core-collapse budget on their first
active mechanical feedback step. With the FIRE-3 IMF sampling, this is the
number of massive stars drawn at birth from the new particle's mass.
The existing FIRE-3 mechanical coupling uses core-collapse ejecta
properties evaluated at a representative age of 10 Myr. For very small
star particles, prompt ejecta mass is capped at 90% of the stellar mass,
with the SN energy budget retained.

Stars born after the window use the normal FIRE-3 age-dependent rates.
Prompt populations do not draw a second core-collapse budget. Type Ia
events retain their usual delay distribution. The experiment in
[Hu et al.](https://arxiv.org/abs/2208.10528) instead used a fixed 10 Myr
delay after the switch and sampled one progenitor per 100 solar masses
on average.

Use the same executable to cross the window boundary. Continue interrupted
runs with native restart files, which retain the per-star prompt state.
Snapshots do not retain that state, so starting a new run from them can
repeat core-collapse feedback for young stars.
