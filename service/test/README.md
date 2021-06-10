Testing for the GEOPM Service
=============================

This directory provides a set of integration test scripts showing
examples of interacting with the GEOPM service using the `geopmaccess`
and `geopmsession` command line tools.  These tests are written in
bash and serve the purpose of testing a fully installed GEOPM service
using only software from the service subdirectory of the GEOPM
reposistory.  The tests in this directory also show examples of how to
use each of the features provided by the service.  The tests each
begin with a long comment describing the feature under test, and
provide a form of tutorial for an end user learning how to interact
with the GEOPM service.


Build install and run
---------------------

The script below walks through the steps to test the GEOPM service on a
compute node on a SLURM development cluster.  The example includes creating an
allocation on a compute node on an ICX partition and running commands in a
sudo shell on the compute node.  These two new shells are donoted by
indentation in the script.

    # Get acccess to GEOPM_SOURCE environment variable
    source ~/.geopmrc

    # Make sure we compile libgeopmd.so with system compiler
    unset CC CXX

    # Checkout the geopm-service-sst-test branch
    cd $GEOPM_SOURCE
    git checkout geopm-service-sst-test

    # Get rid of all non-git files
    echo Warning: CLEANING EVERYTHING 1>&2
    sleep 1
    git clean -dfx

    # Create the GEOPM service RPMs in ~/rpmbuild/RPMS
    cd service
    ./autogen.sh
    ./configure
    make rpm

    # Create an allocation on an ICX node
    salloc -n1 -p icx

        # Within the shell on the compute node sudo into a root shell
        cd $GEOPM_SOURCE/service/test
        HOME_USER=${USER} sudo --preserve-env=HOME_USER su

            # Within the sudo shell load the msr module
            modprobe msr

            # Install the GEOPM service RPMs and start the service
            ./install_service.sh $(cat ../VERSION) ${HOME_USER}

            # Update the allowed lists to open all signals and controls to all users
            geopmaccess -a | geopmaccess -w
            geopmaccess -ac | geopmaccess -wc

            # Get out of the sudo shell
            exit

        # On the compute node check the sst priority
        ./test_sst_priority.sh

        # Exit from the compute node allocation
        exit


Where to find other tests
-------------------------

The tests in this directory do not use any of the tools provided by
`libgeopm` or `libgeopmpolicy`.  Integration tests for `libgeopm` and
`libgeopmpolicy` derived features are located in
`geopm/integration/test`.  Some of these tests may use the GEOPM
service on a system where it is required.

The unit tests for the C++ files in `geopm/service/src` are located in
`geopm/test` along with the unit tests for the files in `geopm/src`.
In the future it may make sense to split the unit tests in to two
directories so that the service subdirectory is fully independent.

The unit tests for the geopmdpy module are located in
`geopm/service/geopmdpy_test`.
