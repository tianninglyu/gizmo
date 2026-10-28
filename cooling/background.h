#ifndef GIZMO_COOLING_BACKGROUND_H
#define GIZMO_COOLING_BACKGROUND_H

/* Background-only scale factor: never use this for density, velocity, time or
 * length conversions. All.cf_* must remain unity in non-comoving runs.
 * Include after allvars.h. RT-only builds retain their existing parameter. */
static inline double cooling_background_scale_factor(void)
{
    if(All.ComovingIntegrationOn) {return All.cf_atime;}
#ifdef COOLING
    return 1.0 / (1.0 + All.CoolingRedshift);
#elif defined(RT_ISRF_BACKGROUND)
    return 1.0 / (1.0 + All.RadiationBackgroundRedshift);
#else
    return 1.0;
#endif
}

static inline double cooling_background_redshift(void)
{
    return 1.0 / cooling_background_scale_factor() - 1.0;
}

#endif
