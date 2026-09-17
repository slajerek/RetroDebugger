make bindist, new approach - README
===================================


Introduction
------------

After some discussion on #vice-dev about perhaps merging the Windows
make-bindist scripts of the SDL and Gtk3 UIs I came up with the following.
It is still very much a work in progress, but I feel I'm on the track.


Goals
-----

* Reduce code duplication in the various make-bindist scripts
* Simplify the code, making it easier to follow and update
* Properly document the code
* Make it easily extensible for new ports (for example gtk4)
* (stretch goal) extend to MacOS, Haiku


-- Reduce code duplication and simplify some of the code

    A lot of code in the bindist scripts at least follows the same pattern:

    * set up a bindist dir, using UI name, svn (or git) revision and CPU-arch
    * copy emulator and tool binaries
    * strip binaries if requested
    * determine DLLs required and copy those
    * copy emulator-specific data (ie ROMs, keymaps) and remove unwanted files
    * copy documentation, including HTML if requested
    * copy any UI-specific data

    The new approach splits that into easy to understand functions, and where
    required will use UI/port-specific code and variables by sourcing
    UI-specific scripts.

-- Properly document the code

    This is a no-brainer. Shell scripts allow comments, so we should use them.
    Especially when using a lot of piping of commands. It only takes a few
    minutes to add documentation, and if you can't document it, it's probably
    too complicated. I haven't settled on a documentation style yet, but it'll
    be close to Doxygen.

-- Extensibility

    Using sourcing of scripts, we can implement some sort of 'plugins' or
    'interfaces'. Of course using shell scripts this means the sourced scripts
    define some vars and functions the main script uses.

    For example `find_dlls` is expected to be defined in the sourced script
    and set $BINDIST_DLLS, which will then be used in the main script to copy
    the DLLs to the subdir $BINDIST_DLL_DIR, also defined in the sourced script.

    Theoretically this should make writing a 'gtk4/make_bindist_win32_gtk4.sh'
    script fairly easy.
    

Sourcing scripts
----------------

In order to have a single entry point for `make bindist` the main script has
been placed in /src/arch/shared.

Any sourced script should reside in src/arch/$UI. So for SDL the sourced script
lives in `src/arch/sdl/make_bindist_win32_sdl.sh`. This is currently hardcoded
in the main script, but perhaps we can make that dynamic.


Sourced script API
------------------

The sourced scripts are expected to define a few things:

* bindist subdirectories
    
    This is an example of the SDL2 script:

    BINDIST_EXE_DIR="."
    BINDIST_DLL_DIR="."
    BINDIST_DOC_DIR="doc"
    BINDIST_DATA_DIR="."

    The main script will barf on empty/non-declared variables and also on '/'.

* functions

    archdep_find_dlls()

        This function is expected to determine the DLLs required. It depends
        on $BINDIST_DIR and $CROSS (set by the main script) and sets
        $BINDIST_DLLS. The entries in $BINDIST_DLLS are expected to be
        absolute paths in Unix format.
    
    archdep_clean_emu_data_dir()

        This function is used to remove files from an emulator-specific data
        directory after copying the entire data dir (for example ./data/C64).
        It's basically intended to remove non-port related files, for example
        removing GTK3 keymaps from an SDL[2] bindist.

    archdep_copy_docs()

        Copy UI-dependent documentation into the bindist.
        

Compyx, 2020-02-03

