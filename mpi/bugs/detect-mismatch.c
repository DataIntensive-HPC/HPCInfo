#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <mpi.h>

static inline
bool string_match(char * string, char * substring)
{
    // this should never happen...
    if (0 == strlen(substring)) return false;

    char * pos = strstr(string, substring);
    return (pos != NULL);
}

int main(void)
{
  MPI_Init(NULL,NULL);

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  int incompatible = 0;

  // MPI standard consistency
  {
      int version = 0;
      int subversion = 0;
      MPI_Get_version(&version, &subversion);

      if ((version != MPI_VERSION) || (subversion != MPI_SUBVERSION)) {
          incompatible++;
          printf("The MPI (version,subversion) is not consistent:\n compiled: (%d,%d)\n runtime:  (%d,%d)\n",
                  MPI_VERSION, MPI_SUBVERSION, version, subversion);
      }
  }

  // MPI implementation consistency
  // We only attempt to detect Open-MPI, MPICH and Intel MPI.
  // Cray MPI and MVAPICH2 may detect as MPICH here.
  {
      bool ompi_compiled = false;
#if defined(OPEN_MPI)
      ompi_compiled = true;
#endif
      bool mpich_compiled = false;
#if defined(MPICH) || defined(MPICH_VERSION)
      mpich_compiled = true;
#endif
      bool intel_compiled = false;
#if defined(I_MPI_VERSION)
      intel_compiled = true;
#endif

      int resultlen = 0;
      char version[MPI_MAX_LIBRARY_VERSION_STRING+1] = {0};
      MPI_Get_library_version(version, &resultlen);
      if (rank == 0) {
          printf("MPI_Get_library_version = %s\n", version);
      }

      bool mpich_linked = string_match(version, "MPICH");
      bool intel_linked = string_match(version, "Intel");
      bool ompi_linked  = string_match(version, "Open");
      if (ompi_compiled && intel_linked) {
          incompatible++;
          printf("Program was compiled with Open-MPI, but runtime library is Intel MPI - this will not work!\n");
      }
      if (ompi_compiled && mpich_linked) {
          incompatible++;
          printf("Program was compiled with Open-MPI, but runtime library is MPICH - this will not work!\n");
      }
      if (mpich_compiled && ompi_linked) {
          incompatible++;
          printf("Program was compiled with Intel MPI, but runtime library is Open-MPI - this will not work!\n");
      }
      // MPICH and Intel MPI are ABI-compatible...

      // per https://www.open-mpi.org/faq/?category=running#mpi-environmental-variables, this works since v1.3
      char * ocws = getenv("OMPI_COMM_WORLD_SIZE");
      bool ompi_launched = (ocws != NULL);

      // MPICH and friends define these
      char * pmsz = getenv("PMI_SIZE");
      char * mlnr = getenv("MPI_LOCALNRANKS");
      bool mpich_launched = ((pmsz != NULL) || (mlnr != NULL));

      // Intel MPI defines this
      char * imin = getenv("I_MPI_INFO_NP");
      bool intel_launched = (imin != NULL);

      // check for compiled-launched compatibility
      if (ompi_compiled && intel_launched) {
          incompatible++;
          printf("Program was compiled with Open-MPI, but launched with Intel MPI mpirun - this will not work!\n");
      }
      if (ompi_compiled && mpich_launched) {
          incompatible++;
          printf("Program was compiled with Open-MPI, but launched with MPICH mpirun - this will not work!\n");
      }
      if (intel_compiled && ompi_launched) {
          incompatible++;
          printf("Program was compiled with Intel MPI, but launched with Open-MPI mpirun - this will not work!\n");
      }
      if (mpich_compiled && ompi_launched) {
          incompatible++;
          printf("Program was compiled with MPICH, but launched with Open-MPI mpirun - this will not work!\n");
      }

      // check for linked-launched compatibility
      if (ompi_linked && intel_launched) {
          incompatible++;
          printf("Program was linked with Open-MPI, but launched with Intel MPI mpirun - this will not work!\n");
      }
      if (ompi_linked && mpich_launched) {
          incompatible++;
          printf("Program was linked with Open-MPI, but launched with MPICH mpirun - this will not work!\n");
      }
      if (intel_linked && ompi_launched) {
          incompatible++;
          printf("Program was linked with Intel MPI, but launched with Open-MPI mpirun - this will not work!\n");
      }
      if (mpich_linked && ompi_launched) {
          incompatible++;
          printf("Program was linked with MPICH, but launched with Open-MPI mpirun - this will not work!\n");
      }
  }

  if (rank == 0) {
      if (incompatible) {
          printf("%d MPI-related incompatibilities were detected!\n", incompatible);
      }
  }

  MPI_Finalize();

  return 0;
}
