# rnd Module

This is an example for a custom system module, meant to show off how to provide
multiple services in the same system module.

It also offers are vastly faster CSPRNG than PS\_GenerateRandomBytes, which
makes a round trip to Process9.

This uses the unused unique ID 0x25 and Result module code 88.

## Usage

See the client/ folder. `lcg.c` and `csg.c` also have extensive headers
documenting the required service call input and expected output.

## Building

`make`. A reasonably recent ctrulib that has to be built from git, rather than
taken from devkitARM as of the time of writing, is required.

## Licensing

See LICENSE.

## Source Tree

```
rnd
├── client (client for rnd:L and rnd:C)
│   ├── Makefile
│   ├── res
│   │   ├── (...)
│   │   └── cia.rsf
│   │         RSF file for building the CIA; if you modify the services
│   │         exposed by rnd, you'll also need to update the dependencies here.
│   └── source
│       └── main.c
│             Main source file, includes the RNDL and RNDC functions.
├── LICENSE
├── Makefile
│     Makefile for building rnd.
│     If you need to debug, you may wish to turn -O0 on.
│     If you build a system module for inclusion as Kernel11 built-in module,
│     add -nocodepadding to the makerom calls, especially the CIA target.
├── README.md
├── rnd.rsf
│     RSF file for rnd. You may want to adjust the unique ID here as not to
│     conflict with other homebrew services/impersonate a system module.
└── source
    ├── arc4random.c
    ├── arc4random.h
    ├── arc4random_uniform.c
    ├── chacha_private.h
    ├── explicit_bzero.c
    ├── explicit_bzero.h
    │     The above files are all related to csg.c, handling the CSPRNG
    │     imported from OpenBSD/libbsd.
    ├── csg.c
    ├── csg.h
    │     The above files handle rnd:C commands.
    ├── lcg.c
    ├── lcg.h
    │     The above files handle rnd:L commands.
    ├── ctrulib_hacks.c
    │     ctrulib overrides necessary to work as system module. MAY be
    │     insufficient for modules running as Kernel11 built-in modules.
    ├── main.c
    │     Contains main().
    │     Does almost nothing but set up ps:ps.
    ├── service.c
    ├── service.h
    │     The above files handle setting up the rnd:C/rnd:L services,
    │     listening for notification events, calling the appropriate handlers
    │     and replying to clients.
    │     This is the real meat of this package.
    ├── util.c
    └── util.h
          Small utility functions: panic() and write_invalid_arg_error().
```

## Known Issues

* The service CIA always rebuilds, even if the source files haven't changed.

