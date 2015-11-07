int mkdir_filename(const char *dir_name);
void *thread_datastore(void * p);

void saveit(double g);
off_t fsize(char *);

pthread_mutex_t a_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t got_request = PTHREAD_COND_INITIALIZER;
pthread_t  p_thread;       // thread's structure                    
pthread_attr_t p_attr;

//extern void get_uid_gid(const char *user, long *uid, long *gid);


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
//--------------------------------------------------------------------------------------------------
void *thread_datastore(void * p){ 
//--------------------------------------------------------------------------------------------------
    double g;
    int rc;
    sigset_t set;
    long tpid = (long)syscall(SYS_gettid);
    printf("data-storage thread started with pid: %ld\n",tpid);
    sigemptyset(&set);
    sigaddset(&set,SIGALRM);
    rc = pthread_sigmask(SIG_SETMASK, &set, NULL);
        PthreadCheck("pthread_sigmask", rc);

    long li=1;
    long *uid=&li, *gid=&li;
    get_uid_gid(user, uid, gid);
    setfsuid(*uid);
    setfsgid(*gid);



    pthread_mutex_lock(&a_mutex);
    while (done==0) {
        pthread_cond_wait(&got_request, &a_mutex);
        pthread_mutex_unlock(&a_mutex); // ??????????
        g = (dst.t1.tv_sec*1E9+dst.t1.tv_nsec)-(dst.t2.tv_sec*1E9+dst.t2.tv_nsec);
        saveit(g);
        if (auto_pga && pga_uf){    //write new pga in ini file 
            update_ini_file(ini_name);
            //printf("New PGA written in %s!\n",ini_name);
            pga_uf = 0; //clear update flag
        }
        pthread_mutex_lock(&a_mutex); // ????????

    }
    pthread_mutex_unlock(&a_mutex);
    pthread_exit(NULL);
}
#pragma GCC diagnostic pop

