### Thermal feedback delay

With `GALSF_FB_THERMAL` enabled, the optional runtime parameter
`ThermalFBDelayMyr` sets the stellar age threshold in physical Myr. It defaults
to `5` when omitted, preserving the original AGORA timing. For prompt thermal
feedback during pre-evolution, add to the parameter file:

```text
ThermalFBDelayMyr 0
```

Values must be finite and nonnegative. Feedback occurs on the first active
feedback evaluation at or above this age, once per stellar particle. Changing
the delay does not reset particles that have already injected feedback. The
parameter can be changed when continuing from a restart; set it to `5` for the
normal-delay phase. Recompile after this source change. Existing binary restart
files from the old executable are incompatible with the changed global parameter
structure; use a snapshot restart when migrating from the old executable.

## Snapshot restart

New snapshots store `PartType4/ThermalFeedbackDone` (unsigned integer):
0 means feedback has not occurred; 1 means the one-shot feedback budget is spent.
Snapshot restart (`RestartFlag = 2`) restores spent particles without injecting
again, and preserves pending particles for the configured age threshold.
A positive transient event flag is saved as spent, matching binary restart's
one-shot semantics. Snapshots should be taken at normal simulation output points.

For HDF5 snapshot restart, set `ICFormat 3`, `SnapFormat 3`, and `InitCondFile`
to the snapshot path (without `.hdf5`), retain the original units and physics,
set the desired `TimeMax` and `ThermalFBDelayMyr`, and run:

```sh
mpirun -np 256 ./GIZMO relax.params 2
```

Old HDF5 snapshots containing stars but lacking `ThermalFeedbackDone` are
rejected: age alone cannot reconstruct feedback history after changing delays.
To migrate, rebuild with the same compile flags and load a valid binary restart,
then produce a new snapshot. This change adds no particle or global-parameter
fields and does not change binary restart layouts relative to the version with
`ThermalFBDelayMyr`. Preserve a separate backup of working checkpoints.
Legacy binary snapshots must be produced and read using the same new block layout;
prefer HDF5, whose missing state can be detected explicitly.
