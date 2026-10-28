# Fixed cooling-background redshift regression

Use a configuration containing `FIRE_PHYSICS_DEFAULTS=3` and `IO_HDF5`:

```sh
make CONFIG=/path/to/fire-config.sh test-cooling-redshift
```

Pass the same compiler, include and library overrides as for a normal build.
As with normal GIZMO builds, clean/regenerate the build when switching configurations.
The target links the real cooling implementation and normal GIZMO objects,
and runs in a temporary directory with synthetic TREECOOL and metal tables.
It requires Python 3 (standard library only), MPI and normal GIZMO build dependencies.
Synthetic tables are interpolation fixtures, not production cooling data.

Checks cover fixed z versus cosmological z at the same physical density over
15 density/temperature combinations at each of z=0, 0.5, 2, 8, 9 and 12;
metal table selection and interpolation, upper endpoint clamping, TREECOOL
adjacent-row interpolation, CMB temperature and Compton scaling, equilibrium
dust temperatures, low-temperature net rates and table cache recovery.
The gas states have matching physical density, size and velocity gradient;
the cosmological comparison includes the Hubble correction to the velocity
gradient used by the CO escape-probability limiter. Net rates agree to 1e-4
relative tolerance; individual background/table checks use 2e-6 or tighter.
The suite also checks parameter parsing, default z=0, rejection of negative
and non-finite values, and the HDF5 header attribute (with `IO_HDF5` enabled).
The CMB-only Compton assertion expects the FIRE radiative-feedback configuration.
With `RT_M1`, `RT_INFRARED` and `RT_ISRF_BACKGROUND` also enabled, the suite
checks the IR/CMB energy density, effective background temperature and dust floor.

CHIMES and Grackle need external libraries and data; this regression does not
validate those external solvers. Their redshift inputs are routed through the
same background accessor.

## Redshift audit

| Path | Source of background epoch |
| --- | --- |
| TREECOOL photoionization and heating; analytic alternative | `cooling_background_redshift()` |
| Metal-table allocation, selection, neighboring table and interpolation weight | `cooling_background_scale_factor()`; clamp to table bins 0..48 |
| Thermal Compton cooling, extragalactic UV/optical energy | background scale factor; CMB energy scales as a^-4 |
| Molecular/metal low-temperature emission correction and gas-dust coupling | common CMB/emission and equilibrium-dust helpers |
| IR radiation bath, dust solver floor and minimum dust temperature | common background scale factor |
| Cosmic-ray inverse-Compton radiation energy floor | common background scale factor |
| CHIMES CMB, UVB and equilibrium tables | global redshift/CMB set before initialization; tables refreshed outside particle-parallel loops |
| Grackle chemistry and tabulated cooling APIs | common background scale factor; physical unit conversions unchanged |
| Restart | parameter retained; actual metal-table cache tracked separately from restored `All` metadata |
| Snapshot | effective epoch in `Header/CoolingRedshift`; dynamical `Header/Redshift` unchanged |

No changes are made to `All.cf_*`, cosmic expansion, physical density/length
conversions, local stellar/AGN fluxes or ISRF strength. Existing mode-specific
IGM suppression of the assumed cosmic-ray background is unchanged.
