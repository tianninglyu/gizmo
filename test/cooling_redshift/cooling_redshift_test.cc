/* Link the actual cooling implementation (including its private table state)
 * against the normal GIZMO objects. No cooling formula is substituted. */
#include "../../cooling/cooling.cc"
#include <assert.h>

static void close_enough(double actual, double expected, double tol=2.e-6)
{
    if(!isfinite(actual) || fabs(actual-expected) > tol*DMAX(fabs(expected),1.e-50))
    {fprintf(stderr,"actual=%.17g expected=%.17g\n",actual,expected); abort();}
}

static double cell_cooling_rate(double logT, double nH)
{
    memset(CellP,0,sizeof(*CellP)); // each evaluation starts from the same gas state
    double rho=nH*PROTONMASS_CGS/HYDROGEN_MASSFRAC;
    CellP[0].Density=rho/(UNIT_DENSITY_IN_CGS*All.cf_a3inv);
    P[0].KernelRadius=1.e-3/All.cf_atime;
    P[0].Mass=CellP[0].Density*pow(P[0].KernelRadius,3);
    // Keep the *physical* velocity gradient fixed, including the Hubble term
    // used by the CO large-velocity-gradient cooling limiter.
    for(int k=0;k<3;k++) {CellP[0].Gradients.Velocity[k][k]=(1.-(All.ComovingIntegrationOn ? All.cf_hubble_a : 0.))/All.cf_a2inv;}
    CellP[0].MolecularMassFraction_perNeutralH=0.5;
    CellP[0].MolecularMassFraction=0.5;
    double ne=1.;
    return CoolingRate(logT,rho,ne,&ne,0);
}