//----------------------------------------------------------------------
void saveit(double g){ // generate BINARY file: *.nc (netCDF)   
//----------------------------------------------------------------------
    static struct tm t1;
    int i;
    char str[500];
    char fn[255];
    int  ncid;  // netCDF id    
    int status;
    float d[MAX_SAMPL];
    static long filesize_c;
    long filesize;
    
    //t1 = *gmtime(&dst.t1.tv_sec); //first sample of batch
    sprintf(fn,"%s/tmp/%ld.nc",datafiledir,dst.t1.tv_sec);
    mkdir_filename(fn);
     
    status = nc_create(fn, NC_CLOBBER|NC_NETCDF4, &ncid);
    if (status != NC_NOERR){ //possible errors: NC_ENOMEM, NC_EHDFERR, NC_EFILEMETA
        logErrDate("savefile: Could not open file %s to write! nc_create: %s\nExit\n",fn,nc_strerror(status));
        exit(EXIT_FAILURE);
    }
    
    int ch1_dim;       // dimension id
    int ch1_id;        // variable id
    size_t s1[1] = {0}, s2[1] = {sampl};
    
    // define dimensions 
    status = nc_def_dim(ncid, "ch1", sampl, &ch1_dim);
    if (status != NC_NOERR){
        logErrDate("savefile, nc_def_dim param ch1: %s\nExit\n",nc_strerror(status));
        exit(EXIT_FAILURE);
    }
    #if (CFG_NR_CH > 1)
        int ch2_dim, ch2_id;
        status = nc_def_dim(ncid, "ch2", sampl, &ch2_dim);
        if (status != NC_NOERR){
            logErrDate("savefile, nc_def_dim param ch2: %s\nExit\n",nc_strerror(status));
            exit(EXIT_FAILURE);
        }
    #endif
    #if (CFG_NR_CH > 2)
        int ch3_dim, ch3_id;
        status = nc_def_dim(ncid, "ch3", sampl, &ch3_dim);
        if (status != NC_NOERR){
            logErrDate("savefile, nc_def_dim param ch3: %s\nExit\n",nc_strerror(status));
            exit(EXIT_FAILURE);
        }
    #endif
    #if (CFG_NR_CH > 3)
        int ch4_dim, ch4_id;
        status = nc_def_dim(ncid, "ch4", sampl, &ch4_dim);
        if (status != NC_NOERR){
            logErrDate("savefile, nc_def_dim param ch4: %s\nExit\n",nc_strerror(status));
            exit(EXIT_FAILURE);
        }
    #endif

    // define variables
    status = nc_def_var(ncid, "ch1", NC_FLOAT, 1, &ch1_dim, &ch1_id);
    if (status != NC_NOERR){ //  NC_BADID, NC_ENOTINDEFINE, NC_ESTRICTNC3, NC_MAX_VARS, NC_EBADTYPE, NC_EINVAL, NC_ENAMEINUSE, NC_EPERM
        logErrDate("savefile, nc_def_var param ch1: %s\nExit\n",nc_strerror(status));
        exit(EXIT_FAILURE);
    }
    #if (CFG_NR_CH > 1)    
        status = nc_def_var(ncid, "ch2", NC_FLOAT, 1, &ch2_dim, &ch2_id);
        if (status != NC_NOERR){
            logErrDate("savefile, nc_def_var param ch2: %s\nExit\n",nc_strerror(status));
            exit(EXIT_FAILURE);
        }
    #endif
    #if (CFG_NR_CH > 2)    
        status = nc_def_var(ncid, "ch3", NC_FLOAT, 1, &ch3_dim, &ch3_id);
        if (status != NC_NOERR){
            logErrDate("savefile, nc_def_var param ch3: %s\nExit\n",nc_strerror(status));
            exit(EXIT_FAILURE);
        }
    #endif
    #if (CFG_NR_CH > 3)    
        status = nc_def_var(ncid, "ch4", NC_FLOAT, 1, &ch4_dim, &ch4_id);
        if (status != NC_NOERR){
            logErrDate("savefile, nc_def_var param ch4: %s\nExit\n",nc_strerror(status));
            exit(EXIT_FAILURE);
        }
    #endif
    


    // assign global attributes
    sprintf(str,"%d-%02d-%02d %02d:%02d:%02d.%06ld",t1.tm_year+1900,t1.tm_mon+1, t1.tm_mday, t1.tm_hour,t1.tm_min,t1.tm_sec, dst.t1.tv_nsec/1000L);
    status = nc_put_att_text(ncid, NC_GLOBAL, "start", strlen(str), str);
    if (status != NC_NOERR){// NC_EINVAL, NC_ENOTVAR, NC_EBADTYPE, NC_ENOMEM, NC_EFILLVALUE
        logErrDate("savefile, nc_put_att_text param start: %s\nExit\n",nc_strerror(status));
        exit(EXIT_FAILURE);
    }
    status = nc_put_att_float(ncid, NC_GLOBAL, "sps", NC_FLOAT, 1, &sps);
    if (status != NC_NOERR){
        logErrDate("savefile, nc_put_att_float param sps: %s\nExit\n",nc_strerror(status));
        exit(EXIT_FAILURE);
    }


    // assign per-variable attributes
    status = nc_put_att_text(ncid, ch1_id, "units", 2, "mV");
    if (status != NC_NOERR){
        logErrDate("savefile, nc_put_att_text param units for ch1: %s\nExit\n",nc_strerror(status));
        exit(EXIT_FAILURE);
    }
    #if (CFG_NR_CH > 1)  
        status = nc_put_att_text(ncid, ch2_id, "units", 2, "mV");
        if (status != NC_NOERR){
            logErrDate("savefile, nc_put_att_text param units for ch2: %s\nExit\n",nc_strerror(status));
            exit(EXIT_FAILURE);
        }    
    #endif
    #if (CFG_NR_CH > 2)
        status = nc_put_att_text(ncid, ch3_id, "units", 2, "mV");
        if (status != NC_NOERR){
            logErrDate("savefile, nc_put_att_text param units for ch3: %s\nExit\n",nc_strerror(status));
            exit(EXIT_FAILURE);
        }    
    #endif
    #if (CFG_NR_CH > 3)  
        status = nc_put_att_text(ncid, ch4_id, "units", 2, "mV");
        if (status != NC_NOERR){
            logErrDate("savefile, nc_put_att_text param units for ch4: %s\nExit\n",nc_strerror(status));
            exit(EXIT_FAILURE);
        }    
    #endif
    
    status = nc_enddef (ncid);    // leave define mode
    if (status != NC_NOERR){
        logErrDate("savefile, nc_enddef: %s\nExit\n",nc_strerror(status));
        exit(EXIT_FAILURE);
    }

    // assign variable data
    for (i=0;i<sampl;i++)
        d[i] = dst.data[piter+i][0];
    status = nc_put_vara_float(ncid, ch1_id, s1, s2, d);
    if (status != NC_NOERR){//NC_EHDFERR, NC_ENOTVAR, NC_EINVALCOORDS, NC_EEDGE, NC_ERANGE, NC_EINDEFINE, NC_EBADID, NC_ECHAR, NC_ENOMEM, NC_EBADTYPE
        logErrDate("savefile, nc_put_vara_float param ch1: %s\nExit\n",nc_strerror(status));
        exit(EXIT_FAILURE);
    }
    
    #if (CFG_NR_CH > 1)
        for (i=0;i<sampl;i++)
            d[i] = dst.data[piter+i][1];
        status = nc_put_vara_float(ncid, ch2_id, s1, s2, d);
        if (status != NC_NOERR){
            logErrDate("savefile, nc_put_vara_float param ch2: %s\nExit\n",nc_strerror(status));
            exit(EXIT_FAILURE);
        }
    #endif
    #if (CFG_NR_CH > 2)
        for (i=0;i<sampl;i++)
            d[i] = dst.data[piter+i][2];
        status = nc_put_vara_float(ncid, ch3_id, s1, s2, d);
        if (status != NC_NOERR){
            logErrDate("savefile, nc_put_vara_float param ch3: %s\nExit\n",nc_strerror(status));
            exit(EXIT_FAILURE);
        }
    #endif
    #if (CFG_NR_CH > 3)
        for (i=0;i<sampl;i++)
            d[i] = dst.data[piter+i][3];
        status = nc_put_vara_float(ncid, ch4_id, s1, s2, d);
        if (status != NC_NOERR){
            logErrDate("savefile, nc_put_vara_float param ch4: %s\nExit\n",nc_strerror(status));
            exit(EXIT_FAILURE);
        }
    #endif

    status = nc_sync(ncid); //ensure it is written to disk        
    if (status != NC_NOERR){
        logErrDate("savefile, nc_sync %s\nExit\n",nc_strerror(status));
        exit(EXIT_FAILURE);
    }
    
    status = nc_close(ncid); //close file
    if (status != NC_NOERR){
        logErrDate("savefile, nc_close %s\nExit\n",nc_strerror(status));
        exit(EXIT_FAILURE);
    }
        //printf("%d-%02d-%02d %02d:%02d:%02d.%04ld  %.06g\n",t1.tm_year+1900,t1.tm_mon+1, t1.tm_mday, t1.tm_hour, t1.tm_min, t1.tm_sec, dst.ts[piter].tv_nsec/100000L, g/1E9);
    if (dst.t2.tv_sec == 0){
        filesize_c=(unsigned long)fsize(fn);
        printf("File written: %s.   (first file)        Size: %ldKiB\n",fn, (filesize_c)/1024);
    }
    else {
        filesize = (unsigned long)fsize(fn);
        printf("File written: %s. Ellapsed: %.6lfs. Size: %ldKiB\n",fn, g/1E9,(filesize)/1024);
        if (filesize != filesize_c){
            logErrDate("Error! Fize size not consistent!\nExit\n");
            exit(EXIT_FAILURE);
        }

    }
}



//--------------------------------------------------------------------------------------------------
int mkdir_filename(const char *dir_name){
//--------------------------------------------------------------------------------------------------
    struct stat st = {0};
    char dirname[255];
    strcpy(dirname,dir_name);
    
    uint h=1,i,j=0;
    char tmp[255];
    for(i=1; i<=strlen(dirname); i++){
        if(dirname[i]=='.')
            j=i;
        if(dirname[i]=='/' || (i==strlen(dirname) && j<h) ){
            h=i;
            snprintf(tmp, (size_t)(i+1), dirname);
            if (stat(tmp, &st) == -1) { //if dir does not exists, try to create it
                printf("Creating directory %s\n",tmp);
                if (mkdir(tmp, S_IRWXU | S_IRWXG | S_IRWXG | S_IXOTH)){
                    logErrDate("%s: could not create directory %s!\n",__func__,tmp);
                    //exit_all(-1);
                    return -1;
                }
            }       
        }
    } 
    return 0;
}

//--------------------------------------------------------------------------------------------------
off_t fsize(char *filename) {
//--------------------------------------------------------------------------------------------------
    struct stat st;

    if (stat(filename, &st) == 0)
        return st.st_size;

    logErrDate("Cannot determine size of %s\n", filename);

    return -1;
}


