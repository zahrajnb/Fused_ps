========================================================
Power modelling code from Fused
========================================================

This is just an extraction of the power modelling part of Fused, along with 
a very simple usage example.

The power model is based on recording high-level events (memory accesses,
peripheral operations etc.) and states (modules on/off, peripheral operation
modes etc.), and computing the instantaneous power consumption at runtime.

This is implemented as a custom SystemC channel, where module report states 
& events, and a "consumer" reads the instantaneous power consumption. The 
channel does the heavy lifting.

Each event & state is an object which implements a small number of methods, so 
they can model arbitrarily simple & complex events/states.

Directory structure
===================

+----------------------+-----------------------------------------------------+
| Directory            | Content                                             |
+======================+=====================================================+
| libs                 | Third-party libraries                               |
+----------------------+-----------------------------------------------------+
| ps                   | Power system/model                                  |
+----------------------+-----------------------------------------------------+
| test                 | tests                                               |
+----------------------+-----------------------------------------------------+
| cmake                | CMake utilities                                     |
+----------------------+-----------------------------------------------------+

Build
=====

On linux (tested on Ubuntu 20.04)
-----------------------------------------

First, install a few tools:

.. code-block:: bash

    $> sudo apt install libboost-dev build-essential g++ ninja-build git gdb \
       libncurses5 libncursesw5 libtinfo5 libpython2.7

Then install a recent version of *CMake* (>= version 3.13):

.. code-block:: bash

    $> wget https://github.com/Kitware/CMake/releases/download/v3.15.4/cmake-3.15.4-Linux-x86_64.sh
    $> chmod a+x cmake*.sh
    $> sudo ./cmake*.sh --skip-license --prefix=/usr/local

If you want to, you can now install the dependencies, using *CMake* (this may take a while):

.. code-block:: bash

    # in fused-ps/
    $> cmake -Bbuild -GNinja \
           -DINSTALL_DEPENDENCIES=ON \
           -DINSTALL_SYSTEMC=ON \
           -DINSTALL_SYSTEMCAMS=ON
    $> cd build
    $> ninja

By default, this installs to ``~/.local``, but you can provide a different
install path with the ``EP_INSTALL_DIR`` variable, e.g.
``-DEP_INSTALL_DIR=${HOME}/fused-deps``.

Now, to build examples & tests boards, disable ``INSTALL_DEPENDENCIES`` and rebuild:

.. code-block:: bash

    # in fused-ps/build
    $> cmake .. -GNinja -DINSTALL_DEPENDENCIES=OFF
    $> ninja

Then run the test(s) with:

.. code-block:: bash

    # in fused-ps/build
    $> ninja test

Basic usage
===========

Run the ``simple`` example.

.. code-block:: bash

    $>./examples/simple/simple

This will run the simple example, spam your console, and generate a few traces.
See ``examples/simple/main.cpp`` for some details.
The main files you get as outputs are: 
  - a ``.vcd`` file which traced the current draw.
  - a ``.csv`` file tracing the event rates over time.
  - a ``.csv`` file tracing the static power over time.