int main(int argc, char **argv)
{
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&ThisTask);
    MPI_Comm_size(MPI_COMM_WORLD,&NTask);
    if(argc==3 && strcmp(argv[1],"--parameters")==0)
    {
        read_parameter_file(argv[2]);
        printf("PARAMETER_TEST CoolingRedshift=%.17g\n",All.CoolingRedshift);
        MPI_Finalize();
        return 0;
    }
    All.MaxMemSize=64;
    mymalloc_init();
    All.MinGasTemp=0;
    All.HubbleParam=1;
    All.OmegaMatter=0.3;
    All.OmegaLambda=0.7;
    All.UnitLength_in_cm=3.085678e21;
    All.UnitMass_in_g=1.989e43;
    All.UnitVelocity_in_cm_per_s=1.e5;
    set_units();
    All.SolarAbundances[0]=0.02;
    All.SolarAbundances[1]=0.28;
    for(int k=2;k<NUM_METAL_SPECIES;k++) {All.SolarAbundances[k]=0.001;}
    P=(decltype(P)) calloc(1,sizeof(*P));
    CellP=(decltype(CellP)) calloc(1,sizeof(*CellP));
    P[0].NumNgb=32;
    for(int k=0;k<NUM_METAL_SPECIES;k++) {P[0].Metallicity[k]=All.SolarAbundances[k];}
    All.Time=All.TimeBegin=0;
    All.CoolingRedshift=0;
    set_cosmo_factors_for_current_time();
    InitCoolMemory();
    MakeCoolingTable();
    ReadIonizeParams((char *)"TREECOOL");

    /* Fractional table bins, endpoints, high-z clamping, and reversed time order. */
    const double redshifts[]={0.,0.5,2.,8.,9.,12.,2.,0.};
    for(double z : redshifts)
    {
        All.ComovingIntegrationOn=0;
        All.Time=123.; // not TimeBegin: fixed-redshift loading must still work
        All.CoolingRedshift=z;
        set_cosmo_factors_for_current_time();
        close_enough(All.cf_atime,1.);
        close_enough(All.cf_a2inv,1.);
        close_enough(All.cf_a3inv,1.);
        close_enough(cooling_background_redshift(),z,1.e-14);
        IonizeParams();
        LoadMultiSpeciesTables();
        double x=DMAX(0.,DMIN(48.,48.*log10(1.+z)));
        assert(All.SpeciesTableInUse==(int)x);
        double Z[NUM_METAL_SPECIES];
        for(int k=0;k<NUM_METAL_SPECIES;k++) {Z[k]=All.SolarAbundances[k];}
        close_enough(GetCoolingRateWSpecies(1.e-3,5.,Z),
                     (NUM_LIVE_SPECIES_FOR_COOLTABLES-1)*(1.+x)*1.e-23/(0.0127/Z[0]));

        // Analytic CMB floor and Compton rate, independently of table interpolation.
        close_enough(get_background_radiation_temperature_for_emission_corrections(-1),2.73*(1.+z));
#if defined(RT_INFRARED) && defined(RT_ISRF_BACKGROUND)
        All.InterstellarRadiationFieldStrength=0.;
        All.InitRadiationTemp=20.;
        close_enough(get_min_allowed_dustIRrad_temperature(),2.73*(1.+z));
        close_enough(background_isrf_cmb_Teff(),2.73*(1.+z));
        double urad[N_RT_FREQ_BINS];
        get_background_isrf_urad(0,urad);
        close_enough(urad[RT_FREQ_BIN_INFRARED]*UNIT_PRESSURE_IN_CGS/ELECTRONVOLT_IN_ERGS,0.26*pow(1.+z,4));
#endif
        double compton=evaluate_Compton_heating_cooling_rate(-1,1.e4,1.,1.,0.);
        close_enough(compton,2.16e-35*0.262*pow(1.+z,4)*(1.e4-2.73*(1.+z)));

        // All cooling terms over a density/temperature grid at fixed physical rho.
        double rates[3][5];
        const double nH[]={1.e-4,1.,1.e3};
        const double logT[]={1.,2.,3.,5.,7.};
        for(int j=0;j<3;j++) for(int k=0;k<5;k++)
        {rates[j][k]=cell_cooling_rate(logT[k],nH[j]);}
        double uv=gJH0, metal=GetCoolingRateWSpecies(1.e-3,5.,Z);
        double dust=get_equilibrium_dust_temperature_estimate(-1,0.4,100.);

        // Simulate restart metadata disagreeing with the in-memory table cache.
        All.SpeciesTableInUse=48;
        LoadMultiSpeciesTables();
        close_enough(GetCoolingRateWSpecies(1.e-3,5.,Z),metal);
        for(int j=0;j<3;j++) for(int k=0;k<5;k++)
        {double repeated=cell_cooling_rate(logT[k],nH[j]);
         if(fabs(repeated-rates[j][k])>1.e-4*DMAX(fabs(rates[j][k]),1.e-50)) {fprintf(stderr,"REPEATED rate z=%g nH=%g logT=%g\n",z,nH[j],logT[k]);}
         close_enough(repeated,rates[j][k],1.e-4);}

        All.ComovingIntegrationOn=1;
        All.Time=1./(1.+z);
        All.CoolingRedshift=77.; // must be ignored in cosmological mode
        set_cosmo_factors_for_current_time();
        IonizeParams();
        LoadMultiSpeciesTables();
        close_enough(gJH0,uv);
        close_enough(GetCoolingRateWSpecies(1.e-3,5.,Z),metal);
        close_enough(get_equilibrium_dust_temperature_estimate(-1,0.4,100.),dust);
        close_enough(evaluate_Compton_heating_cooling_rate(-1,1.e4,1.,1.,0.),compton);
        for(int j=0;j<3;j++) for(int k=0;k<5;k++)
        {double value=cell_cooling_rate(logT[k],nH[j]);
         if(fabs(value-rates[j][k])>1.e-4*DMAX(fabs(rates[j][k]),1.e-50)) {fprintf(stderr,"rate comparison z=%g nH=%g logT=%g\n",z,nH[j],logT[k]);}
         // Single-precision cell state and iterative ionization introduce roundoff.
         close_enough(value,rates[j][k],1.e-4);}
    }

    // Synthetic nonlinear TREECOOL rows: check adjacent-row interpolation explicitly.
    All.ComovingIntegrationOn=0;
    All.CoolingRedshift=pow(10.,0.375)-1.;
    set_cosmo_factors_for_current_time();
    IonizeParams();
    close_enough(gJH0,pow(10.,(-12.+0.25+0.25*0.25-12.+0.5+0.5*0.5)/2.));
    All.CoolingRedshift=9.; // last UV row
    set_cosmo_factors_for_current_time(); IonizeParams();
    close_enough(gJH0,1.e-10);
    All.CoolingRedshift=12.; // above UV table: background off, CMB still present
    set_cosmo_factors_for_current_time(); IonizeParams();
    assert(gJH0==0.);
    close_enough(get_background_radiation_temperature_for_emission_corrections(-1),2.73*13.);
#ifdef IO_HDF5
    header.redshift=0.;
    hid_t file=H5Fcreate("header-test.hdf5",H5F_ACC_TRUNC,H5P_DEFAULT,H5P_DEFAULT);
    hid_t group=H5Gcreate(file,"Header",0);
    write_header_attributes_in_hdf5(group);
    hid_t attribute=H5Aopen_name(group,"CoolingRedshift");
    double saved_redshift=-1;
    H5Aread(attribute,H5T_NATIVE_DOUBLE,&saved_redshift);
    close_enough(saved_redshift,12.);
    H5Aclose(attribute);
    attribute=H5Aopen_name(group,"Redshift");
    H5Aread(attribute,H5T_NATIVE_DOUBLE,&saved_redshift);
    close_enough(saved_redshift,0.);
    H5Aclose(attribute); H5Gclose(group); H5Fclose(file);
#endif
    printf("PASS: fixed/cosmological cooling equality, metal interpolation, UV interpolation, CMB/Compton, dust, low-temperature rates, endpoints and cache recovery\n");
#if defined(RT_INFRARED) && defined(RT_ISRF_BACKGROUND)
    printf("PASS: explicit IR/CMB background energy, temperature and dust floor\n");
#endif
    MPI_Finalize();
}
