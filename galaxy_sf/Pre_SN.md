# Hu17 supernova pre-driving

Enable `Pre_SN` with `FIRE_PHYSICS_DEFAULTS=3` in Config.sh for an isolated
3D preparation run. Set `TimeMax` to the desired end time in code units.
Add these runtime parameters for a disc centered at the origin.

```text
PreSNCenterX 0
PreSNCenterY 0
PreSNSeed 42
```

The center uses code lengths and the disc axis is z. At each active gas
step, gas with cylindrical radius below 1 kpc, hydrogen number density
above 0.1 cm^-3, and temperature above 10^4 K is sampled with probability
min(1, 0.02 dt/t_ff). The seed, particle ID, and integer timestep determine
the random draw.

Each event sends 10^51 erg and up to 8.72 solar masses into the FIRE-3
mechanical coupling. The source must have enough gas to supply the ejecta.
The coupled mass is removed from the source, and the ejecta carry the
source's elemental abundances. The gas kernel supplies the source size.

Star formation and normal stellar feedback are disabled throughout the
preparation run. Cooling, gravity, and hydrodynamics continue. For
production, rebuild without `Pre_SN`, remove the three `PreSN` parameters,
and start from the prepared snapshot as initial conditions. Native restart
files are for continuation with the same build configuration.

The runtime log reports the eligible count, summed probability, triggered
and coupled event counts, and transferred mass.
